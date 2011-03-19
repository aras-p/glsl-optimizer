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
#include "util/u_memory.h"
#include "pipe/p_screen.h"
#include "util/u_inlines.h"

#include "draw/draw_context.h"
#include "draw/draw_private.h"

#include "cell/common.h"
#include "cell_batch.h"
#include "cell_clear.h"
#include "cell_context.h"
#include "cell_draw_arrays.h"
#include "cell_fence.h"
#include "cell_flush.h"
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
   unsigned i;

   for (i = 0; i < cell->num_vertex_buffers; i++) {
      pipe_resource_reference(&cell->vertex_buffer[i].buffer, NULL);
   }

   util_delete_keymap(cell->fragment_ops_cache, NULL);

   cell_spu_exit(cell);

   align_free(cell);
}


static struct draw_context *
cell_draw_create(struct cell_context *cell)
{
   struct draw_context *draw = draw_create(&cell->pipe);

#if 0 /* broken */
   if (getenv("GALLIUM_CELL_VS")) {
      /* plug in SPU-based vertex transformation code */
      draw->shader_queue_flush = cell_vertex_shader_queue_flush;
      draw->driver_private = cell;
   }
#endif

   return draw;
}


static const struct debug_named_value cell_debug_flags[] = {
   {"checker", CELL_DEBUG_CHECKER, NULL},/**< modulate tile clear color by SPU ID */
   {"asm", CELL_DEBUG_ASM, NULL},        /**< dump SPU asm code */
   {"sync", CELL_DEBUG_SYNC, NULL},      /**< SPUs do synchronous DMA */
   {"fragops", CELL_DEBUG_FRAGMENT_OPS, NULL}, /**< SPUs emit fragment ops debug messages*/
   {"fragopfallback", CELL_DEBUG_FRAGMENT_OP_FALLBACK, NULL}, /**< SPUs use reference implementation for fragment ops*/
   {"cmd", CELL_DEBUG_CMD, NULL},       /**< SPUs dump command buffer info */
   {"cache", CELL_DEBUG_CACHE, NULL},   /**< report texture cache stats on exit */
   DEBUG_NAMED_VALUE_END
};


struct pipe_context *
cell_create_context(struct pipe_screen *screen,
                    void *priv )
{
   struct cell_context *cell;
   uint i;

   /* some fields need to be 16-byte aligned, so align the whole object */
   cell = (struct cell_context*) align_malloc(sizeof(struct cell_context), 16);
   if (!cell)
      return NULL;

   memset(cell, 0, sizeof(*cell));

   cell->winsys = NULL;		/* XXX: fixme - get this from screen? */
   cell->pipe.winsys = NULL;
   cell->pipe.screen = screen;
   cell->pipe.priv = priv;
   cell->pipe.destroy = cell_destroy_context;

   cell->pipe.clear = cell_clear;
   cell->pipe.flush = cell_flush;

#if 0
   cell->pipe.begin_query = cell_begin_query;
   cell->pipe.end_query = cell_end_query;
   cell->pipe.wait_query = cell_wait_query;
#endif

   cell_init_draw_functions(cell);
   cell_init_state_functions(cell);
   cell_init_shader_functions(cell);
   cell_init_surface_functions(cell);
   cell_init_vertex_functions(cell);
   cell_init_texture_transfer_funcs(cell);

   cell->draw = cell_draw_create(cell);

   /* Create cache of fragment ops generated code */
   cell->fragment_ops_cache =
      util_new_keymap(sizeof(struct cell_fragment_ops_key), ~0, NULL);

   cell_init_vbuf(cell);

   draw_set_rasterize_stage(cell->draw, cell->vbuf);

   /* convert all points/lines to tris for the time being */
   draw_wide_point_threshold(cell->draw, 0.0);
   draw_wide_line_threshold(cell->draw, 0.0);

   /* get env vars or read config file to get debug flags */
   cell->debug_flags = debug_get_flags_option("CELL_DEBUG", 
                                              cell_debug_flags, 
                                              0 );

   for (i = 0; i < CELL_NUM_BUFFERS; i++)
      cell_fence_init(&cell->fenced_buffers[i].fence);


   /*
    * SPU stuff
    */
   /* This call only works with SDK 3.0.  Anyone still using 2.1??? */
   cell->num_cells = spe_cpu_info_get(SPE_COUNT_PHYSICAL_CPU_NODES, -1);
   cell->num_spus = spe_cpu_info_get(SPE_COUNT_USABLE_SPES, -1);
   if (cell->debug_flags) {
      printf("Cell: found %d Cell(s) with %u SPUs\n",
             cell->num_cells, cell->num_spus);
   }
   if (getenv("CELL_NUM_SPUS")) {
      cell->num_spus = atoi(getenv("CELL_NUM_SPUS"));
      assert(cell->num_spus > 0);
   }

   cell_start_spus(cell);

   cell_init_batch_buffers(cell);

   /* make sure SPU initializations are done before proceeding */
   cell_flush_int(cell, CELL_FLUSH_WAIT);

   return &cell->pipe;
}
