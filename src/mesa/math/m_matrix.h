/* $Id: m_matrix.h,v 1.4 2001/03/12 00:48:41 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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


#ifndef _M_MATRIX_H
#define _M_MATRIX_H



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
#define MAT_FLAG_SINGULAR       0x80
#define MAT_DIRTY_TYPE          0x100
#define MAT_DIRTY_FLAGS         0x200
#define MAT_DIRTY_INVERSE       0x400

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
			    MAT_FLAG_PERSPECTIVE | \
	                    MAT_FLAG_SINGULAR)

#define MAT_DIRTY          (MAT_DIRTY_TYPE | \
			    MAT_DIRTY_FLAGS | \
			    MAT_DIRTY_INVERSE)

#define TEST_MAT_FLAGS(mat, a)  \
    ((MAT_FLAGS_GEOMETRY & (~(a)) & ((mat)->flags) ) == 0)


typedef struct {
   GLfloat *m;		/* 16-byte aligned */
   GLfloat *inv;	/* optional, 16-byte aligned */
   GLuint flags;
   GLuint type;		/* one of the MATRIX_* values */
} GLmatrix;




extern void
_math_matrix_ctr( GLmatrix *m );

extern void
_math_matrix_dtr( GLmatrix *m );

extern void
_math_matrix_alloc_inv( GLmatrix *m );

extern void
_math_matrix_mul_matrix( GLmatrix *dest, const GLmatrix *a, const GLmatrix *b );

extern void
_math_matrix_mul_floats( GLmatrix *dest, const GLfloat *b );

extern void
_math_matrix_loadf( GLmatrix *mat, const GLfloat *m );

extern void
_math_matrix_translate( GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z );

extern void
_math_matrix_rotate( GLmatrix *m, GLfloat angle,
		     GLfloat x, GLfloat y, GLfloat z );

extern void
_math_matrix_scale( GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z );

extern void
_math_matrix_ortho( GLmatrix *mat,
		    GLfloat left, GLfloat right,
		    GLfloat bottom, GLfloat top,
		    GLfloat nearval, GLfloat farval );

extern void
_math_matrix_frustum( GLmatrix *mat,
		      GLfloat left, GLfloat right,
		      GLfloat bottom, GLfloat top,
		      GLfloat nearval, GLfloat farval );

extern void
_math_matrix_set_identity( GLmatrix *dest );

extern void
_math_matrix_copy( GLmatrix *to, const GLmatrix *from );

extern void
_math_matrix_analyse( GLmatrix *mat );

extern void
_math_matrix_print( const GLmatrix *m );




/* Related functions that don't actually operate on GLmatrix structs:
 */
extern void
_math_transposef( GLfloat to[16], const GLfloat from[16] );

extern void
_math_transposed( GLdouble to[16], const GLdouble from[16] );

extern void
_math_transposefd( GLfloat to[16], const GLdouble from[16] );




#endif
