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
#include "main/renderbuffer.h"

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
#include "intel_mipmap_tree.h"

#include "utils.h"
#include "../glsl/ralloc.h"

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
	 intel->front_buffer_dirty = false;
      }
   }
}

static unsigned
intel_bits_per_pixel(const struct intel_renderbuffer *rb)
{
   return _mesa_get_format_bytes(intel_rb_format(rb)) * 8;
}

static void
intel_query_dri2_buffers_no_separate_stencil(struct intel_context *intel,
					     __DRIdrawable *drawable,
					     __DRIbuffer **buffers,
					     int *count);

static void
intel_process_dri2_buffer_no_separate_stencil(struct intel_context *intel,
					      __DRIdrawable *drawable,
					      __DRIbuffer *buffer,
					      struct intel_renderbuffer *rb,
					      const char *buffer_name);

static void
intel_query_dri2_buffers_with_separate_stencil(struct intel_context *intel,
					       __DRIdrawable *drawable,
					       __DRIbuffer **buffers,
					       unsigned **attachments,
					       int *count);

static void
intel_process_dri2_buffer_with_separate_stencil(struct intel_context *intel,
						__DRIdrawable *drawable,
						__DRIbuffer *buffer,
						struct intel_renderbuffer *rb,
						const char *buffer_name);
static void
intel_verify_dri2_has_hiz(struct intel_context *intel,
			  __DRIdrawable *drawable,
			  __DRIbuffer **buffers,
			  unsigned **attachments,
			  int *count);

void
intel_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;
   struct intel_context *intel = context->driverPrivate;
   __DRIbuffer *buffers = NULL;
   unsigned *attachments = NULL;
   int i, count;
   const char *region_name;

   bool try_separate_stencil =
      intel->has_separate_stencil &&
      intel->intelScreen->dri2_has_hiz != INTEL_DRI2_HAS_HIZ_FALSE &&
      intel->intelScreen->driScrnPriv->dri2.loader != NULL &&
      intel->intelScreen->driScrnPriv->dri2.loader->base.version > 2 &&
      intel->intelScreen->driScrnPriv->dri2.loader->getBuffersWithFormat != NULL;

   assert(!intel->must_use_separate_stencil || try_separate_stencil);

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

   if (try_separate_stencil) {
      intel_query_dri2_buffers_with_separate_stencil(intel, drawable, &buffers,
						     &attachments, &count);
   } else {
      intel_query_dri2_buffers_no_separate_stencil(intel, drawable, &buffers,
						   &count);
   }

   if (buffers == NULL)
      return;

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

       case __DRI_BUFFER_HIZ:
	   /* The hiz region resides in the depth renderbuffer. */
	   rb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
	   region_name = "dri2 hiz buffer";
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

       if (try_separate_stencil) {
	 intel_process_dri2_buffer_with_separate_stencil(intel, drawable,
						         &buffers[i], rb,
						         region_name);
       } else {
	 intel_process_dri2_buffer_no_separate_stencil(intel, drawable,
						       &buffers[i], rb,
						       region_name);
       }
   }

   if (try_separate_stencil
       && intel->intelScreen->dri2_has_hiz == INTEL_DRI2_HAS_HIZ_UNKNOWN) {
      intel_verify_dri2_has_hiz(intel, drawable, &buffers, &attachments,
				&count);
   }

   if (attachments)
      free(attachments);

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
      intel_draw_buffer(&intel->ctx);
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
      intel->front_buffer_dirty = true;

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
      intel->need_throttle = false;
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
   _vbo_InvalidateState(ctx, new_state);

   intel->NewGLState |= new_state;

   if (intel->vtbl.invalidate_state)
      intel->vtbl.invalidate_state( intel, new_state );
}

void
intel_flush_rendering_to_batch(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   if (intel->Fallback)
      _swrast_flush(ctx);

   if (intel->gen < 4)
      INTEL_FIREVERTICES(intel);
}

void
_intel_flush(struct gl_context *ctx, const char *file, int line)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush_rendering_to_batch(ctx);

   if (intel->batch.used)
      _intel_batchbuffer_flush(intel, file, line);
}

