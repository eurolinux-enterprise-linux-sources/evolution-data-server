/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Michael Zucchi <notzed@ximian.com>
 */

#if !defined (__CAMEL_H_INSIDE__) && !defined (CAMEL_COMPILATION)
#error "Only <camel/camel.h> can be included directly."
#endif

#ifndef CAMEL_FOLDER_SEARCH_H
#define CAMEL_FOLDER_SEARCH_H

#include <camel/camel-folder.h>
#include <camel/camel-index.h>
#include <camel/camel-sexp.h>

/* Standard GObject macros */
#define CAMEL_TYPE_FOLDER_SEARCH \
	(camel_folder_search_get_type ())
#define CAMEL_FOLDER_SEARCH(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), CAMEL_TYPE_FOLDER_SEARCH, CamelFolderSearch))
#define CAMEL_FOLDER_SEARCH_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), CAMEL_TYPE_FOLDER_SEARCH, CamelFolderSearchClass))
#define CAMEL_IS_FOLDER_SEARCH(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), CAMEL_TYPE_FOLDER_SEARCH))
#define CAMEL_IS_FOLDER_SEARCH_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), CAMEL_TYPE_FOLDER_SEARCH))
#define CAMEL_FOLDER_SEARCH_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), CAMEL_TYPE_FOLDER_SEARCH, CamelFolderSearchClass))

G_BEGIN_DECLS

typedef struct _CamelFolderSearch CamelFolderSearch;
typedef struct _CamelFolderSearchClass CamelFolderSearchClass;
typedef struct _CamelFolderSearchPrivate CamelFolderSearchPrivate;

struct _CamelFolderSearch {
	GObject parent;
	CamelFolderSearchPrivate *priv;

	CamelSExp *sexp;		/* s-exp evaluator */
	gchar *last_search;	/* last searched expression */

	/* these are only valid during the search, and are reset afterwards */
	CamelFolder *folder;	/* folder for current search */
	GPtrArray *summary;	/* summary array for current search */
	GPtrArray *summary_set;	/* subset of summary to actually include in search */
	CamelMessageInfo *current; /* current message info, when searching one by one */
	CamelMimeMessage *current_message; /* cache of current message, if required */
	CamelIndex *body_index;
};

struct _CamelFolderSearchClass {
	GObjectClass parent_class;

	/* General bool/comparison options.  Usually these won't need
	 * to be set, unless it is compiling into another language. */
	CamelSExpResult *	(*and_)		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpTerm **argv,
						 CamelFolderSearch *search);
	CamelSExpResult *	(*or_)		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpTerm **argv,
						 CamelFolderSearch *search);
	CamelSExpResult *	(*not_)		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);
	CamelSExpResult *	(*lt)		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpTerm **argv,
						 CamelFolderSearch *search);
	CamelSExpResult *	(*gt)		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpTerm **argv,
						 CamelFolderSearch *search);
	CamelSExpResult *	(*eq)		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpTerm **argv,
						 CamelFolderSearch *search);

	/* Search Options */

	/* (match-all [boolean expression])
	 * Apply match to all messages. */
	CamelSExpResult *	(*match_all)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpTerm **argv,
						 CamelFolderSearch *search);

	/* (match-threads "type" [array expression])
	 * Add all related threads. */
	CamelSExpResult *	(*match_threads)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpTerm **argv,
						 CamelFolderSearch *search);

	/* (body-contains "string1" "string2" ...)
	 * Returns a list of matches, or true if in single-message mode. */
	CamelSExpResult *	(*body_contains)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (body-regex "regex")
	 * Returns a list of matches, or true if in single-message mode. */
	CamelSExpResult *	(*body_regex)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-contains "headername" "string1" ...)
	 * List of matches, or true if in single-message mode. */
	CamelSExpResult *	(*header_contains)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-matches "headername" "string") */
	CamelSExpResult *	(*header_matches)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-starts-with "headername" "string") */
	CamelSExpResult *	(*header_starts_with)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-ends-with "headername" "string") */
	CamelSExpResult *	(*header_ends_with)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-exists "headername") */
	CamelSExpResult *	(*header_exists)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-soundex "headername" "string") */
	CamelSExpResult *	(*header_soundex)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-regex "headername" "regex_string") */
	CamelSExpResult *	(*header_regex)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (header-full-regex "regex") */
	CamelSExpResult *	(*header_full_regex)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (user-flag "flagname" "flagname" ...)
	 * If one of user-flag set. */
	CamelSExpResult *	(*user_flag)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (user-tag "flagname")
	 * Returns the value of a user tag.  Can only be used in match-all. */
	CamelSExpResult *	(*user_tag)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (system-flag "flagname")
	 * Returns the value of a system flag.
	 * Can only be used in match-all. */
	CamelSExpResult *	(*system_flag)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (get-sent-date)
	 * Retrieve the date that the message was sent on as a time_t. */
	CamelSExpResult *	(*get_sent_date)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (get-received-date)
	 * Retrieve the date that the message was received on as a time_t. */
	CamelSExpResult *	(*get_received_date)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (get-current-date)
	 * Retrieve 'now' as a time_t. */
	CamelSExpResult *	(*get_current_date)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (get-relative-months)
	 * Retrieve relative seconds from 'now' and
	 * specified number of months as a time_t. */
	CamelSExpResult *	(*get_relative_months)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (get-size)
	 * Retrieve message size as an gint (in kilobytes). */
	CamelSExpResult *	(*get_size)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (uid "uid" ...)
	 * True if the uid is in the list. */
	CamelSExpResult *	(*uid)		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);

	/* (message-location "folder_string")
	 * True if the message is in the folder's full name "folder_string". */
	CamelSExpResult *	(*message_location)
						(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 CamelFolderSearch *search);
};

GType		camel_folder_search_get_type	(void) G_GNUC_CONST;
CamelFolderSearch *
		camel_folder_search_new		(void);
void		camel_folder_search_set_only_cached_messages
						(CamelFolderSearch *search,
						 gboolean only_cached_messages);
gboolean	camel_folder_search_get_only_cached_messages
						(CamelFolderSearch *search);

/* XXX This stuff currently gets cleared when you run a search.
 *     What on earth was i thinking ... */
void		camel_folder_search_set_folder	(CamelFolderSearch *search,
						 CamelFolder *folder);
void		camel_folder_search_set_summary	(CamelFolderSearch *search,
						 GPtrArray *summary);
void		camel_folder_search_set_body_index
						(CamelFolderSearch *search,
						 CamelIndex *body_index);

GPtrArray *	camel_folder_search_search	(CamelFolderSearch *search,
						 const gchar *expr,
						 GPtrArray *uids,
						 GCancellable *cancellable,
						 GError **error);
guint32		camel_folder_search_count	(CamelFolderSearch *search,
						 const gchar *expr,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_search_free_result	(CamelFolderSearch *search,
						 GPtrArray *result);

/* XXX This belongs in a general utility file. */
time_t		camel_folder_search_util_add_months
						(time_t t,
						 gint months);

#ifndef CAMEL_DISABLE_DEPRECATED
void		camel_folder_search_construct	(CamelFolderSearch *search);
#endif /* CAMEL_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* CAMEL_FOLDER_SEARCH_H */
