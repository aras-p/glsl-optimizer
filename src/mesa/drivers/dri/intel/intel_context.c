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
#include "main/context.h"
#include "main/extensions.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/imports.h"
#include "main/points.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"

#include "intel_chipset.h"
#include "intel_buffers.h"
#include "intel_tex.h"
#include "intel_batchbuffer.h"
#include "intel_clear.h"
#include "intel_extensions.h"
#include "intel_pixel.h"
#include "intel_regions.h"
#include "intel_buffer_objects.h"
#include "intel_fbo.h"
#include "intel_bufmgr.h"
#include "intel_screen.h"

#include "drirenderbuffer.h"
#include "utils.h"


#ifndef INTEL_DEBUG
int INTEL_DEBUG = (0);
#endif


static const GLubyte *
intelGetString(struct gl_context * ctx, GLenum name)
{
   const struct intel_context *const intel = intel_context(ctx);
   const char *chipset;
   static char buffer[128];

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *) "Tungsten Graphics, Inc";
      break;

   case GL_RENDERER:
      switch (intel->intelScreen->deviceID) {
      case PCI_CHIP_845_G:
         chipset = "Intel(R) 845G";
         break;
      case PCI_CHIP_I830_M:
         chipset = "Intel(R) 830M";
         break;
      case PCI_CHIP_I855_GM:
         chipset = "Intel(R) 852GM/855GM";
         break;
      case PCI_CHIP_I865_G:
         chipset = "Intel(R) 865G";
         break;
      case PCI_CHIP_I915_G:
         chipset = "Intel(R) 915G";
         break;
      case PCI_CHIP_E7221_G:
	 chipset = "Intel (R) E7221G (i915)";
	 break;
      case PCI_CHIP_I915_GM:
         chipset = "Intel(R) 915GM";
         break;
      case PCI_CHIP_I945_G:
         chipset = "Intel(R) 945G";
         break;
      case PCI_CHIP_I945_GM:
         chipset = "Intel(R) 945GM";
         break;
      case PCI_CHIP_I945_GME:
         chipset = "Intel(R) 945GME";
         break;
      case PCI_CHIP_G33_G:
	 chipset = "Intel(R) G33";
	 break;
      case PCI_CHIP_Q35_G:
	 chipset = "Intel(R) Q35";
	 break;
      case PCI_CHIP_Q33_G:
	 chipset = "Intel(R) Q33";
	 break;
      case PCI_CHIP_IGD_GM:
      case PCI_CHIP_IGD_G:
	 chipset = "Intel(R) IGD";
	 break;
      case PCI_CHIP_I965_Q:
	 chipset = "Intel(R) 965Q";
	 break;
      case PCI_CHIP_I965_G:
      case PCI_CHIP_I965_G_1:
	 chipset = "Intel(R) 965G";
	 break;
      case PCI_CHIP_I946_GZ:
	 chipset = "Intel(R) 946GZ";
	 break;
      case PCI_CHIP_I965_GM:
	 chipset = "Intel(R) 965GM";
	 break;
      case PCI_CHIP_I965_GME:
	 chipset = "Intel(R) 965GME/GLE";
	 break;
      case PCI_CHIP_GM45_GM:
	 chipset = "Mobile Intel® GM45 Express Chipset";
	 break; 
      case PCI_CHIP_IGD_E_G:
	 chipset = "Intel(R) Integrated Graphics Device";
	 break;
      case PCI_CHIP_G45_G:
         chipset = "Intel(R) G45/G43";
         break;
      case PCI_CHIP_Q45_G:
         chipset = "Intel(R) Q45/Q43";
         break;
      case PCI_CHIP_G41_G:
         chipset = "Intel(R) G41";
         break;
      case PCI_CHIP_B43_G:
      case PCI_CHIP_B43_G1:
         chipset = "Intel(R) B43";
         break;
      case PCI_CHIP_ILD_G:
         chipset = "Intel(R) Ironlake Desktop";
         break;
      case PCI_CHIP_ILM_G:
         chipset = "Intel(R) Ironlake Mobile";
         break;
      case PCI_CHIP_SANDYBRIDGE_GT1:
      case PCI_CHIP_SANDYBRIDGE_GT2:
      case PCI_CHIP_SANDYBRIDGE_GT2_PLUS:
	 chipset = "Intel(R) Sandybridge Desktop";
	 break;
      case PCI_CHIP_SANDYBRIDGE_M_GT1:
      case PCI_CHIP_SANDYBRIDGE_M_GT2:
      case PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS:
	 chipset = "Intel(R) Sandybridge Mobile";
	 break;
      case PCI_CHIP_SANDYBRIDGE_S:
	 chipset = "Intel(R) Sandybridge Server";
	 break;
      case PCI_CHIP_IVYBRIDGE_GT1:
      case PCI_CHIP_IVYBRIDGE_GT2:
	 chipset = "Intel(R) Ivybridge Desktop";
	 break;
      case PCI_CHIP_IVYBRIDGE_M_GT1:
      case PCI_CHIP_IVYBRIDGE_M_GT2:
	 chipset = "Intel(R) Ivybridge Mobile";
	 break;
      case PCI_CHIP_IVYBRIDGE_S_GT1:
	 chipset = "Intel(R) Ivybridge Server";
	 break;
      default:
         chipset = "Unknown Intel Chipset";
         break;
      }

      (void) driGetRendererString(buffer, chipset, 0);
      return (GLubyte *) buffer;

   default:
      return NULL;
   }
}

