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
#include "cell_context.h"
#include "cell_surface.h"
#include "cell_spu.h"


void
cell_clear_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                   unsigned clearValue)
{
   struct cell_context *cell = cell_context(pipe);
   uint i;

   if (!ps->map)
      pipe_surface_map(ps);

   if (pf_get_size(ps->format) != 4) {
      printf("Cell: Skipping non 32bpp clear_surface\n");
      return;
   }

   for (i = 0; i < cell->num_spus; i++) {
      struct cell_command_framebuffer *fb = &cell_global.command[i].fb;
      printf("%s %u start = 0x%x\n", __FUNCTION__, i, ps->map);
      fb->start = ps->map;
      fb->width = ps->width;
      fb->height = ps->height;
      fb->format = ps->format;
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_FRAMEBUFFER);
   }

   for (i = 0; i < cell->num_spus; i++) {
      /* XXX clear color varies per-SPU for debugging */
      cell_global.command[i].clear.value = clearValue | (i << 21);
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_CLEAR_TILES);
   }
}
