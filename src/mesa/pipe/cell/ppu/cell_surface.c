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



struct pipe_surface *
cell_create_surface(int width, int height)
{
#if 0
   /* XXX total hack */
   struct pipe_surface *ps = CALLOC_STRUCT(pipe_surface);

   printf("cell_create_surface\n");

   ps->width = width;
   ps->height = height;

   ps->region = CALLOC_STRUCT(pipe_region);
   ps->region->map = align_malloc(width * height * 4, 16);
   return ps;
#endif
   return NULL;
}



void
cell_clear_surface(struct pipe_context *pipe, struct pipe_surface *ps,
                   unsigned clearValue)
{
   struct cell_context *cell = cell_context(pipe);
   uint i;

   if (!ps->map)
      pipe_surface_map(ps);

   for (i = 0; i < cell->num_spus; i++) {
      command[i].fb.start = ps->map;
      command[i].fb.width = ps->width;
      command[i].fb.height = ps->height;
      command[i].fb.format = ps->format;
      send_mbox_message(spe_contexts[i], CELL_CMD_FRAMEBUFFER);
   }

   for (i = 0; i < cell->num_spus; i++) {
      /* XXX clear color varies per-SPU for debugging */
      command[i].clear.value = clearValue | (i << 21);
      send_mbox_message(spe_contexts[i], CELL_CMD_CLEAR_TILES);
   }

#if 1
   /* XXX Draw a test triangle over the cleared surface */
   for (i = 0; i < cell->num_spus; i++) {
      /* Same triangle data for all SPUs, of course: */
      command[i].tri.x0 = 20.0;
      command[i].tri.y0 = ps->height - 20;

      command[i].tri.x1 = ps->width - 20.0;
      command[i].tri.y1 = ps->height - 20;

      command[i].tri.x2 = ps->width / 2;
      command[i].tri.y2 = 20.0;

      /* XXX color varies per SPU */
      command[i].tri.color = 0xffff00 | ((i*40)<<24);  /* yellow */

      send_mbox_message(spe_contexts[i], CELL_CMD_TRIANGLE);
   }
#endif
}