static void
intel_flush_front(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
    __DRIcontext *driContext = intel->driContext;
    __DRIscreen *const screen = intel->intelScreen->driScrnPriv;

   if ((ctx->DrawBuffer->Name == 0) && intel->front_buffer_dirty) {
      if (screen->dri2.loader &&
          (screen->dri2.loader->base.version >= 2)
	  && (screen->dri2.loader->flushFrontBuffer != NULL) &&
          driContext->driDrawablePriv &&
	  driContext->driDrawablePriv->loaderPrivate) {
	 (*screen->dri2.loader->flushFrontBuffer)(driContext->driDrawablePriv,
						  driContext->driDrawablePriv->loaderPrivate);

	 /* We set the dirty bit in intel_prepare_render() if we're
	  * front buffer rendering once we get there.
	  */
	 intel->front_buffer_dirty = GL_FALSE;
      }
   }
}

static unsigned
intel_bits_per_pixel(const struct intel_renderbuffer *rb)
{
   return _mesa_get_format_bytes(rb->Base.Format) * 8;
}

void
intel_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;
   struct intel_region *region, *depth_region;
   struct intel_context *intel = context->driverPrivate;
   struct intel_renderbuffer *front_rb, *back_rb, *depth_rb, *stencil_rb;
   __DRIbuffer *buffers = NULL;
   __DRIscreen *screen;
   int i, count;
   unsigned int attachments[10];
   const char *region_name;

   /* If we're rendering to the fake front buffer, make sure all the
    * pending drawing has landed on the real front buffer.  Otherwise
    * when we eventually get to DRI2GetBuffersWithFormat the stale
    * real front buffer contents will get copied to the new fake front
    * buffer.
    */
   if (intel->is_front_buffer_rendering) {
      intel_flush(&intel->ctx);
      intel_flush_front(&intel->ctx);
   }

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   screen = intel->intelScreen->driScrnPriv;

   if (screen->dri2.loader
       && (screen->dri2.loader->base.version > 2)
       && (screen->dri2.loader->getBuffersWithFormat != NULL)) {

      front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
      back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
      depth_rb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
      stencil_rb = intel_get_renderbuffer(fb, BUFFER_STENCIL);

      i = 0;
      if ((intel->is_front_buffer_rendering ||
	   intel->is_front_buffer_reading ||
	   !back_rb) && front_rb) {
	 attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
	 attachments[i++] = intel_bits_per_pixel(front_rb);
      }

      if (back_rb) {
	 attachments[i++] = __DRI_BUFFER_BACK_LEFT;
	 attachments[i++] = intel_bits_per_pixel(back_rb);
      }

      if ((depth_rb != NULL) && (stencil_rb != NULL)) {
	 attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
	 attachments[i++] = intel_bits_per_pixel(depth_rb);
      } else if (depth_rb != NULL) {
	 attachments[i++] = __DRI_BUFFER_DEPTH;
	 attachments[i++] = intel_bits_per_pixel(depth_rb);
      } else if (stencil_rb != NULL) {
	 attachments[i++] = __DRI_BUFFER_STENCIL;
	 attachments[i++] = intel_bits_per_pixel(stencil_rb);
      }

      buffers =
	 (*screen->dri2.loader->getBuffersWithFormat)(drawable,
						      &drawable->w,
						      &drawable->h,
						      attachments, i / 2,
						      &count,
						      drawable->loaderPrivate);
   } else if (screen->dri2.loader) {
      i = 0;
      if (intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT))
	 attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      if (intel_get_renderbuffer(fb, BUFFER_BACK_LEFT))
	 attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      if (intel_get_renderbuffer(fb, BUFFER_DEPTH))
	 attachments[i++] = __DRI_BUFFER_DEPTH;
      if (intel_get_renderbuffer(fb, BUFFER_STENCIL))
	 attachments[i++] = __DRI_BUFFER_STENCIL;

      buffers = (*screen->dri2.loader->getBuffers)(drawable,
						   &drawable->w,
						   &drawable->h,
						   attachments, i,
						   &count,
						   drawable->loaderPrivate);
   }

   if (buffers == NULL)
      return;

   drawable->x = 0;
   drawable->y = 0;
   drawable->backX = 0;
   drawable->backY = 0;
   drawable->numClipRects = 1;
   drawable->pClipRects[0].x1 = 0;
   drawable->pClipRects[0].y1 = 0;
   drawable->pClipRects[0].x2 = drawable->w;
   drawable->pClipRects[0].y2 = drawable->h;
   drawable->numBackClipRects = 1;
   drawable->pBackClipRects[0].x1 = 0;
   drawable->pBackClipRects[0].y1 = 0;
   drawable->pBackClipRects[0].x2 = drawable->w;
   drawable->pBackClipRects[0].y2 = drawable->h;

   depth_region = NULL;
   for (i = 0; i < count; i++) {
       switch (buffers[i].attachment) {
       case __DRI_BUFFER_FRONT_LEFT:
	   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
	   region_name = "dri2 front buffer";
	   break;

       case __DRI_BUFFER_FAKE_FRONT_LEFT:
	   rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
	   region_name = "dri2 fake front buffer";
	   break;

       case __DRI_BUFFER_BACK_LEFT:
	   rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
	   region_name = "dri2 back buffer";
	   break;

       case __DRI_BUFFER_DEPTH:
	   rb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
	   region_name = "dri2 depth buffer";
	   break;

       case __DRI_BUFFER_DEPTH_STENCIL:
	   rb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
	   region_name = "dri2 depth / stencil buffer";
	   break;

       case __DRI_BUFFER_STENCIL:
	   rb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
	   region_name = "dri2 stencil buffer";
	   break;

       case __DRI_BUFFER_ACCUM:
       default:
	   fprintf(stderr,
		   "unhandled buffer attach event, attachment type %d\n",
		   buffers[i].attachment);
	   return;
       }

       if (rb == NULL)
	  continue;

       if (rb->region && rb->region->name == buffers[i].name)
	     continue;

       if (unlikely(INTEL_DEBUG & DEBUG_DRI))
	  fprintf(stderr,
		  "attaching buffer %d, at %d, cpp %d, pitch %d\n",
		  buffers[i].name, buffers[i].attachment,
		  buffers[i].cpp, buffers[i].pitch);
       
       if (buffers[i].attachment == __DRI_BUFFER_STENCIL && depth_region) {
	  if (unlikely(INTEL_DEBUG & DEBUG_DRI))
	     fprintf(stderr, "(reusing depth buffer as stencil)\n");
	  intel_region_reference(&region, depth_region);
       }
       else
          region = intel_region_alloc_for_handle(intel->intelScreen,
						 buffers[i].cpp,
						 drawable->w,
						 drawable->h,
						 buffers[i].pitch / buffers[i].cpp,
						 buffers[i].name,
						 region_name);

       if (buffers[i].attachment == __DRI_BUFFER_DEPTH)
	  depth_region = region;

       intel_renderbuffer_set_region(intel, rb, region);
       intel_region_release(&region);

       if (buffers[i].attachment == __DRI_BUFFER_DEPTH_STENCIL) {
	  rb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
	  if (rb != NULL) {
	     struct intel_region *stencil_region = NULL;

	     if (rb->region && rb->region->name == buffers[i].name)
		   continue;

	     intel_region_reference(&stencil_region, region);
	     intel_renderbuffer_set_region(intel, rb, stencil_region);
	     intel_region_release(&stencil_region);
	  }
       }
   }

   driUpdateFramebufferSize(&intel->ctx, drawable);
}

