/* $Id: m_matrix.c,v 1.8 2001/03/12 00:48:41 gareth Exp $ */

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


/*
 * Matrix operations
 *
 * NOTES:
 * 1. 4x4 transformation matrices are stored in memory in column major order.
 * 2. Points/vertices are to be thought of as column vectors.
 * 3. Transformation of a point p by a matrix M is: p' = M * p
 */

#include <math.h>

#include "glheader.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"

#include "m_matrix.h"


static const char *types[] = {
   "MATRIX_GENERAL",
   "MATRIX_IDENTITY",
   "MATRIX_3D_NO_ROT",
   "MATRIX_PERSPECTIVE",
   "MATRIX_2D",
   "MATRIX_2D_NO_ROT",
   "MATRIX_3D"
};


static GLfloat Identity[16] = {
   1.0, 0.0, 0.0, 0.0,
   0.0, 1.0, 0.0, 0.0,
   0.0, 0.0, 1.0, 0.0,
   0.0, 0.0, 0.0, 1.0
};




/*
 * This matmul was contributed by Thomas Malik
 *
 * Perform a 4x4 matrix multiplication  (product = a x b).
 * Input:  a, b - matrices to multiply
 * Output:  product - product of a and b
 * WARNING: (product != b) assumed
 * NOTE:    (product == a) allowed
 *
 * KW: 4*16 = 64 muls
 */
#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]

static void matmul4( GLfloat *product, const GLfloat *a, const GLfloat *b )
{
   GLint i;
   for (i = 0; i < 4; i++) {
      const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
      P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
      P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
      P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
      P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
   }
}


/* Multiply two matrices known to occupy only the top three rows, such
 * as typical model matrices, and ortho matrices.
 */
static void matmul34( GLfloat *product, const GLfloat *a, const GLfloat *b )
{
   GLint i;
   for (i = 0; i < 3; i++) {
      const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
      P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0);
      P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1);
      P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2);
      P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3;
   }
   P(3,0) = 0;
   P(3,1) = 0;
   P(3,2) = 0;
   P(3,3) = 1;
}


#undef A
#undef B
#undef P


/*
 * Multiply a matrix by an array of floats with known properties.
 */
static void matrix_multf( GLmatrix *mat, const GLfloat *m, GLuint flags )
{
   mat->flags |= (flags | MAT_DIRTY_TYPE | MAT_DIRTY_INVERSE);

   if (TEST_MAT_FLAGS(mat, MAT_FLAGS_3D))
      matmul34( mat->m, mat->m, m );
   else
      matmul4( mat->m, mat->m, m );
}


static void print_matrix_floats( const GLfloat m[16] )
{
   int i;
   for (i=0;i<4;i++) {
      fprintf(stderr,"\t%f %f %f %f\n", m[i], m[4+i], m[8+i], m[12+i] );
   }
}

void
_math_matrix_print( const GLmatrix *m )
{
   fprintf(stderr, "Matrix type: %s, flags: %x\n", types[m->type], m->flags);
   print_matrix_floats(m->m);
   fprintf(stderr, "Inverse: \n");
   if (m->inv) {
      GLfloat prod[16];
      print_matrix_floats(m->inv);
      matmul4(prod, m->m, m->inv);
      fprintf(stderr, "Mat * Inverse:\n");
      print_matrix_floats(prod);
   }
   else {
      fprintf(stderr, "  - not available\n");
   }
}




#define SWAP_ROWS(a, b) { GLfloat *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

/*
 * Compute inverse of 4x4 transformation matrix.
 * Code contributed by Jacques Leroy jle@star.be
 * Return GL_TRUE for success, GL_FALSE for failure (singular matrix)
 */
