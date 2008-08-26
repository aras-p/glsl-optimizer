/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
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
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include "ws_dri_bufpool.h"
#include "ws_dri_fencemgr.h"
#include "ws_dri_bufmgr.h"
#include "pipe/p_thread.h"

#define DRI_SLABPOOL_ALLOC_RETRIES 100

struct _DriSlab;

struct _DriSlabBuffer {
    int isSlabBuffer;
    drmBO *bo;
    struct _DriFenceObject *fence;
    struct _DriSlab *parent;
    drmMMListHead head;
    uint32_t mapCount;
    uint32_t start;
    uint32_t fenceType;
    int unFenced;
    pipe_condvar event;
};

struct _DriKernelBO {
    int fd;
    drmBO bo;
    drmMMListHead timeoutHead;
    drmMMListHead head;
    struct timeval timeFreed;
    uint32_t pageAlignment;
    void *virtual;
};

struct _DriSlab{
    drmMMListHead head;
    drmMMListHead freeBuffers;
    uint32_t numBuffers;
    uint32_t numFree;
    struct _DriSlabBuffer *buffers;
    struct _DriSlabSizeHeader *header;
    struct _DriKernelBO *kbo;
};


struct _DriSlabSizeHeader {
    drmMMListHead slabs;
    drmMMListHead freeSlabs;
    drmMMListHead delayedBuffers;
    uint32_t numDelayed;
    struct _DriSlabPool *slabPool;
    uint32_t bufSize;
    pipe_mutex mutex;
};

struct _DriFreeSlabManager {
    struct timeval slabTimeout;
    struct timeval checkInterval;
    struct timeval nextCheck;
    drmMMListHead timeoutList;
    drmMMListHead unCached;
    drmMMListHead cached;
    pipe_mutex mutex;
};


struct _DriSlabPool {

    /*
     * The data of this structure remains constant after
     * initialization and thus needs no mutex protection.
     */

    struct _DriFreeSlabManager *fMan;
    uint64_t proposedFlags;
    uint64_t validMask;
    uint32_t *bucketSizes;
    uint32_t numBuckets;
    uint32_t pageSize;
    int fd;
    int pageAlignment;
    int maxSlabSize;
    int desiredNumBuffers;
    struct _DriSlabSizeHeader *headers;
};

/*
 * FIXME: Perhaps arrange timeout slabs in size buckets for fast
 * retreival??
 */


static inline int
driTimeAfterEq(struct timeval *arg1, struct timeval *arg2)
{
    return ((arg1->tv_sec > arg2->tv_sec) ||
	    ((arg1->tv_sec == arg2->tv_sec) &&
	     (arg1->tv_usec > arg2->tv_usec)));
}

static inline void
driTimeAdd(struct timeval *arg, struct timeval *add)
{
    unsigned int sec;

    arg->tv_sec += add->tv_sec;
    arg->tv_usec += add->tv_usec;
    sec = arg->tv_usec / 1000000;
    arg->tv_sec += sec;
    arg->tv_usec -= sec*1000000;
}

static void
driFreeKernelBO(struct _DriKernelBO *kbo)
{
    if (!kbo)
	return;

    (void) drmBOUnreference(kbo->fd, &kbo->bo);
    free(kbo);
}


static void
driFreeTimeoutKBOsLocked(struct _DriFreeSlabManager *fMan,
			 struct timeval *time)
{
    drmMMListHead *list, *next;
    struct _DriKernelBO *kbo;

    if (!driTimeAfterEq(time, &fMan->nextCheck))
	return;

    for (list = fMan->timeoutList.next, next = list->next;
	 list != &fMan->timeoutList;
	 list = next, next = list->next) {

	kbo = DRMLISTENTRY(struct _DriKernelBO, list, timeoutHead);

	if (!driTimeAfterEq(time, &kbo->timeFreed))
	    break;

	DRMLISTDELINIT(&kbo->timeoutHead);
	DRMLISTDELINIT(&kbo->head);
	driFreeKernelBO(kbo);
    }

