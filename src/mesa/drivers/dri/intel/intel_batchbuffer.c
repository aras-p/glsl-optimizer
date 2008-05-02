/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "intel_batchbuffer.h"
#include "intel_ioctl.h"
#include "intel_decode.h"
#include "intel_reg.h"

/* Relocations in kernel space:
 *    - pass dma buffer seperately
 *    - memory manager knows how to patch
 *    - pass list of dependent buffers
 *    - pass relocation list
 *
 * Either:
 *    - get back an offset for buffer to fire
 *    - memory manager knows how to fire buffer
 *
 * Really want the buffer to be AGP and pinned.
 *
 */

/* Cliprect fence: The highest fence protecting a dma buffer
 * containing explicit cliprect information.  Like the old drawable
 * lock but irq-driven.  X server must wait for this fence to expire
 * before changing cliprects [and then doing sw rendering?].  For
 * other dma buffers, the scheduler will grab current cliprect info
 * and mix into buffer.  X server must hold the lock while changing
 * cliprects???  Make per-drawable.  Need cliprects in shared memory
 * -- beats storing them with every cmd buffer in the queue.
 *
 * ==> X server must wait for this fence to expire before touching the
 * framebuffer with new cliprects.
 *
 * ==> Cliprect-dependent buffers associated with a
 * cliprect-timestamp.  All of the buffers associated with a timestamp
 * must go to hardware before any buffer with a newer timestamp.
 *
 * ==> Dma should be queued per-drawable for correct X/GL
 * synchronization.  Or can fences be used for this?
 *
 * Applies to: Blit operations, metaops, X server operations -- X
 * server automatically waits on its own dma to complete before
 * modifying cliprects ???
 */

void
intel_batchbuffer_reset(struct intel_batchbuffer *batch)
{
   struct intel_context *intel = batch->intel;

   if (batch->buf != NULL) {
      dri_bo_unreference(batch->buf);
      batch->buf = NULL;
   }

   batch->buf = dri_bo_alloc(intel->bufmgr, "batchbuffer",
			     intel->maxBatchSize, 4096,
			     DRM_BO_FLAG_MEM_LOCAL | DRM_BO_FLAG_CACHED | DRM_BO_FLAG_CACHED_MAPPED);
   dri_bo_map(batch->buf, GL_TRUE);
   batch->map = batch->buf->virtual;
   batch->size = intel->maxBatchSize;
   batch->ptr = batch->map;
   batch->dirty_state = ~0;
   batch->cliprect_mode = IGNORE_CLIPRECTS;

   /* account batchbuffer in aperture */
   dri_bufmgr_check_aperture_space(batch->buf);

}

struct intel_batchbuffer *
intel_batchbuffer_alloc(struct intel_context *intel)
{
   struct intel_batchbuffer *batch = calloc(sizeof(*batch), 1);

   batch->intel = intel;
   batch->last_fence = NULL;
   intel_batchbuffer_reset(batch);

   return batch;
}

void
intel_batchbuffer_free(struct intel_batchbuffer *batch)
{
   if (batch->last_fence) {
      dri_fence_wait(batch->last_fence);
      dri_fence_unreference(batch->last_fence);
      batch->last_fence = NULL;
   }
   if (batch->map) {
      dri_bo_unmap(batch->buf);
      batch->map = NULL;
   }
   dri_bo_unreference(batch->buf);
   batch->buf = NULL;
   free(batch);
}



/* TODO: Push this whole function into bufmgr.
 */
