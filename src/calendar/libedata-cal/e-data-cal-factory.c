/* Evolution calendar factory
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 * Authors:
 *   Federico Mena-Quintero <federico@ximian.com>
 *   JP Rosevear <jpr@ximian.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <bonobo-activation/bonobo-activation.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-main.h>
#include <string.h>
#include "libedataserver/e-url.h"
#include "libedataserver/e-source.h"
#include "libedataserver/e-source-list.h"
#include "libebackend/e-data-server-module.h"
#include "libecal/e-cal.h"
#include "e-cal-backend.h"
#include "e-data-cal.h"
#include "e-data-cal-factory.h"

#define PARENT_TYPE                BONOBO_TYPE_OBJECT
#define DEFAULT_E_DATA_CAL_FACTORY_OAF_ID "OAFIID:GNOME_Evolution_DataServer_CalFactory:" BASE_VERSION

static BonoboObjectClass *parent_class;

/* Private part of the CalFactory structure */
struct _EDataCalFactoryPrivate {
	/* Hash table from URI method strings to GType * for backend class types */
	GHashTable *methods;

	/* Hash table from GnomeVFSURI structures to CalBackend objects */
	GHashTable *backends;
	/* mutex to access backends hash table */
	GMutex *backends_mutex;

	/* OAFIID of the factory */
	gchar *iid;

	/* Whether we have been registered with OAF yet */
	guint registered : 1;

        gint mode;
};