/**
 * intel_prepare_render should be called anywhere that curent read/drawbuffer
 * state is required.
 */
void
intel_prepare_render(struct intel_context *intel)
{
   __DRIcontext *driContext = intel->driContext;
   __DRIdrawable *drawable;

   drawable = driContext->driDrawablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.draw_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
	 intel_update_renderbuffers(driContext, drawable);
      intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
      driContext->dri2.draw_stamp = drawable->dri2.stamp;
   }

   drawable = driContext->driReadablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.read_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
	 intel_update_renderbuffers(driContext, drawable);
      driContext->dri2.read_stamp = drawable->dri2.stamp;
   }

   /* If we're currently rendering to the front buffer, the rendering
    * that will happen next will probably dirty the front buffer.  So
    * mark it as dirty here.
    */
   if (intel->is_front_buffer_rendering)
      intel->front_buffer_dirty = GL_TRUE;

   /* Wait for the swapbuffers before the one we just emitted, so we
    * don't get too many swaps outstanding for apps that are GPU-heavy
    * but not CPU-heavy.
    *
    * We're using intelDRI2Flush (called from the loader before
    * swapbuffer) and glFlush (for front buffer rendering) as the
    * indicator that a frame is done and then throttle when we get
    * here as we prepare to render the next frame.  At this point for
    * round trips for swap/copy and getting new buffers are done and
    * we'll spend less time waiting on the GPU.
    *
    * Unfortunately, we don't have a handle to the batch containing
    * the swap, and getting our hands on that doesn't seem worth it,
    * so we just us the first batch we emitted after the last swap.
    */
   if (intel->need_throttle && intel->first_post_swapbuffers_batch) {
      drm_intel_bo_wait_rendering(intel->first_post_swapbuffers_batch);
      drm_intel_bo_unreference(intel->first_post_swapbuffers_batch);
      intel->first_post_swapbuffers_batch = NULL;
      intel->need_throttle = GL_FALSE;
   }
}

