/* $Id: matrix.h,v 1.2 1999/10/08 09:27:11 keithw Exp $ */

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





#ifndef MATRIX_H
#define MATRIX_H


#include "GL/gl.h"
#include "config.h"

typedef struct {
   GLfloat m[16];
   GLfloat *inv;		/* optional */
   GLuint flags;
   GLuint type;
} GLmatrix;

#ifdef VMS
#define gl_calculate_model_project_matrix gl_calculate_model_project_matr
#endif


extern void gl_rotation_matrix( GLfloat angle, GLfloat x, GLfloat y, GLfloat z,
                                GLfloat m[] );



extern void gl_Frustum( GLcontext *ctx,
                        GLdouble left, GLdouble right,
                        GLdouble bottom, GLdouble top,
                        GLdouble nearval, GLdouble farval );

extern void gl_Ortho( GLcontext *ctx,
                      GLdouble left, GLdouble right,
                      GLdouble bottom, GLdouble top,
                      GLdouble nearval, GLdouble farval );

extern void gl_PushMatrix( GLcontext *ctx );

extern void gl_PopMatrix( GLcontext *ctx );

extern void gl_LoadIdentity( GLcontext *ctx );

extern void gl_LoadMatrixf( GLcontext *ctx, const GLfloat *m );

extern void gl_MatrixMode( GLcontext *ctx, GLenum mode );

extern void gl_MultMatrixf( GLcontext *ctx, const GLfloat *m );

extern void gl_mat_mul_floats( GLmatrix *mat, const GLfloat *m, GLuint flags );
extern void gl_mat_mul_mat( GLmatrix *mat, const GLmatrix *mat2 );

extern void gl_Rotatef( GLcontext *ctx,
                        GLfloat angle, GLfloat x, GLfloat y, GLfloat z );

extern void gl_Scalef( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z );

extern void gl_Translatef( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z );

extern void gl_Viewport( GLcontext *ctx,
                         GLint x, GLint y, GLsizei width, GLsizei height );

extern void gl_DepthRange( GLcontext* ctx, GLclampd nearval, GLclampd farval );





extern void gl_calculate_model_project_matrix( GLcontext *ctx );


extern void gl_matrix_ctr( GLmatrix *m );

extern void gl_matrix_dtr( GLmatrix *m );

extern void gl_matrix_alloc_inv( GLmatrix *m );

extern void gl_matrix_copy( GLmatrix *to, const GLmatrix *from );

extern void gl_matrix_mul( GLmatrix *dest, 
			   const GLmatrix *a, 
			   const GLmatrix *b );

extern void gl_matrix_analyze( GLmatrix *mat );


extern void gl_MultMatrixd( GLcontext *ctx, const GLdouble *m );
extern GLboolean gl_matrix_invert( GLmatrix *mat );
extern void gl_print_matrix( const GLmatrix *m );


#endif
