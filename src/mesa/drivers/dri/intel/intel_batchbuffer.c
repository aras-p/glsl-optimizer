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

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_decode.h"
#include "intel_reg.h"
#include "intel_bufmgr.h"
#include "intel_buffers.h"

void
intel_batchbuffer_reset(struct intel_batchbuffer *batch)
{
   struct intel_context *intel = batch->intel;

   if (batch->buf != NULL) {
      dri_bo_unreference(batch->buf);
      batch->buf = NULL;
   }

   if (!batch->buffer)
      batch->buffer = malloc (intel->maxBatchSize);

   batch->buf = dri_bo_alloc(intel->bufmgr, "batchbuffer",
			     intel->maxBatchSize, 4096);
   if (batch->buffer)
      batch->map = batch->buffer;
   else {
      dri_bo_map(batch->buf, GL_TRUE);
      batch->map = batch->buf->virtual;
   }
   batch->size = intel->maxBatchSize;
   batch->ptr = batch->map;
   batch->dirty_state = ~0;
}

struct intel_batchbuffer *
intel_batchbuffer_alloc(struct intel_context *intel)
{
   struct intel_batchbuffer *batch = calloc(sizeof(*batch), 1);

   batch->intel = intel;
   intel_batchbuffer_reset(batch);

   return batch;
}

void
intel_batchbuffer_free(struct intel_batchbuffer *batch)
{
   if (batch->buffer)
      free (batch->buffer);
   else {
      if (batch->map) {
	 dri_bo_unmap(batch->buf);
	 batch->map = NULL;
      }
   }
   dri_bo_unreference(batch->buf);
   batch->buf = NULL;
   free(batch);
}



/* TODO: Push this whole function into bufmgr.
 */
static void
do_flush_locked(struct intel_batchbuffer *batch, GLuint used)
{
   struct intel_context *intel = batch->intel;
   int ret = 0;
   int x_off = 0, y_off = 0;

   if (batch->buffer)
      dri_bo_subdata (batch->buf, 0, used, batch->buffer);
   else
      dri_bo_unmap(batch->buf);

   batch->map = NULL;
   batch->ptr = NULL;

   if (!intel->no_hw)
      dri_bo_exec(batch->buf, used, NULL, 0, (x_off & 0xffff) | (y_off << 16));

   if (INTEL_DEBUG & DEBUG_BATCH) {
      dri_bo_map(batch->buf, GL_FALSE);
      intel_decode(batch->buf->virtual, used / 4, batch->buf->offset,
		   intel->intelScreen->deviceID);
      dri_bo_unmap(batch->buf);

      if (intel->vtbl.debug_batch != NULL)
	 intel->vtbl.debug_batch(intel);
   }

   if (ret != 0) {
      exit(1);
   }
   intel->vtbl.new_batch(intel);
}

void
_intel_batchbuffer_flush(struct intel_batchbuffer *batch, const char *file,
			 int line)
{
   struct intel_context *intel = batch->intel;
   GLuint used = batch->ptr - batch->map;

   if (!intel->using_dri2_swapbuffers &&
       intel->first_post_swapbuffers_batch == NULL) {
      intel->first_post_swapbuffers_batch = intel->batch->buf;
      drm_intel_bo_reference(intel->first_post_swapbuffers_batch);
   }

   if (used == 0)
      return;

   if (INTEL_DEBUG & DEBUG_BATCH)
      fprintf(stderr, "%s:%d: Batchbuffer flush with %db used\n", file, line,
	      used);

   batch->reserved_space = 0;
   /* Emit a flush if the bufmgr doesn't do it for us. */
   if (intel->always_flush_cache) {
      intel_batchbuffer_emit_mi_flush(batch);
      used = batch->ptr - batch->map;
   }

   /* Round batchbuffer usage to 2 DWORDs. */

   if ((used & 4) == 0) {
      *(GLuint *) (batch->ptr) = 0; /* noop */
      batch->ptr += 4;
      used = batch->ptr - batch->map;
   }

   /* Mark the end of the buffer. */
   *(GLuint *) (batch->ptr) = MI_BATCH_BUFFER_END;
   batch->ptr += 4;
   used = batch->ptr - batch->map;
   assert (used <= batch->buf->size);

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

   if (intel->vtbl.finish_batch)
      intel->vtbl.finish_batch(intel);

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!intel->no_batch_wrap);

   batch->reserved_space = BATCH_RESERVED;

   /* TODO: Just pass the relocation list and dma buffer up to the
    * kernel.
    */
   do_flush_locked(batch, used);

   if (INTEL_DEBUG & DEBUG_SYNC) {
      fprintf(stderr, "waiting for idle\n");
      dri_bo_map(batch->buf, GL_TRUE);
      dri_bo_unmap(batch->buf);
   }

   /* Reset the buffer:
    */
   intel_batchbuffer_reset(batch);
}


