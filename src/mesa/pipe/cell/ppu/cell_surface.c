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
#if 0
   for (i = 0; i < cell->num_spus; i++) {
      struct cell_command_framebuffer *fb = &cell_global.command[i].fb;
      printf("%s %u start = 0x%x\n", __FUNCTION__, i, ps->map);
      fb->color_start = ps->map;
      fb->width = ps->width;
      fb->height = ps->height;
      fb->color_format = ps->format;
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_FRAMEBUFFER);
   }
#endif

   for (i = 0; i < cell->num_spus; i++) {
#if 1
      uint clr = clearValue;
      /* XXX debug: clear color varied per-SPU to visualize tiles */
      if ((clr & 0xff) == 0)
         clr |= 64 + i * 8;
      if ((clr & 0xff00) == 0)
         clr |= (64 + i * 8) << 8;
      if ((clr & 0xff0000) == 0)
         clr |= (64 + i * 8) << 16;
      if ((clr & 0xff000000) == 0)
         clr |= (64 + i * 8) << 24;
      cell_global.command[i].clear.value = clr;
#else
      cell_global.command[i].clear.value = clearValue;
#endif
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_CLEAR_TILES);
   }
}
