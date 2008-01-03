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


/* main() for Cell SPU code */


#include <stdio.h>
#include <assert.h>
#include <libmisc.h>
#include <spu_mfcio.h>

#include "main.h"
#include "tri.h"
#include "pipe/cell/common.h"
#include "pipe/p_defines.h"

/*
helpful headers:
/usr/lib/gcc/spu/4.1.1/include/spu_mfcio.h
/opt/ibm/cell-sdk/prototype/sysroot/usr/include/libmisc.h
*/

volatile struct cell_init_info init;

struct framebuffer fb;

uint tile[TILE_SIZE][TILE_SIZE] ALIGN16_ATTRIB;

int DefaultTag;



void
wait_on_mask(unsigned tag)
{
   mfc_write_tag_mask( tag );
   mfc_read_tag_status_any();
}



void
get_tile(const struct framebuffer *fb, uint tx, uint ty, uint *tile,
         int tag)
{
   uint offset = ty * fb->width_tiles + tx;
   uint bytesPerTile = TILE_SIZE * TILE_SIZE * 4;
   ubyte *src = (ubyte *) fb->color_start + offset * bytesPerTile;

   assert(tx < fb->width_tiles);
   assert(ty < fb->height_tiles);
   ASSERT_ALIGN16(tile);
   /*
   printf("get_tile:  dest: %p  src: 0x%x  size: %d\n",
          tile, (unsigned int) src, bytesPerTile);
   */
   mfc_get(tile,  /* dest in local memory */
           (unsigned int) src, /* src in main memory */
           bytesPerTile,
           tag,
           0, /* tid */
           0  /* rid */);
}


void
put_tile(const struct framebuffer *fb, uint tx, uint ty, const uint *tile,
         int tag)
{
   uint offset = ty * fb->width_tiles + tx;
   uint bytesPerTile = TILE_SIZE * TILE_SIZE * 4;
   ubyte *dst = (ubyte *) fb->color_start + offset * bytesPerTile;

   assert(tx < fb->width_tiles);
   assert(ty < fb->height_tiles);
   ASSERT_ALIGN16(tile);
   /*
   printf("put_tile:  src: %p  dst: 0x%x  size: %d\n",
          tile, (unsigned int) dst, bytesPerTile);
   */
   mfc_put((void *) tile,  /* src in local memory */
           (unsigned int) dst,  /* dst in main memory */
           bytesPerTile,
           tag,
           0, /* tid */
           0  /* rid */);
}



static void
clear_tiles(const struct cell_command_clear_tiles *clear)
{
   uint num_tiles = fb.width_tiles * fb.height_tiles;
   uint i;
   uint tile[TILE_SIZE * TILE_SIZE] ALIGN16_ATTRIB;
   int tag = init.id;

   for (i = 0; i < TILE_SIZE * TILE_SIZE; i++)
      tile[i] = clear->value;

   /*
   printf("SPU: %s num=%d w=%d h=%d\n",
          __FUNCTION__, num_tiles, fb.width_tiles, fb.height_tiles);
   */

   for (i = init.id; i < num_tiles; i += init.num_spus) {
      uint tx = i % fb.width_tiles;
      uint ty = i / fb.width_tiles;
      put_tile(&fb, tx, ty, tile, tag);
      /* XXX we don't want this here, but it fixes bad tile results */
      wait_on_mask(1 << tag);
   }

}


/**
 * Given a rendering command's bounding box (in pixels) compute the
 * location of the corresponding screen tile bounding box.
 */
static INLINE void
tile_bounding_box(const struct cell_command_render *render,
                  uint *txmin, uint *tymin,
                  uint *box_num_tiles, uint *box_width_tiles)
{
#if 1
   /* Debug: full-window bounding box */
   uint txmax = fb.width_tiles - 1;
   uint tymax = fb.height_tiles - 1;
   *txmin = 0;
   *tymin = 0;
   *box_num_tiles = fb.width_tiles * fb.height_tiles;
   *box_width_tiles = fb.width_tiles;
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
   printf("Render bounds: %g, %g  ...  %g, %g\n",
          render->xmin, render->ymin, render->xmax, render->ymax);
   printf("Render tiles:  %u, %u .. %u, %u\n", *txmin, *tymin, txmax, tymax);
#endif
}



