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

#include "st_debug.h"
#include "st_context.h"
#include "st_cb_accum.h"
#include "st_cb_fbo.h"
#include "st_public.h"
#include "st_texture.h"
#include "st_inlines.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_tile.h"


/**
 * For hardware that supports deep color buffers, we could accelerate
 * most/all the accum operations with blending/texturing.
 * For now, just use the get/put_tile() functions and do things in software.
 */


void
st_clear_accum_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *acc_strb = st_renderbuffer(rb);
   const GLint xpos = ctx->DrawBuffer->_Xmin;
   const GLint ypos = ctx->DrawBuffer->_Ymin;
   const GLint width = ctx->DrawBuffer->_Xmax - xpos;
   const GLint height = ctx->DrawBuffer->_Ymax - ypos;
   size_t stride = acc_strb->stride;
   GLubyte *data = acc_strb->data;

   if(!data)
      return;
   
   switch (acc_strb->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         GLshort r = FLOAT_TO_SHORT(ctx->Accum.ClearColor[0]);
         GLshort g = FLOAT_TO_SHORT(ctx->Accum.ClearColor[1]);
         GLshort b = FLOAT_TO_SHORT(ctx->Accum.ClearColor[2]);
         GLshort a = FLOAT_TO_SHORT(ctx->Accum.ClearColor[3]);
         int i, j;
         for (i = 0; i < height; i++) {
            GLshort *dst = (GLshort *) (data + (ypos + i) * stride + xpos * 8);
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
}


/** For ADD/MULT */
static void
accum_mad(GLcontext *ctx, GLfloat scale, GLfloat bias,
          GLint xpos, GLint ypos, GLint width, GLint height,
          struct st_renderbuffer *acc_strb)
{
   size_t stride = acc_strb->stride;
   GLubyte *data = acc_strb->data;

   switch (acc_strb->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         int i, j;
         for (i = 0; i < height; i++) {
            GLshort *acc = (GLshort *) (data + (ypos + i) * stride + xpos * 8);
            for (j = 0; j < width * 4; j++) {
               float val = SHORT_TO_FLOAT(*acc) * scale + bias;
               *acc++ = FLOAT_TO_SHORT(val);
            }
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in st_clear_accum_buffer()");
   }
}


static void
accum_accum(struct st_context *st, GLfloat value,
            GLint xpos, GLint ypos, GLint width, GLint height,
            struct st_renderbuffer *acc_strb,
            struct st_renderbuffer *color_strb)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_transfer *color_trans;
   size_t stride = acc_strb->stride;
   GLubyte *data = acc_strb->data;
   GLfloat *buf;

   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   color_trans = st_cond_flush_get_tex_transfer(st, color_strb->texture,
						0, 0, 0,
						PIPE_TRANSFER_READ, xpos, ypos,
						width, height);

   buf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   pipe_get_tile_rgba(color_trans, 0, 0, width, height, buf);

   switch (acc_strb->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         const GLfloat *color = buf;
         int i, j;
         for (i = 0; i < height; i++) {
            GLshort *acc = (GLshort *) (data + (ypos + i) * stride + xpos * 8);
            for (j = 0; j < width * 4; j++) {
               float val = *color++ * value;
               *acc++ += FLOAT_TO_SHORT(val);
            }
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in st_clear_accum_buffer()");
   }

   free(buf);
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
   struct pipe_transfer *color_trans;
   size_t stride = acc_strb->stride;
   GLubyte *data = acc_strb->data;
   GLfloat *buf;


   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   color_trans = st_cond_flush_get_tex_transfer(st, color_strb->texture,
						0, 0, 0,
						PIPE_TRANSFER_READ, xpos, ypos,
						width, height);

   buf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   pipe_get_tile_rgba(color_trans, 0, 0, width, height, buf);

   switch (acc_strb->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         const GLfloat *color = buf;
         int i, j;
         for (i = 0; i < height; i++) {
            GLshort *acc = (GLshort *) (data + (ypos + i) * stride + xpos * 8);
            for (j = 0; j < width * 4; j++) {
               float val = *color++ * value;
               *acc++ = FLOAT_TO_SHORT(val);
            }
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in st_clear_accum_buffer()");
   }

   free(buf);
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
   const GLubyte *colormask = ctx->Color.ColorMask[0];
   enum pipe_transfer_usage usage;
   struct pipe_transfer *color_trans;
   size_t stride = acc_strb->stride;
   const GLubyte *data = acc_strb->data;
   GLfloat *buf;

   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   buf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   if (!colormask[0] || !colormask[1] || !colormask[2] || !colormask[3])
      usage = PIPE_TRANSFER_READ_WRITE;
   else
      usage = PIPE_TRANSFER_WRITE;
   
   color_trans = st_cond_flush_get_tex_transfer(st_context(ctx),
						color_strb->texture, 0, 0, 0,
						usage,
						xpos, ypos,
						width, height);

   if (usage & PIPE_TRANSFER_READ)
      pipe_get_tile_rgba(color_trans, 0, 0, width, height, buf);

   switch (acc_strb->format) {
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      {
         GLfloat *color = buf;
         int i, j, ch;
         for (i = 0; i < height; i++) {
            const GLshort *acc = (const GLshort *) (data + (ypos + i) * stride + xpos * 8);
            for (j = 0; j < width; j++) {
               for (ch = 0; ch < 4; ch++) {
                  if (colormask[ch]) {
                     GLfloat val = SHORT_TO_FLOAT(*acc * value);
                     *color = CLAMP(val, 0.0f, 1.0f);
                  }
                  else {
                     /* No change */
                  }
                  ++acc;
                  ++color;
               }
            }
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in st_clear_accum_buffer()");
   }

   pipe_put_tile_rgba(color_trans, 0, 0, width, height, buf);

   free(buf);
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

   if(!acc_strb->data)
      return;
   
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
