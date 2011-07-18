/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2011 Intel Corporation
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
 * Authors:
 *     Chad Versace <chad@chad-versace.us>
 *
 **************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "main/glheader.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/colormac.h"
#include "main/renderbuffer.h"

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

#undef DBG
#define DBG 0

#define LOCAL_VARS							\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   int minx = 0, miny = 0;						\
   int maxx = rb->Width;						\
   int maxy = rb->Height;						\
   int pitch = rb->RowStride * irb->region->cpp;			\
   void *buf = rb->Data;						\
   GLuint p;								\
   (void) p;

#define HW_CLIPLOOP()
#define HW_ENDCLIPLOOP()

#define Y_FLIP(_y) (_y)

#define HW_LOCK()

#define HW_UNLOCK()

/* r5g6b5 color span and pixel functions */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5
#define TAG(x) intel_##x##_RGB565
#define TAG2(x,y) intel_##x##y_RGB565
#include "spantmp2.h"

/* a4r4g4b4 color span and pixel functions */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_4_4_4_4_REV
#define TAG(x) intel_##x##_ARGB4444
#define TAG2(x,y) intel_##x##y_ARGB4444
#include "spantmp2.h"

/* a1r5g5b5 color span and pixel functions */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_1_5_5_5_REV
#define TAG(x) intel_##x##_ARGB1555
#define TAG2(x,y) intel_##x##y##_ARGB1555
#include "spantmp2.h"

/* a8r8g8b8 color span and pixel functions */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define TAG(x) intel_##x##_ARGB8888
#define TAG2(x,y) intel_##x##y##_ARGB8888
#include "spantmp2.h"

/* x8r8g8b8 color span and pixel functions */
#define SPANTMP_PIXEL_FMT GL_BGR
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define TAG(x) intel_##x##_xRGB8888
#define TAG2(x,y) intel_##x##y##_xRGB8888
#include "spantmp2.h"

/* a8 color span and pixel functions */
#define SPANTMP_PIXEL_FMT GL_ALPHA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_BYTE
#define TAG(x) intel_##x##_A8
#define TAG2(x,y) intel_##x##y##_A8
#include "spantmp2.h"

/* ------------------------------------------------------------------------- */
/* s8 stencil span and pixel functions                                       */
/* ------------------------------------------------------------------------- */

/*
 * HAVE_HW_STENCIL_SPANS determines if stencil buffer read/writes are done with
 * memcpy or for loops. Since the stencil buffer is interleaved, memcpy won't
 * work.
 */
#define HAVE_HW_STENCIL_SPANS 0

#define LOCAL_STENCIL_VARS						\
   (void) ctx;								\
   int minx = 0;							\
   int miny = 0;							\
   int maxx = rb->Width;						\
   int maxy = rb->Height;						\
									\
   /*									\
    * Here we ignore rb->Data and rb->RowStride as set by		\
    * intelSpanRenderStart. Since intel_offset_S8 decodes the W tile	\
    * manually, the region's *real* base address and stride is		\
    * required.								\
    */									\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   uint8_t *buf = irb->region->buffer->virtual;				\
   unsigned stride = irb->region->pitch;				\
   unsigned height = 2 * irb->region->height;				\
   bool flip = rb->Name == 0;						\
   int y_scale = flip ? -1 : 1;						\
   int y_bias = flip ? (height - 1) : 0;				\

#undef Y_FLIP
#define Y_FLIP(y) (y_scale * (y) + y_bias)

/**
 * \brief Get pointer offset into stencil buffer.
 *
 * The stencil buffer is W tiled. Since the GTT is incapable of W fencing, we
 * must decode the tile's layout in software.
 *
 * See
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.2.1 W-Major Tile
 *     Format.
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.3 Tiling Algorithm
 *
 * Even though the returned offset is always positive, the return type is
 * signed due to
 *    commit e8b1c6d6f55f5be3bef25084fdd8b6127517e137
 *    mesa: Fix return type of  _mesa_get_format_bytes() (#37351)
 */
static inline intptr_t
intel_offset_S8(uint32_t stride, uint32_t x, uint32_t y)
{
   uint32_t tile_size = 4096;
   uint32_t tile_width = 64;
   uint32_t tile_height = 64;
   uint32_t row_size = 64 * stride;

   uint32_t tile_x = x / tile_width;
   uint32_t tile_y = y / tile_height;

   /* The byte's address relative to the tile's base addres. */
   uint32_t byte_x = x % tile_width;
   uint32_t byte_y = y % tile_height;

   uintptr_t u = tile_y * row_size
               + tile_x * tile_size
               + 512 * (byte_x / 8)
               +  64 * (byte_y / 8)
               +  32 * ((byte_y / 4) % 2)
               +  16 * ((byte_x / 4) % 2)
               +   8 * ((byte_y / 2) % 2)
               +   4 * ((byte_x / 2) % 2)
               +   2 * (byte_y % 2)
               +   1 * (byte_x % 2);

   /*
    * Errata for Gen5:
    *
    * An additional offset is needed which is not documented in the PRM.
    *
    * if ((byte_x / 8) % 2 == 1) {
    *    if ((byte_y / 8) % 2) == 0) {
    *       u += 64;
    *    } else {
    *       u -= 64;
    *    }
    * }
    *
    * The offset is expressed more tersely as
    * u += ((int) x & 0x8) * (8 - (((int) y & 0x8) << 1));
    */

   return u;
}

