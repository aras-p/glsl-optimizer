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

#include "main/glheader.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/colormac.h"

#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_screen.h"
#include "intel_span.h"
#include "intel_regions.h"
#include "intel_tex.h"

#include "swrast/swrast.h"

static void
intel_set_span_functions(struct intel_context *intel,
			 struct gl_renderbuffer *rb);

#define SPAN_CACHE_SIZE		4096

static void
get_span_cache(struct intel_renderbuffer *irb, uint32_t offset)
{
   if (irb->span_cache == NULL) {
      irb->span_cache = _mesa_malloc(SPAN_CACHE_SIZE);
      irb->span_cache_offset = -1;
   }

   if ((offset & ~(SPAN_CACHE_SIZE - 1)) != irb->span_cache_offset) {
      irb->span_cache_offset = offset & ~(SPAN_CACHE_SIZE - 1);
      dri_bo_get_subdata(irb->region->buffer, irb->span_cache_offset,
			 SPAN_CACHE_SIZE, irb->span_cache);
   }
}

static void
clear_span_cache(struct intel_renderbuffer *irb)
{
   irb->span_cache_offset = -1;
}

static uint32_t
pread_32(struct intel_renderbuffer *irb, uint32_t offset)
{
   get_span_cache(irb, offset);

   return *(uint32_t *)(irb->span_cache + (offset & (SPAN_CACHE_SIZE - 1)));
}

static uint32_t
pread_xrgb8888(struct intel_renderbuffer *irb, uint32_t offset)
{
   get_span_cache(irb, offset);

   return *(uint32_t *)(irb->span_cache + (offset & (SPAN_CACHE_SIZE - 1))) |
      0xff000000;
}

static uint16_t
pread_16(struct intel_renderbuffer *irb, uint32_t offset)
{
   get_span_cache(irb, offset);

   return *(uint16_t *)(irb->span_cache + (offset & (SPAN_CACHE_SIZE - 1)));
}

static uint8_t
pread_8(struct intel_renderbuffer *irb, uint32_t offset)
{
   get_span_cache(irb, offset);

   return *(uint8_t *)(irb->span_cache + (offset & (SPAN_CACHE_SIZE - 1)));
}

static void
pwrite_32(struct intel_renderbuffer *irb, uint32_t offset, uint32_t val)
{
   clear_span_cache(irb);

   dri_bo_subdata(irb->region->buffer, offset, 4, &val);
}

static void
pwrite_xrgb8888(struct intel_renderbuffer *irb, uint32_t offset, uint32_t val)
{
   clear_span_cache(irb);

   dri_bo_subdata(irb->region->buffer, offset, 3, &val);
}

static void
pwrite_16(struct intel_renderbuffer *irb, uint32_t offset, uint16_t val)
{
   clear_span_cache(irb);

   dri_bo_subdata(irb->region->buffer, offset, 2, &val);
}

static void
pwrite_8(struct intel_renderbuffer *irb, uint32_t offset, uint8_t val)
{
   clear_span_cache(irb);

   dri_bo_subdata(irb->region->buffer, offset, 1, &val);
}

static uint32_t no_tile_swizzle(struct intel_renderbuffer *irb,
				int x, int y)
{
	return (y * irb->region->pitch + x) * irb->region->cpp;
}

/*
 * Deal with tiled surfaces
 */

