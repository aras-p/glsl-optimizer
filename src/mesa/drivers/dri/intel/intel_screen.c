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
#include "main/framebuffer.h"
#include "main/matrix.h"
#include "main/renderbuffer.h"
#include "main/simple_list.h"
#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"


#include "intel_screen.h"

#include "intel_buffers.h"
#include "intel_tex.h"
#include "intel_span.h"
#include "intel_fbo.h"
#include "intel_chipset.h"

#include "i915_drm.h"
#include "i830_dri.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "intel_bufmgr.h"

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN
   DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
      DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_ALWAYS_SYNC)
      /* Options correspond to DRI_CONF_BO_REUSE_DISABLED,
       * DRI_CONF_BO_REUSE_ALL
       */
      DRI_CONF_OPT_BEGIN_V(bo_reuse, enum, 1, "0:1")
	 DRI_CONF_DESC_BEGIN(en, "Buffer object reuse")
	    DRI_CONF_ENUM(0, "Disable buffer object reuse")
	    DRI_CONF_ENUM(1, "Enable reuse of all sizes of buffer objects")
	 DRI_CONF_DESC_END
      DRI_CONF_OPT_END
   DRI_CONF_SECTION_END
   DRI_CONF_SECTION_QUALITY
      DRI_CONF_FORCE_S3TC_ENABLE(false)
      DRI_CONF_ALLOW_LARGE_TEXTURES(2)
   DRI_CONF_SECTION_END
   DRI_CONF_SECTION_DEBUG
     DRI_CONF_NO_RAST(false)
   DRI_CONF_SECTION_END
DRI_CONF_END;

const GLuint __driNConfigOptions = 6;

#ifdef USE_NEW_INTERFACE
static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /*USE_NEW_INTERFACE */

/**
 * Map all the memory regions described by the screen.
 * \return GL_TRUE if success, GL_FALSE if error.
 */
GLboolean
intelMapScreenRegions(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

   if (0)
      _mesa_printf("TEX 0x%08x ", intelScreen->tex.handle);
   if (intelScreen->tex.size != 0) {
      if (drmMap(sPriv->fd,
		 intelScreen->tex.handle,
		 intelScreen->tex.size,
		 (drmAddress *) & intelScreen->tex.map) != 0) {
	 intelUnmapScreenRegions(intelScreen);
	 return GL_FALSE;
      }
   }

   return GL_TRUE;
}

void
intelUnmapScreenRegions(intelScreenPrivate * intelScreen)
{
   if (intelScreen->tex.map) {
      drmUnmap(intelScreen->tex.map, intelScreen->tex.size);
      intelScreen->tex.map = NULL;
   }
}


static void
intelPrintDRIInfo(intelScreenPrivate * intelScreen,
                  __DRIscreenPrivate * sPriv, I830DRIPtr gDRIPriv)
{
   fprintf(stderr, "*** Front size:   0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->front.size, intelScreen->front.offset,
           intelScreen->pitch);
   fprintf(stderr, "*** Back size:    0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->back.size, intelScreen->back.offset,
           intelScreen->pitch);
   fprintf(stderr, "*** Depth size:   0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->depth.size, intelScreen->depth.offset,
           intelScreen->pitch);
   fprintf(stderr, "*** Texture size: 0x%x  offset: 0x%x\n",
           intelScreen->tex.size, intelScreen->tex.offset);
   fprintf(stderr, "*** Memory : 0x%x\n", gDRIPriv->mem);
}


