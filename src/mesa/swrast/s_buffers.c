/* $Id: s_buffers.c,v 1.13 2002/10/04 19:10:12 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "colormac.h"
#include "macros.h"
#include "mem.h"

#include "s_accum.h"
#include "s_alphabuf.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_masking.h"
#include "s_stencil.h"




/*
 * Clear the color buffer when glColorMask or glIndexMask is in effect.
 */
static void
clear_color_buffer_with_masking( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLint x = ctx->DrawBuffer->_Xmin;
   const GLint y = ctx->DrawBuffer->_Ymin;
   const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
   const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;

   if (ctx->Visual.rgbMode) {
      /* RGBA mode */
      GLchan clearColor[4];
      GLint i;
      CLAMPED_FLOAT_TO_CHAN(clearColor[RCOMP], ctx->Color.ClearColor[0]);
      CLAMPED_FLOAT_TO_CHAN(clearColor[GCOMP], ctx->Color.ClearColor[1]);
      CLAMPED_FLOAT_TO_CHAN(clearColor[BCOMP], ctx->Color.ClearColor[2]);
      CLAMPED_FLOAT_TO_CHAN(clearColor[ACOMP], ctx->Color.ClearColor[3]);
      for (i = 0; i < height; i++) {
         GLchan rgba[MAX_WIDTH][4];
         GLint j;
         for (j = 0; j < width; j++) {
            COPY_CHAN4(rgba[j], clearColor);
         }
         _mesa_mask_rgba_array( ctx, width, x, y + i, rgba );
         (*swrast->Driver.WriteRGBASpan)( ctx, width, x, y + i,
				       (CONST GLchan (*)[4]) rgba, NULL );
      }
   }
   else {
      /* Color index mode */
      GLuint span[MAX_WIDTH];
      GLubyte mask[MAX_WIDTH];
      GLint i, j;
      MEMSET( mask, 1, width );
      for (i=0;i<height;i++) {
         for (j=0;j<width;j++) {
            span[j] = ctx->Color.ClearIndex;
         }
         _mesa_mask_index_array( ctx, width, x, y + i, span );
         (*swrast->Driver.WriteCI32Span)( ctx, width, x, y + i, span, mask );
      }
   }
}



/*
 * Clear a color buffer without index/channel masking.
 */
static void
clear_color_buffer(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLint x = ctx->DrawBuffer->_Xmin;
   const GLint y = ctx->DrawBuffer->_Ymin;
   const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
   const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;

   if (ctx->Visual.rgbMode) {
      /* RGBA mode */
      GLchan clearColor[4];
      GLchan span[MAX_WIDTH][4];
      GLint i;

      CLAMPED_FLOAT_TO_CHAN(clearColor[RCOMP], ctx->Color.ClearColor[0]);
      CLAMPED_FLOAT_TO_CHAN(clearColor[GCOMP], ctx->Color.ClearColor[1]);
      CLAMPED_FLOAT_TO_CHAN(clearColor[BCOMP], ctx->Color.ClearColor[2]);
      CLAMPED_FLOAT_TO_CHAN(clearColor[ACOMP], ctx->Color.ClearColor[3]);

      ASSERT(*((GLuint *) &ctx->Color.ColorMask) == 0xffffffff);

      for (i = 0; i < width; i++) {
         COPY_CHAN4(span[i], clearColor);
      }
      for (i = 0; i < height; i++) {
         (*swrast->Driver.WriteRGBASpan)( ctx, width, x, y + i,
                                       (CONST GLchan (*)[4]) span, NULL );
      }
   }
   else {
      /* Color index mode */
      ASSERT((ctx->Color.IndexMask & ((1 << ctx->Visual.indexBits) - 1))
             == (GLuint) ((1 << ctx->Visual.indexBits) - 1));
      if (ctx->Visual.indexBits == 8) {
         /* 8-bit clear */
         GLubyte span[MAX_WIDTH];
         GLint i;
         MEMSET(span, ctx->Color.ClearIndex, width);
         for (i = 0; i < height; i++) {
            (*swrast->Driver.WriteCI8Span)( ctx, width, x, y + i, span, NULL );
         }
      }
      else {
         /* non 8-bit clear */
         GLuint span[MAX_WIDTH];
         GLint i;
         for (i = 0; i < width; i++) {
            span[i] = ctx->Color.ClearIndex;
         }
         for (i = 0; i < height; i++) {
            (*swrast->Driver.WriteCI32Span)( ctx, width, x, y + i, span, NULL );
         }
      }
   }
}



/*
 * Clear the front/back/left/right color buffers.
 * This function is usually only called if we need to clear the
 * buffers with masking.
 */
