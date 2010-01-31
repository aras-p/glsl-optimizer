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
	
        x += irb->region->draw_x;
        y += irb->region->draw_y;

	tile_stride = (irb->region->pitch * irb->region->cpp) << 3;

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
	       irb->region->pitch, tile_stride);
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
	
        x += irb->region->draw_x;
        y += irb->region->draw_y;

	tile_stride = (irb->region->pitch * irb->region->cpp) << 5;

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
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLint yScale = ctx->DrawBuffer->Name ? 1 : -1;			\
   const GLint yBias = ctx->DrawBuffer->Name ? 0 : irb->Base.Height - 1;\
   int minx = 0, miny = 0;						\
   int maxx = ctx->DrawBuffer->Width;					\
   int maxy = ctx->DrawBuffer->Height;					\
   int pitch = irb->region->pitch * irb->region->cpp;			\
   void *buf = irb->region->buffer->virtual;				\
   GLuint p;								\
   (void) p;								\
   (void)buf; (void)pitch; /* unused for non-gttmap. */			\

#define HW_CLIPLOOP()
#define HW_ENDCLIPLOOP()

#define Y_FLIP(_y) ((_y) * yScale + yBias)

#define HW_LOCK()

#define HW_UNLOCK()

/* Convenience macros to avoid typing the swizzle argument over and over */
#define NO_TILE(_X, _Y) no_tile_swizzle(irb, (_X), (_Y))
#define X_TILE(_X, _Y) x_tile_swizzle(irb, (_X), (_Y))
#define Y_TILE(_X, _Y) y_tile_swizzle(irb, (_X), (_Y))

/* r5g6b5 color span and pixel functions */
#define INTEL_PIXEL_FMT GL_RGB
#define INTEL_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5
#define INTEL_READ_VALUE(offset) pread_16(irb, offset)
#define INTEL_WRITE_VALUE(offset, v) pwrite_16(irb, offset, v)
#define INTEL_TAG(x) x##_RGB565
#include "intel_spantmp.h"

/* a4r4g4b4 color span and pixel functions */
#define INTEL_PIXEL_FMT GL_BGRA
#define INTEL_PIXEL_TYPE GL_UNSIGNED_SHORT_4_4_4_4_REV
#define INTEL_READ_VALUE(offset) pread_16(irb, offset)
#define INTEL_WRITE_VALUE(offset, v) pwrite_16(irb, offset, v)
#define INTEL_TAG(x) x##_ARGB4444
#include "intel_spantmp.h"

/* a1r5g5b5 color span and pixel functions */
#define INTEL_PIXEL_FMT GL_BGRA
#define INTEL_PIXEL_TYPE GL_UNSIGNED_SHORT_1_5_5_5_REV
#define INTEL_READ_VALUE(offset) pread_16(irb, offset)
#define INTEL_WRITE_VALUE(offset, v) pwrite_16(irb, offset, v)
#define INTEL_TAG(x) x##_ARGB1555
#include "intel_spantmp.h"

/* a8r8g8b8 color span and pixel functions */
#define INTEL_PIXEL_FMT GL_BGRA
#define INTEL_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define INTEL_READ_VALUE(offset) pread_32(irb, offset)
#define INTEL_WRITE_VALUE(offset, v) pwrite_32(irb, offset, v)
#define INTEL_TAG(x) x##_ARGB8888
#include "intel_spantmp.h"

/* x8r8g8b8 color span and pixel functions */
#define INTEL_PIXEL_FMT GL_BGR
#define INTEL_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define INTEL_READ_VALUE(offset) pread_xrgb8888(irb, offset)
#define INTEL_WRITE_VALUE(offset, v) pwrite_xrgb8888(irb, offset, v)
#define INTEL_TAG(x) x##_xRGB8888
#include "intel_spantmp.h"

#define LOCAL_DEPTH_VARS						\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLint yScale = ctx->DrawBuffer->Name ? 1 : -1;			\
   const GLint yBias = ctx->DrawBuffer->Name ? 0 : irb->Base.Height - 1;\
   int minx = 0, miny = 0;						\
   int maxx = ctx->DrawBuffer->Width;					\
   int maxy = ctx->DrawBuffer->Height;					\
   int pitch = irb->region->pitch * irb->region->cpp;			\
   void *buf = irb->region->buffer->virtual;				\
   (void)buf; (void)pitch; /* unused for non-gttmap. */			\

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

