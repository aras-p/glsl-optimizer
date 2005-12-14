/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
#include "context.h"
#include "colormac.h"
#include "convolve.h"
#include "histogram.h"
#include "image.h"
#include "macros.h"
#include "imports.h"
#include "pixel.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"



/*
 * Determine if there's overlap in an image copy.
 * This test also compensates for the fact that copies are done from
 * bottom to top and overlaps can sometimes be handled correctly
 * without making a temporary image copy.
 */
static GLboolean
regions_overlap(GLint srcx, GLint srcy,
                GLint dstx, GLint dsty,
                GLint width, GLint height,
                GLfloat zoomX, GLfloat zoomY)
{
   if (zoomX == 1.0 && zoomY == 1.0) {
      /* no zoom */
      if (srcx >= dstx + width || (srcx + width <= dstx)) {
         return GL_FALSE;
      }
      else if (srcy < dsty) { /* this is OK */
         return GL_FALSE;
      }
      else if (srcy > dsty + height) {
         return GL_FALSE;
      }
      else {
         return GL_TRUE;
      }
   }
   else {
      /* add one pixel of slop when zooming, just to be safe */
      if ((srcx > dstx + (width * zoomX) + 1) || (srcx + width + 1 < dstx)) {
         return GL_FALSE;
      }
      else if ((srcy < dsty) && (srcy + height < dsty + (height * zoomY))) {
         return GL_FALSE;
      }
      else if ((srcy > dsty) && (srcy + height > dsty + (height * zoomY))) {
         return GL_FALSE;
      }
      else {
         return GL_TRUE;
      }
   }
}


/**
 * Convert GLfloat[n][4] colors to GLchan[n][4].
 * XXX maybe move into image.c
 */
static void
float_span_to_chan(GLuint n, CONST GLfloat in[][4], GLchan out[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      UNCLAMPED_FLOAT_TO_CHAN(out[i][RCOMP], in[i][RCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(out[i][GCOMP], in[i][GCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(out[i][BCOMP], in[i][BCOMP]);
      UNCLAMPED_FLOAT_TO_CHAN(out[i][ACOMP], in[i][ACOMP]);
   }
}


/**
 * Convert GLchan[n][4] colors to GLfloat[n][4].
 * XXX maybe move into image.c
 */
static void
chan_span_to_float(GLuint n, CONST GLchan in[][4], GLfloat out[][4])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      out[i][RCOMP] = CHAN_TO_FLOAT(in[i][RCOMP]);
      out[i][GCOMP] = CHAN_TO_FLOAT(in[i][GCOMP]);
      out[i][BCOMP] = CHAN_TO_FLOAT(in[i][BCOMP]);
      out[i][ACOMP] = CHAN_TO_FLOAT(in[i][ACOMP]);
   }
}



/*
 * RGBA copypixels with convolution.
 */
static void
copy_conv_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                      GLint width, GLint height, GLint destx, GLint desty)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *drawRb = NULL;
   GLboolean quick_draw;
   GLint row;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLuint transferOps = ctx->_ImageTransferState;
   GLfloat *dest, *tmpImage, *convImage;
   struct sw_span span;

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);


   if (SWRAST_CONTEXT(ctx)->_RasterMask == 0
       && !zoom
       && destx >= 0
       && destx + width <= (GLint) ctx->DrawBuffer->Width) {
      quick_draw = GL_TRUE;
      drawRb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   }
   else {
      quick_draw = GL_FALSE;
   }

   /* allocate space for GLfloat image */
   tmpImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
   if (!tmpImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }
   convImage = (GLfloat *) _mesa_malloc(width * height * 4 * sizeof(GLfloat));
   if (!convImage) {
      _mesa_free(tmpImage);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
      return;
   }

   /* read source image */
   dest = tmpImage;
   for (row = 0; row < height; row++) {
      GLchan rgba[MAX_WIDTH][4];
      /* Read GLchan and convert to GLfloat */
      _swrast_read_rgba_span(ctx, ctx->ReadBuffer->_ColorReadBuffer,
                             width, srcx, srcy + row, rgba);
      chan_span_to_float(width, (CONST GLchan (*)[4]) rgba,
                         (GLfloat (*)[4]) dest);
      dest += 4 * width;
   }

   /* do the image transfer ops which preceed convolution */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) (tmpImage + row * width * 4);
      _mesa_apply_rgba_transfer_ops(ctx,
                                    transferOps & IMAGE_PRE_CONVOLUTION_BITS,
                                    width, rgba);
   }

   /* do convolution */
   if (ctx->Pixel.Convolution2DEnabled) {
      _mesa_convolve_2d_image(ctx, &width, &height, tmpImage, convImage);
   }
   else {
      ASSERT(ctx->Pixel.Separable2DEnabled);
      _mesa_convolve_sep_image(ctx, &width, &height, tmpImage, convImage);
   }
   _mesa_free(tmpImage);

   /* do remaining post-convolution image transfer ops */
   for (row = 0; row < height; row++) {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) (convImage + row * width * 4);
      _mesa_apply_rgba_transfer_ops(ctx,
                                    transferOps & IMAGE_POST_CONVOLUTION_BITS,
                                    width, rgba);
   }

   /* write the new image */
   for (row = 0; row < height; row++) {
      const GLfloat *src = convImage + row * width * 4;
      GLint dy;

      /* convert floats back to chan */
      float_span_to_chan(width, (const GLfloat (*)[4]) src, span.array->rgba);

      /* write row to framebuffer */
      dy = desty + row;
      if (quick_draw && dy >= 0 && dy < (GLint) ctx->DrawBuffer->Height) {
         drawRb->PutRow(ctx, drawRb, width, destx, dy, span.array->rgba, NULL);
      }
      else {
         span.x = destx;
         span.y = dy;
         span.end = width;
         if (zoom) {
            _swrast_write_zoomed_rgba_span(ctx, destx, desty, &span, 
                                        (CONST GLchan (*)[4])span.array->rgba);
         }
         else {
            _swrast_write_rgba_span(ctx, &span);
         }
      }
   }

   _mesa_free(convImage);
}


