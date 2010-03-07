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

#include "util/u_memory.h"

#include "brw_batchbuffer.h"
#include "brw_reg.h"
#include "brw_winsys.h"
#include "brw_debug.h"
#include "brw_structs.h"

#define ALWAYS_EMIT_MI_FLUSH 1

enum pipe_error
brw_batchbuffer_reset(struct brw_batchbuffer *batch)
{
   enum pipe_error ret;

   ret = batch->sws->bo_alloc( batch->sws,
                               BRW_BUFFER_TYPE_BATCH,
                               BRW_BATCH_SIZE, 4096,
                               &batch->buf );
   if (ret)
      return ret;

   batch->size = BRW_BATCH_SIZE;

   /* With map_range semantics, the winsys can decide whether to
    * inject a malloc'ed bounce buffer instead of mapping directly.
    */
   batch->map = batch->sws->bo_map(batch->buf,
                                   BRW_DATA_BATCH_BUFFER,
                                   0, batch->size,
                                   GL_TRUE,
                                   GL_TRUE,
                                   GL_TRUE);

   batch->ptr = batch->map;
   return PIPE_OK;
}

struct brw_batchbuffer *
brw_batchbuffer_alloc(struct brw_winsys_screen *sws,
                      struct brw_chipset chipset)
{
   struct brw_batchbuffer *batch = CALLOC_STRUCT(brw_batchbuffer);

   batch->sws = sws;
   batch->chipset = chipset;
   brw_batchbuffer_reset(batch);

   return batch;
}

void
brw_batchbuffer_free(struct brw_batchbuffer *batch)
{
   if (batch->map) {
      batch->sws->bo_unmap(batch->buf);
      batch->map = NULL;
   }

   bo_reference(&batch->buf, NULL);
   FREE(batch);
}


void
_brw_batchbuffer_flush(struct brw_batchbuffer *batch, 
		       const char *file,
		       int line)
{
   GLuint used = batch->ptr - batch->map;

   if (used == 0)
      return;

   /* Post-swap throttling done by the state tracker.
    */

   if (BRW_DEBUG & DEBUG_BATCH)
      debug_printf("%s:%d: Batchbuffer flush with %db used\n", 
		   file, line, used);

   if (ALWAYS_EMIT_MI_FLUSH) {
      *(GLuint *) (batch->ptr) = MI_FLUSH | BRW_FLUSH_STATE_CACHE;
      batch->ptr += 4;
      used = batch->ptr - batch->map;
   }

   /* Round batchbuffer usage to 2 DWORDs. 
    */
   if ((used & 4) == 0) {
      *(GLuint *) (batch->ptr) = 0; /* noop */
      batch->ptr += 4;
      used = batch->ptr - batch->map;
   }

   /* Mark the end of the buffer. 
    */
   *(GLuint *) (batch->ptr) = MI_BATCH_BUFFER_END;
   batch->ptr += 4;
   used = batch->ptr - batch->map;

   batch->sws->bo_flush_range(batch->buf, 0, used);
   batch->sws->bo_unmap(batch->buf);
   batch->map = NULL;
   batch->ptr = NULL;
      
   batch->sws->bo_exec(batch->buf, used );

   if (BRW_DEBUG & DEBUG_SYNC) {
      /* Abuse map/unmap to achieve wait-for-fence.
       *
       * XXX: hide this inside the winsys and export a fence
       * interface.
       */
      debug_printf("waiting for idle\n");
      batch->sws->bo_wait_idle(batch->buf);
   }

   /* Reset the buffer:
    */
   brw_batchbuffer_reset(batch);
}


/* The OUT_RELOC() macro ends up here, generating a relocation within
 * the batch buffer.
 */
enum pipe_error
brw_batchbuffer_emit_reloc(struct brw_batchbuffer *batch,
			   struct brw_winsys_buffer *buffer,
			   enum brw_buffer_usage usage,
			   uint32_t delta)
{
   int ret;

   if (batch->ptr - batch->map > batch->buf->size) {
      debug_printf("bad relocation ptr %p map %p offset %d size %d\n",
		   batch->ptr, batch->map, batch->ptr - batch->map, batch->buf->size);

      return PIPE_ERROR_OUT_OF_MEMORY;
   }

   ret = batch->sws->bo_emit_reloc(batch->buf,
				   usage,
				   delta, 
				   batch->ptr - batch->map,
				   buffer);
   if (ret != 0)
      return ret;

   /* bo_emit_reloc was resposible for writing a zero into the
    * batchbuffer if necessary.  Just need to update our pointer.
    */
   batch->ptr += 4;

   return 0;
}

enum pipe_error
brw_batchbuffer_data(struct brw_batchbuffer *batch,
                       const void *data, GLuint bytes,
		       enum cliprect_mode cliprect_mode)
{
   enum pipe_error ret;

   assert((bytes & 3) == 0);

   ret = brw_batchbuffer_require_space(batch, bytes);
   if (ret)
      return ret;

   memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
   return 0;
}