static void
intel_glFlush(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush(ctx);
   intel_flush_front(ctx);
   if (intel->is_front_buffer_rendering)
      intel->need_throttle = true;
}

void
intelFinish(struct gl_context * ctx)
{
   struct intel_context *intel = intel_context(ctx);

   intel_flush(ctx);
   intel_flush_front(ctx);

   if (intel->batch.last_bo)
      drm_intel_bo_wait_rendering(intel->batch.last_bo);
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

bool
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
   struct intel_screen *intelScreen = sPriv->driverPrivate;
   int bo_reuse_mode;
   struct gl_config visual;

   /* we can't do anything without a connection to the device */
   if (intelScreen->bufmgr == NULL)
      return false;

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
      return false;
   }

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driContext = driContextPriv;
   intel->driFd = sPriv->fd;

   intel->gen = intelScreen->gen;

   const int devID = intelScreen->deviceID;

   if (IS_SNB_GT1(devID) || IS_IVB_GT1(devID))
      intel->gt = 1;
   else if (IS_SNB_GT2(devID) || IS_IVB_GT2(devID))
      intel->gt = 2;
   else
      intel->gt = 0;

   if (IS_G4X(devID)) {
      intel->is_g4x = true;
   } else if (IS_945(devID)) {
      intel->is_945 = true;
   }

   if (intel->gen >= 5) {
      intel->needs_ff_sync = true;
   }

   intel->has_separate_stencil = intel->intelScreen->hw_has_separate_stencil;
   intel->must_use_separate_stencil = intel->intelScreen->hw_must_use_separate_stencil;
   intel->has_hiz = intel->intelScreen->hw_has_hiz;

   memset(&ctx->TextureFormatSupported,
	  0, sizeof(ctx->TextureFormatSupported));

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

   if (intel->gen >= 6)
      ctx->Const.MaxClipPlanes = 8;

   ctx->Const.StripTextureBorder = GL_TRUE;

   /* reinitialize the context point state.
    * It depend on constants in __struct gl_contextRec::Const
    */
   _mesa_init_point(ctx);

   if (intel->gen >= 4) {
      ctx->Const.sRGBCapable = true;
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
   _swrast_allow_pixel_fog(ctx, false);
   _swrast_allow_vertex_fog(ctx, true);

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
      intelInitExtensionsES1(ctx);
      break;
   case API_OPENGLES2:
      intelInitExtensionsES2(ctx);
      break;
   }

   INTEL_DEBUG = driParseDebugString(getenv("INTEL_DEBUG"), debug_control);
   if (INTEL_DEBUG & DEBUG_BUFMGR)
      dri_bufmgr_set_debug(intel->bufmgr, true);

   intel_batchbuffer_init(intel);

   intel_fbo_init(intel);

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

   return true;
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

      _math_matrix_dtr(&intel->ViewportMatrix);

      ralloc_free(intel);
      driContextPriv->driverPrivate = NULL;
   }
}

GLboolean
intelUnbindContext(__DRIcontext * driContextPriv)
{
   /* Unset current context and dispath table */
   _mesa_make_current(NULL, NULL, NULL);

   return true;
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
      intel_draw_buffer(&intel->ctx);
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return true;
}

/**
 * \brief Query DRI2 to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers with
 * DRI2GetBuffers() or DRI2GetBuffersWithFormat().
 *
 * This is called from intel_update_renderbuffers(). It is used only if either
 * the hardware or the X driver lacks separate stencil support.
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see intel_update_renderbuffers()
 * \see DRI2GetBuffers()
 * \see DRI2GetBuffersWithFormat()
 */