static uint32_t x_tile_swizzle(struct intel_renderbuffer *irb,
			       int x, int y)
{
	int	tile_stride;
	int	xbyte;
	int	x_tile_off, y_tile_off;
	int	x_tile_number, y_tile_number;
	int	tile_off, tile_base;
	
	tile_stride = (irb->pfPitch * irb->region->cpp) << 3;

	xbyte = x * irb->region->cpp;

	x_tile_off = xbyte & 0x1ff;
	y_tile_off = y & 7;

	x_tile_number = xbyte >> 9;
	y_tile_number = y >> 3;

	tile_off = (y_tile_off << 9) + x_tile_off;

	switch (irb->region->bit_6_swizzle) {
	case I915_BIT_6_SWIZZLE_NONE:
	   break;
	case I915_BIT_6_SWIZZLE_9:
	   tile_off ^= ((tile_off >> 3) & 64);
	   break;
	case I915_BIT_6_SWIZZLE_9_10:
	   tile_off ^= ((tile_off >> 3) & 64) ^ ((tile_off >> 4) & 64);
	   break;
	case I915_BIT_6_SWIZZLE_9_11:
	   tile_off ^= ((tile_off >> 3) & 64) ^ ((tile_off >> 5) & 64);
	   break;
	case I915_BIT_6_SWIZZLE_9_10_11:
	   tile_off ^= ((tile_off >> 3) & 64) ^ ((tile_off >> 4) & 64) ^
	      ((tile_off >> 5) & 64);
	   break;
	default:
	   fprintf(stderr, "Unknown tile swizzling mode %d\n",
		   irb->region->bit_6_swizzle);
	   exit(1);
	}

	tile_base = (x_tile_number << 12) + y_tile_number * tile_stride;

#if 0
	printf("(%d,%d) -> %d + %d = %d (pitch = %d, tstride = %d)\n",
	       x, y, tile_off, tile_base,
	       tile_off + tile_base,
	       irb->pfPitch, tile_stride);
#endif

	return tile_base + tile_off;
}

static uint32_t y_tile_swizzle(struct intel_renderbuffer *irb,
			       int x, int y)
{
	int	tile_stride;
	int	xbyte;
	int	x_tile_off, y_tile_off;
	int	x_tile_number, y_tile_number;
	int	tile_off, tile_base;
	
	tile_stride = (irb->pfPitch * irb->region->cpp) << 5;

	xbyte = x * irb->region->cpp;

	x_tile_off = xbyte & 0x7f;
	y_tile_off = y & 0x1f;

	x_tile_number = xbyte >> 7;
	y_tile_number = y >> 5;

	tile_off = ((x_tile_off & ~0xf) << 5) + (y_tile_off << 4) +
	   (x_tile_off & 0xf);

	switch (irb->region->bit_6_swizzle) {
	case I915_BIT_6_SWIZZLE_NONE:
	   break;
	case I915_BIT_6_SWIZZLE_9:
	   tile_off ^= ((tile_off >> 3) & 64);
	   break;
	case I915_BIT_6_SWIZZLE_9_10:
	   tile_off ^= ((tile_off >> 3) & 64) ^ ((tile_off >> 4) & 64);
	   break;
	case I915_BIT_6_SWIZZLE_9_11:
	   tile_off ^= ((tile_off >> 3) & 64) ^ ((tile_off >> 5) & 64);
	   break;
	case I915_BIT_6_SWIZZLE_9_10_11:
	   tile_off ^= ((tile_off >> 3) & 64) ^ ((tile_off >> 4) & 64) ^
	      ((tile_off >> 5) & 64);
	   break;
	default:
	   fprintf(stderr, "Unknown tile swizzling mode %d\n",
		   irb->region->bit_6_swizzle);
	   exit(1);
	}

	tile_base = (x_tile_number << 12) + y_tile_number * tile_stride;

	return tile_base + tile_off;
}

/*
  break intelWriteRGBASpan_ARGB8888
*/

#undef DBG
#define DBG 0

#define LOCAL_VARS							\
   struct intel_context *intel = intel_context(ctx);			\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLint yScale = irb->RenderToTexture ? 1 : -1;			\
   const GLint yBias = irb->RenderToTexture ? 0 : irb->Base.Height - 1;	\
   unsigned int num_cliprects;						\
   struct drm_clip_rect *cliprects;					\
   int x_off, y_off;							\
   GLuint p;								\
   (void) p;								\
   intel_get_cliprects(intel, &cliprects, &num_cliprects, &x_off, &y_off);

