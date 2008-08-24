/**************************************************************************
 *
 * Copyright © 2007 Red Hat Inc.
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
 *	    Dave Airlie <airlied@linux.ie>
 */

#include <xf86drm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "errno.h"
#include "mtypes.h"
#include "dri_bufmgr.h"
#include "string.h"
#include "imports.h"

#include "i915_drm.h"

#include "intel_bufmgr_ttm.h"
#ifdef TTM_API

#define DBG(...) do {					\
   if (bufmgr_ttm->bufmgr.debug)			\
      fprintf(stderr, __VA_ARGS__);			\
} while (0)

/*
 * These bits are always specified in each validation
 * request. Other bits are not supported at this point
 * as it would require a bit of investigation to figure
 * out what mask value should be used.
 */
#define INTEL_BO_MASK  (DRM_BO_MASK_MEM | \
			DRM_BO_FLAG_READ | \
			DRM_BO_FLAG_WRITE | \
			DRM_BO_FLAG_EXE)

struct intel_validate_entry {
    dri_bo *bo;
    struct drm_i915_op_arg bo_arg;
};

struct dri_ttm_bo_bucket_entry {
   drmBO drm_bo;
   struct dri_ttm_bo_bucket_entry *next;
};

struct dri_ttm_bo_bucket {
   struct dri_ttm_bo_bucket_entry *head;
   struct dri_ttm_bo_bucket_entry **tail;
   /**
    * Limit on the number of entries in this bucket.
    *
    * 0 means that this caching at this bucket size is disabled.
    * -1 means that there is no limit to caching at this size.
    */
   int max_entries;
   int num_entries;
};

/* Arbitrarily chosen, 16 means that the maximum size we'll cache for reuse
 * is 1 << 16 pages, or 256MB.
 */
#define INTEL_TTM_BO_BUCKETS	16
typedef struct _dri_bufmgr_ttm {
    dri_bufmgr bufmgr;

    int fd;
    unsigned int fence_type;
    unsigned int fence_type_flush;

    uint32_t max_relocs;

    struct intel_validate_entry *validate_array;
    int validate_array_size;
    int validate_count;

    /** Array of lists of cached drmBOs of power-of-two sizes */
    struct dri_ttm_bo_bucket cache_bucket[INTEL_TTM_BO_BUCKETS];
} dri_bufmgr_ttm;

/**
 * Private information associated with a relocation that isn't already stored
 * in the relocation buffer to be passed to the kernel.
 */
struct dri_ttm_reloc {
    dri_bo *target_buf;
    uint64_t validate_flags;
    /** Offset of target_buf after last execution of this relocation entry. */
    unsigned int last_target_offset;
};

typedef struct _dri_bo_ttm {
    dri_bo bo;

    int refcount;
    unsigned int map_count;
    drmBO drm_bo;
    const char *name;

    uint64_t last_flags;

    /**
     * Index of the buffer within the validation list while preparing a
     * batchbuffer execution.
     */
    int validate_index;

    /** DRM buffer object containing relocation list */
    uint32_t *reloc_buf_data;
    struct dri_ttm_reloc *relocs;

    /**
     * Indicates that the buffer may be shared with other processes, so we
     * can't hold maps beyond when the user does.
     */
    GLboolean shared;

    GLboolean delayed_unmap;
    /* Virtual address from the dri_bo_map whose unmap was delayed. */
    void *saved_virtual;
} dri_bo_ttm;

typedef struct _dri_fence_ttm
{
    dri_fence fence;

    int refcount;
    const char *name;
    drmFence drm_fence;
} dri_fence_ttm;

static int
logbase2(int n)
{
   GLint i = 1;
   GLint log2 = 0;

   while (n > i) {
      i *= 2;
      log2++;
   }

   return log2;
}

static struct dri_ttm_bo_bucket *
dri_ttm_bo_bucket_for_size(dri_bufmgr_ttm *bufmgr_ttm, unsigned long size)
{
    int i;

    /* We only do buckets in power of two increments */
    if ((size & (size - 1)) != 0)
	return NULL;

    /* We should only see sizes rounded to pages. */
    assert((size % 4096) == 0);

    /* We always allocate in units of pages */
    i = ffs(size / 4096) - 1;
    if (i >= INTEL_TTM_BO_BUCKETS)
	return NULL;

    return &bufmgr_ttm->cache_bucket[i];
}


