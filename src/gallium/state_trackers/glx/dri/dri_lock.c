/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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


#include "pipe/p_thread.h"
#include "dri_context.h"
#include "xf86drm.h"

pipe_static_mutex( lockMutex );

static void
dri_contended_lock(struct dri_context *ctx)
{
   __DRIdrawablePrivate *dPriv = ctx->dPriv;
   __DRIcontextPrivate *cPriv = ctx->cPriv;
   __DRIscreenPrivate *sPriv = cPriv->driScreenPriv;

   drmGetLock(sPriv->fd, cPriv->hHWContext, 0);

   /* Perform round trip communication with server (including dropping
    * and retaking the above lock) to update window dimensions:
    */
   if (dPriv)
      DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);
}


/* Lock the hardware and validate our state.
 */
void dri_lock_hardware( struct dri_context *ctx )
{
   __DRIcontextPrivate *cPriv = ctx->cPriv;
   __DRIscreenPrivate *sPriv = cPriv->driScreenPriv;
   char __ret = 0;

   pipe_mutex_lock(lockMutex);
   assert(!ctx->locked);

   DRM_CAS((drmLock *) &sPriv->pSAREA->lock,
           cPriv->hHWContext,
           (DRM_LOCK_HELD | cPriv->hHWContext),
           __ret);

   if (__ret)
      dri_contended_lock( ctx );

   ctx->locked = TRUE;
}


/* Unlock the hardware using the global current context
 */
void dri_unlock_hardware( struct dri_context *ctx )
{
   __DRIcontextPrivate *cPriv = ctx->cPriv;
   __DRIscreenPrivate *sPriv = cPriv->driScreenPriv;

   assert(ctx->locked);
   ctx->locked = FALSE;

   DRM_UNLOCK(sPriv->fd, 
              (drmLock *) &sPriv->pSAREA->lock,
              cPriv->hHWContext);

   pipe_mutex_unlock(lockMutex);
}