static GLboolean invert_matrix_general( GLmatrix *mat )
{
   const GLfloat *m = mat->m;
   GLfloat *out = mat->inv;
   GLfloat wtmp[4][8];
   GLfloat m0, m1, m2, m3, s;
   GLfloat *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m,0,0), r0[1] = MAT(m,0,1),
   r0[2] = MAT(m,0,2), r0[3] = MAT(m,0,3),
   r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

   r1[0] = MAT(m,1,0), r1[1] = MAT(m,1,1),
   r1[2] = MAT(m,1,2), r1[3] = MAT(m,1,3),
   r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

   r2[0] = MAT(m,2,0), r2[1] = MAT(m,2,1),
   r2[2] = MAT(m,2,2), r2[3] = MAT(m,2,3),
   r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

   r3[0] = MAT(m,3,0), r3[1] = MAT(m,3,1),
   r3[2] = MAT(m,3,2), r3[3] = MAT(m,3,3),
   r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
   if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
   if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
   if (0.0 == r0[0])  return GL_FALSE;

   /* eliminate first variable     */
   m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
   s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
   s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
   s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r0[5];
   if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r0[6];
   if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r0[7];
   if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

   /* choose pivot - or die */
   if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
   if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
   if (0.0 == r1[1])  return GL_FALSE;

   /* eliminate second variable */
   m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
   r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
   s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

   /* choose pivot - or die */
   if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
   if (0.0 == r2[2])  return GL_FALSE;

   /* eliminate third variable */
   m3 = r3[2]/r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
   r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
   r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3]) return GL_FALSE;

   s = 1.0/r3[3];              /* now back substitute row 3 */
   r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

   m2 = r2[3];                 /* now back substitute row 2 */
   s  = 1.0/r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
   r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
   r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
   r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];                 /* now back substitute row 1 */
   s  = 1.0/r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
   r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
   r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];                 /* now back substitute row 0 */
   s  = 1.0/r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
   r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out,0,0) = r0[4]; MAT(out,0,1) = r0[5],
   MAT(out,0,2) = r0[6]; MAT(out,0,3) = r0[7],
   MAT(out,1,0) = r1[4]; MAT(out,1,1) = r1[5],
   MAT(out,1,2) = r1[6]; MAT(out,1,3) = r1[7],
   MAT(out,2,0) = r2[4]; MAT(out,2,1) = r2[5],
   MAT(out,2,2) = r2[6]; MAT(out,2,3) = r2[7],
   MAT(out,3,0) = r3[4]; MAT(out,3,1) = r3[5],
   MAT(out,3,2) = r3[6]; MAT(out,3,3) = r3[7];

   return GL_TRUE;
}
#undef SWAP_ROWS


/* Adapted from graphics gems II.
 */
static GLboolean invert_matrix_3d_general( GLmatrix *mat )
{
   const GLfloat *in = mat->m;
   GLfloat *out = mat->inv;
   GLfloat pos, neg, t;
   GLfloat det;

   /* Calculate the determinant of upper left 3x3 submatrix and
    * determine if the matrix is singular.
    */
   pos = neg = 0.0;
   t =  MAT(in,0,0) * MAT(in,1,1) * MAT(in,2,2);
   if (t >= 0.0) pos += t; else neg += t;

   t =  MAT(in,1,0) * MAT(in,2,1) * MAT(in,0,2);
   if (t >= 0.0) pos += t; else neg += t;

   t =  MAT(in,2,0) * MAT(in,0,1) * MAT(in,1,2);
   if (t >= 0.0) pos += t; else neg += t;

   t = -MAT(in,2,0) * MAT(in,1,1) * MAT(in,0,2);
   if (t >= 0.0) pos += t; else neg += t;

   t = -MAT(in,1,0) * MAT(in,0,1) * MAT(in,2,2);
   if (t >= 0.0) pos += t; else neg += t;

   t = -MAT(in,0,0) * MAT(in,2,1) * MAT(in,1,2);
   if (t >= 0.0) pos += t; else neg += t;

   det = pos + neg;

   if (det*det < 1e-25)
      return GL_FALSE;

   det = 1.0 / det;
   MAT(out,0,0) = (  (MAT(in,1,1)*MAT(in,2,2) - MAT(in,2,1)*MAT(in,1,2) )*det);
   MAT(out,0,1) = (- (MAT(in,0,1)*MAT(in,2,2) - MAT(in,2,1)*MAT(in,0,2) )*det);
   MAT(out,0,2) = (  (MAT(in,0,1)*MAT(in,1,2) - MAT(in,1,1)*MAT(in,0,2) )*det);
   MAT(out,1,0) = (- (MAT(in,1,0)*MAT(in,2,2) - MAT(in,2,0)*MAT(in,1,2) )*det);
   MAT(out,1,1) = (  (MAT(in,0,0)*MAT(in,2,2) - MAT(in,2,0)*MAT(in,0,2) )*det);
   MAT(out,1,2) = (- (MAT(in,0,0)*MAT(in,1,2) - MAT(in,1,0)*MAT(in,0,2) )*det);
   MAT(out,2,0) = (  (MAT(in,1,0)*MAT(in,2,1) - MAT(in,2,0)*MAT(in,1,1) )*det);
   MAT(out,2,1) = (- (MAT(in,0,0)*MAT(in,2,1) - MAT(in,2,0)*MAT(in,0,1) )*det);
   MAT(out,2,2) = (  (MAT(in,0,0)*MAT(in,1,1) - MAT(in,1,0)*MAT(in,0,1) )*det);

   /* Do the translation part */
   MAT(out,0,3) = - (MAT(in,0,3) * MAT(out,0,0) +
		     MAT(in,1,3) * MAT(out,0,1) +
		     MAT(in,2,3) * MAT(out,0,2) );
   MAT(out,1,3) = - (MAT(in,0,3) * MAT(out,1,0) +
		     MAT(in,1,3) * MAT(out,1,1) +
		     MAT(in,2,3) * MAT(out,1,2) );
   MAT(out,2,3) = - (MAT(in,0,3) * MAT(out,2,0) +
		     MAT(in,1,3) * MAT(out,2,1) +
		     MAT(in,2,3) * MAT(out,2,2) );

   return GL_TRUE;
}


