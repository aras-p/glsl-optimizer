#ifndef _U_CURRENT_H_
#define _U_CURRENT_H_

#ifdef MAPI_GLAPI_CURRENT

#include "glapi/glapi.h"

/* ugly renames to match glapi.h */
#define mapi_table _glapi_table

#define u_current_table_tls _glapi_tls_Dispatch
#define u_current_user_tls _glapi_tls_Context
#define u_current_table _glapi_Dispatch
#define u_current_user _glapi_Context

#define u_current_destroy _glapi_destroy_multithread
#define u_current_init _glapi_check_multithread
#define u_current_set_internal _glapi_set_dispatch
#define u_current_get_internal _glapi_get_dispatch
#define u_current_set_user_internal _glapi_set_context
#define u_current_get_user_internal _glapi_get_context

#define u_current_table_tsd _gl_DispatchTSD

#else /* MAPI_GLAPI_CURRENT */

#include "u_compiler.h"

struct mapi_table;

#ifdef GLX_USE_TLS

extern __thread struct mapi_table *u_current_table_tls
    __attribute__((tls_model("initial-exec")));

extern __thread void *u_current_user_tls
    __attribute__((tls_model("initial-exec")));

extern const struct mapi_table *u_current_table;
extern const void *u_current_user;

#else /* GLX_USE_TLS */

extern struct mapi_table *u_current_table;
extern void *u_current_user;

#endif /* GLX_USE_TLS */

void
u_current_init(void);

void
u_current_destroy(void);

void
u_current_set_internal(struct mapi_table *tbl);

struct mapi_table *
u_current_get_internal(void);

void
u_current_set_user_internal(void *ptr);

void *
u_current_get_user_internal(void);

static INLINE void
u_current_set(const struct mapi_table *tbl)
{
   u_current_set_internal((struct mapi_table *) tbl);
}

static INLINE const struct mapi_table *
u_current_get(void)
{
#ifdef GLX_USE_TLS
   return (const struct mapi_table *) u_current_table_tls;
#else
   return (likely(u_current_table) ?
         (const struct mapi_table *) u_current_table : u_current_get_internal());
#endif
}

static INLINE void
u_current_set_user(void *ptr)
{
   u_current_set_internal(ptr);
}

static INLINE void *
u_current_get_user(void)
{
#ifdef GLX_USE_TLS
   return u_current_user_tls;
#else
   return likely(u_current_user) ? u_current_user : u_current_get_user_internal();
#endif
}

#endif /* MAPI_GLAPI_CURRENT */

#endif /* _U_CURRENT_H_ */
