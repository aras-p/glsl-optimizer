/* $Id: s_imaging.c,v 1.4 2001/03/19 02:28:59 keithw Exp $ */

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

/* KW:  Moved these here to remove knowledge of swrast from core mesa.
 * Should probably pull the entire software implementation of these
 * extensions into either swrast or a sister module.  
 */

#include "s_context.h"
#include "s_span.h"

void
_swrast_CopyColorTable( GLcontext *ctx, 
			GLenum target, GLenum internalformat,
			GLint x, GLint y, GLsizei width)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan data[MAX_WIDTH][4];

   /* Select buffer to read from */
   (*swrast->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                 ctx->Pixel.DriverReadBuffer );

   if (width > MAX_WIDTH)
      width = MAX_WIDTH;

   /* read the data from framebuffer */
   _mesa_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y, data );

   /* Restore reading from draw buffer (the default) */
   (*swrast->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                 ctx->Color.DriverDrawBuffer );

   glColorTable(target, internalformat, width, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void
_swrast_CopyColorSubTable( GLcontext *ctx,GLenum target, GLsizei start,
			   GLint x, GLint y, GLsizei width)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan data[MAX_WIDTH][4];

   /* Select buffer to read from */
   (*swrast->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                 ctx->Pixel.DriverReadBuffer );

   if (width > MAX_WIDTH)
      width = MAX_WIDTH;

   /* read the data from framebuffer */
   _mesa_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y, data );

   /* Restore reading from draw buffer (the default) */
   (*swrast->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                 ctx->Color.DriverDrawBuffer );

   glColorSubTable(target, start, width, GL_RGBA, GL_UNSIGNED_BYTE, data);
}


void
_swrast_CopyConvolutionFilter1D(GLcontext *ctx, GLenum target, 
				GLenum internalFormat, 
				GLint x, GLint y, GLsizei width)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLchan rgba[MAX_CONVOLUTION_WIDTH][4];

   RENDER_START( swrast, ctx );

   /* read the data from framebuffer */
   _mesa_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y,
				(GLchan (*)[4]) rgba );
   
   RENDER_FINISH( swrast, ctx );

   /* store as convolution filter */
   glConvolutionFilter1D(target, internalFormat, width,
			 GL_RGBA, CHAN_TYPE, rgba);
}


void
_swrast_CopyConvolutionFilter2D(GLcontext *ctx, GLenum target, 
				GLenum internalFormat, 
				GLint x, GLint y, GLsizei width, GLsizei height)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_pixelstore_attrib packSave;
   GLchan rgba[MAX_CONVOLUTION_HEIGHT][MAX_CONVOLUTION_WIDTH][4];
   GLint i;

   RENDER_START(swrast,ctx);
   
   /* read pixels from framebuffer */
   for (i = 0; i < height; i++) {
      _mesa_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y + i,
				(GLchan (*)[4]) rgba[i] );
   }

   RENDER_FINISH(swrast,ctx);

   /*
    * HACK: save & restore context state so we can store this as a
    * convolution filter via the GL api.  Doesn't call any callbacks
    * hanging off ctx->Unpack statechanges.
    */

   packSave = ctx->Unpack;  /* save pixel packing params */

   ctx->Unpack.Alignment = 1;
   ctx->Unpack.RowLength = MAX_CONVOLUTION_WIDTH;
   ctx->Unpack.SkipPixels = 0;
   ctx->Unpack.SkipRows = 0;
   ctx->Unpack.ImageHeight = 0;
   ctx->Unpack.SkipImages = 0;
   ctx->Unpack.SwapBytes = GL_FALSE;
   ctx->Unpack.LsbFirst = GL_FALSE;
   ctx->NewState |= _NEW_PACKUNPACK;

   glConvolutionFilter2D(target, internalFormat, width, height,
			 GL_RGBA, CHAN_TYPE, rgba);

   ctx->Unpack = packSave;  /* restore pixel packing params */
   ctx->NewState |= _NEW_PACKUNPACK; 
}
