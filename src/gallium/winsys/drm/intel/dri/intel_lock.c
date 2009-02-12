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
#include "pipe/p_thread.h"
#include <GL/internal/glcore.h>
#include "state_tracker/st_public.h"
#include "intel_context.h"
#include "i830_dri.h"



pipe_static_mutex( lockMutex );


static void
intelContendedLock(struct intel_context *intel, uint flags)
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   __DRIscreenPrivate *sPriv = intel->driScreen;
   struct intel_screen *intelScreen = intel_screen(sPriv);
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
}


/* Lock the hardware and validate our state.
 */
void LOCK_HARDWARE( struct intel_context *intel )
{
    char __ret = 0;

    pipe_mutex_lock(lockMutex);
    assert(!intel->locked);

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

   pipe_mutex_unlock(lockMutex);

   DBG(LOCK, "%s - unlocked\n", __progname);
}
