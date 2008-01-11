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


#include "cell_context.h"
#include "cell_spu.h"
#include "cell_vbuf.h"
#include "pipe/draw/draw_vbuf.h"


/**
 * Subclass of vbuf_render because we need a cell_context pointer in
 * a few places.
 */
struct cell_vbuf_render
{
   struct vbuf_render base;
   struct cell_context *cell;
   uint prim;
};


/** cast wrapper */
static struct cell_vbuf_render *
cell_vbuf_render(struct vbuf_render *vbr)
{
   return (struct cell_vbuf_render *) vbr;
}



static const struct vertex_info *
cell_vbuf_get_vertex_info(struct vbuf_render *vbr)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   return &cvbr->cell->vertex_info;
}


static void *
cell_vbuf_allocate_vertices(struct vbuf_render *vbr,
                            ushort vertex_size, ushort nr_vertices)
{
   /*printf("Alloc verts %u * %u\n", vertex_size, nr_vertices);*/
   return align_malloc(vertex_size * nr_vertices, 16);
}


static void
cell_vbuf_set_primitive(struct vbuf_render *vbr, unsigned prim)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   cvbr->prim = prim;
   /*printf("cell_set_prim %u\n", prim);*/
}


static void
cell_vbuf_draw(struct vbuf_render *vbr,
               uint prim,
	       const ushort *indices,
               uint nr_indices,
               const void *vertices,
               uint nr_vertices,
               uint vertex_size)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   struct cell_context *cell = cvbr->cell;
   float xmin, ymin, xmax, ymax;
   uint i;

   /* compute x/y bounding box */
   xmin = ymin = 1e50;
   xmax = ymax = -1e50;
   for (i = 0; i < nr_vertices; i++) {
      const float *v = (float *) ((ubyte *) vertices + i * vertex_size);
      if (v[0] < xmin)
         xmin = v[0];
      if (v[0] > xmax)
         xmax = v[0];
      if (v[1] < ymin)
         ymin = v[1];
      if (v[1] > ymax)
         ymax = v[1];
   }

   /*printf("cell_vbuf_draw nr_indices = %u\n", nr_indices);*/

   if (prim != PIPE_PRIM_TRIANGLES)
      return; /* only render tris for now */

   for (i = 0; i < cell->num_spus; i++) {
      struct cell_command_render *render = &cell_global.command[i].render;
      render->prim_type = prim;
      render->num_verts = nr_vertices;
      render->num_attribs = CELL_MAX_ATTRIBS; /* XXX fix */
      render->vertex_data = vertices;
      render->index_data = indices;
      render->num_indexes = nr_indices;
      render->xmin = xmin;
      render->ymin = ymin;
      render->xmax = xmax;
      render->ymax = ymax;

      ASSERT_ALIGN16(render->vertex_data);
      ASSERT_ALIGN16(render->index_data);
      send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_RENDER);
   }

   cell_flush(&cell->pipe, 0x0);
}


static void
cell_vbuf_release_vertices(struct vbuf_render *vbr, void *vertices, 
                           unsigned vertex_size, unsigned vertices_used)
{
   /*printf("Free verts %u * %u\n", vertex_size, vertices_used);*/
   align_free(vertices);
}


static void
cell_vbuf_destroy(struct vbuf_render *vbr)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   cvbr->cell->vbuf_render = NULL;
   FREE(cvbr);
}


/**
 * Initialize the post-transform vertex buffer information for the given
 * context.
 */
void
cell_init_vbuf(struct cell_context *cell)
{
   assert(cell->draw);

   cell->vbuf_render = CALLOC_STRUCT(cell_vbuf_render);

   cell->vbuf_render->base.max_indices = CELL_MAX_VBUF_INDEXES;
   cell->vbuf_render->base.max_vertex_buffer_bytes = CELL_MAX_VBUF_SIZE;

   cell->vbuf_render->base.get_vertex_info = cell_vbuf_get_vertex_info;
   cell->vbuf_render->base.allocate_vertices = cell_vbuf_allocate_vertices;
   cell->vbuf_render->base.set_primitive = cell_vbuf_set_primitive;
   cell->vbuf_render->base.draw = cell_vbuf_draw;
   cell->vbuf_render->base.release_vertices = cell_vbuf_release_vertices;
   cell->vbuf_render->base.destroy = cell_vbuf_destroy;

   cell->vbuf_render->cell = cell;

   cell->vbuf = draw_vbuf_stage(cell->draw, &cell->vbuf_render->base);
}