static void dri_ttm_dump_validation_list(dri_bufmgr_ttm *bufmgr_ttm)
{
    int i, j;

    for (i = 0; i < bufmgr_ttm->validate_count; i++) {
	dri_bo *bo = bufmgr_ttm->validate_array[i].bo;
	dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;

	if (bo_ttm->reloc_buf_data != NULL) {
	    for (j = 0; j < (bo_ttm->reloc_buf_data[0] & 0xffff); j++) {
		uint32_t *reloc_entry = bo_ttm->reloc_buf_data +
		    I915_RELOC_HEADER +
		    j * I915_RELOC0_STRIDE;
		dri_bo *target_bo = bo_ttm->relocs[j].target_buf;
		dri_bo_ttm *target_ttm = (dri_bo_ttm *)target_bo;

		DBG("%2d: %s@0x%08x -> %s@0x%08lx + 0x%08x\n",
		    i,
		    bo_ttm->name, reloc_entry[0],
		    target_ttm->name, target_bo->offset,
		    reloc_entry[1]);
	    }
	} else {
	    DBG("%2d: %s\n", i, bo_ttm->name);
	}
    }
}

/**
 * Adds the given buffer to the list of buffers to be validated (moved into the
 * appropriate memory type) with the next batch submission.
 *
 * If a buffer is validated multiple times in a batch submission, it ends up
 * with the intersection of the memory type flags and the union of the
 * access flags.
 */
static void
intel_add_validate_buffer(dri_bo *buf,
			  uint64_t flags)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;
    dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;

    /* If we delayed doing an unmap to mitigate map/unmap syscall thrashing,
     * do that now.
     */
    if (ttm_buf->delayed_unmap) {
	drmBOUnmap(bufmgr_ttm->fd, &ttm_buf->drm_bo);
	ttm_buf->delayed_unmap = GL_FALSE;
    }

    if (ttm_buf->validate_index == -1) {
	struct intel_validate_entry *entry;
	struct drm_i915_op_arg *arg;
	struct drm_bo_op_req *req;
	int index;

	/* Extend the array of validation entries as necessary. */
	if (bufmgr_ttm->validate_count == bufmgr_ttm->validate_array_size) {
	    int i, new_size = bufmgr_ttm->validate_array_size * 2;

	    if (new_size == 0)
		new_size = 5;

	    bufmgr_ttm->validate_array =
	       realloc(bufmgr_ttm->validate_array,
		       sizeof(struct intel_validate_entry) * new_size);
	    bufmgr_ttm->validate_array_size = new_size;

	    /* Update pointers for realloced mem. */
	    for (i = 0; i < bufmgr_ttm->validate_count - 1; i++) {
	       bufmgr_ttm->validate_array[i].bo_arg.next = (unsigned long)
		  &bufmgr_ttm->validate_array[i + 1].bo_arg;
	    }
	}

	/* Pick out the new array entry for ourselves */
	index = bufmgr_ttm->validate_count;
	ttm_buf->validate_index = index;
	entry = &bufmgr_ttm->validate_array[index];
	bufmgr_ttm->validate_count++;

	/* Fill in array entry */
	entry->bo = buf;
	dri_bo_reference(buf);

	/* Fill in kernel arg */
	arg = &entry->bo_arg;
	req = &arg->d.req;

	memset(arg, 0, sizeof(*arg));
	req->bo_req.handle = ttm_buf->drm_bo.handle;
	req->op = drm_bo_validate;
	req->bo_req.flags = flags;
	req->bo_req.hint = 0;
#ifdef DRM_BO_HINT_PRESUMED_OFFSET
	/* PRESUMED_OFFSET indicates that all relocations pointing at this
	 * buffer have the correct offset.  If any of our relocations don't,
	 * this flag will be cleared off the buffer later in the relocation
	 * processing.
	 */
	req->bo_req.hint |= DRM_BO_HINT_PRESUMED_OFFSET;
	req->bo_req.presumed_offset = buf->offset;
#endif
	req->bo_req.mask = INTEL_BO_MASK;
	req->bo_req.fence_class = 0; /* Backwards compat. */

	if (ttm_buf->reloc_buf_data != NULL)
 	    arg->reloc_ptr = (unsigned long)(void *)ttm_buf->reloc_buf_data;
	else
	    arg->reloc_ptr = 0;

	/* Hook up the linked list of args for the kernel */
	arg->next = 0;
	if (index != 0) {
	    bufmgr_ttm->validate_array[index - 1].bo_arg.next =
		(unsigned long)arg;
	}
    } else {
	struct intel_validate_entry *entry =
	    &bufmgr_ttm->validate_array[ttm_buf->validate_index];
	struct drm_i915_op_arg *arg = &entry->bo_arg;
	struct drm_bo_op_req *req = &arg->d.req;
	uint64_t memFlags = req->bo_req.flags & flags & DRM_BO_MASK_MEM;
	uint64_t modeFlags = (req->bo_req.flags | flags) & ~DRM_BO_MASK_MEM;

	/* Buffer was already in the validate list.  Extend its flags as
	 * necessary.
	 */

	if (memFlags == 0) {
	    fprintf(stderr,
		    "%s: No shared memory types between "
		    "0x%16llx and 0x%16llx\n",
		    __FUNCTION__, req->bo_req.flags, flags);
	    abort();
	}
	if (flags & ~INTEL_BO_MASK) {
	    fprintf(stderr,
		    "%s: Flags bits 0x%16llx are not supposed to be used in a relocation\n",
		    __FUNCTION__, flags & ~INTEL_BO_MASK);
	    abort();
	}
	req->bo_req.flags = memFlags | modeFlags;
    }
}