static void
intel_query_dri2_buffers_no_separate_stencil(struct intel_context *intel,
					     __DRIdrawable *drawable,
					     __DRIbuffer **buffers,
					     int *buffer_count)
{
   assert(!intel->must_use_separate_stencil);

   __DRIscreen *screen = intel->intelScreen->driScrnPriv;
   struct gl_framebuffer *fb = drawable->driverPrivate;

   if (screen->dri2.loader
       && screen->dri2.loader->base.version > 2
       && screen->dri2.loader->getBuffersWithFormat != NULL) {

      int i = 0;
      const int max_attachments = 4;
      unsigned *attachments = calloc(2 * max_attachments, sizeof(unsigned));

      struct intel_renderbuffer *front_rb;
      struct intel_renderbuffer *back_rb;
      struct intel_renderbuffer *depth_rb;
      struct intel_renderbuffer *stencil_rb;

      front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
      back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
      depth_rb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
      stencil_rb = intel_get_renderbuffer(fb, BUFFER_STENCIL);

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

      if (depth_rb && stencil_rb) {
	 attachments[i++] = __DRI_BUFFER_DEPTH_STENCIL;
	 attachments[i++] = intel_bits_per_pixel(depth_rb);
      } else if (depth_rb) {
	 attachments[i++] = __DRI_BUFFER_DEPTH;
	 attachments[i++] = intel_bits_per_pixel(depth_rb);
      } else if (stencil_rb) {
	 attachments[i++] = __DRI_BUFFER_STENCIL;
	 attachments[i++] = intel_bits_per_pixel(stencil_rb);
      }

      assert(i <= 2 * max_attachments);

      *buffers = screen->dri2.loader->getBuffersWithFormat(drawable,
							   &drawable->w,
							   &drawable->h,
							   attachments, i / 2,
							   buffer_count,
							   drawable->loaderPrivate);
      free(attachments);

   } else if (screen->dri2.loader) {

      int i = 0;
      const int max_attachments = 4;
      unsigned *attachments = calloc(max_attachments, sizeof(unsigned));

      if (intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT))
	 attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      if (intel_get_renderbuffer(fb, BUFFER_BACK_LEFT))
	 attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      if (intel_get_renderbuffer(fb, BUFFER_DEPTH))
	 attachments[i++] = __DRI_BUFFER_DEPTH;
      if (intel_get_renderbuffer(fb, BUFFER_STENCIL))
	 attachments[i++] = __DRI_BUFFER_STENCIL;

      assert(i <= max_attachments);

      *buffers = screen->dri2.loader->getBuffersWithFormat(drawable,
							   &drawable->w,
							   &drawable->h,
							   attachments, i,
							   buffer_count,
							   drawable->loaderPrivate);
      free(attachments);

   } else {
      *buffers = NULL;
      *buffer_count = 0;
   }
}

/**
 * \brief Assign a DRI buffer's DRM region to a renderbuffer.
 *
 * This is called from intel_update_renderbuffers().  It is used only if
 * either the hardware or the X driver lacks separate stencil support.
 *
 * \par Note:
 *    DRI buffers whose attachment point is DRI2BufferStencil or
 *    DRI2BufferDepthStencil are handled as special cases.
 *
 * \param buffer_name is a human readable name, such as "dri2 front buffer",
 *        that is passed to intel_region_alloc_for_handle().
 *
 * \see intel_update_renderbuffers()
 * \see intel_region_alloc_for_handle()
 */
