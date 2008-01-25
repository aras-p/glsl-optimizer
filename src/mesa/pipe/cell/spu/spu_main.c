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
#include <libmisc.h>
#include <spu_mfcio.h>

#include "spu_main.h"
#include "spu_tri.h"
#include "spu_tile.h"
#include "pipe/cell/common.h"
#include "pipe/p_defines.h"


/*
helpful headers:
/usr/lib/gcc/spu/4.1.1/include/spu_mfcio.h
/opt/ibm/cell-sdk/prototype/sysroot/usr/include/libmisc.h
*/

static boolean Debug = FALSE;

struct spu_global spu;


void
wait_on_mask(unsigned tagMask)
{
   mfc_write_tag_mask( tagMask );
   /* wait for completion of _any_ DMAs specified by tagMask */
   mfc_read_tag_status_any();
}


static void
wait_on_mask_all(unsigned tagMask)
{
   mfc_write_tag_mask( tagMask );
   /* wait for completion of _any_ DMAs specified by tagMask */
   mfc_read_tag_status_all();
}



/**
 * For tiles whose status is TILE_STATUS_CLEAR, write solid-filled
 * tiles back to the main framebuffer.
 */
static void
really_clear_tiles(uint surfaceIndex)
{
   const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   uint i;

   if (surfaceIndex == 0) {
      clear_c_tile(&ctile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (tile_status[ty][tx] == TILE_STATUS_CLEAR) {
            put_tile(tx, ty, &ctile, TAG_SURFACE_CLEAR, 0);
         }
      }
   }
   else {
      clear_z_tile(&ztile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (tile_status_z[ty][tx] == TILE_STATUS_CLEAR)
            put_tile(tx, ty, &ctile, TAG_SURFACE_CLEAR, 1);
      }
   }

#if 0
   wait_on_mask(1 << TAG_SURFACE_CLEAR);
#endif
}


static void
cmd_clear_surface(const struct cell_command_clear_surface *clear)
{
   const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   uint i;

   if (Debug)
      printf("SPU %u: CLEAR SURF %u to 0x%08x\n", spu.init.id,
             clear->surface, clear->value);

#define CLEAR_OPT 1
#if CLEAR_OPT
   /* set all tile's status to CLEAR */
   if (clear->surface == 0) {
      memset(tile_status, TILE_STATUS_CLEAR, sizeof(tile_status));
      spu.fb.color_clear_value = clear->value;
   }
   else {
      memset(tile_status_z, TILE_STATUS_CLEAR, sizeof(tile_status_z));
      spu.fb.depth_clear_value = clear->value;
   }
   return;
#endif

   if (clear->surface == 0) {
      spu.fb.color_clear_value = clear->value;
      clear_c_tile(&ctile);
   }
   else {
      spu.fb.depth_clear_value = clear->value;
      clear_z_tile(&ztile);
   }

   /*
   printf("SPU: %s num=%d w=%d h=%d\n",
          __FUNCTION__, num_tiles, spu.fb.width_tiles, spu.fb.height_tiles);
   */

   for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
      uint tx = i % spu.fb.width_tiles;
      uint ty = i / spu.fb.width_tiles;
      if (clear->surface == 0)
         put_tile(tx, ty, &ctile, TAG_SURFACE_CLEAR, 0);
      else
         put_tile(tx, ty, &ztile, TAG_SURFACE_CLEAR, 1);
      /* XXX we don't want this here, but it fixes bad tile results */
   }

#if 0
   wait_on_mask(1 << TAG_SURFACE_CLEAR);
#endif

   if (Debug)
      printf("SPU %u: CLEAR SURF done\n", spu.init.id);
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
   printf("Render bounds: %g, %g  ...  %g, %g\n",
          render->xmin, render->ymin, render->xmax, render->ymax);
   printf("Render tiles:  %u, %u .. %u, %u\n", *txmin, *tymin, txmax, tymax);
#endif
}