#define RELOC_BUF_SIZE(x) ((I915_RELOC_HEADER + x * I915_RELOC0_STRIDE) * \
	sizeof(uint32_t))

static int
intel_setup_reloc_list(dri_bo *bo)
{
    dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bo->bufmgr;

    bo_ttm->relocs = calloc(bufmgr_ttm->max_relocs,
			    sizeof(struct dri_ttm_reloc));
    bo_ttm->reloc_buf_data = calloc(1, RELOC_BUF_SIZE(bufmgr_ttm->max_relocs));

    /* Initialize the relocation list with the header:
     * DWORD 0: relocation count
     * DWORD 1: relocation type  
     * DWORD 2+3: handle to next relocation list (currently none) 64-bits
     */
    bo_ttm->reloc_buf_data[0] = 0;
    bo_ttm->reloc_buf_data[1] = I915_RELOC_TYPE_0;
    bo_ttm->reloc_buf_data[2] = 0;
    bo_ttm->reloc_buf_data[3] = 0;

    return 0;
}

#if 0
int
driFenceSignaled(DriFenceObject * fence, unsigned type)
{
    int signaled;
    int ret;

    if (fence == NULL)
	return GL_TRUE;

    ret = drmFenceSignaled(bufmgr_ttm->fd, &fence->fence, type, &signaled);
    BM_CKFATAL(ret);
    return signaled;
}
#endif

static dri_bo *
dri_ttm_alloc(dri_bufmgr *bufmgr, const char *name,
	      unsigned long size, unsigned int alignment,
	      uint64_t location_mask)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;
    dri_bo_ttm *ttm_buf;
    unsigned int pageSize = getpagesize();
    int ret;
    uint64_t flags;
    unsigned int hint;
    unsigned long alloc_size;
    struct dri_ttm_bo_bucket *bucket;
    GLboolean alloc_from_cache = GL_FALSE;

    ttm_buf = calloc(1, sizeof(*ttm_buf));
    if (!ttm_buf)
	return NULL;

    /* The mask argument doesn't do anything for us that we want other than
     * determine which pool (TTM or local) the buffer is allocated into, so
     * just pass all of the allocation class flags.
     */
    flags = location_mask | DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE |
	DRM_BO_FLAG_EXE;
    /* No hints we want to use. */
    hint = 0;

    /* Round the allocated size up to a power of two number of pages. */
    alloc_size = 1 << logbase2(size);
    if (alloc_size < pageSize)
	alloc_size = pageSize;
    bucket = dri_ttm_bo_bucket_for_size(bufmgr_ttm, alloc_size);

    /* If we don't have caching at this size, don't actually round the
     * allocation up.
     */
    if (bucket == NULL || bucket->max_entries == 0)
	alloc_size = size;

    /* Get a buffer out of the cache if available */
    if (bucket != NULL && bucket->num_entries > 0) {
	struct dri_ttm_bo_bucket_entry *entry = bucket->head;
	int busy;

	/* Check if the buffer is still in flight.  If not, reuse it. */
	ret = drmBOBusy(bufmgr_ttm->fd, &entry->drm_bo, &busy);
	alloc_from_cache = (ret == 0 && busy == 0);

	if (alloc_from_cache) {
	    bucket->head = entry->next;
	    if (entry->next == NULL)
		bucket->tail = &bucket->head;
	    bucket->num_entries--;

	    ttm_buf->drm_bo = entry->drm_bo;
	    free(entry);
	}
    }

    if (!alloc_from_cache) {
	ret = drmBOCreate(bufmgr_ttm->fd, alloc_size, alignment / pageSize,
			  NULL, flags, hint, &ttm_buf->drm_bo);
	if (ret != 0) {
	    free(ttm_buf);
	    return NULL;
	}
    }

    ttm_buf->bo.size = size;
    ttm_buf->bo.offset = ttm_buf->drm_bo.offset;
    ttm_buf->bo.virtual = NULL;
    ttm_buf->bo.bufmgr = bufmgr;
    ttm_buf->name = name;
    ttm_buf->refcount = 1;
    ttm_buf->reloc_buf_data = NULL;
    ttm_buf->relocs = NULL;
    ttm_buf->last_flags = ttm_buf->drm_bo.flags;
    ttm_buf->shared = GL_FALSE;
    ttm_buf->delayed_unmap = GL_FALSE;
    ttm_buf->validate_index = -1;

    DBG("bo_create: %p (%s) %ldb\n", &ttm_buf->bo, ttm_buf->name, size);

    return &ttm_buf->bo;
}