/* XXX FBO: this is identical to the macro in spantmp2.h except we get
 * the cliprect info from the context, not the driDrawable.
 * Move this into spantmp2.h someday.
 */
#define HW_CLIPLOOP()							\
   do {									\
      int _nc = num_cliprects;						\
      while ( _nc-- ) {							\
	 int minx = cliprects[_nc].x1 - x_off;				\
	 int miny = cliprects[_nc].y1 - y_off;				\
	 int maxx = cliprects[_nc].x2 - x_off;				\
	 int maxy = cliprects[_nc].y2 - y_off;
	
#if 0
      }}
#endif

#define Y_FLIP(_y) ((_y) * yScale + yBias)

/* XXX with GEM, these need to tell the kernel */
#define HW_LOCK()

#define HW_UNLOCK()

/* Convenience macros to avoid typing the swizzle argument over and over */
#define NO_TILE(_X, _Y) no_tile_swizzle(irb, (_X) + x_off, (_Y) + y_off)
#define X_TILE(_X, _Y) x_tile_swizzle(irb, (_X) + x_off, (_Y) + y_off)
#define Y_TILE(_X, _Y) y_tile_swizzle(irb, (_X) + x_off, (_Y) + y_off)

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    intel##x##_RGB565
#define TAG2(x,y) intel##x##_RGB565##y
#define GET_VALUE(X, Y) pread_16(irb, NO_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_16(irb, NO_TILE(X, Y), V)
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    intel##x##_ARGB8888
#define TAG2(x,y) intel##x##_ARGB8888##y
#define GET_VALUE(X, Y) pread_32(irb, NO_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_32(irb, NO_TILE(X, Y), V)
#include "spantmp2.h"

/* 32 bit, xRGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    intel##x##_xRGB8888
#define TAG2(x,y) intel##x##_xRGB8888##y
#define GET_VALUE(X, Y) pread_xrgb8888(irb, NO_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_xrgb8888(irb, NO_TILE(X, Y), V)
#include "spantmp2.h"

/* 16 bit RGB565 color tile spanline and pixel functions
 */

#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    intel_XTile_##x##_RGB565
#define TAG2(x,y) intel_XTile_##x##_RGB565##y
#define GET_VALUE(X, Y) pread_16(irb, X_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_16(irb, X_TILE(X, Y), V)
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    intel_YTile_##x##_RGB565
#define TAG2(x,y) intel_YTile_##x##_RGB565##y
#define GET_VALUE(X, Y) pread_16(irb, Y_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_16(irb, Y_TILE(X, Y), V)
#include "spantmp2.h"

/* 32 bit ARGB888 color tile spanline and pixel functions
 */

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    intel_XTile_##x##_ARGB8888
#define TAG2(x,y) intel_XTile_##x##_ARGB8888##y
#define GET_VALUE(X, Y) pread_32(irb, X_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_32(irb, X_TILE(X, Y), V)
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    intel_YTile_##x##_ARGB8888
#define TAG2(x,y) intel_YTile_##x##_ARGB8888##y
#define GET_VALUE(X, Y) pread_32(irb, Y_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_32(irb, Y_TILE(X, Y), V)
#include "spantmp2.h"

/* 32 bit xRGB888 color tile spanline and pixel functions
 */

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    intel_XTile_##x##_xRGB8888
#define TAG2(x,y) intel_XTile_##x##_xRGB8888##y
#define GET_VALUE(X, Y) pread_xrgb8888(irb, X_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_xrgb8888(irb, X_TILE(X, Y), V)
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    intel_YTile_##x##_xRGB8888
#define TAG2(x,y) intel_YTile_##x##_xRGB8888##y
#define GET_VALUE(X, Y) pread_xrgb8888(irb, Y_TILE(X, Y))
#define PUT_VALUE(X, Y, V) pwrite_xrgb8888(irb, Y_TILE(X, Y), V)
#include "spantmp2.h"

