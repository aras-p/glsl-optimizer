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

/**
 * Authors
 *  Brian Paul
 */


#include <stdio.h>
#include <libspe.h>
#include <libmisc.h>
#include "pipe/cell/ppu/cell_context.h"
#include "pipe/cell/common.h"

#define NUM_SPUS 6


extern spe_program_handle_t g3d_spu;

static speid_t speid[NUM_SPUS];
static struct init_info inits[NUM_SPUS];


static void
start_spus(void)
{
   int i;

   for (i = 0; i < NUM_SPUS; i++) {
      inits[i].foo = i;
      inits[i].bar = i * 10;

      speid[i] = spe_create_thread(0,          /* gid */
                                   &g3d_spu,   /* spe program handle */
                                   &inits[i],  /* argp */
                                   NULL,       /* envp */
                                   -1,         /* mask */
                                   0 );        /* flags */
   }
}



void cell_create_context(void)
{
   printf("cell_create_context\n");

   start_spus();

   /* TODO: do something with the SPUs! */
}
