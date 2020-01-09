/*
 * generic s-exp evaluator class
*/

#if !defined (__CAMEL_H_INSIDE__) && !defined (CAMEL_COMPILATION)
#error "Only <camel/camel.h> can be included directly."
#endif

#ifndef CAMEL_SEXP_H
#define CAMEL_SEXP_H

#include <setjmp.h>
#include <time.h>
#include <glib.h>

#include <glib-object.h>

#include <camel/camel-memchunk.h>

/* Standard GObject macros */
#define CAMEL_TYPE_SEXP \
	(camel_sexp_get_type ())
#define CAMEL_SEXP(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), CAMEL_TYPE_SEXP, CamelSExp))
#define CAMEL_SEXP_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), CAMEL_TYPE_SEXP, CamelSExpClass))
#define CAMEL_IS_SEXP(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), CAMEL_TYPE_SEXP))
#define CAMEL_IS_SEXP_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), CAMEL_TYPE_SEXP))
#define CAMEL_SEXP_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), CAMEL_TYPE_SEXP, CamelSExpClass))

G_BEGIN_DECLS

typedef struct _CamelSExp CamelSExp;
typedef struct _CamelSExpClass CamelSExpClass;

typedef struct _CamelSExpSymbol CamelSExpSymbol;
typedef struct _CamelSExpResult CamelSExpResult;
typedef struct _CamelSExpTerm CamelSExpTerm;

/**
 * CamelSExpResultType:
 *
 * Since: 3.4
 **/
typedef enum {
	CAMEL_SEXP_RES_ARRAY_PTR,	/* type is a ptrarray, what it points to is implementation dependant */
	CAMEL_SEXP_RES_INT,		/* type is a number */
	CAMEL_SEXP_RES_STRING,		/* type is a pointer to a single string */
	CAMEL_SEXP_RES_BOOL,		/* boolean type */
	CAMEL_SEXP_RES_TIME,		/* time_t type */
	CAMEL_SEXP_RES_UNDEFINED	/* unknown type */
} CamelSExpResultType;

/**
 * CamelSExpResult:
 *
 * Since: 3.4
 **/
struct _CamelSExpResult {
	CamelSExpResultType type;
	union {
		GPtrArray *ptrarray;
		gint number;
		gchar *string;
		gint boolean;
		time_t time;
	} value;
	gboolean time_generator;
	time_t occuring_start;
	time_t occuring_end;
};

/**
 * CamelSExpFunc:
 *
 * Since: 3.4
 **/
typedef CamelSExpResult *
			(*CamelSExpFunc)	(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv,
						 gpointer data);

/**
 * CamelSExpIFunc:
 *
 * Since: 3.4
 **/
typedef CamelSExpResult *
			(*CamelSExpIFunc)	(CamelSExp *sexp, gint argc,
						 CamelSExpTerm **argv,
						 gpointer data);

/**
 * CamelSExpTermType:
 *
 * Since: 3.4
 **/
typedef enum {
	CAMEL_SEXP_TERM_INT,	/* integer literal */
	CAMEL_SEXP_TERM_BOOL,	/* boolean literal */
	CAMEL_SEXP_TERM_STRING,	/* string literal */
	CAMEL_SEXP_TERM_TIME,	/* time_t literal (number of seconds past the epoch) */
	CAMEL_SEXP_TERM_FUNC,	/* normal function, arguments are evaluated before calling */
	CAMEL_SEXP_TERM_IFUNC,	/* immediate function, raw terms are arguments */
	CAMEL_SEXP_TERM_VAR	/* variable reference */
} CamelSExpTermType;

/**
 * CamelSExpSymbol:
 *
 * Since: 3.4
 **/
struct _CamelSExpSymbol {
	gint type;		/* TERM_FUNC or TERM_VAR */
	gchar *name;
	gpointer data;
	union {
		CamelSExpFunc func;
		CamelSExpIFunc ifunc;
	} f;
};

/**
 * CamelSExpTerm:
 *
 * Since: 3.4
 **/
struct _CamelSExpTerm {
	CamelSExpTermType type;
	union {
		gchar *string;
		gint number;
		gint boolean;
		time_t time;
		struct {
			CamelSExpSymbol *sym;
			CamelSExpTerm **terms;
			gint termcount;
		} func;
		CamelSExpSymbol *var;
	} value;
};

/**
 * CamelSExp:
 *
 * Since: 3.4
 **/
struct _CamelSExp {
	GObject parent;
	GScanner *scanner;	/* for parsing text version */
	CamelSExpTerm *tree;	/* root of expression tree */

	/* private stuff */
	jmp_buf failenv;
	gchar *error;
	GSList *operators;

	/* TODO: may also need a pool allocator for term strings,
	 *       so we dont lose them in error conditions? */
	CamelMemChunk *term_chunks;
	CamelMemChunk *result_chunks;
};

struct _CamelSExpClass {
	GObjectClass parent_class;
};

GType		camel_sexp_get_type		(void) G_GNUC_CONST;
CamelSExp *	camel_sexp_new			(void);
void		camel_sexp_add_function		(CamelSExp *sexp,
						 guint scope,
						 const gchar *name,
						 CamelSExpFunc func,
						 gpointer data);
void		camel_sexp_add_ifunction	(CamelSExp *sexp,
						 guint scope,
						 const gchar *name,
						 CamelSExpIFunc func,
						 gpointer data);
void		camel_sexp_add_variable		(CamelSExp *sexp,
						 guint scope,
						 gchar *name,
						 CamelSExpTerm *value);
void		camel_sexp_remove_symbol	(CamelSExp *sexp,
						 guint scope,
						 const gchar *name);
gint		camel_sexp_set_scope		(CamelSExp *sexp,
						 guint scope);
void		camel_sexp_input_text		(CamelSExp *sexp,
						 const gchar *text,
						 gint len);
void		camel_sexp_input_file		(CamelSExp *sexp,
						 gint fd);
gint		camel_sexp_parse		(CamelSExp *sexp);
CamelSExpResult *
		camel_sexp_eval			(CamelSExp *sexp);
CamelSExpResult *
		camel_sexp_term_eval		(CamelSExp *sexp,
						 CamelSExpTerm *term);
CamelSExpResult *
		camel_sexp_result_new		(CamelSExp *sexp,
						 gint type);
void		camel_sexp_result_free		(CamelSExp *sexp,
						 CamelSExpResult *term);

/* used in normal functions if they have to abort, to free their arguments */
void		camel_sexp_resultv_free		(CamelSExp *sexp,
						 gint argc,
						 CamelSExpResult **argv);

/* utility functions for creating s-exp strings. */
void		camel_sexp_encode_bool		(GString *string,
						 gboolean v_bool);
void		camel_sexp_encode_string	(GString *string,
						 const gchar *v_string);

/* only to be called from inside a callback to signal a fatal execution error */
void		camel_sexp_fatal_error		(CamelSExp *sexp,
						 const gchar *why,
						 ...) G_GNUC_NORETURN;

/* return the error string */
const gchar *	camel_sexp_error		(CamelSExp *sexp);

CamelSExpTerm *	camel_sexp_parse_value		(CamelSExp *sexp);

gboolean	camel_sexp_evaluate_occur_times	(CamelSExp *sexp,
						 time_t *start,
						 time_t *end);

G_END_DECLS

#endif /* CAMEL_SEXP_H */
