/* $Id: x86.c,v 1.13 2000/11/22 08:55:53 joukj Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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

/*
 * Intel x86 assembly code by Josh Vanderhoof
 */

#include "glheader.h"
#include "context.h"
#include "mtypes.h"
#include "x86.h"

#include "math/m_vertices.h"
#include "math/m_xform.h"

#include "tnl/t_context.h"

#ifdef DEBUG
#include "math/m_debug_xform.h"
#endif


#define XFORM_ARGS 	GLvector4f *to_vec, 				\
			const GLfloat m[16], 				\
			const GLvector4f *from_vec, 			\
			const GLubyte *mask, 				\
			const GLubyte flag


#define DECLARE_XFORM_GROUP( pfx, sz, masked ) \
 extern void _ASMAPI gl_##pfx##_transform_points##sz##_general_##masked( XFORM_ARGS );	    \
 extern void _ASMAPI gl_##pfx##_transform_points##sz##_identity_##masked( XFORM_ARGS );	    \
 extern void _ASMAPI gl_##pfx##_transform_points##sz##_3d_no_rot_##masked( XFORM_ARGS );    \
 extern void _ASMAPI gl_##pfx##_transform_points##sz##_perspective_##masked( XFORM_ARGS );  \
 extern void _ASMAPI gl_##pfx##_transform_points##sz##_2d_##masked( XFORM_ARGS );	    \
 extern void _ASMAPI gl_##pfx##_transform_points##sz##_2d_no_rot_##masked( XFORM_ARGS );    \
 extern void _ASMAPI gl_##pfx##_transform_points##sz##_3d_##masked( XFORM_ARGS );


#define ASSIGN_XFORM_GROUP( pfx, cma, sz, masked )			\
   gl_transform_tab[cma][sz][MATRIX_GENERAL] =				\
      gl_##pfx##_transform_points##sz##_general_##masked;		\
   gl_transform_tab[cma][sz][MATRIX_IDENTITY] =				\
      gl_##pfx##_transform_points##sz##_identity_##masked;		\
   gl_transform_tab[cma][sz][MATRIX_3D_NO_ROT] =			\
      gl_##pfx##_transform_points##sz##_3d_no_rot_##masked;		\
   gl_transform_tab[cma][sz][MATRIX_PERSPECTIVE] =			\
      gl_##pfx##_transform_points##sz##_perspective_##masked;		\
   gl_transform_tab[cma][sz][MATRIX_2D] =				\
      gl_##pfx##_transform_points##sz##_2d_##masked;			\
   gl_transform_tab[cma][sz][MATRIX_2D_NO_ROT] =			\
      gl_##pfx##_transform_points##sz##_2d_no_rot_##masked;		\
   gl_transform_tab[cma][sz][MATRIX_3D] =				\
      gl_##pfx##_transform_points##sz##_3d_##masked;


#ifdef USE_X86_ASM
DECLARE_XFORM_GROUP( x86, 2, raw )
DECLARE_XFORM_GROUP( x86, 3, raw )
DECLARE_XFORM_GROUP( x86, 4, raw )
DECLARE_XFORM_GROUP( x86, 2, masked )
DECLARE_XFORM_GROUP( x86, 3, masked )
DECLARE_XFORM_GROUP( x86, 4, masked )


extern GLvector4f * _ASMAPI gl_x86_cliptest_points4( GLvector4f *clip_vec,
						     GLvector4f *proj_vec,
						     GLubyte clipMask[],
						     GLubyte *orMask,
						     GLubyte *andMask );


extern void _ASMAPI gl_v16_x86_cliptest_points4( GLfloat *first_vert,
						 GLfloat *last_vert,
						 GLubyte *or_mask,
						 GLubyte *and_mask,
						 GLubyte *clip_mask );


extern void _ASMAPI gl_v16_x86_general_xform( GLfloat *dest,
					      const GLfloat *m,
					      const GLfloat *src,
					      GLuint src_stride,
					      GLuint count );
#endif


void gl_init_x86_transform_asm( void )
{
#ifdef USE_X86_ASM
   ASSIGN_XFORM_GROUP( x86, 0, 2, raw );
   ASSIGN_XFORM_GROUP( x86, 0, 3, raw );
   ASSIGN_XFORM_GROUP( x86, 0, 4, raw );

   ASSIGN_XFORM_GROUP( x86, CULL_MASK_ACTIVE, 2, masked );
   ASSIGN_XFORM_GROUP( x86, CULL_MASK_ACTIVE, 3, masked );
   ASSIGN_XFORM_GROUP( x86, CULL_MASK_ACTIVE, 4, masked );

   /* XXX this function has been found to cause FP overflow exceptions */
   gl_clip_tab[4] = gl_x86_cliptest_points4;

#ifdef DEBUG
   gl_test_all_transform_functions( "x86" );
#endif
#endif
}

void gl_init_x86_vertex_asm( void )
{
#ifdef USE_X86_ASM
   gl_xform_points3_v16_general	= gl_v16_x86_general_xform;
   gl_cliptest_points4_v16	= gl_v16_x86_cliptest_points4;

#ifdef DEBUG_NOT
   gl_test_all_vertex_functions( "x86" );
#endif
#endif
}
