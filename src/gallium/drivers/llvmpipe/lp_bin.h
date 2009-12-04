/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/**
 * Binner data structures and bin-related functions.
 * Note: the "setup" code is concerned with building bins while
 * The "rast" code is concerned with consuming/executing bins.
 */

#ifndef LP_BIN_H
#define LP_BIN_H

#include "lp_tile_soa.h"
#include "lp_rast.h"


/* We're limited to 2K by 2K for 32bit fixed point rasterization.
 * Will need a 64-bit version for larger framebuffers.
 */
#define MAXHEIGHT 2048
#define MAXWIDTH 2048
#define TILES_X (MAXWIDTH / TILE_SIZE)
#define TILES_Y (MAXHEIGHT / TILE_SIZE)


#define CMD_BLOCK_MAX 128
#define DATA_BLOCK_SIZE (16 * 1024 - sizeof(unsigned) - sizeof(void *))
   


/* switch to a non-pointer value for this:
 */
typedef void (*lp_rast_cmd)( struct lp_rasterizer *, const union lp_rast_cmd_arg );

struct cmd_block {
   lp_rast_cmd cmd[CMD_BLOCK_MAX];
   union lp_rast_cmd_arg arg[CMD_BLOCK_MAX];
   unsigned count;
   struct cmd_block *next;
};

struct data_block {
   ubyte data[DATA_BLOCK_SIZE];
   unsigned used;
   struct data_block *next;
};

struct cmd_block_list {
   struct cmd_block *head;
   struct cmd_block *tail;
};

/**
 * For each screen tile we have one of these bins.
 */
struct cmd_bin {
   struct cmd_block_list commands;
};
   

/**
 * This stores bulk data which is shared by all bins.
 * Examples include triangle data and state data.  The commands in
 * the per-tile bins will point to chunks of data in this structure.
 */
struct data_block_list {
   struct data_block *head;
   struct data_block *tail;
};


/**
 * All bins and bin data are contained here.
 * Per-bin data goes into the 'tile' bins.
 * Shared bin data goes into the 'data' buffer.
 * When there are multiple threads, will want to double-buffer the
 * bin arrays:
 */
struct lp_bins {
   struct cmd_bin tile[TILES_X][TILES_Y];
   struct data_block_list data;
};



void lp_init_bins(struct lp_bins *bins);

void lp_reset_bins(struct lp_bins *bins, unsigned tiles_x, unsigned tiles_y);

void lp_free_bin_data(struct lp_bins *bins);

void lp_bin_new_data_block( struct data_block_list *list );

void lp_bin_new_cmd_block( struct cmd_block_list *list );


/**
 * Allocate space for a command/data in the bin's data buffer.
 * Grow the block list if needed.
 */
static INLINE void *
lp_bin_alloc( struct lp_bins *bins, unsigned size)
{
   struct data_block_list *list = &bins->data;

   if (list->tail->used + size > DATA_BLOCK_SIZE) {
      lp_bin_new_data_block( list );
   }

   {
      struct data_block *tail = list->tail;
      ubyte *data = tail->data + tail->used;
      tail->used += size;
      return data;
   }
}


/**
 * As above, but with specific alignment.
 */
static INLINE void *
lp_bin_alloc_aligned( struct lp_bins *bins, unsigned size,
                      unsigned alignment )
{
   struct data_block_list *list = &bins->data;

   if (list->tail->used + size + alignment - 1 > DATA_BLOCK_SIZE) {
      lp_bin_new_data_block( list );
   }

   {
      struct data_block *tail = list->tail;
      ubyte *data = tail->data + tail->used;
      unsigned offset = (((uintptr_t)data + alignment - 1) & ~(alignment - 1)) - (uintptr_t)data;
      tail->used += offset + size;
      return data + offset;
   }
}


/* Put back data if we decide not to use it, eg. culled triangles.
 */
static INLINE void
lp_bin_putback_data( struct lp_bins *bins, unsigned size)
{
   struct data_block_list *list = &bins->data;
   assert(list->tail->used >= size);
   list->tail->used -= size;
}


/** Return pointer to a particular tile's bin. */
static INLINE struct cmd_bin *
lp_get_bin(struct lp_bins *bins, unsigned x, unsigned y)
{
   return &bins->tile[x][y];
}



/* Add a command to bin[x][y].
 */
static INLINE void
lp_bin_command( struct lp_bins *bins,
                unsigned x, unsigned y,
                lp_rast_cmd cmd,
                union lp_rast_cmd_arg arg )
{
   struct cmd_bin *bin = lp_get_bin(bins, x, y);
   struct cmd_block_list *list = &bin->commands;

   if (list->tail->count == CMD_BLOCK_MAX) {
      lp_bin_new_cmd_block( list );
   }

   {
      struct cmd_block *tail = list->tail;
      unsigned i = tail->count;
      tail->cmd[i] = cmd;
      tail->arg[i] = arg;
      tail->count++;
   }
}


#endif /* LP_BIN_H */
