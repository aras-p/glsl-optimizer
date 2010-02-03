/*
 * (C) Copyright IBM Corporation 2008
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file cell_vertex_shader.c
 * Vertex shader interface routines for Cell.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "util/u_simple_screen.h"
#include "util/u_math.h"

#include "cell_context.h"
#include "cell_draw_arrays.h"
#include "cell_flush.h"
#include "cell_spu.h"
#include "cell_batch.h"

#include "cell/common.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"

/**
 * Run the vertex shader on all vertices in the vertex queue.
 * Called by the draw module when the vertx cache needs to be flushed.
 */
void
cell_vertex_shader_queue_flush(struct draw_context *draw)
{
#if 0
   struct cell_context *const cell =
       (struct cell_context *) draw->driver_private;
   struct cell_command_vs *const vs = &cell_global.command[0].vs;
   uint64_t *batch;
   struct cell_array_info *array_info;
   unsigned i, j;
   struct cell_attribute_fetch_code *cf;

   assert(draw->vs.queue_nr != 0);

   /* XXX: do this on statechange: 
    */
   draw_update_vertex_fetch(draw);
   cell_update_vertex_fetch(draw);


   batch = cell_batch_alloc(cell, sizeof(batch[0]) + sizeof(*cf));
   batch[0] = CELL_CMD_STATE_ATTRIB_FETCH;
   cf = (struct cell_attribute_fetch_code *) (&batch[1]);
   cf->base = (uint64_t) cell->attrib_fetch.store;
   cf->size = ROUNDUP16((unsigned)((void *) cell->attrib_fetch.csr 
				   - (void *) cell->attrib_fetch.store));


   for (i = 0; i < draw->vertex_fetch.nr_attrs; i++) {
      const enum pipe_format format = draw->vertex_element[i].src_format;
      const unsigned count = ((pf_size_x(format) != 0)
			      + (pf_size_y(format) != 0)
			      + (pf_size_z(format) != 0)
			      + (pf_size_w(format) != 0));
      const unsigned size = pf_size_x(format) * count;

      batch = cell_batch_alloc(cell, sizeof(batch[0]) + sizeof(*array_info));

      batch[0] = CELL_CMD_STATE_VS_ARRAY_INFO;

      array_info = (struct cell_array_info *) &batch[1];
      assert(draw->vertex_fetch.src_ptr[i] != NULL);
      array_info->base = (uintptr_t) draw->vertex_fetch.src_ptr[i];
      array_info->attr = i;
      array_info->pitch = draw->vertex_fetch.pitch[i];
      array_info->size = size;
      array_info->function_offset = cell->attrib_fetch_offsets[i];
   }

   batch = cell_batch_alloc(cell, sizeof(batch[0])
                            + sizeof(struct pipe_viewport_state));
   batch[0] = CELL_CMD_STATE_VIEWPORT;
   (void) memcpy(&batch[1], &draw->viewport,
                 sizeof(struct pipe_viewport_state));

   {
      uint64_t uniforms = (uintptr_t) draw->user.constants;

      batch = cell_batch_alloc(cell, 2 *sizeof(batch[0]));
      batch[0] = CELL_CMD_STATE_UNIFORMS;
      batch[1] = uniforms;
   }

   cell_batch_flush(cell);

   vs->opcode = CELL_CMD_VS_EXECUTE;
   vs->nr_attrs = draw->vertex_fetch.nr_attrs;

   (void) memcpy(vs->plane, draw->plane, sizeof(draw->plane));
   vs->nr_planes = draw->nr_planes;

   for (i = 0; i < draw->vs.queue_nr; i += SPU_VERTS_PER_BATCH) {
      const unsigned n = MIN2(SPU_VERTS_PER_BATCH, draw->vs.queue_nr - i);

      for (j = 0; j < n; j++) {
         vs->elts[j] = draw->vs.queue[i + j].elt;
         vs->vOut[j] = (uintptr_t) draw->vs.queue[i + j].vertex;
      }

      for (/* empty */; j < SPU_VERTS_PER_BATCH; j++) {
         vs->elts[j] = vs->elts[0];
         vs->vOut[j] = (uintptr_t) draw->vs.queue[i + j].vertex;
      }

      vs->num_elts = n;
      send_mbox_message(cell_global.spe_contexts[0], CELL_CMD_VS_EXECUTE);

      cell_flush_int(cell, CELL_FLUSH_WAIT);
   }

   draw->vs.post_nr = draw->vs.queue_nr;
   draw->vs.queue_nr = 0;
#else
   assert(0);
#endif
}
