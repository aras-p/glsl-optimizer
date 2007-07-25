/**************************************************************************
 * 
 * Copyright © 2007 Intel Corporation
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
 *	    Eric Anholt <eric@anholt.net>
 */

#include <xf86drm.h>
#include <stdlib.h>
#include <unistd.h>
#include "glthread.h"
#include "errno.h"
#include "mtypes.h"
#include "dri_bufmgr.h"
#include "string.h"
#include "imports.h"

typedef struct _dri_bufmgr_ttm {
   dri_bufmgr bufmgr;

   int fd;
   _glthread_Mutex mutex;
   unsigned int fence_type;
   unsigned int fence_type_flush;
} dri_bufmgr_ttm;

typedef struct _dri_bo_ttm {
   dri_bo bo;

   int refcount;		/* Protected by bufmgr->mutex */
   drmBO drm_bo;
   const char *name;
} dri_bo_ttm;

typedef struct _dri_fence_ttm
{
   dri_fence fence;

   int refcount;		/* Protected by bufmgr->mutex */
   /** Fence type from when the fence was created, used for later waits */
   unsigned int type;
   const char *name;
   drmFence drm_fence;
} dri_fence_ttm;

#if 0
int
driFenceSignaled(DriFenceObject * fence, unsigned type)
{
   int signaled;
   int ret;

   if (fence == NULL)
      return GL_TRUE;

   _glthread_LOCK_MUTEX(fence->mutex);
   ret = drmFenceSignaled(bufmgr_ttm->fd, &fence->fence, type, &signaled);
   _glthread_UNLOCK_MUTEX(fence->mutex);
   BM_CKFATAL(ret);
   return signaled;
}
#endif

static dri_bo *
dri_ttm_alloc(dri_bufmgr *bufmgr, const char *name,
	      unsigned long size, unsigned int alignment,
	      unsigned int location_mask)
{
   dri_bufmgr_ttm *ttm_bufmgr;
   dri_bo_ttm *ttm_buf;
   unsigned int pageSize = getpagesize();
   int ret;
   unsigned int flags, hint;

   ttm_bufmgr = (dri_bufmgr_ttm *)bufmgr;

   ttm_buf = malloc(sizeof(*ttm_buf));
   if (!ttm_buf)
      return NULL;

   /* The mask argument doesn't do anything for us that we want other than
    * determine which pool (TTM or local) the buffer is allocated into, so just
    * pass all of the allocation class flags.
    */
   flags = location_mask | DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE |
      DRM_BO_FLAG_EXE;
   /* No hints we want to use. */
   hint = 0;

   ret = drmBOCreate(ttm_bufmgr->fd, 0, size, alignment / pageSize,
		     NULL, drm_bo_type_dc,
                     flags, hint, &ttm_buf->drm_bo);
   if (ret != 0) {
      free(ttm_buf);
      return NULL;
   }
   ttm_buf->bo.size = ttm_buf->drm_bo.size;
   ttm_buf->bo.offset = ttm_buf->drm_bo.offset;
   ttm_buf->bo.virtual = NULL;
   ttm_buf->bo.bufmgr = bufmgr;
   ttm_buf->name = name;
   ttm_buf->refcount = 1;

   return &ttm_buf->bo;
}

static dri_bo *
dri_ttm_alloc_static(dri_bufmgr *bufmgr, const char *name,
		     unsigned long offset, unsigned long size, void *virtual,
		     unsigned int location_mask)
{
   dri_bufmgr_ttm *ttm_bufmgr;
   dri_bo_ttm *ttm_buf;
   int ret;
   unsigned int flags, hint;

   ttm_bufmgr = (dri_bufmgr_ttm *)bufmgr;

   ttm_buf = malloc(sizeof(*ttm_buf));
   if (!ttm_buf)
      return NULL;

   /* The mask argument doesn't do anything for us that we want other than
    * determine which pool (TTM or local) the buffer is allocated into, so just
    * pass all of the allocation class flags.
    */
   flags = location_mask | DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE |
      DRM_BO_FLAG_EXE | DRM_BO_FLAG_NO_MOVE;
   /* No hints we want to use. */
   hint = 0;

   ret = drmBOCreate(ttm_bufmgr->fd, offset, size, 0,
		     NULL, drm_bo_type_fake,
                     flags, hint, &ttm_buf->drm_bo);
   if (ret != 0) {
      free(ttm_buf);
      return NULL;
   }
   ttm_buf->bo.size = ttm_buf->drm_bo.size;
   ttm_buf->bo.offset = ttm_buf->drm_bo.offset;
   ttm_buf->bo.virtual = virtual;
   ttm_buf->bo.bufmgr = bufmgr;
   ttm_buf->name = name;
   ttm_buf->refcount = 1;

   return &ttm_buf->bo;
}

