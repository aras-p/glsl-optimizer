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

#include "lp_rast.h"


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



extern void lp_bin_new_data_block( struct data_block_list *list );

extern void lp_bin_new_cmd_block( struct cmd_block_list *list );


/**
 * Allocate space for a command/data in the given block list.
 * Grow the block list if needed.
 */
static INLINE void *
lp_bin_alloc( struct data_block_list *list, unsigned size)
{
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
lp_bin_alloc_aligned( struct data_block_list *list, unsigned size,
                      unsigned alignment )
{
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
lp_bin_putback_data( struct data_block_list *list, unsigned size)
{
   assert(list->tail->used >= size);
   list->tail->used -= size;
}


/* Add a command to a given bin.
 */
static INLINE void
lp_bin_command( struct cmd_bin *bin,
                lp_rast_cmd cmd,
                union lp_rast_cmd_arg arg )
{
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