/*
 * RGBA copypixels
 */
static void
copy_rgba_pixels(GLcontext *ctx, GLint srcx, GLint srcy,
                 GLint width, GLint height, GLint destx, GLint desty)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *drawRb;
   GLchan *tmpImage,*p;
   GLboolean quick_draw;
   GLint sy, dy, stepy, j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   const GLuint transferOps = ctx->_ImageTransferState;
   struct sw_span span;

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no readbuffer - OK */
      return;
   }

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_RGBA);

   if (ctx->Pixel.Convolution2DEnabled || ctx->Pixel.Separable2DEnabled) {
      copy_conv_rgba_pixels(ctx, srcx, srcy, width, height, destx, desty);
      return;
   }

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);

   if (SWRAST_CONTEXT(ctx)->_RasterMask == 0
       && !zoom
       && destx >= 0
       && destx + width <= (GLint) ctx->DrawBuffer->Width) {
      quick_draw = GL_TRUE;
      drawRb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];
   }
   else {
      quick_draw = GL_FALSE;
      drawRb = NULL;
   }

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLchan *) _mesa_malloc(width * height * sizeof(GLchan) * 4);
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      /* read the source image */
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, ssy, (GLchan (*)[4]) p );
         p += width * 4;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warnings */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      /* Get source pixels */
      if (overlapping) {
         /* get from buffered image */
         ASSERT(width < MAX_WIDTH);
         _mesa_memcpy(span.array->rgba, p, width * sizeof(GLchan) * 4);
         p += width * 4;
      }
      else {
         /* get from framebuffer */
         ASSERT(width < MAX_WIDTH);
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, sy, span.array->rgba );
      }

      if (transferOps) {
         GLfloat rgbaFloat[MAX_WIDTH][4];
         /* convert to float, transfer, convert back to chan */
         chan_span_to_float(width, (CONST GLchan (*)[4]) span.array->rgba,
                            rgbaFloat);
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, width, rgbaFloat);
         float_span_to_chan(width, (CONST GLfloat (*)[4]) rgbaFloat,
                            span.array->rgba);
      }

      /* Write color span */
      if (quick_draw && dy >= 0 && dy < (GLint) ctx->DrawBuffer->Height) {
         drawRb->PutRow(ctx, drawRb, width, destx, dy, span.array->rgba, NULL);
      }
      else {
         span.x = destx;
         span.y = dy;
         span.end = width;
         if (zoom) {
            _swrast_write_zoomed_rgba_span(ctx, destx, desty, &span,
                                       (CONST GLchan (*)[4]) span.array->rgba);
         }
         else {
            _swrast_write_rgba_span(ctx, &span);
         }
      }
   }

   if (overlapping)
      _mesa_free(tmpImage);
}