static void
dri_ttm_bo_reference(dri_bo *buf)
{
   dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;
   dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;

   _glthread_LOCK_MUTEX(bufmgr_ttm->mutex);
   ttm_buf->refcount++;
   _glthread_UNLOCK_MUTEX(bufmgr_ttm->mutex);
}

static void
dri_ttm_bo_unreference(dri_bo *buf)
{
   dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;
   dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;

   if (!buf)
      return;

   _glthread_LOCK_MUTEX(bufmgr_ttm->mutex);
   if (--ttm_buf->refcount == 0) {
      drmBOUnReference(bufmgr_ttm->fd, &ttm_buf->drm_bo);
      _glthread_UNLOCK_MUTEX(bufmgr_ttm->mutex);
      free(buf);
      return;
   }
   _glthread_UNLOCK_MUTEX(bufmgr_ttm->mutex);
}

static int
dri_ttm_bo_map(dri_bo *buf, GLboolean write_enable)
{
   dri_bufmgr_ttm *bufmgr_ttm;
   dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;
   unsigned int flags;

   bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;

   flags = DRM_BO_FLAG_READ;
   if (write_enable)
       flags |= DRM_BO_FLAG_WRITE;

   return drmBOMap(bufmgr_ttm->fd, &ttm_buf->drm_bo, flags, 0, &buf->virtual);
}

static int
dri_ttm_bo_unmap(dri_bo *buf)
{
   dri_bufmgr_ttm *bufmgr_ttm;
   dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;

   if (buf == NULL)
      return 0;

   bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;

   buf->virtual = NULL;

   return drmBOUnmap(bufmgr_ttm->fd, &ttm_buf->drm_bo);
}

static int
dri_ttm_validate(dri_bo *buf, unsigned int flags)
{
   dri_bufmgr_ttm *bufmgr_ttm;
   dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;
   unsigned int mask;
   int err;

   /* XXX: Sanity-check whether we've already validated this one under
    * different flags.  See drmAddValidateItem().
    */

   bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;

   /* Calculate the appropriate mask to pass to the DRM. There appears to be
    * be a direct relationship to flags, so it's unnecessary to have it passed
    * in as an argument.
    */
   mask = DRM_BO_MASK_MEM;
   mask |= flags & (DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE | DRM_BO_FLAG_EXE);

   err = drmBOValidate(bufmgr_ttm->fd, &ttm_buf->drm_bo, flags, mask, 0);

   if (err == 0) {
      /* XXX: add to fence list for sanity checking */
   }

   return err;
}

static dri_fence *
dri_ttm_fence_validated(dri_bufmgr *bufmgr, const char *name,
			GLboolean flushed)
{
   dri_fence_ttm *fence_ttm = malloc(sizeof(*fence_ttm));
   dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;
   int ret;
   unsigned int type;

   if (!fence_ttm)
      return NULL;

   if (flushed)
      type = bufmgr_ttm->fence_type_flush;
   else
      type = bufmgr_ttm->fence_type;

   fence_ttm->refcount = 1;
   fence_ttm->name = name;
   fence_ttm->type = type;
   fence_ttm->fence.bufmgr = bufmgr;
   ret = drmFenceBuffers(bufmgr_ttm->fd, type, &fence_ttm->drm_fence);
   if (ret) {
      free(fence_ttm);
      return NULL;
   }
   return &fence_ttm->fence;
}

static void
dri_ttm_fence_reference(dri_fence *fence)
{
   dri_fence_ttm *fence_ttm = (dri_fence_ttm *)fence;
   dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)fence->bufmgr;

   _glthread_LOCK_MUTEX(bufmgr_ttm->mutex);
   ++fence_ttm->refcount;
   _glthread_UNLOCK_MUTEX(bufmgr_ttm->mutex);
}