static void
intelPrintSAREA(const struct drm_i915_sarea * sarea)
{
   fprintf(stderr, "SAREA: sarea width %d  height %d\n", sarea->width,
           sarea->height);
   fprintf(stderr, "SAREA: pitch: %d\n", sarea->pitch);
   fprintf(stderr,
           "SAREA: front offset: 0x%08x  size: 0x%x  handle: 0x%x tiled: %d\n",
           sarea->front_offset, sarea->front_size,
           (unsigned) sarea->front_handle, sarea->front_tiled);
   fprintf(stderr,
           "SAREA: back  offset: 0x%08x  size: 0x%x  handle: 0x%x tiled: %d\n",
           sarea->back_offset, sarea->back_size,
           (unsigned) sarea->back_handle, sarea->back_tiled);
   fprintf(stderr, "SAREA: depth offset: 0x%08x  size: 0x%x  handle: 0x%x tiled: %d\n",
           sarea->depth_offset, sarea->depth_size,
           (unsigned) sarea->depth_handle, sarea->depth_tiled);
   fprintf(stderr, "SAREA: tex   offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->tex_offset, sarea->tex_size, (unsigned) sarea->tex_handle);
}


/**
 * A number of the screen parameters are obtained/computed from
 * information in the SAREA.  This function updates those parameters.
 */
void
intelUpdateScreenFromSAREA(intelScreenPrivate * intelScreen,
                           struct drm_i915_sarea * sarea)
{
   intelScreen->width = sarea->width;
   intelScreen->height = sarea->height;
   intelScreen->pitch = sarea->pitch;

   intelScreen->front.offset = sarea->front_offset;
   intelScreen->front.handle = sarea->front_handle;
   intelScreen->front.size = sarea->front_size;
   intelScreen->front.tiled = sarea->front_tiled;

   intelScreen->back.offset = sarea->back_offset;
   intelScreen->back.handle = sarea->back_handle;
   intelScreen->back.size = sarea->back_size;
   intelScreen->back.tiled = sarea->back_tiled;

   intelScreen->depth.offset = sarea->depth_offset;
   intelScreen->depth.handle = sarea->depth_handle;
   intelScreen->depth.size = sarea->depth_size;
   intelScreen->depth.tiled = sarea->depth_tiled;

   if (intelScreen->driScrnPriv->ddx_version.minor >= 9) {
      intelScreen->front.bo_handle = sarea->front_bo_handle;
      intelScreen->back.bo_handle = sarea->back_bo_handle;
      intelScreen->depth.bo_handle = sarea->depth_bo_handle;
   } else {
      intelScreen->front.bo_handle = -1;
      intelScreen->back.bo_handle = -1;
      intelScreen->depth.bo_handle = -1;
   }

   intelScreen->tex.offset = sarea->tex_offset;
   intelScreen->logTextureGranularity = sarea->log_tex_granularity;
   intelScreen->tex.handle = sarea->tex_handle;
   intelScreen->tex.size = sarea->tex_size;

   if (0)
      intelPrintSAREA(sarea);
}

static const __DRItexOffsetExtension intelTexOffsetExtension = {
   { __DRI_TEX_OFFSET },
   intelSetTexOffset,
};

static const __DRItexBufferExtension intelTexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   intelSetTexBuffer,
};

static const __DRIextension *intelScreenExtensions[] = {
    &driReadDrawableExtension,
    &driCopySubBufferExtension.base,
    &driSwapControlExtension.base,
    &driFrameTrackingExtension.base,
    &driMediaStreamCounterExtension.base,
    &intelTexOffsetExtension.base,
    &intelTexBufferExtension.base,
    NULL
};

