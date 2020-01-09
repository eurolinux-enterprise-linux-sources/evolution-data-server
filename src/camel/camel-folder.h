/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * camel-folder.h: Abstract class for an email folder
 *
 * Authors: Bertrand Guiheneuf <bertrand@helixcode.com>
 *	    Michael Zucchi <notzed@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 * This library is free software you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#if !defined (__CAMEL_H_INSIDE__) && !defined (CAMEL_COMPILATION)
#error "Only <camel/camel.h> can be included directly."
#endif

#ifndef CAMEL_FOLDER_H
#define CAMEL_FOLDER_H

#include <camel/camel-enums.h>
#include <camel/camel-folder-summary.h>
#include <camel/camel-object.h>

/* Standard GObject macros */
#define CAMEL_TYPE_FOLDER \
	(camel_folder_get_type ())
#define CAMEL_FOLDER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), CAMEL_TYPE_FOLDER, CamelFolder))
#define CAMEL_FOLDER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), CAMEL_TYPE_FOLDER, CamelFolderClass))
#define CAMEL_IS_FOLDER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), CAMEL_TYPE_FOLDER))
#define CAMEL_IS_FOLDER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), CAMEL_TYPE_FOLDER))
#define CAMEL_FOLDER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), CAMEL_TYPE_FOLDER, CamelFolderClass))

/**
 * CAMEL_FOLDER_ERROR:
 *
 * Since: 2.32
 **/
#define CAMEL_FOLDER_ERROR \
	(camel_folder_error_quark ())

G_BEGIN_DECLS

struct _CamelStore;

typedef struct _CamelFolderChangeInfo CamelFolderChangeInfo;
typedef struct _CamelFolderChangeInfoPrivate CamelFolderChangeInfoPrivate;

typedef struct _CamelFolder CamelFolder;
typedef struct _CamelFolderClass CamelFolderClass;
typedef struct _CamelFolderPrivate CamelFolderPrivate;

/**
 * CamelFolderError:
 *
 * Since: 2.32
 **/
typedef enum {
	CAMEL_FOLDER_ERROR_INVALID,
	CAMEL_FOLDER_ERROR_INVALID_STATE,
	CAMEL_FOLDER_ERROR_NON_EMPTY,
	CAMEL_FOLDER_ERROR_NON_UID,
	CAMEL_FOLDER_ERROR_INSUFFICIENT_PERMISSION,
	CAMEL_FOLDER_ERROR_INVALID_PATH,
	CAMEL_FOLDER_ERROR_INVALID_UID,
	CAMEL_FOLDER_ERROR_SUMMARY_INVALID
} CamelFolderError;

/**
 * CamelFetchType:
 *
 * Since: 3.4
 **/
typedef enum {
	CAMEL_FETCH_OLD_MESSAGES,
	CAMEL_FETCH_NEW_MESSAGES
} CamelFetchType;

struct _CamelFolderChangeInfo {
	GPtrArray *uid_added;
	GPtrArray *uid_removed;
	GPtrArray *uid_changed;
	GPtrArray *uid_recent;

	CamelFolderChangeInfoPrivate *priv;
};

typedef struct _CamelFolderQuotaInfo CamelFolderQuotaInfo;

/**
 * CamelFolderQuotaInfo:
 *
 * Since: 2.24
 **/
struct _CamelFolderQuotaInfo {
	gchar *name;
	guint64 used;
	guint64 total;

	struct _CamelFolderQuotaInfo *next;
};

struct _CamelFolder {
	CamelObject parent;
	CamelFolderPrivate *priv;

	CamelFolderSummary *summary;

	CamelFolderFlags folder_flags;
	CamelMessageFlags permanent_flags;

	/* Future ABI expansion */
	gpointer later[4];
};

struct _CamelFolderClass {
	CamelObjectClass parent_class;