static void
clear_color_buffers(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
   GLuint bufferBit;

   /* loop over four possible dest color buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit = bufferBit << 1) {
      if (bufferBit & ctx->Color._DrawDestMask) {
         if (bufferBit == FRONT_LEFT_BIT) {
            (*swrast->Driver.SetBuffer)(ctx, ctx->DrawBuffer, GL_FRONT_LEFT);
         }
         else if (bufferBit == FRONT_RIGHT_BIT) {
            (*swrast->Driver.SetBuffer)(ctx, ctx->DrawBuffer, GL_FRONT_RIGHT);
         }
         else if (bufferBit == BACK_LEFT_BIT) {
            (*swrast->Driver.SetBuffer)(ctx, ctx->DrawBuffer, GL_BACK_LEFT);
         }
         else {
            (*swrast->Driver.SetBuffer)(ctx, ctx->DrawBuffer, GL_BACK_RIGHT);
         }

         if (colorMask != 0xffffffff) {
            clear_color_buffer_with_masking(ctx);
         }
         else {
            clear_color_buffer(ctx);
         }
      }
   }

   /* restore default read/draw buffer */
   _swrast_use_draw_buffer(ctx);
}



void
_swrast_Clear( GLcontext *ctx, GLbitfield mask,
	       GLboolean all,
	       GLint x, GLint y, GLint width, GLint height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
#ifdef DEBUG
   {
      GLbitfield legalBits = DD_FRONT_LEFT_BIT |
	 DD_FRONT_RIGHT_BIT |
	 DD_BACK_LEFT_BIT |
	 DD_BACK_RIGHT_BIT |
	 DD_DEPTH_BIT |
	 DD_STENCIL_BIT |
	 DD_ACCUM_BIT;
      assert((mask & (~legalBits)) == 0);
   }
#endif

   RENDER_START(swrast,ctx);

   /* do software clearing here */
   if (mask) {
      if (mask & ctx->Color._DrawDestMask)   clear_color_buffers(ctx);
      if (mask & GL_DEPTH_BUFFER_BIT)    _mesa_clear_depth_buffer(ctx);
      if (mask & GL_ACCUM_BUFFER_BIT)    _mesa_clear_accum_buffer(ctx);
      if (mask & GL_STENCIL_BUFFER_BIT)  _mesa_clear_stencil_buffer(ctx);
   }

   /* clear software-based alpha buffer(s) */
   if ( (mask & GL_COLOR_BUFFER_BIT)
	&& ctx->DrawBuffer->UseSoftwareAlphaBuffers
	&& ctx->Color.ColorMask[ACOMP]) {
      _mesa_clear_alpha_buffers( ctx );
   }

   RENDER_FINISH(swrast,ctx);
}


void
_swrast_alloc_buffers( GLframebuffer *buffer )
{
   /* Reallocate other buffers if needed. */
   if (buffer->UseSoftwareDepthBuffer) {
      _mesa_alloc_depth_buffer( buffer );
   }
   if (buffer->UseSoftwareStencilBuffer) {
      _mesa_alloc_stencil_buffer( buffer );
   }
   if (buffer->UseSoftwareAccumBuffer) {
      _mesa_alloc_accum_buffer( buffer );
   }
   if (buffer->UseSoftwareAlphaBuffers) {
      _mesa_alloc_alpha_buffers( buffer );
   }
}


/*
 * Fallback for ctx->Driver.DrawBuffer()
 */
void
_swrast_DrawBuffer( GLcontext *ctx, GLenum mode )
{
   _swrast_use_draw_buffer(ctx);
}


/*
 * Setup things so that we read/write spans from the user-designated
 * read buffer (set via glReadPixels).  We usually just have to call
 * this for glReadPixels, glCopyPixels, etc.
 */
void
_swrast_use_read_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* Do this so the software-emulated alpha plane span functions work! */
   ctx->Color._DriverDrawBuffer = ctx->Pixel._DriverReadBuffer;
   /* Tell the device driver where to read/write spans */
   (*swrast->Driver.SetBuffer)( ctx, ctx->ReadBuffer,
                                ctx->Pixel._DriverReadBuffer );
}


/*
 * Setup things so that we read/write spans from the default draw buffer.
 * This is the usual mode that Mesa's software rasterizer operates in.
 */
void
_swrast_use_draw_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* The user can specify rendering to zero, one, two, or four color
    * buffers simultaneously with glDrawBuffer()!
    * We don't expect the span/point/line/triangle functions to deal with
    * that mess so we'll iterate over the multiple buffers as needed.
    * But usually we only render to one color buffer at a time.
    * We set ctx->Color._DriverDrawBuffer to that buffer and tell the
    * device driver to use that buffer.
    * Look in s_span.c's multi_write_rgba_span() function to see how
    * we loop over multiple color buffers when needed.
    */

   if (ctx->Color._DrawDestMask & FRONT_LEFT_BIT)
      ctx->Color._DriverDrawBuffer = GL_FRONT_LEFT;
   else if (ctx->Color._DrawDestMask & BACK_LEFT_BIT)
      ctx->Color._DriverDrawBuffer = GL_BACK_LEFT;
   else if (ctx->Color._DrawDestMask & FRONT_RIGHT_BIT)
      ctx->Color._DriverDrawBuffer = GL_FRONT_RIGHT;
   else if (ctx->Color._DrawDestMask & BACK_RIGHT_BIT)
      ctx->Color._DriverDrawBuffer = GL_BACK_RIGHT;
   else
      /* glDrawBuffer(GL_NONE) */
      ctx->Color._DriverDrawBuffer = GL_FRONT_LEFT; /* always have this */

   (*swrast->Driver.SetBuffer)( ctx, ctx->DrawBuffer,
                                ctx->Color._DriverDrawBuffer );
}
