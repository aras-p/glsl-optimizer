/**
 * \file eval.h
 * Eval operations.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */

/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef EVAL_H
#define EVAL_H


#include "main/mtypes.h"
#include <stdbool.h>


extern GLuint _mesa_evaluator_components( GLenum target );


extern GLfloat *_mesa_copy_map_points1f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLfloat *points );

extern GLfloat *_mesa_copy_map_points1d( GLenum target,
                                      GLint ustride, GLint uorder,
                                      const GLdouble *points );

extern GLfloat *_mesa_copy_map_points2f( GLenum target,
                                      GLint ustride, GLint uorder,
                                      GLint vstride, GLint vorder,
                                      const GLfloat *points );

extern GLfloat *_mesa_copy_map_points2d(GLenum target,
                                     GLint ustride, GLint uorder,
                                     GLint vstride, GLint vorder,
                                     const GLdouble *points );

extern void
_mesa_install_eval_vtxfmt(struct _glapi_table *disp,
                          const GLvertexformat *vfmt);

extern void _mesa_init_eval( struct gl_context *ctx );
extern void _mesa_free_eval_data( struct gl_context *ctx );

void GLAPIENTRY
_mesa_Map1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride,
             GLint order, const GLfloat *points );
void GLAPIENTRY
_mesa_Map1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride,
             GLint order, const GLdouble *points );
void GLAPIENTRY
_mesa_Map2f( GLenum target,
             GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
             GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
             const GLfloat *points);
void GLAPIENTRY
_mesa_Map2d( GLenum target,
             GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
             GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
             const GLdouble *points );
void GLAPIENTRY
_mesa_GetnMapdvARB( GLenum target, GLenum query, GLsizei bufSize,
                    GLdouble *v );
void GLAPIENTRY
_mesa_GetMapdv( GLenum target, GLenum query, GLdouble *v );
void GLAPIENTRY
_mesa_GetnMapfvARB( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v );
void GLAPIENTRY
_mesa_GetMapfv( GLenum target, GLenum query, GLfloat *v );
void GLAPIENTRY
_mesa_GetnMapivARB( GLenum target, GLenum query, GLsizei bufSize, GLint *v );
void GLAPIENTRY
_mesa_GetMapiv( GLenum target, GLenum query, GLint *v );
void GLAPIENTRY
_mesa_MapGrid1f( GLint un, GLfloat u1, GLfloat u2 );
void GLAPIENTRY
_mesa_MapGrid1d( GLint un, GLdouble u1, GLdouble u2 );
void GLAPIENTRY
_mesa_MapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                 GLint vn, GLfloat v1, GLfloat v2 );
void GLAPIENTRY
_mesa_MapGrid2d( GLint un, GLdouble u1, GLdouble u2,
                 GLint vn, GLdouble v1, GLdouble v2 );

#endif /* EVAL_H */