#define LOCAL_DEPTH_VARS						\
   struct intel_context *intel = intel_context(ctx);			\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLint yScale = irb->RenderToTexture ? 1 : -1;			\
   const GLint yBias = irb->RenderToTexture ? 0 : irb->Base.Height - 1; \
   unsigned int num_cliprects;						\
   struct drm_clip_rect *cliprects;					\
   int x_off, y_off;							\
   intel_get_cliprects(intel, &cliprects, &num_cliprects, &x_off, &y_off);


#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

/**
 ** 16-bit depthbuffer functions.
 **/
#define VALUE_TYPE GLushort
#define WRITE_DEPTH(_x, _y, d) pwrite_16(irb, NO_TILE(_x, _y), d)
#define READ_DEPTH(d, _x, _y) d = pread_16(irb, NO_TILE(_x, _y))
#define TAG(x) intel##x##_z16
#include "depthtmp.h"


/**
 ** 16-bit x tile depthbuffer functions.
 **/
#define VALUE_TYPE GLushort
#define WRITE_DEPTH(_x, _y, d) pwrite_16(irb, X_TILE(_x, _y), d)
#define READ_DEPTH(d, _x, _y) d = pread_16(irb, X_TILE(_x, _y))
#define TAG(x) intel_XTile_##x##_z16
#include "depthtmp.h"

/**
 ** 16-bit y tile depthbuffer functions.
 **/
#define VALUE_TYPE GLushort
#define WRITE_DEPTH(_x, _y, d) pwrite_16(irb, Y_TILE(_x, _y), d)
#define READ_DEPTH(d, _x, _y) d = pread_16(irb, Y_TILE(_x, _y))
#define TAG(x) intel_YTile_##x##_z16
#include "depthtmp.h"


/**
 ** 24/8-bit interleaved depth/stencil functions
 ** Note: we're actually reading back combined depth+stencil values.
 ** The wrappers in main/depthstencil.c are used to extract the depth
 ** and stencil values.
 **/
#define VALUE_TYPE GLuint

/* Change ZZZS -> SZZZ */
#define WRITE_DEPTH(_x, _y, d)					\
   pwrite_32(irb, NO_TILE(_x, _y), ((d) >> 8) | ((d) << 24))

/* Change SZZZ -> ZZZS */
#define READ_DEPTH( d, _x, _y ) {				\
   GLuint tmp = pread_32(irb, NO_TILE(_x, _y));			\
   d = (tmp << 8) | (tmp >> 24);				\
}

#define TAG(x) intel##x##_z24_s8
#include "depthtmp.h"


/**
 ** 24/8-bit x-tile interleaved depth/stencil functions
 ** Note: we're actually reading back combined depth+stencil values.
 ** The wrappers in main/depthstencil.c are used to extract the depth
 ** and stencil values.
 **/
#define VALUE_TYPE GLuint

/* Change ZZZS -> SZZZ */
#define WRITE_DEPTH(_x, _y, d)					\
   pwrite_32(irb, X_TILE(_x, _y), ((d) >> 8) | ((d) << 24))

/* Change SZZZ -> ZZZS */
#define READ_DEPTH( d, _x, _y ) {				\
   GLuint tmp = pread_32(irb, X_TILE(_x, _y));		\
   d = (tmp << 8) | (tmp >> 24);				\
}

#define TAG(x) intel_XTile_##x##_z24_s8
#include "depthtmp.h"

/**
 ** 24/8-bit y-tile interleaved depth/stencil functions
 ** Note: we're actually reading back combined depth+stencil values.
 ** The wrappers in main/depthstencil.c are used to extract the depth
 ** and stencil values.
 **/
#define VALUE_TYPE GLuint

/* Change ZZZS -> SZZZ */
#define WRITE_DEPTH(_x, _y, d)					\
   pwrite_32(irb, Y_TILE(_x, _y), ((d) >> 8) | ((d) << 24))

