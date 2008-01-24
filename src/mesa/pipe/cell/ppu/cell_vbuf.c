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


#include "cell_batch.h"
#include "cell_context.h"
#include "cell_flush.h"
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
   uint vertex_size;
   void *vertex_buffer;
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
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   /*printf("Alloc verts %u * %u\n", vertex_size, nr_vertices);*/
   assert(!cvbr->vertex_buffer);
   cvbr->vertex_buffer = align_malloc(vertex_size * nr_vertices, 16);
   cvbr->vertex_size = vertex_size;
   return cvbr->vertex_buffer;
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
	       const ushort *indices,
               uint nr_indices)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   struct cell_context *cell = cvbr->cell;
   float xmin, ymin, xmax, ymax;
   uint i;
   uint nr_vertices = 0;
   const void *vertices = cvbr->vertex_buffer;
   const uint vertex_size = cvbr->vertex_size;

   for (i = 0; i < nr_indices; i++) {
      if (indices[i] > nr_vertices)
         nr_vertices = indices[i];
   }
   nr_vertices++;

#if 0
   printf("cell_vbuf_draw() nr_indices = %u nr_verts = %u\n",
          nr_indices, nr_vertices);
   printf("  ");
   for (i = 0; i < nr_indices; i += 3) {
      printf("%u %u %u, ", indices[i+0], indices[i+1], indices[i+2]);
   }
   printf("\n");
#elif 0
   printf("cell_vbuf_draw() nr_indices = %u nr_verts = %u  indexes = [%u %u %u ...]\n",
          nr_indices, nr_vertices,
          indices[0], indices[1], indices[2]);
#endif

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

   if (cvbr->prim != PIPE_PRIM_TRIANGLES)
      return; /* only render tris for now */

   /* build/insert batch RENDER command */
   {
      struct cell_command_render *render
         = (struct cell_command_render *)
         cell_batch_alloc(cell, sizeof(*render));
      render->opcode = CELL_CMD_RENDER;
      render->prim_type = cvbr->prim;
      render->num_verts = nr_vertices;
      render->vertex_size = 4 * cell->vertex_info.size;
      render->vertex_data = vertices;
      render->index_data = indices;
      render->num_indexes = nr_indices;
      render->xmin = xmin;
      render->ymin = ymin;
      render->xmax = xmax;
      render->ymax = ymax;

      ASSERT_ALIGN16(render->vertex_data);
      ASSERT_ALIGN16(render->index_data);
   }

#if 01
   /* XXX this is temporary */
   cell_flush(&cell->pipe, PIPE_FLUSH_WAIT);
#endif
}


static void
cell_vbuf_release_vertices(struct vbuf_render *vbr, void *vertices, 
                           unsigned vertex_size, unsigned vertices_used)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);

   /*printf("Free verts %u * %u\n", vertex_size, vertices_used);*/
   align_free(vertices);

   assert(vertices == cvbr->vertex_buffer);
   cvbr->vertex_buffer = NULL;
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
