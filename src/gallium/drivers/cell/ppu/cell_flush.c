/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "cell_context.h"
#include "cell_batch.h"
#include "cell_flush.h"
#include "cell_spu.h"
#include "cell_render.h"
#include "draw/draw_context.h"


void
cell_flush(struct pipe_context *pipe, unsigned flags,
           struct pipe_fence_handle **fence)
{
   struct cell_context *cell = cell_context(pipe);

   if (fence) {
      *fence = NULL;
      /* XXX: Implement real fencing */
      flags |= CELL_FLUSH_WAIT;
   }

   if (flags & PIPE_FLUSH_SWAPBUFFERS)
      flags |= CELL_FLUSH_WAIT;

   draw_flush( cell->draw );
   cell_flush_int(pipe, flags);
}


/** internal flush */
void
cell_flush_int(struct pipe_context *pipe, unsigned flags)
{
   static boolean flushing = FALSE;  /* recursion catcher */
   struct cell_context *cell = cell_context(pipe);
   uint i;

   ASSERT(!flushing);
   flushing = TRUE;

   if (flags & CELL_FLUSH_WAIT) {
      uint64_t *cmd = (uint64_t *) cell_batch_alloc(cell, sizeof(uint64_t));
      *cmd = CELL_CMD_FINISH;
   }

   cell_batch_flush(cell);

#if 0
   /* Send CMD_FINISH to all SPUs */
   for (i = 0; i < cell->num_spus; i++) {
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_FINISH);
   }
#endif

   if (flags & CELL_FLUSH_WAIT) {
      /* Wait for ack */
      for (i = 0; i < cell->num_spus; i++) {
         uint k = wait_mbox_message(cell_global.spe_contexts[i]);
         assert(k == CELL_CMD_FINISH);
      }
   }

   flushing = FALSE;
}


void
cell_flush_buffer_range(struct cell_context *cell, void *ptr,
			unsigned size)
{
   uint64_t batch[1 + (ROUNDUP8(sizeof(struct cell_buffer_range)) / 8)];
   struct cell_buffer_range *br = (struct cell_buffer_range *) & batch[1];

   batch[0] = CELL_CMD_FLUSH_BUFFER_RANGE;
   br->base = (uintptr_t) ptr;
   br->size = size;
   cell_batch_append(cell, batch, sizeof(batch));
}