static void
intel_viewport(struct gl_context *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    struct intel_context *intel = intel_context(ctx);
    __DRIcontext *driContext = intel->driContext;

    if (intel->saved_viewport)
	intel->saved_viewport(ctx, x, y, w, h);

    if (ctx->DrawBuffer->Name == 0) {
       dri2InvalidateDrawable(driContext->driDrawablePriv);
       dri2InvalidateDrawable(driContext->driReadablePriv);
    }
}

static const struct dri_debug_control debug_control[] = {
   { "tex",   DEBUG_TEXTURE},
   { "state", DEBUG_STATE},
   { "ioctl", DEBUG_IOCTL},
   { "blit",  DEBUG_BLIT},
   { "mip",   DEBUG_MIPTREE},
   { "fall",  DEBUG_FALLBACKS},
   { "verb",  DEBUG_VERBOSE},
   { "bat",   DEBUG_BATCH},
   { "pix",   DEBUG_PIXEL},
   { "buf",   DEBUG_BUFMGR},
   { "reg",   DEBUG_REGION},
   { "fbo",   DEBUG_FBO},
   { "gs",    DEBUG_GS},
   { "sync",  DEBUG_SYNC},
   { "prim",  DEBUG_PRIMS },
   { "vert",  DEBUG_VERTS },
   { "dri",   DEBUG_DRI },
   { "sf",    DEBUG_SF },
   { "san",   DEBUG_SANITY },
   { "sleep", DEBUG_SLEEP },
   { "stats", DEBUG_STATS },
   { "tile",  DEBUG_TILE },
   { "sing",  DEBUG_SINGLE_THREAD },
   { "thre",  DEBUG_SINGLE_THREAD },
   { "wm",    DEBUG_WM },
   { "urb",   DEBUG_URB },
   { "vs",    DEBUG_VS },
   { "clip",  DEBUG_CLIP },
   { NULL,    0 }
};


