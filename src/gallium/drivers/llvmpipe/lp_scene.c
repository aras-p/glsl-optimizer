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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_simple_list.h"
#include "util/u_surface.h"
#include "lp_scene.h"
#include "lp_scene_queue.h"
#include "lp_debug.h"


struct lp_scene *
lp_scene_create( struct pipe_context *pipe,
                 struct lp_scene_queue *queue )
{
   unsigned i, j;
   struct lp_scene *scene = CALLOC_STRUCT(lp_scene);
   if (!scene)
      return NULL;

   scene->pipe = pipe;
   scene->empty_queue = queue;

   for (i = 0; i < TILES_X; i++) {
      for (j = 0; j < TILES_Y; j++) {
         struct cmd_bin *bin = lp_scene_get_bin(scene, i, j);
         bin->commands.head = bin->commands.tail = CALLOC_STRUCT(cmd_block);
      }
   }

   scene->data.head =
      scene->data.tail = CALLOC_STRUCT(data_block);

   make_empty_list(&scene->textures);

   pipe_mutex_init(scene->mutex);

   return scene;
}


/**
 * Free all data associated with the given scene, and free(scene).
 */
void
lp_scene_destroy(struct lp_scene *scene)
{
   unsigned i, j;

   lp_scene_reset(scene);

   for (i = 0; i < TILES_X; i++)
      for (j = 0; j < TILES_Y; j++) {
         struct cmd_bin *bin = lp_scene_get_bin(scene, i, j);
         assert(bin->commands.head == bin->commands.tail);
         FREE(bin->commands.head);
         bin->commands.head = NULL;
         bin->commands.tail = NULL;
      }

   FREE(scene->data.head);
   scene->data.head = NULL;

   pipe_mutex_destroy(scene->mutex);

   FREE(scene);
}


/**
 * Check if the scene's bins are all empty.
 * For debugging purposes.
 */
boolean
lp_scene_is_empty(struct lp_scene *scene )
{
   unsigned x, y;

   for (y = 0; y < TILES_Y; y++) {
      for (x = 0; x < TILES_X; x++) {
         const struct cmd_bin *bin = lp_scene_get_bin(scene, x, y);
         const struct cmd_block_list *list = &bin->commands;
         if (list->head != list->tail || list->head->count > 0) {
            return FALSE;
         }
      }
   }
   return TRUE;
}


/* Free data for one particular bin.  May be called from the
 * rasterizer thread(s).
 */
void
lp_scene_bin_reset(struct lp_scene *scene, unsigned x, unsigned y)
{
   struct cmd_bin *bin = lp_scene_get_bin(scene, x, y);
   struct cmd_block_list *list = &bin->commands;
   struct cmd_block *block;
   struct cmd_block *tmp;

   assert(x < TILES_X);
   assert(y < TILES_Y);

   for (block = list->head; block != list->tail; block = tmp) {
      tmp = block->next;
      FREE(block);
   }

   assert(list->tail->next == NULL);
   list->head = list->tail;
   list->head->count = 0;
}


/**
 * Free all the temporary data in a scene.  May be called from the
 * rasterizer thread(s).
 */
