/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef G_SURFACE_H
#define G_SURFACE_H

#include "glheader.h"
#include "g_headers.h"

struct generic_surface;

#define G_SURFACE_RGBA_8888  0x1

/* Internal structs and helpers for the primitive clip/setup pipeline:
 */
struct generic_surface_type {
   
   GLuint format;

   void (*read_quad_f)( struct generic_surface *,
			GLint x, GLint y,
			GLfloat (*rgba)[NUM_CHANNELS] );

   void (*read_quad_f_swz)( struct generic_surface *,
			    GLint x, GLint y,
			    GLfloat (*rrrr)[QUAD_SIZE] );

   void (*read_quad_ub)( struct generic_surface *,
			 GLint x, GLint y,
			 GLubyte (*rgba)[NUM_CHANNELS] );


   void (*write_quad_f)( struct generic_surface *,
			 GLint x, GLint y,
			 GLfloat (*rgba)[NUM_CHANNELS] );

   void (*write_quad_f_swz)( struct generic_surface *,
			     GLint x, GLint y,
			     GLfloat (*rrrr)[QUAD_SIZE] );


   void (*write_quad_ub)( struct generic_surface *,
			  GLint x, GLint y,
			  GLubyte (*rgba)[NUM_CHANNELS] );


};


struct generic_surface {
   struct generic_surface_type *type;
   struct softpipe_surface surface;
};


static INLINE void gs_read_quad_f( struct generic_surface *gs,
				   GLint x, GLint y,
				   GLfloat (*rgba)[NUM_CHANNELS] )
{
   gs->type->read_quad_f(gs, x, y, rgba);
}

static INLINE void gs_read_quad_f_swz( struct generic_surface *gs,
				       GLint x, GLint y,
				       GLfloat (*rrrr)[QUAD_SIZE] )
{
   gs->type->read_quad_f_swz(gs, x, y, rrrr);
}

static INLINE void gs_write_quad_f( struct generic_surface *gs,
				    GLint x, GLint y,
				    GLfloat (*rgba)[NUM_CHANNELS] )
{
   gs->type->write_quad_f(gs, x, y, rgba);
}

static INLINE void gs_write_quad_f_swz( struct generic_surface *gs,
					GLint x, GLint y,
					GLfloat (*rrrr)[QUAD_SIZE] )
{
   gs->type->write_quad_f_swz(gs, x, y, rrrr);
}

/* Like this, or hidden?
 */
struct generic_surface_type gs_rgba8;

#endif
