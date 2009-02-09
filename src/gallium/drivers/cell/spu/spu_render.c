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
#include "cell/common.h"
#include "util/u_memory.h"


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
   if (txmax >= spu.fb.width_tiles)
      txmax = spu.fb.width_tiles-1;
   if (tymax >= spu.fb.height_tiles)
      tymax = spu.fb.height_tiles-1;
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
 * Start fetching non-clear color/Z tiles from main memory
 */
static INLINE void
get_cz_tiles(uint tx, uint ty)
{
   if (spu.read_depth_stencil) {
      if (spu.cur_ztile_status != TILE_STATUS_CLEAR) {
         //printf("SPU %u: getting Z tile %u, %u\n", spu.init.id, tx, ty);
         get_tile(tx, ty, &spu.ztile, TAG_READ_TILE_Z, 1);
         spu.cur_ztile_status = TILE_STATUS_GETTING;
      }
   }

   if (spu.cur_ctile_status != TILE_STATUS_CLEAR) {
      //printf("SPU %u: getting C tile %u, %u\n", spu.init.id, tx, ty);
      get_tile(tx, ty, &spu.ctile, TAG_READ_TILE_COLOR, 0);
      spu.cur_ctile_status = TILE_STATUS_GETTING;
   }
}


/**
 * Start putting dirty color/Z tiles back to main memory
 */
static INLINE void
put_cz_tiles(uint tx, uint ty)
{
   if (spu.cur_ztile_status == TILE_STATUS_DIRTY) {
      /* tile was modified and needs to be written back */
      //printf("SPU %u: put dirty Z tile %u, %u\n", spu.init.id, tx, ty);
      put_tile(tx, ty, &spu.ztile, TAG_WRITE_TILE_Z, 1);
      spu.cur_ztile_status = TILE_STATUS_DEFINED;
   }
   else if (spu.cur_ztile_status == TILE_STATUS_GETTING) {
      /* tile was never used */
      spu.cur_ztile_status = TILE_STATUS_DEFINED;
      //printf("SPU %u: put getting Z tile %u, %u\n", spu.init.id, tx, ty);
   }

   if (spu.cur_ctile_status == TILE_STATUS_DIRTY) {
      /* tile was modified and needs to be written back */
      //printf("SPU %u: put dirty C tile %u, %u\n", spu.init.id, tx, ty);
      put_tile(tx, ty, &spu.ctile, TAG_WRITE_TILE_COLOR, 0);
      spu.cur_ctile_status = TILE_STATUS_DEFINED;
   }
   else if (spu.cur_ctile_status == TILE_STATUS_GETTING) {
      /* tile was never used */
      spu.cur_ctile_status = TILE_STATUS_DEFINED;
      //printf("SPU %u: put getting C tile %u, %u\n", spu.init.id, tx, ty);
   }
}


/**
 * Wait for 'put' of color/z tiles to complete.
 */
static INLINE void
wait_put_cz_tiles(void)
{
   wait_on_mask(1 << TAG_WRITE_TILE_COLOR);
   if (spu.read_depth_stencil) {
      wait_on_mask(1 << TAG_WRITE_TILE_Z);
   }
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
   uint index_bytes;
   const ubyte *vertices;
   const ushort *indexes;
   uint i, j;
   uint num_tiles;

   D_PRINTF(CELL_DEBUG_CMD,
            "RENDER prim=%u num_vert=%u num_ind=%u inline_vert=%u\n",
            render->prim_type,
            render->num_verts,
            render->num_indexes,
            render->inline_verts);

   ASSERT(sizeof(*render) % 4 == 0);
   ASSERT(total_vertex_bytes % 16 == 0);
   ASSERT(render->prim_type == PIPE_PRIM_TRIANGLES);
   ASSERT(render->num_indexes % 3 == 0);


   /* indexes are right after the render command in the batch buffer */
   indexes = (const ushort *) (render + 1);
   index_bytes = ROUNDUP8(render->num_indexes * 2);
   *pos_incr = index_bytes / 8 + sizeof(*render) / 8;


   if (render->inline_verts) {
      /* Vertices are after indexes in batch buffer at next 16-byte addr */
      vertices = (const ubyte *) render + (*pos_incr * 8);
      vertices = (const ubyte *) align_pointer((void *) vertices, 16);
      ASSERT_ALIGN16(vertices);
      *pos_incr = ((vertices + total_vertex_bytes) - (ubyte *) render) / 8;
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


   num_tiles = 0;

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

      num_tiles++;

      spu.cur_ctile_status = spu.ctile_status[ty][tx];
      spu.cur_ztile_status = spu.ztile_status[ty][tx];

      get_cz_tiles(tx, ty);

      uint drawn = 0;

      /* loop over tris */
      for (j = 0; j < render->num_indexes; j += 3) {
         const float *v0, *v1, *v2;

         v0 = (const float *) (vertices + indexes[j+0] * vertex_size);
         v1 = (const float *) (vertices + indexes[j+1] * vertex_size);
         v2 = (const float *) (vertices + indexes[j+2] * vertex_size);

         drawn += tri_draw(v0, v1, v2, tx, ty);
      }

      //printf("SPU %u: drew %u of %u\n", spu.init.id, drawn, render->num_indexes/3);

      /* write color/z tiles back to main framebuffer, if dirtied */
      put_cz_tiles(tx, ty);

      wait_put_cz_tiles(); /* XXX seems unnecessary... */

      spu.ctile_status[ty][tx] = spu.cur_ctile_status;
      spu.ztile_status[ty][tx] = spu.cur_ztile_status;
   }

   D_PRINTF(CELL_DEBUG_CMD,
            "RENDER done (%u tiles hit)\n",
            num_tiles);
}
