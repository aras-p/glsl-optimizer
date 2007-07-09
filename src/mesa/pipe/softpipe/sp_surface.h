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

#ifndef SP_SURFACE_H
#define SP_SURFACE_H

#include "glheader.h"
#include "sp_headers.h"

struct softpipe_surface;

#define G_SURFACE_RGBA_8888  0x1


/**
 * Softpipe surface is derived from pipe_surface.
 */
struct softpipe_surface {
   struct pipe_surface surface;

   /**
    * Functions for read/writing surface data
    */
   void (*read_quad_f)( struct softpipe_surface *,
			GLint x, GLint y,
			GLfloat (*rgba)[NUM_CHANNELS] );

   void (*read_quad_f_swz)( struct softpipe_surface *,
			    GLint x, GLint y,
			    GLfloat (*rrrr)[QUAD_SIZE] );

   void (*read_quad_ub)( struct softpipe_surface *,
			 GLint x, GLint y,
			 GLubyte (*rgba)[NUM_CHANNELS] );


   void (*write_quad_f)( struct softpipe_surface *,
			 GLint x, GLint y,
			 GLfloat (*rgba)[NUM_CHANNELS] );

   void (*write_quad_f_swz)( struct softpipe_surface *,
			     GLint x, GLint y,
			     GLfloat (*rrrr)[QUAD_SIZE] );


   void (*write_quad_ub)( struct softpipe_surface *,
			  GLint x, GLint y,
			  GLubyte (*rgba)[NUM_CHANNELS] );

   void (*write_mono_row_ub)( struct softpipe_surface *,
                              GLuint count, GLint x, GLint y,
                              GLubyte rgba[NUM_CHANNELS] );

   void (*read_quad_z)(struct softpipe_surface *,
                       GLint x, GLint y, GLfloat zzzz[QUAD_SIZE]);
   void (*write_quad_z)(struct softpipe_surface *,
                        GLint x, GLint y, const GLfloat zzzz[QUAD_SIZE]);
};


/** Cast wrapper */
static INLINE struct softpipe_surface *
softpipe_surface(struct pipe_surface *ps)
{
   return (struct softpipe_surface *) ps;
}


#endif /* SP_SURFACE_H */