static void
copy_ci_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                GLint width, GLint height,
                GLint destx, GLint desty )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint *tmpImage,*p;
   GLint sy, dy, stepy;
   GLint j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;
   struct sw_span span;

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no readbuffer - OK */
      return;
   }

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_INDEX);

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   if (ctx->Depth.Test)
      _swrast_span_default_z(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLuint *) _mesa_malloc(width * height * sizeof(GLuint));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      /* read the image */
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_index_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                  width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      /* Get color indexes */
      if (overlapping) {
         _mesa_memcpy(span.array->index, p, width * sizeof(GLuint));
         p += width;
      }
      else {
         _swrast_read_index_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                  width, srcx, sy, span.array->index );
      }

      /* Apply shift, offset, look-up table */
      if (shift_or_offset) {
         _mesa_shift_and_offset_ci( ctx, width, span.array->index );
      }
      if (ctx->Pixel.MapColorFlag) {
         _mesa_map_ci( ctx, width, span.array->index );
      }

      /* write color indexes */
      span.x = destx;
      span.y = dy;
      span.end = width;
      if (zoom)
         _swrast_write_zoomed_index_span(ctx, destx, desty, &span);
      else
         _swrast_write_index_span(ctx, &span);
   }

   if (overlapping)
      _mesa_free(tmpImage);
}



/*
 * TODO: Optimize!!!!
 */
static void
copy_depth_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                   GLint width, GLint height,
                   GLint destx, GLint desty )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *readRb = fb->_DepthBuffer;
   const GLfloat depthMax = fb->_DepthMaxF;
   GLfloat *p, *tmpImage;
   GLint sy, dy, stepy;
   GLint i, j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   struct sw_span span;

   if (!readRb) {
      /* no readbuffer - OK */
      return;
   }

   INIT_SPAN(span, GL_BITMAP, 0, 0, SPAN_Z);

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy<desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   _swrast_span_default_color(ctx, &span);
   if (swrast->_FogEnabled)
      _swrast_span_default_fog(ctx, &span);

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLfloat *) _mesa_malloc(width * height * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_depth_span_float(ctx, readRb, width, srcx, ssy, p);
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLfloat depth[MAX_WIDTH];
      /* get depth values */
      if (overlapping) {
         _mesa_memcpy(depth, p, width * sizeof(GLfloat));
         p += width;
      }
      else {
         _swrast_read_depth_span_float(ctx, readRb, width, srcx, sy, depth);
      }

      /* apply scale and bias */
      for (i = 0; i < width; i++) {
         GLfloat d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         span.array->z[i] = (GLuint) (CLAMP(d, 0.0F, 1.0F) * depthMax);
      }

      /* write depth values */
      span.x = destx;
      span.y = dy;
      span.end = width;
      if (fb->Visual.rgbMode) {
         if (zoom)
            _swrast_write_zoomed_rgba_span(ctx, destx, desty, &span, 
                                       (const GLchan (*)[4]) span.array->rgba);
         else
            _swrast_write_rgba_span(ctx, &span);
      }
      else {
         if (zoom)
            _swrast_write_zoomed_index_span(ctx, destx, desty, &span);
         else
            _swrast_write_index_span(ctx, &span);
      }
   }

   if (overlapping)
      _mesa_free(tmpImage);
}



