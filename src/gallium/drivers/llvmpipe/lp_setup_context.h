/**************************************************************************
 *
 * Copyright 2007-2009 VMware, Inc.
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

#ifndef LP_SETUP_CONTEXT_H
#define LP_SETUP_CONTEXT_H

#include "lp_setup.h"
#include "lp_rast.h"

#define CMD_BLOCK_MAX 128
#define DATA_BLOCK_SIZE (16 * 1024 - sizeof(unsigned) - sizeof(void *))

/* switch to a non-pointer value for this:
 */
typedef void (*lp_rast_cmd)( struct lp_rasterizer *, const union lp_rast_cmd_arg * );

struct cmd_block {
   lp_rast_cmd cmd[CMD_BLOCK_MAX];
   const union lp_rast_cmd_arg *arg[CMD_BLOCK_MAX];
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

struct data_block_list {
   struct data_block *head;
   struct data_block *tail;
};
   

/* We're limited to 2K by 2K for 32bit fixed point rasterization.
 * Will need a 64-bit version for larger framebuffers.
 */
#define MAXHEIGHT 2048
#define MAXWIDTH 2048

struct setup_context {

   struct lp_rasterizer *rast;

   /* When there are multiple threads, will want to double-buffer the
    * bin arrays:
    */
   struct cmd_block_list tile[MAXHEIGHT / TILESIZE][MAXWIDTH / TILESIZE];
   struct data_block_list data;

   unsigned tiles_x;
   unsigned tiles_y;

   struct {
      struct pipe_surface *color;
      struct pipe_surface *zstencil;
   } fb;

   struct {
      unsigned flags;
      union lp_rast_cmd_arg color;
      union lp_rast_cmd_arg zstencil;
   } clear;

   enum {
      SETUP_FLUSHED,
      SETUP_CLEARED,
      SETUP_ACTIVE
   } state;
   
   struct {
      enum lp_interp inputs[PIPE_MAX_ATTRIBS];
      unsigned nr_inputs;
   } fs;

   void (*point)( struct setup_context *,
                  const float (*v0)[4]);

   void (*line)( struct setup_context *,
                 const float (*v0)[4],
                 const float (*v1)[4]);

   void (*triangle)( struct setup_context *,
                     const float (*v0)[4],
                     const float (*v1)[4],
                     const float (*v2)[4]);
};

void lp_setup_new_data_block( struct data_block_list *list );
void lp_setup_new_cmd_block( struct cmd_block_list *list );

static INLINE void *get_data( struct data_block_list *list,
                              unsigned size)
{

   if (list->tail->used + size > DATA_BLOCK_SIZE) {
      lp_setup_new_data_block( list );
   }

   {
      struct data_block *tail = list->tail;
      ubyte *data = tail->data + tail->used;
      tail->used += size;
      return data;
   }
}

/* Add a command to a given bin.
 */
static INLINE void bin_cmd( struct cmd_block_list *list,
                            lp_rast_cmd cmd,
                            const union lp_rast_cmd_arg *arg )
{
   if (list->tail->count == CMD_BLOCK_MAX) {
      lp_setup_new_cmd_block( list );
   }

   {
      struct cmd_block *tail = list->tail;
      unsigned i = tail->count;
      tail->cmd[i] = cmd;
      tail->arg[i] = arg;
      tail->count++;
   }
}



#endif