/* z16 depthbuffer functions. */
#define INTEL_VALUE_TYPE GLushort
#define INTEL_WRITE_DEPTH(offset, d) pwrite_16(irb, offset, d)
#define INTEL_READ_DEPTH(offset) pread_16(irb, offset)
#define INTEL_TAG(name) name##_z16
#include "intel_depthtmp.h"

/* z24x8 depthbuffer functions. */
#define INTEL_VALUE_TYPE GLuint
#define INTEL_WRITE_DEPTH(offset, d) pwrite_32(irb, offset, d)
#define INTEL_READ_DEPTH(offset) pread_32(irb, offset)
#define INTEL_TAG(name) name##_z24_x8
#include "intel_depthtmp.h"


/**
 ** 8-bit stencil function (XXX FBO: This is obsolete)
 **/
/* XXX */
#define WRITE_STENCIL(_x, _y, d) pwrite_8(irb, NO_TILE(_x, _y) + 3, d)
#define READ_STENCIL(d, _x, _y) d = pread_8(irb, NO_TILE(_x, _y) + 3);
#define TAG(x) intel_gttmap_##x##_z24_s8
#include "stenciltmp.h"

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

   if (intel->intelScreen->kernel_exec_fencing)
      drm_intel_gem_bo_map_gtt(irb->region->buffer);

   intel_set_span_functions(intel, rb);
}

void
intel_renderbuffer_unmap(struct intel_context *intel,
			 struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   if (irb == NULL || irb->region == NULL)
      return;

   if (intel->intelScreen->kernel_exec_fencing)
      drm_intel_gem_bo_unmap_gtt(irb->region->buffer);
   else
      clear_span_cache(irb);

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
intel_map_unmap_framebuffer(struct intel_context *intel,
			    struct gl_framebuffer *fb,
			    GLboolean map)
{
   GLuint i;

   /* color draw buffers */
   for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
      if (map)
         intel_renderbuffer_map(intel, fb->_ColorDrawBuffers[i]);
      else
         intel_renderbuffer_unmap(intel, fb->_ColorDrawBuffers[i]);
   }

   /* color read buffer */
   if (map)
      intel_renderbuffer_map(intel, fb->_ColorReadBuffer);
   else
      intel_renderbuffer_unmap(intel, fb->_ColorReadBuffer);

   /* check for render to textures */
   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att =
         fb->Attachment + i;
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

   /* depth buffer (Note wrapper!) */
   if (fb->_DepthBuffer) {
      if (map)
         intel_renderbuffer_map(intel, fb->_DepthBuffer->Wrapped);
      else
         intel_renderbuffer_unmap(intel, fb->_DepthBuffer->Wrapped);
   }

   /* stencil buffer (Note wrapper!) */
   if (fb->_StencilBuffer) {
      if (map)
         intel_renderbuffer_map(intel, fb->_StencilBuffer->Wrapped);
      else
         intel_renderbuffer_unmap(intel, fb->_StencilBuffer->Wrapped);
   }

   intel_check_front_buffer_rendering(intel);
}

