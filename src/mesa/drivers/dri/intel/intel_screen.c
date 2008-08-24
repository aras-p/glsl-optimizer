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

#include "glheader.h"
#include "context.h"
#include "framebuffer.h"
#include "matrix.h"
#include "renderbuffer.h"
#include "simple_list.h"
#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"


#include "intel_screen.h"

#include "intel_buffers.h"
#include "intel_tex.h"
#include "intel_span.h"
#include "intel_ioctl.h"
#include "intel_fbo.h"
#include "intel_chipset.h"

#include "i915_drm.h"
#include "i830_dri.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "intel_bufmgr_ttm.h"

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN
   DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
      DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_ALWAYS_SYNC)
      /* Options correspond to DRI_CONF_BO_REUSE_DISABLED,
       * DRI_CONF_BO_REUSE_ALL
       */
      DRI_CONF_OPT_BEGIN_V(bo_reuse, enum, 0, "0:1")
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

   if (intelScreen->front.handle) {
      if (drmMap(sPriv->fd,
                 intelScreen->front.handle,
                 intelScreen->front.size,
                 (drmAddress *) & intelScreen->front.map) != 0) {
         _mesa_problem(NULL, "drmMap(frontbuffer) failed!");
         return GL_FALSE;
      }
   }
   else {
      _mesa_warning(NULL, "no front buffer handle in intelMapScreenRegions!");
   }

   if (0)
      _mesa_printf("Back 0x%08x ", intelScreen->back.handle);
   if (drmMap(sPriv->fd,
              intelScreen->back.handle,
              intelScreen->back.size,
              (drmAddress *) & intelScreen->back.map) != 0) {
      intelUnmapScreenRegions(intelScreen);
      return GL_FALSE;
   }

   if (intelScreen->third.handle) {
      if (0)
	 _mesa_printf("Third 0x%08x ", intelScreen->third.handle);
      if (drmMap(sPriv->fd,
		 intelScreen->third.handle,
		 intelScreen->third.size,
		 (drmAddress *) & intelScreen->third.map) != 0) {
	 intelUnmapScreenRegions(intelScreen);
	 return GL_FALSE;
      }
   }

   if (0)
      _mesa_printf("Depth 0x%08x ", intelScreen->depth.handle);
   if (drmMap(sPriv->fd,
              intelScreen->depth.handle,
              intelScreen->depth.size,
              (drmAddress *) & intelScreen->depth.map) != 0) {
      intelUnmapScreenRegions(intelScreen);
      return GL_FALSE;
   }

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

   if (0)
      printf("Mappings:  front: %p  back: %p  third: %p  depth: %p  tex: %p\n",
             intelScreen->front.map,
             intelScreen->back.map, intelScreen->third.map,
             intelScreen->depth.map, intelScreen->tex.map);
   return GL_TRUE;
}