static GLboolean invert_matrix_3d( GLmatrix *mat )
{
   const GLfloat *in = mat->m;
   GLfloat *out = mat->inv;

   if (!TEST_MAT_FLAGS(mat, MAT_FLAGS_ANGLE_PRESERVING)) {
      return invert_matrix_3d_general( mat );
   }

   if (mat->flags & MAT_FLAG_UNIFORM_SCALE) {
      GLfloat scale = (MAT(in,0,0) * MAT(in,0,0) +
                       MAT(in,0,1) * MAT(in,0,1) +
                       MAT(in,0,2) * MAT(in,0,2));

      if (scale == 0.0)
         return GL_FALSE;

      scale = 1.0 / scale;

      /* Transpose and scale the 3 by 3 upper-left submatrix. */
      MAT(out,0,0) = scale * MAT(in,0,0);
      MAT(out,1,0) = scale * MAT(in,0,1);
      MAT(out,2,0) = scale * MAT(in,0,2);
      MAT(out,0,1) = scale * MAT(in,1,0);
      MAT(out,1,1) = scale * MAT(in,1,1);
      MAT(out,2,1) = scale * MAT(in,1,2);
      MAT(out,0,2) = scale * MAT(in,2,0);
      MAT(out,1,2) = scale * MAT(in,2,1);
      MAT(out,2,2) = scale * MAT(in,2,2);
   }
   else if (mat->flags & MAT_FLAG_ROTATION) {
      /* Transpose the 3 by 3 upper-left submatrix. */
      MAT(out,0,0) = MAT(in,0,0);
      MAT(out,1,0) = MAT(in,0,1);
      MAT(out,2,0) = MAT(in,0,2);
      MAT(out,0,1) = MAT(in,1,0);
      MAT(out,1,1) = MAT(in,1,1);
      MAT(out,2,1) = MAT(in,1,2);
      MAT(out,0,2) = MAT(in,2,0);
      MAT(out,1,2) = MAT(in,2,1);
      MAT(out,2,2) = MAT(in,2,2);
   }
   else {
      /* pure translation */
      MEMCPY( out, Identity, sizeof(Identity) );
      MAT(out,0,3) = - MAT(in,0,3);
      MAT(out,1,3) = - MAT(in,1,3);
      MAT(out,2,3) = - MAT(in,2,3);
      return GL_TRUE;
   }

   if (mat->flags & MAT_FLAG_TRANSLATION) {
      /* Do the translation part */
      MAT(out,0,3) = - (MAT(in,0,3) * MAT(out,0,0) +
			MAT(in,1,3) * MAT(out,0,1) +
			MAT(in,2,3) * MAT(out,0,2) );
      MAT(out,1,3) = - (MAT(in,0,3) * MAT(out,1,0) +
			MAT(in,1,3) * MAT(out,1,1) +
			MAT(in,2,3) * MAT(out,1,2) );
      MAT(out,2,3) = - (MAT(in,0,3) * MAT(out,2,0) +
			MAT(in,1,3) * MAT(out,2,1) +
			MAT(in,2,3) * MAT(out,2,2) );
   }
   else {
      MAT(out,0,3) = MAT(out,1,3) = MAT(out,2,3) = 0.0;
   }

   return GL_TRUE;
}



static GLboolean invert_matrix_identity( GLmatrix *mat )
{
   MEMCPY( mat->inv, Identity, sizeof(Identity) );
   return GL_TRUE;
}


static GLboolean invert_matrix_3d_no_rot( GLmatrix *mat )
{
   const GLfloat *in = mat->m;
   GLfloat *out = mat->inv;

   if (MAT(in,0,0) == 0 || MAT(in,1,1) == 0 || MAT(in,2,2) == 0 )
      return GL_FALSE;

   MEMCPY( out, Identity, 16 * sizeof(GLfloat) );
   MAT(out,0,0) = 1.0 / MAT(in,0,0);
   MAT(out,1,1) = 1.0 / MAT(in,1,1);
   MAT(out,2,2) = 1.0 / MAT(in,2,2);

   if (mat->flags & MAT_FLAG_TRANSLATION) {
      MAT(out,0,3) = - (MAT(in,0,3) * MAT(out,0,0));
      MAT(out,1,3) = - (MAT(in,1,3) * MAT(out,1,1));
      MAT(out,2,3) = - (MAT(in,2,3) * MAT(out,2,2));
   }

   return GL_TRUE;
}