#define WRITE_STENCIL(x, y, src)  buf[intel_offset_S8(stride, x, y)] = src;
#define READ_STENCIL(dest, x, y) dest = buf[intel_offset_S8(stride, x, y)]
#define TAG(x) intel_##x##_S8
#include "stenciltmp.h"

/* ------------------------------------------------------------------------- */

void
intel_renderbuffer_map(struct intel_context *intel, struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   if (!irb)
      return;

   if (irb->wrapped_depth)
      intel_renderbuffer_map(intel, irb->wrapped_depth);
   if (irb->wrapped_stencil)
      intel_renderbuffer_map(intel, irb->wrapped_stencil);

   if (!irb->region)
      return;

   drm_intel_gem_bo_map_gtt(irb->region->buffer);

   rb->Data = irb->region->buffer->virtual;
   rb->RowStride = irb->region->pitch;

   if (!rb->Name) {
      /* Flip orientation of the window system buffer */
      rb->Data += rb->RowStride * (irb->region->height - 1) * irb->region->cpp;
      rb->RowStride = -rb->RowStride;
   } else {
      /* Adjust the base pointer of a texture image drawbuffer to the image
       * within the miptree region (all else has draw_x/y = 0).
       */
      rb->Data += irb->draw_x * irb->region->cpp;
      rb->Data += irb->draw_y * rb->RowStride * irb->region->cpp;
   }

   intel_set_span_functions(intel, rb);
}

void
intel_renderbuffer_unmap(struct intel_context *intel,
			 struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   if (!irb)
      return;

   if (irb->wrapped_depth)
      intel_renderbuffer_unmap(intel, irb->wrapped_depth);
   if (irb->wrapped_stencil)
      intel_renderbuffer_unmap(intel, irb->wrapped_stencil);

   if (!irb->region)
      return;

   drm_intel_gem_bo_unmap_gtt(irb->region->buffer);

   rb->GetRow = NULL;
   rb->PutRow = NULL;
   rb->Data = NULL;
   rb->RowStride = 0;
}

static void
intel_framebuffer_map(struct intel_context *intel, struct gl_framebuffer *fb)
{
   int i;

   for (i = 0; i < BUFFER_COUNT; i++) {
      intel_renderbuffer_map(intel, fb->Attachment[i].Renderbuffer);
   }

   intel_check_front_buffer_rendering(intel);
}

static void
intel_framebuffer_unmap(struct intel_context *intel, struct gl_framebuffer *fb)
{
   int i;

   for (i = 0; i < BUFFER_COUNT; i++) {
      intel_renderbuffer_unmap(intel, fb->Attachment[i].Renderbuffer);
   }
}

/**
 * Prepare for software rendering.  Map current read/draw framebuffers'
 * renderbuffes and all currently bound texture objects.
 *
 * Old note: Moved locking out to get reasonable span performance.
 */
void
intelSpanRenderStart(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);
   GLuint i;

   intel_flush(&intel->ctx);
   intel_prepare_render(intel);

   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;

         intel_finalize_mipmap_tree(intel, i);
         intel_tex_map_images(intel, intel_texture_object(texObj));
      }
   }

   intel_framebuffer_map(intel, ctx->DrawBuffer);
   if (ctx->ReadBuffer != ctx->DrawBuffer) {
      intel_framebuffer_map(intel, ctx->ReadBuffer);
   }
}

/**
 * Called when done software rendering.  Unmap the buffers we mapped in
 * the above function.
 */
void
intelSpanRenderFinish(struct gl_context * ctx)
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

   intel_framebuffer_unmap(intel, ctx->DrawBuffer);
   if (ctx->ReadBuffer != ctx->DrawBuffer) {
      intel_framebuffer_unmap(intel, ctx->ReadBuffer);
   }
}


void
intelInitSpanFuncs(struct gl_context * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = intelSpanRenderStart;
   swdd->SpanRenderFinish = intelSpanRenderFinish;
}

void
intel_map_vertex_shader_textures(struct gl_context *ctx)
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
intel_unmap_vertex_shader_textures(struct gl_context *ctx)
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

typedef void (*span_init_func)(struct gl_renderbuffer *rb);

static span_init_func intel_span_init_funcs[MESA_FORMAT_COUNT] =
{
   [MESA_FORMAT_A8] = intel_InitPointers_A8,
   [MESA_FORMAT_RGB565] = intel_InitPointers_RGB565,
   [MESA_FORMAT_ARGB4444] = intel_InitPointers_ARGB4444,
   [MESA_FORMAT_ARGB1555] = intel_InitPointers_ARGB1555,
   [MESA_FORMAT_XRGB8888] = intel_InitPointers_xRGB8888,
   [MESA_FORMAT_ARGB8888] = intel_InitPointers_ARGB8888,
   [MESA_FORMAT_SARGB8] = intel_InitPointers_ARGB8888,
   [MESA_FORMAT_Z16] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_X8_Z24] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_S8_Z24] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_S8] = intel_InitStencilPointers_S8,
   [MESA_FORMAT_R8] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_RG88] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_R16] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_RG1616] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_RGBA_FLOAT32] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_RG_FLOAT32] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_R_FLOAT32] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_INTENSITY_FLOAT32] = _mesa_set_renderbuffer_accessors,
   [MESA_FORMAT_LUMINANCE_FLOAT32] = _mesa_set_renderbuffer_accessors,
};

bool
intel_span_supports_format(gl_format format)
{
   return intel_span_init_funcs[format] != NULL;
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

   assert(intel_span_init_funcs[irb->Base.Format]);
   intel_span_init_funcs[irb->Base.Format](rb);
}
