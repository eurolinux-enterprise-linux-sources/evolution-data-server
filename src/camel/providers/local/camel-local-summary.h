/*
 *  Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 *  Authors: Michael Zucchi <notzed@ximian.com>
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
 */

#ifndef _CAMEL_LOCAL_SUMMARY_H
#define _CAMEL_LOCAL_SUMMARY_H

#include <camel/camel-folder-summary.h>
#include <camel/camel-folder.h>
#include <camel/camel-exception.h>
#include <camel/camel-index.h>

#define CAMEL_LOCAL_SUMMARY(obj)         CAMEL_CHECK_CAST (obj, camel_local_summary_get_type (), CamelLocalSummary)
#define CAMEL_LOCAL_SUMMARY_CLASS(klass) CAMEL_CHECK_CLASS_CAST (klass, camel_local_summary_get_type (), CamelLocalSummaryClass)
#define CAMEL_IS_LOCAL_SUMMARY(obj)      CAMEL_CHECK_TYPE (obj, camel_local_summary_get_type ())

G_BEGIN_DECLS

typedef struct _CamelLocalSummary      CamelLocalSummary;
typedef struct _CamelLocalSummaryClass CamelLocalSummaryClass;

/* extra summary flags */
enum {
	CAMEL_MESSAGE_FOLDER_NOXEV = 1<<17,
	CAMEL_MESSAGE_FOLDER_XEVCHANGE = 1<<18,
	CAMEL_MESSAGE_FOLDER_NOTSEEN = 1<<19 /* have we seen this in processing this loop? */
};

typedef struct _CamelLocalMessageInfo CamelLocalMessageInfo;

struct _CamelLocalMessageInfo {
	CamelMessageInfoBase info;
};

struct _CamelLocalSummary {
	CamelFolderSummary parent;

	guint32 version;	/* file version being loaded */

	gchar *folder_path;	/* name of matching folder */

	CamelIndex *index;
	guint index_force:1; /* do we force index during creation? */
	guint check_force:1; /* does a check force a full check? */
};

struct _CamelLocalSummaryClass {
	CamelFolderSummaryClass parent_class;

	gint (*load)(CamelLocalSummary *cls, gint forceindex, CamelException *ex);
	gint (*check)(CamelLocalSummary *cls, CamelFolderChangeInfo *changeinfo, CamelException *ex);
	gint (*sync)(CamelLocalSummary *cls, gboolean expunge, CamelFolderChangeInfo *changeinfo, CamelException *ex);
	CamelMessageInfo *(*add)(CamelLocalSummary *cls, CamelMimeMessage *msg, const CamelMessageInfo *info, CamelFolderChangeInfo *, CamelException *ex);

	gchar *(*encode_x_evolution)(CamelLocalSummary *cls, const CamelLocalMessageInfo *info);
	gint (*decode_x_evolution)(CamelLocalSummary *cls, const gchar *xev, CamelLocalMessageInfo *info);
	gint (*need_index)(void);
};

CamelType	camel_local_summary_get_type	(void);
void	camel_local_summary_construct	(CamelLocalSummary *new, const gchar *filename, const gchar *local_name, CamelIndex *index);

/* load/check the summary */
gint camel_local_summary_load(CamelLocalSummary *cls, gint forceindex, CamelException *ex);
/* check for new/removed messages */
gint camel_local_summary_check(CamelLocalSummary *cls, CamelFolderChangeInfo *, CamelException *ex);
/* perform a folder sync or expunge, if needed */
gint camel_local_summary_sync(CamelLocalSummary *cls, gboolean expunge, CamelFolderChangeInfo *, CamelException *ex);
/* add a new message to the summary */
CamelMessageInfo *camel_local_summary_add(CamelLocalSummary *cls, CamelMimeMessage *msg, const CamelMessageInfo *info, CamelFolderChangeInfo *, CamelException *ex);

/* force the next check to be a full check/rebuild */
void camel_local_summary_check_force(CamelLocalSummary *cls);

/* generate an X-Evolution header line */
gchar *camel_local_summary_encode_x_evolution(CamelLocalSummary *cls, const CamelLocalMessageInfo *info);
gint camel_local_summary_decode_x_evolution(CamelLocalSummary *cls, const gchar *xev, CamelLocalMessageInfo *info);

/* utility functions - write headers to a file with optional X-Evolution header and/or status header */
gint camel_local_summary_write_headers(gint fd, struct _camel_header_raw *header, const gchar *xevline, const gchar *status, const gchar *xstatus);

G_END_DECLS

#endif /* ! _CAMEL_LOCAL_SUMMARY_H */