static void
intel_process_dri2_buffer_no_separate_stencil(struct intel_context *intel,
					      __DRIdrawable *drawable,
					      __DRIbuffer *buffer,
					      struct intel_renderbuffer *rb,
					      const char *buffer_name)
{
   assert(!intel->must_use_separate_stencil);

   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *depth_rb = NULL;

   if (!rb)
      return;

   if (rb->mt &&
       rb->mt->region &&
       rb->mt->region->name == buffer->name)
      return;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI)) {
      fprintf(stderr,
	      "attaching buffer %d, at %d, cpp %d, pitch %d\n",
	      buffer->name, buffer->attachment,
	      buffer->cpp, buffer->pitch);
   }

   bool identify_depth_and_stencil = false;
   if (buffer->attachment == __DRI_BUFFER_STENCIL) {
      struct intel_renderbuffer *depth_rb =
	 intel_get_renderbuffer(fb, BUFFER_DEPTH);
      identify_depth_and_stencil = depth_rb && depth_rb->mt;
   }

   if (identify_depth_and_stencil) {
      if (unlikely(INTEL_DEBUG & DEBUG_DRI)) {
	 fprintf(stderr, "(reusing depth buffer as stencil)\n");
      }
      intel_miptree_reference(&rb->mt, depth_rb->mt);
   } else {
      intel_miptree_release(&rb->mt);
      struct intel_region *region =
                   intel_region_alloc_for_handle(intel->intelScreen,
						 buffer->cpp,
						 drawable->w,
						 drawable->h,
						 buffer->pitch / buffer->cpp,
						 buffer->name,
						 buffer_name);
      if (!region)
	 return;

      rb->mt = intel_miptree_create_for_region(intel,
                                               GL_TEXTURE_2D,
                                               intel_rb_format(rb),
                                               region);
      intel_region_release(&region);
      if (!rb->mt)
	 return;
   }

   if (buffer->attachment == __DRI_BUFFER_DEPTH_STENCIL) {
      struct intel_renderbuffer *stencil_rb =
	 intel_get_renderbuffer(fb, BUFFER_STENCIL);

      if (!stencil_rb)
	 return;

      /* The rb passed in is the BUFFER_DEPTH attachment, and we need
       * to associate this region to BUFFER_STENCIL as well.
       */
      intel_miptree_reference(&stencil_rb->mt, rb->mt);
   }
}

/**
 * \brief Query DRI2 to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers with
 * DRI2GetBuffersWithFormat().
 *
 * This is called from intel_update_renderbuffers(). It is used when 1) the
 * hardware supports separate stencil and 2) the X driver's separate stencil
 * support has been verified to work or is still unknown.
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 * \param attachments   [out] List of pairs (attachment_point, bits_per_pixel)
 *                      that were submitted in the DRI2 query. Number of pairs
 *                      is same as buffer_count.
 *
 * \see intel_update_renderbuffers()
 * \see DRI2GetBuffersWithFormat()
 * \see enum intel_dri2_has_hiz
 */
static void
intel_query_dri2_buffers_with_separate_stencil(struct intel_context *intel,
					       __DRIdrawable *drawable,
					       __DRIbuffer **buffers,
					       unsigned **attachments,
					       int *count)
{
   assert(intel->has_separate_stencil);

   __DRIscreen *screen = intel->intelScreen->driScrnPriv;
   struct gl_framebuffer *fb = drawable->driverPrivate;

   const int max_attachments = 5;
   int i = 0;

   *attachments = calloc(2 * max_attachments, sizeof(unsigned));
   if (!*attachments) {
      *buffers = NULL;
      *count = 0;
      return;
   }

   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;
   struct intel_renderbuffer *depth_rb;
   struct intel_renderbuffer *stencil_rb;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
   depth_rb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   stencil_rb = intel_get_renderbuffer(fb, BUFFER_STENCIL);

   if ((intel->is_front_buffer_rendering ||
	intel->is_front_buffer_reading ||
	!back_rb) && front_rb) {
      (*attachments)[i++] = __DRI_BUFFER_FRONT_LEFT;
      (*attachments)[i++] = intel_bits_per_pixel(front_rb);
   }

   if (back_rb) {
      (*attachments)[i++] = __DRI_BUFFER_BACK_LEFT;
      (*attachments)[i++] = intel_bits_per_pixel(back_rb);
   }

   /*
    * We request a separate stencil buffer, and perhaps a hiz buffer too, even
    * if we do not yet know if the X driver supports it. See the comments for
    * 'enum intel_dri2_has_hiz'.
    */

   if (depth_rb) {
      (*attachments)[i++] = __DRI_BUFFER_DEPTH;
      (*attachments)[i++] = intel_bits_per_pixel(depth_rb);

      if (intel->vtbl.is_hiz_depth_format(intel, intel_rb_format(depth_rb))) {
	 /* Depth and hiz buffer have same bpp. */
	 (*attachments)[i++] = __DRI_BUFFER_HIZ;
	 (*attachments)[i++] = intel_bits_per_pixel(depth_rb);
      }
   }

   if (stencil_rb) {
      assert(intel_rb_format(stencil_rb) == MESA_FORMAT_S8);
      (*attachments)[i++] = __DRI_BUFFER_STENCIL;
      (*attachments)[i++] = intel_bits_per_pixel(stencil_rb);
   }

   assert(i <= 2 * max_attachments);

   *buffers = screen->dri2.loader->getBuffersWithFormat(drawable,
							&drawable->w,
							&drawable->h,
							*attachments, i / 2,
							count,
							drawable->loaderPrivate);

   if (!*buffers) {
      free(*attachments);
      *attachments = NULL;
      *count = 0;
   }
}