static void
render(const struct cell_command_render *render)
{
   struct cell_prim_buffer prim_buffer ALIGN16_ATTRIB;
   int tag = init.id /**DefaultTag**/;
   uint i, j, vertex_bytes;

   /*
   printf("SPU %u: RENDER buffer dst=%p  src=%p  size=%d\n",
          init.id,
          &prim_buffer, render->vertex_data, (int)sizeof(prim_buffer));
   */

   ASSERT_ALIGN16(render->vertex_data);
   ASSERT_ALIGN16(&prim_buffer);

   /* how much vertex data */
   vertex_bytes = render->num_verts * render->num_attribs * 4 * sizeof(float);

   /* get vertex data from main memory */
   mfc_get(&prim_buffer,  /* dest */
           (unsigned int) render->vertex_data,  /* src */
           vertex_bytes,  /* size */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask( 1 << tag );  /* XXX temporary */

   /* find tiles which intersect the prim bounding box */
   uint txmin, tymin, box_width_tiles, box_num_tiles;
   tile_bounding_box(render, &txmin, &tymin,
                     &box_num_tiles, &box_width_tiles);


   /* loop over tiles */
   for (i = init.id; i < box_num_tiles; i += init.num_spus) {
      const uint tx = txmin + i % box_width_tiles;
      const uint ty = tymin + i / box_width_tiles;

      assert(tx < fb.width_tiles);
      assert(ty < fb.height_tiles);

      get_tile(&fb, tx, ty, (uint *) tile, tag);
      wait_on_mask(1 << tag);  /* XXX temporary */

      assert(render->prim_type == PIPE_PRIM_TRIANGLES);

      /* loop over tris */
      for (j = 0; j < render->num_verts; j += 3) {
         struct prim_header prim;

         /*
         printf("  %u: Triangle %g,%g  %g,%g  %g,%g\n",
                init.id,
                prim_buffer.vertex[j*3+0][0][0],
                prim_buffer.vertex[j*3+0][0][1],
                prim_buffer.vertex[j*3+1][0][0],
                prim_buffer.vertex[j*3+1][0][1],
                prim_buffer.vertex[j*3+2][0][0],
                prim_buffer.vertex[j*3+2][0][1]);
         */

         /* pos */
         COPY_4V(prim.v[0].data[0], prim_buffer.vertex[j+0][0]);
         COPY_4V(prim.v[1].data[0], prim_buffer.vertex[j+1][0]);
         COPY_4V(prim.v[2].data[0], prim_buffer.vertex[j+2][0]);

         /* color */
         COPY_4V(prim.v[0].data[1], prim_buffer.vertex[j+0][1]);
         COPY_4V(prim.v[1].data[1], prim_buffer.vertex[j+1][1]);
         COPY_4V(prim.v[2].data[1], prim_buffer.vertex[j+2][1]);

         tri_draw(&prim, tx, ty);
      }

      put_tile(&fb, tx, ty, (uint *) tile, tag);
      wait_on_mask(1 << tag); /* XXX temp */
   }
}


/**
 * Temporary/simple main loop for SPEs: Get a command, execute it, repeat.
 */
static void
main_loop(void)
{
   struct cell_command cmd;
   int exitFlag = 0;

   printf("SPU %u: Enter main loop\n", init.id);

   assert((sizeof(struct cell_command) & 0xf) == 0);
   ASSERT_ALIGN16(&cmd);

   while (!exitFlag) {
      unsigned opcode;
      int tag = 0;

      printf("SPU %u: Wait for cmd...\n", init.id);

      /* read/wait from mailbox */
      opcode = (unsigned int) spu_read_in_mbox();

      printf("SPU %u: got cmd %u\n", init.id, opcode);

      /* command payload */
      mfc_get(&cmd,  /* dest */
              (unsigned int) init.cmd, /* src */
              sizeof(struct cell_command), /* bytes */
              tag,
              0, /* tid */
              0  /* rid */);
      wait_on_mask( 1 << tag );

      switch (opcode) {
      case CELL_CMD_EXIT:
         printf("SPU %u: EXIT\n", init.id);
         exitFlag = 1;
         break;
      case CELL_CMD_FRAMEBUFFER:
         printf("SPU %u: FRAMEBUFFER: %d x %d at %p, format 0x%x\n", init.id,
                cmd.fb.width,
                cmd.fb.height,
                cmd.fb.color_start,
                cmd.fb.color_format);
         fb.color_start = cmd.fb.color_start;
         fb.depth_start = cmd.fb.depth_start;
         fb.color_format = cmd.fb.color_format;
         fb.depth_format = cmd.fb.depth_format;
         fb.width = cmd.fb.width;
         fb.height = cmd.fb.height;
         fb.width_tiles = (fb.width + TILE_SIZE - 1) / TILE_SIZE;
         fb.height_tiles = (fb.height + TILE_SIZE - 1) / TILE_SIZE;
         /*
         printf("SPU %u: %u x %u tiles\n",
                init.id, fb.width_tiles, fb.height_tiles);
         */
         break;
      case CELL_CMD_CLEAR_TILES:
         printf("SPU %u: CLEAR to 0x%08x\n", init.id, cmd.clear.value);
         clear_tiles(&cmd.clear);
         break;
      case CELL_CMD_RENDER:
         printf("SPU %u: RENDER %u verts, prim %u\n",
                init.id, cmd.render.num_verts, cmd.render.prim_type);
         render(&cmd.render);
         break;

      case CELL_CMD_FINISH:
         printf("SPU %u: FINISH\n", init.id);
         /* wait for all outstanding DMAs to finish */
         mfc_write_tag_mask(~0);
         mfc_read_tag_status_all();
         /* send mbox message to PPU */
         spu_write_out_mbox(CELL_CMD_FINISH);
         break;
      default:
         printf("Bad opcode!\n");
      }

   }

   printf("SPU %u: Exit main loop\n", init.id);
}



/**
 * SPE entrypoint.
 * Note: example programs declare params as 'unsigned long long' but
 * that doesn't work.
 */
int
main(unsigned long speid, unsigned long argp)
{
   int tag = 0;

   (void) speid;

   DefaultTag = 1;

   printf("SPU: main() speid=%lu\n", speid);

   mfc_get(&init,  /* dest */
           (unsigned int) argp, /* src */
           sizeof(struct cell_init_info), /* bytes */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask( 1 << tag );


   main_loop();

   return 0;
}