/* Change SZZZ -> ZZZS */
#define READ_DEPTH( d, _x, _y ) {				\
   GLuint tmp = pread_32(irb, Y_TILE(_x, _y));			\
   d = (tmp << 8) | (tmp >> 24);				\
}

#define TAG(x) intel_YTile_##x##_z24_s8
#include "depthtmp.h"


/**
 ** 8-bit stencil function (XXX FBO: This is obsolete)
 **/
#define WRITE_STENCIL(_x, _y, d) pwrite_8(irb, NO_TILE(_x, _y) + 3, d)
#define READ_STENCIL(d, _x, _y) d = pread_8(irb, NO_TILE(_x, _y) + 3);
#define TAG(x) intel##x##_z24_s8
#include "stenciltmp.h"

/**
 ** 8-bit x-tile stencil function (XXX FBO: This is obsolete)
 **/
#define WRITE_STENCIL(_x, _y, d) pwrite_8(irb, X_TILE(_x, _y) + 3, d)
#define READ_STENCIL(d, _x, _y) d = pread_8(irb, X_TILE(_x, _y) + 3);
#define TAG(x) intel_XTile_##x##_z24_s8
#include "stenciltmp.h"

/**
 ** 8-bit y-tile stencil function (XXX FBO: This is obsolete)
 **/
#define WRITE_STENCIL(_x, _y, d) pwrite_8(irb, Y_TILE(_x, _y) + 3, d)
#define READ_STENCIL(d, _x, _y) d = pread_8(irb, Y_TILE(_x, _y) + 3)
#define TAG(x) intel_YTile_##x##_z24_s8
#include "stenciltmp.h"

void
intel_renderbuffer_map(struct intel_context *intel, struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   if (irb == NULL || irb->region == NULL)
      return;

   irb->pfPitch = irb->region->pitch;

   intel_set_span_functions(intel, rb);
}

void
intel_renderbuffer_unmap(struct intel_context *intel,
			 struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   if (irb == NULL || irb->region == NULL)
      return;

   clear_span_cache(irb);
   irb->pfPitch = 0;

   rb->GetRow = NULL;
   rb->PutRow = NULL;
}

/**
 * Map or unmap all the renderbuffers which we may need during
 * software rendering.
 * XXX in the future, we could probably convey extra information to
 * reduce the number of mappings needed.  I.e. if doing a glReadPixels
 * from the depth buffer, we really only need one mapping.
 *
 * XXX Rewrite this function someday.
 * We can probably just loop over all the renderbuffer attachments,
 * map/unmap all of them, and not worry about the _ColorDrawBuffers
 * _ColorReadBuffer, _DepthBuffer or _StencilBuffer fields.
 */
static void
intel_map_unmap_buffers(struct intel_context *intel, GLboolean map)
{
   GLcontext *ctx = &intel->ctx;
   GLuint i, j;

   /* color draw buffers */
   for (j = 0; j < ctx->DrawBuffer->_NumColorDrawBuffers; j++) {
      if (map)
	 intel_renderbuffer_map(intel, ctx->DrawBuffer->_ColorDrawBuffers[j]);
      else
	 intel_renderbuffer_unmap(intel, ctx->DrawBuffer->_ColorDrawBuffers[j]);
   }

   /* check for render to textures */
   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att =
         ctx->DrawBuffer->Attachment + i;
      struct gl_texture_object *tex = att->Texture;
      if (tex) {
         /* render to texture */
         ASSERT(att->Renderbuffer);
         if (map)
            intel_tex_map_images(intel, intel_texture_object(tex));
         else
            intel_tex_unmap_images(intel, intel_texture_object(tex));
      }
   }

   /* color read buffers */
   if (map)
      intel_renderbuffer_map(intel, ctx->ReadBuffer->_ColorReadBuffer);
   else
      intel_renderbuffer_unmap(intel, ctx->ReadBuffer->_ColorReadBuffer);

   /* depth buffer (Note wrapper!) */
   if (ctx->DrawBuffer->_DepthBuffer) {
      if (map)
	 intel_renderbuffer_map(intel, ctx->DrawBuffer->_DepthBuffer->Wrapped);
      else
	 intel_renderbuffer_unmap(intel,
				  ctx->DrawBuffer->_DepthBuffer->Wrapped);
   }

   /* stencil buffer (Note wrapper!) */
   if (ctx->DrawBuffer->_StencilBuffer) {
      if (map)
	 intel_renderbuffer_map(intel,
				ctx->DrawBuffer->_StencilBuffer->Wrapped);
      else
	 intel_renderbuffer_unmap(intel,
				  ctx->DrawBuffer->_StencilBuffer->Wrapped);
   }
}