    fMan->nextCheck = *time;
    driTimeAdd(&fMan->nextCheck, &fMan->checkInterval);
}


/*
 * Add a _DriKernelBO to the free slab manager.
 * This means that it is available for reuse, but if it's not
 * reused in a while, it will be freed.
 */

static void
driSetKernelBOFree(struct _DriFreeSlabManager *fMan,
		   struct _DriKernelBO *kbo)
{
    struct timeval time;

    pipe_mutex_lock(fMan->mutex);
    gettimeofday(&time, NULL);
    driTimeAdd(&time, &fMan->slabTimeout);

    kbo->timeFreed = time;

    if (kbo->bo.flags & DRM_BO_FLAG_CACHED)
	DRMLISTADD(&kbo->head, &fMan->cached);
    else
	DRMLISTADD(&kbo->head, &fMan->unCached);

    DRMLISTADDTAIL(&kbo->timeoutHead, &fMan->timeoutList);
    driFreeTimeoutKBOsLocked(fMan, &time);

    pipe_mutex_unlock(fMan->mutex);
}

/*
 * Get a _DriKernelBO for us to use as storage for a slab.
 *
 */

static struct _DriKernelBO *
driAllocKernelBO(struct _DriSlabSizeHeader *header)

{
    struct _DriSlabPool *slabPool = header->slabPool;
    struct _DriFreeSlabManager *fMan = slabPool->fMan;
    drmMMListHead *list, *next, *head;
    uint32_t size = header->bufSize * slabPool->desiredNumBuffers;
    struct _DriKernelBO *kbo;
    struct _DriKernelBO *kboTmp;
    int ret;

    /*
     * FIXME: We should perhaps allow some variation in slabsize in order
     * to efficiently reuse slabs.
     */

    size = (size <= slabPool->maxSlabSize) ? size : slabPool->maxSlabSize;
    size = (size + slabPool->pageSize - 1) & ~(slabPool->pageSize - 1);
    pipe_mutex_lock(fMan->mutex);

    kbo = NULL;

  retry:
    head = (slabPool->proposedFlags & DRM_BO_FLAG_CACHED) ?
	&fMan->cached : &fMan->unCached;

    for (list = head->next, next = list->next;
	 list != head;
	 list = next, next = list->next) {

	kboTmp = DRMLISTENTRY(struct _DriKernelBO, list, head);

	if ((kboTmp->bo.size == size) &&
	    (slabPool->pageAlignment == 0 ||
	     (kboTmp->pageAlignment % slabPool->pageAlignment) == 0)) {

	    if (!kbo)
		kbo = kboTmp;

	    if ((kbo->bo.proposedFlags ^ slabPool->proposedFlags) == 0)
		break;

	}
    }

    if (kbo) {
	DRMLISTDELINIT(&kbo->head);
	DRMLISTDELINIT(&kbo->timeoutHead);
    }

    pipe_mutex_unlock(fMan->mutex);

    if (kbo) {
        uint64_t new_mask = kbo->bo.proposedFlags ^ slabPool->proposedFlags;

	ret = 0;
	if (new_mask) {
	    ret = drmBOSetStatus(kbo->fd, &kbo->bo, slabPool->proposedFlags,
				 new_mask, DRM_BO_HINT_DONT_FENCE, 0, 0);
	}
	if (ret == 0)
	    return kbo;

	driFreeKernelBO(kbo);
	kbo = NULL;
	goto retry;
    }

    kbo = calloc(1, sizeof(struct _DriKernelBO));
    if (!kbo)
	return NULL;

    kbo->fd = slabPool->fd;
    DRMINITLISTHEAD(&kbo->head);
    DRMINITLISTHEAD(&kbo->timeoutHead);
    ret = drmBOCreate(kbo->fd, size, slabPool->pageAlignment, NULL,
		      slabPool->proposedFlags,
		      DRM_BO_HINT_DONT_FENCE, &kbo->bo);
    if (ret)
	goto out_err0;

    ret = drmBOMap(kbo->fd, &kbo->bo,
		   DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE,
		   0, &kbo->virtual);

    if (ret)
	goto out_err1;

    ret = drmBOUnmap(kbo->fd, &kbo->bo);
    if (ret)
	goto out_err1;

    return kbo;

  out_err1:
    drmBOUnreference(kbo->fd, &kbo->bo);
  out_err0:
    free(kbo);
    return NULL;
}


