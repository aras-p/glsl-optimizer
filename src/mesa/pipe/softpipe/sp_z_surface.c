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
#include "sp_context.h"
#include "sp_surface.h"
#include "sp_z_surface.h"

static void*
z_map(struct pipe_buffer *pb, GLuint access_mode)
{
   struct softpipe_surface *sps = (struct softpipe_surface *) pb;
   sps->surface.ptr = pb->ptr;
   sps->surface.stride = sps->surface.width;
   return pb->ptr;
}

static void
z_unmap(struct pipe_buffer *pb)
{
   struct softpipe_surface *sps = (struct softpipe_surface *) pb;
   sps->surface.ptr = NULL;
   sps->surface.stride = 0;
}

static void
z_resize(struct pipe_surface *ps, GLuint width, GLuint height)
{
   struct softpipe_surface *sps = (struct softpipe_surface *) ps;

   if (sps->surface.buffer.ptr)
      free(sps->surface.buffer.ptr);

   sps->surface.stride = sps->surface.width;
   if (sps->surface.format == PIPE_FORMAT_U_Z16)
      sps->surface.cpp = 2;
   else if (sps->surface.format == PIPE_FORMAT_U_Z32 ||
            sps->surface.format == PIPE_FORMAT_Z24_S8)
      sps->surface.cpp = 4;
   else
      assert(0);

   ps->buffer.ptr = (GLubyte *) malloc(width * height * sps->surface.cpp);
   ps->width = width;
   ps->height = height;

}

static void
z16_read_quad_z(struct softpipe_surface *sps,
                GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLushort *src
      = (GLushort *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z16);

   /* converting GLushort to GLuint: */
   zzzz[0] = src[0];
   zzzz[1] = src[1];
   zzzz[2] = src[sps->surface.width + 0];
   zzzz[3] = src[sps->surface.width + 1];
}

static void
z16_write_quad_z(struct softpipe_surface *sps,
                 GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLushort *dst = (GLushort *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z16);

   /* converting GLuint to GLushort: */
   dst[0] = zzzz[0];
   dst[1] = zzzz[1];
   dst[sps->surface.width + 0] = zzzz[2];
   dst[sps->surface.width + 1] = zzzz[3];
}

static void
z32_read_quad_z(struct softpipe_surface *sps,
                GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLuint *src
      = (GLuint *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z32);

   zzzz[0] = src[0];
   zzzz[1] = src[1];
   zzzz[2] = src[sps->surface.width + 0];
   zzzz[3] = src[sps->surface.width + 1];
}

static void
z32_write_quad_z(struct softpipe_surface *sps,
                 GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLuint *dst = (GLuint *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z32);

   dst[0] = zzzz[0];
   dst[1] = zzzz[1];
   dst[sps->surface.width + 0] = zzzz[2];
   dst[sps->surface.width + 1] = zzzz[3];
}

static void
z24s8_read_quad_z(struct softpipe_surface *sps,
                  GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLuint *src
      = (GLuint *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);

   zzzz[0] = src[0] >> 8;
   zzzz[1] = src[1] >> 8;
   zzzz[2] = src[sps->surface.width + 0] >> 8;
   zzzz[3] = src[sps->surface.width + 1] >> 8;
}

static void
z24s8_write_quad_z(struct softpipe_surface *sps,
                   GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLuint *dst = (GLuint *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);
   assert(zzzz[0] <= 0xffffff);

   dst[0] = (dst[0] & 0xff) | (zzzz[0] << 8);
   dst[1] = (dst[1] & 0xff) | (zzzz[1] << 8);
   dst += sps->surface.width;
   dst[0] = (dst[0] & 0xff) | (zzzz[2] << 8);
   dst[1] = (dst[1] & 0xff) | (zzzz[3] << 8);
}

static void
z24s8_read_quad_stencil(struct softpipe_surface *sps,
                        GLint x, GLint y, GLubyte ssss[QUAD_SIZE])
{
   const GLuint *src
      = (GLuint *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);

   ssss[0] = src[0] & 0xff;
   ssss[1] = src[1] & 0xff;
   ssss[2] = src[sps->surface.width + 0] & 0xff;
   ssss[3] = src[sps->surface.width + 1] & 0xff;
}

static void
z24s8_write_quad_stencil(struct softpipe_surface *sps,
                         GLint x, GLint y, const GLubyte ssss[QUAD_SIZE])
{
   GLuint *dst = (GLuint *) sps->surface.ptr + y * sps->surface.stride + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);

   dst[0] = (dst[0] & 0xffffff00) | ssss[0];
   dst[1] = (dst[1] & 0xffffff00) | ssss[1];
   dst += sps->surface.width;
   dst[0] = (dst[0] & 0xffffff00) | ssss[2];
   dst[1] = (dst[1] & 0xffffff00) | ssss[3];
}



/**
 * Create a new software-based Z buffer.
 * \param format  one of the PIPE_FORMAT_z* formats
 */
struct softpipe_surface *
softpipe_new_z_surface(GLuint format)
{
   struct softpipe_surface *sps = CALLOC_STRUCT(softpipe_surface);
   if (!sps)
      return NULL;

   sps->surface.format = format;
   sps->surface.resize = z_resize;
   sps->surface.buffer.map = z_map;
   sps->surface.buffer.unmap = z_unmap;

   if (format == PIPE_FORMAT_U_Z16) {
      sps->read_quad_z = z16_read_quad_z;
      sps->write_quad_z = z16_write_quad_z;
   }
   else if (format == PIPE_FORMAT_U_Z32) {
      sps->read_quad_z = z32_read_quad_z;
      sps->write_quad_z = z32_write_quad_z;
   }
   else if (format == PIPE_FORMAT_Z24_S8) {
      sps->read_quad_z = z24s8_read_quad_z;
      sps->write_quad_z = z24s8_write_quad_z;
      sps->read_quad_stencil = z24s8_read_quad_stencil;
      sps->write_quad_stencil = z24s8_write_quad_stencil;
   }
   else {
      assert(0);
   }

   return sps;
}
