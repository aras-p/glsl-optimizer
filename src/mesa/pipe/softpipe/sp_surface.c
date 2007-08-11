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
#include "pipe/p_defines.h"
#include "main/imports.h"
#include "main/macros.h"


/**
 * Softpipe surface functions.
 * Basically, create surface of a particular type, then plug in default
 * read/write_quad and get/put_tile() functions.
 * Note that these quad funcs assume the buffer/region is in a linear
 * layout with Y=0=top.
 * If we had swizzled/AOS buffers the read/write quad functions could be
 * simplified a lot....
 */



/*** PIPE_FORMAT_U_A8_R8_G8_B8 ***/

static void
a8r8g8b8_read_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                         GLfloat (*rrrr)[QUAD_SIZE])
{
   const GLuint *src
      = ((const GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;
   GLuint i, j;

   assert(sps->surface.format == PIPE_FORMAT_U_A8_R8_G8_B8);
   assert(x < sps->surface.width - 1);
   assert(y < sps->surface.height - 1);

   for (i = 0; i < 2; i++) { /* loop over pixel row */
      for (j = 0; j < 2; j++) {  /* loop over pixel column */
         const GLuint p = src[j];
         rrrr[0][i * 2 + j] = UBYTE_TO_FLOAT((p >> 16) & 0xff); /*R*/
         rrrr[1][i * 2 + j] = UBYTE_TO_FLOAT((p >>  8) & 0xff); /*G*/
         rrrr[2][i * 2 + j] = UBYTE_TO_FLOAT((p      ) & 0xff); /*B*/
         rrrr[3][i * 2 + j] = UBYTE_TO_FLOAT((p >> 24) & 0xff); /*A*/
      }
      src += sps->surface.region->pitch;
   }
}

static void
a8r8g8b8_write_quad_f_swz(struct softpipe_surface *sps, GLint x, GLint y,
                          GLfloat (*rrrr)[QUAD_SIZE])
{
   GLuint *dst
      = ((GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;
   GLuint i, j;

   assert(sps->surface.format == PIPE_FORMAT_U_A8_R8_G8_B8);

   for (i = 0; i < 2; i++) { /* loop over pixel row */
      for (j = 0; j < 2; j++) {  /* loop over pixel column */
         GLubyte r, g, b, a;
         UNCLAMPED_FLOAT_TO_UBYTE(r, rrrr[0][i * 2 + j]); /*R*/
         UNCLAMPED_FLOAT_TO_UBYTE(g, rrrr[1][i * 2 + j]); /*G*/
         UNCLAMPED_FLOAT_TO_UBYTE(b, rrrr[2][i * 2 + j]); /*B*/
         UNCLAMPED_FLOAT_TO_UBYTE(a, rrrr[3][i * 2 + j]); /*A*/
         dst[j] = (a << 24) | (r << 16) | (g << 8) | b;
      }
      dst += sps->surface.region->pitch;
   }
}

static void
a8r8g8b8_get_tile(struct pipe_surface *ps,
                  GLuint x, GLuint y, GLuint w, GLuint h, GLfloat *p)
{
   const GLuint *src
      = ((const GLuint *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   GLuint i, j;
   GLuint w0 = w;

   assert(ps->format == PIPE_FORMAT_U_A8_R8_G8_B8);

#if 0
   assert(x + w <= ps->width);
   assert(y + h <= ps->height);
#else
   /* temp clipping hack */
   if (x + w > ps->width)
      w = ps->width - x;
   if (y + h > ps->height)
      h = ps->height -y;
#endif
   for (i = 0; i < h; i++) {
      GLfloat *pRow = p;
      for (j = 0; j < w; j++) {
         const GLuint pixel = src[j];
         pRow[0] = UBYTE_TO_FLOAT((pixel >> 16) & 0xff);
         pRow[1] = UBYTE_TO_FLOAT((pixel >>  8) & 0xff);
         pRow[2] = UBYTE_TO_FLOAT((pixel >>  0) & 0xff);
         pRow[3] = UBYTE_TO_FLOAT((pixel >> 24) & 0xff);
         pRow += 4;
      }
      src += ps->region->pitch;
      p += w0 * 4;
   }
}


/*** PIPE_FORMAT_U_A1_R5_G5_B5 ***/

static void
a1r5g5b5_get_tile(struct pipe_surface *ps,
                  GLuint x, GLuint y, GLuint w, GLuint h, GLfloat *p)
{
   const GLushort *src
      = ((const GLushort *) (ps->region->map + ps->offset))
      + y * ps->region->pitch + x;
   GLuint i, j;

   assert(ps->format == PIPE_FORMAT_U_A1_R5_G5_B5);

   for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
         const GLushort pixel = src[j];
         p[0] = ((pixel >> 10) & 0x1f) * (1.0 / 31);
         p[1] = ((pixel >>  5) & 0x1f) * (1.0 / 31);
         p[2] = ((pixel      ) & 0x1f) * (1.0 / 31);
         p[3] = ((pixel >> 15)       );
         p += 4;
      }
      src += ps->region->pitch;
   }
}



/*** PIPE_FORMAT_U_Z16 ***/

static void
z16_read_quad_z(struct softpipe_surface *sps,
                GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLushort *src
      = ((const GLushort *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z16);

   /* converting GLushort to GLuint: */
   zzzz[0] = src[0];
   zzzz[1] = src[1];
   src += sps->surface.region->pitch;
   zzzz[2] = src[0];
   zzzz[3] = src[1];
}

static void
z16_write_quad_z(struct softpipe_surface *sps,
                 GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLushort *dst
      = ((GLushort *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z16);

   /* converting GLuint to GLushort: */
   dst[0] = zzzz[0];
   dst[1] = zzzz[1];
   dst += sps->surface.region->pitch;
   dst[0] = zzzz[2];
   dst[1] = zzzz[3];
}


/*** PIPE_FORMAT_U_Z32 ***/

static void
z32_read_quad_z(struct softpipe_surface *sps,
                GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   const GLuint *src
      = ((GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z32);

   zzzz[0] = src[0];
   zzzz[1] = src[1];
   src += sps->surface.region->pitch;
   zzzz[2] = src[0];
   zzzz[3] = src[1];
}

static void
z32_write_quad_z(struct softpipe_surface *sps,
                 GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   GLuint *dst
      = ((GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_Z32);

   dst[0] = zzzz[0];
   dst[1] = zzzz[1];
   dst += sps->surface.region->pitch;
   dst[0] = zzzz[2];
   dst[1] = zzzz[3];
}


/*** PIPE_FORMAT_S8_Z24 ***/

static void
s8z24_read_quad_z(struct softpipe_surface *sps,
                  GLint x, GLint y, GLuint zzzz[QUAD_SIZE])
{
   static const GLuint mask = 0x00ffffff;
   const GLuint *src
      = ((GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);

   /* extract lower three bytes */
   zzzz[0] = src[0] & mask;
   zzzz[1] = src[1] & mask;
   src += sps->surface.region->pitch;
   zzzz[2] = src[0] & mask;
   zzzz[3] = src[1] & mask;
}

static void
s8z24_write_quad_z(struct softpipe_surface *sps,
                   GLint x, GLint y, const GLuint zzzz[QUAD_SIZE])
{
   static const GLuint mask = 0xff000000;
   GLuint *dst
      = ((GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);
   assert(zzzz[0] <= 0xffffff);

   dst[0] = (dst[0] & mask) | zzzz[0];
   dst[1] = (dst[1] & mask) | zzzz[1];
   dst += sps->surface.region->pitch;
   dst[0] = (dst[0] & mask) | zzzz[2];
   dst[1] = (dst[1] & mask) | zzzz[3];
}

static void
s8z24_read_quad_stencil(struct softpipe_surface *sps,
                        GLint x, GLint y, GLubyte ssss[QUAD_SIZE])
{
   const GLuint *src
      = ((GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);

   ssss[0] = src[0] >> 24;
   ssss[1] = src[1] >> 24;
   src += sps->surface.region->pitch;
   ssss[2] = src[0] >> 24;
   ssss[3] = src[1] >> 24;
}

static void
s8z24_write_quad_stencil(struct softpipe_surface *sps,
                         GLint x, GLint y, const GLubyte ssss[QUAD_SIZE])
{
   static const GLuint mask = 0x00ffffff;
   GLuint *dst
      = ((GLuint *) (sps->surface.region->map + sps->surface.offset))
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_S8_Z24);

   dst[0] = (dst[0] & mask) | (ssss[0] << 24);
   dst[1] = (dst[1] & mask) | (ssss[1] << 24);
   dst += sps->surface.region->pitch;
   dst[0] = (dst[0] & mask) | (ssss[2] << 24);
   dst[1] = (dst[1] & mask) | (ssss[3] << 24);
}


/*** PIPE_FORMAT_U_S8 ***/

static void
s8_read_quad_stencil(struct softpipe_surface *sps,
                     GLint x, GLint y, GLubyte ssss[QUAD_SIZE])
{
   const GLubyte *src
      = sps->surface.region->map + sps->surface.offset
      + y * sps->surface.region->pitch + x;

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
      = sps->surface.region->map + sps->surface.offset
      + y * sps->surface.region->pitch + x;

   assert(sps->surface.format == PIPE_FORMAT_U_S8);

   dst[0] = ssss[0];
   dst[1] = ssss[1];
   dst += sps->surface.region->pitch;
   dst[0] = ssss[2];
   dst[1] = ssss[3];
}


/**
 * Initialize the quad_read/write and get/put_tile() methods.
 */
void
softpipe_init_surface_funcs(struct softpipe_surface *sps)
{
   assert(sps->surface.format);

   switch (sps->surface.format) {
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      sps->read_quad_f_swz = a8r8g8b8_read_quad_f_swz;
      sps->write_quad_f_swz = a8r8g8b8_write_quad_f_swz;
      sps->surface.get_tile = a8r8g8b8_get_tile;
      break;
   case PIPE_FORMAT_U_A1_R5_G5_B5:
      sps->surface.get_tile = a1r5g5b5_get_tile;
      break;
   case PIPE_FORMAT_U_Z16:
      sps->read_quad_z = z16_read_quad_z;
      sps->write_quad_z = z16_write_quad_z;
      break;
   case PIPE_FORMAT_U_Z32:
      sps->read_quad_z = z32_read_quad_z;
      sps->write_quad_z = z32_write_quad_z;
      break;
   case PIPE_FORMAT_S8_Z24:
      sps->read_quad_z = s8z24_read_quad_z;
      sps->write_quad_z = s8z24_write_quad_z;
      sps->read_quad_stencil = s8z24_read_quad_stencil;
      sps->write_quad_stencil = s8z24_write_quad_stencil;
      break;
   case PIPE_FORMAT_U_S8:
      sps->read_quad_stencil = s8_read_quad_stencil;
      sps->write_quad_stencil = s8_write_quad_stencil;
      break;
   default:
      assert(0);
   }
}


static struct pipe_surface *
softpipe_surface_alloc(struct pipe_context *pipe, GLuint pipeFormat)
{
   struct softpipe_surface *sps = CALLOC_STRUCT(softpipe_surface);
   if (!sps)
      return NULL;

   assert(pipeFormat < PIPE_FORMAT_COUNT);

   sps->surface.format = pipeFormat;
   sps->surface.refcount = 1;
   softpipe_init_surface_funcs(sps);

   return &sps->surface;
}





/**
 * Called via pipe->get_tex_surface()
 * XXX is this in the right place?
 */
struct pipe_surface *
softpipe_get_tex_surface(struct pipe_context *pipe,
                         struct pipe_mipmap_tree *mt,
                         GLuint face, GLuint level, GLuint zslice)
{
   struct pipe_surface *ps;
   GLuint offset;  /* in bytes */

   offset = mt->level[level].level_offset;

   if (mt->target == GL_TEXTURE_CUBE_MAP_ARB) {
      offset += mt->level[level].image_offset[face] * mt->cpp;
   }
   else if (mt->target == GL_TEXTURE_3D) {
      offset += mt->level[level].image_offset[zslice] * mt->cpp;
   }
   else {
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = pipe->surface_alloc(pipe, mt->format);
   if (ps) {
      assert(ps->format);
      assert(ps->refcount);
      ps->region = mt->region;
      ps->width = mt->level[level].width;
      ps->height = mt->level[level].height;
      ps->offset = offset;
   }
   return ps;
}


void
sp_init_surface_functions(struct softpipe_context *sp)
{
   sp->pipe.surface_alloc = softpipe_surface_alloc;
}
