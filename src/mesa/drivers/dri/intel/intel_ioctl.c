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
#include <sys/types.h>
#include <sys/ioctl.h>

#include "mtypes.h"
#include "context.h"
#include "swrast/swrast.h"

#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "drm.h"
#include "i915_drm.h"

#include "intel_bufmgr.h"

#define FILE_DEBUG_FLAG DEBUG_IOCTL

int
intelEmitIrqLocked(struct intel_context *intel)
{
   struct drm_i915_irq_emit ie;
   int ret, seq = 1;

   if (intel->no_hw)
      return 1;

   /*
     assert(((*(int *)intel->driHwLock) & ~DRM_LOCK_CONT) ==
     (DRM_LOCK_HELD|intel->hHWContext));
   */

   ie.irq_seq = &seq;

   ret = drmCommandWriteRead(intel->driFd, DRM_I915_IRQ_EMIT, &ie, sizeof(ie));
   if (ret) {
      fprintf(stderr, "%s: drm_i915_irq_emit: %d\n", __FUNCTION__, ret);
      exit(1);
   }

   DBG("%s -->  %d\n", __FUNCTION__, seq);

   return seq;
}

void
intelWaitIrq(struct intel_context *intel, int seq)
{
   struct drm_i915_irq_wait iw;
   int ret, lastdispatch;
   volatile struct drm_i915_sarea *sarea = intel->sarea;

   if (intel->no_hw)
      return;

   DBG("%s %d\n", __FUNCTION__, seq);

   iw.irq_seq = seq;

   do {
      lastdispatch = sarea->last_dispatch;
      ret = drmCommandWrite(intel->driFd, DRM_I915_IRQ_WAIT, &iw, sizeof(iw));
   } while (ret == -EAGAIN ||
	    ret == -EINTR ||
	    (ret == -EBUSY && lastdispatch != sarea->last_dispatch) ||
	    (ret == 0 && seq > sarea->last_dispatch) ||
	    (ret == 0 && sarea->last_dispatch - seq >= (1 << 24)));

   if (ret) {
      fprintf(stderr, "%s: drm_i915_irq_wait: %d\n", __FUNCTION__, ret);
      exit(1);
   }
}


int
intel_batch_ioctl(struct intel_context *intel,
                  GLuint start_offset,
                  GLuint used,
                  GLboolean ignore_cliprects, GLboolean allow_unlock)
{
   struct drm_i915_batchbuffer batch;

   if (intel->no_hw)
      return 0;

   assert(intel->locked);
   assert(used);

   DBG("%s used %d offset %x..%x ignore_cliprects %d\n",
       __FUNCTION__,
       used, start_offset, start_offset + used, ignore_cliprects);

   /* Throw away non-effective packets.  Won't work once we have
    * hardware contexts which would preserve statechanges beyond a
    * single buffer.
    */
   batch.start = start_offset;
   batch.used = used;
   batch.cliprects = intel->pClipRects;
   batch.num_cliprects = ignore_cliprects ? 0 : intel->numClipRects;
   batch.DR1 = 0;
   batch.DR4 = ((((GLuint) intel->drawX) & 0xffff) |
                (((GLuint) intel->drawY) << 16));

   DBG("%s: 0x%x..0x%x DR4: %x cliprects: %d\n",
       __FUNCTION__,
       batch.start,
       batch.start + batch.used * 4, batch.DR4, batch.num_cliprects);

   if (drmCommandWrite(intel->driFd, DRM_I915_BATCHBUFFER, &batch,
                       sizeof(batch))) {
      fprintf(stderr, "DRM_I915_BATCHBUFFER: %d\n", -errno);
      return -errno;
   }

   return 0;
}

#ifdef TTM_API
int
intel_exec_ioctl(struct intel_context *intel,
		 GLuint used,
		 GLboolean ignore_cliprects, GLboolean allow_unlock,
		 struct drm_i915_gem_execbuffer *execbuf)
{
   int ret;

   assert(intel->locked);
   assert(used);

   if (intel->no_hw)
      return 0;

   execbuf->batch_start_offset = 0;
   execbuf->batch_len = used;
   execbuf->cliprects_ptr = (uintptr_t)intel->pClipRects;
   execbuf->num_cliprects = ignore_cliprects ? 0 : intel->numClipRects;
   execbuf->DR1 = 0;
   execbuf->DR4 = ((((GLuint) intel->drawX) & 0xffff) |
		   (((GLuint) intel->drawY) << 16));

   do {
      ret = ioctl(intel->driFd, DRM_IOCTL_I915_GEM_EXECBUFFER, execbuf);
   } while (ret == -EAGAIN);

   if (ret != 0) {
      fprintf(stderr, "DRM_I915_GEM_EXECBUFFER: %d\n", -errno);
      return -errno;
   }

   return 0;
}
#else
int
int intel_exec_ioctl(struct intel_context *intel,
		     GLuint used,
		     GLboolean ignore_cliprects, GLboolean allow_unlock,
		     struct drm_i915_gem_execbuffer *execbuf);
{
    return -EINVAL;
}
#endif
