/* $Id: eval.h,v 1.1 1999/08/19 00:55:41 jtg Exp $ */

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


#ifndef EVAL_H
#define EVAL_H


#include "types.h"


extern void gl_init_eval( void );


extern void gl_free_control_points( GLcontext *ctx,
                                    GLenum target, GLfloat *data );


extern GLfloat *gl_copy_map_points1f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLfloat *points );

extern GLfloat *gl_copy_map_points1d( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLdouble *points );

extern GLfloat *gl_copy_map_points2f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      GLint vstride, GLint vorder,
                                      const GLfloat *points );

extern GLfloat *gl_copy_map_points2d(GLenum target,
                                     GLint ustride, GLint uorder,
                                     GLint vstride, GLint vorder,
                                     const GLdouble *points );


extern void gl_Map1f( GLcontext* ctx,
                      GLenum target, GLfloat u1, GLfloat u2, GLint stride,
                      GLint order, const GLfloat *points, GLboolean retain );

extern void gl_Map2f( GLcontext* ctx, GLenum target,
                      GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                      GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
                      const GLfloat *points, GLboolean retain );



extern void gl_MapGrid1f( GLcontext* ctx, GLint un, GLfloat u1, GLfloat u2 );

extern void gl_MapGrid2f( GLcontext* ctx,
                          GLint un, GLfloat u1, GLfloat u2,
                          GLint vn, GLfloat v1, GLfloat v2 );

extern void gl_GetMapdv( GLcontext* ctx,
                         GLenum target, GLenum query, GLdouble *v );

extern void gl_GetMapfv( GLcontext* ctx,
                         GLenum target, GLenum query, GLfloat *v );

extern void gl_GetMapiv( GLcontext* ctx,
                         GLenum target, GLenum query, GLint *v );

extern void gl_EvalMesh1( GLcontext* ctx, GLenum mode, GLint i1, GLint i2 );

extern void gl_EvalMesh2( GLcontext* ctx, GLenum mode,
                          GLint i1, GLint i2, GLint j1, GLint j2 );

extern void gl_eval_vb( struct vertex_buffer *VB );


#endif
