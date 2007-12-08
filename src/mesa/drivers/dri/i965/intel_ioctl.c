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


#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#include "mtypes.h"
#include "context.h"
#include "swrast/swrast.h"

#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "drm.h"
#include "dri_bufmgr.h"
#include "intel_bufmgr_ttm.h"
#include "i915_drm.h"

static void intelWaitIdleLocked( struct intel_context *intel )
{
   unsigned int fence;

   if (INTEL_DEBUG & DEBUG_SYNC)
      fprintf(stderr, "waiting for idle\n");

   fence = intelEmitIrqLocked(intel->intelScreen);
   intelWaitIrq(intel->intelScreen, fence);
}

int intelEmitIrqLocked( intelScreenPrivate *intelScreen )
{
   int seq = 1;

   if (!intelScreen->no_hw) {
      drmI830IrqEmit ie;
      int ret;
      /*
      assert(((*(int *)intel->driHwLock) & ~DRM_LOCK_CONT) == 
	     (DRM_LOCK_HELD|intel->hHWContext));
      */
      ie.irq_seq = &seq;

      ret = drmCommandWriteRead( intelScreen->driScrnPriv->fd,
				 DRM_I830_IRQ_EMIT, 
				 &ie, sizeof(ie) );
      if ( ret ) {
	 fprintf( stderr, "%s: drmI830IrqEmit: %d\n", __FUNCTION__, ret );
	 exit(1);
      }   

      if (0)
	 fprintf(stderr, "%s -->  %d\n", __FUNCTION__, seq );
   }

   return seq;
}

void intelWaitIrq( intelScreenPrivate *intelScreen, int seq )
{
   if (!intelScreen->no_hw) {
      drmI830IrqWait iw;
      int ret, lastdispatch;
      volatile drmI830Sarea *sarea = (volatile drmI830Sarea *)
	 (((GLubyte *)intelScreen->driScrnPriv->pSAREA) +
	  intelScreen->sarea_priv_offset);

      if (0)
	 fprintf(stderr, "%s %d\n", __FUNCTION__, seq );

      iw.irq_seq = seq;
	
      do {
	 lastdispatch = sarea->last_dispatch;
	 ret = drmCommandWrite( intelScreen->driScrnPriv->fd,
				DRM_I830_IRQ_WAIT, &iw, sizeof(iw) );

	 /* This seems quite often to return before it should!?! 
	  */
      } while (ret == -EAGAIN ||
	       ret == -EINTR ||
	       (ret == -EBUSY && lastdispatch != sarea->last_dispatch) ||
	       (ret == 0 && seq > sarea->last_dispatch) ||
	       (ret == 0 && sarea->last_dispatch - seq >= (1 << 24)));

      if ( ret ) {
	 fprintf( stderr, "%s: drmI830IrqWait: %d\n", __FUNCTION__, ret );

	 exit(1);
      }
   }
}


void intel_batch_ioctl( struct intel_context *intel, 
			GLuint start_offset,
			GLuint used,
			GLboolean ignore_cliprects,
			GLboolean allow_unlock )
{
   drmI830BatchBuffer batch;

   assert(intel->locked);
   assert(used);

   if (0)
      fprintf(stderr, "%s used %d offset %x..%x\n",
	      __FUNCTION__, 
	      used, 
	      start_offset,
	      start_offset + used);

   batch.start = start_offset;
   batch.used = used;
   batch.cliprects = NULL;
   batch.num_cliprects = 0;
   batch.DR1 = 0;
   batch.DR4 = 0;
      
   if (INTEL_DEBUG & DEBUG_DMA)
      fprintf(stderr, "%s: 0x%x..0x%x\n",
	      __FUNCTION__, 
	      batch.start, 
	      batch.start + batch.used * 4);

   if (!intel->intelScreen->no_hw) {
      if (drmCommandWrite (intel->driFd, DRM_I830_BATCHBUFFER, &batch, 
			   sizeof(batch))) {
	 fprintf(stderr, "DRM_I830_BATCHBUFFER: %d\n",  -errno);
	 UNLOCK_HARDWARE(intel);
	 exit(1);
      }
   }
}

void
intel_exec_ioctl(struct intel_context *intel,
		 GLuint used,
		 GLboolean ignore_cliprects, GLboolean allow_unlock,
		 void *start, GLuint count, dri_fence **fence)
{
   struct drm_i915_execbuffer execbuf;
   dri_fence *fo;

   assert(intel->locked);
   assert(used);

   if (*fence) {
     dri_fence_unreference(*fence);
   }

   memset(&execbuf, 0, sizeof(execbuf));

   execbuf.num_buffers = count;
   execbuf.batch.used = used;
   execbuf.batch.cliprects = intel->pClipRects;
   execbuf.batch.num_cliprects = ignore_cliprects ? 0 : intel->numClipRects;
   execbuf.batch.DR1 = 0;
   execbuf.batch.DR4 = ((((GLuint) intel->drawX) & 0xffff) |
			(((GLuint) intel->drawY) << 16));

   execbuf.ops_list = (unsigned)start; // TODO
   execbuf.fence_arg.flags = DRM_FENCE_FLAG_SHAREABLE | DRM_I915_FENCE_FLAG_FLUSHED;

   if (intel->intelScreen->no_hw)
      return;

   if (drmCommandWriteRead(intel->driFd, DRM_I915_EXECBUFFER, &execbuf,
                       sizeof(execbuf))) {
      fprintf(stderr, "DRM_I830_EXECBUFFER: %d\n", -errno);
      UNLOCK_HARDWARE(intel);
      exit(1);
   }


   fo = intel_ttm_fence_create_from_arg(intel->intelScreen->bufmgr, "fence buffers",
					&execbuf.fence_arg);
   if (!fo) {
      fprintf(stderr, "failed to fence handle: %08x\n", execbuf.fence_arg.handle);
      UNLOCK_HARDWARE(intel);
      exit(1);
   }
   *fence = fo;

   /* FIXME: use hardware contexts to avoid 'losing' hardware after
    * each buffer flush.
    */
   intel->vtbl.lost_hardware(intel);

}
