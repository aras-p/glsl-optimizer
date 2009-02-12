/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/
/*
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#include <xf86drm.h>
#include <stdlib.h>
#include <errno.h>
#include "pipe/p_debug.h"
#include "pipe/p_thread.h"
#include "ws_dri_bufpool.h"
#include "ws_dri_bufmgr.h"

static void *
pool_create(struct _DriBufferPool *pool,
            unsigned long size, uint64_t flags, unsigned hint,
            unsigned alignment)
{
    unsigned long *private = malloc(size + 2*sizeof(unsigned long));
    if ((flags & DRM_BO_MASK_MEM) != DRM_BO_FLAG_MEM_LOCAL)
      abort();

    *private = size;
    return (void *)private;
}


static int
pool_destroy(struct _DriBufferPool *pool, void *private)
{
    free(private);
    return 0;
}

static int
pool_waitIdle(struct _DriBufferPool *pool, void *private,
	      pipe_mutex *mutex, int lazy)
{
    return 0;
}

static int
pool_map(struct _DriBufferPool *pool, void *private, unsigned flags,
         int hint, pipe_mutex *mutex, void **virtual)
{
    *virtual = (void *)((unsigned long *)private + 2);
    return 0;
}

static int
pool_unmap(struct _DriBufferPool *pool, void *private)
{
    return 0;
}

static unsigned long
pool_offset(struct _DriBufferPool *pool, void *private)
{
    /*
     * BUG
     */
    abort();
    return 0UL;
}

static unsigned long
pool_poolOffset(struct _DriBufferPool *pool, void *private)
{
    /*
     * BUG
     */
    abort();
}

static uint64_t
pool_flags(struct _DriBufferPool *pool, void *private)
{
    return DRM_BO_FLAG_MEM_LOCAL | DRM_BO_FLAG_CACHED;
}

static unsigned long
pool_size(struct _DriBufferPool *pool, void *private)
{
    return *(unsigned long *) private;
}


static int
pool_fence(struct _DriBufferPool *pool, void *private,
           struct _DriFenceObject *fence)
{
    abort();
    return 0UL;
}

static drmBO *
pool_kernel(struct _DriBufferPool *pool, void *private)
{
    abort();
    return NULL;
}

static void
pool_takedown(struct _DriBufferPool *pool)
{
    free(pool);
}


struct _DriBufferPool *
driMallocPoolInit(void)
{
   struct _DriBufferPool *pool;

   pool = (struct _DriBufferPool *) malloc(sizeof(*pool));
   if (!pool)
       return NULL;

   pool->data = NULL;
   pool->fd = -1;
   pool->map = &pool_map;
   pool->unmap = &pool_unmap;
   pool->destroy = &pool_destroy;
   pool->offset = &pool_offset;
   pool->poolOffset = &pool_poolOffset;
   pool->flags = &pool_flags;
   pool->size = &pool_size;
   pool->create = &pool_create;
   pool->fence = &pool_fence;
   pool->kernel = &pool_kernel;
   pool->validate = NULL;
   pool->waitIdle = &pool_waitIdle;
   pool->takeDown = &pool_takedown;
   return pool;
}
