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

#include "util/u_memory.h"
#include "lp_bin.h"


void
lp_init_bins(struct lp_bins *bins)
{
   unsigned i, j;
   for (i = 0; i < TILES_X; i++)
      for (j = 0; j < TILES_Y; j++) {
         struct cmd_bin *bin = lp_get_bin(bins, i, j);
         bin->commands.head = bin->commands.tail = CALLOC_STRUCT(cmd_block);
      }

   bins->data.head =
      bins->data.tail = CALLOC_STRUCT(data_block);

   pipe_mutex_init(bins->mutex);
}


/**
 * Set bins to empty state.
 */
void
lp_reset_bins(struct lp_bins *bins )
{
   unsigned i, j;

   /* Free all but last binner command lists:
    */
   for (i = 0; i < bins->tiles_x; i++) {
      for (j = 0; j < bins->tiles_y; j++) {
         struct cmd_bin *bin = lp_get_bin(bins, i, j);
         struct cmd_block_list *list = &bin->commands;
         struct cmd_block *block;
         struct cmd_block *tmp;
         
         for (block = list->head; block != list->tail; block = tmp) {
            tmp = block->next;
            FREE(block);
         }
         
         assert(list->tail->next == NULL);
         list->head = list->tail;
         list->head->count = 0;
      }
   }

   /* Free all but last binned data block:
    */
   {
      struct data_block_list *list = &bins->data;
      struct data_block *block, *tmp;

      for (block = list->head; block != list->tail; block = tmp) {
         tmp = block->next;
         FREE(block);
      }
         
      assert(list->tail->next == NULL);
      list->head = list->tail;
      list->head->used = 0;
   }
}


/**
 * Free all data associated with the given bin, but don't free(bins).
 */
void
lp_free_bin_data(struct lp_bins *bins)
{
   unsigned i, j;

   for (i = 0; i < TILES_X; i++)
      for (j = 0; j < TILES_Y; j++) {
         struct cmd_bin *bin = lp_get_bin(bins, i, j);
         /* lp_reset_bins() should have been already called */
         assert(bin->commands.head == bin->commands.tail);
         FREE(bin->commands.head);
         bin->commands.head = NULL;
         bin->commands.tail = NULL;
      }

   FREE(bins->data.head);
   bins->data.head = NULL;

   pipe_mutex_destroy(bins->mutex);
}


void
lp_bin_set_num_bins( struct lp_bins *bins,
                     unsigned tiles_x, unsigned tiles_y )
{
   bins->tiles_x = tiles_x;
   bins->tiles_y = tiles_y;
}

void
lp_bin_new_cmd_block( struct cmd_block_list *list )
{
   struct cmd_block *block = MALLOC_STRUCT(cmd_block);
   list->tail->next = block;
   list->tail = block;
   block->next = NULL;
   block->count = 0;
}


void
lp_bin_new_data_block( struct data_block_list *list )
{
   struct data_block *block = MALLOC_STRUCT(data_block);
   list->tail->next = block;
   list->tail = block;
   block->next = NULL;
   block->used = 0;
}


/**
 * Return last command in the bin
 */
static lp_rast_cmd
lp_get_last_command( const struct cmd_bin *bin )
{
   const struct cmd_block *tail = bin->commands.tail;
   const unsigned i = tail->count;
   if (i > 0)
      return tail->cmd[i - 1];
   else
      return NULL;
}


/**
 * Replace the arg of the last command in the bin.
 */
static void
lp_replace_last_command_arg( struct cmd_bin *bin,
                             const union lp_rast_cmd_arg arg )
{
   struct cmd_block *tail = bin->commands.tail;
   const unsigned i = tail->count;
   assert(i > 0);
   tail->arg[i - 1] = arg;
}



/**
 * Put a state-change command into all bins.
 * If we find that the last command in a bin was also a state-change
 * command, we can simply replace that one with the new one.
 */
void
lp_bin_state_command( struct lp_bins *bins,
                      lp_rast_cmd cmd,
                      const union lp_rast_cmd_arg arg )
{
   unsigned i, j;
   for (i = 0; i < bins->tiles_x; i++) {
      for (j = 0; j < bins->tiles_y; j++) {
         struct cmd_bin *bin = lp_get_bin(bins, i, j);
         lp_rast_cmd last_cmd = lp_get_last_command(bin);
         if (last_cmd == cmd) {
            lp_replace_last_command_arg(bin, arg);
         }
         else {
            lp_bin_command( bins, i, j, cmd, arg );
         }
      }
   }
}


/** advance curr_x,y to the next bin */
static boolean
next_bin(struct lp_bins *bins)
{
   bins->curr_x++;
   if (bins->curr_x >= bins->tiles_x) {
      bins->curr_x = 0;
      bins->curr_y++;
   }
   if (bins->curr_y >= bins->tiles_y) {
      /* no more bins */
      return FALSE;
   }
   return TRUE;
}


void
lp_bin_iter_begin( struct lp_bins *bins )
{
   bins->curr_x = bins->curr_y = -1;
}


/**
 * Return point to next bin to be rendered.
 * The lp_bins::curr_x and ::curr_y fields will be advanced.
 * Multiple rendering threads will call this function to get a chunk
 * of work (a bin) to work on.
 */
struct cmd_bin *
lp_bin_iter_next( struct lp_bins *bins, int *bin_x, int *bin_y )
{
   struct cmd_bin *bin = NULL;

   pipe_mutex_lock(bins->mutex);

   if (bins->curr_x < 0) {
      /* first bin */
      bins->curr_x = 0;
      bins->curr_y = 0;
   }
   else if (!next_bin(bins)) {
      /* no more bins left */
      goto end;
   }

   bin = lp_get_bin(bins, bins->curr_x, bins->curr_y);
   *bin_x = bins->curr_x;
   *bin_y = bins->curr_y;

end:
   /*printf("return bin %p at %d, %d\n", (void *) bin, *bin_x, *bin_y);*/
   pipe_mutex_unlock(bins->mutex);
   return bin;
}
