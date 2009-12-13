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
 * Scene queue.  We'll use two queues.  One contains "full" scenes which
 * are produced by the "setup" code.  The other contains "empty" scenes
 * which are produced by the "rast" code when it finishes rendering a scene.
 */


#include "pipe/p_thread.h"
#include "util/u_memory.h"
#include "lp_scene_queue.h"



#define MAX_SCENE_QUEUE 4


/**
 * A queue of scenes
 */
struct lp_scene_queue
{
   /** XXX might use a linked list here somedone, but the list will
    * probably always be pretty short.
    */
   struct lp_scene *scenes[MAX_SCENE_QUEUE];
   unsigned count;

   pipe_condvar count_change;
   pipe_mutex mutex;
};



/** Allocate a new scene queue */
struct lp_scene_queue *
lp_scene_queue_create(void)
{
   struct lp_scene_queue *queue = CALLOC_STRUCT(lp_scene_queue);
   if (queue) {
      pipe_condvar_init(queue->count_change);
      pipe_mutex_init(queue->mutex);
   }
   return queue;
}


/** Delete a scene queue */
void
lp_scene_queue_destroy(struct lp_scene_queue *queue)
{
   pipe_condvar_destroy(queue->count_change);
   pipe_mutex_destroy(queue->mutex);
}


/** Remove first lp_scene from head of queue */
struct lp_scene *
lp_scene_dequeue(struct lp_scene_queue *queue)
{
   struct lp_scene *scene;
   unsigned i;

   pipe_mutex_lock(queue->mutex);
   while (queue->count == 0) {
      pipe_condvar_wait(queue->count_change, queue->mutex);
   }

   assert(queue->count >= 1);

   /* get head */
   scene = queue->scenes[0];

   /* shift entries */
   for (i = 0; i < queue->count - 1; i++) {
      queue->scenes[i] = queue->scenes[i + 1];
   }

   queue->count--;

   /* signal size change */
   pipe_condvar_signal(queue->count_change);

   pipe_mutex_unlock(queue->mutex);

   return scene;
}


/** Add an lp_scene to tail of queue */
void
lp_scene_enqueue(struct lp_scene_queue *queue, struct lp_scene *scene)
{
   pipe_mutex_lock(queue->mutex);

   assert(queue->count < MAX_SCENE_QUEUE);

   /* debug: check that scene is not already in the queue */
   if (0) {
      unsigned i;
      for (i = 0; i < queue->count; i++) {
         assert(queue->scenes[i] != scene);
      }
   }

   /* add to end */
   queue->scenes[queue->count++] = scene;

   /* signal size change */
   pipe_condvar_signal(queue->count_change);

   pipe_mutex_unlock(queue->mutex);
}


/** Return number of entries in the queue */
unsigned
lp_scene_queue_count(struct lp_scene_queue *queue)
{
   unsigned count;
   pipe_mutex_lock(queue->mutex);
   count = queue->count;
   pipe_mutex_unlock(queue->mutex);
   return count;
}


/** Wait until the queue has exactly 'count' entries */
void
lp_scene_queue_wait_count(struct lp_scene_queue *queue, unsigned count)
{
   pipe_mutex_lock(queue->mutex);
   while (queue->count != count) {
      pipe_condvar_wait(queue->count_change, queue->mutex);
   }
   pipe_mutex_unlock(queue->mutex);
}
