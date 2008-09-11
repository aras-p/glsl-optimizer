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
 *          Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#include <xf86drm.h>
#include <stdlib.h>
#include <stdio.h>
#include "pipe/p_thread.h"
#include "errno.h"
#include "ws_dri_bufmgr.h"
#include "string.h"
#include "pipe/p_debug.h"
#include "ws_dri_bufpool.h"
#include "ws_dri_fencemgr.h"


/*
 * This lock is here to protect drmBO structs changing underneath us during a
 * validate list call, since validatelist cannot take individiual locks for
 * each drmBO. Validatelist takes this lock in write mode. Any access to an
 * individual drmBO should take this lock in read mode, since in that case, the
 * driBufferObject mutex will protect the access. Locking order is
 * driBufferObject mutex - > this rw lock.
 */

pipe_static_mutex(bmMutex);
pipe_static_condvar(bmCond);

static int kernelReaders = 0;
static int num_buffers = 0;
static int num_user_buffers = 0;

static drmBO *drmBOListBuf(void *iterator)
{
    drmBONode *node;
    drmMMListHead *l = (drmMMListHead *) iterator;
    node = DRMLISTENTRY(drmBONode, l, head);
    return node->buf;
}

static void *drmBOListIterator(drmBOList *list)
{
    void *ret = list->list.next;

    if (ret == &list->list)
	return NULL;
    return ret;
}

static void *drmBOListNext(drmBOList *list, void *iterator)
{
    void *ret;

    drmMMListHead *l = (drmMMListHead *) iterator;
    ret = l->next;
    if (ret == &list->list)
	return NULL;
    return ret;
}

static drmBONode *drmAddListItem(drmBOList *list, drmBO *item,
				 uint64_t arg0,
				 uint64_t arg1)
{
    drmBONode *node;
    drmMMListHead *l;

    l = list->free.next;
    if (l == &list->free) {
	node = (drmBONode *) malloc(sizeof(*node));
	if (!node) {
	    return NULL;
	}
	list->numCurrent++;
    }
    else {
	DRMLISTDEL(l);
	node = DRMLISTENTRY(drmBONode, l, head);
    }
    node->buf = item;
    node->arg0 = arg0;
    node->arg1 = arg1;
    DRMLISTADD(&node->head, &list->list);
    list->numOnList++;
    return node;
}

static int drmAddValidateItem(drmBOList *list, drmBO *buf, uint64_t flags,
			      uint64_t mask, int *newItem)
{
    drmBONode *node, *cur;
    drmMMListHead *l;

    *newItem = 0;
    cur = NULL;

    for (l = list->list.next; l != &list->list; l = l->next) {
	node = DRMLISTENTRY(drmBONode, l, head);
	if (node->buf == buf) {
	    cur = node;
	    break;
	}
    }
    if (!cur) {
	cur = drmAddListItem(list, buf, flags, mask);
	if (!cur) {
	    return -ENOMEM;
	}
	*newItem = 1;
	cur->arg0 = flags;
	cur->arg1 = mask;
    }
    else {
        uint64_t memFlags = cur->arg0 & flags & DRM_BO_MASK_MEM;
	uint64_t accFlags = (cur->arg0 | flags) & ~DRM_BO_MASK_MEM;

	if (mask & cur->arg1 & ~DRM_BO_MASK_MEM  & (cur->arg0 ^ flags)) {
	    return -EINVAL;
	}

	cur->arg1 |= mask;
	cur->arg0 = (cur->arg0 & ~mask) | ((memFlags | accFlags) & mask);

	if (((cur->arg1 & DRM_BO_MASK_MEM) != 0) &&
	    (cur->arg0 & DRM_BO_MASK_MEM) == 0) {
	    return -EINVAL;
	}
    }
    return 0;
}

static void drmBOFreeList(drmBOList *list)
{
    drmBONode *node;
    drmMMListHead *l;

    l = list->list.next;
    while(l != &list->list) {
	DRMLISTDEL(l);
	node = DRMLISTENTRY(drmBONode, l, head);
	free(node);
	l = list->list.next;
	list->numCurrent--;
	list->numOnList--;
    }

    l = list->free.next;
    while(l != &list->free) {
	DRMLISTDEL(l);
	node = DRMLISTENTRY(drmBONode, l, head);
	free(node);
	l = list->free.next;
	list->numCurrent--;
    }
}

