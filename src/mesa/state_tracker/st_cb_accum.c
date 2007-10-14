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


/**
 * For hardware that supports deep color buffers, we could accelerate
 * most/all the accum operations with blending/texturing.
 * For now, just use the get/put_tile() functions and do things in software.
 */


/** For ADD/MULT */
static void
accum_mad(struct pipe_context *pipe, GLfloat scale, GLfloat bias,
          GLint xpos, GLint ypos, GLint width, GLint height,
          struct pipe_surface *acc_ps)
{
   GLfloat *accBuf;
   GLint i;

   accBuf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   (void) pipe->region_map(pipe, acc_ps->region);

   acc_ps->get_tile(acc_ps, xpos, ypos, width, height, accBuf);

   for (i = 0; i < 4 * width * height; i++) {
      GLfloat val = accBuf[i] * scale + bias;
      accBuf[i] = CLAMP(val, 0.0, 1.0);
   }

   acc_ps->put_tile(acc_ps, xpos, ypos, width, height, accBuf);

   _mesa_free(accBuf);

   pipe->region_unmap(pipe, acc_ps->region);
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

   colorMap = pipe->region_map(pipe, color_ps->region);
   accMap = pipe->region_map(pipe, acc_ps->region);

   color_ps->get_tile(color_ps, xpos, ypos, width, height, colorBuf);
   acc_ps->get_tile(acc_ps, xpos, ypos, width, height, accBuf);

   for (i = 0; i < 4 * width * height; i++) {
      GLfloat val = accBuf[i] + colorBuf[i] * value;
      accBuf[i] = CLAMP(val, 0.0, 1.0);
   }

   acc_ps->put_tile(acc_ps, xpos, ypos, width, height, accBuf);

   _mesa_free(colorBuf);
   _mesa_free(accBuf);

   pipe->region_unmap(pipe, color_ps->region);
   pipe->region_unmap(pipe, acc_ps->region);
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

   (void) pipe->region_map(pipe, color_ps->region);
   (void) pipe->region_map(pipe, acc_ps->region);

   color_ps->get_tile(color_ps, xpos, ypos, width, height, buf);

   for (i = 0; i < 4 * width * height; i++) {
      GLfloat val = buf[i] * value;
      buf[i] = CLAMP(val, 0.0, 1.0);
   }

   acc_ps->put_tile(acc_ps, xpos, ypos, width, height, buf);

   _mesa_free(buf);

   pipe->region_unmap(pipe, color_ps->region);
   pipe->region_unmap(pipe, acc_ps->region);
}


static void
accum_return(GLcontext *ctx, GLfloat value,
             GLint xpos, GLint ypos, GLint width, GLint height,
             struct pipe_surface *acc_ps,
             struct pipe_surface *color_ps)
{
   struct pipe_context *pipe = ctx->st->pipe;
   const GLboolean writeR = ctx->Color.ColorMask[0];
   const GLboolean writeG = ctx->Color.ColorMask[1];
   const GLboolean writeB = ctx->Color.ColorMask[2];
   const GLboolean writeA = ctx->Color.ColorMask[3];
   GLfloat *buf;
   GLint i;

   buf = (GLfloat *) malloc(width * height * 4 * sizeof(GLfloat));

   (void) pipe->region_map(pipe, color_ps->region);
   (void) pipe->region_map(pipe, acc_ps->region);

   acc_ps->get_tile(acc_ps, xpos, ypos, width, height, buf);

   for (i = 0; i < width * height; i++) {
      if (writeR) {
         GLfloat r = buf[i * 4 + 0] * value;
         buf[i * 4 + 0] = CLAMP(r, 0.0, 1.0);
      }
      if (writeG) {
         GLfloat g = buf[i * 4 + 1] * value;
         buf[i * 4 + 1] = CLAMP(g, 0.0, 1.0);
      }
      if (writeB) {
         GLfloat b = buf[i * 4 + 2] * value;
         buf[i * 4 + 2] = CLAMP(b, 0.0, 1.0);
      }
      if (writeA) {
         GLfloat a = buf[i * 4 + 3] * value;
         buf[i * 4 + 3] = CLAMP(a, 0.0, 1.0);
      }
   }

   color_ps->put_tile(color_ps, xpos, ypos, width, height, buf);

   _mesa_free(buf);

   pipe->region_unmap(pipe, color_ps->region);
   pipe->region_unmap(pipe, acc_ps->region);
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