static void
cmd_render(const struct cell_command_render *render)
{
   /* we'll DMA into these buffers */
   ubyte vertex_data[CELL_MAX_VBUF_SIZE] ALIGN16_ATTRIB;
   ushort indexes[CELL_MAX_VBUF_INDEXES] ALIGN16_ATTRIB;
   uint i, j, total_vertex_bytes, total_index_bytes;
   const uint vertex_size = render->vertex_size; /* in bytes */

   if (Debug) {
      printf("SPU %u: RENDER prim %u, indices: %u, nr_vert: %u\n",
             spu.init.id,
             render->prim_type,
             render->num_verts,
             render->num_indexes);
      /*
      printf("       bound: %g, %g .. %g, %g\n",
             render->xmin, render->ymin, render->xmax, render->ymax);
      */
   }

   ASSERT_ALIGN16(render->vertex_data);
   ASSERT_ALIGN16(render->index_data);

   /* how much vertex data */
   total_vertex_bytes = render->num_verts * vertex_size;
   total_index_bytes = render->num_indexes * sizeof(ushort);
   if (total_index_bytes < 16)
      total_index_bytes = 16;
   else
      total_index_bytes = ROUNDUP16(total_index_bytes);

   /*
   printf("VBUF: indices at %p,  vertices at %p  total_vertex_bytes %u  ind_bytes %u\n",
          render->index_data, render->vertex_data, total_vertex_bytes, total_index_bytes);
   */

   ASSERT(total_vertex_bytes % 16 == 0);
   /* get vertex data from main memory */
   mfc_get(vertex_data,  /* dest */
           (unsigned int) render->vertex_data,  /* src */
           total_vertex_bytes,  /* size */
           TAG_VERTEX_BUFFER,
           0, /* tid */
           0  /* rid */);

   ASSERT(total_index_bytes % 16 == 0);

   /* get index data from main memory */
   mfc_get(indexes,  /* dest */
           (unsigned int) render->index_data,  /* src */
           total_index_bytes,
           TAG_INDEX_BUFFER,
           0, /* tid */
           0  /* rid */);

   wait_on_mask_all((1 << TAG_VERTEX_BUFFER) |
                    (1 << TAG_INDEX_BUFFER));

   /* find tiles which intersect the prim bounding box */
   uint txmin, tymin, box_width_tiles, box_num_tiles;
#if 0
   tile_bounding_box(render, &txmin, &tymin,
                     &box_num_tiles, &box_width_tiles);
#else
   txmin = 0;
   tymin = 0;
   box_num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   box_width_tiles = spu.fb.width_tiles;
#endif

   /* make sure any pending clears have completed */
   wait_on_mask(1 << TAG_SURFACE_CLEAR);

   /* loop over tiles */
   for (i = spu.init.id; i < box_num_tiles; i += spu.init.num_spus) {
      const uint tx = txmin + i % box_width_tiles;
      const uint ty = tymin + i / box_width_tiles;

      ASSERT(tx < spu.fb.width_tiles);
      ASSERT(ty < spu.fb.height_tiles);

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

      /* loop over tris */
      for (j = 0; j < render->num_indexes; j += 3) {
         const float *v0, *v1, *v2;

         v0 = (const float *) (vertex_data + indexes[j+0] * vertex_size);
         v1 = (const float *) (vertex_data + indexes[j+1] * vertex_size);
         v2 = (const float *) (vertex_data + indexes[j+2] * vertex_size);

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


static void
cmd_framebuffer(const struct cell_command_framebuffer *cmd)
{
   if (Debug)
      printf("SPU %u: FRAMEBUFFER: %d x %d at %p, cformat 0x%x  zformat 0x%x\n",
             spu.init.id,
             cmd->width,
             cmd->height,
             cmd->color_start,
             cmd->color_format,
             cmd->depth_format);

   spu.fb.color_start = cmd->color_start;
   spu.fb.depth_start = cmd->depth_start;
   spu.fb.color_format = cmd->color_format;
   spu.fb.depth_format = cmd->depth_format;
   spu.fb.width = cmd->width;
   spu.fb.height = cmd->height;
   spu.fb.width_tiles = (spu.fb.width + TILE_SIZE - 1) / TILE_SIZE;
   spu.fb.height_tiles = (spu.fb.height + TILE_SIZE - 1) / TILE_SIZE;

   if (spu.fb.depth_format == PIPE_FORMAT_Z32_UNORM)
      spu.fb.zsize = 4;
   else if (spu.fb.depth_format == PIPE_FORMAT_Z16_UNORM)
      spu.fb.zsize = 2;
   else
      spu.fb.zsize = 0;
}


static void
cmd_state_depth_stencil(const struct pipe_depth_stencil_alpha_state *state)
{
   if (Debug)
      printf("SPU %u: DEPTH_STENCIL: ztest %d\n",
             spu.init.id,
             state->depth.enabled);

   memcpy(&spu.depth_stencil, state, sizeof(*state));
}


static void
cmd_state_sampler(const struct pipe_sampler_state *state)
{
   if (Debug)
      printf("SPU %u: SAMPLER\n",
             spu.init.id);

   memcpy(&spu.sampler[0], state, sizeof(*state));
}


static void
cmd_state_vertex_info(const struct vertex_info *vinfo)
{
   if (Debug)
      printf("SPU %u: VERTEX_INFO num_attribs=%u\n", spu.init.id,
             vinfo->num_attribs);
   memcpy(&spu.vertex_info, vinfo, sizeof(*vinfo));
}



static void
cmd_finish(void)
{
   if (Debug)
      printf("SPU %u: FINISH\n", spu.init.id);
   really_clear_tiles(0);
   /* wait for all outstanding DMAs to finish */
   mfc_write_tag_mask(~0);
   mfc_read_tag_status_all();
   /* send mbox message to PPU */
   spu_write_out_mbox(CELL_CMD_FINISH);
}


/**
 * Tell the PPU that this SPU has finished copying a batch buffer to
 * local store and that it may be reused by the PPU.
 * This is done by writting a 16-byte batch-buffer-status block back into
 * main memory (in cell_contex->buffer_status[]).
 */
static void
release_batch_buffer(uint buffer)
{
   /* Evidently, using less than a 16-byte status doesn't work reliably */
   static const uint status[4] ALIGN16_ATTRIB
      = {CELL_BUFFER_STATUS_FREE, 0, 0, 0};

   const uint index = 4 * (spu.init.id * CELL_NUM_BATCH_BUFFERS + buffer);
   uint *dst = spu.init.buffer_status + index;

   ASSERT(buffer < CELL_NUM_BATCH_BUFFERS);

   /*
   printf("SPU %u: Set batch status buf=%u, index %u, at %p to FREE\n",
          spu.init.id, buffer, index, dst);
   */

   mfc_put((void *) &status,    /* src in local memory */
           (unsigned int) dst,  /* dst in main memory */
           sizeof(status),      /* size */
           TAG_MISC,            /* tag is unimportant */
           0, /* tid */
           0  /* rid */);
}


/**
 * Execute a batch of commands
 * The opcode param encodes the location of the buffer and its size.
 */
static void
cmd_batch(uint opcode)
{
   const uint buf = (opcode >> 8) & 0xff;
   uint size = (opcode >> 16);
   uint buffer[CELL_BATCH_BUFFER_SIZE / 4] ALIGN16_ATTRIB;
   const uint usize = size / sizeof(uint);
   uint pos;

   if (Debug)
      printf("SPU %u: BATCH buffer %u, len %u, from %p\n",
             spu.init.id, buf, size, spu.init.batch_buffers[buf]);

   ASSERT((opcode & CELL_CMD_OPCODE_MASK) == CELL_CMD_BATCH);

   ASSERT_ALIGN16(spu.init.batch_buffers[buf]);

   size = ROUNDUP16(size);

   ASSERT_ALIGN16(spu.init.batch_buffers[buf]);

   mfc_get(buffer,  /* dest */
           (unsigned int) spu.init.batch_buffers[buf],  /* src */
           size,
           TAG_BATCH_BUFFER,
           0, /* tid */
           0  /* rid */);
   wait_on_mask(1 << TAG_BATCH_BUFFER);

   /* Tell PPU we're done copying the buffer to local store */
   release_batch_buffer(buf);

   for (pos = 0; pos < usize; /* no incr */) {
      switch (buffer[pos]) {
      case CELL_CMD_FRAMEBUFFER:
         {
            struct cell_command_framebuffer *fb
               = (struct cell_command_framebuffer *) &buffer[pos];
            cmd_framebuffer(fb);
            pos += sizeof(*fb) / 4;
         }
         break;
      case CELL_CMD_CLEAR_SURFACE:
         {
            struct cell_command_clear_surface *clr
               = (struct cell_command_clear_surface *) &buffer[pos];
            cmd_clear_surface(clr);
            pos += sizeof(*clr) / 4;
         }
         break;
      case CELL_CMD_RENDER:
         {
            struct cell_command_render *render
               = (struct cell_command_render *) &buffer[pos];
            cmd_render(render);
            pos += sizeof(*render) / 4;
         }
         break;
      case CELL_CMD_FINISH:
         cmd_finish();
         pos += 1;
         break;
      case CELL_CMD_STATE_DEPTH_STENCIL:
         cmd_state_depth_stencil((struct pipe_depth_stencil_alpha_state *)
                                 &buffer[pos+1]);
         pos += (1 + sizeof(struct pipe_depth_stencil_alpha_state) / 4);
         break;
      case CELL_CMD_STATE_SAMPLER:
         cmd_state_sampler((struct pipe_sampler_state *) &buffer[pos+1]);
         pos += (1 + sizeof(struct pipe_sampler_state) / 4);
         break;
      case CELL_CMD_STATE_VERTEX_INFO:
         cmd_state_vertex_info((struct vertex_info *) &buffer[pos+1]);
         pos += (1 + sizeof(struct vertex_info) / 4);
         break;
      default:
         printf("SPU %u: bad opcode: 0x%x\n", spu.init.id, buffer[pos]);
         ASSERT(0);
         break;
      }
   }

   if (Debug)
      printf("SPU %u: BATCH complete\n", spu.init.id);
}


/**
 * Temporary/simple main loop for SPEs: Get a command, execute it, repeat.
 */
static void
main_loop(void)
{
   struct cell_command cmd;
   int exitFlag = 0;

   if (Debug)
      printf("SPU %u: Enter main loop\n", spu.init.id);

   ASSERT((sizeof(struct cell_command) & 0xf) == 0);
   ASSERT_ALIGN16(&cmd);

   while (!exitFlag) {
      unsigned opcode;
      int tag = 0;

      if (Debug)
         printf("SPU %u: Wait for cmd...\n", spu.init.id);

      /* read/wait from mailbox */
      opcode = (unsigned int) spu_read_in_mbox();

      if (Debug)
         printf("SPU %u: got cmd 0x%x\n", spu.init.id, opcode);

      /* command payload */
      mfc_get(&cmd,  /* dest */
              (unsigned int) spu.init.cmd, /* src */
              sizeof(struct cell_command), /* bytes */
              tag,
              0, /* tid */
              0  /* rid */);
      wait_on_mask( 1 << tag );

      switch (opcode & CELL_CMD_OPCODE_MASK) {
      case CELL_CMD_EXIT:
         if (Debug)
            printf("SPU %u: EXIT\n", spu.init.id);
         exitFlag = 1;
         break;
      case CELL_CMD_FRAMEBUFFER:
         cmd_framebuffer(&cmd.fb);
         break;
      case CELL_CMD_CLEAR_SURFACE:
         cmd_clear_surface(&cmd.clear);
         break;
      case CELL_CMD_RENDER:
         cmd_render(&cmd.render);
         break;
      case CELL_CMD_BATCH:
         cmd_batch(opcode);
         break;
      case CELL_CMD_FINISH:
         cmd_finish();
         break;
      default:
         printf("Bad opcode!\n");
      }

   }

   if (Debug)
      printf("SPU %u: Exit main loop\n", spu.init.id);
}



static void
one_time_init(void)
{
   memset(tile_status, TILE_STATUS_DEFINED, sizeof(tile_status));
   memset(tile_status_z, TILE_STATUS_DEFINED, sizeof(tile_status_z));
}


/* In some versions of the SDK the SPE main takes 'unsigned long' as a
 * parameter.  In others it takes 'unsigned long long'.  Use a define to
 * select between the two.
 */
#ifdef SPU_MAIN_PARAM_LONG_LONG
typedef unsigned long long main_param_t
#else
typedef unsigned long main_param_t
#endif

/**
 * SPE entrypoint.
 */
int
main(main_param_t speid, main_param_t argp)
{
   int tag = 0;

   (void) speid;

   one_time_init();

   if (Debug)
      printf("SPU: main() speid=%lu\n", speid);

   mfc_get(&spu.init,  /* dest */
           (unsigned int) argp, /* src */
           sizeof(struct cell_init_info), /* bytes */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask( 1 << tag );


   main_loop();

   return 0;
}