static GLboolean
intel_get_param(__DRIscreenPrivate *psp, int param, int *value)
{
   int ret;
   struct drm_i915_getparam gp;

   gp.param = param;
   gp.value = value;

   ret = drmCommandWriteRead(psp->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
   if (ret) {
      fprintf(stderr, "drm_i915_getparam: %d\n", ret);
      return GL_FALSE;
   }

   return GL_TRUE;
}

static GLboolean intelInitDriver(__DRIscreenPrivate *sPriv)
{
   intelScreenPrivate *intelScreen;
   I830DRIPtr gDRIPriv = (I830DRIPtr) sPriv->pDevPriv;
   struct drm_i915_sarea *sarea;

   if (sPriv->devPrivSize != sizeof(I830DRIRec)) {
      fprintf(stderr,
              "\nERROR!  sizeof(I830DRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   intelScreen = (intelScreenPrivate *) CALLOC(sizeof(intelScreenPrivate));
   if (!intelScreen) {
      fprintf(stderr, "\nERROR!  Allocating private area failed\n");
      return GL_FALSE;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = sPriv;
   sPriv->private = (void *) intelScreen;
   sarea = (struct drm_i915_sarea *)
      (((GLubyte *) sPriv->pSAREA) + gDRIPriv->sarea_priv_offset);
   intelScreen->sarea = sarea;

   intelScreen->deviceID = gDRIPriv->deviceID;

   intelUpdateScreenFromSAREA(intelScreen, sarea);

   if (!intelMapScreenRegions(sPriv)) {
      fprintf(stderr, "\nERROR!  mapping regions\n");
      _mesa_free(intelScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   }

   if (0)
      intelPrintDRIInfo(intelScreen, sPriv, gDRIPriv);

   intelScreen->drmMinor = sPriv->drm_version.minor;

   /* Determine if IRQs are active? */
   if (!intel_get_param(sPriv, I915_PARAM_IRQ_ACTIVE,
			&intelScreen->irq_active))
      return GL_FALSE;

   sPriv->extensions = intelScreenExtensions;

   return GL_TRUE;
}


static void
intelDestroyScreen(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

   dri_bufmgr_destroy(intelScreen->bufmgr);
   intelUnmapScreenRegions(intelScreen);

   FREE(intelScreen);
   sPriv->private = NULL;
}


/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static GLboolean
intelCreateBuffer(__DRIscreenPrivate * driScrnPriv,
                  __DRIdrawablePrivate * driDrawPriv,
                  const __GLcontextModes * mesaVis, GLboolean isPixmap)
{
   intelScreenPrivate *screen = (intelScreenPrivate *) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE;          /* not implemented */
   }
   else {
      GLboolean swStencil = (mesaVis->stencilBits > 0 &&
                             mesaVis->depthBits != 24);
      GLenum rgbFormat = (mesaVis->redBits == 5 ? GL_RGB5 : GL_RGBA8);

      struct intel_framebuffer *intel_fb = CALLOC_STRUCT(intel_framebuffer);

      if (!intel_fb)
	 return GL_FALSE;

      _mesa_initialize_framebuffer(&intel_fb->Base, mesaVis);

      /* setup the hardware-based renderbuffers */
      intel_fb->color_rb[0] = intel_create_renderbuffer(rgbFormat);
      _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT,
			     &intel_fb->color_rb[0]->Base);

      if (mesaVis->doubleBufferMode) {
	 intel_fb->color_rb[1] = intel_create_renderbuffer(rgbFormat);

         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT,
				&intel_fb->color_rb[1]->Base);

      }

      if (mesaVis->depthBits == 24) {
	 if (mesaVis->stencilBits == 8) {
	    /* combined depth/stencil buffer */
	    struct intel_renderbuffer *depthStencilRb
	       = intel_create_renderbuffer(GL_DEPTH24_STENCIL8_EXT);
	    /* note: bind RB to two attachment points */
	    _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH,
				   &depthStencilRb->Base);
	    _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_STENCIL,
				   &depthStencilRb->Base);
	 } else {
	    struct intel_renderbuffer *depthRb
	       = intel_create_renderbuffer(GL_DEPTH_COMPONENT24);
	    _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH,
				   &depthRb->Base);
	 }
      }
      else if (mesaVis->depthBits == 16) {
         /* just 16-bit depth buffer, no hw stencil */
         struct intel_renderbuffer *depthRb
	    = intel_create_renderbuffer(GL_DEPTH_COMPONENT16);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH, &depthRb->Base);
      }

      /* now add any/all software-based renderbuffers we may need */
      _mesa_add_soft_renderbuffers(&intel_fb->Base,
                                   GL_FALSE, /* never sw color */
                                   GL_FALSE, /* never sw depth */
                                   swStencil, mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* never sw alpha */
                                   GL_FALSE  /* never sw aux */ );
      driDrawPriv->driverPrivate = (void *) intel_fb;

      return GL_TRUE;
   }
}

static void
intelDestroyBuffer(__DRIdrawablePrivate * driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}


/**
 * Get information about previous buffer swaps.
 */
static int
intelGetSwapInfo(__DRIdrawablePrivate * dPriv, __DRIswapInfo * sInfo)
{
   struct intel_framebuffer *intel_fb;

   if ((dPriv == NULL) || (dPriv->driverPrivate == NULL)
       || (sInfo == NULL)) {
      return -1;
   }

   intel_fb = dPriv->driverPrivate;
   sInfo->swap_count = intel_fb->swap_count;
   sInfo->swap_ust = intel_fb->swap_ust;
   sInfo->swap_missed_count = intel_fb->swap_missed_count;

   sInfo->swap_missed_usage = (sInfo->swap_missed_count != 0)
      ? driCalculateSwapUsage(dPriv, 0, intel_fb->swap_missed_ust)
      : 0.0;

   return 0;
}


/* There are probably better ways to do this, such as an
 * init-designated function to register chipids and createcontext
 * functions.
 */
extern GLboolean i830CreateContext(const __GLcontextModes * mesaVis,
                                   __DRIcontextPrivate * driContextPriv,
                                   void *sharedContextPrivate);

extern GLboolean i915CreateContext(const __GLcontextModes * mesaVis,
                                   __DRIcontextPrivate * driContextPriv,
                                   void *sharedContextPrivate);
extern GLboolean brwCreateContext(const __GLcontextModes * mesaVis,
				  __DRIcontextPrivate * driContextPriv,
				  void *sharedContextPrivate);

