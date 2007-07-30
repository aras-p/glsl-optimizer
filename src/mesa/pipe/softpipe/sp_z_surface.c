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


/**
 * Software Z buffer/surface.
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "sp_surface.h"
#include "sp_z_surface.h"

static void*
z16_map(struct pipe_buffer *pb, GLuint access_mode)
{
   struct softpipe_surface *sps = (struct softpipe_surface *) pb;
   sps->surface.ptr = pb->ptr;
   return pb->ptr;
}

static void
z16_unmap(struct pipe_buffer *pb)
{
   struct softpipe_surface *sps = (struct softpipe_surface *) pb;
   sps->surface.ptr = NULL;
}

static void
z16_resize(struct pipe_surface *ps, GLuint width, GLuint height)
{
   struct softpipe_surface *sps = (struct softpipe_surface *) ps;

   if (sps->surface.buffer.ptr)
      free(sps->surface.buffer.ptr);

   ps->buffer.ptr = (GLubyte *) malloc(width * height * sizeof(GLushort));
   ps->width = width;
   ps->height = height;

   sps->surface.stride = sps->surface.width;
   sps->surface.cpp = 2;
}

static void
z16_read_quad_z(struct softpipe_surface *sps,
                GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLushort *src
      = (GLushort *) sps->surface.ptr + y * sps->surface.stride + x;

   /* converting GLushort to GLuint: */
   zzzz[0] = src[0];
   zzzz[1] = src[1];
   zzzz[2] = src[sps->surface.width];
   zzzz[3] = src[sps->surface.width + 1];
}

static void
z16_write_quad_z(struct softpipe_surface *sps,
                 GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLushort *dst = (GLushort *) sps->surface.ptr + y * sps->surface.stride + x;

   /* converting GLuint to GLushort: */
   dst[0] = zzzz[0];
   dst[1] = zzzz[1];
   dst[sps->surface.width] = zzzz[2];
   dst[sps->surface.width + 1] = zzzz[3];
}

struct softpipe_surface *
softpipe_new_z_surface(GLuint depth)
{
   struct softpipe_surface *sps = CALLOC_STRUCT(softpipe_surface);
   if (!sps)
      return NULL;

   /* XXX ignoring depth param for now */

   sps->surface.format = PIPE_FORMAT_U_Z16;

   sps->surface.resize = z16_resize;
   sps->surface.buffer.map = z16_map;
   sps->surface.buffer.unmap = z16_unmap;
   sps->read_quad_z = z16_read_quad_z;
   sps->write_quad_z = z16_write_quad_z;

   return sps;
}
