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


#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "lp_debug.h"
#include "lp_fence.h"


/**
 * Create a new fence object.
 *
 * The rank will be the number of bins in the scene.  Whenever a rendering
 * thread hits a fence command, it'll increment the fence counter.  When
 * the counter == the rank, the fence is finished.
 *
 * \param rank  the expected finished value of the fence counter.
 */
struct lp_fence *
lp_fence_create(unsigned rank)
{
   static int fence_id;
   struct lp_fence *fence = CALLOC_STRUCT(lp_fence);

   pipe_reference_init(&fence->reference, 1);

   pipe_mutex_init(fence->mutex);
   pipe_condvar_init(fence->signalled);

   fence->id = fence_id++;
   fence->rank = rank;

   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s %d\n", __FUNCTION__, fence->id);

   return fence;
}


/** Destroy a fence.  Called when refcount hits zero. */
void
lp_fence_destroy(struct lp_fence *fence)
{
   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s %d\n", __FUNCTION__, fence->id);

   pipe_mutex_destroy(fence->mutex);
   pipe_condvar_destroy(fence->signalled);
   FREE(fence);
}


/**
 * For reference counting.
 * This is a Gallium API function.
 */
static void
llvmpipe_fence_reference(struct pipe_screen *screen,
                         struct pipe_fence_handle **ptr,
                         struct pipe_fence_handle *fence)
{
   struct lp_fence **old = (struct lp_fence **) ptr;
   struct lp_fence *f = (struct lp_fence *) fence;

   lp_fence_reference(old, f);
}


/**
 * Has the fence been executed/finished?
 * This is a Gallium API function.
 */
static int
llvmpipe_fence_signalled(struct pipe_screen *screen,
                         struct pipe_fence_handle *fence,
                         unsigned flag)
{
   struct lp_fence *f = (struct lp_fence *) fence;

   return f->count == f->rank;
}


/**
 * Wait for the fence to finish.
 * This is a Gallium API function.
 */
static int
llvmpipe_fence_finish(struct pipe_screen *screen,
                      struct pipe_fence_handle *fence_handle,
                      unsigned flag)
{
   struct lp_fence *fence = (struct lp_fence *) fence_handle;

   pipe_mutex_lock(fence->mutex);
   while (fence->count < fence->rank) {
      pipe_condvar_wait(fence->signalled, fence->mutex);
   }
   pipe_mutex_unlock(fence->mutex);

   return 0;
}


/**
 * Called by the rendering threads to increment the fence counter.
 * When the counter == the rank, the fence is finished.
 */
void
lp_fence_signal(struct lp_fence *fence)
{
   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s %d\n", __FUNCTION__, fence->id);

   pipe_mutex_lock(fence->mutex);

   fence->count++;
   assert(fence->count <= fence->rank);

   if (LP_DEBUG & DEBUG_FENCE)
      debug_printf("%s count=%u rank=%u\n", __FUNCTION__,
                   fence->count, fence->rank);

   pipe_condvar_signal(fence->signalled);

   pipe_mutex_unlock(fence->mutex);
}


void
llvmpipe_init_screen_fence_funcs(struct pipe_screen *screen)
{
   screen->fence_reference = llvmpipe_fence_reference;
   screen->fence_signalled = llvmpipe_fence_signalled;
   screen->fence_finish = llvmpipe_fence_finish;
}
