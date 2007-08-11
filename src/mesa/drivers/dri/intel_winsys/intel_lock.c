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
#include "extensions.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_buffers.h"

#include "i830_dri.h"


#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"

#include "drirenderbuffer.h"
#include "vblank.h"
#include "utils.h"
#include "xmlpool.h"            /* for symbolic values of enum-type options */



_glthread_DECLARE_STATIC_MUTEX( lockMutex );


static void
intelContendedLock(struct intel_context *intel, GLuint flags)
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   __DRIscreenPrivate *sPriv = intel->driScreen;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;
   drmI830Sarea *sarea = intel->sarea;

   drmGetLock(intel->driFd, intel->hHWContext, flags);

   DBG(LOCK, "%s - got contended lock\n", __progname);

   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   if (dPriv)
      DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);

   if (sarea->width != intelScreen->front.width ||
       sarea->height != intelScreen->front.height) {

      intelUpdateScreenRotation(sPriv, sarea);
   }

#if 0
   if (sarea->width != intel->width ||
       sarea->height != intel->height ||
       sarea->rotation != intel->current_rotation) {
      int numClipRects = intel->numClipRects;

      /*
       * FIXME: Really only need to do this when drawing to a
       * common back- or front buffer.
       */

      /*
       * This will essentially drop the outstanding batchbuffer on the floor.
       */
      intel->numClipRects = 0;

      INTEL_FIREVERTICES(intel);

      if (intel->batch->map != intel->batch->ptr)
	 intel_batchbuffer_flush(intel->batch);

      intel->numClipRects = numClipRects;

      /* force window update */
      intel->lastStamp = 0;

      intel->width = sarea->width;
      intel->height = sarea->height;
      intel->current_rotation = sarea->rotation;
   }
#endif
}

/* Lock the hardware and validate our state.
 */
void LOCK_HARDWARE( struct intel_context *intel )
{
    char __ret=0;
    struct intel_framebuffer *intel_fb = NULL;
    int curbuf;

    _glthread_LOCK_MUTEX(lockMutex);
    assert(!intel->locked);

    if (intel->driDrawable) {
       intel_fb = intel->driDrawable->driverPrivate;
    }

    curbuf = 0; /* current draw buf: 0 = front, 1 = back */

    if (intel_fb && intel_fb->vblank_flags &&
	!(intel_fb->vblank_flags & VBLANK_FLAG_NO_IRQ) &&
	(intel_fb->vbl_waited - intel_fb->vbl_pending[curbuf]) > (1<<23)) {
       drmVBlank vbl;

       vbl.request.type = DRM_VBLANK_ABSOLUTE;

       if ( intel_fb->vblank_flags & VBLANK_FLAG_SECONDARY ) {
          vbl.request.type |= DRM_VBLANK_SECONDARY;
       }

       vbl.request.sequence = intel_fb->vbl_pending[curbuf];
       drmWaitVBlank(intel->driFd, &vbl);
       intel_fb->vbl_waited = vbl.reply.sequence;
    }

    DRM_CAS(intel->driHwLock, intel->hHWContext,
            (DRM_LOCK_HELD|intel->hHWContext), __ret);

    if (__ret)
       intelContendedLock( intel, 0 );

    DBG(LOCK, "%s - locked\n", __progname);

    intel->locked = 1;
}


  /* Unlock the hardware using the global current context 
   */
void UNLOCK_HARDWARE( struct intel_context *intel )
{
   assert(intel->locked);
   intel->locked = 0;

   DRM_UNLOCK(intel->driFd, intel->driHwLock, intel->hHWContext);

   _glthread_UNLOCK_MUTEX(lockMutex);

   DBG(LOCK, "%s - unlocked\n", __progname);
} 
