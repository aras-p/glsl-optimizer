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
#include "util/u_inlines.h"
#include "lp_fence.h"


struct lp_fence *
lp_fence_create(unsigned rank)
{
   struct lp_fence *fence = CALLOC_STRUCT(lp_fence);

   pipe_reference_init(&fence->reference, 1);

   pipe_mutex_init(fence->mutex);
   pipe_condvar_init(fence->signalled);

   fence->rank = rank;

   return fence;
}


static void
lp_fence_destroy(struct lp_fence *fence)
{
   pipe_mutex_destroy(fence->mutex);
   pipe_condvar_destroy(fence->signalled);
   FREE(fence);
}


static void
llvmpipe_fence_reference(struct pipe_screen *screen,
                         struct pipe_fence_handle **ptr,
                         struct pipe_fence_handle *fence)
{
   struct lp_fence *old = (struct lp_fence *) *ptr;
   struct lp_fence *f = (struct lp_fence *) fence;

   if (pipe_reference(&old->reference, &f->reference)) {
      lp_fence_destroy(old);
   }
}


static int
llvmpipe_fence_signalled(struct pipe_screen *screen,
                         struct pipe_fence_handle *fence,
                         unsigned flag)
{
   struct lp_fence *f = (struct lp_fence *) fence;

   return f->count == f->rank;
}


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




void
llvmpipe_init_screen_fence_funcs(struct pipe_screen *screen)
{
   screen->fence_reference = llvmpipe_fence_reference;
   screen->fence_signalled = llvmpipe_fence_signalled;
   screen->fence_finish = llvmpipe_fence_finish;
}
