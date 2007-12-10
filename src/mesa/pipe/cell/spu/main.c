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

#include "tri.h"
#include "pipe/cell/common.h"

/*
helpful headers:
/usr/lib/gcc/spu/4.1.1/include/spu_mfcio.h
/opt/ibm/cell-sdk/prototype/sysroot/usr/include/libmisc.h
*/

static struct cell_init_info init;

struct framebuffer {
   void *start;
   uint width, height;
   uint width_tiles, height_tiles; /**< width and height in tiles */
};
static struct framebuffer fb;


static int DefaultTag = 1;



static inline void
wait_on_mask(unsigned tag)
{
   mfc_write_tag_mask( tag );
   mfc_read_tag_status_any();
}



static void
get_tile(const struct framebuffer *fb, uint tx, uint ty, uint *tile)
{
   uint offset = ty * fb->width_tiles + tx;
   uint bytesPerTile = TILE_SIZE * TILE_SIZE * 4;
   ubyte *src = (ubyte *) fb->start + offset * bytesPerTile;
   int tag = DefaultTag;

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

static void
put_tile(const struct framebuffer *fb, uint tx, uint ty, const uint *tile)
{
   uint offset = ty * fb->width_tiles + tx;
   uint bytesPerTile = TILE_SIZE * TILE_SIZE * 4;
   ubyte *dst = (ubyte *) fb->start + offset * bytesPerTile;
   int tag = DefaultTag;

   assert(tx < fb->width_tiles);
   assert(ty < fb->height_tiles);
   ASSERT_ALIGN16(tile);
   /*
   printf("put_tile:  src: %p  dst: 0x%x  size: %d\n",
          tile, (unsigned int) dst, bytesPerTile);
   */
   mfc_put((void *) tile,  /* src in local memory */
           (unsigned int) dst,  /* dst in main mory */
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
   uint tile[TILE_SIZE * TILE_SIZE] ALIGN16;

   for (i = 0; i < TILE_SIZE * TILE_SIZE; i++)
      tile[i] = clear->value;

   printf("SPU: %s num=%d w=%d h=%d\n",
          __FUNCTION__, num_tiles, fb.width_tiles, fb.height_tiles);
   for (i = init.id; i < num_tiles; i += init.num_spus) {
      uint tx = i % fb.width_tiles;
      uint ty = i / fb.width_tiles;
      put_tile(&fb, tx, ty, tile);
      /* XXX we don't want this here, but it fixes bad tile results */
      wait_on_mask(1 << DefaultTag);
   }
}


/** Invert all pixels in all tiles */
static void
invert_tiles(void)
{
   uint num_tiles = fb.width_tiles * fb.height_tiles;
   uint i, j;
   uint tile[TILE_SIZE * TILE_SIZE] ALIGN16;

   for (i = init.id; i < num_tiles; i += init.num_spus) {
      uint tx = i % fb.width_tiles;
      uint ty = i / fb.width_tiles;

      get_tile(&fb, tx, ty, tile);
      wait_on_mask(1 << DefaultTag);

      for (j = 0; j < TILE_SIZE * TILE_SIZE; j++) {
         tile[j] = ~tile[j];
      }

      put_tile(&fb, tx, ty, tile);
   }
}


struct cell_command cmd ALIGN16;


/**
 * Temporary/simple main loop for SPEs: Get a command, execute it, repeat.
 */
static void
main_loop(void)
{
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
         printf("SPU %u: FRAMEBUFFER: %d x %d at %p\n", init.id,
                cmd.fb.width,
                cmd.fb.height,
                cmd.fb.start);
         fb.width = cmd.fb.width;
         fb.height = cmd.fb.height;
         fb.width_tiles = fb.width / TILE_SIZE;
         fb.height_tiles = fb.height / TILE_SIZE;
         fb.start = cmd.fb.start;
         break;
      case CELL_CMD_CLEAR_TILES:
         printf("SPU %u: CLEAR to 0x%08x\n", init.id, cmd.clear.value);
         clear_tiles(&cmd.clear);
         break;
      case CELL_CMD_INVERT_TILES:
         printf("SPU %u: INVERT_TILES\n", init.id);
         invert_tiles();
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



int
main(unsigned long long speid,
     unsigned long long argp,
     unsigned long long envp)
{
   int tag = 0;

   (void) speid;
   (void) envp;

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