	/* Non-Blocking Methods */
	gint		(*get_message_count)	(CamelFolder *folder);
	CamelMessageFlags
			(*get_permanent_flags)	(CamelFolder *folder);
	CamelMessageFlags
			(*get_message_flags)	(CamelFolder *folder,
						 const gchar *uid);
	gboolean	(*set_message_flags)	(CamelFolder *folder,
						 const gchar *uid,
						 CamelMessageFlags flags,
						 CamelMessageFlags set);
	gboolean	(*get_message_user_flag)(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name);
	void		(*set_message_user_flag)(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name,
						 gboolean value);
	const gchar *	(*get_message_user_tag)	(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name);
	void		(*set_message_user_tag)	(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name,
						 const gchar *value);
	GPtrArray *	(*get_uids)		(CamelFolder *folder);
	void		(*free_uids)		(CamelFolder *folder,
						 GPtrArray *array);
	gint		(*cmp_uids)		(CamelFolder *folder,
						 const gchar *uid1,
						 const gchar *uid2);
	void		(*sort_uids)		(CamelFolder *folder,
						 GPtrArray *uids);
	GPtrArray *	(*get_summary)		(CamelFolder *folder);
	void		(*free_summary)		(CamelFolder *folder,
						 GPtrArray *summary);
	gboolean	(*has_search_capability)(CamelFolder *folder);
	GPtrArray *	(*search_by_expression)	(CamelFolder *folder,
						 const gchar *expression,
						 GCancellable *cancellable,
						 GError **error);
	GPtrArray *	(*search_by_uids)	(CamelFolder *folder,
						 const gchar *expression,
						 GPtrArray *uids,
						 GCancellable *cancellable,
						 GError **error);
	void		(*search_free)		(CamelFolder *folder,
						 GPtrArray *result);
	CamelMessageInfo *
			(*get_message_info)	(CamelFolder *folder,
						 const gchar *uid);
	void		(*delete_)		(CamelFolder *folder);
	void		(*rename)		(CamelFolder *folder,
						 const gchar *newname);
	void		(*freeze)		(CamelFolder *folder);
	void		(*thaw)			(CamelFolder *folder);
	gboolean	(*is_frozen)		(CamelFolder *folder);
	guint32		(*count_by_expression)	(CamelFolder *folder,
						 const gchar *expression,
						 GCancellable *cancellable,
						 GError **error);
	GPtrArray *	(*get_uncached_uids)	(CamelFolder *folder,
						 GPtrArray *uids,
						 GError **error);
	gchar *		(*get_filename)		(CamelFolder *folder,
						 const gchar *uid,
						 GError **error);
	CamelMimeMessage *
			(*get_message_cached)	(CamelFolder *folder,
						 const gchar *message_uid,
						 GCancellable *cancellable);

	/* Synchronous I/O Methods */
	gboolean	(*append_message_sync)	(CamelFolder *folder,
						 CamelMimeMessage *message,
						 CamelMessageInfo *info,
						 gchar **appended_uid,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*expunge_sync)		(CamelFolder *folder,
						 GCancellable *cancellable,
						 GError **error);
	CamelMimeMessage *
			(*get_message_sync)	(CamelFolder *folder,
						 const gchar *message_uid,
						 GCancellable *cancellable,
						 GError **error);
	CamelFolderQuotaInfo *
			(*get_quota_info_sync)	(CamelFolder *folder,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*purge_message_cache_sync)
						(CamelFolder *folder,
						 gchar *start_uid,
						 gchar *end_uid,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*refresh_info_sync)	(CamelFolder *folder,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*synchronize_sync)	(CamelFolder *folder,
						 gboolean expunge,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*synchronize_message_sync)
						(CamelFolder *folder,
						 const gchar *message_uid,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*transfer_messages_to_sync)
						(CamelFolder *source,
						 GPtrArray *message_uids,
						 CamelFolder *destination,
						 gboolean delete_originals,
						 GPtrArray **transferred_uids,
						 GCancellable *cancellable,
						 GError **error);

	/* Reserved slots for methods. */
	gpointer reserved_for_methods[20];

	/* Signals */
	void		(*changed)		(CamelFolder *folder,
						 CamelFolderChangeInfo *info);
	void		(*deleted)		(CamelFolder *folder);
	void		(*renamed)		(CamelFolder *folder,
						 const gchar *old_name);
};

GType		camel_folder_get_type		(void);
GQuark		camel_folder_error_quark	(void) G_GNUC_CONST;
void		camel_folder_set_lock_async	(CamelFolder *folder,
						 gboolean skip_folder_lock);
struct _CamelStore *
		camel_folder_get_parent_store	(CamelFolder *folder);
const gchar *	camel_folder_get_full_name	(CamelFolder *folder);
gchar *		camel_folder_dup_full_name	(CamelFolder *folder);
void		camel_folder_set_full_name	(CamelFolder *folder,
						 const gchar *full_name);
const gchar *	camel_folder_get_display_name	(CamelFolder *folder);
gchar *		camel_folder_dup_display_name	(CamelFolder *folder);
void		camel_folder_set_display_name	(CamelFolder *folder,
						 const gchar *display_name);
const gchar *	camel_folder_get_description	(CamelFolder *folder);
gchar *		camel_folder_dup_description	(CamelFolder *folder);
void		camel_folder_set_description	(CamelFolder *folder,
						 const gchar *description);
