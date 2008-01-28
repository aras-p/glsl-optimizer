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



uint
cell_get_empty_buffer(struct cell_context *cell)
{
   uint buf = 0, tries = 0;

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
               printf("PPU: ALLOC BUFFER %u\n", buf);
               */
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


void
cell_batch_flush(struct cell_context *cell)
{
   static boolean flushing = FALSE;
   uint batch = cell->cur_batch;
   const uint size = cell->buffer_size[batch];
   uint spu, cmd_word;

   assert(!flushing);

   if (size == 0)
      return;

   flushing = TRUE;

   assert(batch < CELL_NUM_BUFFERS);

   /*
   printf("cell_batch_dispatch: buf %u at %p, size %u\n",
          batch, &cell->batch_buffer[batch][0], size);
   */
     
   /*
    * Build "BATCH" command and sent to all SPUs.
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


uint
cell_batch_free_space(const struct cell_context *cell)
{
   uint free = CELL_BUFFER_SIZE - cell->buffer_size[cell->cur_batch];
   return free;
}


/**
 * Append data to current batch.
 */
void
cell_batch_append(struct cell_context *cell, const void *data, uint bytes)
{
   uint size;

   ASSERT(bytes % 4 == 0);
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

   if (size + bytes > CELL_BUFFER_SIZE) {
      cell_batch_flush(cell);
      size = 0;
   }

   assert(size + bytes <= CELL_BUFFER_SIZE);

   memcpy(cell->buffer[cell->cur_batch] + size, data, bytes);

   cell->buffer_size[cell->cur_batch] = size + bytes;
}


void *
cell_batch_alloc(struct cell_context *cell, uint bytes)
{
   void *pos;
   uint size;

   ASSERT(bytes % 4 == 0);
   ASSERT(bytes <= CELL_BUFFER_SIZE);

   assert(cell->cur_batch >= 0);

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

   if (size + bytes > CELL_BUFFER_SIZE) {
      cell_batch_flush(cell);
      size = 0;
   }

   assert(size + bytes <= CELL_BUFFER_SIZE);

   pos = (void *) (cell->buffer[cell->cur_batch] + size);

   cell->buffer_size[cell->cur_batch] = size + bytes;

   return pos;
}