static int
driAllocSlab(struct _DriSlabSizeHeader *header)
{
    struct _DriSlab *slab;
    struct _DriSlabBuffer *buf;
    uint32_t numBuffers;
    int ret;
    int i;

    slab = calloc(1, sizeof(*slab));
    if (!slab)
	return -ENOMEM;

    slab->kbo = driAllocKernelBO(header);
    if (!slab->kbo) {
	ret = -ENOMEM;
	goto out_err0;
    }

    numBuffers = slab->kbo->bo.size / header->bufSize;

    slab->buffers = calloc(numBuffers, sizeof(*slab->buffers));
    if (!slab->buffers) {
	ret = -ENOMEM;
	goto out_err1;
    }

    DRMINITLISTHEAD(&slab->head);
    DRMINITLISTHEAD(&slab->freeBuffers);
    slab->numBuffers = numBuffers;
    slab->numFree = 0;
    slab->header = header;

    buf = slab->buffers;
    for (i=0; i < numBuffers; ++i) {
	buf->parent = slab;
	buf->start = i* header->bufSize;
	buf->mapCount = 0;
	buf->isSlabBuffer = 1;
	pipe_condvar_init(buf->event);
	DRMLISTADDTAIL(&buf->head, &slab->freeBuffers);
	slab->numFree++;
	buf++;
    }

    DRMLISTADDTAIL(&slab->head, &header->slabs);

    return 0;

  out_err1:
    driSetKernelBOFree(header->slabPool->fMan, slab->kbo);
    free(slab->buffers);
  out_err0:
    free(slab);
    return ret;
}

/*
 * Delete a buffer from the slab header delayed list and put
 * it on the slab free list.
 */

static void
driSlabFreeBufferLocked(struct _DriSlabBuffer *buf)
{
    struct _DriSlab *slab = buf->parent;
    struct _DriSlabSizeHeader *header = slab->header;
    drmMMListHead *list = &buf->head;

    DRMLISTDEL(list);
    DRMLISTADDTAIL(list, &slab->freeBuffers);
    slab->numFree++;

    if (slab->head.next == &slab->head)
	DRMLISTADDTAIL(&slab->head, &header->slabs);

    if (slab->numFree == slab->numBuffers) {
	list = &slab->head;
	DRMLISTDEL(list);
	DRMLISTADDTAIL(list, &header->freeSlabs);
    }

    if (header->slabs.next == &header->slabs ||
	slab->numFree != slab->numBuffers) {

	drmMMListHead *next;
	struct _DriFreeSlabManager *fMan = header->slabPool->fMan;

	for (list = header->freeSlabs.next, next = list->next;
	     list != &header->freeSlabs;
	     list = next, next = list->next) {

	    slab = DRMLISTENTRY(struct _DriSlab, list, head);

	    DRMLISTDELINIT(list);
	    driSetKernelBOFree(fMan, slab->kbo);
	    free(slab->buffers);
	    free(slab);
	}
    }
}

