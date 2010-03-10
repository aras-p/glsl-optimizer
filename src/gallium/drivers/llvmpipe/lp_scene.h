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
 * Note: the "setup" code is concerned with building scenes while
 * The "rast" code is concerned with consuming/executing scenes.
 */

#ifndef LP_SCENE_H
#define LP_SCENE_H

#include "os/os_thread.h"
#include "lp_tile_soa.h"
#include "lp_rast.h"

struct lp_scene_queue;

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
typedef void (*lp_rast_cmd)( struct lp_rasterizer_task *,
                             const union lp_rast_cmd_arg );

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
 * This stores bulk data which is shared by all bins within a scene.
 * Examples include triangle data and state data.  The commands in
 * the per-tile bins will point to chunks of data in this structure.
 */
struct data_block_list {
   struct data_block *head;
   struct data_block *tail;
};


/** List of texture references */
struct texture_ref {
   struct pipe_texture *texture;
   struct texture_ref *prev, *next;  /**< linked list w/ u_simple_list.h */
};


/**
 * All bins and bin data are contained here.
 * Per-bin data goes into the 'tile' bins.
 * Shared data goes into the 'data' buffer.
 *
 * When there are multiple threads, will want to double-buffer between
 * scenes:
 */
struct lp_scene {
   struct pipe_context *pipe;
   struct pipe_transfer *cbuf_transfer[PIPE_MAX_COLOR_BUFS];
   struct pipe_transfer *zsbuf_transfer;

   /* Scene's buffers are mapped at the time the scene is enqueued:
    */
   void *cbuf_map[PIPE_MAX_COLOR_BUFS];
   uint8_t *zsbuf_map;

   /** the framebuffer to render the scene into */
   struct pipe_framebuffer_state fb;

   /** list of textures referenced by the scene commands */
   struct texture_ref textures;

   boolean write_depth;

   /**
    * Number of active tiles in each dimension.
    * This basically the framebuffer size divided by tile size
    */
   unsigned tiles_x, tiles_y;

   int curr_x, curr_y;  /**< for iterating over bins */
   pipe_mutex mutex;

   /* Where to place this scene once it has been rasterized:
    */
   struct lp_scene_queue *empty_queue;

   struct cmd_bin tile[TILES_X][TILES_Y];
   struct data_block_list data;
};



struct lp_scene *lp_scene_create(struct pipe_context *pipe,
                                 struct lp_scene_queue *empty_queue);

void lp_scene_destroy(struct lp_scene *scene);



boolean lp_scene_is_empty(struct lp_scene *scene );

void lp_scene_reset(struct lp_scene *scene );


void lp_bin_new_data_block( struct data_block_list *list );

void lp_bin_new_cmd_block( struct cmd_block_list *list );

unsigned lp_scene_data_size( const struct lp_scene *scene );

unsigned lp_scene_bin_size( const struct lp_scene *scene, unsigned x, unsigned y );

void lp_scene_texture_reference( struct lp_scene *scene,
                                 struct pipe_texture *texture );

boolean lp_scene_is_texture_referenced( const struct lp_scene *scene,
                                        const struct pipe_texture *texture );


/**
 * Allocate space for a command/data in the bin's data buffer.
 * Grow the block list if needed.
 */
static INLINE void *
lp_scene_alloc( struct lp_scene *scene, unsigned size)
{
   struct data_block_list *list = &scene->data;

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
lp_scene_alloc_aligned( struct lp_scene *scene, unsigned size,
			unsigned alignment )
{
   struct data_block_list *list = &scene->data;

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
lp_scene_putback_data( struct lp_scene *scene, unsigned size)
{
   struct data_block_list *list = &scene->data;
   assert(list->tail->used >= size);
   list->tail->used -= size;
}


/** Return pointer to a particular tile's bin. */
static INLINE struct cmd_bin *
lp_scene_get_bin(struct lp_scene *scene, unsigned x, unsigned y)
{
   return &scene->tile[x][y];
}


/** Remove all commands from a bin */
void
lp_scene_bin_reset(struct lp_scene *scene, unsigned x, unsigned y);


/* Add a command to bin[x][y].
 */
static INLINE void
lp_scene_bin_command( struct lp_scene *scene,
                unsigned x, unsigned y,
                lp_rast_cmd cmd,
                union lp_rast_cmd_arg arg )
{
   struct cmd_bin *bin = lp_scene_get_bin(scene, x, y);
   struct cmd_block_list *list = &bin->commands;

   assert(x < scene->tiles_x);
   assert(y < scene->tiles_y);

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


/* Add a command to all active bins.
 */
static INLINE void
lp_scene_bin_everywhere( struct lp_scene *scene,
			 lp_rast_cmd cmd,
			 const union lp_rast_cmd_arg arg )
{
   unsigned i, j;
   for (i = 0; i < scene->tiles_x; i++)
      for (j = 0; j < scene->tiles_y; j++)
         lp_scene_bin_command( scene, i, j, cmd, arg );
}


void
lp_scene_bin_state_command( struct lp_scene *scene,
			    lp_rast_cmd cmd,
			    const union lp_rast_cmd_arg arg );


static INLINE unsigned
lp_scene_get_num_bins( const struct lp_scene *scene )
{
   return scene->tiles_x * scene->tiles_y;
}


void
lp_scene_bin_iter_begin( struct lp_scene *scene );

struct cmd_bin *
lp_scene_bin_iter_next( struct lp_scene *scene, int *bin_x, int *bin_y );

void
lp_scene_rasterize( struct lp_scene *scene,
                    struct lp_rasterizer *rast,
                    boolean write_depth );

void
lp_scene_begin_binning( struct lp_scene *scene,
                        struct pipe_framebuffer_state *fb );

#endif /* LP_BIN_H */