static int drmAdjustListNodes(drmBOList *list)
{
    drmBONode *node;
    drmMMListHead *l;
    int ret = 0;

    while(list->numCurrent < list->numTarget) {
	node = (drmBONode *) malloc(sizeof(*node));
	if (!node) {
	    ret = -ENOMEM;
	    break;
	}
	list->numCurrent++;
	DRMLISTADD(&node->head, &list->free);
    }

    while(list->numCurrent > list->numTarget) {
	l = list->free.next;
	if (l == &list->free)
	    break;
	DRMLISTDEL(l);
	node = DRMLISTENTRY(drmBONode, l, head);
	free(node);
	list->numCurrent--;
    }
    return ret;
}

static int drmBOCreateList(int numTarget, drmBOList *list)
{
    DRMINITLISTHEAD(&list->list);
    DRMINITLISTHEAD(&list->free);
    list->numTarget = numTarget;
    list->numCurrent = 0;
    list->numOnList = 0;
    return drmAdjustListNodes(list);
}

static int drmBOResetList(drmBOList *list)
{
    drmMMListHead *l;
    int ret;

    ret = drmAdjustListNodes(list);
    if (ret)
	return ret;

    l = list->list.next;
    while (l != &list->list) {
	DRMLISTDEL(l);
	DRMLISTADD(l, &list->free);
	list->numOnList--;
	l = list->list.next;
    }
    return drmAdjustListNodes(list);
}

void driWriteLockKernelBO(void)
{
    pipe_mutex_lock(bmMutex);
    while(kernelReaders != 0)
	pipe_condvar_wait(bmCond, bmMutex);
}

void driWriteUnlockKernelBO(void)
{
    pipe_mutex_unlock(bmMutex);
}

void driReadLockKernelBO(void)
{
    pipe_mutex_lock(bmMutex);
    kernelReaders++;
    pipe_mutex_unlock(bmMutex);
}

void driReadUnlockKernelBO(void)
{
    pipe_mutex_lock(bmMutex);
    if (--kernelReaders == 0)
       pipe_condvar_broadcast(bmCond);
    pipe_mutex_unlock(bmMutex);
}




/*
 * TODO: Introduce fence pools in the same way as
 * buffer object pools.
 */

typedef struct _DriBufferObject
{
   DriBufferPool *pool;
   pipe_mutex mutex;
   int refCount;
   const char *name;
   uint64_t flags;
   unsigned hint;
   unsigned alignment;
   unsigned createdByReference;
   void *private;
   /* user-space buffer: */
   unsigned userBuffer;
   void *userData;
   unsigned userSize;
} DriBufferObject;

typedef struct _DriBufferList {
    drmBOList drmBuffers;  /* List of kernel buffers needing validation */
    drmBOList driBuffers;  /* List of user-space buffers needing validation */
} DriBufferList;


void
bmError(int val, const char *file, const char *function, int line)
{
   printf("Fatal video memory manager error \"%s\".\n"
          "Check kernel logs or set the LIBGL_DEBUG\n"
          "environment variable to \"verbose\" for more info.\n"
          "Detected in file %s, line %d, function %s.\n",
          strerror(-val), file, line, function);
#ifndef NDEBUG
   abort();
#else
   abort();
#endif
}

extern drmBO *
driBOKernel(struct _DriBufferObject *buf)
{
   drmBO *ret;

   driReadLockKernelBO();
   pipe_mutex_lock(buf->mutex);
   assert(buf->private != NULL);
   ret = buf->pool->kernel(buf->pool, buf->private);
   if (!ret)
      BM_CKFATAL(-EINVAL);
   pipe_mutex_unlock(buf->mutex);
   driReadUnlockKernelBO();

   return ret;
}

void
driBOWaitIdle(struct _DriBufferObject *buf, int lazy)
{

  /*
   * This function may block. Is it sane to keep the mutex held during
   * that time??
   */

   pipe_mutex_lock(buf->mutex);
   BM_CKFATAL(buf->pool->waitIdle(buf->pool, buf->private, &buf->mutex, lazy));
   pipe_mutex_unlock(buf->mutex);
}