static void
copy_stencil_pixels( GLcontext *ctx, GLint srcx, GLint srcy,
                     GLint width, GLint height,
                     GLint destx, GLint desty )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_StencilBuffer;
   GLint sy, dy, stepy;
   GLint j;
   GLstencil *p, *tmpImage;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shift_or_offset = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   GLint overlapping;

   if (!rb) {
      /* no readbuffer - OK */
      return;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLstencil *) _mesa_malloc(width * height * sizeof(GLstencil));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_stencil_span( ctx, rb, width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLstencil stencil[MAX_WIDTH];

      /* Get stencil values */
      if (overlapping) {
         _mesa_memcpy(stencil, p, width * sizeof(GLstencil));
         p += width;
      }
      else {
         _swrast_read_stencil_span( ctx, rb, width, srcx, sy, stencil );
      }

      /* Apply shift, offset, look-up table */
      if (shift_or_offset) {
         _mesa_shift_and_offset_stencil( ctx, width, stencil );
      }
      if (ctx->Pixel.MapStencilFlag) {
         _mesa_map_stencil( ctx, width, stencil );
      }

      /* Write stencil values */
      if (zoom) {
         _swrast_write_zoomed_stencil_span(ctx, destx, desty, width,
                                           destx, dy, stencil);
      }
      else {
         _swrast_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }

   if (overlapping)
      _mesa_free(tmpImage);
}


/**
 * This isn't terribly efficient.  If a driver really has combined
 * depth/stencil buffers the driver should implement an optimized
 * CopyPixels function.
 */