static GLboolean invert_matrix_2d_no_rot( GLmatrix *mat )
{
   const GLfloat *in = mat->m;
   GLfloat *out = mat->inv;

   if (MAT(in,0,0) == 0 || MAT(in,1,1) == 0)
      return GL_FALSE;

   MEMCPY( out, Identity, 16 * sizeof(GLfloat) );
   MAT(out,0,0) = 1.0 / MAT(in,0,0);
   MAT(out,1,1) = 1.0 / MAT(in,1,1);

   if (mat->flags & MAT_FLAG_TRANSLATION) {
      MAT(out,0,3) = - (MAT(in,0,3) * MAT(out,0,0));
      MAT(out,1,3) = - (MAT(in,1,3) * MAT(out,1,1));
   }

   return GL_TRUE;
}


static GLboolean invert_matrix_perspective( GLmatrix *mat )
{
   const GLfloat *in = mat->m;
   GLfloat *out = mat->inv;

   if (MAT(in,2,3) == 0)
      return GL_FALSE;

   MEMCPY( out, Identity, 16 * sizeof(GLfloat) );

   MAT(out,0,0) = 1.0 / MAT(in,0,0);
   MAT(out,1,1) = 1.0 / MAT(in,1,1);

   MAT(out,0,3) = MAT(in,0,2);
   MAT(out,1,3) = MAT(in,1,2);

   MAT(out,2,2) = 0;
   MAT(out,2,3) = -1;

   MAT(out,3,2) = 1.0 / MAT(in,2,3);
   MAT(out,3,3) = MAT(in,2,2) * MAT(out,3,2);

   return GL_TRUE;
}


typedef GLboolean (*inv_mat_func)( GLmatrix *mat );


static inv_mat_func inv_mat_tab[7] = {
   invert_matrix_general,
   invert_matrix_identity,
   invert_matrix_3d_no_rot,
   invert_matrix_perspective,
   invert_matrix_3d,		/* lazy! */
   invert_matrix_2d_no_rot,
   invert_matrix_3d
};


static GLboolean matrix_invert( GLmatrix *mat )
{
   if (inv_mat_tab[mat->type](mat)) {
      mat->flags &= ~MAT_FLAG_SINGULAR;
      return GL_TRUE;
   } else {
      mat->flags |= MAT_FLAG_SINGULAR;
      MEMCPY( mat->inv, Identity, sizeof(Identity) );
      return GL_FALSE;
   }
}






/*
 * Generate a 4x4 transformation matrix from glRotate parameters, and
 * postmultiply the input matrix by it.
 */