/* Our TTM backend doesn't allow creation of static buffers, as that requires
 * privelege for the non-fake case, and the lock in the fake case where we were
 * working around the X Server not creating buffers and passing handles to us.
 */
static dri_bo *
dri_ttm_alloc_static(dri_bufmgr *bufmgr, const char *name,
		     unsigned long offset, unsigned long size, void *virtual,
		     uint64_t location_mask)
{
    return NULL;
}

/**
 * Returns a dri_bo wrapping the given buffer object handle.
 *
 * This can be used when one application needs to pass a buffer object
 * to another.
 */
dri_bo *
intel_ttm_bo_create_from_handle(dri_bufmgr *bufmgr, const char *name,
			      unsigned int handle)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;
    dri_bo_ttm *ttm_buf;
    int ret;

    ttm_buf = calloc(1, sizeof(*ttm_buf));
    if (!ttm_buf)
	return NULL;

    ret = drmBOReference(bufmgr_ttm->fd, handle, &ttm_buf->drm_bo);
    if (ret != 0) {
       fprintf(stderr, "Couldn't reference %s handle 0x%08x: %s\n",
	       name, handle, strerror(-ret));
	free(ttm_buf);
	return NULL;
    }
    ttm_buf->bo.size = ttm_buf->drm_bo.size;
    ttm_buf->bo.offset = ttm_buf->drm_bo.offset;
    ttm_buf->bo.virtual = NULL;
    ttm_buf->bo.bufmgr = bufmgr;
    ttm_buf->name = name;
    ttm_buf->refcount = 1;
    ttm_buf->reloc_buf_data = NULL;
    ttm_buf->relocs = NULL;
    ttm_buf->last_flags = ttm_buf->drm_bo.flags;
    ttm_buf->shared = GL_TRUE;
    ttm_buf->delayed_unmap = GL_FALSE;
    ttm_buf->validate_index = -1;

    DBG("bo_create_from_handle: %p %08x (%s)\n",
	&ttm_buf->bo, handle, ttm_buf->name);

    return &ttm_buf->bo;
}

static void
dri_ttm_bo_reference(dri_bo *buf)
{
    dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;

    ttm_buf->refcount++;
}

static void
dri_ttm_bo_unreference(dri_bo *buf)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;
    dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;

    if (!buf)
	return;

    if (--ttm_buf->refcount == 0) {
	struct dri_ttm_bo_bucket *bucket;
	int ret;

	assert(ttm_buf->map_count == 0);

	if (ttm_buf->reloc_buf_data) {
	    int i;

	    /* Unreference all the target buffers */
	    for (i = 0; i < (ttm_buf->reloc_buf_data[0] & 0xffff); i++)
		 dri_bo_unreference(ttm_buf->relocs[i].target_buf);
	    free(ttm_buf->relocs);

	    /* Free the kernel BO containing relocation entries */
	    free(ttm_buf->reloc_buf_data);
	    ttm_buf->reloc_buf_data = NULL;
	}

	if (ttm_buf->delayed_unmap) {
	    int ret = drmBOUnmap(bufmgr_ttm->fd, &ttm_buf->drm_bo);

	    if (ret != 0) {
		fprintf(stderr, "%s:%d: Error unmapping buffer %s: %s.\n",
			__FILE__, __LINE__, ttm_buf->name, strerror(-ret));
	   }
	}

	bucket = dri_ttm_bo_bucket_for_size(bufmgr_ttm, ttm_buf->drm_bo.size);
	/* Put the buffer into our internal cache for reuse if we can. */
	if (!ttm_buf->shared &&
	    bucket != NULL &&
	    (bucket->max_entries == -1 ||
	     (bucket->max_entries > 0 &&
	      bucket->num_entries < bucket->max_entries)))
	{
	    struct dri_ttm_bo_bucket_entry *entry;

	    entry = calloc(1, sizeof(*entry));
	    entry->drm_bo = ttm_buf->drm_bo;

	    entry->next = NULL;
	    *bucket->tail = entry;
	    bucket->tail = &entry->next;
	    bucket->num_entries++;
	} else {
	    /* Decrement the kernel refcount for the buffer. */
	    ret = drmBOUnreference(bufmgr_ttm->fd, &ttm_buf->drm_bo);
	    if (ret != 0) {
	       fprintf(stderr, "drmBOUnreference failed (%s): %s\n",
		       ttm_buf->name, strerror(-ret));
	    }
	}

	DBG("bo_unreference final: %p (%s)\n", &ttm_buf->bo, ttm_buf->name);

	free(buf);
	return;
    }
}