/*  This is the only way buffers get added to the validate list.
 */
GLboolean
intel_batchbuffer_emit_reloc(struct intel_batchbuffer *batch,
                             dri_bo *buffer,
                             uint32_t read_domains, uint32_t write_domain,
			     uint32_t delta)
{
   int ret;

   assert(delta < buffer->size);

   if (batch->ptr - batch->map > batch->buf->size)
    printf ("bad relocation ptr %p map %p offset %d size %lu\n",
	    batch->ptr, batch->map, batch->ptr - batch->map, batch->buf->size);
   ret = dri_bo_emit_reloc(batch->buf, read_domains, write_domain,
			   delta, batch->ptr - batch->map, buffer);

   /*
    * Using the old buffer offset, write in what the right data would be, in case
    * the buffer doesn't move and we can short-circuit the relocation processing
    * in the kernel
    */
   intel_batchbuffer_emit_dword (batch, buffer->offset + delta);

   return GL_TRUE;
}

GLboolean
intel_batchbuffer_emit_reloc_fenced(struct intel_batchbuffer *batch,
				    drm_intel_bo *buffer,
				    uint32_t read_domains, uint32_t write_domain,
				    uint32_t delta)
{
   int ret;

   assert(delta < buffer->size);

   if (batch->ptr - batch->map > batch->buf->size)
    printf ("bad relocation ptr %p map %p offset %d size %lu\n",
	    batch->ptr, batch->map, batch->ptr - batch->map, batch->buf->size);
   ret = drm_intel_bo_emit_reloc_fence(batch->buf, batch->ptr - batch->map,
				       buffer, delta,
				       read_domains, write_domain);

   /*
    * Using the old buffer offset, write in what the right data would
    * be, in case the buffer doesn't move and we can short-circuit the
    * relocation processing in the kernel
    */
   intel_batchbuffer_emit_dword (batch, buffer->offset + delta);

   return GL_TRUE;
}

void
intel_batchbuffer_data(struct intel_batchbuffer *batch,
                       const void *data, GLuint bytes)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(batch, bytes);
   __memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
}

/* Emit a pipelined flush to either flush render and texture cache for
 * reading from a FBO-drawn texture, or flush so that frontbuffer
 * render appears on the screen in DRI1.
 *
 * This is also used for the always_flush_cache driconf debug option.
 */
void
intel_batchbuffer_emit_mi_flush(struct intel_batchbuffer *batch)
{
   struct intel_context *intel = batch->intel;

   if (intel->gen >= 4) {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_PIPE_CONTROL |
		PIPE_CONTROL_INSTRUCTION_FLUSH |
		PIPE_CONTROL_WRITE_FLUSH |
		PIPE_CONTROL_NO_WRITE);
      OUT_BATCH(0); /* write address */
      OUT_BATCH(0); /* write data */
      OUT_BATCH(0); /* write data */
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(1);
      OUT_BATCH(MI_FLUSH);
      ADVANCE_BATCH();
   }
}
