/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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

#ifndef FEEDBACK_H
#define FEEDBACK_H


#include "mtypes.h"


extern void
_mesa_init_feedback( GLcontext *ctx );

extern void
_mesa_feedback_vertex( GLcontext *ctx,
                       const GLfloat win[4],
                       const GLfloat color[4],
                       GLfloat index,
                       const GLfloat texcoord[4] );


static INLINE void
_mesa_feedback_token( GLcontext *ctx, GLfloat token )
{
   if (ctx->Feedback.Count < ctx->Feedback.BufferSize) {
      ctx->Feedback.Buffer[ctx->Feedback.Count] = token;
   }
   ctx->Feedback.Count++;
}


extern void
_mesa_update_hitflag( GLcontext *ctx, GLfloat z );


extern void GLAPIENTRY
_mesa_PassThrough( GLfloat token );

extern void GLAPIENTRY
_mesa_FeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer );

extern void GLAPIENTRY
_mesa_SelectBuffer( GLsizei size, GLuint *buffer );

extern void GLAPIENTRY
_mesa_InitNames( void );

extern void GLAPIENTRY
_mesa_LoadName( GLuint name );

extern void GLAPIENTRY
_mesa_PushName( GLuint name );

extern void GLAPIENTRY
_mesa_PopName( void );

extern GLint GLAPIENTRY
_mesa_RenderMode( GLenum mode );


#endif
