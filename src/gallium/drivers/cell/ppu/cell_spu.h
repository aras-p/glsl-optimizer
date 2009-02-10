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

#ifndef CELL_SPU
#define CELL_SPU


#include <libspe2.h>
#include <pthread.h>
#include "cell/common.h"

#include "cell_context.h"


/**
 * Global vars, for now anyway.
 */
struct cell_global_info
{
   /**
    * SPU/SPE handles, etc
    */
   spe_context_ptr_t spe_contexts[CELL_MAX_SPUS];
   pthread_t spe_threads[CELL_MAX_SPUS];

   /**
    * Data sent to SPUs at start-up
    */
   struct cell_init_info inits[CELL_MAX_SPUS];
};


extern struct cell_global_info cell_global;


/** This is the handle for the actual SPE code */
extern spe_program_handle_t g3d_spu;


extern void
send_mbox_message(spe_context_ptr_t ctx, unsigned int msg);

extern uint
wait_mbox_message(spe_context_ptr_t ctx);


extern void
cell_start_spus(struct cell_context *cell);


extern void
cell_spu_exit(struct cell_context *cell);


#endif /* CELL_SPU */