static void
intelInvalidateState(struct gl_context * ctx, GLuint new_state)
{
    struct intel_context *intel = intel_context(ctx);

   _swrast_InvalidateState(ctx, new_state);
   _swsetup_InvalidateState(ctx, new_state);
   _vbo_InvalidateState(ctx, new_state);
   _tnl_InvalidateState(ctx, new_state);
   _tnl_invalidate_vertex_state(ctx, new_state);

   intel->NewGLState |= new_state;

   if (intel->vtbl.invalidate_state)
      intel->vtbl.invalidate_state( intel, new_state );
}

void
intel_flush(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   if (intel->Fallback)
      _swrast_flush(ctx);

   if (intel->gen < 4)
      INTEL_FIREVERTICES(intel);

   if (intel->batch.used)
      intel_batchbuffer_flush(intel);
}

static void
intel_glFlush(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush(ctx);
   intel_flush_front(ctx);
   if (intel->is_front_buffer_rendering)
      intel->need_throttle = GL_TRUE;
}

void
intelFinish(struct gl_context * ctx)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   int i;

   intel_flush(ctx);
   intel_flush_front(ctx);

   for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
       struct intel_renderbuffer *irb;

       irb = intel_renderbuffer(fb->_ColorDrawBuffers[i]);

       if (irb && irb->region && irb->region->buffer)
	  drm_intel_bo_wait_rendering(irb->region->buffer);
   }
   if (fb->_DepthBuffer) {
      /* XXX: Wait on buffer idle */
   }
}

void
intelInitDriverFunctions(struct dd_function_table *functions)
{
   _mesa_init_driver_functions(functions);

   functions->Flush = intel_glFlush;
   functions->Finish = intelFinish;
   functions->GetString = intelGetString;
   functions->UpdateState = intelInvalidateState;

   intelInitTextureFuncs(functions);
   intelInitTextureImageFuncs(functions);
   intelInitTextureSubImageFuncs(functions);
   intelInitTextureCopyImageFuncs(functions);
   intelInitStateFuncs(functions);
   intelInitClearFuncs(functions);
   intelInitBufferFuncs(functions);
   intelInitPixelFuncs(functions);
   intelInitBufferObjectFuncs(functions);
   intel_init_syncobj_functions(functions);
}


