/* $Id: m_xform.h,v 1.2 2000/11/17 21:01:49 brianp Exp $ */

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





#ifndef _M_XFORM_H
#define _M_XFORM_H


#include "glheader.h"
#include "config.h"
#include "math/math.h"
#include "math/m_vector.h"
#include "math/m_matrix.h"

#ifdef USE_X86_ASM
#define _XFORMAPI _ASMAPI
#define _XFORMAPIP _ASMAPIP
#else
#define _XFORMAPI
#define _XFORMAPIP *
#endif

/*
 * Transform a point (column vector) by a matrix:   Q = M * P
 */
#define TRANSFORM_POINT( Q, M, P )					\
   Q[0] = M[0] * P[0] + M[4] * P[1] + M[8] *  P[2] + M[12] * P[3];	\
   Q[1] = M[1] * P[0] + M[5] * P[1] + M[9] *  P[2] + M[13] * P[3];	\
   Q[2] = M[2] * P[0] + M[6] * P[1] + M[10] * P[2] + M[14] * P[3];	\
   Q[3] = M[3] * P[0] + M[7] * P[1] + M[11] * P[2] + M[15] * P[3];


#define TRANSFORM_POINT3( Q, M, P )				\
   Q[0] = M[0] * P[0] + M[4] * P[1] + M[8] *  P[2] + M[12];	\
   Q[1] = M[1] * P[0] + M[5] * P[1] + M[9] *  P[2] + M[13];	\
   Q[2] = M[2] * P[0] + M[6] * P[1] + M[10] * P[2] + M[14];	\
   Q[3] = M[3] * P[0] + M[7] * P[1] + M[11] * P[2] + M[15];


/*
 * Transform a normal (row vector) by a matrix:  [NX NY NZ] = N * MAT
 */
#define TRANSFORM_NORMAL( TO, N, MAT )				\
do {								\
   TO[0] = N[0] * MAT[0] + N[1] * MAT[1] + N[2] * MAT[2];	\
   TO[1] = N[0] * MAT[4] + N[1] * MAT[5] + N[2] * MAT[6];	\
   TO[2] = N[0] * MAT[8] + N[1] * MAT[9] + N[2] * MAT[10];	\
} while (0)


extern void gl_transform_vector( GLfloat u[4],
				 const GLfloat v[4],
                                 const GLfloat m[16] );


extern void 
_math_init_transformation( void );


/* KW: Clip functions now do projective divide as well.  The projected
 * coordinates are very useful to us because they let us cull
 * backfaces and eliminate vertices from lighting, fogging, etc
 * calculations.  Despite the fact that this divide could be done one
 * day in hardware, we would still have a reason to want to do it here
 * as long as those other calculations remain in software.
 *
 * Clipping is a convenient place to do the divide on x86 as it should be
 * possible to overlap with integer outcode calculations.
 *
 * There are two cases where we wouldn't want to do the divide in cliptest:
 *    - When we aren't clipping.  We still might want to cull backfaces
 *      so the divide should be done elsewhere.  This currently never 
 *      happens.
 *
 *    - When culling isn't likely to help us, such as when the GL culling
 *      is disabled and we not lighting or are only lighting
 *      one-sided.  In this situation, backface determination provides
 *      us with no useful information.  A tricky case to detect is when
 *      all input data is already culled, although hopefully the
 *      application wouldn't turn on culling in such cases.
 *
 * We supply a buffer to hold the [x/w,y/w,z/w,1/w] values which
 * are the result of the projection.  This is only used in the 
 * 4-vector case - in other cases, we just use the clip coordinates
 * as the projected coordinates - they are identical.
 * 
 * This is doubly convenient because it means the Win[] array is now
 * of the same stride as all the others, so I can now turn map_vertices
 * into a straight-forward matrix transformation, with asm acceleration
 * automatically available.  
 */

/* Vertex buffer clipping flags 
 */
#define CLIP_RIGHT_SHIFT 	0
#define CLIP_LEFT_SHIFT 	1
#define CLIP_TOP_SHIFT  	2
#define CLIP_BOTTOM_SHIFT       3
#define CLIP_NEAR_SHIFT  	4
#define CLIP_FAR_SHIFT  	5

#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
#define CLIP_USER_BIT    0x40
#define CLIP_CULLED_BIT  0x80	/* Vertex has been culled */
#define CLIP_ALL_BITS    0x3f


typedef GLvector4f * (_XFORMAPIP clip_func)( GLvector4f *vClip,
				  GLvector4f *vProj, 
				  GLubyte clipMask[],
				  GLubyte *orMask, 
				  GLubyte *andMask );

typedef void (*dotprod_func)( GLvector4f *out_vec, 
			      GLuint elt,
			      const GLvector4f *coord_vec, 
			      const GLfloat plane[4], 
			      const GLubyte mask[]);

typedef void (*vec_copy_func)( GLvector4f *to, 
			       const GLvector4f *from, 
			       const GLubyte mask[]);



/*
 * Functions for transformation of normals in the VB.
 */
typedef void (_NORMAPIP normal_func)( const GLmatrix *mat,
			     GLfloat scale,
			     const GLvector3f *in,
			     const GLfloat lengths[],
			     const GLubyte mask[],
			     GLvector3f *dest );


/* Flags for selecting a normal transformation function. 
 */
#define NORM_RESCALE   0x1	/* apply the scale factor */
#define NORM_NORMALIZE 0x2	/* normalize */
#define NORM_TRANSFORM 0x4	/* apply the transformation matrix */
#define NORM_TRANSFORM_NO_ROT 0x8	/* apply the transformation matrix */




/* KW: New versions of the transform function allow a mask array
 *     specifying that individual vector transform should be skipped
 *     when the mask byte is zero.  This is always present as a
 *     parameter, to allow a unified interface.  
 */
typedef void (_XFORMAPIP transform_func)( GLvector4f *to_vec,
					  const GLfloat m[16],
				const GLvector4f *from_vec, 
				const GLubyte *clipmask,
				const GLubyte flag );


extern GLvector4f *gl_project_points( GLvector4f *to,
			       const GLvector4f *from );

extern void gl_transform_bounds3( GLubyte *orMask, GLubyte *andMask,
				  const GLfloat m[16],
			   CONST GLfloat src[][3] );

extern void gl_transform_bounds2( GLubyte *orMask, GLubyte *andMask,
				  const GLfloat m[16],
			   CONST GLfloat src[][3] );


extern dotprod_func  gl_dotprod_tab[2][5];
extern vec_copy_func gl_copy_tab[2][0x10];
extern clip_func     gl_clip_tab[5];
extern normal_func   gl_normal_tab[0xf][0x4];

/* Use of 3 layers of linked 1-dimensional arrays to reduce
 * cost of lookup.
 */
extern transform_func **(gl_transform_tab[2]);


extern void gl_transform_point_sz( GLfloat Q[4], const GLfloat M[16],
				   const GLfloat P[4], GLuint sz );


#define TransformRaw( to, mat, from ) \
      ( (*gl_transform_tab[0][(from)->size][(mat)->type])( to, (mat)->m, from, 0, 0 ), \
        (to) )

#define Transform( to, mat, from, mask, cull ) \
      ( (*gl_transform_tab[cull!=0][(from)->size][(mat)->type])( to, (mat)->m, from, mask, cull ), \
	(to) )


#endif