void *
driBOMap(struct _DriBufferObject *buf, unsigned flags, unsigned hint)
{
   void *virtual;
   int retval;

   if (buf->userBuffer) {
      return buf->userData;
   }

   pipe_mutex_lock(buf->mutex);
   assert(buf->private != NULL);
   retval = buf->pool->map(buf->pool, buf->private, flags, hint,
			   &buf->mutex, &virtual);
   pipe_mutex_unlock(buf->mutex);

   return retval == 0 ? virtual : NULL;
}

void
driBOUnmap(struct _DriBufferObject *buf)
{
   if (buf->userBuffer)
      return;

   assert(buf->private != NULL);
   pipe_mutex_lock(buf->mutex);
   BM_CKFATAL(buf->pool->unmap(buf->pool, buf->private));
   pipe_mutex_unlock(buf->mutex);
}

unsigned long
driBOOffset(struct _DriBufferObject *buf)
{
   unsigned long ret;

   assert(buf->private != NULL);

   pipe_mutex_lock(buf->mutex);
   ret = buf->pool->offset(buf->pool, buf->private);
   pipe_mutex_unlock(buf->mutex);
   return ret;
}

unsigned long
driBOPoolOffset(struct _DriBufferObject *buf)
{
   unsigned long ret;

   assert(buf->private != NULL);

   pipe_mutex_lock(buf->mutex);
   ret = buf->pool->poolOffset(buf->pool, buf->private);
   pipe_mutex_unlock(buf->mutex);
   return ret;
}

uint64_t
driBOFlags(struct _DriBufferObject *buf)
{
   uint64_t ret;

   assert(buf->private != NULL);

   driReadLockKernelBO();
   pipe_mutex_lock(buf->mutex);
   ret = buf->pool->flags(buf->pool, buf->private);
   pipe_mutex_unlock(buf->mutex);
   driReadUnlockKernelBO();
   return ret;
}

struct _DriBufferObject *
driBOReference(struct _DriBufferObject *buf)
{
   pipe_mutex_lock(buf->mutex);
   if (++buf->refCount == 1) {
      pipe_mutex_unlock(buf->mutex);
      BM_CKFATAL(-EINVAL);
   }
   pipe_mutex_unlock(buf->mutex);
   return buf;
}

void
driBOUnReference(struct _DriBufferObject *buf)
{
   int tmp;

   if (!buf)
      return;

   pipe_mutex_lock(buf->mutex);
   tmp = --buf->refCount;
   if (!tmp) {
      pipe_mutex_unlock(buf->mutex);
      if (buf->private) {
	 if (buf->createdByReference)
	    buf->pool->unreference(buf->pool, buf->private);
	 else
	    buf->pool->destroy(buf->pool, buf->private);
      }
      if (buf->userBuffer)
	 num_user_buffers--;
      else
	 num_buffers--;
      free(buf);
   } else
     pipe_mutex_unlock(buf->mutex);

}


