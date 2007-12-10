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

 /*
  * Authors:
  *   Brian Paul
  */

#include "main/imports.h"
#include "main/image.h"
#include "main/macros.h"

#include "st_context.h"
#include "st_cache.h"
#include "st_cb_accum.h"
#include "st_cb_fbo.h"
#include "st_draw.h"
#include "st_format.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"


#define UNCLAMPED_FLOAT_TO_SHORT(us, f)  \
   us = ( (short) ( CLAMP((f), -1.0, 1.0) * 32767.0F) )


/**
 * For hardware that supports deep color buffers, we could accelerate
 * most/all the accum operations with blending/texturing.
 * For now, just use the get/put_tile() functions and do things in software.
 */


void
st_clear_accum_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *acc_strb = st_renderbuffer(rb);
   struct pipe_surface *acc_ps = acc_strb->surface;
   const GLint xpos = ctx->DrawBuffer->_Xmin;
   const GLint ypos = ctx->DrawBuffer->_Ymin;
   const GLint width = ctx->DrawBuffer->_Xmax - xpos;
   const GLint height = ctx->DrawBuffer->_Ymax - ypos;
   const GLfloat r = ctx->Accum.ClearColor[0];
   const GLfloat g = ctx->Accum.ClearColor[1];
   const GLfloat b = ctx->Accum.ClearColor[2];
   const GLfloat a = ctx->Accum.ClearColor[3];

   (void) pipe_surface_map(acc_ps);

   switch (acc_ps->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         const short sr = (short) (32767 * r);
         const short sg = (short) (32767 * g);
         const short sb = (short) (32767 * b);
         const short sa = (short) (32767 * a);
         short *acc = ((short *) acc_ps->map)
            + (ypos * acc_ps->pitch + xpos) * 4;
         int i, j;
         for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
               acc[j*4+0] = sr;
               acc[j*4+1] = sg;
               acc[j*4+2] = sb;
               acc[j*4+3] = sa;
            }
            acc += acc_ps->pitch * 4;
         }
      }
      break;
   default:
      assert(0);
   }

   pipe_surface_unmap(acc_ps);
}


/** Get block of values from accum buffer, converting to float */
static void
get_accum_tile(struct pipe_context *pipe,
               struct pipe_surface *acc_surf,
               int xpos, int ypos, int width, int height,
               float *buf)
{
   switch (acc_surf->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         const short *acc = ((const short *) acc_surf->map)
            + (ypos * acc_surf->pitch + xpos) * 4;
         int i, j;
         for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
               buf[j*4+0] = SHORT_TO_FLOAT(acc[j*4+0]);
               buf[j*4+1] = SHORT_TO_FLOAT(acc[j*4+1]);
               buf[j*4+2] = SHORT_TO_FLOAT(acc[j*4+2]);
               buf[j*4+3] = SHORT_TO_FLOAT(acc[j*4+3]);
            }
            acc += acc_surf->pitch * 4;
            buf += width * 4;
         }
      }
      break;
   default:
      assert(0);
   }
}


/** Put block of values into accum buffer, converting from float */
static void
put_accum_tile(struct pipe_context *pipe,
               struct pipe_surface *acc_surf,
               int xpos, int ypos, int width, int height,
               const float *buf)
{
   switch (acc_surf->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         short *acc = ((short *) acc_surf->map)
            + (ypos * acc_surf->pitch + xpos) * 4;
         int i, j;
         for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
               short r, g, b, a;
               UNCLAMPED_FLOAT_TO_SHORT(r, buf[j*4+0]);
               UNCLAMPED_FLOAT_TO_SHORT(g, buf[j*4+1]);
               UNCLAMPED_FLOAT_TO_SHORT(b, buf[j*4+2]);
               UNCLAMPED_FLOAT_TO_SHORT(a, buf[j*4+3]);
               acc[j*4+0] = r;
               acc[j*4+1] = g;
               acc[j*4+2] = b;
               acc[j*4+3] = a;
            }
            acc += acc_surf->pitch * 4;
            buf += width * 4;
         }
      }
      break;
   default:
      assert(0);
   }
}


/** For ADD/MULT */
static void
accum_mad(struct pipe_context *pipe, GLfloat scale, GLfloat bias,
          GLint xpos, GLint ypos, GLint width, GLint height,
          struct pipe_surface *acc_ps)
{
   GLfloat *accBuf;
   GLint i;

   accBuf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   (void) pipe_surface_map(acc_ps);

   get_accum_tile(pipe, acc_ps, xpos, ypos, width, height, accBuf);

   for (i = 0; i < 4 * width * height; i++) {
      accBuf[i] = accBuf[i] * scale + bias;
   }

   put_accum_tile(pipe, acc_ps, xpos, ypos, width, height, accBuf);

   free(accBuf);

   pipe_surface_unmap(acc_ps);
}