CamelMessageFlags
		camel_folder_get_permanent_flags
						(CamelFolder *folder);
#ifndef CAMEL_DISABLE_DEPRECATED
CamelMessageFlags
		camel_folder_get_message_flags	(CamelFolder *folder,
						 const gchar *uid);
gboolean	camel_folder_set_message_flags	(CamelFolder *folder,
						 const gchar *uid,
						 CamelMessageFlags flags,
						 CamelMessageFlags set);
gboolean	camel_folder_get_message_user_flag
						(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name);
void		camel_folder_set_message_user_flag
						(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name,
						 gboolean value);
const gchar *	camel_folder_get_message_user_tag
						(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name);
void		camel_folder_set_message_user_tag
						(CamelFolder *folder,
						 const gchar *uid,
						 const gchar *name,
						 const gchar *value);
#endif /* CAMEL_DISABLE_DEPRECATED */
gboolean	camel_folder_has_summary_capability
						(CamelFolder *folder);
gint		camel_folder_get_message_count	(CamelFolder *folder);
#ifndef CAMEL_DISABLE_DEPRECATED
gint		camel_folder_get_unread_message_count
						(CamelFolder *folder);
#endif
gint		camel_folder_get_deleted_message_count
						(CamelFolder *folder);
GPtrArray *	camel_folder_get_summary	(CamelFolder *folder);
void		camel_folder_free_summary	(CamelFolder *folder,
						 GPtrArray *array);

#define camel_folder_delete_message(folder, uid) \
	(camel_folder_set_message_flags ( \
		folder, uid, \
		CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN, \
		CAMEL_MESSAGE_DELETED | CAMEL_MESSAGE_SEEN))

GPtrArray *	camel_folder_get_uids		(CamelFolder *folder);
void		camel_folder_free_uids		(CamelFolder *folder,
						 GPtrArray *array);
GPtrArray *	camel_folder_get_uncached_uids	(CamelFolder *folder,
						 GPtrArray *uids,
						 GError **error);
gint		camel_folder_cmp_uids		(CamelFolder *folder,
						 const gchar *uid1,
						 const gchar *uid2);
void		camel_folder_sort_uids		(CamelFolder *folder,
						 GPtrArray *uids);
GPtrArray *	camel_folder_search_by_expression
						(CamelFolder *folder,
						 const gchar *expr,
						 GCancellable *cancellable,
						 GError **error);
