/*
 * Mesa 3-D graphics library
 * Version:  7.0.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/feedback.h"
#include "main/formats.h"
#include "main/format_unpack.h"
#include "main/image.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/pack.h"
#include "main/pbo.h"
#include "main/state.h"

#include "s_context.h"
#include "s_depth.h"
#include "s_span.h"
#include "s_stencil.h"

/**
 * Tries to implement glReadPixels() of GL_DEPTH_COMPONENT using memcpy of the
 * mapping.
 */
static GLboolean
fast_read_depth_pixels( struct gl_context *ctx,
			GLint x, GLint y,
			GLsizei width, GLsizei height,
			GLenum type, GLvoid *pixels,
			const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLubyte *map, *dst;
   int stride, dstStride, j;

   if (ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0)
      return GL_FALSE;

   if (packing->SwapBytes)
      return GL_FALSE;

   if (_mesa_get_format_datatype(rb->Format) != GL_UNSIGNED_INT)
      return GL_FALSE;

   if (!((type == GL_UNSIGNED_SHORT && rb->Format == MESA_FORMAT_Z16) ||
	 type == GL_UNSIGNED_INT))
      return GL_FALSE;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   dstStride = _mesa_image_row_stride(packing, width, GL_DEPTH_COMPONENT, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   GL_DEPTH_COMPONENT, type, 0, 0);

   for (j = 0; j < height; j++) {
      if (type == GL_UNSIGNED_INT) {
	 _mesa_unpack_uint_z_row(rb->Format, width, map, (GLuint *)dst);
      } else {
	 ASSERT(type == GL_UNSIGNED_SHORT && rb->Format == MESA_FORMAT_Z16);
	 memcpy(dst, map, width * 2);
      }

      map += stride;
      dst += dstStride;
   }
   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}

/**
 * Read pixels for format=GL_DEPTH_COMPONENT.
 */