static void
driSlabCheckFreeLocked(struct _DriSlabSizeHeader *header, int wait)
{
  drmMMListHead *list, *prev, *first;
   struct _DriSlabBuffer *buf;
   struct _DriSlab *slab;
   int firstWasSignaled = 1;
   int signaled;
   int i;
   int ret;

   /*
    * Rerun the freeing test if the youngest tested buffer
    * was signaled, since there might be more idle buffers
    * in the delay list.
    */

   while (firstWasSignaled) {
       firstWasSignaled = 0;
       signaled = 0;
       first = header->delayedBuffers.next;

       /* Only examine the oldest 1/3 of delayed buffers:
	*/
       if (header->numDelayed > 3) {
	   for (i = 0; i < header->numDelayed; i += 3) {
	       first = first->next;
	   }
       }

       for (list = first, prev = list->prev;
	    list != &header->delayedBuffers;
	    list = prev, prev = list->prev) {
	   buf = DRMLISTENTRY(struct _DriSlabBuffer, list, head);
	   slab = buf->parent;

	   if (!signaled) {
	       if (wait) {
		   ret = driFenceFinish(buf->fence, buf->fenceType, 0);
		   if (ret)
		       break;
		   signaled = 1;
		   wait = 0;
	       } else {
		   signaled = driFenceSignaled(buf->fence, buf->fenceType);
	       }
	       if (signaled) {
		   if (list == first)
		       firstWasSignaled = 1;
		   driFenceUnReference(&buf->fence);
		   header->numDelayed--;
		   driSlabFreeBufferLocked(buf);
	       }
	   } else if (driFenceSignaledCached(buf->fence, buf->fenceType)) {
	       driFenceUnReference(&buf->fence);
	       header->numDelayed--;
	       driSlabFreeBufferLocked(buf);
	   }
       }
   }
}


static struct _DriSlabBuffer *
driSlabAllocBuffer(struct _DriSlabSizeHeader *header)
{
    static struct _DriSlabBuffer *buf;
    struct _DriSlab *slab;
    drmMMListHead *list;
    int count = DRI_SLABPOOL_ALLOC_RETRIES;

    pipe_mutex_lock(header->mutex);
    while(header->slabs.next == &header->slabs && count > 0) {
        driSlabCheckFreeLocked(header, 0);
	if (header->slabs.next != &header->slabs)
	  break;

	pipe_mutex_unlock(header->mutex);
	if (count != DRI_SLABPOOL_ALLOC_RETRIES)
	    usleep(1);
	pipe_mutex_lock(header->mutex);
	(void) driAllocSlab(header);
	count--;
    }

    list = header->slabs.next;
    if (list == &header->slabs) {
	pipe_mutex_unlock(header->mutex);
	return NULL;
    }
    slab = DRMLISTENTRY(struct _DriSlab, list, head);
    if (--slab->numFree == 0)
	DRMLISTDELINIT(list);

    list = slab->freeBuffers.next;
    DRMLISTDELINIT(list);

    pipe_mutex_unlock(header->mutex);
    buf = DRMLISTENTRY(struct _DriSlabBuffer, list, head);
    return buf;
}

static void *
pool_create(struct _DriBufferPool *driPool, unsigned long size,
	    uint64_t flags, unsigned hint, unsigned alignment)
{
    struct _DriSlabPool *pool = (struct _DriSlabPool *) driPool->data;
    struct _DriSlabSizeHeader *header;
    struct _DriSlabBuffer *buf;
    void *dummy;
    int i;
    int ret;

    /*
     * FIXME: Check for compatibility.
     */

    header = pool->headers;
    for (i=0; i<pool->numBuckets; ++i) {
      if (header->bufSize >= size)
	break;
      header++;
    }

    if (i < pool->numBuckets)
	return driSlabAllocBuffer(header);


    /*
     * Fall back to allocate a buffer object directly from DRM.
     * and wrap it in a driBO structure.
     */


    buf = calloc(1, sizeof(*buf));

    if (!buf)
	return NULL;

    buf->bo = calloc(1, sizeof(*buf->bo));
    if (!buf->bo)
	goto out_err0;

    if (alignment) {
	if ((alignment < pool->pageSize) && (pool->pageSize % alignment))
	    goto out_err1;
	if ((alignment > pool->pageSize) && (alignment % pool->pageSize))
	    goto out_err1;
    }

    ret = drmBOCreate(pool->fd, size, alignment / pool->pageSize, NULL,
			flags, hint, buf->bo);
    if (ret)
	goto out_err1;

    ret  = drmBOMap(pool->fd, buf->bo, DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE,
		    0, &dummy);
    if (ret)
	goto out_err2;

    ret = drmBOUnmap(pool->fd, buf->bo);
    if (ret)
	goto out_err2;

    return buf;
  out_err2:
    drmBOUnreference(pool->fd, buf->bo);
  out_err1:
    free(buf->bo);
  out_err0:
    free(buf);
    return NULL;
}

