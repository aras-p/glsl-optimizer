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
#include "cell_spu.h"


void
cell_batch_flush(struct cell_context *cell)
{
   const uint batch = cell->cur_batch;
   const uint size = cell->batch_buffer_size[batch];
   uint i, cmd_word;

   if (size == 0)
      return;

   assert(batch < CELL_NUM_BATCH_BUFFERS);

   printf("cell_batch_dispatch: buf %u, size %u\n", batch, size);
          
   cmd_word = CELL_CMD_BATCH | (batch << 8) | (size << 16);

   for (i = 0; i < cell->num_spus; i++) {
      send_mbox_message(cell_global.spe_contexts[i], cmd_word);
   }

   /* XXX wait on DMA xfer of prev buffer to complete */

   cell->cur_batch = (batch + 1) % CELL_NUM_BATCH_BUFFERS;

   cell->batch_buffer_size[cell->cur_batch] = 0;  /* empty */
}


/**
 * \param cmd  command to append
 * \param length  command size in bytes
 */
void
cell_batch_append(struct cell_context *cell, const void *cmd, uint length)
{
   uint size;

   assert(cell->cur_batch >= 0);

   size = cell->batch_buffer_size[cell->cur_batch];

   if (size + length > CELL_BATCH_BUFFER_SIZE) {
      cell_batch_flush(cell);
      size = 0;
   }

   assert(size + length <= CELL_BATCH_BUFFER_SIZE);

   memcpy(cell->batch_buffer[cell->cur_batch] + size, cmd, length);

   cell->batch_buffer_size[cell->cur_batch] = size + length;
}
