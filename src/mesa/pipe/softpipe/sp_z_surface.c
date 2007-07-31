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

static void
z16_read_quad_z(struct softpipe_surface *sps,
                GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLushort *src
      = (GLushort *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z16);

   /* converting GLushort to GLuint: */
   zzzz[0] = src[0];
   zzzz[1] = src[1];
   zzzz[2] = src[sps->surface.region->pitch + 0];
   zzzz[3] = src[sps->surface.region->pitch + 1];
}

static void
z16_write_quad_z(struct softpipe_surface *sps,
                 GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLushort *dst = (GLushort *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z16);

   /* converting GLuint to GLushort: */
   dst[0] = zzzz[0];
   dst[1] = zzzz[1];
   dst[sps->surface.region->pitch + 0] = zzzz[2];
   dst[sps->surface.region->pitch + 1] = zzzz[3];
}

static void
z32_read_quad_z(struct softpipe_surface *sps,
                GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLuint *src
      = (GLuint *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z32);

   zzzz[0] = src[0];
   zzzz[1] = src[1];
   zzzz[2] = src[sps->surface.region->pitch + 0];
   zzzz[3] = src[sps->surface.region->pitch + 1];
}

static void
z32_write_quad_z(struct softpipe_surface *sps,
                 GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLuint *dst = (GLuint *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z32);

   dst[0] = zzzz[0];
   dst[1] = zzzz[1];
   dst[sps->surface.region->pitch + 0] = zzzz[2];
   dst[sps->surface.region->pitch + 1] = zzzz[3];
}

static void
z24s8_read_quad_z(struct softpipe_surface *sps,
                  GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLuint *src
      = (GLuint *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);

   zzzz[0] = src[0] >> 8;
   zzzz[1] = src[1] >> 8;
   zzzz[2] = src[sps->surface.region->pitch + 0] >> 8;
   zzzz[3] = src[sps->surface.region->pitch + 1] >> 8;
}

static void
z24s8_write_quad_z(struct softpipe_surface *sps,
                   GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLuint *dst = (GLuint *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);
   assert(zzzz[0] <= 0xffffff);

   dst[0] = (dst[0] & 0xff) | (zzzz[0] << 8);
   dst[1] = (dst[1] & 0xff) | (zzzz[1] << 8);
   dst += sps->surface.region->pitch;
   dst[0] = (dst[0] & 0xff) | (zzzz[2] << 8);
   dst[1] = (dst[1] & 0xff) | (zzzz[3] << 8);
}

static void
z24s8_read_quad_stencil(struct softpipe_surface *sps,
                        GLint x, GLint y, GLubyte ssss[QUAD_SIZE])
{
   const GLuint *src
      = (GLuint *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);

   ssss[0] = src[0] & 0xff;
   ssss[1] = src[1] & 0xff;
   ssss[2] = src[sps->surface.region->pitch + 0] & 0xff;
   ssss[3] = src[sps->surface.region->pitch + 1] & 0xff;
}

static void
z24s8_write_quad_stencil(struct softpipe_surface *sps,
                         GLint x, GLint y, const GLubyte ssss[QUAD_SIZE])
{
   GLuint *dst = (GLuint *) sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_Z24_S8);

   dst[0] = (dst[0] & 0xffffff00) | ssss[0];
   dst[1] = (dst[1] & 0xffffff00) | ssss[1];
   dst += sps->surface.region->pitch;
   dst[0] = (dst[0] & 0xffffff00) | ssss[2];
   dst[1] = (dst[1] & 0xffffff00) | ssss[3];
}


static void
s8_read_quad_stencil(struct softpipe_surface *sps,
                     GLint x, GLint y, GLubyte ssss[QUAD_SIZE])
{
   const GLubyte *src
      = sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_S8);

   ssss[0] = src[0];
   ssss[1] = src[1];
   src += sps->surface.region->pitch;
   ssss[2] = src[0];
   ssss[3] = src[1];
}

static void
s8_write_quad_stencil(struct softpipe_surface *sps,
                      GLint x, GLint y, const GLubyte ssss[QUAD_SIZE])
{
   GLubyte *dst
      = sps->surface.region->map + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_S8);

   dst[0] = ssss[0];
   dst[1] = ssss[1];
   dst += sps->surface.region->pitch;
   dst[0] = ssss[2];
   dst[1] = ssss[3];
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

   return sps;
}