static void
read_depth_pixels( struct gl_context *ctx,
                   GLint x, GLint y,
                   GLsizei width, GLsizei height,
                   GLenum type, GLvoid *pixels,
                   const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLint j;
   GLubyte *dst, *map;
   int dstStride, stride;

   if (!rb)
      return;

   /* clipping should have been done already */
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(x + width <= (GLint) rb->Width);
   ASSERT(y + height <= (GLint) rb->Height);
   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   if (fast_read_depth_pixels(ctx, x, y, width, height, type, pixels, packing))
      return;

   dstStride = _mesa_image_row_stride(packing, width, GL_DEPTH_COMPONENT, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   GL_DEPTH_COMPONENT, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   /* General case (slower) */
   for (j = 0; j < height; j++, y++) {
      GLfloat depthValues[MAX_WIDTH];
      _mesa_unpack_float_z_row(rb->Format, width, map, depthValues);
      _mesa_pack_depth_span(ctx, width, dst, type, depthValues, packing);

      dst += dstStride;
      map += stride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Read pixels for format=GL_STENCIL_INDEX.
 */
static void
read_stencil_pixels( struct gl_context *ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum type, GLvoid *pixels,
                     const struct gl_pixelstore_attrib *packing )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLint j;
   GLubyte *map;
   GLint stride;

   if (!rb)
      return;

   /* width should never be > MAX_WIDTH since we did clipping earlier */
   ASSERT(width <= MAX_WIDTH);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   /* process image row by row */
   for (j = 0; j < height; j++) {
      GLvoid *dest;
      GLstencil stencil[MAX_WIDTH];

      _mesa_unpack_ubyte_stencil_row(rb->Format, width, map, stencil);
      dest = _mesa_image_address2d(packing, pixels, width, height,
                                   GL_STENCIL_INDEX, type, j, 0);

      _mesa_pack_stencil_span(ctx, width, type, dest, stencil, packing);

      map += stride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}

static GLboolean
fast_read_rgba_pixels_memcpy( struct gl_context *ctx,
			      GLint x, GLint y,
			      GLsizei width, GLsizei height,
			      GLenum format, GLenum type,
			      GLvoid *pixels,
			      const struct gl_pixelstore_attrib *packing,
			      GLbitfield transferOps )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   GLubyte *dst, *map;
   int dstStride, stride, j, texelBytes;

   if (!_mesa_format_matches_format_and_type(rb->Format, format, type))
      return GL_FALSE;

   /* check for things we can't handle here */
   if (packing->SwapBytes ||
       packing->LsbFirst) {
      return GL_FALSE;
   }

   dstStride = _mesa_image_row_stride(packing, width, format, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   format, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   texelBytes = _mesa_get_format_bytes(rb->Format);
   for (j = 0; j < height; j++) {
      memcpy(dst, map, width * texelBytes);
      dst += dstStride;
      map += stride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}

static GLboolean
slow_read_rgba_pixels( struct gl_context *ctx,
		       GLint x, GLint y,
		       GLsizei width, GLsizei height,
		       GLenum format, GLenum type,
		       GLvoid *pixels,
		       const struct gl_pixelstore_attrib *packing,
		       GLbitfield transferOps )
{
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
   union {
      float f[MAX_WIDTH][4];
      unsigned int i[MAX_WIDTH][4];
   } rgba;
   GLubyte *dst, *map;
   int dstStride, stride, j;

   dstStride = _mesa_image_row_stride(packing, width, format, type);
   dst = (GLubyte *) _mesa_image_address2d(packing, pixels, width, height,
					   format, type, 0, 0);

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   for (j = 0; j < height; j++) {
      if (_mesa_is_integer_format(format)) {
	 _mesa_unpack_int_rgba_row(rb->Format, width, map, rgba.i);
	 _mesa_pack_rgba_span_int(ctx, width, rgba.i, format, type, dst);
      } else {
	 _mesa_unpack_rgba_row(_mesa_get_srgb_format_linear(rb->Format),
			       width, map, rgba.f);
	 _mesa_pack_rgba_span_float(ctx, width, rgba.f, format, type, dst,
				    packing, transferOps);
      }
      dst += dstStride;
      map += stride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}

/*
 * Read R, G, B, A, RGB, L, or LA pixels.
 */
static void
read_rgba_pixels( struct gl_context *ctx,
                  GLint x, GLint y,
                  GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels,
                  const struct gl_pixelstore_attrib *packing )
{
   GLbitfield transferOps = ctx->_ImageTransferState;
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->_ColorReadBuffer;

   if (!rb)
      return;

   if ((ctx->Color._ClampReadColor == GL_TRUE || type != GL_FLOAT) &&
       !_mesa_is_integer_format(format)) {
      transferOps |= IMAGE_CLAMP_BIT;
   }

   if (!transferOps) {
      /* Try the optimized paths first. */
      if (fast_read_rgba_pixels_memcpy(ctx, x, y, width, height,
				       format, type, pixels, packing,
				       transferOps)) {
	 return;
      }
   }

   slow_read_rgba_pixels(ctx, x, y, width, height,
			 format, type, pixels, packing, transferOps);
}

/**
 * For a packed depth/stencil buffer being read as depth/stencil, just memcpy the
 * data (possibly swapping 8/24 vs 24/8 as we go).
 */
static GLboolean
fast_read_depth_stencil_pixels(struct gl_context *ctx,
			       GLint x, GLint y,
			       GLsizei width, GLsizei height,
			       GLubyte *dst, int dstStride)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLubyte *map;
   int stride, i;

   if (rb != stencilRb)
      return GL_FALSE;

   if (rb->Format != MESA_FORMAT_Z24_S8 &&
       rb->Format != MESA_FORMAT_S8_Z24)
      return GL_FALSE;

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height, GL_MAP_READ_BIT,
			       &map, &stride);

   for (i = 0; i < height; i++) {
      _mesa_unpack_uint_24_8_depth_stencil_row(rb->Format, width,
					       map, (GLuint *)dst);
      map += stride;
      dst += dstStride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);

   return GL_TRUE;
}


/**
 * For non-float-depth and stencil buffers being read as 24/8 depth/stencil,
 * copy the integer data directly instead of converting depth to float and
 * re-packing.
 */
static GLboolean
fast_read_depth_stencil_pixels_separate(struct gl_context *ctx,
					GLint x, GLint y,
					GLsizei width, GLsizei height,
					uint32_t *dst, int dstStride)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *depthRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLubyte *depthMap, *stencilMap;
   int depthStride, stencilStride, i, j;

   if (_mesa_get_format_datatype(depthRb->Format) != GL_UNSIGNED_INT)
      return GL_FALSE;

   ctx->Driver.MapRenderbuffer(ctx, depthRb, x, y, width, height,
			       GL_MAP_READ_BIT, &depthMap, &depthStride);
   ctx->Driver.MapRenderbuffer(ctx, stencilRb, x, y, width, height,
			       GL_MAP_READ_BIT, &stencilMap, &stencilStride);

   for (j = 0; j < height; j++) {
      GLstencil stencilVals[MAX_WIDTH];

      _mesa_unpack_uint_z_row(depthRb->Format, width, depthMap, dst);
      _mesa_unpack_ubyte_stencil_row(stencilRb->Format, width,
				     stencilMap, stencilVals);

      for (i = 0; i < width; i++) {
	 dst[i] = (dst[i] & 0xffffff00) | stencilVals[i];
      }

      depthMap += depthStride;
      stencilMap += stencilStride;
      dst += dstStride / 4;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, depthRb);
   ctx->Driver.UnmapRenderbuffer(ctx, stencilRb);

   return GL_TRUE;
}

static void
slow_read_depth_stencil_pixels_separate(struct gl_context *ctx,
					GLint x, GLint y,
					GLsizei width, GLsizei height,
					GLenum type,
					const struct gl_pixelstore_attrib *packing,
					GLubyte *dst, int dstStride)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *depthRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLubyte *depthMap, *stencilMap;
   int depthStride, stencilStride, j;

   ctx->Driver.MapRenderbuffer(ctx, depthRb, x, y, width, height,
			       GL_MAP_READ_BIT, &depthMap, &depthStride);
   ctx->Driver.MapRenderbuffer(ctx, stencilRb, x, y, width, height,
			       GL_MAP_READ_BIT, &stencilMap, &stencilStride);

   for (j = 0; j < height; j++) {
      GLstencil stencilVals[MAX_WIDTH];
      GLfloat depthVals[MAX_WIDTH];

      _mesa_unpack_float_z_row(depthRb->Format, width, depthMap, depthVals);
      _mesa_unpack_ubyte_stencil_row(stencilRb->Format, width,
				     stencilMap, stencilVals);

      _mesa_pack_depth_stencil_span(ctx, width, type, (GLuint *)dst,
				    depthVals, stencilVals, packing);

      depthMap += depthStride;
      stencilMap += stencilStride;
      dst += dstStride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, depthRb);
   ctx->Driver.UnmapRenderbuffer(ctx, stencilRb);
}


/**
 * Read combined depth/stencil values.
 * We'll have already done error checking to be sure the expected
 * depth and stencil buffers really exist.
 */
static void
read_depth_stencil_pixels(struct gl_context *ctx,
                          GLint x, GLint y,
                          GLsizei width, GLsizei height,
                          GLenum type, GLvoid *pixels,
                          const struct gl_pixelstore_attrib *packing )
{
   const GLboolean scaleOrBias
      = ctx->Pixel.DepthScale != 1.0 || ctx->Pixel.DepthBias != 0.0;
   const GLboolean stencilTransfer = ctx->Pixel.IndexShift
      || ctx->Pixel.IndexOffset || ctx->Pixel.MapStencilFlag;
   GLubyte *dst;
   int dstStride;

   dst = (GLubyte *) _mesa_image_address2d(packing, pixels,
					   width, height,
					   GL_DEPTH_STENCIL_EXT,
					   type, 0, 0);
   dstStride = _mesa_image_row_stride(packing, width,
				      GL_DEPTH_STENCIL_EXT, type);

   /* Fast 24/8 reads. */
   if (type == GL_UNSIGNED_INT_24_8 &&
       !scaleOrBias && !stencilTransfer && !packing->SwapBytes) {
      if (fast_read_depth_stencil_pixels(ctx, x, y, width, height,
					 dst, dstStride))
	 return;

      if (fast_read_depth_stencil_pixels_separate(ctx, x, y, width, height,
						  (uint32_t *)dst, dstStride))
	 return;
   }

   slow_read_depth_stencil_pixels_separate(ctx, x, y, width, height,
					   type, packing,
					   dst, dstStride);
}



/**
 * Software fallback routine for ctx->Driver.ReadPixels().
 * By time we get here, all error checking will have been done.
 */
void
_swrast_ReadPixels( struct gl_context *ctx,
		    GLint x, GLint y, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const struct gl_pixelstore_attrib *packing,
		    GLvoid *pixels )
{
   struct gl_pixelstore_attrib clippedPacking = *packing;

   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Do all needed clipping here, so that we can forget about it later */
   if (_mesa_clip_readpixels(ctx, &x, &y, &width, &height, &clippedPacking)) {

      pixels = _mesa_map_pbo_dest(ctx, &clippedPacking, pixels);

      if (pixels) {
         switch (format) {
         case GL_STENCIL_INDEX:
            read_stencil_pixels(ctx, x, y, width, height, type, pixels,
                                &clippedPacking);
            break;
         case GL_DEPTH_COMPONENT:
            read_depth_pixels(ctx, x, y, width, height, type, pixels,
                              &clippedPacking);
            break;
         case GL_DEPTH_STENCIL_EXT:
            read_depth_stencil_pixels(ctx, x, y, width, height, type, pixels,
                                      &clippedPacking);
            break;
         default:
            /* all other formats should be color formats */
            read_rgba_pixels(ctx, x, y, width, height, format, type, pixels,
                             &clippedPacking);
         }

         _mesa_unmap_pbo_dest(ctx, &clippedPacking);
      }
   }
}
