/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* server.c
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Nat Friedman (nat@ximian.com)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* define this if you need/want to be able to send USR2 to server and
   get a list of the active backends */
/*#define DEBUG_BACKENDS*/

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <bonobo-activation/bonobo-activation.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-generic-factory.h>
#include <gconf/gconf-client.h>

#include <libebackend/e-data-server-module.h>
#include <libedata-book/e-data-book-factory.h>
#if ENABLE_CALENDAR
#include <libedata-cal/e-data-cal-factory.h>
#endif

#ifdef G_OS_WIN32
#include <libedataserver/e-data-server-util.h>
#endif

#include "server-interface-check.h"
#include "server-logging.h"
#include "offline-listener.h"

#define E_DATA_SERVER_INTERFACE_CHECK_OAF_ID "OAFIID:GNOME_Evolution_DataServer_InterfaceCheck"
#define E_DATA_SERVER_LOGGING_OAF_ID "OAFIID:GNOME_Evolution_DataServer_Logging"

#define E_DATA_CAL_FACTORY_OAF_ID "OAFIID:GNOME_Evolution_DataServer_CalFactory:" API_VERSION
#define E_DATA_BOOK_FACTORY_OAF_ID "OAFIID:GNOME_Evolution_DataServer_BookFactory:" API_VERSION

/* The and addressbook calendar factories */

#if ENABLE_CALENDAR
static EDataCalFactory *e_data_cal_factory;
#endif

static EDataBookFactory *e_data_book_factory;

/* The other interfaces we implement */

static ServerLogging *logging_iface;
static ServerInterfaceCheck *interface_check_iface;

/* Timeout interval in milliseconds for termination */
#define EXIT_TIMEOUT 5000

/* Timeout ID for termination handler */
static guint termination_handler_id;

static GStaticMutex termination_lock = G_STATIC_MUTEX_INIT;

/* Termination */

/* Termination handler.  Checks if both factories have zero running backends,
 * and if so terminates the program.
 */
static gboolean
termination_handler (gpointer data)
{
	gint count = 0;

#if ENABLE_CALENDAR
	count += e_data_cal_factory_get_n_backends (e_data_cal_factory);
#endif
	count += e_data_book_factory_get_n_backends (e_data_book_factory);

	if (count == 0) {
		g_message ("termination_handler(): Terminating the Server.  Have a nice day.");
		bonobo_main_quit ();
	}

	termination_handler_id = 0;
	return FALSE;
}

/* Queues a timeout for handling termination of Server */
static void
queue_termination (void)
{
	g_static_mutex_lock (&termination_lock);
	if (termination_handler_id)
		g_source_remove (termination_handler_id);

	termination_handler_id = g_timeout_add (EXIT_TIMEOUT, termination_handler, NULL);
	g_static_mutex_unlock (&termination_lock);
}



static void
last_book_gone_cb (EDataBookFactory *factory, gpointer data)
{
	queue_termination ();
}

static gboolean
setup_books (void)
{
	e_data_book_factory = e_data_book_factory_new ();

	if (!e_data_book_factory)
		return FALSE;

	e_data_book_factory_register_backends (e_data_book_factory);

	g_signal_connect (e_data_book_factory,
			  "last_book_gone",
			  G_CALLBACK (last_book_gone_cb),
			  NULL);

	if (!e_data_book_factory_activate (e_data_book_factory, E_DATA_BOOK_FACTORY_OAF_ID)) {
		bonobo_object_unref (BONOBO_OBJECT (e_data_book_factory));
		e_data_book_factory = NULL;
		return FALSE;
	}

	return TRUE;
}


/* Personal calendar server */

#if ENABLE_CALENDAR
/* Callback used when the calendar factory has no more running backends */
static void
last_calendar_gone_cb (EDataCalFactory *factory, gpointer data)
{
	queue_termination ();
}

/* Creates the calendar factory object and registers it */
static gboolean
setup_cals (void)
{
	e_data_cal_factory = e_data_cal_factory_new ();

	if (!e_data_cal_factory) {
		g_warning (G_STRLOC ": Could not create the calendar factory");
		return FALSE;
	}

	e_data_cal_factory_register_backends (e_data_cal_factory);

	if (!e_data_cal_factory_register_storage (e_data_cal_factory, E_DATA_CAL_FACTORY_OAF_ID)) {
		bonobo_object_unref (BONOBO_OBJECT (e_data_cal_factory));
		e_data_cal_factory = NULL;
		return FALSE;
	}

	g_signal_connect (G_OBJECT (e_data_cal_factory),
			  "last_calendar_gone",
			  G_CALLBACK (last_calendar_gone_cb),
			  NULL);

	return TRUE;

}
#else
static gboolean
setup_cals (void)
{
	return TRUE;
}
#endif


