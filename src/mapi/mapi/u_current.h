#ifndef _U_CURRENT_H_
#define _U_CURRENT_H_

#include "u_compiler.h"

#ifdef MAPI_GLAPI_CURRENT
#define GLAPI_EXPORT PUBLIC
#else
#define GLAPI_EXPORT
#endif

/*
 * Unlike other utility functions, we need to keep the old names (_glapi_*) for
 * ABI compatibility.  The desired functions are wrappers to the old ones.
 */

struct _glapi_table;

#ifdef GLX_USE_TLS

GLAPI_EXPORT extern __thread struct _glapi_table *_glapi_tls_Dispatch
    __attribute__((tls_model("initial-exec")));

GLAPI_EXPORT extern __thread void *_glapi_tls_Context
    __attribute__((tls_model("initial-exec")));

GLAPI_EXPORT extern const struct _glapi_table *_glapi_Dispatch;
GLAPI_EXPORT extern const void *_glapi_Context;

#else /* GLX_USE_TLS */

GLAPI_EXPORT extern struct _glapi_table *_glapi_Dispatch;
GLAPI_EXPORT extern void *_glapi_Context;

#endif /* GLX_USE_TLS */

GLAPI_EXPORT void
_glapi_check_multithread(void);

GLAPI_EXPORT void
_glapi_set_context(void *context);

GLAPI_EXPORT void *
_glapi_get_context(void);

GLAPI_EXPORT void
_glapi_set_dispatch(struct _glapi_table *dispatch);

GLAPI_EXPORT struct _glapi_table *
_glapi_get_dispatch(void);

void
_glapi_destroy_multithread(void);


struct mapi_table;

static INLINE void
u_current_set(const struct mapi_table *tbl)
{
   _glapi_check_multithread();
   _glapi_set_dispatch((struct _glapi_table *) tbl);
}

static INLINE const struct mapi_table *
u_current_get(void)
{
#ifdef GLX_USE_TLS
   return (const struct mapi_table *) _glapi_tls_Dispatch;
#else
   return (const struct mapi_table *) (likely(_glapi_Dispatch) ?
      _glapi_Dispatch : _glapi_get_dispatch());
#endif
}

static INLINE void
u_current_set_user(void *ptr)
{
   _glapi_check_multithread();
   _glapi_set_context(ptr);
}

static INLINE void *
u_current_get_user(void)
{
#ifdef GLX_USE_TLS
   return _glapi_tls_Context;
#else
   return likely(_glapi_Context) ? _glapi_Context : _glapi_get_context();
#endif
}

#endif /* GLX_USE_TLS */
