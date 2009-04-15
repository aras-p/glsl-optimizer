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
#include "st_cb_accum.h"
#include "st_cb_fbo.h"
#include "st_draw.h"
#include "st_public.h"
#include "st_format.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "util/u_tile.h"


#define UNCLAMPED_FLOAT_TO_SHORT(us, f)  \
   us = ( (short) ( CLAMP((f), -1.0, 1.0) * 32767.0F) )


/**
 * For hardware that supports deep color buffers, we could accelerate
 * most/all the accum operations with blending/texturing.
 * For now, just use the get/put_tile() functions and do things in software.
 */


/**
 * Wrapper for pipe_get_tile_rgba().  Do format/cpp override to make the
 * tile util function think the surface is 16bit/channel, even if it's not.
 * See also: st_renderbuffer_alloc_storage()
 */
static void
acc_get_tile_rgba(struct pipe_context *pipe, struct pipe_transfer *acc_pt,
                  uint x, uint y, uint w, uint h, float *p)
{
   const enum pipe_format f = acc_pt->format;
   const struct pipe_format_block b = acc_pt->block;

   acc_pt->format = DEFAULT_ACCUM_PIPE_FORMAT;
   acc_pt->block.size = 8;
   acc_pt->block.width = 1;
   acc_pt->block.height = 1;

   pipe_get_tile_rgba(acc_pt, x, y, w, h, p);

   acc_pt->format = f;
   acc_pt->block = b;
}


/**
 * Wrapper for pipe_put_tile_rgba().  Do format/cpp override to make the
 * tile util function think the surface is 16bit/channel, even if it's not.
 * See also: st_renderbuffer_alloc_storage()
 */
static void
acc_put_tile_rgba(struct pipe_context *pipe, struct pipe_transfer *acc_pt,
                  uint x, uint y, uint w, uint h, const float *p)
{
   enum pipe_format f = acc_pt->format;
   const struct pipe_format_block b = acc_pt->block;

   acc_pt->format = DEFAULT_ACCUM_PIPE_FORMAT;
   acc_pt->block.size = 8;
   acc_pt->block.width = 1;
   acc_pt->block.height = 1;

   pipe_put_tile_rgba(acc_pt, x, y, w, h, p);

   acc_pt->format = f;
   acc_pt->block = b;
}



void
st_clear_accum_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *acc_strb = st_renderbuffer(rb);
   struct pipe_transfer *acc_pt;
   struct pipe_screen *screen = ctx->st->pipe->screen;
   const GLint xpos = ctx->DrawBuffer->_Xmin;
   const GLint ypos = ctx->DrawBuffer->_Ymin;
   const GLint width = ctx->DrawBuffer->_Xmax - xpos;
   const GLint height = ctx->DrawBuffer->_Ymax - ypos;
   GLubyte *map;

   st_teximage_flush_before_map(ctx->st, acc_strb->texture, 0, 0,
				PIPE_TRANSFER_WRITE);

   acc_pt = screen->get_tex_transfer(screen, acc_strb->texture, 0, 0, 0,
                                     PIPE_TRANSFER_WRITE, xpos, ypos,
                                     width, height);
   map = screen->transfer_map(screen, acc_pt);

   /* note acc_strb->format might not equal acc_pt->format */
   switch (acc_strb->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         GLshort r = FLOAT_TO_SHORT(ctx->Accum.ClearColor[0]);
         GLshort g = FLOAT_TO_SHORT(ctx->Accum.ClearColor[1]);
         GLshort b = FLOAT_TO_SHORT(ctx->Accum.ClearColor[2]);
         GLshort a = FLOAT_TO_SHORT(ctx->Accum.ClearColor[3]);
         int i, j;
         for (i = 0; i < height; i++) {
            GLshort *dst = (GLshort *) (map + i * acc_pt->stride + xpos * 8);
            for (j = 0; j < width; j++) {
               dst[0] = r;
               dst[1] = g;
               dst[2] = b;
               dst[3] = a;
               dst += 4;
            }
         }
      }
      break;
   default:
      _mesa_problem(ctx, "unexpected format in st_clear_accum_buffer()");
   }

   screen->transfer_unmap(screen, acc_pt);
   screen->tex_transfer_destroy(acc_pt);
}


