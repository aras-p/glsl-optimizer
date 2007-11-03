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

#include "i830_dri.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_swapbuffers.h"
#include "intel_winsys.h"
#include "intel_batchbuffer.h"

#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_context.h"

#include "utils.h"
#include "xmlpool.h"            /* for symbolic values of enum-type options */


#ifdef DEBUG
int __intel_debug = 0;
#endif




/**
 * Extension strings exported by the intel driver.
 *
 * \note
 * It appears that ARB_texture_env_crossbar has "disappeared" compared to the
 * old i830-specific driver.
 */
const struct dri_extension card_extensions[] = {
   {NULL, NULL}
};



#ifdef DEBUG
static const struct dri_debug_control debug_control[] = {
   {"ioctl", DEBUG_IOCTL},
   {"bat", DEBUG_BATCH},
   {"lock", DEBUG_LOCK},
   {"swap", DEBUG_SWAP},
   {NULL, 0}
};
#endif



GLboolean
intelCreateContext(const __GLcontextModes * mesaVis,
                   __DRIcontextPrivate * driContextPriv,
                   void *sharedContextPrivate)
{
   struct intel_context *intel = CALLOC_STRUCT(intel_context);

   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;
   drmI830Sarea *saPriv = intelScreen->sarea;
   int fthrottle_mode;
   GLboolean havePools;
   struct pipe_context *pipe;

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driScreen = sPriv;
   intel->sarea = saPriv;

   driParseConfigFiles(&intel->optionCache, &intelScreen->optionCache,
                       intel->driScreen->myNum, "i915");


   /*
    * memory pools
    */
   DRM_LIGHT_LOCK(sPriv->fd, &sPriv->pSAREA->lock, driContextPriv->hHWContext);
   havePools = intelCreatePools(intelScreen);
   DRM_UNLOCK(sPriv->fd, &sPriv->pSAREA->lock, driContextPriv->hHWContext);
   if (!havePools)
      return GL_FALSE;


   /* Dri stuff */
   intel->hHWContext = driContextPriv->hHWContext;
   intel->driFd = sPriv->fd;
   intel->driHwLock = (drmLock *) & sPriv->pSAREA->lock;

   fthrottle_mode = driQueryOptioni(&intel->optionCache, "fthrottle_mode");
   intel->iw.irq_seq = -1;
   intel->irqsEmitted = 0;

   intel->batch = intel_batchbuffer_alloc(intel);
   intel->last_swap_fence = NULL;
   intel->first_swap_fence = NULL;

#ifdef DEBUG
   __intel_debug = driParseDebugString(getenv("INTEL_DEBUG"), debug_control);
#endif


   /*
    * Pipe-related setup
    */
   if (!getenv("INTEL_HW")) {
      pipe = intel_create_softpipe( intel );
   }
   else {
      switch (intel->intelScreen->deviceID) {
      case PCI_CHIP_I945_G:
      case PCI_CHIP_I945_GM:
      case PCI_CHIP_I945_GME:
      case PCI_CHIP_G33_G:
      case PCI_CHIP_Q33_G:
      case PCI_CHIP_Q35_G:
      case PCI_CHIP_I915_G:
      case PCI_CHIP_I915_GM:
	 pipe = intel_create_i915simple( intel );
	 break;
      default:
	 _mesa_printf("Unknown PCIID %x in %s, using software driver\n", 
		      intel->intelScreen->deviceID, __FUNCTION__);

	 pipe = intel_create_softpipe( intel );
	 break;
      }
   }

   intel->st = st_create_context2(pipe,  mesaVis, NULL);
   intel->st->ctx->DriverCtx = intel;  /* hope to get rid of this... */

   return GL_TRUE;
}


void
intelDestroyContext(__DRIcontextPrivate * driContextPriv)
{
   struct intel_context *intel =
      (struct intel_context *) driContextPriv->driverPrivate;

   assert(intel);               /* should never be null */
   if (intel) {
      st_flush(intel->st);

      intel_batchbuffer_free(intel->batch);

      if (intel->last_swap_fence) {
	 driFenceFinish(intel->last_swap_fence, DRM_FENCE_TYPE_EXE, GL_TRUE);
	 driFenceUnReference(intel->last_swap_fence);
	 intel->last_swap_fence = NULL;
      }
      if (intel->first_swap_fence) {
	 driFenceFinish(intel->first_swap_fence, DRM_FENCE_TYPE_EXE, GL_TRUE);
	 driFenceUnReference(intel->first_swap_fence);
	 intel->first_swap_fence = NULL;
      }

      st_destroy_context2(intel->st);
   }
}


GLboolean
intelUnbindContext(__DRIcontextPrivate * driContextPriv)
{
   struct intel_context *intel
      = (struct intel_context *) driContextPriv->driverPrivate;
   st_flush(intel->st);
   return GL_TRUE;
}


GLboolean
intelMakeCurrent(__DRIcontextPrivate * driContextPriv,
                 __DRIdrawablePrivate * driDrawPriv,
                 __DRIdrawablePrivate * driReadPriv)
{
   if (driContextPriv) {
      struct intel_context *intel
         = (struct intel_context *) driContextPriv->driverPrivate;
      struct st_framebuffer *draw_fb
         = (struct st_framebuffer *) driDrawPriv->driverPrivate;
      struct st_framebuffer *read_fb
         = (struct st_framebuffer *) driReadPriv->driverPrivate;

      /* this is a hack so we have a valid context when the region allocation
         is done. Need a per-screen context? */
      intel->intelScreen->dummyctxptr = intel;

      st_make_current(intel->st, draw_fb, read_fb);

      /* update size of Mesa framebuffer(s) to match window */
      st_resize_framebuffer(draw_fb, driDrawPriv->w, driDrawPriv->h);
      if (driReadPriv != driDrawPriv) {
         st_resize_framebuffer(read_fb, driReadPriv->w, driReadPriv->h);
      }

      if ((intel->driDrawable != driDrawPriv) ||
	  (intel->lastStamp != driDrawPriv->lastStamp)) {
         intel->driDrawable = driDrawPriv;
         intelWindowMoved(intel);
         intel->lastStamp = driDrawPriv->lastStamp;
      }
   }
   else {
      st_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}
