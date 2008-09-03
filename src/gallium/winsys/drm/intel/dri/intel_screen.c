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

#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"

#include "intel_context.h"
#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_swapbuffers.h"

#include "i830_dri.h"
#include "ws_dri_bufpool.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_inlines.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_cb_fbo.h"

static void
intelCreateSurface(struct intel_screen *intelScreen, struct pipe_winsys *winsys, unsigned handle);

static void
intelCreateSurface(struct intel_screen *intelScreen, struct pipe_winsys *winsys, unsigned handle)
{
   struct pipe_screen *screen = intelScreen->base.screen;
   struct pipe_texture *texture;
   struct pipe_texture templat;
   struct pipe_surface *surface;
   struct pipe_buffer *buffer;
   unsigned pitch;

   assert(intelScreen->front.cpp == 4);

   buffer = intel_be_buffer_from_handle(&intelScreen->base,
                                        "front", handle);

   if (!buffer)
      return;

   intelScreen->front.buffer = dri_bo(buffer);

   memset(&templat, 0, sizeof(templat));
   templat.tex_usage |= PIPE_TEXTURE_USAGE_DISPLAY_TARGET;
   templat.target = PIPE_TEXTURE_2D;
   templat.last_level = 0;
   templat.depth[0] = 1;
   templat.format = PIPE_FORMAT_A8R8G8B8_UNORM;
   templat.width[0] = intelScreen->front.width;
   templat.height[0] = intelScreen->front.height;
   pf_get_block(templat.format, &templat.block);
   pitch = intelScreen->front.pitch;

   texture = screen->texture_blanket(screen,
                                     &templat,
                                     &pitch,
                                     buffer);

   /* Unref the buffer we don't need it anyways */
   pipe_buffer_reference(screen, &buffer, NULL);

   surface = screen->get_tex_surface(screen,
                                     texture,
                                     0,
                                     0,
                                     0,
                                     PIPE_BUFFER_USAGE_GPU_WRITE);

   intelScreen->front.texture = texture;
   intelScreen->front.surface = surface;
}

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN DRI_CONF_SECTION_PERFORMANCE
   DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
   DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
   DRI_CONF_SECTION_END DRI_CONF_SECTION_QUALITY
//   DRI_CONF_FORCE_S3TC_ENABLE(false)
   DRI_CONF_ALLOW_LARGE_TEXTURES(1)
   DRI_CONF_SECTION_END DRI_CONF_END;

const uint __driNConfigOptions = 3;

#ifdef USE_NEW_INTERFACE
static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /*USE_NEW_INTERFACE */

extern const struct dri_extension card_extensions[];




static void
intelPrintDRIInfo(struct intel_screen * intelScreen,
                  __DRIscreenPrivate * sPriv, I830DRIPtr gDRIPriv)
{
   fprintf(stderr, "*** Front size:   0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->front.size, intelScreen->front.offset,
           intelScreen->front.pitch);
   fprintf(stderr, "*** Memory : 0x%x\n", gDRIPriv->mem);
}


#if 0
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
#endif


/**
 * Use the information in the sarea to update the screen parameters
 * related to screen rotation. Needs to be called locked.
 */
void
intelUpdateScreenRotation(__DRIscreenPrivate * sPriv, drmI830Sarea * sarea)
{
   struct intel_screen *intelScreen = intel_screen(sPriv);

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

#if 0 /* JB not important */
   if (!sarea->front_handle)
      return;

   if (drmMap(sPriv->fd,
	      sarea->front_handle,
	      intelScreen->front.size,
	      (drmAddress *) & intelScreen->front.map) != 0) {
      fprintf(stderr, "drmMap(frontbuffer) failed!\n");
      return;
   }
#endif

#if 0 /* JB */
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
#else
   if (intelScreen->base.staticPool) {
      if (intelScreen->front.buffer) {
	 driBOUnReference(intelScreen->front.buffer);
	 pipe_surface_reference(&intelScreen->front.surface, NULL);
	 pipe_texture_reference(&intelScreen->front.texture, NULL);
      }
      intelCreateSurface(intelScreen, &intelScreen->base.base, sarea->front_bo_handle);
   }
#endif
}


boolean
intelCreatePools(__DRIscreenPrivate * sPriv)
{
   //unsigned batchPoolSize = 1024*1024;
   struct intel_screen *intelScreen = intel_screen(sPriv);

   if (intelScreen->havePools)
      return GL_TRUE;

   intelScreen->havePools = GL_TRUE;

   intelUpdateScreenRotation(sPriv, intelScreen->sarea);

   return GL_TRUE;
}

static const char *
intel_get_name( struct pipe_winsys *winsys )
{
   return "Intel/DRI/ttm";
}

/*
 * The state tracker (should!) keep track of whether the fake
 * frontbuffer has been touched by any rendering since the last time
 * we copied its contents to the real frontbuffer.  Our task is easy:
 */
static void
intel_flush_frontbuffer( struct pipe_winsys *winsys,
                         struct pipe_surface *surf,
                         void *context_private)
{
   struct intel_context *intel = (struct intel_context *) context_private;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   intelDisplaySurface(dPriv, surf, NULL);
}

