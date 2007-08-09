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
#include "intel_batchbuffer.h"
#include "intel_buffers.h"
/*#include "intel_tex.h"*/
#include "intel_ioctl.h"
#include "intel_fbo.h"

#include "i830_dri.h"
#include "dri_bufpool.h"

#include "pipe/p_context.h"
#include "state_tracker/st_cb_fbo.h"



PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN DRI_CONF_SECTION_PERFORMANCE
   DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
   DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
   DRI_CONF_SECTION_END DRI_CONF_SECTION_QUALITY
   DRI_CONF_FORCE_S3TC_ENABLE(false)
   DRI_CONF_ALLOW_LARGE_TEXTURES(1)
   DRI_CONF_SECTION_END DRI_CONF_END;
     const GLuint __driNConfigOptions = 4;

#ifdef USE_NEW_INTERFACE
     static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /*USE_NEW_INTERFACE */

     extern const struct dri_extension card_extensions[];





static void
intelPrintDRIInfo(intelScreenPrivate * intelScreen,
                  __DRIscreenPrivate * sPriv, I830DRIPtr gDRIPriv)
{
   fprintf(stderr, "*** Front size:   0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->front.size, intelScreen->front.offset,
           intelScreen->front.pitch);
   fprintf(stderr, "*** Memory : 0x%x\n", gDRIPriv->mem);
}


static void
intelPrintSAREA(const drmI830Sarea * sarea)
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
   fprintf(stderr, "SAREA: rotation: %d\n", sarea->rotation);
   fprintf(stderr,
           "SAREA: rotated offset: 0x%08x  size: 0x%x\n",
           sarea->rotated_offset, sarea->rotated_size);
   fprintf(stderr, "SAREA: rotated pitch: %d\n", sarea->rotated_pitch);
}



/**
 * Use the information in the sarea to update the screen parameters
 * related to screen rotation. Needs to be called locked.
 */
void
intelUpdateScreenRotation(__DRIscreenPrivate * sPriv, drmI830Sarea * sarea)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

   if (intelScreen->front.map) {
      drmUnmap(intelScreen->front.map, intelScreen->front.size);
      intelScreen->front.map = NULL;
   }

   if (intelScreen->front.buffer)
      driDeleteBuffers(1, &intelScreen->front.buffer);

   intelScreen->front.width = sarea->width;
   intelScreen->front.height = sarea->height;
   intelScreen->front.offset = sarea->front_offset;
   intelScreen->front.pitch = sarea->pitch * intelScreen->front.cpp;
   intelScreen->front.size = sarea->front_size;
   intelScreen->front.handle = sarea->front_handle;

   assert( sarea->front_size >=
	   intelScreen->front.pitch * intelScreen->front.height );

   if (!sarea->front_handle)
      return;

   if (drmMap(sPriv->fd,
	      sarea->front_handle,
	      intelScreen->front.size,
	      (drmAddress *) & intelScreen->front.map) != 0) {
      _mesa_problem(NULL, "drmMap(frontbuffer) failed!");
      return;
   }

   if (intelScreen->staticPool) {
      driGenBuffers(intelScreen->staticPool, "static region", 1,
		    &intelScreen->front.buffer, 64,
		    DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_NO_MOVE |
		    DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0);
      
      driBOSetStatic(intelScreen->front.buffer, 
		     intelScreen->front.offset, 		  
		     intelScreen->front.pitch * intelScreen->front.height, 
		     intelScreen->front.map, 0);
   }
}



GLboolean
intelCreatePools(intelScreenPrivate *intelScreen)
{
   unsigned batchPoolSize = 1024*1024;
   __DRIscreenPrivate * sPriv = intelScreen->driScrnPriv;

   if (intelScreen->havePools)
      return GL_TRUE;

   batchPoolSize /= BATCH_SZ;
   intelScreen->regionPool = driDRMPoolInit(sPriv->fd);

   if (!intelScreen->regionPool)
      return GL_FALSE;

   intelScreen->staticPool = driDRMStaticPoolInit(sPriv->fd);

   if (!intelScreen->staticPool)
      return GL_FALSE;

   intelScreen->texPool = intelScreen->regionPool;

   intelScreen->batchPool = driBatchPoolInit(sPriv->fd,
                                             DRM_BO_FLAG_EXE |
                                             DRM_BO_FLAG_MEM_TT |
                                             DRM_BO_FLAG_MEM_LOCAL,
                                             BATCH_SZ, 
					     batchPoolSize, 5);
   if (!intelScreen->batchPool) {
      fprintf(stderr, "Failed to initialize batch pool - possible incorrect agpgart installed\n");
      return GL_FALSE;
   }
   
   intelScreen->havePools = GL_TRUE;

   intelUpdateScreenRotation(sPriv, intelScreen->sarea);

   return GL_TRUE;
}


