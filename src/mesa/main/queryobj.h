/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef QUERYOBJ_H
#define QUERYOBJ_H


#include "main/mtypes.h"
#include "main/hash.h"


#if FEATURE_queryobj

#define _MESA_INIT_QUERYOBJ_FUNCTIONS(driver, impl)      \
   do {                                                  \
      (driver)->NewQueryObject = impl ## NewQueryObject; \
      (driver)->DeleteQuery    = impl ## DeleteQuery;    \
      (driver)->BeginQuery     = impl ## BeginQuery;     \
      (driver)->EndQuery       = impl ## EndQuery;       \
      (driver)->WaitQuery      = impl ## WaitQuery;      \
      (driver)->CheckQuery     = impl ## CheckQuery;     \
   } while (0)


static INLINE struct gl_query_object *
_mesa_lookup_query_object(GLcontext *ctx, GLuint id)
{
   return (struct gl_query_object *)
      _mesa_HashLookup(ctx->Query.QueryObjects, id);
}


extern void GLAPIENTRY
_mesa_GenQueriesARB(GLsizei n, GLuint *ids);

extern void GLAPIENTRY
_mesa_DeleteQueriesARB(GLsizei n, const GLuint *ids);

extern GLboolean GLAPIENTRY
_mesa_IsQueryARB(GLuint id);

extern void GLAPIENTRY
_mesa_GetQueryivARB(GLenum target, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetQueryObjectivARB(GLuint id, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *params);

extern void
_mesa_init_query_object_functions(struct dd_function_table *driver);

extern void
_mesa_init_queryobj_dispatch(struct _glapi_table *disp);

#else /* FEATURE_queryobj */

#define _MESA_INIT_QUERYOBJ_FUNCTIONS(driver, impl) do { } while (0)

static INLINE void
_mesa_init_query_object_functions(struct dd_function_table *driver)
{
}

static INLINE void
_mesa_init_queryobj_dispatch(struct _glapi_table *disp)
{
}

#endif /* FEATURE_queryobj */

extern void
_mesa_init_queryobj(GLcontext *ctx);

extern void
_mesa_free_queryobj_data(GLcontext *ctx);


#endif /* QUERYOBJ_H */