GLboolean
intelInitContext(struct intel_context *intel,
		 int api,
                 const struct gl_config * mesaVis,
                 __DRIcontext * driContextPriv,
                 void *sharedContextPrivate,
                 struct dd_function_table *functions)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_context *shareCtx = (struct gl_context *) sharedContextPrivate;
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct intel_screen *intelScreen = sPriv->private;
   int bo_reuse_mode;
   struct gl_config visual;

   /* we can't do anything without a connection to the device */
   if (intelScreen->bufmgr == NULL)
      return GL_FALSE;

   /* Can't rely on invalidate events, fall back to glViewport hack */
   if (!driContextPriv->driScreenPriv->dri2.useInvalidate) {
      intel->saved_viewport = functions->Viewport;
      functions->Viewport = intel_viewport;
   }

   if (mesaVis == NULL) {
      memset(&visual, 0, sizeof visual);
      mesaVis = &visual;
   }

   if (!_mesa_initialize_context(&intel->ctx, api, mesaVis, shareCtx,
                                 functions, (void *) intel)) {
      printf("%s: failed to init mesa context\n", __FUNCTION__);
      return GL_FALSE;
   }

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driContext = driContextPriv;
   intel->driFd = sPriv->fd;

   intel->has_xrgb_textures = GL_TRUE;
   intel->gen = intelScreen->gen;
   if (IS_GEN7(intel->intelScreen->deviceID)) {
      intel->needs_ff_sync = GL_TRUE;
      intel->has_luminance_srgb = GL_TRUE;
   } else if (IS_GEN6(intel->intelScreen->deviceID)) {
      intel->needs_ff_sync = GL_TRUE;
      intel->has_luminance_srgb = GL_TRUE;
   } else if (IS_GEN5(intel->intelScreen->deviceID)) {
      intel->needs_ff_sync = GL_TRUE;
      intel->has_luminance_srgb = GL_TRUE;
   } else if (IS_965(intel->intelScreen->deviceID)) {
      if (IS_G4X(intel->intelScreen->deviceID)) {
	  intel->has_luminance_srgb = GL_TRUE;
	  intel->is_g4x = GL_TRUE;
      }
   } else if (IS_9XX(intel->intelScreen->deviceID)) {
      if (IS_945(intel->intelScreen->deviceID)) {
	 intel->is_945 = GL_TRUE;
      }
   } else {
      if (intel->intelScreen->deviceID == PCI_CHIP_I830_M ||
	  intel->intelScreen->deviceID == PCI_CHIP_845_G) {
	 intel->has_xrgb_textures = GL_FALSE;
      }
   }

   memset(&ctx->TextureFormatSupported, 0,
	  sizeof(ctx->TextureFormatSupported));
   ctx->TextureFormatSupported[MESA_FORMAT_ARGB8888] = GL_TRUE;
   if (intel->has_xrgb_textures)
      ctx->TextureFormatSupported[MESA_FORMAT_XRGB8888] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_ARGB4444] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_ARGB1555] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RGB565] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_L8] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_A8] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_I8] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_AL88] = GL_TRUE;
   if (intel->gen >= 4)
      ctx->TextureFormatSupported[MESA_FORMAT_AL1616] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_S8_Z24] = GL_TRUE;
   /*
    * This was disabled in initial FBO enabling to avoid combinations
    * of depth+stencil that wouldn't work together.  We since decided
    * that it was OK, since it's up to the app to come up with the
    * combo that actually works, so this can probably be re-enabled.
    */
   /*
   ctx->TextureFormatSupported[MESA_FORMAT_Z16] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_Z24] = GL_TRUE;
   */

   /* ctx->Extensions.MESA_ycbcr_texture */
   ctx->TextureFormatSupported[MESA_FORMAT_YCBCR] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_YCBCR_REV] = GL_TRUE;

   /* GL_3DFX_texture_compression_FXT1 */
   ctx->TextureFormatSupported[MESA_FORMAT_RGB_FXT1] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_FXT1] = GL_TRUE;

   /* GL_EXT_texture_compression_s3tc */
   ctx->TextureFormatSupported[MESA_FORMAT_RGB_DXT1] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_DXT1] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_DXT3] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_DXT5] = GL_TRUE;

#ifndef I915
   /* GL_ARB_texture_compression_rgtc */
   ctx->TextureFormatSupported[MESA_FORMAT_RED_RGTC1] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SIGNED_RED_RGTC1] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RG_RGTC2] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SIGNED_RG_RGTC2] = GL_TRUE;

   /* GL_ARB_texture_rg */
   ctx->TextureFormatSupported[MESA_FORMAT_R8] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_R16] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RG88] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RG1616] = GL_TRUE;

   /* GL_MESA_texture_signed_rgba / GL_EXT_texture_snorm */
   ctx->TextureFormatSupported[MESA_FORMAT_DUDV8] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SIGNED_RGBA8888_REV] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SIGNED_R8] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SIGNED_RG88_REV] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SIGNED_R16] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SIGNED_GR1616] = GL_TRUE;

   /* GL_EXT_texture_sRGB */
   ctx->TextureFormatSupported[MESA_FORMAT_SARGB8] = GL_TRUE;
   if (intel->gen >= 5 || intel->is_g4x)
      ctx->TextureFormatSupported[MESA_FORMAT_SRGB_DXT1] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SRGBA_DXT1] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SRGBA_DXT3] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_SRGBA_DXT5] = GL_TRUE;
   if (intel->has_luminance_srgb) {
      ctx->TextureFormatSupported[MESA_FORMAT_SL8] = GL_TRUE;
      ctx->TextureFormatSupported[MESA_FORMAT_SLA8] = GL_TRUE;
   }

