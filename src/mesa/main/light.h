/* $Id: light.h,v 1.6 2000/11/22 07:32:17 joukj Exp $ */

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


#ifndef LIGHT_H
#define LIGHT_H


#include "mtypes.h"


extern void
_mesa_ShadeModel( GLenum mode );

extern void
_mesa_ColorMaterial( GLenum face, GLenum mode );

extern void
_mesa_Lightf( GLenum light, GLenum pname, GLfloat param );

extern void
_mesa_Lightfv( GLenum light, GLenum pname, const GLfloat *params );

extern void
_mesa_Lightiv( GLenum light, GLenum pname, const GLint *params );

extern void
_mesa_Lighti( GLenum light, GLenum pname, GLint param );

extern void
_mesa_LightModelf( GLenum pname, GLfloat param );

extern void
_mesa_LightModelfv( GLenum pname, const GLfloat *params );

extern void
_mesa_LightModeli( GLenum pname, GLint param );

extern void
_mesa_LightModeliv( GLenum pname, const GLint *params );

extern void
_mesa_Materialf( GLenum face, GLenum pname, GLfloat param );

extern void
_mesa_Materialfv( GLenum face, GLenum pname, const GLfloat *params );

extern void
_mesa_Materiali( GLenum face, GLenum pname, GLint param );

extern void
_mesa_Materialiv( GLenum face, GLenum pname, const GLint *params );

extern void
_mesa_GetLightfv( GLenum light, GLenum pname, GLfloat *params );

extern void
_mesa_GetLightiv( GLenum light, GLenum pname, GLint *params );

extern void
_mesa_GetMaterialfv( GLenum face, GLenum pname, GLfloat *params );

extern void
_mesa_GetMaterialiv( GLenum face, GLenum pname, GLint *params );


/* Lerp between adjacent values in the f(x) lookup table, giving a
 * continuous function, with adequeate overall accuracy.  (Though
 * still pretty good compared to a straight lookup).
 */
#define GET_SHINE_TAB_ENTRY( table, dp, result )			\
do {									\
   struct gl_shine_tab *_tab = table;					\
   if (dp>1.0)								\
      result = pow( dp, _tab->shininess );				\
   else {								\
      float f = (dp * (SHINE_TABLE_SIZE-1));				\
      int k = (int) f;							\
      result = _tab->tab[k] + (f-k)*(_tab->tab[k+1]-_tab->tab[k]);	\
   }									\
} while (0)



extern GLuint gl_material_bitmask( GLcontext *ctx,
				   GLenum face, GLenum pname,
				   GLuint legal,
				   const char * );

extern void gl_set_material( GLcontext *ctx, GLuint bitmask,
                             const GLfloat *params);

extern void gl_compute_spot_exp_table( struct gl_light *l );

extern void gl_compute_shine_table( GLcontext *ctx, GLuint i,
				    GLfloat shininess );

extern void gl_update_lighting( GLcontext *ctx );

extern void gl_compute_light_positions( GLcontext *ctx );

extern void gl_update_material( GLcontext *ctx,
				const struct gl_material src[2],
				GLuint bitmask );

extern void gl_update_color_material( GLcontext *ctx, const GLchan rgba[4] );


#endif

