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
}


void
lp_reset_bins(struct lp_bins *bins, unsigned tiles_x, unsigned tiles_y)
{
   unsigned i, j;

   /* Free all but last binner command lists:
    */
   for (i = 0; i < tiles_x; i++) {
      for (j = 0; j < tiles_y; j++) {
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