/**
 * Prepare for software rendering.  Map current read/draw framebuffers'
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

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;
         intel_tex_map_images(intel, intel_texture_object(texObj));
      }
   }

   intel_map_unmap_framebuffer(intel, ctx->DrawBuffer, GL_TRUE);
   if (ctx->ReadBuffer != ctx->DrawBuffer)
      intel_map_unmap_framebuffer(intel, ctx->ReadBuffer, GL_TRUE);
}

/**
 * Called when done software rendering.  Unmap the buffers we mapped in
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

   intel_map_unmap_framebuffer(intel, ctx->DrawBuffer, GL_FALSE);
   if (ctx->ReadBuffer != ctx->DrawBuffer)
      intel_map_unmap_framebuffer(intel, ctx->ReadBuffer, GL_FALSE);
}


void
intelInitSpanFuncs(GLcontext * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = intelSpanRenderStart;
   swdd->SpanRenderFinish = intelSpanRenderFinish;
}

void
intel_map_vertex_shader_textures(GLcontext *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   int i;

   if (ctx->VertexProgram._Current == NULL)
      return;

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled &&
	  ctx->VertexProgram._Current->Base.TexturesUsed[i] != 0) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;

         intel_tex_map_images(intel, intel_texture_object(texObj));
      }
   }
}

void
intel_unmap_vertex_shader_textures(GLcontext *ctx)
{
   struct intel_context *intel = intel_context(ctx);
   int i;

   if (ctx->VertexProgram._Current == NULL)
      return;

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled &&
	  ctx->VertexProgram._Current->Base.TexturesUsed[i] != 0) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;

         intel_tex_unmap_images(intel, intel_texture_object(texObj));
      }
   }
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
   uint32_t tiling = irb->region->tiling;

   if (intel->intelScreen->kernel_exec_fencing) {
      switch (irb->Base.Format) {
      case MESA_FORMAT_RGB565:
	 intel_gttmap_InitPointers_RGB565(rb);
	 break;
      case MESA_FORMAT_ARGB4444:
	 intel_gttmap_InitPointers_ARGB4444(rb);
	 break;
      case MESA_FORMAT_ARGB1555:
	 intel_gttmap_InitPointers_ARGB1555(rb);
	 break;
      case MESA_FORMAT_XRGB8888:
         intel_gttmap_InitPointers_xRGB8888(rb);
	 break;
      case MESA_FORMAT_ARGB8888:
	 intel_gttmap_InitPointers_ARGB8888(rb);
	 break;
      case MESA_FORMAT_Z16:
	 intel_gttmap_InitDepthPointers_z16(rb);
	 break;
      case MESA_FORMAT_X8_Z24:
	 intel_gttmap_InitDepthPointers_z24_x8(rb);
	 break;
      case MESA_FORMAT_S8_Z24:
	 /* There are a few different ways SW asks us to access the S8Z24 data:
	  * Z24 depth-only depth reads
	  * S8Z24 depth reads
	  * S8Z24 stencil reads.
	  */
	 if (rb->Format == MESA_FORMAT_S8_Z24) {
	    intel_gttmap_InitDepthPointers_z24_x8(rb);
	 } else if (rb->Format == MESA_FORMAT_S8) {
	    intel_gttmap_InitStencilPointers_z24_s8(rb);
	 }
	 break;
      default:
	 _mesa_problem(NULL,
		       "Unexpected MesaFormat %d in intelSetSpanFunctions",
		       irb->Base.Format);
	 break;
      }
      return;
   }

   /* If in GEM mode, we need to do the tile address swizzling ourselves,
    * instead of the fence registers handling it.
    */
   switch (irb->Base.Format) {
   case MESA_FORMAT_RGB565:
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
      break;
   case MESA_FORMAT_ARGB4444:
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitPointers_ARGB4444(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitPointers_ARGB4444(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitPointers_ARGB4444(rb);
	 break;
      }
      break;
   case MESA_FORMAT_ARGB1555:
      switch (tiling) {
      case I915_TILING_NONE:
      default:
	 intelInitPointers_ARGB1555(rb);
	 break;
      case I915_TILING_X:
	 intel_XTile_InitPointers_ARGB1555(rb);
	 break;
      case I915_TILING_Y:
	 intel_YTile_InitPointers_ARGB1555(rb);
	 break;
      }
      break;
   case MESA_FORMAT_XRGB8888:
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
      break;
   case MESA_FORMAT_ARGB8888:
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
      break;
   case MESA_FORMAT_Z16:
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
      break;
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_S8_Z24:
      /* There are a few different ways SW asks us to access the S8Z24 data:
       * Z24 depth-only depth reads
       * S8Z24 depth reads
       * S8Z24 stencil reads.
       */
      if (rb->Format == MESA_FORMAT_S8_Z24) {
	 switch (tiling) {
	 case I915_TILING_NONE:
	 default:
	    intelInitDepthPointers_z24_x8(rb);
	    break;
	 case I915_TILING_X:
	    intel_XTile_InitDepthPointers_z24_x8(rb);
	    break;
	 case I915_TILING_Y:
	    intel_YTile_InitDepthPointers_z24_x8(rb);
	    break;
	 }
      } else if (rb->Format == MESA_FORMAT_S8) {
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
      } else {
	 _mesa_problem(NULL,
		       "Unexpected ActualFormat in intelSetSpanFunctions");
      }
      break;
   default:
      _mesa_problem(NULL,
                    "Unexpected MesaFormat in intelSetSpanFunctions");
      break;
   }
}
