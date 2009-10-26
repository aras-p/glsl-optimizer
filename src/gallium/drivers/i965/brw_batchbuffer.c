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

#include "brw_batchbuffer.h"
#include "brw_decode.h"
#include "brw_reg.h"
#include "brw_winsys.h"


void
brw_batchbuffer_reset(struct brw_batchbuffer *batch)
{
   struct intel_context *intel = batch->intel;

   if (batch->buf != NULL) {
      brw->sws->bo_unreference(batch->buf);
      batch->buf = NULL;
   }

   if (!batch->buffer && intel->ttm == GL_TRUE)
      batch->buffer = malloc (intel->maxBatchSize);

   batch->buf = batch->sws->bo_alloc(batch->sws,
				     BRW_BUFFER_TYPE_BATCH,
				     intel->maxBatchSize, 4096);
   if (batch->buffer)
      batch->map = batch->buffer;
   else {
      batch->sws->bo_map(batch->buf, GL_TRUE);
      batch->map = batch->buf->virtual;
   }
   batch->size = intel->maxBatchSize;
   batch->ptr = batch->map;
   batch->dirty_state = ~0;
   batch->cliprect_mode = IGNORE_CLIPRECTS;
}

struct brw_batchbuffer *
brw_batchbuffer_alloc(struct brw_winsys_screen *sws)
{
   struct brw_batchbuffer *batch = CALLOC_STRUCT(brw_batchbuffer);

   batch->sws = sws;
   brw_batchbuffer_reset(batch);

   return batch;
}

void
brw_batchbuffer_free(struct brw_batchbuffer *batch)
{
   if (batch->map) {
      dri_bo_unmap(batch->buf);
      batch->map = NULL;
   }

   brw->sws->bo_unreference(batch->buf);
   batch->buf = NULL;
   FREE(batch);
}


void
_brw_batchbuffer_flush(struct brw_batchbuffer *batch, const char *file,
			 int line)
{
   struct intel_context *intel = batch->intel;
   GLuint used = batch->ptr - batch->map;

   if (used == 0)
      return;

   if (intel->first_post_swapbuffers_batch == NULL) {
      intel->first_post_swapbuffers_batch = intel->batch->buf;
      batch->sws->bo_reference(intel->first_post_swapbuffers_batch);
   }

   if (intel->first_post_swapbuffers_batch == NULL) {
      intel->first_post_swapbuffers_batch = intel->batch->buf;
      batch->sws->bo_reference(intel->first_post_swapbuffers_batch);
   }


   if (BRW_DEBUG & DEBUG_BATCH)
      debug_printf("%s:%d: Batchbuffer flush with %db used\n", file, line,
	      used);

   /* Emit a flush if the bufmgr doesn't do it for us. */
   if (intel->always_flush_cache || !intel->ttm) {
      *(GLuint *) (batch->ptr) = ((CMD_MI_FLUSH << 16) | BRW_FLUSH_STATE_CACHE);
      batch->ptr += 4;
      used = batch->ptr - batch->map;
   }

   /* Round batchbuffer usage to 2 DWORDs. */

   if ((used & 4) == 0) {
      *(GLuint *) (batch->ptr) = 0; /* noop */
      batch->ptr += 4;
      used = batch->ptr - batch->map;
   }

   /* Mark the end of the buffer. */
   *(GLuint *) (batch->ptr) = MI_BATCH_BUFFER_END; /* noop */
   batch->ptr += 4;
   used = batch->ptr - batch->map;

   batch->sws->bo_unmap(batch->buf);

   batch->map = NULL;
   batch->ptr = NULL;
      
   batch->sws->bo_exec(batch->buf, used, NULL, 0, 0 );
      
   if (BRW_DEBUG & DEBUG_BATCH) {
      dri_bo_map(batch->buf, GL_FALSE);
      intel_decode(batch->buf->virtual, used / 4, batch->buf->offset,
		   brw->brw_screen->pci_id);
      dri_bo_unmap(batch->buf);
   }

   if (BRW_DEBUG & DEBUG_SYNC) {
      debug_printf("waiting for idle\n");
      dri_bo_map(batch->buf, GL_TRUE);
      dri_bo_unmap(batch->buf);
   }

   /* Reset the buffer:
    */
   brw_batchbuffer_reset(batch);
}


/*  This is the only way buffers get added to the validate list.
 */
GLboolean
brw_batchbuffer_emit_reloc(struct brw_batchbuffer *batch,
                             struct brw_winsys_buffer *buffer,
                             uint32_t read_domains, uint32_t write_domain,
			     uint32_t delta)
{
   int ret;

   if (batch->ptr - batch->map > batch->buf->size)
      debug_printf ("bad relocation ptr %p map %p offset %d size %d\n",
		    batch->ptr, batch->map, batch->ptr - batch->map, batch->buf->size);

   ret = batch->sws->bo_emit_reloc(batch->buf,
				   read_domains,
				   write_domain,
				   delta, 
				   batch->ptr - batch->map,
				   buffer);

   /*
    * Using the old buffer offset, write in what the right data would be, in case
    * the buffer doesn't move and we can short-circuit the relocation processing
    * in the kernel
    */
   brw_batchbuffer_emit_dword (batch, buffer->offset + delta);

   return GL_TRUE;
}

void
brw_batchbuffer_data(struct brw_batchbuffer *batch,
                       const void *data, GLuint bytes,
		       enum cliprect_mode cliprect_mode)
{
   assert((bytes & 3) == 0);
   brw_batchbuffer_require_space(batch, bytes);
   __memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
}
