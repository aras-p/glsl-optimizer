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

#include "util/u_framebuffer.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_simple_list.h"
#include "lp_scene.h"
#include "lp_scene_queue.h"
#include "lp_fence.h"


/** List of texture references */
struct texture_ref {
   struct pipe_resource *texture;
   struct texture_ref *prev, *next;  /**< linked list w/ u_simple_list.h */
};



/**
 * Create a new scene object.
 * \param queue  the queue to put newly rendered/emptied scenes into
 */
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

   make_empty_list(&scene->resources);

   pipe_mutex_init(scene->mutex);

   return scene;
}


/**
 * Free all data associated with the given scene, and the scene itself.
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

   /* If there are any bins which weren't cleared by the loop above,
    * they will be caught (on debug builds at least) by this assert:
    */
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
      struct resource_ref *ref, *next, *ref_list = &scene->resources;
      for (ref = ref_list->next; ref != ref_list; ref = next) {
         next = next_elem(ref);
         pipe_resource_reference(&ref->resource, NULL);
         FREE(ref);
      }
      make_empty_list(ref_list);
   }

   lp_fence_reference(&scene->fence, NULL);

   scene->scene_size = 0;

   scene->has_color_clear = FALSE;
   scene->has_depthstencil_clear = FALSE;
}






struct cmd_block *
lp_bin_new_cmd_block( struct cmd_block_list *list )
{
   struct cmd_block *block = MALLOC_STRUCT(cmd_block);
   if (block) {
      list->tail->next = block;
      list->tail = block;
      block->next = NULL;
      block->count = 0;
   }
   return block;
}


struct data_block *
lp_bin_new_data_block( struct data_block_list *list )
{
   struct data_block *block = MALLOC_STRUCT(data_block);
   if (block) {
      list->tail->next = block;
      list->tail = block;
      block->next = NULL;
      block->used = 0;
   }
   return block;
}


/**
 * Return number of bytes used for all bin data within a scene.
 * This does not include resources (textures) referenced by the scene.
 */
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
 * Add a reference to a resource by the scene.
 */
void
lp_scene_add_resource_reference(struct lp_scene *scene,
                                struct pipe_resource *resource)
{
   struct resource_ref *ref = CALLOC_STRUCT(resource_ref);
   if (ref) {
      struct resource_ref *ref_list = &scene->resources;
      pipe_resource_reference(&ref->resource, resource);
      insert_at_tail(ref_list, ref);
   }

   scene->scene_size += llvmpipe_resource_size(resource);
}


/**
 * Does this scene have a reference to the given resource?
 */
boolean
lp_scene_is_resource_referenced(const struct lp_scene *scene,
                                const struct pipe_resource *resource)
{
   const struct resource_ref *ref_list = &scene->resources;
   const struct resource_ref *ref;
   foreach (ref, ref_list) {
      if (ref->resource == resource)
         return TRUE;
   }
   return FALSE;
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


void lp_scene_begin_binning( struct lp_scene *scene,
                             struct pipe_framebuffer_state *fb )
{
   assert(lp_scene_is_empty(scene));

   util_copy_framebuffer_state(&scene->fb, fb);

   scene->tiles_x = align(fb->width, TILE_SIZE) / TILE_SIZE;
   scene->tiles_y = align(fb->height, TILE_SIZE) / TILE_SIZE;

   assert(scene->tiles_x <= TILES_X);
   assert(scene->tiles_y <= TILES_Y);
}


void lp_scene_rasterize( struct lp_scene *scene,
                         struct lp_rasterizer *rast )
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

   /* Enqueue the scene for rasterization, then immediately wait for
    * it to finish.
    */
   lp_rast_queue_scene( rast, scene );

   /* Currently just wait for the rasterizer to finish.  Some
    * threading interactions need to be worked out, particularly once
    * transfers become per-context:
    */
   lp_rast_finish( rast );

   util_unreference_framebuffer_state( &scene->fb );

   /* put scene into the empty list */
   lp_scene_enqueue( scene->empty_queue, scene );
}