/** For ADD/MULT */
static void
accum_mad(GLcontext *ctx, GLfloat scale, GLfloat bias,
          GLint xpos, GLint ypos, GLint width, GLint height,
          struct st_renderbuffer *acc_strb)
{
   struct pipe_screen *screen = ctx->st->pipe->screen;
   struct pipe_transfer *acc_pt;
   GLubyte *map;

   st_teximage_flush_before_map(ctx->st, acc_strb->texture, 0, 0,
				PIPE_TRANSFER_READ_WRITE);

   acc_pt = screen->get_tex_transfer(screen, acc_strb->texture, 0, 0, 0,
                                     PIPE_TRANSFER_READ_WRITE, xpos, ypos,
                                     width, height);
   map = screen->transfer_map(screen, acc_pt);

   /* note acc_strb->format might not equal acc_pt->format */
   switch (acc_strb->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         int i, j;
         for (i = 0; i < height; i++) {
            GLshort *acc = (GLshort *) (map + (ypos + i) * acc_pt->stride + xpos * 8);
            for (j = 0; j < width * 4; j++) {
               float val = SHORT_TO_FLOAT(acc[j]) * scale + bias;
               acc[j] = FLOAT_TO_SHORT(val);
            }
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in st_clear_accum_buffer()");
   }

   screen->transfer_unmap(screen, acc_pt);
   screen->tex_transfer_destroy(acc_pt);
}


static void
accum_accum(struct st_context *st, GLfloat value,
            GLint xpos, GLint ypos, GLint width, GLint height,
            struct st_renderbuffer *acc_strb,
            struct st_renderbuffer *color_strb)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_transfer *acc_trans, *color_trans;
   GLfloat *colorBuf, *accBuf;
   GLint i;

   st_teximage_flush_before_map(st, acc_strb->texture, 0, 0,
				PIPE_TRANSFER_READ);

   acc_trans = screen->get_tex_transfer(screen, acc_strb->texture, 0, 0, 0,
                                        PIPE_TRANSFER_READ, xpos, ypos,
                                        width, height);

   st_teximage_flush_before_map(st, color_strb->texture, 0, 0,
				PIPE_TRANSFER_READ);

   color_trans = screen->get_tex_transfer(screen, color_strb->texture, 0, 0, 0,
                                          PIPE_TRANSFER_READ, xpos, ypos,
                                          width, height);

   colorBuf = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
   accBuf = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));

   pipe_get_tile_rgba(color_trans, 0, 0, width, height, colorBuf);
   acc_get_tile_rgba(pipe, acc_trans, 0, 0, width, height, accBuf);

   for (i = 0; i < 4 * width * height; i++) {
      accBuf[i] = accBuf[i] + colorBuf[i] * value;
   }

   screen->tex_transfer_destroy(acc_trans);
   acc_trans = screen->get_tex_transfer(screen, acc_strb->texture, 0, 0, 0,
                                        PIPE_TRANSFER_WRITE, xpos, ypos,
                                        width, height);

   acc_put_tile_rgba(pipe, acc_trans, 0, 0, width, height, accBuf);

   _mesa_free(colorBuf);
   _mesa_free(accBuf);
   screen->tex_transfer_destroy(acc_trans);
   screen->tex_transfer_destroy(color_trans);
}


static void
accum_load(struct st_context *st, GLfloat value,
           GLint xpos, GLint ypos, GLint width, GLint height,
           struct st_renderbuffer *acc_strb,
           struct st_renderbuffer *color_strb)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_transfer *acc_trans, *color_trans;
   GLfloat *buf;
   GLint i;

   st_teximage_flush_before_map(st, acc_strb->texture, 0, 0,
				PIPE_TRANSFER_WRITE);

   acc_trans = screen->get_tex_transfer(screen, acc_strb->texture, 0, 0, 0,
                                        PIPE_TRANSFER_WRITE, xpos, ypos,
                                        width, height);

   st_teximage_flush_before_map(st, color_strb->texture, 0, 0,
				PIPE_TRANSFER_READ);

   color_trans = screen->get_tex_transfer(screen, color_strb->texture, 0, 0, 0,
                                        PIPE_TRANSFER_READ, xpos, ypos,
                                        width, height);

   buf = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));

   pipe_get_tile_rgba(color_trans, 0, 0, width, height, buf);

   for (i = 0; i < 4 * width * height; i++) {
      buf[i] = buf[i] * value;
   }

   acc_put_tile_rgba(pipe, acc_trans, 0, 0, width, height, buf);

   _mesa_free(buf);
   screen->tex_transfer_destroy(acc_trans);
   screen->tex_transfer_destroy(color_trans);
}