static int
dri_ttm_bo_map(dri_bo *buf, GLboolean write_enable)
{
    dri_bufmgr_ttm *bufmgr_ttm;
    dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;
    uint64_t flags;
    int ret;

    bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;

    flags = DRM_BO_FLAG_READ;
    if (write_enable)
	flags |= DRM_BO_FLAG_WRITE;

    /* Allow recursive mapping. Mesa may recursively map buffers with
     * nested display loops.
     */
    if (ttm_buf->map_count++ != 0)
	return 0;

    assert(buf->virtual == NULL);

    DBG("bo_map: %p (%s)\n", &ttm_buf->bo, ttm_buf->name);

    /* XXX: What about if we're upgrading from READ to WRITE? */
    if (ttm_buf->delayed_unmap) {
	buf->virtual = ttm_buf->saved_virtual;
	return 0;
    }

    ret = drmBOMap(bufmgr_ttm->fd, &ttm_buf->drm_bo, flags, 0, &buf->virtual);
    if (ret != 0) {
        fprintf(stderr, "%s:%d: Error mapping buffer %s: %s .\n",
		__FILE__, __LINE__, ttm_buf->name, strerror(-ret));
    }

    return ret;
}

static int
dri_ttm_bo_unmap(dri_bo *buf)
{
    dri_bufmgr_ttm *bufmgr_ttm;
    dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;
    int ret;

    if (buf == NULL)
	return 0;

    assert(ttm_buf->map_count != 0);
    if (--ttm_buf->map_count != 0)
	return 0;

    bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;

    assert(buf->virtual != NULL);

    DBG("bo_unmap: %p (%s)\n", &ttm_buf->bo, ttm_buf->name);

    if (!ttm_buf->shared) {
	ttm_buf->saved_virtual = buf->virtual;
	ttm_buf->delayed_unmap = GL_TRUE;
	buf->virtual = NULL;

	return 0;
    }

    buf->virtual = NULL;

    ret = drmBOUnmap(bufmgr_ttm->fd, &ttm_buf->drm_bo);
    if (ret != 0) {
        fprintf(stderr, "%s:%d: Error unmapping buffer %s: %s.\n",
		__FILE__, __LINE__, ttm_buf->name, strerror(-ret));
    }

    return ret;
}

/**
 * Returns a dri_bo wrapping the given buffer object handle.
 *
 * This can be used when one application needs to pass a buffer object
 * to another.
 */
dri_fence *
intel_ttm_fence_create_from_arg(dri_bufmgr *bufmgr, const char *name,
				drm_fence_arg_t *arg)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;
    dri_fence_ttm *ttm_fence;

    ttm_fence = malloc(sizeof(*ttm_fence));
    if (!ttm_fence)
	return NULL;

    ttm_fence->drm_fence.handle = arg->handle;
    ttm_fence->drm_fence.fence_class = arg->fence_class;
    ttm_fence->drm_fence.type = arg->type;
    ttm_fence->drm_fence.flags = arg->flags;
    ttm_fence->drm_fence.signaled = 0;
    ttm_fence->drm_fence.sequence = arg->sequence;

    ttm_fence->fence.bufmgr = bufmgr;
    ttm_fence->name = name;
    ttm_fence->refcount = 1;

    DBG("fence_create_from_handle: %p (%s)\n",
	&ttm_fence->fence, ttm_fence->name);

    return &ttm_fence->fence;
}


static void
dri_ttm_fence_reference(dri_fence *fence)
{
    dri_fence_ttm *fence_ttm = (dri_fence_ttm *)fence;
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)fence->bufmgr;

    ++fence_ttm->refcount;
    DBG("fence_reference: %p (%s)\n", &fence_ttm->fence, fence_ttm->name);
}