static void
do_flush_locked(struct intel_batchbuffer *batch,
		GLuint used, GLboolean allow_unlock)
{
   struct intel_context *intel = batch->intel;
   void *start;
   GLuint count;

   dri_bo_unmap(batch->buf);
   start = dri_process_relocs(batch->buf, &count);

   batch->map = NULL;
   batch->ptr = NULL;

   /* Throw away non-effective packets.  Won't work once we have
    * hardware contexts which would preserve statechanges beyond a
    * single buffer.
    */

   if (!(intel->numClipRects == 0 &&
	 batch->cliprect_mode == LOOP_CLIPRECTS)) {
      if (intel->ttm == GL_TRUE) {
	 intel_exec_ioctl(batch->intel,
			  used,
			  batch->cliprect_mode != LOOP_CLIPRECTS,
			  allow_unlock,
			  start, count, &batch->last_fence);
      } else {
	 intel_batch_ioctl(batch->intel,
			   batch->buf->offset,
			   used,
			   batch->cliprect_mode != LOOP_CLIPRECTS,
			   allow_unlock);
      }
   }
      
   dri_post_submit(batch->buf, &batch->last_fence);

   if (intel->numClipRects == 0 &&
       batch->cliprect_mode == LOOP_CLIPRECTS) {
      if (allow_unlock) {
	 /* If we are not doing any actual user-visible rendering,
	  * do a sched_yield to keep the app from pegging the cpu while
	  * achieving nothing.
	  */
         UNLOCK_HARDWARE(intel);
         sched_yield();
         LOCK_HARDWARE(intel);
      }
   }

   if (INTEL_DEBUG & DEBUG_BATCH) {
      dri_bo_map(batch->buf, GL_FALSE);
      intel_decode(batch->buf->virtual, used / 4, batch->buf->offset,
		   intel->intelScreen->deviceID);
      dri_bo_unmap(batch->buf);

      if (intel->vtbl.debug_batch != NULL)
	 intel->vtbl.debug_batch(intel);
   }

   intel->vtbl.new_batch(intel);
}

void
_intel_batchbuffer_flush(struct intel_batchbuffer *batch, const char *file,
			 int line)
{
   struct intel_context *intel = batch->intel;
   GLuint used = batch->ptr - batch->map;
   GLboolean was_locked = intel->locked;

   if (used == 0)
      return;

   if (INTEL_DEBUG & DEBUG_BATCH)
      fprintf(stderr, "%s:%d: Batchbuffer flush with %db used\n", file, line,
	      used);
   /* Add the MI_BATCH_BUFFER_END.  Always add an MI_FLUSH - this is a
    * performance drain that we would like to avoid.
    */
   if (used & 4) {
      ((int *) batch->ptr)[0] = intel->vtbl.flush_cmd();
      ((int *) batch->ptr)[1] = 0;
      ((int *) batch->ptr)[2] = MI_BATCH_BUFFER_END;
      used += 12;
   }
   else {
      ((int *) batch->ptr)[0] = intel->vtbl.flush_cmd();
      ((int *) batch->ptr)[1] = MI_BATCH_BUFFER_END;
      used += 8;
   }

   /* Workaround for recursive batchbuffer flushing: If the window is
    * moved, we can get into a case where we try to flush during a
    * flush.  What happens is that when we try to grab the lock for
    * the first flush, we detect that the window moved which then
    * causes another flush (from the intel_draw_buffer() call in
    * intelUpdatePageFlipping()).  To work around this we reset the
    * batchbuffer tail pointer before trying to get the lock.  This
    * prevent the nested buffer flush, but a better fix would be to
    * avoid that in the first place. */
   batch->ptr = batch->map;

   /* TODO: Just pass the relocation list and dma buffer up to the
    * kernel.
    */
   if (!was_locked)
      LOCK_HARDWARE(intel);

   do_flush_locked(batch, used, GL_FALSE);

   if (!was_locked)
      UNLOCK_HARDWARE(intel);

   if (INTEL_DEBUG & DEBUG_SYNC) {
      fprintf(stderr, "waiting for idle\n");
      if (batch->last_fence != NULL)
	 dri_fence_wait(batch->last_fence);
   }

   /* Reset the buffer:
    */
   intel_batchbuffer_reset(batch);
}

void
intel_batchbuffer_finish(struct intel_batchbuffer *batch)
{
   intel_batchbuffer_flush(batch);
   if (batch->last_fence != NULL)
      dri_fence_wait(batch->last_fence);
}


/*  This is the only way buffers get added to the validate list.
 */
GLboolean
intel_batchbuffer_emit_reloc(struct intel_batchbuffer *batch,
                             dri_bo *buffer,
                             GLuint flags, GLuint delta)
{
   int ret;

   ret = dri_emit_reloc(batch->buf, flags, delta, batch->ptr - batch->map, buffer);

   /*
    * Using the old buffer offset, write in what the right data would be, in case
    * the buffer doesn't move and we can short-circuit the relocation processing
    * in the kernel
    */
   intel_batchbuffer_emit_dword (batch, buffer->offset + delta);

   return GL_TRUE;
}

void
intel_batchbuffer_data(struct intel_batchbuffer *batch,
                       const void *data, GLuint bytes,
		       enum cliprect_mode cliprect_mode)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(batch, bytes, cliprect_mode);
   __memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
}