static void
accum_return(GLcontext *ctx, GLfloat value,
             GLint xpos, GLint ypos, GLint width, GLint height,
             struct st_renderbuffer *acc_strb,
             struct st_renderbuffer *color_strb)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen = pipe->screen;
   const GLubyte *colormask = ctx->Color.ColorMask;
   struct pipe_transfer *acc_trans, *color_trans;
   GLfloat *abuf, *cbuf = NULL;
   GLint i, ch;

   abuf = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));

   st_teximage_flush_before_map(ctx->st, acc_strb->texture, 0, 0,
				PIPE_TRANSFER_READ);

   acc_trans = screen->get_tex_transfer(screen, acc_strb->texture, 0, 0, 0,
                                        PIPE_TRANSFER_READ, xpos, ypos,
                                        width, height);

   st_teximage_flush_before_map(ctx->st, color_strb->texture, 0, 0,
				PIPE_TRANSFER_READ_WRITE);

   color_trans = screen->get_tex_transfer(screen, color_strb->texture, 0, 0, 0,
                                          PIPE_TRANSFER_READ_WRITE, xpos, ypos,
                                          width, height);

   acc_get_tile_rgba(pipe, acc_trans, 0, 0, width, height, abuf);

   if (!colormask[0] || !colormask[1] || !colormask[2] || !colormask[3]) {
      cbuf = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
      pipe_get_tile_rgba(color_trans, 0, 0, width, height, cbuf);
   }

   for (i = 0; i < width * height; i++) {
      for (ch = 0; ch < 4; ch++) {
         if (colormask[ch]) {
            GLfloat val = abuf[i * 4 + ch] * value;
            abuf[i * 4 + ch] = CLAMP(val, 0.0f, 1.0f);
         }
         else {
            abuf[i * 4 + ch] = cbuf[i * 4 + ch];
         }
      }
   }

   pipe_put_tile_rgba(color_trans, 0, 0, width, height, abuf);

   _mesa_free(abuf);
   if (cbuf)
      _mesa_free(cbuf);
   screen->tex_transfer_destroy(acc_trans);
   screen->tex_transfer_destroy(color_trans);
}


static void
st_Accum(GLcontext *ctx, GLenum op, GLfloat value)
{
   struct st_context *st = ctx->st;
   struct st_renderbuffer *acc_strb
     = st_renderbuffer(ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer);
   struct st_renderbuffer *color_strb
      = st_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);

   const GLint xpos = ctx->DrawBuffer->_Xmin;
   const GLint ypos = ctx->DrawBuffer->_Ymin;
   const GLint width = ctx->DrawBuffer->_Xmax - xpos;
   const GLint height = ctx->DrawBuffer->_Ymax - ypos;

   /* make sure color bufs aren't cached */
   st_flush( st, PIPE_FLUSH_RENDER_CACHE, NULL );

   switch (op) {
   case GL_ADD:
      if (value != 0.0F) {
         accum_mad(ctx, 1.0, value, xpos, ypos, width, height, acc_strb);
      }
      break;
   case GL_MULT:
      if (value != 1.0F) {
         accum_mad(ctx, value, 0.0, xpos, ypos, width, height, acc_strb);
      }
      break;
   case GL_ACCUM:
      if (value != 0.0F) {
         accum_accum(st, value, xpos, ypos, width, height, acc_strb, color_strb);
      }
      break;
   case GL_LOAD:
      accum_load(st, value, xpos, ypos, width, height, acc_strb, color_strb);
      break;
   case GL_RETURN:
      accum_return(ctx, value, xpos, ypos, width, height, acc_strb, color_strb);
      break;
   default:
      assert(0);
   }
}



void st_init_accum_functions(struct dd_function_table *functions)
{
   functions->Accum = st_Accum;
}