void
_math_matrix_rotate( GLmatrix *mat,
		     GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
   /* This function contributed by Erich Boleyn (erich@uruk.org) */
   GLfloat mag, s, c;
   GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;
   GLfloat m[16];

   s = sin( angle * DEG2RAD );
   c = cos( angle * DEG2RAD );

   mag = GL_SQRT( x*x + y*y + z*z );

   if (mag <= 1.0e-4) {
      /* generate an identity matrix and return */
      MEMCPY(m, Identity, sizeof(GLfloat)*16);
      return;
   }

   x /= mag;
   y /= mag;
   z /= mag;

#define M(row,col)  m[col*4+row]

   /*
    *     Arbitrary axis rotation matrix.
    *
    *  This is composed of 5 matrices, Rz, Ry, T, Ry', Rz', multiplied
    *  like so:  Rz * Ry * T * Ry' * Rz'.  T is the final rotation
    *  (which is about the X-axis), and the two composite transforms
    *  Ry' * Rz' and Rz * Ry are (respectively) the rotations necessary
    *  from the arbitrary axis to the X-axis then back.  They are
    *  all elementary rotations.
    *
    *  Rz' is a rotation about the Z-axis, to bring the axis vector
    *  into the x-z plane.  Then Ry' is applied, rotating about the
    *  Y-axis to bring the axis vector parallel with the X-axis.  The
    *  rotation about the X-axis is then performed.  Ry and Rz are
    *  simply the respective inverse transforms to bring the arbitrary
    *  axis back to it's original orientation.  The first transforms
    *  Rz' and Ry' are considered inverses, since the data from the
    *  arbitrary axis gives you info on how to get to it, not how
    *  to get away from it, and an inverse must be applied.
    *
    *  The basic calculation used is to recognize that the arbitrary
    *  axis vector (x, y, z), since it is of unit length, actually
    *  represents the sines and cosines of the angles to rotate the
    *  X-axis to the same orientation, with theta being the angle about
    *  Z and phi the angle about Y (in the order described above)
    *  as follows:
    *
    *  cos ( theta ) = x / sqrt ( 1 - z^2 )
    *  sin ( theta ) = y / sqrt ( 1 - z^2 )
    *
    *  cos ( phi ) = sqrt ( 1 - z^2 )
    *  sin ( phi ) = z
    *
    *  Note that cos ( phi ) can further be inserted to the above
    *  formulas:
    *
    *  cos ( theta ) = x / cos ( phi )
    *  sin ( theta ) = y / sin ( phi )
    *
    *  ...etc.  Because of those relations and the standard trigonometric
    *  relations, it is pssible to reduce the transforms down to what
    *  is used below.  It may be that any primary axis chosen will give the
    *  same results (modulo a sign convention) using thie method.
    *
    *  Particularly nice is to notice that all divisions that might
    *  have caused trouble when parallel to certain planes or
    *  axis go away with care paid to reducing the expressions.
    *  After checking, it does perform correctly under all cases, since
    *  in all the cases of division where the denominator would have
    *  been zero, the numerator would have been zero as well, giving
    *  the expected result.
    */

   xx = x * x;
   yy = y * y;
   zz = z * z;
   xy = x * y;
   yz = y * z;
   zx = z * x;
   xs = x * s;
   ys = y * s;
   zs = z * s;
   one_c = 1.0F - c;

   M(0,0) = (one_c * xx) + c;
   M(0,1) = (one_c * xy) - zs;
   M(0,2) = (one_c * zx) + ys;
   M(0,3) = 0.0F;

   M(1,0) = (one_c * xy) + zs;
   M(1,1) = (one_c * yy) + c;
   M(1,2) = (one_c * yz) - xs;
   M(1,3) = 0.0F;

   M(2,0) = (one_c * zx) - ys;
   M(2,1) = (one_c * yz) + xs;
   M(2,2) = (one_c * zz) + c;
   M(2,3) = 0.0F;

   M(3,0) = 0.0F;
   M(3,1) = 0.0F;
   M(3,2) = 0.0F;
   M(3,3) = 1.0F;

#undef M

   matrix_multf( mat, m, MAT_FLAG_ROTATION );
}


void
_math_matrix_frustum( GLmatrix *mat,
		      GLfloat left, GLfloat right,
		      GLfloat bottom, GLfloat top,
		      GLfloat nearval, GLfloat farval )
{
   GLfloat x, y, a, b, c, d;
   GLfloat m[16];

   x = (2.0*nearval) / (right-left);
   y = (2.0*nearval) / (top-bottom);
   a = (right+left) / (right-left);
   b = (top+bottom) / (top-bottom);
   c = -(farval+nearval) / ( farval-nearval);
   d = -(2.0*farval*nearval) / (farval-nearval);  /* error? */

#define M(row,col)  m[col*4+row]
   M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
   M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
   M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
   M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M

   matrix_multf( mat, m, MAT_FLAG_PERSPECTIVE );
}

void
_math_matrix_ortho( GLmatrix *mat,
		    GLfloat left, GLfloat right,
		    GLfloat bottom, GLfloat top,
		    GLfloat nearval, GLfloat farval )
{
   GLfloat x, y, z;
   GLfloat tx, ty, tz;
   GLfloat m[16];

   x = 2.0 / (right-left);
   y = 2.0 / (top-bottom);
   z = -2.0 / (farval-nearval);
   tx = -(right+left) / (right-left);
   ty = -(top+bottom) / (top-bottom);
   tz = -(farval+nearval) / (farval-nearval);

#define M(row,col)  m[col*4+row]
   M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = 0.0F;  M(0,3) = tx;
   M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = 0.0F;  M(1,3) = ty;
   M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = z;     M(2,3) = tz;
   M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = 0.0F;  M(3,3) = 1.0F;
#undef M

   matrix_multf( mat, m, (MAT_FLAG_GENERAL_SCALE|MAT_FLAG_TRANSLATION));
}


#define ZERO(x) (1<<x)
#define ONE(x)  (1<<(x+16))

#define MASK_NO_TRX      (ZERO(12) | ZERO(13) | ZERO(14))
#define MASK_NO_2D_SCALE ( ONE(0)  | ONE(5))