/* Signal IDs */
enum SIGNALS {
	LAST_CALENDAR_GONE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/* Opening calendars */

static icalcomponent_kind
calobjtype_to_icalkind (const GNOME_Evolution_Calendar_CalObjType type)
{
	switch (type) {
	case GNOME_Evolution_Calendar_TYPE_EVENT:
		return ICAL_VEVENT_COMPONENT;
	case GNOME_Evolution_Calendar_TYPE_TODO:
		return ICAL_VTODO_COMPONENT;
	case GNOME_Evolution_Calendar_TYPE_JOURNAL:
		return ICAL_VJOURNAL_COMPONENT;
	}

	return ICAL_NO_COMPONENT;
}

static void
update_source_in_backend (ECalBackend *backend, ESource *updated_source)
{
	xmlNodePtr xml;

	g_return_if_fail (backend != NULL);
	g_return_if_fail (updated_source != NULL);

	xml = xmlNewNode (NULL, (const xmlChar *)"dummy");
	e_source_dump_to_xml_node (updated_source, xml);
	e_source_update_from_xml_node (e_cal_backend_get_source (backend), xml->children, NULL);
	xmlFreeNode (xml);
}

static ECalBackendFactory*
get_backend_factory (GHashTable *methods, const gchar *method, icalcomponent_kind kind)
{
	GHashTable *kinds;
	ECalBackendFactory *factory;

	kinds = g_hash_table_lookup (methods, method);
	if (!kinds) {
		return NULL;
	}

	factory = g_hash_table_lookup (kinds, GINT_TO_POINTER (kind));

	return factory;
}

/* Callback used when a backend loses its last connected client */
static void
backend_last_client_gone_cb (ECalBackend *backend, gpointer data)
{
	EDataCalFactory *factory;
	EDataCalFactoryPrivate *priv;
	ECalBackend *ret_backend;
	ESource *source;
	gchar *uid_type_string;
	gboolean last_calendar;

	fprintf (stderr, "backend_last_client_gone_cb() called!\n");

	factory = E_DATA_CAL_FACTORY (data);
	priv = factory->priv;

	/* Remove the backend from the hash table */

	source = e_cal_backend_get_source (backend);
	g_assert (source != NULL);
	uid_type_string = g_strdup_printf ("%s:%d", e_source_peek_uid (source), (gint)e_cal_backend_get_kind (backend));

	g_mutex_lock (priv->backends_mutex);

	ret_backend = g_hash_table_lookup (priv->backends, uid_type_string);
	g_assert (ret_backend != NULL);
	g_assert (ret_backend == backend);

	g_hash_table_remove (priv->backends, uid_type_string);
	g_free (uid_type_string);

	g_signal_handlers_disconnect_matched (backend, G_SIGNAL_MATCH_DATA,
					      0, 0, NULL, NULL, data);

	last_calendar = (g_hash_table_size (priv->backends) == 0);

	g_mutex_unlock (priv->backends_mutex);

	/* Notify upstream if there are no more backends */
	if (last_calendar)
		g_signal_emit (G_OBJECT (factory), signals[LAST_CALENDAR_GONE], 0);
}

struct find_backend_data
{
	const gchar *str_uri;
	ECalBackend *backend;
	icalcomponent_kind kind;
};

static void
find_backend_cb (gpointer key, gpointer value, gpointer data)
{
	struct find_backend_data *fbd = data;

	if (fbd && fbd->str_uri && !fbd->backend) {
		ECalBackend *backend = value;
		gchar *str_uri;

		str_uri = e_source_get_uri (e_cal_backend_get_source (backend));

		if (str_uri && g_str_equal (str_uri, fbd->str_uri)) {
			const gchar *uid_kind = key, *pos;

			pos = strrchr (uid_kind, ':');
			if (pos && atoi (pos + 1) == fbd->kind)
				fbd->backend = backend;
		}

		g_free (str_uri);
	}
}



static GNOME_Evolution_Calendar_Cal
impl_CalFactory_getCal (PortableServer_Servant servant,
			const CORBA_char *source_xml,
			const GNOME_Evolution_Calendar_CalObjType type,
			const GNOME_Evolution_Calendar_CalListener listener,
			CORBA_Environment *ev)
{
	GNOME_Evolution_Calendar_Cal ret_cal = CORBA_OBJECT_NIL;
	EDataCalFactory *factory;
	EDataCalFactoryPrivate *priv;
	EDataCal *cal = CORBA_OBJECT_NIL;
	ECalBackend *backend;
	ECalBackendFactory *backend_factory;
	ESource *source;
	gchar *str_uri;
	EUri *uri;
	gchar *uid_type_string;

	factory = E_DATA_CAL_FACTORY (bonobo_object_from_servant (servant));
	priv = factory->priv;

	source = e_source_new_from_standalone_xml (source_xml);
	if (!source) {
		bonobo_exception_set (ev, ex_GNOME_Evolution_Calendar_CalFactory_InvalidURI);

		return CORBA_OBJECT_NIL;
	}

	/* Get the URI so we can extract the protocol */
	str_uri = e_source_get_uri (source);
	if (!str_uri) {
		g_object_unref (source);
		bonobo_exception_set (ev, ex_GNOME_Evolution_Calendar_CalFactory_InvalidURI);

		return CORBA_OBJECT_NIL;
	}

	/* Parse the uri */
	uri = e_uri_new (str_uri);
	if (!uri) {
		bonobo_exception_set (ev, ex_GNOME_Evolution_Calendar_CalFactory_InvalidURI);

		return CORBA_OBJECT_NIL;
	}

	uid_type_string = g_strdup_printf ("%s:%d", e_source_peek_uid (source), (gint)calobjtype_to_icalkind (type));

	/* Find the associated backend factory (if any) */
	backend_factory = get_backend_factory (priv->methods, uri->protocol, calobjtype_to_icalkind (type));
	if (!backend_factory) {
		/* FIXME Distinguish between method and kind failures? */
		bonobo_exception_set (ev, ex_GNOME_Evolution_Calendar_CalFactory_UnsupportedMethod);
		g_free (str_uri);
		goto cleanup2;
	}

	g_mutex_lock (priv->backends_mutex);

	/* Look for an existing backend */
	backend = g_hash_table_lookup (factory->priv->backends, uid_type_string);

	if (!backend) {
		/* find backend by URL, if opened, thus functions like e_cal_system_new_* will not
		   create new backends for the same url */
		struct find_backend_data fbd;

		fbd.str_uri = str_uri;
		fbd.kind = calobjtype_to_icalkind (type);
		fbd.backend = NULL;

		g_hash_table_foreach (priv->backends, find_backend_cb, &fbd);

		if (fbd.backend) {
			backend = fbd.backend;
			g_object_unref (source);
			source = g_object_ref (e_cal_backend_get_source (backend));
		}
	}

	g_free (str_uri);

	if (!backend) {
		/* There was no existing backend, create a new one */
		if (E_IS_CAL_BACKEND_LOADER_FACTORY (backend_factory)) {
			backend = E_CAL_BACKEND_LOADER_FACTORY_GET_CLASS (backend_factory)->new_backend_with_protocol ((ECalBackendLoaderFactory *)backend_factory,
					source, uri->protocol);
		} else
			backend = e_cal_backend_factory_new_backend (backend_factory, source);

		if (!backend) {
			g_warning (G_STRLOC ": could not instantiate backend");
			bonobo_exception_set (ev, ex_GNOME_Evolution_Calendar_CalFactory_UnsupportedMethod);
			goto cleanup;
		}

		/* Track the backend */
		g_hash_table_insert (priv->backends, g_strdup (uid_type_string), backend);

		g_signal_connect (G_OBJECT (backend), "last_client_gone",
				  G_CALLBACK (backend_last_client_gone_cb),
				  factory);
	} else if (!e_source_equal (source, e_cal_backend_get_source (backend))) {
		/* source changed, update it in a backend */
		update_source_in_backend (backend, source);
	}

	/* Create the corba calendar */
	cal = e_data_cal_new (backend, listener);
	if (cal) {
		/* Let the backend know about its clients corba clients */
		e_cal_backend_add_client (backend, cal);
		e_cal_backend_set_mode (backend, priv->mode);
		ret_cal = bonobo_object_corba_objref (BONOBO_OBJECT (cal));
	} else {
		g_warning (G_STRLOC ": could not create the corba calendar");
		bonobo_exception_set (ev, ex_GNOME_Evolution_Calendar_CalFactory_UnsupportedMethod);
	}

 cleanup:
	/* The reason why the lock is held for such a long time is that there is
	   a subtle race where e_cal_backend_add_client() can be called just
	   before e_cal_backend_finalize() is called from the
	   backend_last_client_gone_cb(), for details see bug 506457. */
	g_mutex_unlock (priv->backends_mutex);
 cleanup2:
	e_uri_free (uri);
	g_free (uid_type_string);
	g_object_unref (source);

	return ret_cal;
}

/**
 * e_data_cal_factory_new:
 * @void:
 *
 * Creates a new #EDataCalFactory object.
 *
 * Return value: A newly-created #EDataCalFactory, or NULL if its corresponding CORBA
 * object could not be created.
 **/
EDataCalFactory *
e_data_cal_factory_new (void)
{
	EDataCalFactory *factory;

	factory = g_object_new (E_TYPE_DATA_CAL_FACTORY,
				"poa", bonobo_poa_get_threaded (ORBIT_THREAD_HINT_PER_REQUEST, NULL),
				NULL);

	return factory;
}

/* Destroy handler for the calendar */
static void
e_data_cal_factory_finalize (GObject *object)
{
	EDataCalFactory *factory;
	EDataCalFactoryPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (E_IS_DATA_CAL_FACTORY (object));

	factory = E_DATA_CAL_FACTORY (object);
	priv = factory->priv;

	g_hash_table_destroy (priv->methods);
	priv->methods = NULL;

	/* Should we assert that there are no more backends? */
	g_hash_table_destroy (priv->backends);
	g_mutex_free (priv->backends_mutex);
	priv->backends = NULL;

	if (priv->registered) {
		bonobo_activation_active_server_unregister (priv->iid, BONOBO_OBJREF (factory));
		priv->registered = FALSE;
	}
	g_free (priv->iid);

	g_free (priv);
	factory->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/* Class initialization function for the calendar factory */
static void
e_data_cal_factory_class_init (EDataCalFactoryClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_GNOME_Evolution_Calendar_CalFactory__epv *epv = &klass->epv;

	parent_class = g_type_class_peek_parent (klass);

	signals[LAST_CALENDAR_GONE] =
		g_signal_new ("last_calendar_gone",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EDataCalFactoryClass, last_calendar_gone),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* Class method overrides */
	object_class->finalize = e_data_cal_factory_finalize;

	/* Epv methods */
	epv->getCal = impl_CalFactory_getCal;
}

static void
set_backend_online_status (gpointer key, gpointer value, gpointer data)
{
	ECalBackend *backend = E_CAL_BACKEND (value);

	e_cal_backend_set_mode (backend,  GPOINTER_TO_INT (data));
}

/**
 * e_data_cal_factory_set_backend_mode:
 * @factory: A calendar factory.
 * @mode: Online mode to set.
 *
 * Sets the online mode for all backends created by the given factory.
 */
void
e_data_cal_factory_set_backend_mode (EDataCalFactory *factory, gint mode)
{
	EDataCalFactoryPrivate *priv = factory->priv;

	priv->mode = mode;
	g_mutex_lock (priv->backends_mutex);
	g_hash_table_foreach (priv->backends, set_backend_online_status, GINT_TO_POINTER (priv->mode));
	g_mutex_unlock (priv->backends_mutex);
}

/* Object initialization function for the calendar factory */
static void
e_data_cal_factory_init (EDataCalFactory *factory, EDataCalFactoryClass *klass)
{
	EDataCalFactoryPrivate *priv;

	priv = g_new0 (EDataCalFactoryPrivate, 1);
	factory->priv = priv;

	priv->methods = g_hash_table_new_full (g_str_hash, g_str_equal,
					       (GDestroyNotify) g_free, (GDestroyNotify) g_hash_table_destroy);
	priv->backends = g_hash_table_new_full (g_str_hash, g_str_equal,
						(GDestroyNotify) g_free, (GDestroyNotify) g_object_unref);
	priv->registered = FALSE;
	priv->backends_mutex = g_mutex_new ();
}

BONOBO_TYPE_FUNC_FULL (EDataCalFactory,
		       GNOME_Evolution_Calendar_CalFactory,
		       PARENT_TYPE,
		       e_data_cal_factory)

/**
 * e_data_cal_factory_register_storage:
 * @factory: A calendar factory.
 * @iid: OAFIID for the factory to be registered.
 *
 * Registers a calendar factory with the OAF object activation daemon.  This
 * function must be called before any clients can activate the factory.
 *
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
e_data_cal_factory_register_storage (EDataCalFactory *factory, const gchar *iid)
{
	EDataCalFactoryPrivate *priv;
	Bonobo_RegistrationResult result;
	gchar *tmp_iid;

	g_return_val_if_fail (factory != NULL, FALSE);
	g_return_val_if_fail (E_IS_DATA_CAL_FACTORY (factory), FALSE);

	priv = factory->priv;

	g_return_val_if_fail (!priv->registered, FALSE);

	/* if iid is NULL, use the default factory OAFIID */
	if (iid)
		tmp_iid = g_strdup (iid);
	else
		tmp_iid = g_strdup (DEFAULT_E_DATA_CAL_FACTORY_OAF_ID);

	result = bonobo_activation_active_server_register (tmp_iid, BONOBO_OBJREF (factory));

	switch (result) {
	case Bonobo_ACTIVATION_REG_SUCCESS:
		priv->registered = TRUE;
		priv->iid = tmp_iid;
		return TRUE;

	case Bonobo_ACTIVATION_REG_NOT_LISTED:
		g_warning (G_STRLOC ": cannot register the calendar factory %s (not listed)", tmp_iid);
		break;

	case Bonobo_ACTIVATION_REG_ALREADY_ACTIVE:
		g_warning (G_STRLOC ": cannot register the calendar factory (already active)");
		break;

	case Bonobo_ACTIVATION_REG_ERROR:
	default:
		g_warning (G_STRLOC ": cannot register the calendar factory (generic error)");
		break;
	}

	g_free (tmp_iid);

	return FALSE;
}

/**
 * e_data_cal_factory_register_backend:
 * @factory: A calendar factory.
 * @backend_factory: The object responsible for creating backends.
 *
 * Registers an #ECalBackend subclass that will be used to handle URIs
 * with a particular method.  When the factory is asked to open a
 * particular URI, it will look in its list of registered methods and
 * create a backend of the appropriate type.
 **/
void
e_data_cal_factory_register_backend (EDataCalFactory *factory, ECalBackendFactory *backend_factory)
{
	EDataCalFactoryPrivate *priv;
	const gchar *method;
	GHashTable *kinds;
	GType type;
	icalcomponent_kind kind;
	GSList *methods = NULL, *l;

	g_return_if_fail (factory && E_IS_DATA_CAL_FACTORY (factory));
	g_return_if_fail (backend_factory && E_IS_CAL_BACKEND_FACTORY (backend_factory));

	priv = factory->priv;

	if (E_IS_CAL_BACKEND_LOADER_FACTORY (backend_factory)) {
		GSList *list = E_CAL_BACKEND_LOADER_FACTORY_GET_CLASS (backend_factory)->get_protocol_list ((ECalBackendLoaderFactory *) backend_factory);
		methods = g_slist_copy (list);
	} else if (E_CAL_BACKEND_FACTORY_GET_CLASS (backend_factory)->get_protocol) {
		method = E_CAL_BACKEND_FACTORY_GET_CLASS (backend_factory)->get_protocol (backend_factory);
		methods = g_slist_append (methods, (gpointer) method);
	} else {
		g_assert_not_reached ();
		return;
	}

	kind = E_CAL_BACKEND_FACTORY_GET_CLASS (backend_factory)->get_kind (backend_factory);

	for (l= methods; l != NULL; l = g_slist_next (l)) {
		gchar *method_str;

		method = l->data;

		method_str = g_ascii_strdown (method, -1);

		kinds = g_hash_table_lookup (priv->methods, method_str);
		if (kinds) {
			type = GPOINTER_TO_INT (g_hash_table_lookup (kinds, GINT_TO_POINTER (kind)));
			if (type) {
				g_warning (G_STRLOC ": method `%s' already registered", method_str);
				g_free (method_str);
				g_slist_free (methods);
				return;
			}

			g_free (method_str);
		} else {
			kinds = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
			g_hash_table_insert (priv->methods, method_str, kinds);
		}

		g_hash_table_insert (kinds, GINT_TO_POINTER (kind), backend_factory);
	}
	g_slist_free (methods);
}

/**
 * e_data_cal_factory_register_backends:
 * @cal_factory: A calendar factory.
 *
 * Register all backends for the given factory.
 */
void
e_data_cal_factory_register_backends (EDataCalFactory *cal_factory)
{
	GList *factories, *f;

	factories = e_data_server_get_extensions_for_type (E_TYPE_CAL_BACKEND_FACTORY);

	for (f = factories; f; f = f->next) {
		ECalBackendFactory *backend_factory = f->data;

		e_data_cal_factory_register_backend (cal_factory, g_object_ref (backend_factory));
	}

	e_data_server_extension_list_free (factories);
}

/**
 * e_data_cal_factory_get_n_backends
 * @factory: A calendar factory.
 *
 * Get the number of backends currently active in the given factory.
 *
 * Returns: the number of backends.
 */
gint
e_data_cal_factory_get_n_backends (EDataCalFactory *factory)
{
	EDataCalFactoryPrivate *priv;
	gint sz;

	g_return_val_if_fail (E_IS_DATA_CAL_FACTORY (factory), 0);

	priv = factory->priv;
	g_mutex_lock (priv->backends_mutex);
	sz = g_hash_table_size (priv->backends);
	g_mutex_unlock (priv->backends_mutex);

	return sz;
}

/* Frees a uri/backend pair from the backends hash table */
static void
dump_backend (gpointer key, gpointer value, gpointer data)
{
	gchar *uri;
	ECalBackend *backend;

	uri = key;
	backend = value;

	g_message ("  %s: %p", uri, (gpointer) backend);
}

/**
 * e_data_cal_factory_dump_active_backends:
 * @factory: A calendar factory.
 *
 * Dumps to standard output a list of all active backends for the given
 * factory.
 */
void
e_data_cal_factory_dump_active_backends (EDataCalFactory *factory)
{
	EDataCalFactoryPrivate *priv;

	g_message ("Active PCS backends");

	priv = factory->priv;
	g_mutex_lock (priv->backends_mutex);
	g_hash_table_foreach (priv->backends, dump_backend, NULL);
	g_mutex_unlock (priv->backends_mutex);
}
