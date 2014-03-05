#ifndef _U_CURRENT_H_
#define _U_CURRENT_H_

#if defined(MAPI_MODE_UTIL) || defined(MAPI_MODE_GLAPI) || \
    defined(MAPI_MODE_BRIDGE)

#include "glapi/glapi.h"

/* ugly renames to match glapi.h */
#define mapi_table _glapi_table

#ifdef GLX_USE_TLS
#define u_current_table _glapi_tls_Dispatch
#define u_current_context _glapi_tls_Context
#else
#define u_current_table _glapi_Dispatch
#define u_current_context _glapi_Context
#endif

#define u_current_get_table_internal _glapi_get_dispatch
#define u_current_get_context_internal _glapi_get_context

#define u_current_table_tsd _gl_DispatchTSD

#else /* MAPI_MODE_UTIL || MAPI_MODE_GLAPI || MAPI_MODE_BRIDGE */

#include "u_compiler.h"

struct mapi_table;

#ifdef GLX_USE_TLS

extern __thread struct mapi_table *u_current_table
    __attribute__((tls_model("initial-exec")));

extern __thread void *u_current_context
    __attribute__((tls_model("initial-exec")));

#else /* GLX_USE_TLS */

extern struct mapi_table *u_current_table;
extern void *u_current_context;

#endif /* GLX_USE_TLS */

#endif /* MAPI_MODE_UTIL || MAPI_MODE_GLAPI || MAPI_MODE_BRIDGE */

void
u_current_init(void);

void
u_current_destroy(void);

void
u_current_set_table(const struct mapi_table *tbl);

struct mapi_table *
u_current_get_table_internal(void);

void
u_current_set_context(const void *ptr);

void *
u_current_get_context_internal(void);

static INLINE const struct mapi_table *
u_current_get_table(void)
{
#ifdef GLX_USE_TLS
   return u_current_table;
#else
   return (likely(u_current_table) ?
         u_current_table : u_current_get_table_internal());
#endif
}

static INLINE const void *
u_current_get_context(void)
{
#ifdef GLX_USE_TLS
   return u_current_context;
#else
   return likely(u_current_context) ? u_current_context : u_current_get_context_internal();
#endif
}

#endif /* _U_CURRENT_H_ */
