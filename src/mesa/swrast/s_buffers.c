/* $Id: s_buffers.c,v 1.8 2001/03/19 02:25:36 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
      const GLchan r = ctx->Color.ClearColor[0];
      const GLchan g = ctx->Color.ClearColor[1];
      const GLchan b = ctx->Color.ClearColor[2];
      const GLchan a = ctx->Color.ClearColor[3];
      GLint i;
      for (i = 0; i < height; i++) {
         GLchan rgba[MAX_WIDTH][4];
         GLint j;
         for (j = 0; j < width; j++) {
            rgba[j][RCOMP] = r;
            rgba[j][GCOMP] = g;
            rgba[j][BCOMP] = b;
            rgba[j][ACOMP] = a;
         }
         _mesa_mask_rgba_span( ctx, width, x, y + i, rgba );
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
         _mesa_mask_index_span( ctx, width, x, y + i, span );
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
      const GLchan r = ctx->Color.ClearColor[0];
      const GLchan g = ctx->Color.ClearColor[1];
      const GLchan b = ctx->Color.ClearColor[2];
      const GLchan a = ctx->Color.ClearColor[3];
      GLchan span[MAX_WIDTH][4];
      GLint i;

      ASSERT(*((GLuint *) &ctx->Color.ColorMask) == 0xffffffff);

      for (i = 0; i < width; i++) {
         span[i][RCOMP] = r;
         span[i][GCOMP] = g;
         span[i][BCOMP] = b;
         span[i][ACOMP] = a;
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
      if (bufferBit & ctx->Color.DrawDestMask) {
         if (bufferBit == FRONT_LEFT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_LEFT);
            (void) (*swrast->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer, GL_FRONT_LEFT);
         }
         else if (bufferBit == FRONT_RIGHT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_RIGHT);
            (void) (*swrast->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer, GL_FRONT_RIGHT);
         }
         else if (bufferBit == BACK_LEFT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_LEFT);
            (void) (*swrast->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer, GL_BACK_LEFT);
         }
         else {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_RIGHT);
            (void) (*swrast->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer, GL_BACK_RIGHT);
         }

         if (colorMask != 0xffffffff) {
            clear_color_buffer_with_masking(ctx);
         }
         else {
            clear_color_buffer(ctx);
         }
      }
   }

   /* restore default read/draw buffers */
   (void) (*ctx->Driver.SetDrawBuffer)( ctx, ctx->Color.DriverDrawBuffer );
   (void) (*swrast->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer, ctx->Pixel.DriverReadBuffer );
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
      if (mask & ctx->Color.DrawDestMask)   clear_color_buffers(ctx);
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
_swrast_alloc_buffers( GLcontext *ctx )
{
   /* Reallocate other buffers if needed. */
   if (ctx->DrawBuffer->UseSoftwareDepthBuffer) {
      _mesa_alloc_depth_buffer( ctx );
   }
   if (ctx->DrawBuffer->UseSoftwareStencilBuffer) {
      _mesa_alloc_stencil_buffer( ctx );
   }
   if (ctx->DrawBuffer->UseSoftwareAccumBuffer) {
      _mesa_alloc_accum_buffer( ctx );
   }
   if (ctx->DrawBuffer->UseSoftwareAlphaBuffers) {
      _mesa_alloc_alpha_buffers( ctx );
   }
}
