/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "sp_context.h"
#include "sp_state.h"
#include "sp_surface.h"
#include "sp_headers.h"

static void rgba8_read_quad_f( struct softpipe_surface *gs,
			       GLint x, GLint y,
			       GLfloat (*rgba)[NUM_CHANNELS] )
{
   GLuint i, j, k = 0;

   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++, k++) {
	 GLubyte *ptr = gs->surface.ptr + (y+i) * gs->surface.stride + (x+j) * 4;
	 rgba[k][0] = ptr[0] * (1.0 / 255.0);
	 rgba[k][1] = ptr[1] * (1.0 / 255.0);
	 rgba[k][2] = ptr[2] * (1.0 / 255.0);
	 rgba[k][3] = ptr[3] * (1.0 / 255.0);
      }
   }
}

static void rgba8_read_quad_f_swz( struct softpipe_surface *gs,
				   GLint x, GLint y,
				   GLfloat (*rrrr)[QUAD_SIZE] )
{
   GLuint i, j, k = 0;

   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++, k++) {
	 GLubyte *ptr = gs->surface.ptr + (y+i) * gs->surface.stride + (x+j) * 4;
	 rrrr[0][k] = ptr[0] * (1.0 / 255.0);
	 rrrr[1][k] = ptr[1] * (1.0 / 255.0);
	 rrrr[2][k] = ptr[2] * (1.0 / 255.0);
	 rrrr[3][k] = ptr[3] * (1.0 / 255.0);
      }
   }
}

static void rgba8_write_quad_f( struct softpipe_surface *gs,
				GLint x, GLint y,
				GLfloat (*rgba)[NUM_CHANNELS] )
{
   GLuint i, j, k = 0;

   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++, k++) {
	 GLubyte *ptr = gs->surface.ptr + (y+i) * gs->surface.stride + (x+j) * 4;
	 ptr[0] = rgba[k][0] * 255.0;
	 ptr[1] = rgba[k][1] * 255.0;
	 ptr[2] = rgba[k][2] * 255.0;
	 ptr[3] = rgba[k][3] * 255.0;
      }
   }
}

static void rgba8_write_quad_f_swz( struct softpipe_surface *gs,
				    GLint x, GLint y,
				    GLfloat (*rrrr)[QUAD_SIZE] )
{
   GLuint i, j, k = 0;

   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++, k++) {
	 GLubyte *ptr = gs->surface.ptr + (y+i) * gs->surface.stride + (x+j) * 4;
	 ptr[0] = rrrr[0][k] * 255.0;
	 ptr[1] = rrrr[1][k] * 255.0;
	 ptr[2] = rrrr[2][k] * 255.0;
	 ptr[3] = rrrr[3][k] * 255.0;
      }
   }
}




static void rgba8_read_quad_ub( struct softpipe_surface *gs,
				GLint x, GLint y,
				GLubyte (*rgba)[NUM_CHANNELS] )
{
   GLuint i, j, k = 0;

   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++, k++) {
	 GLubyte *ptr = gs->surface.ptr + (y+i) * gs->surface.stride + (x+j) * 4;
	 rgba[k][0] = ptr[0];
	 rgba[k][1] = ptr[1];
	 rgba[k][2] = ptr[2];
	 rgba[k][3] = ptr[3];
      }
   }
}


static void rgba8_write_quad_ub( struct softpipe_surface *gs,
				 GLint x, GLint y,
				 GLubyte (*rgba)[NUM_CHANNELS] )
{
   GLuint i, j, k = 0;

   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++, k++) {
	 GLubyte *ptr = gs->surface.ptr + (y+i) * gs->surface.stride + (x+j) * 4;
	 ptr[0] = rgba[k][0];
	 ptr[1] = rgba[k][1];
	 ptr[2] = rgba[k][2];
	 ptr[3] = rgba[k][3];
      }
   }
}




struct softpipe_surface_type gs_rgba8 = {
   G_SURFACE_RGBA_8888,
   rgba8_read_quad_f,
   rgba8_read_quad_f_swz,
   rgba8_read_quad_ub,
   rgba8_write_quad_f,
   rgba8_write_quad_f_swz,
   rgba8_write_quad_ub,
};