static GLboolean
intelCreateContext(const __GLcontextModes * mesaVis,
                   __DRIcontextPrivate * driContextPriv,
                   void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

#ifdef I915
   if (IS_9XX(intelScreen->deviceID)) {
      if (!IS_965(intelScreen->deviceID)) {
	 return i915CreateContext(mesaVis, driContextPriv,
				  sharedContextPrivate);
      }
   } else {
      intelScreen->no_vbo = GL_TRUE;
      return i830CreateContext(mesaVis, driContextPriv, sharedContextPrivate);
   }
#else
   if (IS_965(intelScreen->deviceID))
      return brwCreateContext(mesaVis, driContextPriv, sharedContextPrivate);
#endif
   fprintf(stderr, "Unrecognized deviceID %x\n", intelScreen->deviceID);
   return GL_FALSE;
}


static __DRIconfig **
intelFillInModes(__DRIscreenPrivate *psp,
		 unsigned pixel_bits, unsigned depth_bits,
                 unsigned stencil_bits, GLboolean have_back_buffer)
{
   __DRIconfig **configs;
   __GLcontextModes *m;
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;
   int i;

   /* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
    * support pageflipping at all.
    */
   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };

   uint8_t depth_bits_array[3];
   uint8_t stencil_bits_array[3];

   depth_bits_array[0] = 0;
   depth_bits_array[1] = depth_bits;
   depth_bits_array[2] = depth_bits;

   /* Just like with the accumulation buffer, always provide some modes
    * with a stencil buffer.  It will be a sw fallback, but some apps won't
    * care about that.
    */
   stencil_bits_array[0] = 0;
   stencil_bits_array[1] = 0;
   if (depth_bits == 24)
      stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

   stencil_bits_array[2] = (stencil_bits == 0) ? 8 : stencil_bits;

   depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 3 : 1;
   back_buffer_factor = (have_back_buffer) ? 3 : 1;

   if (pixel_bits == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   }
   else {
      fb_format = GL_BGRA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   configs = driCreateConfigs(fb_format, fb_type,
			      depth_bits_array, stencil_bits_array,
			      depth_buffer_factor, back_buffer_modes,
			      back_buffer_factor);
   if (configs == NULL) {
    fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   /* Mark the visual as slow if there are "fake" stencil bits.
    */
   for (i = 0; configs[i]; i++) {
      m = &configs[i]->modes;
      if ((m->stencilBits != 0) && (m->stencilBits != stencil_bits)) {
         m->visualRating = GLX_SLOW_CONFIG;
      }
   }

   return configs;
}

static GLboolean
intel_init_bufmgr(intelScreenPrivate *intelScreen)
{
   GLboolean gem_disable = getenv("INTEL_NO_GEM") != NULL;
   int gem_kernel = 0;
   GLboolean gem_supported;
   struct drm_i915_getparam gp;
   __DRIscreenPrivate *spriv = intelScreen->driScrnPriv;

   intelScreen->no_hw = getenv("INTEL_NO_HW") != NULL;

   gp.param = I915_PARAM_HAS_GEM;
   gp.value = &gem_kernel;

   (void) drmCommandWriteRead(spriv->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));

   /* If we've got a new enough DDX that's initializing GEM and giving us
    * object handles for the shared buffers, use that.
    */
   intelScreen->ttm = GL_FALSE;
   if (intelScreen->driScrnPriv->dri2.enabled)
       gem_supported = GL_TRUE;
   else if (intelScreen->driScrnPriv->ddx_version.minor >= 9 &&
	    gem_kernel &&
	    intelScreen->front.bo_handle != -1)
       gem_supported = GL_TRUE;
   else
       gem_supported = GL_FALSE;

   if (!gem_disable && gem_supported) {
      intelScreen->bufmgr = intel_bufmgr_gem_init(spriv->fd, BATCH_SZ);
      if (intelScreen->bufmgr != NULL)
	 intelScreen->ttm = GL_TRUE;
   }
   /* Otherwise, use the classic buffer manager. */
   if (intelScreen->bufmgr == NULL) {
      if (gem_disable) {
	 fprintf(stderr, "GEM disabled.  Using classic.\n");
      } else {
	 fprintf(stderr, "Failed to initialize GEM.  "
		 "Falling back to classic.\n");
      }

      if (intelScreen->tex.size == 0) {
	 fprintf(stderr, "[%s:%u] Error initializing buffer manager.\n",
		 __func__, __LINE__);
	 return GL_FALSE;
      }

      intelScreen->bufmgr =
	 intel_bufmgr_fake_init(spriv->fd,
				intelScreen->tex.offset,
				intelScreen->tex.map,
				intelScreen->tex.size,
				(unsigned int * volatile)
				&intelScreen->sarea->last_dispatch);
   }

   /* XXX bufmgr should be per-screen, not per-context */
   intelScreen->ttm = intelScreen->ttm;

   return GL_TRUE;
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using legacy DRI.
 * 
 * \todo maybe fold this into intelInitDriver
 *
 * \return the __GLcontextModes supported by this driver
 */
static const __DRIconfig **intelInitScreen(__DRIscreenPrivate *psp)
{
   intelScreenPrivate *intelScreen;
#ifdef I915
   static const __DRIversion ddx_expected = { 1, 5, 0 };
#else
   static const __DRIversion ddx_expected = { 1, 6, 0 };
#endif
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 5, 0 };
   I830DRIPtr dri_priv = (I830DRIPtr) psp->pDevPriv;

   if (!driCheckDriDdxDrmVersions2("i915",
                                   &psp->dri_version, &dri_expected,
                                   &psp->ddx_version, &ddx_expected,
                                   &psp->drm_version, &drm_expected)) {
      return NULL;
   }

   /* Calling driInitExtensions here, with a NULL context pointer,
    * does not actually enable the extensions.  It just makes sure
    * that all the dispatch offsets for all the extensions that
    * *might* be enables are known.  This is needed because the
    * dispatch offsets need to be known when _mesa_context_create is
    * called, but we can't enable the extensions until we have a
    * context pointer.
    *
    * Hello chicken.  Hello egg.  How are you two today?
    */
   intelInitExtensions(NULL, GL_TRUE);
	   
   if (!intelInitDriver(psp))
       return NULL;

   psp->extensions = intelScreenExtensions;

   intelScreen = psp->private;
   if (!intel_init_bufmgr(intelScreen))
       return GL_FALSE;

   return (const __DRIconfig **)
       intelFillInModes(psp, dri_priv->cpp * 8,
			(dri_priv->cpp == 2) ? 16 : 24,
			(dri_priv->cpp == 2) ? 0  : 8, 1);
}

