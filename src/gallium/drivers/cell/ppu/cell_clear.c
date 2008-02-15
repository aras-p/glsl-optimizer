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
#include "cell_flush.h"
#include "cell_spu.h"


void
cell_clear_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                   unsigned clearValue)
{
   struct cell_context *cell = cell_context(pipe);
   uint surfIndex;

   if (cell->dirty)
      cell_update_derived(cell);


   if (!cell->cbuf_map[0])
      cell->cbuf_map[0] = pipe_surface_map(ps);

   if (ps == cell->framebuffer.zsbuf) {
      surfIndex = 1;
   }
   else {
      surfIndex = 0;
   }


   {
      struct cell_command_clear_surface *clr
         = (struct cell_command_clear_surface *)
         cell_batch_alloc(cell, sizeof(*clr));
      clr->opcode = CELL_CMD_CLEAR_SURFACE;
      clr->surface = surfIndex;
      clr->value = clearValue;
   }
}
