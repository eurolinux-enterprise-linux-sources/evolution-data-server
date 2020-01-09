/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Authors: Bertrand Guiheneuf <bertrand@helixcode.com>
 *          Michael Zucchi <notzed@ximian.com>
 *          Jeffrey Stedfast <fejj@ximian.com>
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
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#if !defined (__CAMEL_H_INSIDE__) && !defined (CAMEL_COMPILATION)
#error "Only <camel/camel.h> can be included directly."
#endif

#ifndef CAMEL_DATA_WRAPPER_H
#define CAMEL_DATA_WRAPPER_H

#include <sys/types.h>

#include <camel/camel-mime-utils.h>
#include <camel/camel-stream.h>

/* Standard GObject macros */
#define CAMEL_TYPE_DATA_WRAPPER \
	(camel_data_wrapper_get_type ())
#define CAMEL_DATA_WRAPPER(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), CAMEL_TYPE_DATA_WRAPPER, CamelDataWrapper))
#define CAMEL_DATA_WRAPPER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), CAMEL_TYPE_DATA_WRAPPER, CamelDataWrapperClass))
#define CAMEL_IS_DATA_WRAPPER(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), CAMEL_TYPE_DATA_WRAPPER))
#define CAMEL_IS_DATA_WRAPPER_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), CAMEL_TYPE_DATA_WRAPPER))
#define CAMEL_DATA_WRAPPER_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), CAMEL_TYPE_DATA_WRAPPER, CamelDataWrapperClass))

G_BEGIN_DECLS

typedef struct _CamelDataWrapper CamelDataWrapper;
typedef struct _CamelDataWrapperClass CamelDataWrapperClass;
typedef struct _CamelDataWrapperPrivate CamelDataWrapperPrivate;

struct _CamelDataWrapper {
	GObject parent;
	CamelDataWrapperPrivate *priv;

	CamelTransferEncoding encoding;

	CamelContentType *mime_type;

	guint offline : 1;
};

struct _CamelDataWrapperClass {
	GObjectClass parent_class;

	/* Non-Blocking Methods */
	void		(*set_mime_type)	(CamelDataWrapper *data_wrapper,
						 const gchar *mime_type);
	gchar *		(*get_mime_type)	(CamelDataWrapper *data_wrapper);
	CamelContentType *
			(*get_mime_type_field)	(CamelDataWrapper *data_wrapper);
	void		(*set_mime_type_field)	(CamelDataWrapper *data_wrapper,
						 CamelContentType *mime_type_field);
	gboolean	(*is_offline)		(CamelDataWrapper *data_wrapper);

	/* Synchronous I/O Methods */
	gssize		(*write_to_stream_sync)	(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 GCancellable *cancellable,
						 GError **error);
	gssize		(*decode_to_stream_sync)(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*construct_from_stream_sync)
						(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 GCancellable *cancellable,
						 GError **error);
	gssize		(*write_to_output_stream_sync)
						(CamelDataWrapper *data_wrapper,
						 GOutputStream *output_stream,
						 GCancellable *cancellable,
						 GError **error);
	gssize		(*decode_to_output_stream_sync)
						(CamelDataWrapper *data_wrapper,
						 GOutputStream *output_stream,
						 GCancellable *cancellable,
						 GError **error);
	gboolean	(*construct_from_input_stream_sync)
						(CamelDataWrapper *data_wrapper,
						 GInputStream *input_stream,
						 GCancellable *cancellable,
						 GError **error);

	/* Reserved slots. */
	gpointer reserved[3];
};

GType		camel_data_wrapper_get_type	(void);
CamelDataWrapper *
		camel_data_wrapper_new		(void);
GByteArray *	camel_data_wrapper_get_byte_array
						(CamelDataWrapper *data_wrapper);
void		camel_data_wrapper_set_mime_type
						(CamelDataWrapper *data_wrapper,
						 const gchar *mime_type);
gchar *		camel_data_wrapper_get_mime_type
						(CamelDataWrapper *data_wrapper);
CamelContentType *
		camel_data_wrapper_get_mime_type_field
						(CamelDataWrapper *data_wrapper);
void		camel_data_wrapper_set_mime_type_field
						(CamelDataWrapper *data_wrapper,
						 CamelContentType *mime_type);
gboolean	camel_data_wrapper_is_offline	(CamelDataWrapper *data_wrapper);

gssize		camel_data_wrapper_write_to_stream_sync
						(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 GCancellable *cancellable,
						 GError **error);
void		camel_data_wrapper_write_to_stream
						(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gssize		camel_data_wrapper_write_to_stream_finish
						(CamelDataWrapper *data_wrapper,
						 GAsyncResult *result,
						 GError **error);
gssize		camel_data_wrapper_decode_to_stream_sync
						(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 GCancellable *cancellable,
						 GError **error);
void		camel_data_wrapper_decode_to_stream
						(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gssize		camel_data_wrapper_decode_to_stream_finish
						(CamelDataWrapper *data_wrapper,
						 GAsyncResult *result,
						 GError **error);
gboolean	camel_data_wrapper_construct_from_stream_sync
						(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 GCancellable *cancellable,
						 GError **error);
void		camel_data_wrapper_construct_from_stream
						(CamelDataWrapper *data_wrapper,
						 CamelStream *stream,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_data_wrapper_construct_from_stream_finish
						(CamelDataWrapper *data_wrapper,
						 GAsyncResult *result,
						 GError **error);
gssize		camel_data_wrapper_write_to_output_stream_sync
						(CamelDataWrapper *data_wrapper,
						 GOutputStream *output_stream,
						 GCancellable *cancellable,
						 GError **error);
void		camel_data_wrapper_write_to_output_stream
						(CamelDataWrapper *data_wrapper,
						 GOutputStream *output_stream,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gssize		camel_data_wrapper_write_to_output_stream_finish
						(CamelDataWrapper *data_wrapper,
						 GAsyncResult *result,
						 GError **error);
gssize		camel_data_wrapper_decode_to_output_stream_sync
						(CamelDataWrapper *data_wrapper,
						 GOutputStream *output_stream,
						 GCancellable *cancellable,
						 GError **error);
void		camel_data_wrapper_decode_to_output_stream
						(CamelDataWrapper *data_wrapper,
						 GOutputStream *output_stream,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gssize		camel_data_wrapper_decode_to_output_stream_finish
						(CamelDataWrapper *data_wrapper,
						 GAsyncResult *result,
						 GError **error);
gboolean	camel_data_wrapper_construct_from_input_stream_sync
						(CamelDataWrapper *data_wrapper,
						 GInputStream *input_stream,
						 GCancellable *cancellable,
						 GError **error);
void		camel_data_wrapper_construct_from_input_stream
						(CamelDataWrapper *data_wrapper,
						 GInputStream *input_stream,
						 gint io_priority,
						 GCancellable *cancellable,
						 GAsyncReadyCallback callback,
						 gpointer user_data);
gboolean	camel_data_wrapper_construct_from_input_stream_finish
						(CamelDataWrapper *data_wrapper,
						 GAsyncResult *result,
						 GError **error);

G_END_DECLS

#endif /* CAMEL_DATA_WRAPPER_H */