static void
accum_accum(struct pipe_context *pipe, GLfloat value,
            GLint xpos, GLint ypos, GLint width, GLint height,
            struct pipe_surface *acc_ps,
            struct pipe_surface *color_ps)
{
   ubyte *colorMap, *accMap;
   GLfloat *colorBuf, *accBuf;
   GLint i;

   colorBuf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));
   accBuf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   colorMap = pipe_surface_map(color_ps);
   accMap = pipe_surface_map(acc_ps);

   pipe->get_tile_rgba(pipe, color_ps, xpos, ypos, width, height, colorBuf);
   get_accum_tile(pipe, acc_ps, xpos, ypos, width, height, accBuf);

   for (i = 0; i < 4 * width * height; i++) {
      accBuf[i] = accBuf[i] + colorBuf[i] * value;
   }

   put_accum_tile(pipe, acc_ps, xpos, ypos, width, height, accBuf);

   free(colorBuf);
   free(accBuf);

   pipe_surface_unmap(color_ps);
   pipe_surface_unmap(acc_ps);
}


static void
accum_load(struct pipe_context *pipe, GLfloat value,
           GLint xpos, GLint ypos, GLint width, GLint height,
           struct pipe_surface *acc_ps,
           struct pipe_surface *color_ps)
{
   GLfloat *buf;
   GLint i;

   buf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   (void) pipe_surface_map(color_ps);
   (void) pipe_surface_map(acc_ps);

   pipe->get_tile_rgba(pipe, color_ps, xpos, ypos, width, height, buf);

   for (i = 0; i < 4 * width * height; i++) {
      buf[i] = buf[i] * value;
   }

   put_accum_tile(pipe, acc_ps, xpos, ypos, width, height, buf);

   free(buf);

   pipe_surface_unmap(color_ps);
   pipe_surface_unmap(acc_ps);
}


static void
accum_return(GLcontext *ctx, GLfloat value,
             GLint xpos, GLint ypos, GLint width, GLint height,
             struct pipe_surface *acc_ps,
             struct pipe_surface *color_ps)
{
   struct pipe_context *pipe = ctx->st->pipe;
   const GLubyte *colormask = ctx->Color.ColorMask;
   GLfloat *abuf, *cbuf = NULL;
   GLint i, ch;

   abuf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   (void) pipe_surface_map(color_ps);
   (void) pipe_surface_map(acc_ps);

   get_accum_tile(pipe, acc_ps, xpos, ypos, width, height, abuf);

   if (!colormask[0] || !colormask[1] || !colormask[2] || !colormask[3]) {
      cbuf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));
      pipe->get_tile_rgba(pipe, color_ps, xpos, ypos, width, height, cbuf);
   }

   for (i = 0; i < width * height; i++) {
      for (ch = 0; ch < 4; ch++) {
         if (colormask[ch]) {
            GLfloat val = abuf[i * 4 + ch] * value;
            abuf[i * 4 + ch] = CLAMP(val, 0.0, 1.0);
         }
         else {
            abuf[i * 4 + ch] = cbuf[i * 4 + ch];
         }
      }
   }

   pipe->put_tile_rgba(pipe, color_ps, xpos, ypos, width, height, abuf);

   free(abuf);
   if (cbuf)
      free(cbuf);

   pipe_surface_unmap(color_ps);
   pipe_surface_unmap(acc_ps);
}


static void
st_Accum(GLcontext *ctx, GLenum op, GLfloat value)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   struct st_renderbuffer *acc_strb
     = st_renderbuffer(ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer);
   struct st_renderbuffer *color_strb
      = st_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
   struct pipe_surface *acc_ps = acc_strb->surface;
   struct pipe_surface *color_ps = color_strb->surface;

   const GLint xpos = ctx->DrawBuffer->_Xmin;
   const GLint ypos = ctx->DrawBuffer->_Ymin;
   const GLint width = ctx->DrawBuffer->_Xmax - xpos;
   const GLint height = ctx->DrawBuffer->_Ymax - ypos;

   /* make sure color bufs aren't cached */
   pipe->flush(pipe, 0);

   switch (op) {
   case GL_ADD:
      if (value != 0.0F) {
         accum_mad(pipe, 1.0, value, xpos, ypos, width, height, acc_ps);
      }
      break;
   case GL_MULT:
      if (value != 1.0F) {
         accum_mad(pipe, value, 0.0, xpos, ypos, width, height, acc_ps);
      }
      break;
   case GL_ACCUM:
      if (value != 0.0F) {
         accum_accum(pipe, value, xpos, ypos, width, height, acc_ps, color_ps);
      }
      break;
   case GL_LOAD:
      accum_load(pipe, value, xpos, ypos, width, height, acc_ps, color_ps);
      break;
   case GL_RETURN:
      accum_return(ctx, value, xpos, ypos, width, height, acc_ps, color_ps);
      break;
   default:
      assert(0);
   }
}



void st_init_accum_functions(struct dd_function_table *functions)
{
   functions->Accum = st_Accum;
}
