/* $Id: sparc.c,v 1.1 2001/05/23 14:27:03 brianp Exp $ */

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

/*
 * Sparc assembly code by David S. Miller
 */


#include "glheader.h"
#include "context.h"
#include "math/m_vertices.h"
#include "math/m_xform.h"
#include "tnl/t_context.h"
#include "sparc.h"

#ifdef DEBUG
#include "math/m_debug.h"
#endif

#define XFORM_ARGS 	GLvector4f *to_vec, 		\
			const GLfloat m[16], 		\
			const GLvector4f *from_vec

#define DECLARE_XFORM_GROUP(pfx, sz)					   \
 extern void _mesa_##pfx##_transform_points##sz##_general(XFORM_ARGS);     \
 extern void _mesa_##pfx##_transform_points##sz##_identity(XFORM_ARGS);    \
 extern void _mesa_##pfx##_transform_points##sz##_3d_no_rot(XFORM_ARGS);   \
 extern void _mesa_##pfx##_transform_points##sz##_perspective(XFORM_ARGS); \
 extern void _mesa_##pfx##_transform_points##sz##_2d(XFORM_ARGS);          \
 extern void _mesa_##pfx##_transform_points##sz##_2d_no_rot(XFORM_ARGS);   \
 extern void _mesa_##pfx##_transform_points##sz##_3d(XFORM_ARGS);

#define ASSIGN_XFORM_GROUP(pfx, sz)					\
   _mesa_transform_tab[sz][MATRIX_GENERAL] =				\
      _mesa_##pfx##_transform_points##sz##_general;			\
   _mesa_transform_tab[sz][MATRIX_IDENTITY] =				\
      _mesa_##pfx##_transform_points##sz##_identity;			\
   _mesa_transform_tab[sz][MATRIX_3D_NO_ROT] =				\
      _mesa_##pfx##_transform_points##sz##_3d_no_rot;			\
   _mesa_transform_tab[sz][MATRIX_PERSPECTIVE] =			\
      _mesa_##pfx##_transform_points##sz##_perspective;			\
   _mesa_transform_tab[sz][MATRIX_2D] =					\
      _mesa_##pfx##_transform_points##sz##_2d;				\
   _mesa_transform_tab[sz][MATRIX_2D_NO_ROT] =				\
      _mesa_##pfx##_transform_points##sz##_2d_no_rot;			\
   _mesa_transform_tab[sz][MATRIX_3D] =					\
      _mesa_##pfx##_transform_points##sz##_3d;


#ifdef USE_SPARC_ASM
DECLARE_XFORM_GROUP(sparc, 1)
DECLARE_XFORM_GROUP(sparc, 2)
DECLARE_XFORM_GROUP(sparc, 3)
DECLARE_XFORM_GROUP(sparc, 4)

extern GLvector4f  *_mesa_sparc_cliptest_points4(GLvector4f *clip_vec,
						 GLvector4f *proj_vec,
						 GLubyte clipMask[],
						 GLubyte *orMask,
						 GLubyte *andMask);

extern GLvector4f  *_mesa_sparc_cliptest_points4_np(GLvector4f *clip_vec,
						    GLvector4f *proj_vec,
						    GLubyte clipMask[],
						    GLubyte *orMask,
						    GLubyte *andMask);
#endif

void _mesa_init_all_sparc_transform_asm(void)
{
#ifdef USE_SPARC_ASM
   ASSIGN_XFORM_GROUP(sparc, 1)
   ASSIGN_XFORM_GROUP(sparc, 2)
   ASSIGN_XFORM_GROUP(sparc, 3)
   ASSIGN_XFORM_GROUP(sparc, 4)

   _mesa_clip_tab[4] = _mesa_sparc_cliptest_points4;
   _mesa_clip_np_tab[4] = _mesa_sparc_cliptest_points4_np;

#ifdef DEBUG
   _math_test_all_transform_functions("sparc");
   _math_test_all_cliptest_functions("sparc");
#endif

#endif
}
