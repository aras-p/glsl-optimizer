/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include <stdio.h>
#include <libmisc.h>
#include <spu_mfcio.h>

#include "spu_main.h"
#include "spu_render.h"
#include "spu_tri.h"
#include "spu_tile.h"
#include "pipe/cell/common.h"



/**
 * Given a rendering command's bounding box (in pixels) compute the
 * location of the corresponding screen tile bounding box.
 */
static INLINE void
tile_bounding_box(const struct cell_command_render *render,
                  uint *txmin, uint *tymin,
                  uint *box_num_tiles, uint *box_width_tiles)
{
#if 0
   /* Debug: full-window bounding box */
   uint txmax = spu.fb.width_tiles - 1;
   uint tymax = spu.fb.height_tiles - 1;
   *txmin = 0;
   *tymin = 0;
   *box_num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   *box_width_tiles = spu.fb.width_tiles;
   (void) render;
   (void) txmax;
   (void) tymax;
#else
   uint txmax, tymax, box_height_tiles;

   *txmin = (uint) render->xmin / TILE_SIZE;
   *tymin = (uint) render->ymin / TILE_SIZE;
   txmax = (uint) render->xmax / TILE_SIZE;
   tymax = (uint) render->ymax / TILE_SIZE;
   *box_width_tiles = txmax - *txmin + 1;
   box_height_tiles = tymax - *tymin + 1;
   *box_num_tiles = *box_width_tiles * box_height_tiles;
#endif
#if 0
   printf("SPU %u: bounds: %g, %g  ...  %g, %g\n", spu.init.id,
          render->xmin, render->ymin, render->xmax, render->ymax);
   printf("SPU %u: tiles:  %u, %u .. %u, %u\n",
           spu.init.id, *txmin, *tymin, txmax, tymax);
   ASSERT(render->xmin <= render->xmax);
   ASSERT(render->ymin <= render->ymax);
#endif
}


/** Check if the tile at (tx,ty) belongs to this SPU */
static INLINE boolean
my_tile(uint tx, uint ty)
{
   return (spu.fb.width_tiles * ty + tx) % spu.init.num_spus == spu.init.id;
}


/**
 * Render primitives
 * \param pos_incr  returns value indicating how may words to skip after
 *                  this command in the batch buffer
 */
void
cmd_render(const struct cell_command_render *render, uint *pos_incr)
{
   /* we'll DMA into these buffers */
   ubyte vertex_data[CELL_BUFFER_SIZE] ALIGN16_ATTRIB;
   const uint vertex_size = render->vertex_size; /* in bytes */
   /*const*/ uint total_vertex_bytes = render->num_verts * vertex_size;
   const ubyte *vertices;
   const ushort *indexes;
   uint i, j;


   if (Debug) {
      printf("SPU %u: RENDER prim %u, num_vert=%u  num_ind=%u  "
             "inline_vert=%u\n",
             spu.init.id,
             render->prim_type,
             render->num_verts,
             render->num_indexes,
             render->inline_verts);

      /*
      printf("       bound: %g, %g .. %g, %g\n",
             render->xmin, render->ymin, render->xmax, render->ymax);
      */
   }

   ASSERT(sizeof(*render) % 4 == 0);
   ASSERT(total_vertex_bytes % 16 == 0);

   /* indexes are right after the render command in the batch buffer */
   indexes = (const ushort *) (render + 1);
   *pos_incr = (render->num_indexes * 2 + 3) / 4;


   if (render->inline_verts) {
      /* Vertices are right after indexes in batch buffer */
      vertices = (const ubyte *) (render + 1) + *pos_incr * 4;
      *pos_incr = *pos_incr + total_vertex_bytes / 4;
   }
   else {
      /* Begin DMA fetch of vertex buffer */
      ubyte *src = spu.init.buffers[render->vertex_buf];
      ubyte *dest = vertex_data;

      /* skip vertex data we won't use */
#if 01
      src += render->min_index * vertex_size;
      dest += render->min_index * vertex_size;
      total_vertex_bytes -= render->min_index * vertex_size;
#endif
      ASSERT(total_vertex_bytes % 16 == 0);
      ASSERT_ALIGN16(dest);
      ASSERT_ALIGN16(src);

      mfc_get(dest,   /* in vertex_data[] array */
              (unsigned int) src,  /* src in main memory */
              total_vertex_bytes,  /* size */
              TAG_VERTEX_BUFFER,
              0, /* tid */
              0  /* rid */);

      vertices = vertex_data;

      wait_on_mask(1 << TAG_VERTEX_BUFFER);
   }


   /**
    ** find tiles which intersect the prim bounding box
    **/
   uint txmin, tymin, box_width_tiles, box_num_tiles;
   tile_bounding_box(render, &txmin, &tymin,
                     &box_num_tiles, &box_width_tiles);


   /* make sure any pending clears have completed */
   wait_on_mask(1 << TAG_SURFACE_CLEAR); /* XXX temporary */


   /**
    ** loop over tiles, rendering tris
    **/
   for (i = 0; i < box_num_tiles; i++) {
      const uint tx = txmin + i % box_width_tiles;
      const uint ty = tymin + i / box_width_tiles;

      ASSERT(tx < spu.fb.width_tiles);
      ASSERT(ty < spu.fb.height_tiles);

      if (!my_tile(tx, ty))
         continue;

      /* Start fetching color/z tiles.  We'll wait for completion when
       * we need read/write to them later in triangle rasterization.
       */
      if (spu.depth_stencil.depth.enabled) {
         if (tile_status_z[ty][tx] != TILE_STATUS_CLEAR) {
            get_tile(tx, ty, &ztile, TAG_READ_TILE_Z, 1);
         }
      }

      if (tile_status[ty][tx] != TILE_STATUS_CLEAR) {
         get_tile(tx, ty, &ctile, TAG_READ_TILE_COLOR, 0);
      }

      ASSERT(render->prim_type == PIPE_PRIM_TRIANGLES);
      ASSERT(render->num_indexes % 3 == 0);

      /* loop over tris */
      for (j = 0; j < render->num_indexes; j += 3) {
         const float *v0, *v1, *v2;

         v0 = (const float *) (vertices + indexes[j+0] * vertex_size);
         v1 = (const float *) (vertices + indexes[j+1] * vertex_size);
         v2 = (const float *) (vertices + indexes[j+2] * vertex_size);

         tri_draw(v0, v1, v2, tx, ty);
      }

      /* write color/z tiles back to main framebuffer, if dirtied */
      if (tile_status[ty][tx] == TILE_STATUS_DIRTY) {
         put_tile(tx, ty, &ctile, TAG_WRITE_TILE_COLOR, 0);
         tile_status[ty][tx] = TILE_STATUS_DEFINED;
      }
      if (spu.depth_stencil.depth.enabled) {
         if (tile_status_z[ty][tx] == TILE_STATUS_DIRTY) {
            put_tile(tx, ty, &ztile, TAG_WRITE_TILE_Z, 1);
            tile_status_z[ty][tx] = TILE_STATUS_DEFINED;
         }
      }

      /* XXX move these... */
      wait_on_mask(1 << TAG_WRITE_TILE_COLOR);
      if (spu.depth_stencil.depth.enabled) {
         wait_on_mask(1 << TAG_WRITE_TILE_Z);
      }
   }

   if (Debug)
      printf("SPU %u: RENDER done\n",
             spu.init.id);
}


