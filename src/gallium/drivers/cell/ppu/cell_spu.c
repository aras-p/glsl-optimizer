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


#include <pthread.h>

#include "cell_spu.h"
#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include "pipe/cell/common.h"


/*
helpful headers:
/opt/ibm/cell-sdk/prototype/src/include/ppu/cbe_mfc.h
*/


struct cell_global_info cell_global;


/**
 * Write a 1-word message to the given SPE mailbox.
 */
void
send_mbox_message(spe_context_ptr_t ctx, unsigned int msg)
{
   spe_in_mbox_write(ctx, &msg, 1, SPE_MBOX_ALL_BLOCKING);
}


/**
 * Wait for a 1-word message to arrive in given mailbox.
 */
uint
wait_mbox_message(spe_context_ptr_t ctx)
{
   do {
      unsigned data;
      int count = spe_out_mbox_read(ctx, &data, 1);

      if (count == 1) {
	 return data;
      }
      
      if (count < 0) {
	 /* error */ ;
      }
   } while (1);
}


static void *cell_thread_function(void *arg)
{
   struct cell_init_info *init = (struct cell_init_info *) arg;
   unsigned entry = SPE_DEFAULT_ENTRY;

   ASSERT_ALIGN16(init);

   if (spe_context_run(cell_global.spe_contexts[init->id], &entry, 0,
                       init, NULL, NULL) < 0) {
      fprintf(stderr, "spe_context_run() failed\n");
      exit(1);
   }

   pthread_exit(NULL);
}


/**
 * Create the SPU threads
 */
void
cell_start_spus(struct cell_context *cell)
{
   uint i, j;

   assert(cell->num_spus <= MAX_SPUS);

   ASSERT_ALIGN16(&cell_global.command[0]);
   ASSERT_ALIGN16(&cell_global.command[1]);

   ASSERT_ALIGN16(&cell_global.inits[0]);
   ASSERT_ALIGN16(&cell_global.inits[1]);

   for (i = 0; i < cell->num_spus; i++) {
      cell_global.inits[i].id = i;
      cell_global.inits[i].num_spus = cell->num_spus;
      cell_global.inits[i].cmd = &cell_global.command[i];
      for (j = 0; j < CELL_NUM_BUFFERS; j++) {
         cell_global.inits[i].buffers[j] = cell->buffer[j];
      }
      cell_global.inits[i].buffer_status = &cell->buffer_status[0][0][0];

      cell_global.spe_contexts[i] = spe_context_create(0, NULL);
      if (!cell_global.spe_contexts[i]) {
         fprintf(stderr, "spe_context_create() failed\n");
         exit(1);
      }

      if (spe_program_load(cell_global.spe_contexts[i], &g3d_spu)) {
         fprintf(stderr, "spe_program_load() failed\n");
         exit(1);
      }
      
      pthread_create(&cell_global.spe_threads[i], NULL, &cell_thread_function,
		     &cell_global.inits[i]);
   }
}


/**
 * Tell all the SPUs to stop/exit.
 */
void
cell_spu_exit(struct cell_context *cell)
{
   uint i;

   for (i = 0; i < cell->num_spus; i++) {
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_EXIT);
   }

   /* wait for threads to exit */
   for (i = 0; i < cell->num_spus; i++) {
      void *value;
      pthread_join(cell_global.spe_threads[i], &value);
      cell_global.spe_threads[i] = 0;
      cell_global.spe_contexts[i] = 0;
   }
}
