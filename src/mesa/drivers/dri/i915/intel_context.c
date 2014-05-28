/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
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

int INTEL_DEBUG = (0);

const char *const i915_vendor_string = "Intel Open Source Technology Center";

const char *
i915_get_renderer_string(unsigned deviceID)
{
   const char *chipset;
   static char buffer[128];

   switch (deviceID) {
#undef CHIPSET
#define CHIPSET(id, symbol, str) case id: chipset = str; break;
#include "pci_ids/i915_pci_ids.h"
   default:
      chipset = "Unknown Intel Chipset";
      break;
   }

   (void) driGetRendererString(buffer, chipset, 0);
   return buffer;
}

static const GLubyte *
intelGetString(struct gl_context * ctx, GLenum name)
{
   const struct intel_context *const intel = intel_context(ctx);

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *) i915_vendor_string;

   case GL_RENDERER:
      return
         (GLubyte *) i915_get_renderer_string(intel->intelScreen->deviceID);

   default:
      return NULL;
   }
}

#define flushFront(screen)      ((screen)->image.loader ? (screen)->image.loader->flushFrontBuffer : (screen)->dri2.loader->flushFrontBuffer)

static void
intel_flush_front(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);
    __DRIcontext *driContext = intel->driContext;
    __DRIdrawable *driDrawable = driContext->driDrawablePriv;
    __DRIscreen *const screen = intel->intelScreen->driScrnPriv;

    if (intel->front_buffer_dirty && _mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      if (flushFront(screen) && 
          driDrawable &&
          driDrawable->loaderPrivate) {
         flushFront(screen)(driDrawable, driDrawable->loaderPrivate);

	 /* We set the dirty bit in intel_prepare_render() if we're
	  * front buffer rendering once we get there.
	  */
	 intel->front_buffer_dirty = false;
      }
   }
}

static void
intel_update_image_buffers(struct intel_context *intel, __DRIdrawable *drawable);

static unsigned
intel_bits_per_pixel(const struct intel_renderbuffer *rb)
{
   return _mesa_get_format_bytes(intel_rb_format(rb)) * 8;
}

static void
intel_query_dri2_buffers(struct intel_context *intel,
			 __DRIdrawable *drawable,
			 __DRIbuffer **buffers,
			 int *count);

static void
intel_process_dri2_buffer(struct intel_context *intel,
			  __DRIdrawable *drawable,
			  __DRIbuffer *buffer,
			  struct intel_renderbuffer *rb,
			  const char *buffer_name);

static void
intel_update_dri2_buffers(struct intel_context *intel, __DRIdrawable *drawable)
{
   __DRIbuffer *buffers = NULL;
   int i, count;
   const char *region_name;
   struct intel_renderbuffer *rb;
   struct gl_framebuffer *fb = drawable->driverPrivate;

   intel_query_dri2_buffers(intel, drawable, &buffers, &count);

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
      case __DRI_BUFFER_HIZ:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_STENCIL:
      case __DRI_BUFFER_ACCUM:
      default:
         fprintf(stderr,
                 "unhandled buffer attach event, attachment type %d\n",
                 buffers[i].attachment);
         return;
      }

      intel_process_dri2_buffer(intel, drawable, &buffers[i], rb, region_name);
   }
}

void
intel_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
   struct intel_context *intel = context->driverPrivate;
   __DRIscreen *screen = intel->intelScreen->driScrnPriv;

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   if (screen->image.loader)
      intel_update_image_buffers(intel, drawable);
   else
      intel_update_dri2_buffers(intel, drawable);

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
      if (!intel->disable_throttling)
         drm_intel_bo_wait_rendering(intel->first_post_swapbuffers_batch);
      drm_intel_bo_unreference(intel->first_post_swapbuffers_batch);
      intel->first_post_swapbuffers_batch = NULL;
      intel->need_throttle = false;
   }
}

static void
intel_noninvalidate_viewport(struct gl_context *ctx)
{
    struct intel_context *intel = intel_context(ctx);
    __DRIcontext *driContext = intel->driContext;

    intelCalcViewport(ctx);

    if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
       dri2InvalidateDrawable(driContext->driDrawablePriv);
       dri2InvalidateDrawable(driContext->driReadablePriv);
    }
}