static void
copy_depth_stencil_pixels(GLcontext *ctx,
                          const GLint srcX, const GLint srcY,
                          const GLint width, const GLint height,
                          const GLint destX, const GLint destY)
{
   struct gl_renderbuffer *stencilReadRb, *depthReadRb, *depthDrawRb;
   GLint sy, dy, stepy;
   GLint j;
   GLstencil *tempStencilImage = NULL, *stencilPtr = NULL;
   GLfloat *tempDepthImage = NULL, *depthPtr = NULL;
   const GLfloat depthScale = ctx->DrawBuffer->_DepthMaxF;
   const GLuint stencilMask = ctx->Stencil.WriteMask[0];
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   const GLboolean shiftOrOffset
      = ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset;
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   GLint overlapping;

   depthDrawRb = ctx->DrawBuffer->_DepthBuffer;
   depthReadRb = ctx->ReadBuffer->_DepthBuffer;
   stencilReadRb = ctx->ReadBuffer->_StencilBuffer;

   ASSERT(depthDrawRb);
   ASSERT(depthReadRb);
   ASSERT(stencilReadRb);

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (srcY < destY) {
      /* top-down  max-to-min */
      sy = srcY + height - 1;
      dy = destY + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcY;
      dy = destY;
      stepy = 1;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcX, srcY, destX, destY, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   if (overlapping) {
      GLint ssy = sy;

      if (stencilMask != 0x0) {
         tempStencilImage
            = (GLstencil *) _mesa_malloc(width * height * sizeof(GLstencil));
         if (!tempStencilImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
            return;
         }

         /* get copy of stencil pixels */
         stencilPtr = tempStencilImage;
         for (j = 0; j < height; j++, ssy += stepy) {
            _swrast_read_stencil_span(ctx, stencilReadRb,
                                      width, srcX, ssy, stencilPtr);
            stencilPtr += width;
         }
         stencilPtr = tempStencilImage;
      }

      if (ctx->Depth.Mask) {
         tempDepthImage
            = (GLfloat *) _mesa_malloc(width * height * sizeof(GLfloat));
         if (!tempDepthImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
            _mesa_free(tempStencilImage);
            return;
         }

         /* get copy of depth pixels */
         depthPtr = tempDepthImage;
         for (j = 0; j < height; j++, ssy += stepy) {
            _swrast_read_depth_span_float(ctx, depthReadRb,
                                          width, srcX, ssy, depthPtr);
            depthPtr += width;
         }
         depthPtr = tempDepthImage;
      }
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      if (stencilMask != 0x0) {
         GLstencil stencil[MAX_WIDTH];

         /* Get stencil values */
         if (overlapping) {
            _mesa_memcpy(stencil, stencilPtr, width * sizeof(GLstencil));
            stencilPtr += width;
         }
         else {
            _swrast_read_stencil_span(ctx, stencilReadRb,
                                      width, srcX, sy, stencil);
         }

         /* Apply shift, offset, look-up table */
         if (shiftOrOffset) {
            _mesa_shift_and_offset_stencil(ctx, width, stencil);
         }
         if (ctx->Pixel.MapStencilFlag) {
            _mesa_map_stencil(ctx, width, stencil);
         }

         /* Write values */
         if (zoom) {
            _swrast_write_zoomed_stencil_span(ctx, destX, destY, width,
                                              destX, dy, stencil);
         }
         else {
            _swrast_write_stencil_span( ctx, width, destX, dy, stencil );
         }
      }

      if (ctx->Depth.Mask) {
         GLfloat depth[MAX_WIDTH];
         GLuint zVals32[MAX_WIDTH];
         GLushort zVals16[MAX_WIDTH];
         GLvoid *zVals;
         GLuint zBytes;

         /* get depth values */
         if (overlapping) {
            _mesa_memcpy(depth, depthPtr, width * sizeof(GLfloat));
            depthPtr += width;
         }
         else {
            _swrast_read_depth_span_float(ctx, depthReadRb,
                                          width, srcX, sy, depth);
         }

         /* scale & bias */
         if (scaleOrBias) {
            _mesa_scale_and_bias_depth(ctx, width, depth);
         }
         /* convert to integer Z values */
         if (depthDrawRb->DataType == GL_UNSIGNED_SHORT) {
            GLint k;
            for (k = 0; k < width; k++)
               zVals16[k] = (GLushort) (depth[k] * depthScale);
            zVals = zVals16;
            zBytes = 2;
         }
         else {
            GLint k;
            for (k = 0; k < width; k++)
               zVals32[k] = (GLuint) (depth[k] * depthScale);
            zVals = zVals32;
            zBytes = 4;
         }

         /* Write values */
         if (zoom) {
            _swrast_write_zoomed_z_span(ctx, destX, destY, width,
                                        destX, dy, zVals);
         }
         else {
            _swrast_put_row(ctx, depthDrawRb, width, destX, dy, zVals, zBytes);
         }
      }
   }

   if (tempStencilImage)
      _mesa_free(tempStencilImage);

   if (tempDepthImage)
      _mesa_free(tempDepthImage);
}


/**
 * Do software-based glCopyPixels.
 * By time we get here, all parameters will have been error-checked.
 */
void
_swrast_CopyPixels( GLcontext *ctx,
		    GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		    GLint destx, GLint desty, GLenum type )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   RENDER_START(swrast,ctx);
      
   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   switch (type) {
   case GL_COLOR:
      if (ctx->Visual.rgbMode) {
         copy_rgba_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      else {
         copy_ci_pixels( ctx, srcx, srcy, width, height, destx, desty );
      }
      break;
   case GL_DEPTH:
      copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
      break;
   case GL_STENCIL:
      copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
      break;
   case GL_DEPTH_STENCIL_EXT:
      copy_depth_stencil_pixels(ctx, srcx, srcy, width, height, destx, desty);
      break;
   default:
      _mesa_problem(ctx, "unexpected type in _swrast_CopyPixels");
   }

   RENDER_FINISH(swrast,ctx);
}