/* Logging iface.  */
static gboolean
setup_logging (void)
{
	gint result;

	logging_iface = server_logging_new ();

	server_logging_register_domain (logging_iface, NULL);
	server_logging_register_domain (logging_iface, "Gdk");
	server_logging_register_domain (logging_iface, "Gtk");
	server_logging_register_domain (logging_iface, "GdkPixbuf");
	server_logging_register_domain (logging_iface, "GLib");
	server_logging_register_domain (logging_iface, "GModule");
	server_logging_register_domain (logging_iface, "GLib-GObject");
	server_logging_register_domain (logging_iface, "GThread");

	server_logging_register_domain (logging_iface, "evolution-data-server");
	server_logging_register_domain (logging_iface, "libebookbackend");
	server_logging_register_domain (logging_iface, "libecalbackendfile");

	result = bonobo_activation_active_server_register (E_DATA_SERVER_LOGGING_OAF_ID,
							   BONOBO_OBJREF (logging_iface));

	return result == Bonobo_ACTIVATION_REG_SUCCESS;
}


/* Interface check iface.  */

static gboolean
setup_interface_check (void)
{
	gint result;

	interface_check_iface = server_interface_check_new ();
	result = bonobo_activation_active_server_register (E_DATA_SERVER_INTERFACE_CHECK_OAF_ID,
							   BONOBO_OBJREF (interface_check_iface));

	return result == Bonobo_ACTIVATION_REG_SUCCESS;
}

#ifdef DEBUG_BACKENDS
static void
dump_backends (gint signal)
{
	e_data_book_factory_dump_active_backends (e_data_book_factory);
#if ENABLE_CALENDAR
	e_data_cal_factory_dump_active_backends (e_data_cal_factory);
#endif
}
#endif

#ifdef G_OS_WIN32
#undef EVOLUTION_LOCALEDIR
#define EVOLUTION_LOCALEDIR e_util_get_localedir ()

/* Used in GNOME_PROGRAM_STANDARD_PROPERTIES: */
#undef PREFIX
#define PREFIX e_util_get_prefix ()

static const gchar *
sysconfdir (void)
{
	return e_util_replace_prefix (PREFIX,
				      e_util_get_prefix (),
				      SYSCONFDIR);
}
#undef SYSCONFDIR
#define SYSCONFDIR sysconfdir ()

static const gchar *
datadir (void)
{
	return e_util_replace_prefix (PREFIX,
				      e_util_get_prefix (),
				      DATADIR);
}
#undef DATADIR
#define DATADIR datadir ()

static const gchar *
libdir (void)
{
	return e_util_replace_prefix (PREFIX,
				      e_util_get_prefix (),
				      LIBDIR);
}
#undef LIBDIR
#define LIBDIR libdir ()

#endif

gint
main (gint argc, gchar **argv)
{
	gboolean did_books=FALSE, did_cals=FALSE;
	OfflineListener *offline_listener = NULL;

	bindtextdomain (GETTEXT_PACKAGE, EVOLUTION_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	printf ("evolution-data-server-Message: Starting server\n");

#ifdef DEBUG_BACKENDS
	signal (SIGUSR2, dump_backends);
#endif

	bonobo_init_full (&argc, argv,
			  bonobo_activation_orb_get(),
			  CORBA_OBJECT_NIL,
			  CORBA_OBJECT_NIL);

	e_data_server_module_init ();

	if (!( (did_books = setup_books ())
	       && (did_cals = setup_cals ())
		    )) {

		const gchar *failed = NULL;

		if (!did_books)
			failed = "BOOKS";
		else if (!did_cals)
			failed = "CALS";

		g_warning (G_STRLOC ": could not initialize Server service \"%s\"; terminating", failed);

		if (e_data_book_factory) {
			bonobo_object_unref (BONOBO_OBJECT (e_data_book_factory));
			e_data_book_factory = NULL;
		}

#if ENABLE_CALENDAR
		if (e_data_cal_factory) {
			bonobo_object_unref (BONOBO_OBJECT (e_data_cal_factory));
			e_data_cal_factory = NULL;
		}
#endif
		exit (EXIT_FAILURE);
	}

#if ENABLE_CALENDAR
	offline_listener = offline_listener_new (e_data_book_factory, e_data_cal_factory);
#else
	offline_listener = offline_listener_new (e_data_book_factory);
#endif

	if ( setup_logging ()) {
			if ( setup_interface_check ()) {
				g_message ("Server up and running");

				bonobo_main ();
			} else
				g_error (G_STRLOC "Cannot register DataServer::InterfaceCheck object");
	} else
		g_error (G_STRLOC "Cannot register DataServer::Logging object");

	g_object_unref (offline_listener);

#if ENABLE_CALENDAR
	bonobo_object_unref (BONOBO_OBJECT (e_data_cal_factory));
	e_data_cal_factory = NULL;
#endif

	bonobo_object_unref (BONOBO_OBJECT (e_data_book_factory));
	e_data_book_factory = NULL;

	bonobo_object_unref (BONOBO_OBJECT (logging_iface));
	logging_iface = NULL;

	bonobo_object_unref (BONOBO_OBJECT (interface_check_iface));
	interface_check_iface = NULL;

	return 0;
}
