/* $Id: sse.c,v 1.2 2001/03/30 14:44:43 gareth Exp $ */

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
 * PentiumIII-SIMD (SSE) optimizations contributed by
 * Andre Werthmann <wertmann@cs.uni-potsdam.de>
 */

#include "glheader.h"
#include "context.h"
#include "math/m_vertices.h"
#include "math/m_xform.h"
#include "tnl/t_context.h"

#include "sse.h"
#include "common_x86_macros.h"

#ifdef DEBUG
#include "math/m_debug.h"
#endif


#ifdef USE_SSE_ASM
DECLARE_XFORM_GROUP( sse, 2 )
DECLARE_XFORM_GROUP( sse, 3 )

#if 1
/* Some functions are not written in SSE-assembly, because the fpu ones are faster */
extern void _mesa_sse_transform_normals_no_rot( NORM_ARGS );
extern void _mesa_sse_transform_rescale_normals( NORM_ARGS );
extern void _mesa_sse_transform_rescale_normals_no_rot( NORM_ARGS );

extern void _mesa_sse_transform_points4_general( XFORM_ARGS );
extern void _mesa_sse_transform_points4_3d( XFORM_ARGS );
extern void _mesa_sse_transform_points4_identity( XFORM_ARGS );
#else
DECLARE_NORM_GROUP( sse )
#endif


extern void _ASMAPI
_mesa_v16_sse_general_xform( GLfloat *first_vert,
			     const GLfloat *m,
			     const GLfloat *src,
			     GLuint src_stride,
			     GLuint count );

extern void _ASMAPI
_mesa_sse_project_vertices( GLfloat *first,
			    GLfloat *last,
			    const GLfloat *m,
			    GLuint stride );

extern void _ASMAPI
_mesa_sse_project_clipped_vertices( GLfloat *first,
				    GLfloat *last,
				    const GLfloat *m,
				    GLuint stride,
				    const GLubyte *clipmask );
#endif


void _mesa_init_sse_transform_asm( void )
{
#ifdef USE_SSE_ASM
   ASSIGN_XFORM_GROUP( sse, 2 );
   ASSIGN_XFORM_GROUP( sse, 3 );

#if 1
   /* TODO: Finish these off.
    */
   _mesa_transform_tab[4][MATRIX_GENERAL] =
      _mesa_sse_transform_points4_general;
   _mesa_transform_tab[4][MATRIX_3D] =
      _mesa_sse_transform_points4_3d;
   _mesa_transform_tab[4][MATRIX_IDENTITY] =
      _mesa_sse_transform_points4_identity;

   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT] =
      _mesa_sse_transform_normals_no_rot;
   _mesa_normal_tab[NORM_TRANSFORM | NORM_RESCALE] =
      _mesa_sse_transform_rescale_normals;
   _mesa_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_RESCALE] =
      _mesa_sse_transform_rescale_normals_no_rot;
#else
   ASSIGN_XFORM_GROUP( sse, 4 );

   ASSIGN_NORM_GROUP( sse );
#endif

#ifdef DEBUG
   _math_test_all_transform_functions( "SSE" );
   _math_test_all_normal_transform_functions( "SSE" );
#endif
#endif
}

void _mesa_init_sse_vertex_asm( void )
{
#ifdef USE_SSE_ASM
   _mesa_xform_points3_v16_general = _mesa_v16_sse_general_xform;
#if 0
   /* GH: These are broken.  I'm fixing them now.
    */
   _mesa_project_v16 = _mesa_sse_project_vertices;
   _mesa_project_clipped_v16 = _mesa_sse_project_clipped_vertices;
#endif

#ifdef DEBUG_NOT
   _math_test_all_vertex_functions( "SSE" );
#endif
#endif
}
