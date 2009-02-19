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
 * Vertex buffer code.  The draw module transforms vertices to window
 * coords, etc. and emits the vertices into buffer supplied by this module.
 * When a vertex buffer is full, or we flush, we'll send the vertex data
 * to the SPUs.
 *
 * Authors
 *  Brian Paul
 */


#include "cell_batch.h"
#include "cell_context.h"
#include "cell_fence.h"
#include "cell_flush.h"
#include "cell_spu.h"
#include "cell_vbuf.h"
#include "draw/draw_vbuf.h"
#include "util/u_memory.h"


/** Allow vertex data to be inlined after RENDER command */
#define ALLOW_INLINE_VERTS 1


/**
 * Subclass of vbuf_render because we need a cell_context pointer in
 * a few places.
 */
struct cell_vbuf_render
{
   struct vbuf_render base;
   struct cell_context *cell;
   uint prim;            /**< PIPE_PRIM_x */
   uint vertex_size;     /**< in bytes */
   void *vertex_buffer;  /**< just for debug, really */
   uint vertex_buf;      /**< in [0, CELL_NUM_BUFFERS-1] */
   uint vertex_buffer_size;  /**< size in bytes */
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


static boolean
cell_vbuf_allocate_vertices(struct vbuf_render *vbr,
                            ushort vertex_size, ushort nr_vertices)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   unsigned size = vertex_size * nr_vertices;
   /*printf("Alloc verts %u * %u\n", vertex_size, nr_vertices);*/

   assert(cvbr->vertex_buf == ~0);
   cvbr->vertex_buf = cell_get_empty_buffer(cvbr->cell);
   cvbr->vertex_buffer = cvbr->cell->buffer[cvbr->vertex_buf];
   cvbr->vertex_buffer_size = size;
   cvbr->vertex_size = vertex_size;

   return cvbr->vertex_buffer != NULL;
}


static void
cell_vbuf_release_vertices(struct vbuf_render *vbr)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   struct cell_context *cell = cvbr->cell;

   /*
   printf("%s vertex_buf = %u  count = %u\n",
          __FUNCTION__, cvbr->vertex_buf, vertices_used);
   */

   /* Make sure texture buffers aren't released until we're done rendering
    * with them.
    */
   cell_add_fenced_textures(cell);

   /* Tell SPUs they can release the vert buf */
   if (cvbr->vertex_buf != ~0U) {
      STATIC_ASSERT(sizeof(struct cell_command_release_verts) % 16 == 0);
      struct cell_command_release_verts *release
         = (struct cell_command_release_verts *)
         cell_batch_alloc16(cell, sizeof(struct cell_command_release_verts));
      release->opcode[0] = CELL_CMD_RELEASE_VERTS;
      release->vertex_buf = cvbr->vertex_buf;
   }

   cvbr->vertex_buf = ~0;
   cell_flush_int(cell, 0x0);

   cvbr->vertex_buffer = NULL;
}


static void *
cell_vbuf_map_vertices(struct vbuf_render *vbr)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   return cvbr->vertex_buffer;
}


static void 
cell_vbuf_unmap_vertices(struct vbuf_render *vbr, 
                         ushort min_index,
                         ushort max_index )
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   assert( cvbr->vertex_buffer_size >= (max_index+1) * cvbr->vertex_size );
   /* do nothing */
}



