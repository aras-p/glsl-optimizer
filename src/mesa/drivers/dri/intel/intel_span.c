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

#undef DBG
#define DBG 0

#define LOCAL_VARS							\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLint yScale = rb->Name ? 1 : -1;				\
   const GLint yBias = rb->Name ? 0 : rb->Height - 1;			\
   int minx = 0, miny = 0;						\
   int maxx = rb->Width;						\
   int maxy = rb->Height;						\
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

/* Convenience macros to avoid typing the address argument over and over */
#define NO_TILE(_X, _Y) (((_Y) * irb->region->pitch + (_X)) * irb->region->cpp)

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

#define LOCAL_DEPTH_VARS						\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLint yScale = rb->Name ? 1 : -1;				\
   const GLint yBias = rb->Name ? 0 : rb->Height - 1;			\
   int minx = 0, miny = 0;						\
   int maxx = rb->Width;						\
   int maxy = rb->Height;						\
   int pitch = irb->region->pitch * irb->region->cpp;			\
   void *buf = irb->region->buffer->virtual;				\
   (void)buf; (void)pitch; /* unused for non-gttmap. */			\

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

/* z16 depthbuffer functions. */
#define VALUE_TYPE GLushort
#define WRITE_DEPTH(_x, _y, d) \
   (*(uint16_t *)(irb->region->buffer->virtual + NO_TILE(_x, _y)) = d)
#define READ_DEPTH(d, _x, _y) \
   d = *(uint16_t *)(irb->region->buffer->virtual + NO_TILE(_x, _y))
#define TAG(x) intel_##x##_z16
#include "depthtmp.h"

/* z24_s8 and z24_x8 depthbuffer functions. */
#define VALUE_TYPE GLuint
#define WRITE_DEPTH(_x, _y, d) \
   (*(uint32_t *)(irb->region->buffer->virtual + NO_TILE(_x, _y)) = d)
#define READ_DEPTH(d, _x, _y) \
   d = *(uint32_t *)(irb->region->buffer->virtual + NO_TILE(_x, _y))
#define TAG(x) intel_##x##_z24_x8
#include "depthtmp.h"

void
intel_renderbuffer_map(struct intel_context *intel, struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   if (irb == NULL || irb->region == NULL)
      return;

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

   drm_intel_gem_bo_unmap_gtt(irb->region->buffer);

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
   intel_prepare_render(intel);

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

   switch (irb->Base.Format) {
   case MESA_FORMAT_RGB565:
      intel_InitPointers_RGB565(rb);
      break;
   case MESA_FORMAT_ARGB4444:
      intel_InitPointers_ARGB4444(rb);
      break;
   case MESA_FORMAT_ARGB1555:
      intel_InitPointers_ARGB1555(rb);
      break;
   case MESA_FORMAT_XRGB8888:
      intel_InitPointers_xRGB8888(rb);
      break;
   case MESA_FORMAT_ARGB8888:
      intel_InitPointers_ARGB8888(rb);
      break;
   case MESA_FORMAT_Z16:
      intel_InitDepthPointers_z16(rb);
      break;
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_S8_Z24:
      intel_InitDepthPointers_z24_x8(rb);
      break;
   default:
      _mesa_problem(NULL,
		    "Unexpected MesaFormat %d in intelSetSpanFunctions",
		    irb->Base.Format);
      break;
   }
}