#define MASK_IDENTITY    ( ONE(0)  | ZERO(4)  | ZERO(8)  | ZERO(12) |\
			  ZERO(1)  |  ONE(5)  | ZERO(9)  | ZERO(13) |\
			  ZERO(2)  | ZERO(6)  |  ONE(10) | ZERO(14) |\
			  ZERO(3)  | ZERO(7)  | ZERO(11) |  ONE(15) )

#define MASK_2D_NO_ROT   (           ZERO(4)  | ZERO(8)  |           \
			  ZERO(1)  |            ZERO(9)  |           \
			  ZERO(2)  | ZERO(6)  |  ONE(10) | ZERO(14) |\
			  ZERO(3)  | ZERO(7)  | ZERO(11) |  ONE(15) )

#define MASK_2D          (                      ZERO(8)  |           \
			                        ZERO(9)  |           \
			  ZERO(2)  | ZERO(6)  |  ONE(10) | ZERO(14) |\
			  ZERO(3)  | ZERO(7)  | ZERO(11) |  ONE(15) )


#define MASK_3D_NO_ROT   (           ZERO(4)  | ZERO(8)  |           \
			  ZERO(1)  |            ZERO(9)  |           \
			  ZERO(2)  | ZERO(6)  |                      \
			  ZERO(3)  | ZERO(7)  | ZERO(11) |  ONE(15) )

#define MASK_3D          (                                           \
			                                             \
			                                             \
			  ZERO(3)  | ZERO(7)  | ZERO(11) |  ONE(15) )


#define MASK_PERSPECTIVE (           ZERO(4)  |            ZERO(12) |\
			  ZERO(1)  |                       ZERO(13) |\
			  ZERO(2)  | ZERO(6)  |                      \
			  ZERO(3)  | ZERO(7)  |            ZERO(15) )

#define SQ(x) ((x)*(x))

/* Determine type and flags from scratch.  This is expensive enough to
 * only want to do it once.
 */
static void analyse_from_scratch( GLmatrix *mat )
{
   const GLfloat *m = mat->m;
   GLuint mask = 0;
   GLuint i;

   for (i = 0 ; i < 16 ; i++) {
      if (m[i] == 0.0) mask |= (1<<i);
   }

   if (m[0] == 1.0F) mask |= (1<<16);
   if (m[5] == 1.0F) mask |= (1<<21);
   if (m[10] == 1.0F) mask |= (1<<26);
   if (m[15] == 1.0F) mask |= (1<<31);

   mat->flags &= ~MAT_FLAGS_GEOMETRY;

   /* Check for translation - no-one really cares
    */
   if ((mask & MASK_NO_TRX) != MASK_NO_TRX)
      mat->flags |= MAT_FLAG_TRANSLATION;

   /* Do the real work
    */
   if (mask == (GLuint) MASK_IDENTITY) {
      mat->type = MATRIX_IDENTITY;
   }
   else if ((mask & MASK_2D_NO_ROT) == (GLuint) MASK_2D_NO_ROT) {
      mat->type = MATRIX_2D_NO_ROT;

      if ((mask & MASK_NO_2D_SCALE) != MASK_NO_2D_SCALE)
	 mat->flags = MAT_FLAG_GENERAL_SCALE;
   }
   else if ((mask & MASK_2D) == (GLuint) MASK_2D) {
      GLfloat mm = DOT2(m, m);
      GLfloat m4m4 = DOT2(m+4,m+4);
      GLfloat mm4 = DOT2(m,m+4);

      mat->type = MATRIX_2D;

      /* Check for scale */
      if (SQ(mm-1) > SQ(1e-6) ||
	  SQ(m4m4-1) > SQ(1e-6))
	 mat->flags |= MAT_FLAG_GENERAL_SCALE;

      /* Check for rotation */
      if (SQ(mm4) > SQ(1e-6))
	 mat->flags |= MAT_FLAG_GENERAL_3D;
      else
	 mat->flags |= MAT_FLAG_ROTATION;

   }
   else if ((mask & MASK_3D_NO_ROT) == (GLuint) MASK_3D_NO_ROT) {
      mat->type = MATRIX_3D_NO_ROT;

      /* Check for scale */
      if (SQ(m[0]-m[5]) < SQ(1e-6) &&
	  SQ(m[0]-m[10]) < SQ(1e-6)) {
	 if (SQ(m[0]-1.0) > SQ(1e-6)) {
	    mat->flags |= MAT_FLAG_UNIFORM_SCALE;
         }
      }
      else {
	 mat->flags |= MAT_FLAG_GENERAL_SCALE;
      }
   }
   else if ((mask & MASK_3D) == (GLuint) MASK_3D) {
      GLfloat c1 = DOT3(m,m);
      GLfloat c2 = DOT3(m+4,m+4);
      GLfloat c3 = DOT3(m+8,m+8);
      GLfloat d1 = DOT3(m, m+4);
      GLfloat cp[3];

      mat->type = MATRIX_3D;

      /* Check for scale */
      if (SQ(c1-c2) < SQ(1e-6) && SQ(c1-c3) < SQ(1e-6)) {
	 if (SQ(c1-1.0) > SQ(1e-6))
	    mat->flags |= MAT_FLAG_UNIFORM_SCALE;
	 /* else no scale at all */
      }
      else {
	 mat->flags |= MAT_FLAG_GENERAL_SCALE;
      }

      /* Check for rotation */
      if (SQ(d1) < SQ(1e-6)) {
	 CROSS3( cp, m, m+4 );
	 SUB_3V( cp, cp, (m+8) );
	 if (LEN_SQUARED_3FV(cp) < SQ(1e-6))
	    mat->flags |= MAT_FLAG_ROTATION;
	 else
	    mat->flags |= MAT_FLAG_GENERAL_3D;
      }
      else {
	 mat->flags |= MAT_FLAG_GENERAL_3D; /* shear, etc */
      }
   }
   else if ((mask & MASK_PERSPECTIVE) == MASK_PERSPECTIVE && m[11]==-1.0F) {
      mat->type = MATRIX_PERSPECTIVE;
      mat->flags |= MAT_FLAG_GENERAL;
   }
   else {
      mat->type = MATRIX_GENERAL;
      mat->flags |= MAT_FLAG_GENERAL;
   }
}