void
intelUnmapScreenRegions(intelScreenPrivate * intelScreen)
{
#define REALLY_UNMAP 1
   if (intelScreen->front.map) {
#if REALLY_UNMAP
      if (drmUnmap(intelScreen->front.map, intelScreen->front.size) != 0)
         printf("drmUnmap front failed!\n");
#endif
      intelScreen->front.map = NULL;
   }
   if (intelScreen->back.map) {
#if REALLY_UNMAP
      if (drmUnmap(intelScreen->back.map, intelScreen->back.size) != 0)
         printf("drmUnmap back failed!\n");
#endif
      intelScreen->back.map = NULL;
   }
   if (intelScreen->third.map) {
#if REALLY_UNMAP
      if (drmUnmap(intelScreen->third.map, intelScreen->third.size) != 0)
         printf("drmUnmap third failed!\n");
#endif
      intelScreen->third.map = NULL;
   }
   if (intelScreen->depth.map) {
#if REALLY_UNMAP
      drmUnmap(intelScreen->depth.map, intelScreen->depth.size);
      intelScreen->depth.map = NULL;
#endif
   }
   if (intelScreen->tex.map) {
#if REALLY_UNMAP
      drmUnmap(intelScreen->tex.map, intelScreen->tex.size);
      intelScreen->tex.map = NULL;
#endif
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
           "SAREA: front offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->front_offset, sarea->front_size,
           (unsigned) sarea->front_handle);
   fprintf(stderr,
           "SAREA: back  offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->back_offset, sarea->back_size,
           (unsigned) sarea->back_handle);
   fprintf(stderr, "SAREA: depth offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->depth_offset, sarea->depth_size,
           (unsigned) sarea->depth_handle);
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

   if (intelScreen->driScrnPriv->ddx_version.minor >= 8) {
      intelScreen->third.offset = sarea->third_offset;
      intelScreen->third.handle = sarea->third_handle;
      intelScreen->third.size = sarea->third_size;
      intelScreen->third.tiled = sarea->third_tiled;
   }

   intelScreen->depth.offset = sarea->depth_offset;
   intelScreen->depth.handle = sarea->depth_handle;
   intelScreen->depth.size = sarea->depth_size;
   intelScreen->depth.tiled = sarea->depth_tiled;

   if (intelScreen->driScrnPriv->ddx_version.minor >= 9) {
      intelScreen->front.bo_handle = sarea->front_bo_handle;
      intelScreen->back.bo_handle = sarea->back_bo_handle;
      intelScreen->third.bo_handle = sarea->third_bo_handle;
      intelScreen->depth.bo_handle = sarea->depth_bo_handle;
   } else {
      intelScreen->front.bo_handle = -1;
      intelScreen->back.bo_handle = -1;
      intelScreen->third.bo_handle = -1;
      intelScreen->depth.bo_handle = -1;
   }

   intelScreen->tex.offset = sarea->tex_offset;
   intelScreen->logTextureGranularity = sarea->log_tex_granularity;
   intelScreen->tex.handle = sarea->tex_handle;
   intelScreen->tex.size = sarea->tex_size;

   if (0)
      intelPrintSAREA(sarea);
}


/**
 * DRI2 entrypoint
 */
static void
intelHandleDrawableConfig(__DRIdrawablePrivate *dPriv,
			  __DRIcontextPrivate *pcp,
			  __DRIDrawableConfigEvent *event)
{
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
   struct intel_region *region = NULL;
   struct intel_renderbuffer *rb, *depth_rb, *stencil_rb;
   struct intel_context *intel = pcp->driverPrivate;
   int cpp, pitch;

   cpp = intel->ctx.Visual.rgbBits / 8;
   pitch = ((cpp * dPriv->w + 63) & ~63) / cpp;

   rb = intel_fb->color_rb[1];
   if (rb) {
      region = intel_region_alloc(intel, cpp, pitch, dPriv->h);
      intel_renderbuffer_set_region(rb, region);
   }

   rb = intel_fb->color_rb[2];
   if (rb) {
      region = intel_region_alloc(intel, cpp, pitch, dPriv->h);
      intel_renderbuffer_set_region(rb, region);
   }

   depth_rb = intel_get_renderbuffer(&intel_fb->Base, BUFFER_DEPTH);
   stencil_rb = intel_get_renderbuffer(&intel_fb->Base, BUFFER_STENCIL);
   if (depth_rb || stencil_rb)
      region = intel_region_alloc(intel, cpp, pitch, dPriv->h);
   if (depth_rb)
      intel_renderbuffer_set_region(depth_rb, region);
   if (stencil_rb)
      intel_renderbuffer_set_region(stencil_rb, region);

   /* FIXME: Tell the X server about the regions we just allocated and
    * attached. */
}

#define BUFFER_FLAG_TILED 0x0100

/**
 * DRI2 entrypoint
 */