int
driBOData(struct _DriBufferObject *buf,
          unsigned size, const void *data,
	  DriBufferPool *newPool,
	  uint64_t flags)
{
   void *virtual = NULL;
   int newBuffer;
   int retval = 0;
   struct _DriBufferPool *pool;

   assert(!buf->userBuffer); /* XXX just do a memcpy? */

   pipe_mutex_lock(buf->mutex);
   pool = buf->pool;

   if (pool == NULL && newPool != NULL) {
       buf->pool = newPool;
       pool = newPool;
   }
   if (newPool == NULL)
       newPool = pool;

   if (!pool->create) {
      assert((size_t)"driBOData called on invalid buffer\n" & 0);
      BM_CKFATAL(-EINVAL);
   }

   newBuffer = (!buf->private || pool != newPool ||
 		pool->size(pool, buf->private) < size);

   if (!flags)
       flags = buf->flags;

   if (newBuffer) {

       if (buf->createdByReference) {
	  assert((size_t)"driBOData requiring resizing called on shared buffer.\n" & 0);
	  BM_CKFATAL(-EINVAL);
       }

       if (buf->private)
	   buf->pool->destroy(buf->pool, buf->private);

       pool = newPool;
       buf->pool = newPool;
       buf->private = pool->create(pool, size, flags, DRM_BO_HINT_DONT_FENCE,
				  buf->alignment);
      if (!buf->private)
	  retval = -ENOMEM;

      if (retval == 0)
	  retval = pool->map(pool, buf->private,
			     DRM_BO_FLAG_WRITE,
			     DRM_BO_HINT_DONT_BLOCK, &buf->mutex, &virtual);
   } else if (pool->map(pool, buf->private, DRM_BO_FLAG_WRITE,
			DRM_BO_HINT_DONT_BLOCK, &buf->mutex, &virtual)) {
       /*
	* Buffer is busy. need to create a new one.
	*/

       void *newBuf;

       newBuf = pool->create(pool, size, flags, DRM_BO_HINT_DONT_FENCE,
			     buf->alignment);
       if (newBuf) {
	   buf->pool->destroy(buf->pool, buf->private);
	   buf->private = newBuf;
       }

       retval = pool->map(pool, buf->private,
			  DRM_BO_FLAG_WRITE, 0, &buf->mutex, &virtual);
   } else {
       uint64_t flag_diff = flags ^ buf->flags;

       /*
	* We might need to change buffer flags.
	*/

       if (flag_diff){
	   assert(pool->setStatus != NULL);
	   BM_CKFATAL(pool->unmap(pool, buf->private));
	   BM_CKFATAL(pool->setStatus(pool, buf->private, flag_diff,
				      buf->flags));
	   if (!data)
	     goto out;

	   retval = pool->map(pool, buf->private,
			      DRM_BO_FLAG_WRITE, 0, &buf->mutex, &virtual);
       }
   }

   if (retval == 0) {
      if (data)
	 memcpy(virtual, data, size);

      BM_CKFATAL(pool->unmap(pool, buf->private));
   }

 out:
   pipe_mutex_unlock(buf->mutex);

   return retval;
}

void
driBOSubData(struct _DriBufferObject *buf,
             unsigned long offset, unsigned long size, const void *data)
{
   void *virtual;

   assert(!buf->userBuffer); /* XXX just do a memcpy? */

   pipe_mutex_lock(buf->mutex);
   if (size && data) {
      BM_CKFATAL(buf->pool->map(buf->pool, buf->private,
                                DRM_BO_FLAG_WRITE, 0, &buf->mutex,
				&virtual));
      memcpy((unsigned char *) virtual + offset, data, size);
      BM_CKFATAL(buf->pool->unmap(buf->pool, buf->private));
   }
   pipe_mutex_unlock(buf->mutex);
}

void
driBOGetSubData(struct _DriBufferObject *buf,
                unsigned long offset, unsigned long size, void *data)
{
   void *virtual;

   assert(!buf->userBuffer); /* XXX just do a memcpy? */

   pipe_mutex_lock(buf->mutex);
   if (size && data) {
      BM_CKFATAL(buf->pool->map(buf->pool, buf->private,
                                DRM_BO_FLAG_READ, 0, &buf->mutex, &virtual));
      memcpy(data, (unsigned char *) virtual + offset, size);
      BM_CKFATAL(buf->pool->unmap(buf->pool, buf->private));
   }
   pipe_mutex_unlock(buf->mutex);
}

void
driBOSetReferenced(struct _DriBufferObject *buf,
		   unsigned long handle)
{
   pipe_mutex_lock(buf->mutex);
   if (buf->private != NULL) {
      assert((size_t)"Invalid buffer for setReferenced\n" & 0);
      BM_CKFATAL(-EINVAL);

   }
   if (buf->pool->reference == NULL) {
      assert((size_t)"Invalid buffer pool for setReferenced\n" & 0);
      BM_CKFATAL(-EINVAL);
   }
   buf->private = buf->pool->reference(buf->pool, handle);
   if (!buf->private) {
      assert((size_t)"Invalid buffer pool for setStatic\n" & 0);
      BM_CKFATAL(-ENOMEM);
   }
   buf->createdByReference = TRUE;
   buf->flags = buf->pool->kernel(buf->pool, buf->private)->flags;
   pipe_mutex_unlock(buf->mutex);
}

