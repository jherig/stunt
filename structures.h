/******************************************************************************
  Copyright (c) 1992, 1995, 1996 Xerox Corporation.  All rights reserved.
  Portions of this code were written by Stephen White, aka ghond.
  Use and copying of this software and preparation of derivative works based
  upon this software are permitted.  Any distribution of this software or
  derivative works must comply with all applicable United States export
  control laws.  This software is made available AS IS, and Xerox Corporation
  makes no warranty about the software, its performance or its conformity to
  any specification.  Any person obtaining a copy of this software is requested
  to send their name and post office or electronic mail address to:
    Pavel Curtis
    Xerox PARC
    3333 Coyote Hill Rd.
    Palo Alto, CA 94304
    Pavel@Xerox.Com
 *****************************************************************************/

#ifndef Structures_h
#define Structures_h 1

#include <stdbool.h>

#include "my-stdio.h"

#include "config.h"

#define MAXINT	((int32) 2147483647L)
#define MININT	((int32) -2147483648L)
#define MAXOBJ	((Objid) MAXINT)
#define MINOBJ	((Objid) MININT)

typedef int32 Objid;

/*
 * Special Objid's
 */
#define SYSTEM_OBJECT	0
#define NOTHING		-1
#define AMBIGUOUS	-2
#define FAILED_MATCH	-3

/* Do not reorder or otherwise modify this list, except to add new elements at
 * the end, since the order here defines the numeric equivalents of the error
 * values, and those equivalents are both DB-accessible knowledge and stored in
 * raw form in the DB.
 */
enum error {
    E_NONE, E_TYPE, E_DIV, E_PERM, E_PROPNF, E_VERBNF, E_VARNF, E_INVIND,
    E_RECMOVE, E_MAXREC, E_RANGE, E_ARGS, E_NACC, E_INVARG, E_QUOTA, E_FLOAT,
    E_FILE, E_EXEC, E_INTRPT
};

/* Types which have external data should be marked with the
 * TYPE_COMPLEX_FLAG so that `free_var()'/`var_ref()'/`var_dup()' can
 * recognize them easily.  This flag is only set in memory.  The
 * original _TYPE_XYZ values are used in the database file and
 * returned to verbs calling typeof().  This allows the inlines to be
 * extremely cheap (both in space and time) for simple types like oids
 * and ints.
 */
#define TYPE_COMPLEX_FLAG	0x80
#define TYPE_DB_MASK		0x7f

/* Do not reorder or otherwise modify this list, except to add new
 * elements at the end (see THE END), since the order here defines the
 * numeric equivalents of the type values, and those equivalents are
 * both DB-accessible knowledge and stored in raw form in the DB.  For
 * new complex types add both a _TYPE_XYZ definition and a TYPE_XYZ
 * definition.
 */
typedef enum {
    TYPE_INT, TYPE_OBJ, _TYPE_STR, TYPE_ERR, _TYPE_LIST, /* user-visible */
    TYPE_CLEAR,			/* in clear properties' value slot */
    TYPE_NONE,			/* in uninitialized MOO variables */
    TYPE_CATCH,			/* on-stack marker for an exception handler */
    TYPE_FINALLY,		/* on-stack marker for a TRY-FINALLY clause */
    _TYPE_FLOAT,		/* floating-point number; user-visible */
    _TYPE_MAP,			/* map; user-visible */
    _TYPE_ITER,			/* map iterator; not visible */
    _TYPE_ANON,			/* anonymous object; user-visible */
    /* THE END - complex aliases come next */
    TYPE_STR = (_TYPE_STR | TYPE_COMPLEX_FLAG),
    TYPE_FLOAT = (_TYPE_FLOAT | TYPE_COMPLEX_FLAG),
    TYPE_LIST = (_TYPE_LIST | TYPE_COMPLEX_FLAG),
    TYPE_MAP = (_TYPE_MAP | TYPE_COMPLEX_FLAG),
    TYPE_ITER = (_TYPE_ITER | TYPE_COMPLEX_FLAG),
    TYPE_ANON = (_TYPE_ANON | TYPE_COMPLEX_FLAG)
} var_type;

#define TYPE_ANY ((var_type) -1)	/* wildcard for use in declaring built-ins */
#define TYPE_NUMERIC ((var_type) -2)	/* wildcard for (integer or float) */

typedef struct Var Var;

/* see map.c */
typedef struct rbtree rbtree;
typedef struct rbnode rbnode;
typedef struct rbtrav rbtrav;

/* Experimental.  On the Alpha, DEC cc allows us to specify certain
 * pointers to be 32 bits, but only if we compile and link with "-taso
 * -xtaso" in CFLAGS, which limits us to a 31-bit address space.  This
 * could be a win if your server is thrashing.  Running JHM's db, SIZE
 * went from 50M to 42M.  No doubt these pragmas could be applied
 * elsewhere as well, but I know this at least manages to load and run
 * a non-trivial db.
 */

/* #define SHORT_ALPHA_VAR_POINTERS 1 */

#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size save
#pragma pointer_size short
#endif

/* defined in db_private.h */
typedef struct Object Object;

struct Var {
    union {
	const char *str;	/* STR */
	int32 num;		/* NUM, CATCH, FINALLY */
	Objid obj;		/* OBJ */
	enum error err;		/* ERR */
	Var *list;		/* LIST */
	rbtree *tree;		/* MAP */
	rbtrav *trav;		/* ITER */
	double *fnum;		/* FLOAT */
	Object *anon;		/* ANON */
    } v;
    var_type type;
};

/* generic tuples */
typedef struct var_pair {
    Var a;
    Var b;
} var_pair;

#ifdef SHORT_ALPHA_VAR_POINTERS
#pragma pointer_size restore
#endif

extern Var zero;		/* see numbers.c */
extern Var nothing;		/* see objects.c */
extern Var clear;		/* see objects.c */
extern Var none;		/* see objects.c */

/* Hard limits on string, list and map sizes are imposed mainly to
 * keep malloc calculations from rolling over, and thus preventing the
 * ensuing buffer overruns.  Sizes allow extra space for reference
 * counts and cached length values.  Actual limits imposed on
 * user-constructed maps, lists and strings should generally be
 * smaller (see options.h)
 */
#define MAX_STRING	(INT32_MAX - MIN_STRING_CONCAT_LIMIT)
#define MAX_LIST_VALUE_BYTES_LIMIT	(INT32_MAX - MIN_LIST_VALUE_BYTES_LIMIT)
#define MAX_MAP_VALUE_BYTES_LIMIT	(INT32_MAX - MIN_MAP_VALUE_BYTES_LIMIT)

static inline bool
is_none(Var v)
{
    return TYPE_NONE == v.type;
}

static inline bool
is_collection(Var v)
{
    return TYPE_LIST == v.type || TYPE_MAP == v.type || TYPE_ANON == v.type;
}

static inline bool
is_object(Var v)
{
    return TYPE_OBJ == v.type || TYPE_ANON == v.type;
}

static inline Var
new_int(int32 num)
{
    Var r;
    r.type = TYPE_INT;
    r.v.num = num;
    return r;
}

static inline bool
is_int(Var v)
{
    return TYPE_INT == v.type;
}

static inline Var
new_obj(Objid obj)
{
    Var r;
    r.type = TYPE_OBJ;
    r.v.obj = obj;
    return r;
}

static inline bool
is_obj(Var v)
{
    return TYPE_OBJ == v.type;
}

#endif				/* !Structures_h */