/**
 * \brief Assign a DRI buffer's DRM region to a renderbuffer.
 *
 * This is called from intel_update_renderbuffers().  It is used when 1) the
 * hardware supports separate stencil and 2) the X driver's separate stencil
 * support has been verified to work or is still unknown.
 *
 * \par Note:
 *    DRI buffers whose attachment point is DRI2BufferStencil or DRI2BufferHiz
 *    are handled as special cases.
 *
 * \param buffer_name is a human readable name, such as "dri2 front buffer",
 *        that is passed to intel_region_alloc_for_handle().
 *
 * \see intel_update_renderbuffers()
 * \see intel_region_alloc_for_handle()
 * \see enum intel_dri2_has_hiz
 */
static void
intel_process_dri2_buffer_with_separate_stencil(struct intel_context *intel,
						__DRIdrawable *drawable,
						__DRIbuffer *buffer,
						struct intel_renderbuffer *rb,
						const char *buffer_name)
{
   assert(intel->has_separate_stencil);
   assert(buffer->attachment != __DRI_BUFFER_DEPTH_STENCIL);

   if (!rb)
      return;

   /* If the renderbuffer's and DRIbuffer's regions match, then continue. */
   if ((buffer->attachment != __DRI_BUFFER_HIZ &&
	rb->mt &&
	rb->mt->region &&
	rb->mt->region->name == buffer->name) ||
       (buffer->attachment == __DRI_BUFFER_HIZ &&
	rb->mt &&
	rb->mt->hiz_mt &&
	rb->mt->hiz_mt->region->name == buffer->name)) {
      return;
   }

   if (unlikely(INTEL_DEBUG & DEBUG_DRI)) {
      fprintf(stderr,
	      "attaching buffer %d, at %d, cpp %d, pitch %d\n",
	      buffer->name, buffer->attachment,
	      buffer->cpp, buffer->pitch);
   }

   int buffer_width;
   int buffer_height;
   if (buffer->attachment == __DRI_BUFFER_STENCIL) {
      /* The stencil buffer has quirky pitch requirements.  From Section
       * 2.11.5.6.2.1 3DSTATE_STENCIL_BUFFER, field "Surface Pitch":
       *    The pitch must be set to 2x the value computed based on width, as
       *    the stencil buffer is stored with two rows interleaved.
       *
       * To satisfy the pitch requirement, the X driver allocated the region
       * with the following dimensions.
       */
       buffer_width = ALIGN(drawable->w, 64);
       buffer_height = ALIGN(ALIGN(drawable->h, 2) / 2, 64);
   } else {
       buffer_width = drawable->w;
       buffer_height = drawable->h;
   }

   /* Release the buffer storage now in case we have to return early
    * due to failure to allocate new storage.
    */
   if (buffer->attachment == __DRI_BUFFER_HIZ) {
      intel_miptree_release(&rb->mt->hiz_mt);
   } else {
      intel_miptree_release(&rb->mt);
   }

   struct intel_region *region =
      intel_region_alloc_for_handle(intel->intelScreen,
				    buffer->cpp,
				    buffer_width,
				    buffer_height,
				    buffer->pitch / buffer->cpp,
				    buffer->name,
				    buffer_name);
   if (!region)
      return;

   struct intel_mipmap_tree *mt =
      intel_miptree_create_for_region(intel,
				      GL_TEXTURE_2D,
				      intel_rb_format(rb),
				      region);
   intel_region_release(&region);

   /* Associate buffer with new storage. */
   if (buffer->attachment == __DRI_BUFFER_HIZ) {
      rb->mt->hiz_mt = mt;
   } else {
      rb->mt = mt;
   }
}

