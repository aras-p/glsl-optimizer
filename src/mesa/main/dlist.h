
/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#ifndef DLIST_H
#define DLIST_H


#include "mtypes.h"


#define ASSERT_OUTSIDE_SAVE_BEGIN_END_WITH_RETVAL(ctx, retval)		\
do {									\
   if (ctx->Driver.CurrentSavePrimitive <= GL_POLYGON ||		\
       ctx->Driver.CurrentSavePrimitive == PRIM_INSIDE_UNKNOWN_PRIM) {	\
      _mesa_compile_error( ctx, GL_INVALID_OPERATION, "begin/end" );	\
      return retval;							\
   }									\
} while (0)

#define ASSERT_OUTSIDE_SAVE_BEGIN_END(ctx)				\
do {									\
   if (ctx->Driver.CurrentSavePrimitive <= GL_POLYGON ||		\
       ctx->Driver.CurrentSavePrimitive == PRIM_INSIDE_UNKNOWN_PRIM) {	\
      _mesa_compile_error( ctx, GL_INVALID_OPERATION, "begin/end" );	\
      return;								\
   }									\
} while (0)

#define ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx)			\
do {									\
   ASSERT_OUTSIDE_SAVE_BEGIN_END(ctx);					\
   FLUSH_VERTICES(ctx, 0);						\
} while (0)

#define ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, retval)\
do {									\
   ASSERT_OUTSIDE_SAVE_BEGIN_END_WITH_RETVAL(ctx, retval);		\
   FLUSH_VERTICES(ctx, 0);						\
} while (0)


extern void _mesa_init_lists( void );

extern void _mesa_destroy_list( GLcontext *ctx, GLuint list );

extern void _mesa_CallList( GLuint list );

extern void _mesa_CallLists( GLsizei n, GLenum type, const GLvoid *lists );

extern void _mesa_DeleteLists( GLuint list, GLsizei range );

extern void _mesa_EndList( void );

extern GLuint _mesa_GenLists( GLsizei range );

extern GLboolean _mesa_IsList( GLuint list );

extern void _mesa_ListBase( GLuint base );

extern void _mesa_NewList( GLuint list, GLenum mode );

extern void _mesa_init_dlist_table( struct _glapi_table *table,
                                    GLuint tableSize );

extern void _mesa_save_error( GLcontext *ctx, GLenum error, const char *s );

extern void _mesa_compile_error( GLcontext *ctx, GLenum error, const char *s );


extern void *_mesa_alloc_instruction( GLcontext *ctx, int opcode, GLint sz );

extern int _mesa_alloc_opcode( GLcontext *ctx, GLuint sz,
                               void (*execute)( GLcontext *, void * ),
                               void (*destroy)( GLcontext *, void * ),
                               void (*print)( GLcontext *, void * ) );

extern void _mesa_save_EvalMesh2(GLenum mode, GLint i1, GLint i2,
				 GLint j1, GLint j2 );
extern void _mesa_save_EvalMesh1( GLenum mode, GLint i1, GLint i2 );
extern void _mesa_save_CallLists( GLsizei n, GLenum type, const GLvoid *lists );
extern void _mesa_save_CallList( GLuint list );



#endif