static void
dri_ttm_fence_unreference(dri_fence *fence)
{
    dri_fence_ttm *fence_ttm = (dri_fence_ttm *)fence;
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)fence->bufmgr;

    if (!fence)
	return;

    DBG("fence_unreference: %p (%s)\n", &fence_ttm->fence, fence_ttm->name);

    if (--fence_ttm->refcount == 0) {
	int ret;

	ret = drmFenceUnreference(bufmgr_ttm->fd, &fence_ttm->drm_fence);
	if (ret != 0) {
	    fprintf(stderr, "drmFenceUnreference failed (%s): %s\n",
		    fence_ttm->name, strerror(-ret));
	}

	free(fence);
	return;
    }
}

static void
dri_ttm_fence_wait(dri_fence *fence)
{
    dri_fence_ttm *fence_ttm = (dri_fence_ttm *)fence;
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)fence->bufmgr;
    int ret;

    ret = drmFenceWait(bufmgr_ttm->fd, DRM_FENCE_FLAG_WAIT_LAZY, &fence_ttm->drm_fence, 0);
    if (ret != 0) {
        fprintf(stderr, "%s:%d: Error waiting for fence %s: %s.\n",
		__FILE__, __LINE__, fence_ttm->name, strerror(-ret));
	abort();
    }

    DBG("fence_wait: %p (%s)\n", &fence_ttm->fence, fence_ttm->name);
}

static void
dri_bufmgr_ttm_destroy(dri_bufmgr *bufmgr)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;
    int i;

    free(bufmgr_ttm->validate_array);

    /* Free any cached buffer objects we were going to reuse */
    for (i = 0; i < INTEL_TTM_BO_BUCKETS; i++) {
	struct dri_ttm_bo_bucket *bucket = &bufmgr_ttm->cache_bucket[i];
	struct dri_ttm_bo_bucket_entry *entry;

	while ((entry = bucket->head) != NULL) {
	    int ret;

	    bucket->head = entry->next;
	    if (entry->next == NULL)
		bucket->tail = &bucket->head;
	    bucket->num_entries--;

	    /* Decrement the kernel refcount for the buffer. */
	    ret = drmBOUnreference(bufmgr_ttm->fd, &entry->drm_bo);
	    if (ret != 0) {
	       fprintf(stderr, "drmBOUnreference failed: %s\n",
		       strerror(-ret));
	    }

	    free(entry);
	}
    }

    free(bufmgr);
}

/**
 * Adds the target buffer to the validation list and adds the relocation
 * to the reloc_buffer's relocation list.
 *
 * The relocation entry at the given offset must already contain the
 * precomputed relocation value, because the kernel will optimize out
 * the relocation entry write when the buffer hasn't moved from the
 * last known offset in target_buf.
 */
static int
dri_ttm_emit_reloc(dri_bo *reloc_buf, uint64_t flags, GLuint delta,
		   GLuint offset, dri_bo *target_buf)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)reloc_buf->bufmgr;
    dri_bo_ttm *reloc_buf_ttm = (dri_bo_ttm *)reloc_buf;
    dri_bo_ttm *target_buf_ttm = (dri_bo_ttm *)target_buf;
    int num_relocs;
    uint32_t *this_reloc;

    /* Create a new relocation list if needed */
    if (reloc_buf_ttm->reloc_buf_data == NULL)
	intel_setup_reloc_list(reloc_buf);

    num_relocs = reloc_buf_ttm->reloc_buf_data[0];

    /* Check overflow */
    assert(num_relocs < bufmgr_ttm->max_relocs);

    this_reloc = reloc_buf_ttm->reloc_buf_data + I915_RELOC_HEADER +
	num_relocs * I915_RELOC0_STRIDE;

    this_reloc[0] = offset;
    this_reloc[1] = delta;
    this_reloc[2] = target_buf_ttm->drm_bo.handle; /* To be filled in at exec time */
    this_reloc[3] = 0;

    reloc_buf_ttm->relocs[num_relocs].validate_flags = flags;
    reloc_buf_ttm->relocs[num_relocs].target_buf = target_buf;
    dri_bo_reference(target_buf);

    reloc_buf_ttm->reloc_buf_data[0]++; /* Increment relocation count */
    /* Check wraparound */
    assert(reloc_buf_ttm->reloc_buf_data[0] != 0);
    return 0;
}

/**
 * Walk the tree of relocations rooted at BO and accumulate the list of
 * validations to be performed and update the relocation buffers with
 * index values into the validation list.
 */