/**
 * \brief Verify that the X driver supports hiz and separate stencil.
 *
 * This implements the cleanup stage of the handshake described in the
 * comments for 'enum intel_dri2_has_hiz'.
 *
 * This should be called from intel_update_renderbuffers() after 1) the
 * DRIdrawable has been queried for its buffers via DRI2GetBuffersWithFormat()
 * and 2) the DRM region of each returned DRIbuffer has been assigned to the
 * appropriate intel_renderbuffer. Furthermore, this should be called *only*
 * when 1) intel_update_renderbuffers() tried to used the X driver's separate
 * stencil functionality and 2) it has not yet been determined if the X driver
 * supports separate stencil.
 *
 * If we determine that the X driver does have support, then we set
 * intel_screen.dri2_has_hiz to true and return.
 *
 * If we determine that the X driver lacks support, and we requested
 * a DRI2BufferDepth and DRI2BufferStencil, then we must remedy the mistake by
 * taking the following actions:
 *    1. Discard the framebuffer's stencil and depth renderbuffers.
 *    2. Create a combined depth/stencil renderbuffer and attach
 *       it to the framebuffer's depth and stencil attachment points.
 *    3. Query the drawable for a new set of buffers, which consists of the
 *       originally requested set plus DRI2BufferDepthStencil.
 *    4. Assign the DRI2BufferDepthStencil's DRM region to the new
 *       depth/stencil renderbuffer.
 *
 * \pre intel->intelScreen->dri2_has_hiz == INTEL_DRI2_HAS_HIZ_UNKNOWN
 *
 * \param drawable      Drawable whose buffers were queried.
 *
 * \param buffers       [in/out] As input, the buffer list returned by the
 *                      original DRI2 query. As output, the current buffer
 *                      list, which may have been altered by a new DRI2 query.
 *
 * \param attachments   [in/out] As input, the attachment list submitted
 *                      in the original DRI2 query. As output, the attachment
 *                      list that was submitted in the DRI2 query that
 *                      obtained the current buffer list, as returned in the
 *                      output parameter \c buffers.  (Note: If no new query
 *                      was made, then the list remains unaltered).
 *
 * \param count         [out] Number of buffers in the current buffer list, as
 *                      returned in the output parameter \c buffers.
 *
 * \see enum intel_dri2_has_hiz
 * \see struct intel_screen::dri2_has_hiz
 * \see intel_update_renderbuffers
 */
