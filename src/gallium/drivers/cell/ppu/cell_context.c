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

#include "pipe/p_defines.h"
#include "pipe/p_format.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_screen.h"

#include "draw/draw_context.h"
#include "draw/draw_private.h"

#include "cell/common.h"
#include "cell_clear.h"
#include "cell_context.h"
#include "cell_draw_arrays.h"
#include "cell_flush.h"
#include "cell_render.h"
#include "cell_state.h"
#include "cell_surface.h"
#include "cell_spu.h"
#include "cell_pipe_state.h"
#include "cell_texture.h"
#include "cell_vbuf.h"



static void
cell_destroy_context( struct pipe_context *pipe )
{
   struct cell_context *cell = cell_context(pipe);

   cell_spu_exit(cell);

   align_free(cell);
}


static struct draw_context *
cell_draw_create(struct cell_context *cell)
{
   struct draw_context *draw = draw_create();

   if (getenv("GALLIUM_CELL_VS")) {
      /* plug in SPU-based vertex transformation code */
      draw->shader_queue_flush = cell_vertex_shader_queue_flush;
      draw->driver_private = cell;
   }

   return draw;
}


struct pipe_context *
cell_create_context(struct pipe_screen *screen,
                    struct cell_winsys *cws)
{
   struct cell_context *cell;
   uint spu, buf;

   /* some fields need to be 16-byte aligned, so align the whole object */
   cell = (struct cell_context*) align_malloc(sizeof(struct cell_context), 16);
   if (!cell)
      return NULL;

   memset(cell, 0, sizeof(*cell));

   cell->winsys = cws;
   cell->pipe.winsys = screen->winsys;
   cell->pipe.screen = screen;
   cell->pipe.destroy = cell_destroy_context;

   /* state setters */
   cell->pipe.set_vertex_buffers = cell_set_vertex_buffers;
   cell->pipe.set_vertex_elements = cell_set_vertex_elements;

   cell->pipe.draw_arrays = cell_draw_arrays;
   cell->pipe.draw_elements = cell_draw_elements;

   cell->pipe.clear = cell_clear_surface;
   cell->pipe.flush = cell_flush;

#if 0
   cell->pipe.begin_query = cell_begin_query;
   cell->pipe.end_query = cell_end_query;
   cell->pipe.wait_query = cell_wait_query;
#endif

   cell_init_state_functions(cell);
   cell_init_shader_functions(cell);
   cell_init_surface_functions(cell);
   cell_init_texture_functions(cell);

   cell->draw = cell_draw_create(cell);

   cell_init_vbuf(cell);
   draw_set_rasterize_stage(cell->draw, cell->vbuf);

   /* convert all points/lines to tris for the time being */
   draw_wide_point_threshold(cell->draw, 0.0);
   draw_wide_line_threshold(cell->draw, 0.0);

   /*
    * SPU stuff
    */
   cell->num_spus = 6;
   /* XXX is this in SDK 3.0 only?
   cell->num_spus = spe_cpu_info_get(SPE_COUNT_PHYSICAL_SPES, -1);
   */

   cell_start_spus(cell);

   /* init command, vertex/index buffer info */
   for (buf = 0; buf < CELL_NUM_BUFFERS; buf++) {
      cell->buffer_size[buf] = 0;

      /* init batch buffer status values,
       * mark 0th buffer as used, rest as free.
       */
      for (spu = 0; spu < cell->num_spus; spu++) {
         if (buf == 0)
            cell->buffer_status[spu][buf][0] = CELL_BUFFER_STATUS_USED;
         else
            cell->buffer_status[spu][buf][0] = CELL_BUFFER_STATUS_FREE;
      }
   }

   return &cell->pipe;
}
