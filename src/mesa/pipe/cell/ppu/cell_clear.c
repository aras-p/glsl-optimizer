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
#include <assert.h>
#include <stdint.h>
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/cell/common.h"
#include "cell_clear.h"
#include "cell_context.h"
#include "cell_batch.h"
#include "cell_spu.h"


void
cell_clear_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                   unsigned clearValue)
{
   struct cell_context *cell = cell_context(pipe);
   uint i;
   uint surfIndex;

   if (!cell->cbuf_map[0])
      cell->cbuf_map[0] = pipe_surface_map(ps);

   if (ps == cell->framebuffer.zsbuf) {
      surfIndex = 1;
   }
   else {
      surfIndex = 0;
   }

#if 0
   for (i = 0; i < cell->num_spus; i++) {
#if 1
      uint clr = clearValue;
      if (surfIndex == 0) {
         /* XXX debug: clear color varied per-SPU to visualize tiles */
         if ((clr & 0xff) == 0)
            clr |= 64 + i * 8;
         if ((clr & 0xff00) == 0)
            clr |= (64 + i * 8) << 8;
         if ((clr & 0xff0000) == 0)
            clr |= (64 + i * 8) << 16;
         if ((clr & 0xff000000) == 0)
            clr |= (64 + i * 8) << 24;
      }
      cell_global.command[i].clear.value = clr;
#else
      cell_global.command[i].clear.value = clearValue;
#endif
      cell_global.command[i].clear.surface = surfIndex;
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_CLEAR_SURFACE);
   }
#else
   {
      struct cell_command_clear_surface *clr
         = (struct cell_command_clear_surface *)
         cell_batch_alloc(cell, sizeof(*clr));
      clr->opcode = CELL_CMD_CLEAR_SURFACE;
      clr->surface = surfIndex;
      clr->value = clearValue;
   }
#endif

   /* XXX temporary */
   cell_flush(&cell->pipe, 0x0);

}
