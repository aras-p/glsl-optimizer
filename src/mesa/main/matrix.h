/* $Id: matrix.h,v 1.6 2000/06/27 22:10:00 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
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


#ifndef MATRIX_H
#define MATRIX_H


#include "types.h"
#include "config.h"


typedef struct {
   GLfloat *m;			/* 16-byte aligned */
   GLfloat *inv;		/* optional, 16-byte aligned */
   GLuint flags;
   GLuint type;
} GLmatrix;


#ifdef VMS
#define gl_calculate_model_project_matrix gl_calculate_model_project_matr
#endif


extern void gl_matrix_transposef( GLfloat to[16], const GLfloat from[16] );

extern void gl_matrix_transposed( GLdouble to[16], const GLdouble from[16] );


extern void gl_rotation_matrix( GLfloat angle, GLfloat x, GLfloat y, GLfloat z,
                                GLfloat m[] );


extern void gl_mat_mul_floats( GLmatrix *mat, const GLfloat *m, GLuint flags );

extern void gl_mat_mul_mat( GLmatrix *mat, const GLmatrix *mat2 );

extern void gl_calculate_model_project_matrix( GLcontext *ctx );

extern void gl_matrix_copy( GLmatrix *to, const GLmatrix *from );

extern void gl_matrix_ctr( GLmatrix *m );

extern void gl_matrix_dtr( GLmatrix *m );

extern void gl_matrix_alloc_inv( GLmatrix *m );

extern void gl_matrix_mul( GLmatrix *dest, 
			   const GLmatrix *a, 
			   const GLmatrix *b );

extern void gl_matrix_analyze( GLmatrix *mat );

extern GLboolean gl_matrix_invert( GLmatrix *mat );

extern void gl_print_matrix( const GLmatrix *m );



extern void
_mesa_Frustum( GLdouble left, GLdouble right,
               GLdouble bottom, GLdouble top,
               GLdouble nearval, GLdouble farval );

extern void
_mesa_Ortho( GLdouble left, GLdouble right,
             GLdouble bottom, GLdouble top,
             GLdouble nearval, GLdouble farval );

extern void
_mesa_PushMatrix( void );

extern void
_mesa_PopMatrix( void );

extern void
_mesa_LoadIdentity( void );

extern void
_mesa_LoadMatrixf( const GLfloat *m );

extern void
_mesa_LoadMatrixd( const GLdouble *m );

extern void
_mesa_MatrixMode( GLenum mode );

extern void
_mesa_MultMatrixf( const GLfloat *m );

extern void
_mesa_MultMatrixd( const GLdouble *m );

extern void
_mesa_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );

extern void
_mesa_Rotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );

extern void
_mesa_Scalef( GLfloat x, GLfloat y, GLfloat z );

extern void
_mesa_Scaled( GLdouble x, GLdouble y, GLdouble z );

extern void
_mesa_Translatef( GLfloat x, GLfloat y, GLfloat z );

extern void
_mesa_Translated( GLdouble x, GLdouble y, GLdouble z );

extern void
_mesa_LoadTransposeMatrixfARB( const GLfloat *m );

extern void
_mesa_LoadTransposeMatrixdARB( const GLdouble *m );

extern void
_mesa_MultTransposeMatrixfARB( const GLfloat *m );

extern void
_mesa_MultTransposeMatrixdARB( const GLdouble *m );

extern void
_mesa_Viewport( GLint x, GLint y, GLsizei width, GLsizei height );

extern void
gl_Viewport( GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height );

extern void
_mesa_DepthRange( GLclampd nearval, GLclampd farval );



#endif