/* Analyse a matrix given that its flags are accurate - this is the
 * more common operation, hopefully.
 */
static void analyse_from_flags( GLmatrix *mat )
{
   const GLfloat *m = mat->m;

   if (TEST_MAT_FLAGS(mat, 0)) {
      mat->type = MATRIX_IDENTITY;
   }
   else if (TEST_MAT_FLAGS(mat, (MAT_FLAG_TRANSLATION |
				 MAT_FLAG_UNIFORM_SCALE |
				 MAT_FLAG_GENERAL_SCALE))) {
      if ( m[10]==1.0F && m[14]==0.0F ) {
	 mat->type = MATRIX_2D_NO_ROT;
      }
      else {
	 mat->type = MATRIX_3D_NO_ROT;
      }
   }
   else if (TEST_MAT_FLAGS(mat, MAT_FLAGS_3D)) {
      if (                                 m[ 8]==0.0F
            &&                             m[ 9]==0.0F
            && m[2]==0.0F && m[6]==0.0F && m[10]==1.0F && m[14]==0.0F) {
	 mat->type = MATRIX_2D;
      }
      else {
	 mat->type = MATRIX_3D;
      }
   }
   else if (                 m[4]==0.0F                 && m[12]==0.0F
            && m[1]==0.0F                               && m[13]==0.0F
            && m[2]==0.0F && m[6]==0.0F
            && m[3]==0.0F && m[7]==0.0F && m[11]==-1.0F && m[15]==0.0F) {
      mat->type = MATRIX_PERSPECTIVE;
   }
   else {
      mat->type = MATRIX_GENERAL;
   }
}


void
_math_matrix_analyse( GLmatrix *mat )
{
   if (mat->flags & MAT_DIRTY_TYPE) {
      if (mat->flags & MAT_DIRTY_FLAGS)
	 analyse_from_scratch( mat );
      else
	 analyse_from_flags( mat );
   }

   if (mat->inv && (mat->flags & MAT_DIRTY_INVERSE)) {
      matrix_invert( mat );
   }

   mat->flags &= ~(MAT_DIRTY_FLAGS|
		   MAT_DIRTY_TYPE|
		   MAT_DIRTY_INVERSE);
}


void
_math_matrix_copy( GLmatrix *to, const GLmatrix *from )
{
   MEMCPY( to->m, from->m, sizeof(Identity) );
   to->flags = from->flags;
   to->type = from->type;

   if (to->inv != 0) {
      if (from->inv == 0) {
	 matrix_invert( to );
      }
      else {
	 MEMCPY(to->inv, from->inv, sizeof(GLfloat)*16);
      }
   }
}


void
_math_matrix_scale( GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z )
{
   GLfloat *m = mat->m;
   m[0] *= x;   m[4] *= y;   m[8]  *= z;
   m[1] *= x;   m[5] *= y;   m[9]  *= z;
   m[2] *= x;   m[6] *= y;   m[10] *= z;
   m[3] *= x;   m[7] *= y;   m[11] *= z;

   if (fabs(x - y) < 1e-8 && fabs(x - z) < 1e-8)
      mat->flags |= MAT_FLAG_UNIFORM_SCALE;
   else
      mat->flags |= MAT_FLAG_GENERAL_SCALE;

   mat->flags |= (MAT_DIRTY_TYPE |
		  MAT_DIRTY_INVERSE);
}