static boolean
intelInitDriver(__DRIscreenPrivate * sPriv)
{
   struct intel_screen *intelScreen;
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
   intelScreen = CALLOC_STRUCT(intel_screen);
   if (!intelScreen)
      return GL_FALSE;

   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

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

   intelScreen->base.base.flush_frontbuffer = intel_flush_frontbuffer;
   intelScreen->base.base.get_name = intel_get_name;
   intel_be_init_device(&intelScreen->base, sPriv->fd, intelScreen->deviceID);

   return GL_TRUE;
}


static void
intelDestroyScreen(__DRIscreenPrivate * sPriv)
{
   struct intel_screen *intelScreen = intel_screen(sPriv);

   intel_be_destroy_device(&intelScreen->base);
   /*  intelUnmapScreenRegions(intelScreen); */

   FREE(intelScreen);
   sPriv->private = NULL;
}


/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static boolean
intelCreateBuffer(__DRIscreenPrivate * driScrnPriv,
                  __DRIdrawablePrivate * driDrawPriv,
                  const __GLcontextModes * visual, boolean isPixmap)
{
   if (isPixmap) {
      return GL_FALSE;          /* not implemented */
   }
   else {
      enum pipe_format colorFormat, depthFormat, stencilFormat;
      struct intel_framebuffer *intelfb = CALLOC_STRUCT(intel_framebuffer);

      if (!intelfb)
         return GL_FALSE;

      if (visual->redBits == 5)
         colorFormat = PIPE_FORMAT_R5G6B5_UNORM;
      else
         colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

      if (visual->depthBits == 16)
         depthFormat = PIPE_FORMAT_Z16_UNORM;
      else if (visual->depthBits == 24)
         depthFormat = PIPE_FORMAT_S8Z24_UNORM;
      else
         depthFormat = PIPE_FORMAT_NONE;

      if (visual->stencilBits == 8)
         stencilFormat = PIPE_FORMAT_S8Z24_UNORM;
      else
         stencilFormat = PIPE_FORMAT_NONE;

      intelfb->stfb = st_create_framebuffer(visual,
                                            colorFormat,
                                            depthFormat,
                                            stencilFormat,
                                            driDrawPriv->w,
                                            driDrawPriv->h,
                                            (void*) intelfb);
      if (!intelfb->stfb) {
         free(intelfb);
         return GL_FALSE;
      }

      driDrawPriv->driverPrivate = (void *) intelfb;
      return GL_TRUE;
   }
}

static void
intelDestroyBuffer(__DRIdrawablePrivate * driDrawPriv)
{
   struct intel_framebuffer *intelfb = intel_framebuffer(driDrawPriv);
   assert(intelfb->stfb);
   st_unreference_framebuffer(&intelfb->stfb);
   free(intelfb);
}


/**
 * Get information about previous buffer swaps.
 */
static int
intelGetSwapInfo(__DRIdrawablePrivate * dPriv, __DRIswapInfo * sInfo)
{
   if ((dPriv == NULL) || (dPriv->driverPrivate == NULL)
       || (sInfo == NULL)) {
      return -1;
   }

   return 0;
}


static void
intelSetTexOffset(__DRIcontext *pDRICtx, int texname,
		  unsigned long long offset, int depth, uint pitch)
{
   abort();
#if 0
   struct intel_context *intel = (struct intel_context*)
      ((__DRIcontextPrivate*)pDRICtx->private)->driverPrivate;
   struct gl_texture_object *tObj = _mesa_lookup_texture(&intel->ctx, texname);
   struct st_texture_object *stObj = st_texture_object(tObj);

   if (!stObj)
      return;

   if (stObj->pt)
      st->pipe->texture_release(intel->st->pipe, &stObj->pt);

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
                 unsigned stencil_bits, boolean have_back_buffer)
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

   uint8_t depth_bits_array[3];
   uint8_t stencil_bits_array[3];
   uint8_t msaa_samples_array[1];


   depth_bits_array[0] = 0;
   depth_bits_array[1] = depth_bits;
   depth_bits_array[2] = depth_bits;
   msaa_samples_array[0] = 0;

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
                       back_buffer_factor, msaa_samples_array, 1, GLX_TRUE_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }
   if (!driFillInModes(&m, fb_format, fb_type,
                       depth_bits_array, stencil_bits_array,
                       depth_buffer_factor, back_buffer_modes,
                       back_buffer_factor, msaa_samples_array, 1, GLX_DIRECT_COLOR)) {
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

      /* Calling driInitExtensions here, with a NULL context pointer,
       * does not actually enable the extensions.  It just makes sure
       * that all the dispatch offsets for all the extensions that
       * *might* be enables are known.  This is needed because the
       * dispatch offsets need to be known when _mesa_context_create
       * is called, but we can't enable the extensions until we have a
       * context pointer.
       *
       * Hello chicken.  Hello egg.  How are you two today?
       */
      driInitExtensions(NULL, card_extensions, GL_FALSE);
   }

   return (void *) psp;
}