struct intel_context *intelScreenContext(intelScreenPrivate *intelScreen)
{
  /*
   * This should probably change to have the screen allocate a dummy
   * context at screen creation. For now just use the current context.
   */

  GET_CURRENT_CONTEXT(ctx);
  if (ctx == NULL) {
     _mesa_problem(NULL, "No current context in intelScreenContext\n");
     return NULL;
  }
  return intel_context(ctx);
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using DRI2.
 *
 * \return the __GLcontextModes supported by this driver
 */
static const
__DRIconfig **intelInitScreen2(__DRIscreenPrivate *psp)
{
   intelScreenPrivate *intelScreen;

   /* Calling driInitExtensions here, with a NULL context pointer,
    * does not actually enable the extensions.  It just makes sure
    * that all the dispatch offsets for all the extensions that
    * *might* be enables are known.  This is needed because the
    * dispatch offsets need to be known when _mesa_context_create is
    * called, but we can't enable the extensions until we have a
    * context pointer.
    *
    * Hello chicken.  Hello egg.  How are you two today?
    */
   intelInitExtensions(NULL, GL_TRUE);

   /* Allocate the private area */
   intelScreen = (intelScreenPrivate *) CALLOC(sizeof(intelScreenPrivate));
   if (!intelScreen) {
      fprintf(stderr, "\nERROR!  Allocating private area failed\n");
      return GL_FALSE;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = psp;
   psp->private = (void *) intelScreen;

   intelScreen->drmMinor = psp->drm_version.minor;

   /* Determine chipset ID */
   if (!intel_get_param(psp, I915_PARAM_CHIPSET_ID,
			&intelScreen->deviceID))
      return GL_FALSE;

   if (!intel_init_bufmgr(intelScreen))
       return GL_FALSE;

   intelScreen->irq_active = 1;
   psp->extensions = intelScreenExtensions;

   return driConcatConfigs(intelFillInModes(psp, 16, 16, 0, 1),
			   intelFillInModes(psp, 32, 24, 8, 1));
}

const struct __DriverAPIRec driDriverAPI = {
   .InitScreen		 = intelInitScreen,
   .DestroyScreen	 = intelDestroyScreen,
   .CreateContext	 = intelCreateContext,
   .DestroyContext	 = intelDestroyContext,
   .CreateBuffer	 = intelCreateBuffer,
   .DestroyBuffer	 = intelDestroyBuffer,
   .SwapBuffers		 = intelSwapBuffers,
   .MakeCurrent		 = intelMakeCurrent,
   .UnbindContext	 = intelUnbindContext,
   .GetSwapInfo		 = intelGetSwapInfo,
   .GetDrawableMSC	 = driDrawableGetMSC32,
   .WaitForMSC		 = driWaitForMSC32,
   .CopySubBuffer	 = intelCopySubBuffer,

   .InitScreen2		 = intelInitScreen2,
};
