/* $Id: feedback.h,v 1.2 1999/09/18 20:41:23 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


#include "types.h"


#define FEEDBACK_TOKEN( CTX, T )				\
	if (CTX->Feedback.Count < CTX->Feedback.BufferSize) {	\
	   CTX->Feedback.Buffer[CTX->Feedback.Count] = (GLfloat) (T); \
	}							\
	CTX->Feedback.Count++;


extern void gl_feedback_vertex( GLcontext *ctx,
                                const GLfloat win[4],
                                const GLfloat color[4], 
				GLuint index,
                                const GLfloat texcoord[4] );


extern void gl_update_hitflag( GLcontext *ctx, GLfloat z );


extern void gl_PassThrough( GLcontext *ctx, GLfloat token );

extern void gl_FeedbackBuffer( GLcontext *ctx, GLsizei size,
                               GLenum type, GLfloat *buffer );

extern void gl_SelectBuffer( GLcontext *ctx, GLsizei size, GLuint *buffer );

extern void gl_InitNames( GLcontext *ctx );

extern void gl_LoadName( GLcontext *ctx, GLuint name );

extern void gl_PushName( GLcontext *ctx, GLuint name );

extern void gl_PopName( GLcontext *ctx );

extern GLint gl_RenderMode( GLcontext *ctx, GLenum mode );

extern void gl_feedback_points( GLcontext *ctx, GLuint first, GLuint last );
extern void gl_feedback_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv );
extern void gl_feedback_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
				  GLuint v2, GLuint pv );

extern void gl_select_points( GLcontext *ctx, GLuint first, GLuint last );
extern void gl_select_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv );
extern void gl_select_triangle( GLcontext *ctx, GLuint v0, GLuint v1,
				GLuint v2, GLuint pv );

#endif