static void
intel_viewport(struct gl_context *ctx)
{
    intelCalcViewport(ctx);
}

static const struct dri_debug_control debug_control[] = {
   { "tex",   DEBUG_TEXTURE},
   { "state", DEBUG_STATE},
   { "blit",  DEBUG_BLIT},
   { "mip",   DEBUG_MIPTREE},
   { "fall",  DEBUG_PERF},
   { "perf",  DEBUG_PERF},
   { "bat",   DEBUG_BATCH},
   { "pix",   DEBUG_PIXEL},
   { "buf",   DEBUG_BUFMGR},
   { "reg",   DEBUG_REGION},
   { "fbo",   DEBUG_FBO},
   { "fs",    DEBUG_WM },
   { "sync",  DEBUG_SYNC},
   { "dri",   DEBUG_DRI },
   { "stats", DEBUG_STATS },
   { "wm",    DEBUG_WM },
   { "aub",   DEBUG_AUB },
   { NULL,    0 }
};


static void
intelInvalidateState(struct gl_context * ctx, GLuint new_state)
{
    struct intel_context *intel = intel_context(ctx);

    if (ctx->swrast_context)
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
   intelInitClearFuncs(functions);
   intelInitBufferFuncs(functions);
   intelInitPixelFuncs(functions);
   intelInitBufferObjectFuncs(functions);
   intel_init_syncobj_functions(functions);
}

bool
intelInitContext(struct intel_context *intel,
                 int api,
                 unsigned major_version,
                 unsigned minor_version,
                 uint32_t flags,
                 const struct gl_config * mesaVis,
                 __DRIcontext * driContextPriv,
                 void *sharedContextPrivate,
                 struct dd_function_table *functions,
                 unsigned *dri_ctx_error)
{
   struct gl_context *ctx = &intel->ctx;
   struct gl_context *shareCtx = (struct gl_context *) sharedContextPrivate;
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   struct intel_screen *intelScreen = sPriv->driverPrivate;
   int bo_reuse_mode;

   /* Can't rely on invalidate events, fall back to glViewport hack */
   if (!driContextPriv->driScreenPriv->dri2.useInvalidate)
      functions->Viewport = intel_noninvalidate_viewport;
   else
      functions->Viewport = intel_viewport;

   intel->intelScreen = intelScreen;

   if (!_mesa_initialize_context(&intel->ctx, api, mesaVis, shareCtx,
                                 functions)) {
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      printf("%s: failed to init mesa context\n", __FUNCTION__);
      return false;
   }

   driContextPriv->driverPrivate = intel;
   intel->driContext = driContextPriv;
   intel->driFd = sPriv->fd;

   intel->gen = intelScreen->gen;

   const int devID = intelScreen->deviceID;

   intel->is_945 = IS_945(devID);

   intel->has_swizzling = intel->intelScreen->hw_has_swizzling;

   memset(&ctx->TextureFormatSupported,
	  0, sizeof(ctx->TextureFormatSupported));

   driParseConfigFiles(&intel->optionCache, &intelScreen->optionCache,
                       sPriv->myNum, "i915");
   intel->maxBatchSize = 4096;

   /* Estimate the size of the mappable aperture into the GTT.  There's an
    * ioctl to get the whole GTT size, but not one to get the mappable subset.
    * It turns out it's basically always 256MB, though some ancient hardware
    * was smaller.
    */
   uint32_t gtt_size = 256 * 1024 * 1024;
   if (intel->gen == 2)
      gtt_size = 128 * 1024 * 1024;

   /* We don't want to map two objects such that a memcpy between them would
    * just fault one mapping in and then the other over and over forever.  So
    * we would need to divide the GTT size by 2.  Additionally, some GTT is
    * taken up by things like the framebuffer and the ringbuffer and such, so
    * be more conservative.
    */
   intel->max_gtt_map_object_size = gtt_size / 4;

   intel->bufmgr = intelScreen->bufmgr;

   bo_reuse_mode = driQueryOptioni(&intel->optionCache, "bo_reuse");
   switch (bo_reuse_mode) {
   case DRI_CONF_BO_REUSE_DISABLED:
      break;
   case DRI_CONF_BO_REUSE_ALL:
      intel_bufmgr_gem_enable_reuse(intel->bufmgr);
      break;
   }

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 5.0;
   ctx->Const.MaxLineWidthAA = 5.0;
   ctx->Const.LineWidthGranularity = 0.5;

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 3.0;
   ctx->Const.PointSizeGranularity = 1.0;

   ctx->Const.StripTextureBorder = GL_TRUE;

   /* reinitialize the context point state.
    * It depend on constants in __struct gl_contextRec::Const
    */
   _mesa_init_point(ctx);

   ctx->Const.MaxRenderbufferSize = 2048;

   _swrast_CreateContext(ctx);
   _vbo_CreateContext(ctx);
   if (ctx->swrast_context) {
      _tnl_CreateContext(ctx);
      _swsetup_CreateContext(ctx);

      /* Configure swrast to match hardware characteristics: */
      _swrast_allow_pixel_fog(ctx, false);
      _swrast_allow_vertex_fog(ctx, true);
   }

   _mesa_meta_init(ctx);

   intel->hw_stencil = mesaVis && mesaVis->stencilBits && mesaVis->depthBits == 24;
   intel->hw_stipple = 1;

   intel->RenderIndex = ~0;

   intelInitExtensions(ctx);

   INTEL_DEBUG = driParseDebugString(getenv("INTEL_DEBUG"), debug_control);
   if (INTEL_DEBUG & DEBUG_BUFMGR)
      dri_bufmgr_set_debug(intel->bufmgr, true);
   if (INTEL_DEBUG & DEBUG_PERF)
      intel->perf_debug = true;

   if (INTEL_DEBUG & DEBUG_AUB)
      drm_intel_bufmgr_gem_set_aub_dump(intel->bufmgr, true);

   intel_batchbuffer_init(intel);

   intel_fbo_init(intel);

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

   if (driQueryOptionb(&intel->optionCache, "disable_throttling")) {
      fprintf(stderr, "disabling flush throttling\n");
      intel->disable_throttling = 1;
   }

   return true;
}