static void
dri_ttm_fence_unreference(dri_fence *fence)
{
   dri_fence_ttm *fence_ttm = (dri_fence_ttm *)fence;
   dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)fence->bufmgr;

   if (!fence)
      return;

   _glthread_LOCK_MUTEX(bufmgr_ttm->mutex);
   if (--fence_ttm->refcount == 0) {
      drmFenceDestroy(bufmgr_ttm->fd, &fence_ttm->drm_fence);
      _glthread_UNLOCK_MUTEX(bufmgr_ttm->mutex);
      free(fence);
      return;
   }
   _glthread_UNLOCK_MUTEX(bufmgr_ttm->mutex);
}

static void
dri_ttm_fence_wait(dri_fence *fence)
{
   dri_fence_ttm *fence_ttm = (dri_fence_ttm *)fence;
   dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)fence->bufmgr;
   int ret;

   _glthread_LOCK_MUTEX(bufmgr_ttm->mutex);
   ret = drmFenceWait(bufmgr_ttm->fd, 0, &fence_ttm->drm_fence,
		      fence_ttm->type);
   _glthread_UNLOCK_MUTEX(bufmgr_ttm->mutex);
   if (ret != 0) {
      _mesa_printf("%s:%d: Error %d waiting for fence %s.\n",
		   __FILE__, __LINE__, fence_ttm->name);
      abort();
   }
}

static void
dri_bufmgr_ttm_destroy(dri_bufmgr *bufmgr)
{
   dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;

   _glthread_DESTROY_MUTEX(bufmgr_ttm->mutex);
   free(bufmgr);
}

/**
 * Initializes the TTM buffer manager, which uses the kernel to allocate, map,
 * and manage map buffer objections.
 *
 * \param fd File descriptor of the opened DRM device.
 * \param fence_type Driver-specific fence type used for fences with no flush.
 * \param fence_type_flush Driver-specific fence type used for fences with a
 *	  flush.
 */
dri_bufmgr *
dri_bufmgr_ttm_init(int fd, unsigned int fence_type,
		    unsigned int fence_type_flush)
{
   dri_bufmgr_ttm *bufmgr_ttm;
   dri_bo *test_alloc;

   bufmgr_ttm = malloc(sizeof(*bufmgr_ttm));
   bufmgr_ttm->fd = fd;
   bufmgr_ttm->fence_type = fence_type;
   bufmgr_ttm->fence_type_flush = fence_type_flush;
   _glthread_INIT_MUTEX(bufmgr_ttm->mutex);

   bufmgr_ttm->bufmgr.bo_alloc = dri_ttm_alloc;
   bufmgr_ttm->bufmgr.bo_alloc_static = dri_ttm_alloc_static;
   bufmgr_ttm->bufmgr.bo_reference = dri_ttm_bo_reference;
   bufmgr_ttm->bufmgr.bo_unreference = dri_ttm_bo_unreference;
   bufmgr_ttm->bufmgr.bo_map = dri_ttm_bo_map;
   bufmgr_ttm->bufmgr.bo_unmap = dri_ttm_bo_unmap;
   bufmgr_ttm->bufmgr.bo_validate = dri_ttm_validate;
   bufmgr_ttm->bufmgr.fence_validated = dri_ttm_fence_validated;
   bufmgr_ttm->bufmgr.fence_reference = dri_ttm_fence_reference;
   bufmgr_ttm->bufmgr.fence_unreference = dri_ttm_fence_unreference;
   bufmgr_ttm->bufmgr.fence_wait = dri_ttm_fence_wait;
   bufmgr_ttm->bufmgr.destroy = dri_bufmgr_ttm_destroy;

   /* Attempt an allocation to make sure that the DRM was actually set up for
    * TTM.
    */
   test_alloc = dri_bo_alloc((dri_bufmgr *)bufmgr_ttm, "test allocation",
     4096, 4096, DRM_BO_FLAG_MEM_LOCAL | DRM_BO_FLAG_MEM_TT);
   if (test_alloc == NULL) {
      fprintf(stderr, "TTM test allocation failed\n");
      _glthread_DESTROY_MUTEX(bufmgr_ttm->mutex);
      free(bufmgr_ttm);
      return NULL;
   }
   dri_bo_unreference(test_alloc);

   return &bufmgr_ttm->bufmgr;
}