static void
intelHandleBufferAttach(__DRIdrawablePrivate *dPriv,
			__DRIcontextPrivate *pcp,
			__DRIBufferAttachEvent *ba)
{
   struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
   struct intel_renderbuffer *rb;
   struct intel_region *region;
   struct intel_context *intel = pcp->driverPrivate;
   GLuint tiled;

   switch (ba->buffer.attachment) {
   case DRI_DRAWABLE_BUFFER_FRONT_LEFT:
      rb = intel_fb->color_rb[0];
      break;

   case DRI_DRAWABLE_BUFFER_BACK_LEFT:
      rb = intel_fb->color_rb[0];
      break;

   case DRI_DRAWABLE_BUFFER_DEPTH:
     rb = intel_get_renderbuffer(&intel_fb->Base, BUFFER_DEPTH);
     break;

   case DRI_DRAWABLE_BUFFER_STENCIL:
     rb = intel_get_renderbuffer(&intel_fb->Base, BUFFER_STENCIL);
     break;

   case DRI_DRAWABLE_BUFFER_ACCUM:
   default:
      fprintf(stderr, "unhandled buffer attach event, attacment type %d\n",
	      ba->buffer.attachment);
      return;
   }

#if 0
   /* FIXME: Add this so we can filter out when the X server sends us
    * attachment events for the buffers we just allocated.  Need to
    * get the BO handle for a render buffer. */
   if (intel_renderbuffer_get_region_handle(rb) == ba->buffer.handle)
      return;
#endif

   tiled = (ba->buffer.flags & BUFFER_FLAG_TILED) > 0;
   region = intel_region_alloc_for_handle(intel, ba->buffer.cpp,
					  ba->buffer.pitch / ba->buffer.cpp,
					  dPriv->h, tiled,
					  ba->buffer.handle);

   intel_renderbuffer_set_region(rb, region);
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
   intelScreen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;
   sarea = (struct drm_i915_sarea *)
      (((GLubyte *) sPriv->pSAREA) + intelScreen->sarea_priv_offset);

   intelScreen->deviceID = gDRIPriv->deviceID;

   intelUpdateScreenFromSAREA(intelScreen, sarea);

   if (!intelMapScreenRegions(sPriv)) {
      fprintf(stderr, "\nERROR!  mapping regions\n");
      _mesa_free(intelScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   }

   intelScreen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;

   if (0)
      intelPrintDRIInfo(intelScreen, sPriv, gDRIPriv);

   intelScreen->drmMinor = sPriv->drm_version.minor;

   /* Determine if IRQs are active? */
   if (!intel_get_param(sPriv, I915_PARAM_IRQ_ACTIVE,
			&intelScreen->irq_active))
      return GL_FALSE;

   /* Determine if batchbuffers are allowed */
   if (!intel_get_param(sPriv, I915_PARAM_ALLOW_BATCHBUFFER,
			&intelScreen->allow_batchbuffer))
      return GL_FALSE;

   sPriv->extensions = intelScreenExtensions;

   return GL_TRUE;
}


static void
intelDestroyScreen(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

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
      {
         intel_fb->color_rb[0] = intel_create_renderbuffer(rgbFormat);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT,
				&intel_fb->color_rb[0]->Base);
      }

      if (mesaVis->doubleBufferMode) {
         intel_fb->color_rb[1] = intel_create_renderbuffer(rgbFormat);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT,
				&intel_fb->color_rb[1]->Base);

	 if (screen->third.handle) {
	    struct gl_renderbuffer *tmp_rb = NULL;

	    intel_fb->color_rb[2] = intel_create_renderbuffer(rgbFormat);
	    _mesa_reference_renderbuffer(&tmp_rb, &intel_fb->color_rb[2]->Base);
	 }
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

   u_int8_t depth_bits_array[3];
   u_int8_t stencil_bits_array[3];

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

   /* Determine chipset ID? */
   if (!intel_get_param(psp, I915_PARAM_CHIPSET_ID,
			&intelScreen->deviceID))
      return GL_FALSE;

   /* Determine if IRQs are active? */
   if (!intel_get_param(psp, I915_PARAM_IRQ_ACTIVE,
			&intelScreen->irq_active))
      return GL_FALSE;

   /* Determine if batchbuffers are allowed */
   if (!intel_get_param(psp, I915_PARAM_ALLOW_BATCHBUFFER,
			&intelScreen->allow_batchbuffer))
      return GL_FALSE;

   if (!intelScreen->allow_batchbuffer) {
      fprintf(stderr, "batch buffer not allowed\n");
      return GL_FALSE;
   }

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
   .HandleDrawableConfig = intelHandleDrawableConfig,
   .HandleBufferAttach	 = intelHandleBufferAttach,
};
