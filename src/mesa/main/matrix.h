/* $Id: matrix.h,v 1.7 2000/09/17 21:56:07 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


/* Give symbolic names to some of the entries in the matrix to help
 * out with the rework of the viewport_map as a matrix transform.
 */
#define MAT_SX 0
#define MAT_SY 5
#define MAT_SZ 10
#define MAT_TX 12
#define MAT_TY 13
#define MAT_TZ 14


/*
 * Different kinds of 4x4 transformation matrices:
 */
#define MATRIX_GENERAL		0	/* general 4x4 matrix */
#define MATRIX_IDENTITY		1	/* identity matrix */
#define MATRIX_3D_NO_ROT	2	/* ortho projection and others... */
#define MATRIX_PERSPECTIVE	3	/* perspective projection matrix */
#define MATRIX_2D		4	/* 2-D transformation */
#define MATRIX_2D_NO_ROT	5	/* 2-D scale & translate only */
#define MATRIX_3D		6	/* 3-D transformation */

#define MAT_FLAG_IDENTITY       0
#define MAT_FLAG_GENERAL        0x1
#define MAT_FLAG_ROTATION       0x2
#define MAT_FLAG_TRANSLATION    0x4
#define MAT_FLAG_UNIFORM_SCALE  0x8
#define MAT_FLAG_GENERAL_SCALE  0x10
#define MAT_FLAG_GENERAL_3D     0x20
#define MAT_FLAG_PERSPECTIVE    0x40
#define MAT_DIRTY_TYPE          0x80
#define MAT_DIRTY_FLAGS         0x100
#define MAT_DIRTY_INVERSE       0x200
#define MAT_DIRTY_DEPENDENTS    0x400

#define MAT_FLAGS_ANGLE_PRESERVING (MAT_FLAG_ROTATION | \
				    MAT_FLAG_TRANSLATION | \
				    MAT_FLAG_UNIFORM_SCALE)

#define MAT_FLAGS_LENGTH_PRESERVING (MAT_FLAG_ROTATION | \
				     MAT_FLAG_TRANSLATION)

#define MAT_FLAGS_3D (MAT_FLAG_ROTATION | \
		      MAT_FLAG_TRANSLATION | \
		      MAT_FLAG_UNIFORM_SCALE | \
		      MAT_FLAG_GENERAL_SCALE | \
		      MAT_FLAG_GENERAL_3D)

#define MAT_FLAGS_GEOMETRY (MAT_FLAG_GENERAL | \
			    MAT_FLAG_ROTATION | \
			    MAT_FLAG_TRANSLATION | \
			    MAT_FLAG_UNIFORM_SCALE | \
			    MAT_FLAG_GENERAL_SCALE | \
			    MAT_FLAG_GENERAL_3D | \
			    MAT_FLAG_PERSPECTIVE)

#define MAT_DIRTY_ALL_OVER (MAT_DIRTY_TYPE | \
			    MAT_DIRTY_DEPENDENTS | \
			    MAT_DIRTY_FLAGS | \
			    MAT_DIRTY_INVERSE)

#define TEST_MAT_FLAGS(mat, a)  ((MAT_FLAGS_GEOMETRY&(~(a))&((mat)->flags))==0)




typedef struct {
   GLfloat *m;		/* 16-byte aligned */
   GLfloat *inv;	/* optional, 16-byte aligned */
   GLuint flags;
   GLuint type;		/* one of the MATRIX_* values */
} GLmatrix;


#ifdef VMS
#define gl_calculate_model_project_matrix gl_calculate_model_project_matr
#endif


extern void
gl_matrix_transposef( GLfloat to[16], const GLfloat from[16] );

extern void
gl_matrix_transposed( GLdouble to[16], const GLdouble from[16] );


extern void
gl_rotation_matrix( GLfloat angle, GLfloat x, GLfloat y, GLfloat z,
                    GLfloat m[] );


extern void
gl_calculate_model_project_matrix( GLcontext *ctx );

extern void
gl_matrix_ctr( GLmatrix *m );

extern void
gl_matrix_dtr( GLmatrix *m );

extern void
gl_matrix_alloc_inv( GLmatrix *m );

extern void
gl_matrix_mul( GLmatrix *dest, const GLmatrix *a, const GLmatrix *b );

extern void
gl_matrix_analyze( GLmatrix *mat );

extern void
gl_print_matrix( const GLmatrix *m );



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