static boolean
cell_vbuf_set_primitive(struct vbuf_render *vbr, unsigned prim)
{
   struct cell_vbuf_render *cvbr = cell_vbuf_render(vbr);
   cvbr->prim = prim;
   /*printf("cell_set_prim %u\n", prim);*/
   return TRUE;
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
   uint nr_vertices = 0, min_index = ~0;
   const void *vertices = cvbr->vertex_buffer;
   const uint vertex_size = cvbr->vertex_size;

   for (i = 0; i < nr_indices; i++) {
      if (indices[i] > nr_vertices)
         nr_vertices = indices[i];
      if (indices[i] < min_index)
         min_index = indices[i];
   }
   nr_vertices++;

#if 0
   /*if (min_index > 0)*/
      printf("%s min_index = %u\n", __FUNCTION__, min_index);
#endif

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
   printf("ind space = %u, vert space = %u, space = %u\n",
          nr_indices * 2,
          nr_vertices * 4 * cell->vertex_info.size,
          cell_batch_free_space(cell));
#endif

   /* compute x/y bounding box */
   xmin = ymin = 1e50;
   xmax = ymax = -1e50;
   for (i = min_index; i < nr_vertices; i++) {
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
#if 0
   printf("PPU Bounds %g, %g .. %g, %g\n", xmin, ymin, xmax, ymax);
   fflush(stdout);
#endif

   if (cvbr->prim != PIPE_PRIM_TRIANGLES)
      return; /* only render tris for now */

   /* build/insert batch RENDER command */
   {
      const uint index_bytes = ROUNDUP16(nr_indices * 2);
      const uint vertex_bytes = ROUNDUP16(nr_vertices * 4 * cell->vertex_info.size);
      STATIC_ASSERT(sizeof(struct cell_command_render) % 16 == 0);
      const uint batch_size = sizeof(struct cell_command_render) + index_bytes;

      struct cell_command_render *render
         = (struct cell_command_render *)
         cell_batch_alloc16(cell, batch_size);

      render->opcode[0] = CELL_CMD_RENDER;
      render->prim_type = cvbr->prim;

      render->num_indexes = nr_indices;
      render->min_index = min_index;

      /* append indices after render command */
      memcpy(render + 1, indices, nr_indices * 2);

      /* if there's room, append vertices after the indices, else leave
       * vertices in the original/separate buffer.
       */
      render->vertex_size = 4 * cell->vertex_info.size;
      render->num_verts = nr_vertices;
      if (ALLOW_INLINE_VERTS &&
          min_index == 0 &&
          vertex_bytes + 16 <= cell_batch_free_space(cell)) {
         /* vertex data inlined, after indices, at 16-byte boundary */
         void *dst = cell_batch_alloc16(cell, vertex_bytes);
         memcpy(dst, vertices, vertex_bytes);
         render->inline_verts = TRUE;
         render->vertex_buf = ~0;
      }
      else {
         /* vertex data in separate buffer */
         render->inline_verts = FALSE;
         ASSERT(cvbr->vertex_buf >= 0);
         render->vertex_buf = cvbr->vertex_buf;
      }

      render->xmin = xmin;
      render->ymin = ymin;
      render->xmax = xmax;
      render->ymax = ymax;
   }

#if 0
   /* helpful for debug */
   cell_flush_int(cell, CELL_FLUSH_WAIT);
#endif
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

   /* The max number of indexes is what can fix into a batch buffer,
    * minus the render and release-verts commands.
    */
   cell->vbuf_render->base.max_indices
      = (CELL_BUFFER_SIZE
         - sizeof(struct cell_command_render)
         - sizeof(struct cell_command_release_verts))
      / sizeof(ushort);
   cell->vbuf_render->base.max_vertex_buffer_bytes = CELL_BUFFER_SIZE;

   cell->vbuf_render->base.get_vertex_info = cell_vbuf_get_vertex_info;
   cell->vbuf_render->base.allocate_vertices = cell_vbuf_allocate_vertices;
   cell->vbuf_render->base.map_vertices = cell_vbuf_map_vertices;
   cell->vbuf_render->base.unmap_vertices = cell_vbuf_unmap_vertices;
   cell->vbuf_render->base.set_primitive = cell_vbuf_set_primitive;
   cell->vbuf_render->base.draw = cell_vbuf_draw;
   cell->vbuf_render->base.release_vertices = cell_vbuf_release_vertices;
   cell->vbuf_render->base.destroy = cell_vbuf_destroy;

   cell->vbuf_render->cell = cell;
#if 1
   cell->vbuf_render->vertex_buf = ~0;
#endif

   cell->vbuf = draw_vbuf_stage(cell->draw, &cell->vbuf_render->base);
}