void
_math_matrix_translate( GLmatrix *mat, GLfloat x, GLfloat y, GLfloat z )
{
   GLfloat *m = mat->m;
   m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
   m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
   m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
   m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];

   mat->flags |= (MAT_FLAG_TRANSLATION |
		  MAT_DIRTY_TYPE |
		  MAT_DIRTY_INVERSE);
}


void
_math_matrix_loadf( GLmatrix *mat, const GLfloat *m )
{
   MEMCPY( mat->m, m, 16*sizeof(GLfloat) );
   mat->flags = (MAT_FLAG_GENERAL | MAT_DIRTY);
}

void
_math_matrix_ctr( GLmatrix *m )
{
   if ( m->m == 0 ) {
      m->m = (GLfloat *) ALIGN_MALLOC( 16 * sizeof(GLfloat), 16 );
   }
   MEMCPY( m->m, Identity, sizeof(Identity) );
   m->inv = 0;
   m->type = MATRIX_IDENTITY;
   m->flags = 0;
}

void
_math_matrix_dtr( GLmatrix *m )
{
   if ( m->m != 0 ) {
      ALIGN_FREE( m->m );
      m->m = 0;
   }
   if ( m->inv != 0 ) {
      ALIGN_FREE( m->inv );
      m->inv = 0;
   }
}


void
_math_matrix_alloc_inv( GLmatrix *m )
{
   if ( m->inv == 0 ) {
      m->inv = (GLfloat *) ALIGN_MALLOC( 16 * sizeof(GLfloat), 16 );
      MEMCPY( m->inv, Identity, 16 * sizeof(GLfloat) );
   }
}


void
_math_matrix_mul_matrix( GLmatrix *dest, const GLmatrix *a, const GLmatrix *b )
{
   dest->flags = (a->flags |
		  b->flags |
		  MAT_DIRTY_TYPE |
		  MAT_DIRTY_INVERSE);

   if (TEST_MAT_FLAGS(dest, MAT_FLAGS_3D))
      matmul34( dest->m, a->m, b->m );
   else
      matmul4( dest->m, a->m, b->m );
}


void
_math_matrix_mul_floats( GLmatrix *dest, const GLfloat *m )
{
   dest->flags |= (MAT_FLAG_GENERAL |
		   MAT_DIRTY_TYPE |
		   MAT_DIRTY_INVERSE);

   matmul4( dest->m, dest->m, m );
}

void
_math_matrix_set_identity( GLmatrix *mat )
{
   MEMCPY( mat->m, Identity, 16*sizeof(GLfloat) );

   if (mat->inv)
      MEMCPY( mat->inv, Identity, 16*sizeof(GLfloat) );

   mat->type = MATRIX_IDENTITY;
   mat->flags &= ~(MAT_DIRTY_FLAGS|
		   MAT_DIRTY_TYPE|
		   MAT_DIRTY_INVERSE);
}



void
_math_transposef( GLfloat to[16], const GLfloat from[16] )
{
   to[0] = from[0];
   to[1] = from[4];
   to[2] = from[8];
   to[3] = from[12];
   to[4] = from[1];
   to[5] = from[5];
   to[6] = from[9];
   to[7] = from[13];
   to[8] = from[2];
   to[9] = from[6];
   to[10] = from[10];
   to[11] = from[14];
   to[12] = from[3];
   to[13] = from[7];
   to[14] = from[11];
   to[15] = from[15];
}


void
_math_transposed( GLdouble to[16], const GLdouble from[16] )
{
   to[0] = from[0];
   to[1] = from[4];
   to[2] = from[8];
   to[3] = from[12];
   to[4] = from[1];
   to[5] = from[5];
   to[6] = from[9];
   to[7] = from[13];
   to[8] = from[2];
   to[9] = from[6];
   to[10] = from[10];
   to[11] = from[14];
   to[12] = from[3];
   to[13] = from[7];
   to[14] = from[11];
   to[15] = from[15];
}

void
_math_transposefd( GLfloat to[16], const GLdouble from[16] )
{
   to[0] = from[0];
   to[1] = from[4];
   to[2] = from[8];
   to[3] = from[12];
   to[4] = from[1];
   to[5] = from[5];
   to[6] = from[9];
   to[7] = from[13];
   to[8] = from[2];
   to[9] = from[6];
   to[10] = from[10];
   to[11] = from[14];
   to[12] = from[3];
   to[13] = from[7];
   to[14] = from[11];
   to[15] = from[15];
}