GPtrArray *	camel_folder_search_by_uids	(CamelFolder *folder,
						 const gchar *expr,
						 GPtrArray *uids,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_search_free	(CamelFolder *folder,
						 GPtrArray *result);
guint32		camel_folder_count_by_expression (CamelFolder *folder,
						 const gchar *expression,
						 GCancellable *cancellable,
						 GError **error);
CamelMessageInfo *
		camel_folder_get_message_info	(CamelFolder *folder,
						 const gchar *uid);
void		camel_folder_delete		(CamelFolder *folder);
void		camel_folder_rename		(CamelFolder *folder,
						 const gchar *new_name);
void		camel_folder_changed		(CamelFolder *folder,
						 CamelFolderChangeInfo *changes);
void		camel_folder_freeze		(CamelFolder *folder);
void		camel_folder_thaw		(CamelFolder *folder);
gboolean	camel_folder_is_frozen		(CamelFolder *folder);
gint		camel_folder_get_frozen_count	(CamelFolder *folder);
CamelFolderQuotaInfo *
		camel_folder_quota_info_new	(const gchar *name,
						 guint64 used,
						 guint64 total);
CamelFolderQuotaInfo *
		camel_folder_quota_info_clone	(const CamelFolderQuotaInfo *info);
void		camel_folder_quota_info_free	(CamelFolderQuotaInfo *info);
void		camel_folder_free_nop		(CamelFolder *folder,
						 GPtrArray *array);
void		camel_folder_free_shallow	(CamelFolder *folder,
						 GPtrArray *array);
void		camel_folder_free_deep		(CamelFolder *folder,
						 GPtrArray *array);
gchar *		camel_folder_get_filename	(CamelFolder *folder,
						 const gchar *uid,
						 GError **error);
void		camel_folder_lock		(CamelFolder *folder);
void		camel_folder_unlock		(CamelFolder *folder);

gboolean	camel_folder_append_message_sync
						(CamelFolder *folder,
						 CamelMimeMessage *message,
						 CamelMessageInfo *info,
						 gchar **appended_uid,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_append_message	(CamelFolder *folder,
						 CamelMimeMessage *message,
						 CamelMessageInfo *info,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_folder_append_message_finish
						(CamelFolder *folder,
						 GAsyncResult *result,
						 gchar **appended_uid,
						 GError **error);
gboolean	camel_folder_expunge_sync	(CamelFolder *folder,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_expunge		(CamelFolder *folder,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_folder_expunge_finish	(CamelFolder *folder,
						 GAsyncResult *result,
						 GError **error);
CamelMimeMessage *
		camel_folder_get_message_sync	(CamelFolder *folder,
						 const gchar *message_uid,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_get_message	(CamelFolder *folder,
						 const gchar *message_uid,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
CamelMimeMessage *
		camel_folder_get_message_finish	(CamelFolder *folder,
						 GAsyncResult *result,
						 GError **error);
CamelFolderQuotaInfo *
		camel_folder_get_quota_info_sync
						(CamelFolder *folder,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_get_quota_info	(CamelFolder *folder,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
CamelFolderQuotaInfo *
		camel_folder_get_quota_info_finish
						(CamelFolder *folder,
						 GAsyncResult *result,
						 GError **error);
gboolean	camel_folder_purge_message_cache_sync
						(CamelFolder *folder,
						 gchar *start_uid,
						 gchar *end_uid,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_purge_message_cache
						(CamelFolder *folder,
						 gchar *start_uid,
						 gchar *end_uid,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_folder_purge_message_cache_finish
						(CamelFolder *folder,
						 GAsyncResult *result,
						 GError **error);
gboolean	camel_folder_refresh_info_sync	(CamelFolder *folder,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_refresh_info	(CamelFolder *folder,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_folder_refresh_info_finish
						(CamelFolder *folder,
						 GAsyncResult *result,
						 GError **error);
gboolean	camel_folder_synchronize_sync	(CamelFolder *folder,
						 gboolean expunge,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_synchronize	(CamelFolder *folder,
						 gboolean expunge,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_folder_synchronize_finish	(CamelFolder *folder,
						 GAsyncResult *result,
						 GError **error);
gboolean	camel_folder_synchronize_message_sync
						(CamelFolder *folder,
						 const gchar *message_uid,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_synchronize_message
						(CamelFolder *folder,
						 const gchar *message_uid,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_folder_synchronize_message_finish
						(CamelFolder *folder,
						 GAsyncResult *result,
						 GError **error);
gboolean	camel_folder_transfer_messages_to_sync
						(CamelFolder *source,
						 GPtrArray *message_uids,
						 CamelFolder *destination,
						 gboolean delete_originals,
						 GPtrArray **transferred_uids,
						 GCancellable *cancellable,
						 GError **error);
void		camel_folder_transfer_messages_to
						(CamelFolder *source,
						 GPtrArray *message_uids,
						 CamelFolder *destination,
						 gboolean delete_originals,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_folder_transfer_messages_to_finish
						(CamelFolder *source,
						 GAsyncResult *result,
						 GPtrArray **transferred_uids,
						 GError **error);

/* update functions for change info */
CamelFolderChangeInfo *
		camel_folder_change_info_new	(void);
void		camel_folder_change_info_clear	(CamelFolderChangeInfo *info);
void		camel_folder_change_info_free	(CamelFolderChangeInfo *info);
gboolean	camel_folder_change_info_changed (CamelFolderChangeInfo *info);

/* for building diff's automatically */
void		camel_folder_change_info_add_source
						(CamelFolderChangeInfo *info,
						 const gchar *uid);
void		camel_folder_change_info_add_source_list
						(CamelFolderChangeInfo *info,
						 const GPtrArray *list);
void		camel_folder_change_info_add_update
						(CamelFolderChangeInfo *info,
						 const gchar *uid);
void		camel_folder_change_info_add_update_list
						(CamelFolderChangeInfo *info,
						 const GPtrArray *list);
void		camel_folder_change_info_build_diff
						(CamelFolderChangeInfo *info);

/* for manipulating diff's directly */
void		camel_folder_change_info_cat	(CamelFolderChangeInfo *info,
						 CamelFolderChangeInfo *src);
void		camel_folder_change_info_add_uid (CamelFolderChangeInfo *info,
						 const gchar *uid);
void		camel_folder_change_info_remove_uid
						(CamelFolderChangeInfo *info,
						 const gchar *uid);
void		camel_folder_change_info_change_uid
						(CamelFolderChangeInfo *info,
						 const gchar *uid);
void		camel_folder_change_info_recent_uid
						(CamelFolderChangeInfo *info,
						 const gchar *uid);

G_END_DECLS

#endif /* CAMEL_FOLDER_H */