static void
intel_verify_dri2_has_hiz(struct intel_context *intel,
			  __DRIdrawable *drawable,
			  __DRIbuffer **buffers,
			  unsigned **attachments,
			  int *count)
{
   assert(intel->intelScreen->dri2_has_hiz == INTEL_DRI2_HAS_HIZ_UNKNOWN);

   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *stencil_rb =
      intel_get_renderbuffer(fb, BUFFER_STENCIL);

   if (stencil_rb) {
      /*
       * We requested a DRI2BufferStencil without knowing if the X driver
       * supports it. Now, check if X handled the request correctly and clean
       * up if it did not. (See comments for 'enum intel_dri2_has_hiz').
       */
      struct intel_renderbuffer *depth_rb =
	 intel_get_renderbuffer(fb, BUFFER_DEPTH);
      assert(intel_rb_format(stencil_rb) == MESA_FORMAT_S8);
      assert(depth_rb && intel_rb_format(depth_rb) == MESA_FORMAT_X8_Z24);

      if (stencil_rb->mt->region->tiling == I915_TILING_NONE) {
	 /*
	  * The stencil buffer is actually W tiled. The region's tiling is
	  * I915_TILING_NONE, however, because the GTT is incapable of W
	  * fencing.
	  */
	 intel->intelScreen->dri2_has_hiz = INTEL_DRI2_HAS_HIZ_TRUE;
	 return;
      } else {
	 /*
	  * Oops... the screen doesn't support separate stencil. Discard the
	  * separate depth and stencil buffers and replace them with
	  * a combined depth/stencil buffer. Discard the hiz buffer too.
	  */
	 intel->intelScreen->dri2_has_hiz = INTEL_DRI2_HAS_HIZ_FALSE;
	 if (intel->must_use_separate_stencil) {
	    _mesa_problem(&intel->ctx,
			  "intel_context requires separate stencil, but the "
			  "DRIscreen does not support it. You may need to "
			  "upgrade the Intel X driver to 2.16.0");
	    abort();
	 }

	 /* 1. Discard depth and stencil renderbuffers. */
	 _mesa_remove_renderbuffer(fb, BUFFER_DEPTH);
	 depth_rb = NULL;
	 _mesa_remove_renderbuffer(fb, BUFFER_STENCIL);
	 stencil_rb = NULL;

	 /* 2. Create new depth/stencil renderbuffer. */
	 struct intel_renderbuffer *depth_stencil_rb =
	    intel_create_renderbuffer(MESA_FORMAT_S8_Z24);
	 _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depth_stencil_rb->Base.Base);
	 _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &depth_stencil_rb->Base.Base);

	 /* 3. Append DRI2BufferDepthStencil to attachment list. */
	 int old_count = *count;
	 unsigned int *old_attachments = *attachments;
	 *count = old_count + 1;
	 *attachments = malloc(2 * (*count) * sizeof(unsigned));
	 memcpy(*attachments, old_attachments, 2 * old_count * sizeof(unsigned));
	 free(old_attachments);
	 (*attachments)[2 * old_count + 0] = __DRI_BUFFER_DEPTH_STENCIL;
	 (*attachments)[2 * old_count + 1] = intel_bits_per_pixel(depth_stencil_rb);

	 /* 4. Request new set of DRI2 attachments. */
	 __DRIscreen *screen = intel->intelScreen->driScrnPriv;
	 *buffers = screen->dri2.loader->getBuffersWithFormat(drawable,
							      &drawable->w,
							      &drawable->h,
							      *attachments,
							      *count,
							      count,
							      drawable->loaderPrivate);
	 if (!*buffers)
	    return;

	 /*
	  * I don't know how to recover from the failure assertion below.
	  * Rather than fail gradually and unexpectedly, we should just die
	  * now.
	  */
	 assert(*count == old_count + 1);

	 /* 5. Assign the DRI buffer's DRM region to the its renderbuffers. */
	 __DRIbuffer *depth_stencil_buffer = NULL;
	 for (int i = 0; i < *count; ++i) {
	    if ((*buffers)[i].attachment == __DRI_BUFFER_DEPTH_STENCIL) {
	       depth_stencil_buffer = &(*buffers)[i];
	       break;
	    }
	 }
	 struct intel_region *region =
	    intel_region_alloc_for_handle(intel->intelScreen,
					  depth_stencil_buffer->cpp,
					  drawable->w,
					  drawable->h,
					  depth_stencil_buffer->pitch
					     / depth_stencil_buffer->cpp,
					  depth_stencil_buffer->name,
					  "dri2 depth / stencil buffer");
	 if (!region)
	    return;

	 struct intel_mipmap_tree *mt =
	       intel_miptree_create_for_region(intel,
	                                       GL_TEXTURE_2D,
	                                       intel_rb_format(depth_stencil_rb),
	                                       region);
	 intel_region_release(&region);
	 if (!mt)
	    return;

	 intel_miptree_reference(&intel_get_renderbuffer(fb, BUFFER_DEPTH)->mt, mt);
	 intel_miptree_reference(&intel_get_renderbuffer(fb, BUFFER_STENCIL)->mt, mt);
	 intel_miptree_release(&mt);
      }
   }

   if (intel_framebuffer_has_hiz(fb)) {
      /*
       * In the future, the driver may advertise a GL config with hiz
       * compatible depth bits and 0 stencil bits (for example, when the
       * driver gains support for float32 depth buffers). When that day comes,
       * here we need to verify that the X driver does in fact support hiz and
       * clean up if it doesn't.
       *
       * Presently, however, no verification or clean up is necessary, and
       * execution should not reach here. If the framebuffer still has a hiz
       * region, then we have already set dri2_has_hiz to true after
       * confirming above that the stencil buffer is W tiled.
       */
      assert(0);
   }
}
