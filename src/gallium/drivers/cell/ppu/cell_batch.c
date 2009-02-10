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
#include "cell_fence.h"
#include "cell_spu.h"



/**
 * Search the buffer pool for an empty/free buffer and return its index.
 * Buffers are used for storing vertex data, state and commands which
 * will be sent to the SPUs.
 * If no empty buffers are available, wait for one.
 * \return buffer index in [0, CELL_NUM_BUFFERS-1]
 */
uint
cell_get_empty_buffer(struct cell_context *cell)
{
   static uint prev_buffer = 0;
   uint buf = (prev_buffer + 1) % CELL_NUM_BUFFERS;
   uint tries = 0;

   /* Find a buffer that's marked as free by all SPUs */
   while (1) {
      uint spu, num_free = 0;

      for (spu = 0; spu < cell->num_spus; spu++) {
         if (cell->buffer_status[spu][buf][0] == CELL_BUFFER_STATUS_FREE) {
            num_free++;

            if (num_free == cell->num_spus) {
               /* found a free buffer, now mark status as used */
               for (spu = 0; spu < cell->num_spus; spu++) {
                  cell->buffer_status[spu][buf][0] = CELL_BUFFER_STATUS_USED;
               }
               /*
               printf("PPU: ALLOC BUFFER %u, %u tries\n", buf, tries);
               */
               prev_buffer = buf;

               /* release tex buffer associated w/ prev use of this batch buf */
               cell_free_fenced_buffers(cell, &cell->fenced_buffers[buf]);

               return buf;
            }
         }
         else {
            break;
         }
      }

      /* try next buf */
      buf = (buf + 1) % CELL_NUM_BUFFERS;

      tries++;
      if (tries == 100) {
         /*
         printf("PPU WAITING for buffer...\n");
         */
      }
   }
}


/**
 * Append a fence command to the current batch buffer.
 * Note that we're sure there's always room for this because of the
 * adjusted size check in cell_batch_free_space().
 */
static void
emit_fence(struct cell_context *cell)
{
   const uint batch = cell->cur_batch;
   const uint size = cell->buffer_size[batch];
   struct cell_command_fence *fence_cmd;
   struct cell_fence *fence = &cell->fenced_buffers[batch].fence;
   uint i;

   /* set fence status to emitted, not yet signalled */
   for (i = 0; i < cell->num_spus; i++) {
      fence->status[i][0] = CELL_FENCE_EMITTED;
   }

   STATIC_ASSERT(sizeof(struct cell_command_fence) % 16 == 0);
   ASSERT(size % 16 == 0);
   ASSERT(size + sizeof(struct cell_command_fence) <= CELL_BUFFER_SIZE);

   fence_cmd = (struct cell_command_fence *) (cell->buffer[batch] + size);
   fence_cmd->opcode[0] = CELL_CMD_FENCE;
   fence_cmd->fence = fence;

   /* update batch buffer size */
   cell->buffer_size[batch] = size + sizeof(struct cell_command_fence);
}


/**
 * Flush the current batch buffer to the SPUs.
 * An empty buffer will be found and set as the new current batch buffer
 * for subsequent commands/data.
 */
void
cell_batch_flush(struct cell_context *cell)
{
   static boolean flushing = FALSE;
   uint batch = cell->cur_batch;
   uint size = cell->buffer_size[batch];
   uint spu, cmd_word;

   assert(!flushing);

   if (size == 0)
      return;

   /* Before we use this batch buffer, make sure any fenced texture buffers
    * are released.
    */
   if (cell->fenced_buffers[batch].head) {
      emit_fence(cell);
      size = cell->buffer_size[batch];
   }

   flushing = TRUE;

   assert(batch < CELL_NUM_BUFFERS);

   /*
   printf("cell_batch_dispatch: buf %u at %p, size %u\n",
          batch, &cell->buffer[batch][0], size);
   */
     
   /*
    * Build "BATCH" command and send to all SPUs.
    */
   cmd_word = CELL_CMD_BATCH | (batch << 8) | (size << 16);

   for (spu = 0; spu < cell->num_spus; spu++) {
      assert(cell->buffer_status[spu][batch][0] == CELL_BUFFER_STATUS_USED);
      send_mbox_message(cell_global.spe_contexts[spu], cmd_word);
   }

   /* When the SPUs are done copying the buffer into their locals stores
    * they'll write a BUFFER_STATUS_FREE message into the buffer_status[]
    * array indicating that the PPU can re-use the buffer.
    */

   batch = cell_get_empty_buffer(cell);

   cell->buffer_size[batch] = 0;  /* empty */
   cell->cur_batch = batch;

   flushing = FALSE;
}


/**
 * Return the number of bytes free in the current batch buffer.
 */
uint
cell_batch_free_space(const struct cell_context *cell)
{
   uint free = CELL_BUFFER_SIZE - cell->buffer_size[cell->cur_batch];
   free -= sizeof(struct cell_command_fence);
   return free;
}


/**
 * Allocate space in the current batch buffer for 'bytes' space.
 * Bytes must be a multiple of 16 bytes.  Allocation will be 16 byte aligned.
 * \return address in batch buffer to put data
 */
void *
cell_batch_alloc16(struct cell_context *cell, uint bytes)
{
   void *pos;
   uint size;

   ASSERT(bytes % 16 == 0);
   ASSERT(bytes <= CELL_BUFFER_SIZE);
   ASSERT(cell->cur_batch >= 0);

#ifdef ASSERT
   {
      uint spu;
      for (spu = 0; spu < cell->num_spus; spu++) {
         ASSERT(cell->buffer_status[spu][cell->cur_batch][0]
                 == CELL_BUFFER_STATUS_USED);
      }
   }
#endif

   size = cell->buffer_size[cell->cur_batch];

   if (bytes > cell_batch_free_space(cell)) {
      cell_batch_flush(cell);
      size = 0;
   }

   ASSERT(size % 16 == 0);
   ASSERT(size + bytes <= CELL_BUFFER_SIZE);

   pos = (void *) (cell->buffer[cell->cur_batch] + size);

   cell->buffer_size[cell->cur_batch] = size + bytes;

   return pos;
}


/**
 * One-time init of batch buffers.
 */
void
cell_init_batch_buffers(struct cell_context *cell)
{
   uint spu, buf;

   /* init command, vertex/index buffer info */
   for (buf = 0; buf < CELL_NUM_BUFFERS; buf++) {
      cell->buffer_size[buf] = 0;

      /* init batch buffer status values,
       * mark 0th buffer as used, rest as free.
       */
      for (spu = 0; spu < cell->num_spus; spu++) {
         if (buf == 0)
            cell->buffer_status[spu][buf][0] = CELL_BUFFER_STATUS_USED;
         else
            cell->buffer_status[spu][buf][0] = CELL_BUFFER_STATUS_FREE;
      }
   }
}
