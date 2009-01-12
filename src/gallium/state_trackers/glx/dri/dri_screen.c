/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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

#include "dri_context.h"
#include "dri_screen.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_inlines.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_cb_fbo.h"


PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN DRI_CONF_SECTION_PERFORMANCE
    DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
    DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
   DRI_CONF_SECTION_END DRI_CONF_SECTION_QUALITY
//   DRI_CONF_FORCE_S3TC_ENABLE(false)
    DRI_CONF_ALLOW_LARGE_TEXTURES(1)
   DRI_CONF_SECTION_END DRI_CONF_END;

const uint __driNConfigOptions = 3;

static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;

extern const struct dri_extension card_extensions[];



static const __DRIextension *driScreenExtensions[] = {
    &driReadDrawableExtension,
    &driCopySubBufferExtension.base,
    &driSwapControlExtension.base,
    &driFrameTrackingExtension.base,
    &driMediaStreamCounterExtension.base,
    NULL
};




static const char *
dri_get_name( struct pipe_winsys *winsys )
{
   return "dri";
}



static void
dri_destroy_screen(__DRIscreenPrivate * sPriv)
{
   struct dri_screen *screen = dri_screen(sPriv);

   screen->pipe_screen->destroy( screen->pipe_screen );
   screen->pipe_winsys->destroy( screen->pipe_winsys );
   FREE(screen);
   sPriv->private = NULL;
}


/**
 * Get information about previous buffer swaps.
 */
static int
dri_get_swap_info(__DRIdrawablePrivate * dPriv, 
                  __DRIswapInfo * sInfo)
{
   if (dPriv == NULL || 
       dPriv->driverPrivate == NULL ||
       sInfo == NULL) 
      return -1;
   else
      return 0;
}

static const __DRIconfig **
dri_fill_in_modes(__DRIscreenPrivate *psp,
                  unsigned pixel_bits )
{
   __DRIconfig **configs;
   __GLcontextModes *m;
   unsigned num_modes;
   uint8_t depth_bits_array[3];
   uint8_t stencil_bits_array[3];
   uint8_t msaa_samples_array[1];
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;
   int i;

   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML
   };

   depth_bits_array[0] = 0;
   depth_bits_array[1] = depth_bits;
   depth_bits_array[2] = depth_bits;

   stencil_bits_array[0] = 0;   /* no depth or stencil */
   stencil_bits_array[1] = 0;   /* z24x8 */
   stencil_bits_array[2] = 8;   /* z24s8 */

   msaa_samples_array[0] = 0;

   depth_buffer_factor = 3;
   back_buffer_factor = 1;

   num_modes = depth_buffer_factor * back_buffer_factor * 4;

   if (pixel_bits == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   }
   else {
      fb_format = GL_BGRA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   configs = driCreateConfigs(fb_format, fb_type,
			      depth_bits_array, 
                              stencil_bits_array, depth_buffer_factor, 
                              back_buffer_modes, back_buffer_factor,
                              msaa_samples_array, 1);
   if (configs == NULL) {
      debug_printf("%s: driCreateConfigs failed\n", __FUNCTION__);
      return NULL;
   }

   return configs;
}



/* This is the driver specific part of the createNewScreen entry point.
 * 
 * Returns the __GLcontextModes supported by this driver.
 */
static const __DRIconfig **dri_init_screen(__DRIscreenPrivate *sPriv)
{
   static const __DRIversion ddx_expected = { 1, 6, 0 }; /* hw query */
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 5, 0 }; /* hw query */
   struct dri_screen *screen;

   if (!driCheckDriDdxDrmVersions2("dri",
                                   &sPriv->dri_version, &dri_expected,
                                   &sPriv->ddx_version, &ddx_expected,
                                   &sPriv->drm_version, &drm_expected)) {
      return NULL;
   }

   /* Set up dispatch table to cope with all known extensions:
    */
   driInitExtensions( NULL, card_extensions, GL_FALSE );


   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      goto fail;

   screen->sPriv = sPriv;
   sPriv->private = (void *) screen;


   /* Search the registered winsys' for one that likes this sPriv.
    * This is required in situations where multiple devices speak to
    * the same DDX and are built into the same binary.  
    *
    * Note that cases like Intel i915 vs i965 doesn't fall into this
    * category because they are built into separate binaries.
    *
    * Nonetheless, it's healthy to keep that level of detail out of
    * this state_tracker.
    */
   for (i = 0; 
        i < dri1_winsys_count && 
           screen->st_winsys == NULL; 
        i++) 
   {
      screen->dri_winsys = 
         dri_winsys[i]->check_dri_privates( sPriv->pDevPriv,
                                            sPriv->pSAREA
                                            /* versions, etc?? */));
   }
                                             

   driParseOptionInfo(&screen->optionCache,
                      __driConfigOptions, 
                      __driNConfigOptions);


   /* Plug our info back into the __DRIscreenPrivate:
    */
   sPriv->private = (void *) screen;
   sPriv->extensions = driScreenExtensions;

   return dri_fill_in_modes(sPriv, 
                            dri_priv->cpp * 8,
                            24,
                            8,
                            1);
fail:
   return NULL;
}



const struct __DriverAPIRec driDriverAPI = {
   .InitScreen		 = dri_init_screen,
   .DestroyScreen	 = dri_destroy_screen,
   .CreateContext	 = dri_create_context,
   .DestroyContext	 = dri_destroy_context,
   .CreateBuffer	 = dri_create_buffer,
   .DestroyBuffer	 = dri_destroy_buffer,
   .SwapBuffers		 = dri_swap_buffers,
   .MakeCurrent		 = dri_make_current,
   .UnbindContext	 = dri_unbind_context,
   .GetSwapInfo		 = dri_get_swap_info,
   .GetDrawableMSC	 = driDrawableGetMSC32,
   .WaitForMSC		 = driWaitForMSC32,
   .CopySubBuffer	 = dri_copy_sub_buffer,

   //.InitScreen2		 = dri_init_screen2,
};