static void
dri_ttm_bo_process_reloc(dri_bo *bo)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bo->bufmgr;
    dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;
    unsigned int nr_relocs;
    int i;

    if (bo_ttm->reloc_buf_data == NULL)
	return;

    nr_relocs = bo_ttm->reloc_buf_data[0] & 0xffff;

    for (i = 0; i < nr_relocs; i++) {
	struct dri_ttm_reloc *r = &bo_ttm->relocs[i];

	/* Continue walking the tree depth-first. */
	dri_ttm_bo_process_reloc(r->target_buf);

	/* Add the target to the validate list */
	intel_add_validate_buffer(r->target_buf, r->validate_flags);

	/* Clear the PRESUMED_OFFSET flag from the validate list entry of the
	 * target if this buffer has a stale relocated pointer at it.
	 */
	if (r->last_target_offset != r->target_buf->offset) {
	   dri_bo_ttm *target_buf_ttm = (dri_bo_ttm *)r->target_buf;
	   struct intel_validate_entry *entry =
	      &bufmgr_ttm->validate_array[target_buf_ttm->validate_index];

	   entry->bo_arg.d.req.bo_req.hint &= ~DRM_BO_HINT_PRESUMED_OFFSET;
	}
    }
}

static void *
dri_ttm_process_reloc(dri_bo *batch_buf, GLuint *count)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)batch_buf->bufmgr;

    /* Update indices and set up the validate list. */
    dri_ttm_bo_process_reloc(batch_buf);

    /* Add the batch buffer to the validation list.  There are no relocations
     * pointing to it.
     */
    intel_add_validate_buffer(batch_buf,
			      DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_EXE);

    *count = bufmgr_ttm->validate_count;
    return &bufmgr_ttm->validate_array[0].bo_arg;
}

static const char *
intel_get_flags_mem_type_string(uint64_t flags)
{
    switch (flags & DRM_BO_MASK_MEM) {
    case DRM_BO_FLAG_MEM_LOCAL: return "local";
    case DRM_BO_FLAG_MEM_TT: return "ttm";
    case DRM_BO_FLAG_MEM_VRAM: return "vram";
    case DRM_BO_FLAG_MEM_PRIV0: return "priv0";
    case DRM_BO_FLAG_MEM_PRIV1: return "priv1";
    case DRM_BO_FLAG_MEM_PRIV2: return "priv2";
    case DRM_BO_FLAG_MEM_PRIV3: return "priv3";
    case DRM_BO_FLAG_MEM_PRIV4: return "priv4";
    default: return NULL;
    }
}

static const char *
intel_get_flags_caching_string(uint64_t flags)
{
    switch (flags & (DRM_BO_FLAG_CACHED | DRM_BO_FLAG_CACHED_MAPPED)) {
    case 0: return "UU";
    case DRM_BO_FLAG_CACHED: return "CU";
    case DRM_BO_FLAG_CACHED_MAPPED: return "UC";
    case DRM_BO_FLAG_CACHED | DRM_BO_FLAG_CACHED_MAPPED: return "CC";
    default: return NULL;
    }
}

static void
intel_update_buffer_offsets (dri_bufmgr_ttm *bufmgr_ttm)
{
    int i;

    for (i = 0; i < bufmgr_ttm->validate_count; i++) {
	dri_bo *bo = bufmgr_ttm->validate_array[i].bo;
	dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;
	struct drm_i915_op_arg *arg = &bufmgr_ttm->validate_array[i].bo_arg;
	struct drm_bo_arg_rep *rep = &arg->d.rep;

	/* Update the flags */
	if (rep->bo_info.flags != bo_ttm->last_flags) {
	    DBG("BO %s migrated: %s/%s -> %s/%s\n",
		bo_ttm->name,
		intel_get_flags_mem_type_string(bo_ttm->last_flags),
		intel_get_flags_caching_string(bo_ttm->last_flags),
		intel_get_flags_mem_type_string(rep->bo_info.flags),
		intel_get_flags_caching_string(rep->bo_info.flags));

	    bo_ttm->last_flags = rep->bo_info.flags;
	}
	/* Update the buffer offset */
	if (rep->bo_info.offset != bo->offset) {
	    DBG("BO %s migrated: 0x%08lx -> 0x%08lx\n",
		bo_ttm->name, bo->offset, (unsigned long)rep->bo_info.offset);
	    bo->offset = rep->bo_info.offset;
	}
    }
}

/**
 * Update the last target offset field of relocation entries for PRESUMED_OFFSET
 * computation.
 */
static void
dri_ttm_bo_post_submit(dri_bo *bo)
{
    dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;
    unsigned int nr_relocs;
    int i;

    if (bo_ttm->reloc_buf_data == NULL)
	return;

    nr_relocs = bo_ttm->reloc_buf_data[0] & 0xffff;

    for (i = 0; i < nr_relocs; i++) {
	struct dri_ttm_reloc *r = &bo_ttm->relocs[i];

	/* Continue walking the tree depth-first. */
	dri_ttm_bo_post_submit(r->target_buf);

	r->last_target_offset = r->target_buf->offset;
    }
}

