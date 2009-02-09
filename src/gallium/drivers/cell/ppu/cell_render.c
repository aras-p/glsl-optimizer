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
 * \brief  Last stage of 'draw' pipeline: send tris to SPUs.
 * \author  Brian Paul
 */

#include "cell_context.h"
#include "cell_render.h"
#include "cell_spu.h"
#include "util/u_memory.h"
#include "draw/draw_private.h"


struct render_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct cell_context *cell;
};


static INLINE struct render_stage *
render_stage(struct draw_stage *stage)
{
   return (struct render_stage *) stage;
}


static void render_begin( struct draw_stage *stage )
{
#if 0
   struct render_stage *render = render_stage(stage);
   struct cell_context *sp = render->cell;
   const struct pipe_shader_state *fs = &render->cell->fs->shader;
   render->quad.nr_attrs = render->cell->nr_frag_attrs;

   render->firstFpInput = fs->input_semantic_name[0];

   sp->quad.first->begin(sp->quad.first);
#endif
}


static void render_end( struct draw_stage *stage )
{
}


static void reset_stipple_counter( struct draw_stage *stage )
{
   struct render_stage *render = render_stage(stage);
   /*render->cell->line_stipple_counter = 0;*/
}


static void
render_point(struct draw_stage *stage, struct prim_header *prim)
{
}


static void
render_line(struct draw_stage *stage, struct prim_header *prim)
{
}


/** Write a vertex into the prim buffer */
static void
save_vertex(struct cell_prim_buffer *buf, uint pos,
            const struct vertex_header *vert)
{
   uint attr, j;

   for (attr = 0; attr < 2; attr++) {
      for (j = 0; j < 4; j++) {
         buf->vertex[pos][attr][j] = vert->data[attr][j];
      }
   }

   /* update bounding box */
   if (vert->data[0][0] < buf->xmin)
      buf->xmin = vert->data[0][0];
   if (vert->data[0][0] > buf->xmax)
      buf->xmax = vert->data[0][0];
   if (vert->data[0][1] < buf->ymin)
      buf->ymin = vert->data[0][1];
   if (vert->data[0][1] > buf->ymax)
      buf->ymax = vert->data[0][1];
}


static void
render_tri(struct draw_stage *stage, struct prim_header *prim)
{
   struct render_stage *rs = render_stage(stage);
   struct cell_context *cell = rs->cell;
   struct cell_prim_buffer *buf = &cell->prim_buffer;
   uint i;

   if (buf->num_verts + 3 > CELL_MAX_VERTS) {
      cell_flush_prim_buffer(cell);
   }

   i = buf->num_verts;
   assert(i+2 <= CELL_MAX_VERTS);
   save_vertex(buf, i+0, prim->v[0]);
   save_vertex(buf, i+1, prim->v[1]);
   save_vertex(buf, i+2, prim->v[2]);
   buf->num_verts += 3;
}


/**
 * Send the a RENDER command to all SPUs to have them render the prims
 * in the current prim_buffer.
 */
void
cell_flush_prim_buffer(struct cell_context *cell)
{
   uint i;

   if (cell->prim_buffer.num_verts == 0)
      return;

   for (i = 0; i < cell->num_spus; i++) {
      struct cell_command_render *render = &cell_global.command[i].render;
      render->prim_type = PIPE_PRIM_TRIANGLES;
      render->num_verts = cell->prim_buffer.num_verts;
      render->front_winding = cell->rasterizer->front_winding;
      render->vertex_size = cell->vertex_info->size * 4;
      render->xmin = cell->prim_buffer.xmin;
      render->ymin = cell->prim_buffer.ymin;
      render->xmax = cell->prim_buffer.xmax;
      render->ymax = cell->prim_buffer.ymax;
      render->vertex_data = &cell->prim_buffer.vertex;
      ASSERT_ALIGN16(render->vertex_data);
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_RENDER);
   }

   cell->prim_buffer.num_verts = 0;

   cell->prim_buffer.xmin = 1e100;
   cell->prim_buffer.ymin = 1e100;
   cell->prim_buffer.xmax = -1e100;
   cell->prim_buffer.ymax = -1e100;

   /* XXX temporary, need to double-buffer the prim buffer until we get
    * a real command buffer/list system.
    */
   cell_flush(&cell->pipe, 0x0);
}



static void render_destroy( struct draw_stage *stage )
{
   FREE( stage );
}


/**
 * Create a new draw/render stage.  This will be plugged into the
 * draw module as the last pipeline stage.
 */
struct draw_stage *cell_draw_render_stage( struct cell_context *cell )
{
   struct render_stage *render = CALLOC_STRUCT(render_stage);

   render->cell = cell;
   render->stage.draw = cell->draw;
   render->stage.begin = render_begin;
   render->stage.point = render_point;
   render->stage.line = render_line;
   render->stage.tri = render_tri;
   render->stage.end = render_end;
   render->stage.reset_stipple_counter = reset_stipple_counter;
   render->stage.destroy = render_destroy;

   /*
   render->quad.coef = render->coef;
   render->quad.posCoef = &render->posCoef;
   */

   return &render->stage;
}
