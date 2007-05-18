/*
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "mtypes.h"
#include "dri_bufmgr.h"

/** @file dri_bufmgr.c
 *
 * Convenience functions for buffer management methods.
 */

dri_bo *
dri_bo_alloc(dri_bufmgr *bufmgr, const char *name, unsigned long size,
	     unsigned int alignment, unsigned int flags, unsigned int hint)
{
   return bufmgr->bo_alloc(bufmgr, name, size, alignment, flags, hint);
}

dri_bo *
dri_bo_alloc_static(dri_bufmgr *bufmgr, const char *name, unsigned long offset,
		    unsigned long size, void *virtual, unsigned int flags,
		    unsigned int hint)
{
   return bufmgr->bo_alloc_static(bufmgr, name, offset, size, virtual,
				  flags, hint);
}

void
dri_bo_reference(dri_bo *bo)
{
   bo->bufmgr->bo_reference(bo);
}

void
dri_bo_unreference(dri_bo *bo)
{
   if (bo == NULL)
      return;

   bo->bufmgr->bo_unreference(bo);
}

int
dri_bo_map(dri_bo *buf, GLboolean write_enable)
{
   return buf->bufmgr->bo_map(buf, write_enable);
}

int
dri_bo_unmap(dri_bo *buf)
{
   return buf->bufmgr->bo_unmap(buf);
}

int
dri_bo_validate(dri_bo *buf, unsigned int flags)
{
   return buf->bufmgr->bo_validate(buf, flags);
}

dri_fence *
dri_fence_validated(dri_bufmgr *bufmgr, const char *name, GLboolean flushed)
{
   return bufmgr->fence_validated(bufmgr, name, flushed);
}

void
dri_fence_wait(dri_fence *fence)
{
   fence->bufmgr->fence_wait(fence);
}

void
dri_fence_reference(dri_fence *fence)
{
   fence->bufmgr->fence_reference(fence);
}

void
dri_fence_unreference(dri_fence *fence)
{
   if (fence == NULL)
      return;

   fence->bufmgr->fence_unreference(fence);
}

void
dri_bo_subdata(dri_bo *bo, unsigned long offset,
	       unsigned long size, const void *data)
{
   if (size == 0 || data == NULL)
      return;

   dri_bo_map(bo, GL_TRUE);
   memcpy((unsigned char *)bo->virtual + offset, data, size);
   dri_bo_unmap(bo);
}


void
dri_bo_get_subdata(dri_bo *bo, unsigned long offset,
		   unsigned long size, void *data)
{
   if (size == 0 || data == NULL)
      return;

   dri_bo_map(bo, GL_FALSE);
   memcpy(data, (unsigned char *)bo->virtual + offset, size);
   dri_bo_unmap(bo);
}