/**
 * Prepare for softare rendering.  Map current read/draw framebuffers'
 * renderbuffes and all currently bound texture objects.
 *
 * Old note: Moved locking out to get reasonable span performance.
 */
void
intelSpanRenderStart(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);
   GLuint i;

   intelFlush(&intel->ctx);
   LOCK_HARDWARE(intel);

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;
         intel_tex_map_images(intel, intel_texture_object(texObj));
      }
   }

   intel_map_unmap_buffers(intel, GL_TRUE);
}

/**
 * Called when done softare rendering.  Unmap the buffers we mapped in
 * the above function.
 */
void
intelSpanRenderFinish(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);
   GLuint i;

   _swrast_flush(ctx);

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;
         intel_tex_unmap_images(intel, intel_texture_object(texObj));
      }
   }

   intel_map_unmap_buffers(intel, GL_FALSE);

   UNLOCK_HARDWARE(intel);
}


void
intelInitSpanFuncs(GLcontext * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = intelSpanRenderStart;
   swdd->SpanRenderFinish = intelSpanRenderFinish;
}


/**
 * Plug in appropriate span read/write functions for the given renderbuffer.
 * These are used for the software fallbacks.
 */
static void
intel_set_span_functions(struct intel_context *intel,
			 struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = (struct intel_renderbuffer *) rb;
   uint32_t tiling;

   /* If in GEM mode, we need to do the tile address swizzling ourselves,
    * instead of the fence registers handling it.
    */
   if (intel->ttm)
      tiling = irb->region->tiling;
   else
      tiling = I915_TILING_NONE;

   if (rb->_ActualFormat == GL_RGB5) {
      /* 565 RGB */
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitPointers_RGB565(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitPointers_RGB565(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitPointers_RGB565(rb);
	 break;
      }
   }
   else if (rb->_ActualFormat == GL_RGB8) {
      /* 8888 RGBx */
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitPointers_xRGB8888(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitPointers_xRGB8888(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitPointers_xRGB8888(rb);
	 break;
      }
   }
   else if (rb->_ActualFormat == GL_RGBA8) {
      /* 8888 RGBA */
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitPointers_ARGB8888(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitPointers_ARGB8888(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitPointers_ARGB8888(rb);
	 break;
      }
   }
   else if (rb->_ActualFormat == GL_DEPTH_COMPONENT16) {
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitDepthPointers_z16(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitDepthPointers_z16(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitDepthPointers_z16(rb);
	 break;
      }
   }
   else if (rb->_ActualFormat == GL_DEPTH_COMPONENT24 ||        /* XXX FBO remove */
            rb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT) {
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitDepthPointers_z24_s8(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitDepthPointers_z24_s8(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitDepthPointers_z24_s8(rb);
	 break;
      }
   }
   else if (rb->_ActualFormat == GL_STENCIL_INDEX8_EXT) {
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitStencilPointers_z24_s8(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitStencilPointers_z24_s8(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitStencilPointers_z24_s8(rb);
	 break;
      }
   }
   else {
      _mesa_problem(NULL,
                    "Unexpected _ActualFormat in intelSetSpanFunctions");
   }
}
