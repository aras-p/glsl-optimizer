/* $Id: x86.c,v 1.20 2001/03/29 06:46:27 gareth Exp $ */

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
#include "math/m_debug.h"
#endif


#define XFORM_ARGS 	GLvector4f *to_vec, 				\
			const GLfloat m[16], 				\
			const GLvector4f *from_vec, 			\
			const GLubyte *mask, 				\
			const GLubyte flag


#define DECLARE_XFORM_GROUP( pfx, sz ) \
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_general( XFORM_ARGS );		\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_identity( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_3d_no_rot( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_perspective( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_2d( XFORM_ARGS );		\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_2d_no_rot( XFORM_ARGS );	\
extern void _ASMAPI _mesa_##pfx##_transform_points##sz##_3d( XFORM_ARGS );


#define ASSIGN_XFORM_GROUP( pfx, sz )					\
   _mesa_transform_tab[0][sz][MATRIX_GENERAL] =				\
      _mesa_##pfx##_transform_points##sz##_general;			\
   _mesa_transform_tab[0][sz][MATRIX_IDENTITY] =			\
      _mesa_##pfx##_transform_points##sz##_identity;			\
   _mesa_transform_tab[0][sz][MATRIX_3D_NO_ROT] =			\
      _mesa_##pfx##_transform_points##sz##_3d_no_rot;			\
   _mesa_transform_tab[0][sz][MATRIX_PERSPECTIVE] =			\
      _mesa_##pfx##_transform_points##sz##_perspective;			\
   _mesa_transform_tab[0][sz][MATRIX_2D] =				\
      _mesa_##pfx##_transform_points##sz##_2d;				\
   _mesa_transform_tab[0][sz][MATRIX_2D_NO_ROT] =			\
      _mesa_##pfx##_transform_points##sz##_2d_no_rot;			\
   _mesa_transform_tab[0][sz][MATRIX_3D] =				\
      _mesa_##pfx##_transform_points##sz##_3d;


#ifdef USE_X86_ASM
DECLARE_XFORM_GROUP( x86, 2 )
DECLARE_XFORM_GROUP( x86, 3 )
DECLARE_XFORM_GROUP( x86, 4 )


extern GLvector4f * _ASMAPI
_mesa_x86_cliptest_points4( GLvector4f *clip_vec,
			    GLvector4f *proj_vec,
			    GLubyte clipMask[],
			    GLubyte *orMask,
			    GLubyte *andMask );

extern GLvector4f * _ASMAPI
_mesa_x86_cliptest_points4_np( GLvector4f *clip_vec,
			       GLvector4f *proj_vec,
			       GLubyte clipMask[],
			       GLubyte *orMask,
			       GLubyte *andMask );

extern void _ASMAPI
_mesa_v16_x86_cliptest_points4( GLfloat *first_vert,
				GLfloat *last_vert,
				GLubyte *or_mask,
				GLubyte *and_mask,
				GLubyte *clip_mask );

extern void _ASMAPI
_mesa_v16_x86_general_xform( GLfloat *dest,
			     const GLfloat *m,
			     const GLfloat *src,
			     GLuint src_stride,
			     GLuint count );
#endif


void _mesa_init_x86_transform_asm( void )
{
#ifdef USE_X86_ASM
   ASSIGN_XFORM_GROUP( x86, 2 );
   ASSIGN_XFORM_GROUP( x86, 3 );
   ASSIGN_XFORM_GROUP( x86, 4 );

   /* XXX this function has been found to cause FP overflow exceptions */
   _mesa_clip_tab[4] = _mesa_x86_cliptest_points4;
   _mesa_clip_np_tab[4] = _mesa_x86_cliptest_points4_np;

#ifdef DEBUG
   _math_test_all_transform_functions( "x86" );
#endif
#endif
}

void _mesa_init_x86_vertex_asm( void )
{
#ifdef USE_X86_ASM
   _mesa_xform_points3_v16_general = _mesa_v16_x86_general_xform;
   _mesa_cliptest_points4_v16 = _mesa_v16_x86_cliptest_points4;

#ifdef DEBUG
   _math_test_all_vertex_functions( "x86" );
#endif
#endif
}