int
driGenBuffers(struct _DriBufferPool *pool,
              const char *name,
              unsigned n,
              struct _DriBufferObject *buffers[],
              unsigned alignment, uint64_t flags, unsigned hint)
{
   struct _DriBufferObject *buf;
   int i;

   flags = (flags) ? flags : DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_MEM_VRAM |
      DRM_BO_FLAG_MEM_LOCAL | DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE;

   ++num_buffers;

   assert(pool);

   for (i = 0; i < n; ++i) {
      buf = (struct _DriBufferObject *) calloc(1, sizeof(*buf));
      if (!buf)
	 return -ENOMEM;

      pipe_mutex_init(buf->mutex);
      pipe_mutex_lock(buf->mutex);
      buf->refCount = 1;
      buf->flags = flags;
      buf->hint = hint;
      buf->name = name;
      buf->alignment = alignment;
      buf->pool = pool;
      buf->createdByReference = 0;
      pipe_mutex_unlock(buf->mutex);
      buffers[i] = buf;
   }
   return 0;
}

void
driGenUserBuffer(struct _DriBufferPool *pool,
                 const char *name,
                 struct _DriBufferObject **buffers,
                 void *ptr, unsigned bytes)
{
   const unsigned alignment = 1, flags = 0, hint = 0;

   --num_buffers; /* JB: is inced in GenBuffes */
   driGenBuffers(pool, name, 1, buffers, alignment, flags, hint);
   ++num_user_buffers;

   (*buffers)->userBuffer = 1;
   (*buffers)->userData = ptr;
   (*buffers)->userSize = bytes;
}

void
driDeleteBuffers(unsigned n, struct _DriBufferObject *buffers[])
{
   int i;

   for (i = 0; i < n; ++i) {
      driBOUnReference(buffers[i]);
   }
}


void
driInitBufMgr(int fd)
{
   ;
}

/*
 * Note that lists are per-context and don't need mutex protection.
 */

struct _DriBufferList *
driBOCreateList(int target)
{
    struct _DriBufferList *list = calloc(sizeof(*list), 1);

    BM_CKFATAL(drmBOCreateList(target, &list->drmBuffers));
    BM_CKFATAL(drmBOCreateList(target, &list->driBuffers));
    return list;
}

int
driBOResetList(struct _DriBufferList * list)
{
    int ret;
    ret = drmBOResetList(&list->drmBuffers);
    if (ret)
	return ret;
    ret = drmBOResetList(&list->driBuffers);
    return ret;
}

void
driBOFreeList(struct _DriBufferList * list)
{
   drmBOFreeList(&list->drmBuffers);
   drmBOFreeList(&list->driBuffers);
   free(list);
}


/*
 * Copied from libdrm, because it is needed by driAddValidateItem.
 */

static drmBONode *
driAddListItem(drmBOList * list, drmBO * item,
	       uint64_t arg0, uint64_t arg1)
{
    drmBONode *node;
    drmMMListHead *l;

    l = list->free.next;
    if (l == &list->free) {
	node = (drmBONode *) malloc(sizeof(*node));
	if (!node) {
	    return NULL;
	}
	list->numCurrent++;
    } else {
	DRMLISTDEL(l);
	node = DRMLISTENTRY(drmBONode, l, head);
    }
    memset(&node->bo_arg, 0, sizeof(node->bo_arg));
    node->buf = item;
    node->arg0 = arg0;
    node->arg1 = arg1;
    DRMLISTADDTAIL(&node->head, &list->list);
    list->numOnList++;
    return node;
}

/*
 * Slightly modified version compared to the libdrm version.
 * This one returns the list index of the buffer put on the list.
 */

static int
driAddValidateItem(drmBOList * list, drmBO * buf, uint64_t flags,
		   uint64_t mask, int *itemLoc,
		   struct _drmBONode **pnode)
{
    drmBONode *node, *cur;
    drmMMListHead *l;
    int count = 0;

    cur = NULL;

    for (l = list->list.next; l != &list->list; l = l->next) {
	node = DRMLISTENTRY(drmBONode, l, head);
	if (node->buf == buf) {
	    cur = node;
	    break;
	}
	count++;
    }
    if (!cur) {
	cur = driAddListItem(list, buf, flags, mask);
	if (!cur)
	    return -ENOMEM;

	cur->arg0 = flags;
	cur->arg1 = mask;
    } else {
        uint64_t memFlags = cur->arg0 & flags & DRM_BO_MASK_MEM;
	uint64_t accFlags = (cur->arg0 | flags) & ~DRM_BO_MASK_MEM;

	if (mask & cur->arg1 & ~DRM_BO_MASK_MEM  & (cur->arg0 ^ flags)) {
	    return -EINVAL;
	}

	cur->arg1 |= mask;
	cur->arg0 = (cur->arg0 & ~mask) | ((memFlags | accFlags) & mask);

	if (((cur->arg1 & DRM_BO_MASK_MEM) != 0) &&
	    (cur->arg0 & DRM_BO_MASK_MEM) == 0) {
	    return -EINVAL;
	}
    }
    *itemLoc = count;
    *pnode = cur;
    return 0;
}