static GLboolean
intelInitDriver(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen;
   I830DRIPtr gDRIPriv = (I830DRIPtr) sPriv->pDevPriv;

   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
      (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->
                                      getProcAddress("glxEnableExtension"));
   void *const psc = sPriv->psc->screenConfigs;

   if (sPriv->devPrivSize != sizeof(I830DRIRec)) {
      fprintf(stderr,
              "\nERROR!  sizeof(I830DRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   intelScreen = (intelScreenPrivate *) CALLOC(sizeof(intelScreenPrivate));
   if (!intelScreen) 
      return GL_FALSE;

   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = sPriv;
   sPriv->private = (void *) intelScreen;

   intelScreen->sarea = (drmI830Sarea *) (((GLubyte *) sPriv->pSAREA) +
					  gDRIPriv->sarea_priv_offset);
   intelScreen->deviceID = gDRIPriv->deviceID;
   intelScreen->front.cpp = gDRIPriv->cpp;
   intelScreen->drmMinor = sPriv->drmMinor;

   assert(gDRIPriv->bitsPerPixel == 16 ||
	  gDRIPriv->bitsPerPixel == 32);

   intelUpdateScreenRotation(sPriv, intelScreen->sarea);

   if (0)
      intelPrintDRIInfo(intelScreen, sPriv, gDRIPriv);

   if (glx_enable_extension != NULL) {
      (*glx_enable_extension) (psc, "GLX_SGI_swap_control");
      (*glx_enable_extension) (psc, "GLX_SGI_video_sync");
      (*glx_enable_extension) (psc, "GLX_MESA_swap_control");
      (*glx_enable_extension) (psc, "GLX_MESA_swap_frame_usage");
      (*glx_enable_extension) (psc, "GLX_SGI_make_current_read");
   }

   return GL_TRUE;
}


static void
intelDestroyScreen(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

//   intelUnmapScreenRegions(intelScreen);

   if (intelScreen->havePools) {
      driPoolTakeDown(intelScreen->regionPool);
      driPoolTakeDown(intelScreen->staticPool);
      driPoolTakeDown(intelScreen->batchPool);
   }
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

      {
	 /* fake frontbuffer */
	 /* XXX allocation should only happen in the unusual case
            it's actually needed */
         intel_fb->color_rb[0]
            = intel_new_renderbuffer_fb(rgbFormat);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT,
				&intel_fb->color_rb[0]->Base);
      }

      if (mesaVis->doubleBufferMode) {
#if 01
         intel_fb->color_rb[1]
            = intel_new_renderbuffer_fb(rgbFormat);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT,
				&intel_fb->color_rb[1]->Base);
#else
         intel_fb->color_rb[1]
            = st_new_renderbuffer_fb(rgbFormat);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT,
				&intel_fb->color_rb[1]->Base);
#endif
      }

      if (mesaVis->depthBits == 24 && mesaVis->stencilBits == 8) {
         /* combined depth/stencil buffer */
         struct intel_renderbuffer *depthStencilRb
            = intel_new_renderbuffer_fb(GL_DEPTH24_STENCIL8_EXT);
         /* note: bind RB to two attachment points */
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH,
				&depthStencilRb->Base);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_STENCIL,
				&depthStencilRb->Base);
      }
      else if (mesaVis->depthBits == 16) {
         /* just 16-bit depth buffer, no hw stencil */
         struct intel_renderbuffer *depthRb
            = intel_new_renderbuffer_fb(GL_DEPTH_COMPONENT16);
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


static void
intelSetTexOffset(__DRIcontext *pDRICtx, GLint texname,
		  unsigned long long offset, GLint depth, GLuint pitch)
{
   abort();
#if 0
   struct intel_context *intel = (struct intel_context*)
      ((__DRIcontextPrivate*)pDRICtx->private)->driverPrivate;
   struct gl_texture_object *tObj = _mesa_lookup_texture(&intel->ctx, texname);
   struct st_texture_object *stObj = st_texture_object(tObj);

   if (!stObj)
      return;

   if (stObj->mt)
      st_miptree_release(intel->pipe, &stObj->mt);

   stObj->imageOverride = GL_TRUE;
   stObj->depthOverride = depth;
   stObj->pitchOverride = pitch;

   if (offset)
      stObj->textureOffset = offset;
#endif
}


static const struct __DriverAPIRec intelAPI = {
   .InitDriver = intelInitDriver,
   .DestroyScreen = intelDestroyScreen,
   .CreateContext = intelCreateContext,
   .DestroyContext = intelDestroyContext,
   .CreateBuffer = intelCreateBuffer,
   .DestroyBuffer = intelDestroyBuffer,
   .SwapBuffers = intelSwapBuffers,
   .MakeCurrent = intelMakeCurrent,
   .UnbindContext = intelUnbindContext,
   .GetSwapInfo = intelGetSwapInfo,
   .GetMSC = driGetMSC32,
   .WaitForMSC = driWaitForMSC32,
   .WaitForSBC = NULL,
   .SwapBuffersMSC = NULL,
   .CopySubBuffer = intelCopySubBuffer,
   .setTexOffset = intelSetTexOffset,
};


static __GLcontextModes *
intelFillInModes(unsigned pixel_bits, unsigned depth_bits,
                 unsigned stencil_bits, GLboolean have_back_buffer)
{
   __GLcontextModes *modes;
   __GLcontextModes *m;
   unsigned num_modes;
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;

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

   num_modes = depth_buffer_factor * back_buffer_factor * 4;

   if (pixel_bits == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   }
   else {
      fb_format = GL_BGRA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   modes =
      (*dri_interface->createContextModes) (num_modes,
                                            sizeof(__GLcontextModes));
   m = modes;
   if (!driFillInModes(&m, fb_format, fb_type,
                       depth_bits_array, stencil_bits_array,
                       depth_buffer_factor, back_buffer_modes,
                       back_buffer_factor, GLX_TRUE_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }
   if (!driFillInModes(&m, fb_format, fb_type,
                       depth_bits_array, stencil_bits_array,
                       depth_buffer_factor, back_buffer_modes,
                       back_buffer_factor, GLX_DIRECT_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   /* Mark the visual as slow if there are "fake" stencil bits.
    */
   for (m = modes; m != NULL; m = m->next) {
      if ((m->stencilBits != 0) && (m->stencilBits != stencil_bits)) {
         m->visualRating = GLX_SLOW_CONFIG;
      }
   }

   return modes;
}


/**
 * This is the bootstrap function for the driver.  libGL supplies all of the
 * requisite information about the system, and the driver initializes itself.
 * This routine also fills in the linked list pointed to by \c driver_modes
 * with the \c __GLcontextModes that the driver can support for windows or
 * pbuffers.
 * 
 * \return A pointer to a \c __DRIscreenPrivate on success, or \c NULL on 
 *         failure.
 */
PUBLIC void *
__driCreateNewScreen_20050727(__DRInativeDisplay * dpy, int scrn,
                              __DRIscreen * psc,
                              const __GLcontextModes * modes,
                              const __DRIversion * ddx_version,
                              const __DRIversion * dri_version,
                              const __DRIversion * drm_version,
                              const __DRIframebuffer * frame_buffer,
                              drmAddress pSAREA, int fd,
                              int internal_api_version,
                              const __DRIinterfaceMethods * interface,
                              __GLcontextModes ** driver_modes)
{
   __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = { 1, 7, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 7, 0 };

   dri_interface = interface;

   if (!driCheckDriDdxDrmVersions2("i915",
                                   dri_version, &dri_expected,
                                   ddx_version, &ddx_expected,
                                   drm_version, &drm_expected)) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
                                  ddx_version, dri_version, drm_version,
                                  frame_buffer, pSAREA, fd,
                                  internal_api_version, &intelAPI);

   if (psp != NULL) {
      I830DRIPtr dri_priv = (I830DRIPtr) psp->pDevPriv;
      *driver_modes = intelFillInModes(dri_priv->cpp * 8,
                                       (dri_priv->cpp == 2) ? 16 : 24,
                                       (dri_priv->cpp == 2) ? 0 : 8, 1);

      /* Calling driInitExtensions here, with a NULL context pointer, does not actually
       * enable the extensions.  It just makes sure that all the dispatch offsets for all
       * the extensions that *might* be enables are known.  This is needed because the
       * dispatch offsets need to be known when _mesa_context_create is called, but we can't
       * enable the extensions until we have a context pointer.
       *
       * Hello chicken.  Hello egg.  How are you two today?
       */
      driInitExtensions(NULL, card_extensions, GL_FALSE);
   }

   return (void *) psp;
}

struct intel_context *intelScreenContext(intelScreenPrivate *intelScreen)
{
  /*
   * This should probably change to have the screen allocate a dummy
   * context at screen creation. For now just use the current context.
   */

  GET_CURRENT_CONTEXT(ctx);
  if (ctx == NULL) {
/*     _mesa_problem(NULL, "No current context in intelScreenContext\n");
     return NULL; */
     /* need a context for the first time makecurrent is called (for hw lock
        when allocating priv buffers) */
     if (intelScreen->dummyctxptr == NULL) {
        _mesa_problem(NULL, "No current context in intelScreenContext\n");
        return NULL;
     }
     return intelScreen->dummyctxptr;
  }
  return intel_context(ctx);

}