static int
pool_destroy(struct _DriBufferPool *driPool, void *private)
{
    struct _DriSlabBuffer *buf =
	(struct _DriSlabBuffer *) private;
    struct _DriSlab *slab;
    struct _DriSlabSizeHeader *header;

    if (!buf->isSlabBuffer) {
	struct _DriSlabPool *pool = (struct _DriSlabPool *) driPool->data;
	int ret;

	ret = drmBOUnreference(pool->fd, buf->bo);
	free(buf->bo);
	free(buf);
	return ret;
    }

    slab = buf->parent;
    header = slab->header;

    pipe_mutex_lock(header->mutex);
    buf->unFenced = 0;
    buf->mapCount = 0;

    if (buf->fence && !driFenceSignaledCached(buf->fence, buf->fenceType)) {
	DRMLISTADDTAIL(&buf->head, &header->delayedBuffers);
	header->numDelayed++;
    } else {
	if (buf->fence)
	    driFenceUnReference(&buf->fence);
	driSlabFreeBufferLocked(buf);
    }

    pipe_mutex_unlock(header->mutex);
    return 0;
}

static int
pool_waitIdle(struct _DriBufferPool *driPool, void *private,
	      pipe_mutex *mutex, int lazy)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;

   while(buf->unFenced)
       pipe_condvar_wait(buf->event, *mutex);

   if (!buf->fence)
     return 0;

   driFenceFinish(buf->fence, buf->fenceType, lazy);
   driFenceUnReference(&buf->fence);

   return 0;
}

static int
pool_map(struct _DriBufferPool *pool, void *private, unsigned flags,
         int hint, pipe_mutex *mutex, void **virtual)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;
   int busy;

   if (buf->isSlabBuffer)
       busy = buf->unFenced || (buf->fence && !driFenceSignaledCached(buf->fence, buf->fenceType));
   else
       busy = buf->fence && !driFenceSignaled(buf->fence, buf->fenceType);


   if (busy) {
       if (hint & DRM_BO_HINT_DONT_BLOCK)
	   return -EBUSY;
       else {
	   (void) pool_waitIdle(pool, private, mutex, 0);
       }
   }

   ++buf->mapCount;
   *virtual = (buf->isSlabBuffer) ?
       (void *) ((uint8_t *) buf->parent->kbo->virtual + buf->start) :
       (void *) buf->bo->virtual;

   return 0;
}

static int
pool_unmap(struct _DriBufferPool *pool, void *private)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;

   --buf->mapCount;
   if (buf->mapCount == 0 && buf->isSlabBuffer)
      pipe_condvar_broadcast(buf->event);

   return 0;
}

static unsigned long
pool_offset(struct _DriBufferPool *pool, void *private)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;
   struct _DriSlab *slab;
   struct _DriSlabSizeHeader *header;

   if (!buf->isSlabBuffer) {
       assert(buf->bo->proposedFlags & DRM_BO_FLAG_NO_MOVE);
       return buf->bo->offset;
   }

   slab = buf->parent;
   header = slab->header;

   (void) header;
   assert(header->slabPool->proposedFlags & DRM_BO_FLAG_NO_MOVE);
   return slab->kbo->bo.offset + buf->start;
}