#ifdef TEXTURE_FLOAT_ENABLED
   ctx->TextureFormatSupported[MESA_FORMAT_RGBA_FLOAT32] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_RG_FLOAT32] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_R_FLOAT32] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_INTENSITY_FLOAT32] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_LUMINANCE_FLOAT32] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_ALPHA_FLOAT32] = GL_TRUE;
   ctx->TextureFormatSupported[MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32] = GL_TRUE;
#endif

#endif /* !I915 */

   driParseConfigFiles(&intel->optionCache, &intelScreen->optionCache,
                       sPriv->myNum, (intel->gen >= 4) ? "i965" : "i915");
   if (intel->gen < 4)
      intel->maxBatchSize = 4096;
   else
      intel->maxBatchSize = sizeof(intel->batch.map);

   intel->bufmgr = intelScreen->bufmgr;

   bo_reuse_mode = driQueryOptioni(&intel->optionCache, "bo_reuse");
   switch (bo_reuse_mode) {
   case DRI_CONF_BO_REUSE_DISABLED:
      break;
   case DRI_CONF_BO_REUSE_ALL:
      intel_bufmgr_gem_enable_reuse(intel->bufmgr);
      break;
   }

   /* This doesn't yet catch all non-conformant rendering, but it's a
    * start.
    */
   if (getenv("INTEL_STRICT_CONFORMANCE")) {
      unsigned int value = atoi(getenv("INTEL_STRICT_CONFORMANCE"));
      if (value > 0) {
         intel->conformance_mode = value;
      }
      else {
         intel->conformance_mode = 1;
      }
   }

   if (intel->conformance_mode > 0) {
      ctx->Const.MinLineWidth = 1.0;
      ctx->Const.MinLineWidthAA = 1.0;
      ctx->Const.MaxLineWidth = 1.0;
      ctx->Const.MaxLineWidthAA = 1.0;
      ctx->Const.LineWidthGranularity = 1.0;
   }
   else {
      ctx->Const.MinLineWidth = 1.0;
      ctx->Const.MinLineWidthAA = 1.0;
      ctx->Const.MaxLineWidth = 5.0;
      ctx->Const.MaxLineWidthAA = 5.0;
      ctx->Const.LineWidthGranularity = 0.5;
   }

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 3.0;
   ctx->Const.PointSizeGranularity = 1.0;

   ctx->Const.MaxSamples = 1.0;

   /* reinitialize the context point state.
    * It depend on constants in __struct gl_contextRec::Const
    */
   _mesa_init_point(ctx);

   if (intel->gen >= 4) {
      ctx->Const.sRGBCapable = GL_TRUE;
      if (MAX_WIDTH > 8192)
	 ctx->Const.MaxRenderbufferSize = 8192;
   } else {
      if (MAX_WIDTH > 2048)
	 ctx->Const.MaxRenderbufferSize = 2048;
   }

   /* Initialize the software rasterizer and helper modules. */
   _swrast_CreateContext(ctx);
   _vbo_CreateContext(ctx);
   _tnl_CreateContext(ctx);
   _swsetup_CreateContext(ctx);
 
   /* Configure swrast to match hardware characteristics: */
   _swrast_allow_pixel_fog(ctx, GL_FALSE);
   _swrast_allow_vertex_fog(ctx, GL_TRUE);

   _mesa_meta_init(ctx);

   intel->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;
   intel->hw_stipple = 1;

   /* XXX FBO: this doesn't seem to be used anywhere */
   switch (mesaVis->depthBits) {
   case 0:                     /* what to do in this case? */
   case 16:
      intel->polygon_offset_scale = 1.0;
      break;
   case 24:
      intel->polygon_offset_scale = 2.0;     /* req'd to pass glean */
      break;
   default:
      assert(0);
      break;
   }

   if (intel->gen >= 4)
      intel->polygon_offset_scale /= 0xffff;

   intel->RenderIndex = ~0;

   switch (ctx->API) {
   case API_OPENGL:
      intelInitExtensions(ctx);
      break;
   case API_OPENGLES:
      break;
   case API_OPENGLES2:
      intelInitExtensionsES2(ctx);
      break;
   }

   INTEL_DEBUG = driParseDebugString(getenv("INTEL_DEBUG"), debug_control);
   if (INTEL_DEBUG & DEBUG_BUFMGR)
      dri_bufmgr_set_debug(intel->bufmgr, GL_TRUE);

   intel_batchbuffer_reset(intel);

   intel_fbo_init(intel);

   if (intel->ctx.Mesa_DXTn) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
      _mesa_enable_extension(ctx, "GL_S3_s3tc");
   }
   else if (driQueryOptionb(&intel->optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
   }
   intel->use_texture_tiling = driQueryOptionb(&intel->optionCache,
					       "texture_tiling");
   intel->use_early_z = driQueryOptionb(&intel->optionCache, "early_z");

   intel->prim.primitive = ~0;

   /* Force all software fallbacks */
   if (driQueryOptionb(&intel->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D rasterization\n");
      intel->no_rast = 1;
   }

   if (driQueryOptionb(&intel->optionCache, "always_flush_batch")) {
      fprintf(stderr, "flushing batchbuffer before/after each draw call\n");
      intel->always_flush_batch = 1;
   }

   if (driQueryOptionb(&intel->optionCache, "always_flush_cache")) {
      fprintf(stderr, "flushing GPU caches before/after each draw call\n");
      intel->always_flush_cache = 1;
   }

   return GL_TRUE;
}

void
intelDestroyContext(__DRIcontext * driContextPriv)
{
   struct intel_context *intel =
      (struct intel_context *) driContextPriv->driverPrivate;

   assert(intel);               /* should never be null */
   if (intel) {
      INTEL_FIREVERTICES(intel);

      _mesa_meta_free(&intel->ctx);

      intel->vtbl.destroy(intel);

      _swsetup_DestroyContext(&intel->ctx);
      _tnl_DestroyContext(&intel->ctx);
      _vbo_DestroyContext(&intel->ctx);

      _swrast_DestroyContext(&intel->ctx);
      intel->Fallback = 0x0;      /* don't call _swrast_Flush later */

      intel_batchbuffer_free(intel);

      free(intel->prim.vb);
      intel->prim.vb = NULL;
      drm_intel_bo_unreference(intel->prim.vb_bo);
      intel->prim.vb_bo = NULL;
      drm_intel_bo_unreference(intel->first_post_swapbuffers_batch);
      intel->first_post_swapbuffers_batch = NULL;

      driDestroyOptionCache(&intel->optionCache);

      /* free the Mesa context */
      _mesa_free_context_data(&intel->ctx);

      FREE(intel);
      driContextPriv->driverPrivate = NULL;
   }
}

GLboolean
intelUnbindContext(__DRIcontext * driContextPriv)
{
   /* Unset current context and dispath table */
   _mesa_make_current(NULL, NULL, NULL);

   return GL_TRUE;
}

GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv)
{
   struct intel_context *intel;
   GET_CURRENT_CONTEXT(curCtx);

   if (driContextPriv)
      intel = (struct intel_context *) driContextPriv->driverPrivate;
   else
      intel = NULL;

   /* According to the glXMakeCurrent() man page: "Pending commands to
    * the previous context, if any, are flushed before it is released."
    * But only flush if we're actually changing contexts.
    */
   if (intel_context(curCtx) && intel_context(curCtx) != intel) {
      _mesa_flush(curCtx);
   }

   if (driContextPriv) {
      struct gl_framebuffer *fb, *readFb;
      
      if (driDrawPriv == NULL && driReadPriv == NULL) {
	 fb = _mesa_get_incomplete_framebuffer();
	 readFb = _mesa_get_incomplete_framebuffer();
      } else {
	 fb = driDrawPriv->driverPrivate;
	 readFb = driReadPriv->driverPrivate;
	 driContextPriv->dri2.draw_stamp = driDrawPriv->dri2.stamp - 1;
	 driContextPriv->dri2.read_stamp = driReadPriv->dri2.stamp - 1;
      }

      intel_prepare_render(intel);
      _mesa_make_current(&intel->ctx, fb, readFb);
      
      /* We do this in intel_prepare_render() too, but intel->ctx.DrawBuffer
       * is NULL at that point.  We can't call _mesa_makecurrent()
       * first, since we need the buffer size for the initial
       * viewport.  So just call intel_draw_buffer() again here. */
      intel_draw_buffer(&intel->ctx, intel->ctx.DrawBuffer);
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}
