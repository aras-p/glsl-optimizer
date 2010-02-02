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
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"
#include "cell/common.h"
#include "cell_clear.h"
#include "cell_context.h"
#include "cell_batch.h"
#include "cell_flush.h"
#include "cell_spu.h"
#include "cell_state.h"


/**
 * Called via pipe->clear()
 */
void
cell_clear(struct pipe_context *pipe, unsigned buffers, const float *rgba,
           double depth, unsigned stencil)
{
   struct cell_context *cell = cell_context(pipe);

   if (cell->dirty)
      cell_update_derived(cell);

   if (buffers & PIPE_CLEAR_COLOR) {
      uint surfIndex = 0;
      union util_color uc;

      util_pack_color(rgba, cell->framebuffer.cbufs[0]->format, &uc);

      /* Build a CLEAR command and place it in the current batch buffer */
      STATIC_ASSERT(sizeof(struct cell_command_clear_surface) % 16 == 0);
      struct cell_command_clear_surface *clr
         = (struct cell_command_clear_surface *)
         cell_batch_alloc16(cell, sizeof(*clr));
      clr->opcode[0] = CELL_CMD_CLEAR_SURFACE;
      clr->surface = surfIndex;
      clr->value = uc.ui;
   }

   if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      uint surfIndex = 1;
      uint clearValue;

      clearValue = util_pack_z_stencil(cell->framebuffer.zsbuf->format,
                                       depth, stencil);

      /* Build a CLEAR command and place it in the current batch buffer */
      STATIC_ASSERT(sizeof(struct cell_command_clear_surface) % 16 == 0);
      struct cell_command_clear_surface *clr
         = (struct cell_command_clear_surface *)
         cell_batch_alloc16(cell, sizeof(*clr));
      clr->opcode[0] = CELL_CMD_CLEAR_SURFACE;
      clr->surface = surfIndex;
      clr->value = clearValue;
   }
}
