/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


#ifndef COLORTAB_H
#define COLORTAB_H


#include "main/mtypes.h"

#if FEATURE_colortable

#define _MESA_INIT_COLORTABLE_FUNCTIONS(driver, impl)                \
   do {                                                              \
      (driver)->CopyColorTable       = impl ## CopyColorTable;       \
      (driver)->CopyColorSubTable    = impl ## CopyColorSubTable;    \
      (driver)->UpdateTexturePalette = impl ## UpdateTexturePalette; \
   } while (0)

extern void GLAPIENTRY
_mesa_ColorTable( GLenum target, GLenum internalformat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *table );

extern void GLAPIENTRY
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *table );

extern void
_mesa_init_colortable_dispatch(struct _glapi_table *disp);

#else /* FEATURE_colortable */

#define _MESA_INIT_COLORTABLE_FUNCTIONS(driver, impl) do { } while (0)

static INLINE void GLAPIENTRY
_mesa_ColorTable( GLenum target, GLenum internalformat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *table )
{
   ASSERT_NO_FEATURE();
}

static INLINE void GLAPIENTRY
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *table )
{
   ASSERT_NO_FEATURE();
}

static INLINE void
_mesa_init_colortable_dispatch(struct _glapi_table *disp)
{
}

#endif /* FEATURE_colortable */


extern void
_mesa_init_colortable( struct gl_color_table *table );

extern void
_mesa_free_colortable_data( struct gl_color_table *table );

extern void 
_mesa_init_colortables( GLcontext *ctx );

extern void 
_mesa_free_colortables_data( GLcontext *ctx );


#endif /* COLORTAB_H */