void
intelDestroyContext(__DRIcontext * driContextPriv)
{
   struct intel_context *intel =
      (struct intel_context *) driContextPriv->driverPrivate;
   struct gl_context *ctx = &intel->ctx;

   assert(intel);               /* should never be null */
   if (intel) {
      INTEL_FIREVERTICES(intel);

      /* Dump a final BMP in case the application doesn't call SwapBuffers */
      if (INTEL_DEBUG & DEBUG_AUB) {
         intel_batchbuffer_flush(intel);
	 aub_dump_bmp(&intel->ctx);
      }

      _mesa_meta_free(&intel->ctx);

      intel->vtbl.destroy(intel);

      if (ctx->swrast_context) {
         _swsetup_DestroyContext(&intel->ctx);
         _tnl_DestroyContext(&intel->ctx);
      }
      _vbo_DestroyContext(&intel->ctx);

      if (ctx->swrast_context)
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
      struct gl_context *ctx = &intel->ctx;
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
      _mesa_make_current(ctx, fb, readFb);

      /* We do this in intel_prepare_render() too, but intel->ctx.DrawBuffer
       * is NULL at that point.  We can't call _mesa_makecurrent()
       * first, since we need the buffer size for the initial
       * viewport.  So just call intel_draw_buffer() again here. */
      intel_draw_buffer(ctx);
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
 * This is called from intel_update_renderbuffers().
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
intel_query_dri2_buffers(struct intel_context *intel,
			 __DRIdrawable *drawable,
			 __DRIbuffer **buffers,
			 int *buffer_count)
{
   __DRIscreen *screen = intel->intelScreen->driScrnPriv;
   struct gl_framebuffer *fb = drawable->driverPrivate;
   int i = 0;
   unsigned attachments[8];

   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   memset(attachments, 0, sizeof(attachments));
   if ((intel->is_front_buffer_rendering ||
	intel->is_front_buffer_reading ||
	!back_rb) && front_rb) {
      /* If a fake front buffer is in use, then querying for
       * __DRI_BUFFER_FRONT_LEFT will cause the server to copy the image from
       * the real front buffer to the fake front buffer.  So before doing the
       * query, we need to make sure all the pending drawing has landed in the
       * real front buffer.
       */
      intel_flush(&intel->ctx);
      intel_flush_front(&intel->ctx);

      attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      attachments[i++] = intel_bits_per_pixel(front_rb);
   } else if (front_rb && intel->front_buffer_dirty) {
      /* We have pending front buffer rendering, but we aren't querying for a
       * front buffer.  If the front buffer we have is a fake front buffer,
       * the X server is going to throw it away when it processes the query.
       * So before doing the query, make sure all the pending drawing has
       * landed in the real front buffer.
       */
      intel_flush(&intel->ctx);
      intel_flush_front(&intel->ctx);
   }

   if (back_rb) {
      attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      attachments[i++] = intel_bits_per_pixel(back_rb);
   }

   assert(i <= ARRAY_SIZE(attachments));

   *buffers = screen->dri2.loader->getBuffersWithFormat(drawable,
							&drawable->w,
							&drawable->h,
							attachments, i / 2,
							buffer_count,
							drawable->loaderPrivate);
}

/**
 * \brief Assign a DRI buffer's DRM region to a renderbuffer.
 *
 * This is called from intel_update_renderbuffers().
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
intel_process_dri2_buffer(struct intel_context *intel,
			  __DRIdrawable *drawable,
			  __DRIbuffer *buffer,
			  struct intel_renderbuffer *rb,
			  const char *buffer_name)
{
   struct intel_region *region = NULL;

   if (!rb)
      return;

   /* We try to avoid closing and reopening the same BO name, because the first
    * use of a mapping of the buffer involves a bunch of page faulting which is
    * moderately expensive.
    */
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

   intel_miptree_release(&rb->mt);
   region = intel_region_alloc_for_handle(intel->intelScreen,
                                          buffer->cpp,
                                          drawable->w,
                                          drawable->h,
                                          buffer->pitch,
                                          buffer->name,
                                          buffer_name);
   if (!region)
      return;

   rb->mt = intel_miptree_create_for_dri2_buffer(intel,
                                                 buffer->attachment,
                                                 intel_rb_format(rb),
                                                 region);
   intel_region_release(&region);
}

/**
 * \brief Query DRI Image loader to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers with
 * dri3
 *
 * This is called from intel_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see intel_update_renderbuffers()
 */

static void
intel_update_image_buffer(struct intel_context *intel,
                          __DRIdrawable *drawable,
                          struct intel_renderbuffer *rb,
                          __DRIimage *buffer,
                          enum __DRIimageBufferMask buffer_type)
{
   struct intel_region *region = buffer->region;

   if (!rb || !region)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   if (rb->mt &&
       rb->mt->region &&
       rb->mt->region == region)
      return;

   intel_miptree_release(&rb->mt);
   rb->mt = intel_miptree_create_for_image_buffer(intel,
                                                  buffer_type,
                                                  intel_rb_format(rb),
                                                  num_samples,
                                                  region);
}


static void
intel_update_image_buffers(struct intel_context *intel, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   __DRIscreen *screen = intel->intelScreen->driScrnPriv;
   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;
   struct __DRIimageList images;
   unsigned int format;
   uint32_t buffer_mask = 0;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   if (back_rb)
      format = intel_rb_format(back_rb);
   else if (front_rb)
      format = intel_rb_format(front_rb);
   else
      return;

   if ((intel->is_front_buffer_rendering || intel->is_front_buffer_reading || !back_rb) && front_rb)
      buffer_mask |= __DRI_IMAGE_BUFFER_FRONT;

   if (back_rb)
      buffer_mask |= __DRI_IMAGE_BUFFER_BACK;

   (*screen->image.loader->getBuffers) (drawable,
                                        driGLFormatToImageFormat(format),
                                        &drawable->dri2.stamp,
                                        drawable->loaderPrivate,
                                        buffer_mask,
                                        &images);

   if (images.image_mask & __DRI_IMAGE_BUFFER_FRONT) {
      drawable->w = images.front->width;
      drawable->h = images.front->height;
      intel_update_image_buffer(intel,
                                drawable,
                                front_rb,
                                images.front,
                                __DRI_IMAGE_BUFFER_FRONT);
   }
   if (images.image_mask & __DRI_IMAGE_BUFFER_BACK) {
      drawable->w = images.back->width;
      drawable->h = images.back->height;
      intel_update_image_buffer(intel,
                                drawable,
                                back_rb,
                                images.back,
                                __DRI_IMAGE_BUFFER_BACK);
   }
}