static unsigned long
pool_poolOffset(struct _DriBufferPool *pool, void *private)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;

   return buf->start;
}

static uint64_t
pool_flags(struct _DriBufferPool *pool, void *private)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;

   if (!buf->isSlabBuffer)
       return buf->bo->flags;

   return buf->parent->kbo->bo.flags;
}

static unsigned long
pool_size(struct _DriBufferPool *pool, void *private)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;
   if (!buf->isSlabBuffer)
       return buf->bo->size;

   return buf->parent->header->bufSize;
}

static int
pool_fence(struct _DriBufferPool *pool, void *private,
           struct _DriFenceObject *fence)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;
   drmBO *bo;

   if (buf->fence)
      driFenceUnReference(&buf->fence);

   buf->fence = driFenceReference(fence);
   bo = (buf->isSlabBuffer) ?
     &buf->parent->kbo->bo:
     buf->bo;
   buf->fenceType = bo->fenceFlags;

   buf->unFenced = 0;
   pipe_condvar_broadcast(buf->event);

   return 0;
}

static drmBO *
pool_kernel(struct _DriBufferPool *pool, void *private)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;

   return (buf->isSlabBuffer) ? &buf->parent->kbo->bo : buf->bo;
}

static int
pool_validate(struct _DriBufferPool *pool, void *private,
	      pipe_mutex *mutex)
{
   struct _DriSlabBuffer *buf = (struct _DriSlabBuffer *) private;

   if (!buf->isSlabBuffer)
       return 0;

   while(buf->mapCount != 0)
      pipe_condvar_wait(buf->event, *mutex);

   buf->unFenced = 1;
   return 0;
}


struct _DriFreeSlabManager *
driInitFreeSlabManager(uint32_t checkIntervalMsec, uint32_t slabTimeoutMsec)
{
    struct _DriFreeSlabManager *tmp;

    tmp = calloc(1, sizeof(*tmp));
    if (!tmp)
	return NULL;

    pipe_mutex_init(tmp->mutex);
    pipe_mutex_lock(tmp->mutex);
    tmp->slabTimeout.tv_usec = slabTimeoutMsec*1000;
    tmp->slabTimeout.tv_sec = tmp->slabTimeout.tv_usec / 1000000;
    tmp->slabTimeout.tv_usec -=  tmp->slabTimeout.tv_sec*1000000;

    tmp->checkInterval.tv_usec = checkIntervalMsec*1000;
    tmp->checkInterval.tv_sec = tmp->checkInterval.tv_usec / 1000000;
    tmp->checkInterval.tv_usec -=  tmp->checkInterval.tv_sec*1000000;

    gettimeofday(&tmp->nextCheck, NULL);
    driTimeAdd(&tmp->nextCheck, &tmp->checkInterval);
    DRMINITLISTHEAD(&tmp->timeoutList);
    DRMINITLISTHEAD(&tmp->unCached);
    DRMINITLISTHEAD(&tmp->cached);
    pipe_mutex_unlock(tmp->mutex);

    return tmp;
}

void
driFinishFreeSlabManager(struct _DriFreeSlabManager *fMan)
{
    struct timeval time;

    time = fMan->nextCheck;
    driTimeAdd(&time, &fMan->checkInterval);

    pipe_mutex_lock(fMan->mutex);
    driFreeTimeoutKBOsLocked(fMan, &time);
    pipe_mutex_unlock(fMan->mutex);

    assert(fMan->timeoutList.next == &fMan->timeoutList);
    assert(fMan->unCached.next == &fMan->unCached);
    assert(fMan->cached.next == &fMan->cached);

    free(fMan);
}