void
driBOAddListItem(struct _DriBufferList * list, struct _DriBufferObject *buf,
                 uint64_t flags, uint64_t mask, int *itemLoc,
		 struct _drmBONode **node)
{
   int newItem;

   pipe_mutex_lock(buf->mutex);
   BM_CKFATAL(driAddValidateItem(&list->drmBuffers,
				 buf->pool->kernel(buf->pool, buf->private),
                                 flags, mask, itemLoc, node));
   BM_CKFATAL(drmAddValidateItem(&list->driBuffers, (drmBO *) buf,
				 flags, mask, &newItem));
   if (newItem)
     buf->refCount++;

   pipe_mutex_unlock(buf->mutex);
}

drmBOList *driGetdrmBOList(struct _DriBufferList *list)
{
	driWriteLockKernelBO();
	return &list->drmBuffers;
}

void driPutdrmBOList(struct _DriBufferList *list)
{
	driWriteUnlockKernelBO();
}


void
driBOFence(struct _DriBufferObject *buf, struct _DriFenceObject *fence)
{
   pipe_mutex_lock(buf->mutex);
   if (buf->pool->fence)
       BM_CKFATAL(buf->pool->fence(buf->pool, buf->private, fence));
   pipe_mutex_unlock(buf->mutex);

}

void
driBOUnrefUserList(struct _DriBufferList *list)
{
    struct _DriBufferObject *buf;
    void *curBuf;

    curBuf = drmBOListIterator(&list->driBuffers);
    while (curBuf) {
	buf = (struct _DriBufferObject *)drmBOListBuf(curBuf);
	driBOUnReference(buf);
	curBuf = drmBOListNext(&list->driBuffers, curBuf);
    }
}

struct _DriFenceObject *
driBOFenceUserList(struct _DriFenceMgr *mgr,
		   struct _DriBufferList *list, const char *name,
		   drmFence *kFence)
{
    struct _DriFenceObject *fence;
    struct _DriBufferObject *buf;
    void *curBuf;

    fence = driFenceCreate(mgr, kFence->fence_class, kFence->type,
			   kFence, sizeof(*kFence));
    curBuf = drmBOListIterator(&list->driBuffers);

   /*
    * User-space fencing callbacks.
    */

   while (curBuf) {
        buf = (struct _DriBufferObject *) drmBOListBuf(curBuf);
	driBOFence(buf, fence);
	driBOUnReference(buf);
	curBuf = drmBOListNext(&list->driBuffers, curBuf);
   }

   driBOResetList(list);
   return fence;
}

void
driBOValidateUserList(struct _DriBufferList * list)
{
    void *curBuf;
    struct _DriBufferObject *buf;

    curBuf = drmBOListIterator(&list->driBuffers);

    /*
     * User-space validation callbacks.
     */

    while (curBuf) {
	buf = (struct _DriBufferObject *) drmBOListBuf(curBuf);
	pipe_mutex_lock(buf->mutex);
	if (buf->pool->validate)
	    BM_CKFATAL(buf->pool->validate(buf->pool, buf->private, &buf->mutex));
	pipe_mutex_unlock(buf->mutex);
	curBuf = drmBOListNext(&list->driBuffers, curBuf);
    }
}


void
driPoolTakeDown(struct _DriBufferPool *pool)
{
   pool->takeDown(pool);

}

unsigned long
driBOSize(struct _DriBufferObject *buf)
{
  unsigned long size;

   pipe_mutex_lock(buf->mutex);
   size = buf->pool->size(buf->pool, buf->private);
   pipe_mutex_unlock(buf->mutex);

  return size;

}

drmBOList *driBOGetDRMBuffers(struct _DriBufferList *list)
{
    return &list->drmBuffers;
}

drmBOList *driBOGetDRIBuffers(struct _DriBufferList *list)
{
    return &list->driBuffers;
}