static void
dri_ttm_post_submit(dri_bo *batch_buf, dri_fence **last_fence)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)batch_buf->bufmgr;
    int i;

    intel_update_buffer_offsets (bufmgr_ttm);

    dri_ttm_bo_post_submit(batch_buf);

    if (bufmgr_ttm->bufmgr.debug)
	dri_ttm_dump_validation_list(bufmgr_ttm);

    for (i = 0; i < bufmgr_ttm->validate_count; i++) {
	dri_bo *bo = bufmgr_ttm->validate_array[i].bo;
	dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;

	/* Disconnect the buffer from the validate list */
	bo_ttm->validate_index = -1;
	dri_bo_unreference(bo);
	bufmgr_ttm->validate_array[i].bo = NULL;
    }
    bufmgr_ttm->validate_count = 0;
}

/**
 * Enables unlimited caching of buffer objects for reuse.
 *
 * This is potentially very memory expensive, as the cache at each bucket
 * size is only bounded by how many buffers of that size we've managed to have
 * in flight at once.
 */
void
intel_ttm_enable_bo_reuse(dri_bufmgr *bufmgr)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;
    int i;

    for (i = 0; i < INTEL_TTM_BO_BUCKETS; i++) {
	bufmgr_ttm->cache_bucket[i].max_entries = -1;
    }
}

/*
 *
 */
static int
dri_ttm_check_aperture_space(dri_bo *bo)
{
    return 0;
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
intel_bufmgr_ttm_init(int fd, unsigned int fence_type,
		      unsigned int fence_type_flush, int batch_size)
{
    dri_bufmgr_ttm *bufmgr_ttm;
    int i;

    bufmgr_ttm = calloc(1, sizeof(*bufmgr_ttm));
    bufmgr_ttm->fd = fd;
    bufmgr_ttm->fence_type = fence_type;
    bufmgr_ttm->fence_type_flush = fence_type_flush;

    /* Let's go with one relocation per every 2 dwords (but round down a bit
     * since a power of two will mean an extra page allocation for the reloc
     * buffer).
     *
     * Every 4 was too few for the blender benchmark.
     */
    bufmgr_ttm->max_relocs = batch_size / sizeof(uint32_t) / 2 - 2;

    bufmgr_ttm->bufmgr.bo_alloc = dri_ttm_alloc;
    bufmgr_ttm->bufmgr.bo_alloc_static = dri_ttm_alloc_static;
    bufmgr_ttm->bufmgr.bo_reference = dri_ttm_bo_reference;
    bufmgr_ttm->bufmgr.bo_unreference = dri_ttm_bo_unreference;
    bufmgr_ttm->bufmgr.bo_map = dri_ttm_bo_map;
    bufmgr_ttm->bufmgr.bo_unmap = dri_ttm_bo_unmap;
    bufmgr_ttm->bufmgr.fence_reference = dri_ttm_fence_reference;
    bufmgr_ttm->bufmgr.fence_unreference = dri_ttm_fence_unreference;
    bufmgr_ttm->bufmgr.fence_wait = dri_ttm_fence_wait;
    bufmgr_ttm->bufmgr.destroy = dri_bufmgr_ttm_destroy;
    bufmgr_ttm->bufmgr.emit_reloc = dri_ttm_emit_reloc;
    bufmgr_ttm->bufmgr.process_relocs = dri_ttm_process_reloc;
    bufmgr_ttm->bufmgr.post_submit = dri_ttm_post_submit;
    bufmgr_ttm->bufmgr.debug = GL_FALSE;
    bufmgr_ttm->bufmgr.check_aperture_space = dri_ttm_check_aperture_space;
    /* Initialize the linked lists for BO reuse cache. */
    for (i = 0; i < INTEL_TTM_BO_BUCKETS; i++)
	bufmgr_ttm->cache_bucket[i].tail = &bufmgr_ttm->cache_bucket[i].head;

    return &bufmgr_ttm->bufmgr;
}
#else
dri_bufmgr *
intel_bufmgr_ttm_init(int fd, unsigned int fence_type,
		      unsigned int fence_type_flush, int batch_size)
{
    return NULL;
}

dri_bo *
intel_ttm_bo_create_from_handle(dri_bufmgr *bufmgr, const char *name,
			      unsigned int handle)
{
    return NULL;
}

void
intel_ttm_enable_bo_reuse(dri_bufmgr *bufmgr)
{
}
#endif