void
lp_scene_reset(struct lp_scene *scene )
{
   unsigned i, j;

   /* Free all but last binner command lists:
    */
   for (i = 0; i < scene->tiles_x; i++) {
      for (j = 0; j < scene->tiles_y; j++) {
         lp_scene_bin_reset(scene, i, j);
      }
   }

   assert(lp_scene_is_empty(scene));

   /* Free all but last binned data block:
    */
   {
      struct data_block_list *list = &scene->data;
      struct data_block *block, *tmp;

      for (block = list->head; block != list->tail; block = tmp) {
         tmp = block->next;
         FREE(block);
      }
         
      assert(list->tail->next == NULL);
      list->head = list->tail;
      list->head->used = 0;
   }

   /* Release texture refs
    */
   {
      struct texture_ref *ref, *next, *ref_list = &scene->textures;
      for (ref = ref_list->next; ref != ref_list; ref = next) {
         next = next_elem(ref);
         pipe_texture_reference(&ref->texture, NULL);
         FREE(ref);
      }
      make_empty_list(ref_list);
   }
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


/** Return number of bytes used for all bin data within a scene */
unsigned
lp_scene_data_size( const struct lp_scene *scene )
{
   unsigned size = 0;
   const struct data_block *block;
   for (block = scene->data.head; block; block = block->next) {
      size += block->used;
   }
   return size;
}


/** Return number of bytes used for a single bin */
unsigned
lp_scene_bin_size( const struct lp_scene *scene, unsigned x, unsigned y )
{
   struct cmd_bin *bin = lp_scene_get_bin((struct lp_scene *) scene, x, y);
   const struct cmd_block *cmd;
   unsigned size = 0;
   for (cmd = bin->commands.head; cmd; cmd = cmd->next) {
      size += (cmd->count *
               (sizeof(lp_rast_cmd) + sizeof(union lp_rast_cmd_arg)));
   }
   return size;
}


/**
 * Add a reference to a texture by the scene.
 */
void
lp_scene_texture_reference( struct lp_scene *scene,
                            struct pipe_texture *texture )
{
   struct texture_ref *ref = CALLOC_STRUCT(texture_ref);
   if (ref) {
      struct texture_ref *ref_list = &scene->textures;
      pipe_texture_reference(&ref->texture, texture);
      insert_at_tail(ref_list, ref);
   }
}


/**
 * Does this scene have a reference to the given texture?
 */
boolean
lp_scene_is_texture_referenced( const struct lp_scene *scene,
                                const struct pipe_texture *texture )
{
   const struct texture_ref *ref_list = &scene->textures;
   const struct texture_ref *ref;
   foreach (ref, ref_list) {
      if (ref->texture == texture)
         return TRUE;
   }
   return FALSE;
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
lp_scene_bin_state_command( struct lp_scene *scene,
                            lp_rast_cmd cmd,
                            const union lp_rast_cmd_arg arg )
{
   unsigned i, j;
   for (i = 0; i < scene->tiles_x; i++) {
      for (j = 0; j < scene->tiles_y; j++) {
         struct cmd_bin *bin = lp_scene_get_bin(scene, i, j);
         lp_rast_cmd last_cmd = lp_get_last_command(bin);
         if (last_cmd == cmd) {
            lp_replace_last_command_arg(bin, arg);
         }
         else {
            lp_scene_bin_command( scene, i, j, cmd, arg );
         }
      }
   }
}


/** advance curr_x,y to the next bin */
static boolean
next_bin(struct lp_scene *scene)
{
   scene->curr_x++;
   if (scene->curr_x >= scene->tiles_x) {
      scene->curr_x = 0;
      scene->curr_y++;
   }
   if (scene->curr_y >= scene->tiles_y) {
      /* no more bins */
      return FALSE;
   }
   return TRUE;
}


void
lp_scene_bin_iter_begin( struct lp_scene *scene )
{
   scene->curr_x = scene->curr_y = -1;
}


/**
 * Return pointer to next bin to be rendered.
 * The lp_scene::curr_x and ::curr_y fields will be advanced.
 * Multiple rendering threads will call this function to get a chunk
 * of work (a bin) to work on.
 */
struct cmd_bin *
lp_scene_bin_iter_next( struct lp_scene *scene, int *bin_x, int *bin_y )
{
   struct cmd_bin *bin = NULL;

   pipe_mutex_lock(scene->mutex);

   if (scene->curr_x < 0) {
      /* first bin */
      scene->curr_x = 0;
      scene->curr_y = 0;
   }
   else if (!next_bin(scene)) {
      /* no more bins left */
      goto end;
   }

   bin = lp_scene_get_bin(scene, scene->curr_x, scene->curr_y);
   *bin_x = scene->curr_x;
   *bin_y = scene->curr_y;

end:
   /*printf("return bin %p at %d, %d\n", (void *) bin, *bin_x, *bin_y);*/
   pipe_mutex_unlock(scene->mutex);
   return bin;
}


/**
 * Prepare this scene for the rasterizer.
 * Map the framebuffer surfaces.  Initialize the 'rast' state.
 */
static boolean
lp_scene_map_buffers( struct lp_scene *scene )
{
   struct pipe_screen *screen = scene->pipe->screen;
   struct pipe_surface *cbuf, *zsbuf;
   int i;

   LP_DBG(DEBUG_RAST, "%s\n", __FUNCTION__);


   /* Map all color buffers 
    */
   for (i = 0; i < scene->fb.nr_cbufs; i++) {
      cbuf = scene->fb.cbufs[i];
      if (cbuf) {
	 scene->cbuf_transfer[i] = screen->get_tex_transfer(screen,
                                                          cbuf->texture,
                                                          cbuf->face,
                                                          cbuf->level,
                                                          cbuf->zslice,
                                                          PIPE_TRANSFER_READ_WRITE,
                                                          0, 0,
                                                          cbuf->width, 
                                                          cbuf->height);
	 if (!scene->cbuf_transfer[i])
	    goto fail;

	 scene->cbuf_map[i] = screen->transfer_map(screen, 
                                                 scene->cbuf_transfer[i]);
	 if (!scene->cbuf_map[i])
	    goto fail;
      }
   }

   /* Map the zsbuffer
    */
   zsbuf = scene->fb.zsbuf;
   if (zsbuf) {
      scene->zsbuf_transfer = screen->get_tex_transfer(screen,
                                                       zsbuf->texture,
                                                       zsbuf->face,
                                                       zsbuf->level,
                                                       zsbuf->zslice,
                                                       PIPE_TRANSFER_READ_WRITE,
                                                       0, 0,
                                                       zsbuf->width,
                                                       zsbuf->height);
      if (!scene->zsbuf_transfer)
         goto fail;

      scene->zsbuf_map = screen->transfer_map(screen, 
                                              scene->zsbuf_transfer);
      if (!scene->zsbuf_map)
	 goto fail;
   }

   return TRUE;

fail:
   /* Unmap and release transfers?
    */
   return FALSE;
}



/**
 * Called after rasterizer as finished rasterizing a scene. 
 * 
 * We want to call this from the pipe_context's current thread to
 * avoid having to have mutexes on the transfer functions.
 */
static void
lp_scene_unmap_buffers( struct lp_scene *scene )
{
   struct pipe_screen *screen = scene->pipe->screen;
   unsigned i;

   for (i = 0; i < scene->fb.nr_cbufs; i++) {
      if (scene->cbuf_map[i]) 
	 screen->transfer_unmap(screen, scene->cbuf_transfer[i]);

      if (scene->cbuf_transfer[i])
	 screen->tex_transfer_destroy(scene->cbuf_transfer[i]);

      scene->cbuf_transfer[i] = NULL;
      scene->cbuf_map[i] = NULL;
   }

   if (scene->zsbuf_map) 
      screen->transfer_unmap(screen, scene->zsbuf_transfer);

   if (scene->zsbuf_transfer)
      screen->tex_transfer_destroy(scene->zsbuf_transfer);

   scene->zsbuf_transfer = NULL;
   scene->zsbuf_map = NULL;

   util_unreference_framebuffer_state( &scene->fb );
}


void lp_scene_begin_binning( struct lp_scene *scene,
                             struct pipe_framebuffer_state *fb )
{
   assert(lp_scene_is_empty(scene));

   util_copy_framebuffer_state(&scene->fb, fb);

   scene->tiles_x = align(fb->width, TILE_SIZE) / TILE_SIZE;
   scene->tiles_y = align(fb->height, TILE_SIZE) / TILE_SIZE;
}


void lp_scene_rasterize( struct lp_scene *scene,
                         struct lp_rasterizer *rast,
                         boolean write_depth )
{
   if (0) {
      unsigned x, y;
      debug_printf("rasterize scene:\n");
      debug_printf("  data size: %u\n", lp_scene_data_size(scene));
      for (y = 0; y < scene->tiles_y; y++) {
         for (x = 0; x < scene->tiles_x; x++) {
            debug_printf("  bin %u, %u size: %u\n", x, y,
                         lp_scene_bin_size(scene, x, y));
         }
      }
   }


   scene->write_depth = (scene->fb.zsbuf != NULL &&
                         write_depth);

   lp_scene_map_buffers( scene );

   /* Enqueue the scene for rasterization, then immediately wait for
    * it to finish.
    */
   lp_rast_queue_scene( rast, scene );

   /* Currently just wait for the rasterizer to finish.  Some
    * threading interactions need to be worked out, particularly once
    * transfers become per-context:
    */
   lp_rast_finish( rast );
   lp_scene_unmap_buffers( scene );
   lp_scene_enqueue( scene->empty_queue, scene );
}
