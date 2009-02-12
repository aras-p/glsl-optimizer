/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
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
#include <unistd.h>
#include "ws_dri_bufpool.h"
#include "ws_dri_bufmgr.h"
#include "assert.h"

/*
 * Buffer pool implementation using DRM buffer objects as DRI buffer objects.
 */

static void *
pool_create(struct _DriBufferPool *pool,
            unsigned long size, uint64_t flags, unsigned hint,
            unsigned alignment)
{
   drmBO *buf = (drmBO *) malloc(sizeof(*buf));
   int ret;
   unsigned pageSize = getpagesize();

   if (!buf)
      return NULL;

   if ((alignment > pageSize) && (alignment % pageSize)) {
      free(buf);
      return NULL;
   }

   ret = drmBOCreate(pool->fd, size, alignment / pageSize,
		     NULL,
                     flags, hint, buf);
   if (ret) {
      free(buf);
      return NULL;
   }

   return (void *) buf;
}

static void *
pool_reference(struct _DriBufferPool *pool, unsigned handle)
{
   drmBO *buf = (drmBO *) malloc(sizeof(*buf));
   int ret;

   if (!buf)
      return NULL;

   ret = drmBOReference(pool->fd, handle, buf);

   if (ret) {
      free(buf);
      return NULL;
   }

   return (void *) buf;
}

static int
pool_destroy(struct _DriBufferPool *pool, void *private)
{
   int ret;
   drmBO *buf = (drmBO *) private;
   driReadLockKernelBO();
   ret = drmBOUnreference(pool->fd, buf);
   free(buf);
   driReadUnlockKernelBO();
   return ret;
}

static int
pool_unreference(struct _DriBufferPool *pool, void *private)
{
   int ret;
   drmBO *buf = (drmBO *) private;
   driReadLockKernelBO();
   ret = drmBOUnreference(pool->fd, buf);
   free(buf);
   driReadUnlockKernelBO();
   return ret;
}

static int
pool_map(struct _DriBufferPool *pool, void *private, unsigned flags,
         int hint, pipe_mutex *mutex, void **virtual)
{
   drmBO *buf = (drmBO *) private;
   int ret;

   driReadLockKernelBO();
   ret = drmBOMap(pool->fd, buf, flags, hint, virtual);
   driReadUnlockKernelBO();
   return ret;
}

static int
pool_unmap(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   int ret;

   driReadLockKernelBO();
   ret = drmBOUnmap(pool->fd, buf);
   driReadUnlockKernelBO();

   return ret;
}

static unsigned long
pool_offset(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   unsigned long offset;

   driReadLockKernelBO();
   assert(buf->flags & DRM_BO_FLAG_NO_MOVE);
   offset = buf->offset;
   driReadUnlockKernelBO();

   return buf->offset;
}

static unsigned long
pool_poolOffset(struct _DriBufferPool *pool, void *private)
{
   return 0;
}

static uint64_t
pool_flags(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   uint64_t flags;

   driReadLockKernelBO();
   flags = buf->flags;
   driReadUnlockKernelBO();

   return flags;
}


static unsigned long
pool_size(struct _DriBufferPool *pool, void *private)
{
   drmBO *buf = (drmBO *) private;
   unsigned long size;

   driReadLockKernelBO();
   size = buf->size;
   driReadUnlockKernelBO();

   return buf->size;
}

static int
pool_fence(struct _DriBufferPool *pool, void *private,
           struct _DriFenceObject *fence)
{
   /*
    * Noop. The kernel handles all fencing.
    */

   return 0;
}

static drmBO *
pool_kernel(struct _DriBufferPool *pool, void *private)
{
   return (drmBO *) private;
}

static int
pool_waitIdle(struct _DriBufferPool *pool, void *private, pipe_mutex *mutex,
	      int lazy)
{
   drmBO *buf = (drmBO *) private;
   int ret;

   driReadLockKernelBO();
   ret = drmBOWaitIdle(pool->fd, buf, (lazy) ? DRM_BO_HINT_WAIT_LAZY:0);
   driReadUnlockKernelBO();

   return ret;
}


static void
pool_takedown(struct _DriBufferPool *pool)
{
   free(pool);
}

/*static int
pool_setStatus(struct _DriBufferPool *pool, void *private,
	       uint64_t flag_diff, uint64_t old_flags)
{
   drmBO *buf = (drmBO *) private;
   uint64_t new_flags = old_flags ^ flag_diff;
   int ret;

   driReadLockKernelBO();
   ret = drmBOSetStatus(pool->fd, buf, new_flags, flag_diff,
			0, 0, 0);
   driReadUnlockKernelBO();
   return ret;
}*/

struct _DriBufferPool *
driDRMPoolInit(int fd)
{
   struct _DriBufferPool *pool;

   pool = (struct _DriBufferPool *) malloc(sizeof(*pool));

   if (!pool)
      return NULL;

   pool->fd = fd;
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
   pool->reference = &pool_reference;
   pool->unreference = &pool_unreference;
   pool->data = NULL;
   return pool;
}