static void
driInitSizeHeader(struct _DriSlabPool *pool, uint32_t size,
		  struct _DriSlabSizeHeader *header)
{
    pipe_mutex_init(header->mutex);
    pipe_mutex_lock(header->mutex);

    DRMINITLISTHEAD(&header->slabs);
    DRMINITLISTHEAD(&header->freeSlabs);
    DRMINITLISTHEAD(&header->delayedBuffers);

    header->numDelayed = 0;
    header->slabPool = pool;
    header->bufSize = size;

    pipe_mutex_unlock(header->mutex);
}

static void
driFinishSizeHeader(struct _DriSlabSizeHeader *header)
{
    drmMMListHead *list, *next;
    struct _DriSlabBuffer *buf;

    pipe_mutex_lock(header->mutex);
    for (list = header->delayedBuffers.next, next = list->next;
	 list != &header->delayedBuffers;
	 list = next, next = list->next) {

	buf = DRMLISTENTRY(struct _DriSlabBuffer, list , head);
	if (buf->fence) {
	    (void) driFenceFinish(buf->fence, buf->fenceType, 0);
	    driFenceUnReference(&buf->fence);
	}
	header->numDelayed--;
	driSlabFreeBufferLocked(buf);
    }
    pipe_mutex_unlock(header->mutex);
}

static void
pool_takedown(struct _DriBufferPool *driPool)
{
   struct _DriSlabPool *pool = driPool->data;
   int i;

   for (i=0; i<pool->numBuckets; ++i) {
     driFinishSizeHeader(&pool->headers[i]);
   }

   free(pool->headers);
   free(pool->bucketSizes);
   free(pool);
   free(driPool);
}

struct _DriBufferPool *
driSlabPoolInit(int fd, uint64_t flags,
		uint64_t validMask,
		uint32_t smallestSize,
		uint32_t numSizes,
		uint32_t desiredNumBuffers,
		uint32_t maxSlabSize,
		uint32_t pageAlignment,
		struct _DriFreeSlabManager *fMan)
{
    struct _DriBufferPool *driPool;
    struct _DriSlabPool *pool;
    uint32_t i;

    driPool = calloc(1, sizeof(*driPool));
    if (!driPool)
	return NULL;

    pool = calloc(1, sizeof(*pool));
    if (!pool)
	goto out_err0;

    pool->bucketSizes = calloc(numSizes, sizeof(*pool->bucketSizes));
    if (!pool->bucketSizes)
	goto out_err1;

    pool->headers = calloc(numSizes, sizeof(*pool->headers));
    if (!pool->headers)
	goto out_err2;

    pool->fMan = fMan;
    pool->proposedFlags = flags;
    pool->validMask = validMask;
    pool->numBuckets = numSizes;
    pool->pageSize = getpagesize();
    pool->fd = fd;
    pool->pageAlignment = pageAlignment;
    pool->maxSlabSize = maxSlabSize;
    pool->desiredNumBuffers = desiredNumBuffers;

    for (i=0; i<pool->numBuckets; ++i) {
	pool->bucketSizes[i] = (smallestSize << i);
	driInitSizeHeader(pool, pool->bucketSizes[i],
			  &pool->headers[i]);
    }

    driPool->data = (void *) pool;
    driPool->map = &pool_map;
    driPool->unmap = &pool_unmap;
    driPool->destroy = &pool_destroy;
    driPool->offset = &pool_offset;
    driPool->poolOffset = &pool_poolOffset;
    driPool->flags = &pool_flags;
    driPool->size = &pool_size;
    driPool->create = &pool_create;
    driPool->fence = &pool_fence;
    driPool->kernel = &pool_kernel;
    driPool->validate = &pool_validate;
    driPool->waitIdle = &pool_waitIdle;
    driPool->takeDown = &pool_takedown;

    return driPool;

  out_err2:
    free(pool->bucketSizes);
  out_err1:
    free(pool);
  out_err0:
    free(driPool);

    return NULL;
}
