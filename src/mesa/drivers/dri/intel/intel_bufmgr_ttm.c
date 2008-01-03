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
#include <stdlib.h>
#include <unistd.h>
#include "glthread.h"
#include "errno.h"
#include "mtypes.h"
#include "dri_bufmgr.h"
#include "string.h"
#include "imports.h"

#include "i915_drm.h"

#include "intel_bufmgr_ttm.h"

#define DBG(...) do {					\
   if (bufmgr_ttm->bufmgr.debug)			\
      _mesa_printf(__VA_ARGS__);			\
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

/* Buffer validation list */
struct intel_bo_list {
    unsigned numCurrent;
    drmMMListHead list;
};

typedef struct _dri_bufmgr_ttm {
    dri_bufmgr bufmgr;

    int fd;
    unsigned int fence_type;
    unsigned int fence_type_flush;

    uint32_t max_relocs;
    struct intel_bo_list list; /* list of buffers to be validated */
} dri_bufmgr_ttm;

/**
 * Private information associated with a relocation that isn't already stored
 * in the relocation buffer to be passed to the kernel.
 */
struct dri_ttm_reloc {
    dri_bo *target_buf;
    uint64_t validate_flags;
};

typedef struct _dri_bo_ttm {
    dri_bo bo;

    int refcount;
    drmBO drm_bo;
    const char *name;

    /* Index of the buffer within the validation list while preparing a
     * batchbuffer execution.
     */
    int validate_index;

    /** DRM buffer object containing relocation list */
    drmBO *reloc_buf;
    uint32_t *reloc_buf_data;
    struct dri_ttm_reloc *relocs;
} dri_bo_ttm;

typedef struct _dri_fence_ttm
{
    dri_fence fence;

    int refcount;
    const char *name;
    drmFence drm_fence;
} dri_fence_ttm;

/* Validation list node */
struct intel_bo_node
{
    drmMMListHead head;
    dri_bo *bo;
    struct drm_i915_op_arg bo_arg;
    uint64_t flags;
};

static void
intel_init_validate_list(struct intel_bo_list *list)
{
    DRMINITLISTHEAD(&list->list);
    list->numCurrent = 0;
}

/**
 * Empties the validation list and clears the relocations 
 */
static void
intel_free_validate_list(dri_bufmgr_ttm *bufmgr_ttm)
{
    struct intel_bo_list *list = &bufmgr_ttm->list;
    drmMMListHead *l;

    for (l = list->list.next; l != &list->list; l = list->list.next) {
        struct intel_bo_node *node =
	   DRMLISTENTRY(struct intel_bo_node, l, head);

	DRMLISTDEL(l);

	dri_bo_unreference(node->bo);

	drmFree(node);
	list->numCurrent--;
    }
}

static void dri_ttm_dump_validation_list(dri_bufmgr_ttm *bufmgr_ttm)
{
    struct intel_bo_list *list = &bufmgr_ttm->list;
    drmMMListHead *l;
    int i = 0;

    for (l = list->list.next; l != &list->list; l = l->next) {
	int j;
        struct intel_bo_node *node =
	    DRMLISTENTRY(struct intel_bo_node, l, head);
	dri_bo_ttm *bo_ttm = (dri_bo_ttm *)node->bo;

	if (bo_ttm->reloc_buf_data != NULL) {
	    for (j = 0; j < (bo_ttm->reloc_buf_data[0] & 0xffff); j++) {
		uint32_t *reloc_entry = bo_ttm->reloc_buf_data +
		    I915_RELOC_HEADER +
		    j * I915_RELOC0_STRIDE;

		DBG("%2d: %s@0x%08x -> %d + 0x%08x\n",
		    i, bo_ttm->name,
		    reloc_entry[0], reloc_entry[2], reloc_entry[1]);
	    }
	} else {
	    DBG("%2d: %s\n", i, bo_ttm->name);
	}
	i++;
    }
}

static struct drm_i915_op_arg *
intel_setup_validate_list(dri_bufmgr_ttm *bufmgr_ttm, GLuint *count_p)
{
    struct intel_bo_list *list = &bufmgr_ttm->list;
    drmMMListHead *l;
    struct drm_i915_op_arg *first;
    uint64_t *prevNext = NULL;
    GLuint count = 0;

    first = NULL;

    for (l = list->list.next; l != &list->list; l = l->next) {
        struct intel_bo_node *node =
	    DRMLISTENTRY(struct intel_bo_node, l, head);
	dri_bo_ttm *ttm_buf = (dri_bo_ttm *)node->bo;
	struct drm_i915_op_arg *arg = &node->bo_arg;
	struct drm_bo_op_req *req = &arg->d.req;

        if (!first)
            first = arg;

	if (prevNext)
	    *prevNext = (unsigned long) arg;

	memset(arg, 0, sizeof(*arg));
	prevNext = &arg->next;
	req->bo_req.handle = ttm_buf->drm_bo.handle;
	req->op = drm_bo_validate;
	req->bo_req.flags = node->flags;
	req->bo_req.hint = 0;
#ifdef DRM_BO_HINT_PRESUMED_OFFSET
	req->bo_req.hint |= DRM_BO_HINT_PRESUMED_OFFSET;
	req->bo_req.presumed_offset = node->bo->offset;
#endif
	req->bo_req.mask = INTEL_BO_MASK;
	req->bo_req.fence_class = 0; /* Backwards compat. */

	if (ttm_buf->reloc_buf != NULL)
	    arg->reloc_handle = ttm_buf->reloc_buf->handle;
	else
	    arg->reloc_handle = 0;

	count++;
    }

    if (!first)
	return 0;

    *count_p = count;
    return first;
}

/**
 * Adds the given buffer to the list of buffers to be validated (moved into the
 * appropriate memory type) with the next batch submission.
 *
 * If a buffer is validated multiple times in a batch submission, it ends up
 * with the intersection of the memory type flags and the union of the
 * access flags.
 */
static struct intel_bo_node *
intel_add_validate_buffer(dri_bo *buf,
			  uint64_t flags)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;
    struct intel_bo_list *list = &bufmgr_ttm->list;
    struct intel_bo_node *cur;
    dri_bo_ttm *ttm_buf = (dri_bo_ttm *)buf;
    drmMMListHead *l;
    int count = 0;
    int ret = 0;
    cur = NULL;

    /* Find the buffer in the validation list if it's already there. */
    for (l = list->list.next; l != &list->list; l = l->next) {
	struct intel_bo_node *node =
	    DRMLISTENTRY(struct intel_bo_node, l, head);

	if (((dri_bo_ttm *)node->bo)->drm_bo.handle == ttm_buf->drm_bo.handle) {
	    cur = node;
	    break;
	}
	count++;
    }

    if (!cur) {
	cur = drmMalloc(sizeof(*cur));
	if (!cur) {
	    return NULL;
	}
	cur->bo = buf;
	dri_bo_reference(buf);
	cur->flags = flags;
	ret = 1;

	DRMLISTADDTAIL(&cur->head, &list->list);
    } else {
	uint64_t memFlags = cur->flags & flags & DRM_BO_MASK_MEM;
	uint64_t modeFlags = (cur->flags | flags) & ~DRM_BO_MASK_MEM;

	if (memFlags == 0) {
	    fprintf(stderr,
		    "%s: No shared memory types between "
		    "0x%16llx and 0x%16llx\n",
		    __FUNCTION__, cur->flags, flags);
	    return NULL;
	}
	if (flags & ~INTEL_BO_MASK) {
	    fprintf(stderr,
		    "%s: Flags bits 0x%16llx are not supposed to be used in a relocation\n",
		    __FUNCTION__, flags & ~INTEL_BO_MASK);
	    return NULL;
	}
	cur->flags = memFlags | modeFlags;
    }

    ttm_buf->validate_index = count;

    return cur;
}


#define RELOC_BUF_SIZE(x) ((I915_RELOC_HEADER + x * I915_RELOC0_STRIDE) * \
	sizeof(uint32_t))

static int
intel_setup_reloc_list(dri_bo *bo)
{
    dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bo->bufmgr;
    int ret;

    /* If the buffer exists, then it was just created, or it was reintialized
     * at the last intel_free_validate_list().
     */
    if (bo_ttm->reloc_buf != NULL)
       return 0;

    bo_ttm->reloc_buf = malloc(sizeof(bo_ttm->drm_bo));
    bo_ttm->relocs = malloc(sizeof(struct dri_ttm_reloc) *
			    bufmgr_ttm->max_relocs);

    ret = drmBOCreate(bufmgr_ttm->fd,
		      RELOC_BUF_SIZE(bufmgr_ttm->max_relocs), 0,
		      NULL,
		      DRM_BO_FLAG_MEM_LOCAL |
		      DRM_BO_FLAG_READ |
		      DRM_BO_FLAG_WRITE |
		      DRM_BO_FLAG_MAPPABLE |
		      DRM_BO_FLAG_CACHED,
		      0, bo_ttm->reloc_buf);
    if (ret) {
       fprintf(stderr, "Failed to create relocation BO: %s\n",
	       strerror(-ret));
       return ret;
    }

    ret = drmBOMap(bufmgr_ttm->fd, bo_ttm->reloc_buf,
		   DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE,
		   0, (void **)&bo_ttm->reloc_buf_data);
    if (ret) {
       fprintf(stderr, "Failed to map relocation BO: %s\n",
	       strerror(-ret));
       return ret;
    }

    /* Initialize the relocation list with the header:
     * DWORD 0: relocation type, relocation count
     * DWORD 1: handle to next relocation list (currently none)
     * DWORD 2: unused
     * DWORD 3: unused
     */
    bo_ttm->reloc_buf_data[0] = I915_RELOC_TYPE_0 << 16;
    bo_ttm->reloc_buf_data[1] = 0;
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
    unsigned int flags, hint;

    ttm_buf = malloc(sizeof(*ttm_buf));
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

    ret = drmBOCreate(bufmgr_ttm->fd, size, alignment / pageSize,
		      NULL, flags, hint, &ttm_buf->drm_bo);
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
    ttm_buf->reloc_buf = NULL;
    ttm_buf->reloc_buf_data = NULL;
    ttm_buf->relocs = NULL;

    DBG("bo_create: %p (%s)\n", &ttm_buf->bo, ttm_buf->name);

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

    ttm_buf = malloc(sizeof(*ttm_buf));
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
    ttm_buf->reloc_buf = NULL;
    ttm_buf->reloc_buf_data = NULL;
    ttm_buf->relocs = NULL;

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
	int ret;

	if (ttm_buf->reloc_buf) {
	    int i;

	    /* Unreference all the target buffers */
	    for (i = 0; i < (ttm_buf->reloc_buf_data[0] & 0xffff); i++)
		 dri_bo_unreference(ttm_buf->relocs[i].target_buf);
	    free(ttm_buf->relocs);

	    /* Free the kernel BO containing relocation entries */
	    drmBOUnmap(bufmgr_ttm->fd, ttm_buf->reloc_buf);
	    drmBOUnreference(bufmgr_ttm->fd, ttm_buf->reloc_buf);
	    free(ttm_buf->reloc_buf);
	}

	ret = drmBOUnreference(bufmgr_ttm->fd, &ttm_buf->drm_bo);
	if (ret != 0) {
	    fprintf(stderr, "drmBOUnreference failed (%s): %s\n",
		    ttm_buf->name, strerror(-ret));
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
    unsigned int flags;

    bufmgr_ttm = (dri_bufmgr_ttm *)buf->bufmgr;

    flags = DRM_BO_FLAG_READ;
    if (write_enable)
	flags |= DRM_BO_FLAG_WRITE;

    assert(buf->virtual == NULL);

    DBG("bo_map: %p (%s)\n", &ttm_buf->bo, ttm_buf->name);

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

    assert(buf->virtual != NULL);

    buf->virtual = NULL;

    DBG("bo_unmap: %p (%s)\n", &ttm_buf->bo, ttm_buf->name);

    return drmBOUnmap(bufmgr_ttm->fd, &ttm_buf->drm_bo);
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
	_mesa_printf("%s:%d: Error %d waiting for fence %s.\n",
		     __FILE__, __LINE__, ret, fence_ttm->name);
	abort();
    }

    DBG("fence_wait: %p (%s)\n", &fence_ttm->fence, fence_ttm->name);
}

static void
dri_bufmgr_ttm_destroy(dri_bufmgr *bufmgr)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)bufmgr;

    intel_free_validate_list(bufmgr_ttm);

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
static void
dri_ttm_emit_reloc(dri_bo *reloc_buf, uint64_t flags, GLuint delta,
		   GLuint offset, dri_bo *target_buf)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)reloc_buf->bufmgr;
    dri_bo_ttm *reloc_buf_ttm = (dri_bo_ttm *)reloc_buf;
    int num_relocs;
    uint32_t *this_reloc;

    intel_setup_reloc_list(reloc_buf);

    num_relocs = (reloc_buf_ttm->reloc_buf_data[0] & 0xffff);

    /* Check overflow */
    assert((reloc_buf_ttm->reloc_buf_data[0] & 0xffff) <
	   bufmgr_ttm->max_relocs);

    this_reloc = reloc_buf_ttm->reloc_buf_data + I915_RELOC_HEADER +
	num_relocs * I915_RELOC0_STRIDE;

    this_reloc[0] = offset;
    this_reloc[1] = delta;
    this_reloc[2] = -1; /* To be filled in at exec time */
    this_reloc[3] = 0;

    reloc_buf_ttm->relocs[num_relocs].validate_flags = flags;
    reloc_buf_ttm->relocs[num_relocs].target_buf = target_buf;
    dri_bo_reference(target_buf);

    reloc_buf_ttm->reloc_buf_data[0]++; /* Increment relocation count */
    /* Check wraparound */
    assert((reloc_buf_ttm->reloc_buf_data[0] & 0xffff) != 0);
}

/**
 * Walk the tree of relocations rooted at BO and accumulate the list of
 * validations to be performed and update the relocation buffers with
 * index values into the validation list.
 */
static void
dri_ttm_bo_process_reloc(dri_bo *bo)
{
    dri_bo_ttm *bo_ttm = (dri_bo_ttm *)bo;
    unsigned int nr_relocs;
    int i;

    if (bo_ttm->reloc_buf_data == NULL)
	return;

    nr_relocs = bo_ttm->reloc_buf_data[0] & 0xffff;

    for (i = 0; i < nr_relocs; i++) {
	struct dri_ttm_reloc *r = &bo_ttm->relocs[i];
	dri_bo_ttm *target_ttm = (dri_bo_ttm *)r->target_buf;
	uint32_t *reloc_entry;

	/* Continue walking the tree depth-first. */
	dri_ttm_bo_process_reloc(r->target_buf);

	/* Add the target to the validate list */
	intel_add_validate_buffer(r->target_buf, r->validate_flags);

	/* Update the index of the target in the relocation entry */
	reloc_entry = bo_ttm->reloc_buf_data + I915_RELOC_HEADER +
	    i * I915_RELOC0_STRIDE;
	reloc_entry[2] = target_ttm->validate_index;
    }
}

static void *
dri_ttm_process_reloc(dri_bo *batch_buf, GLuint *count)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)batch_buf->bufmgr;
    void *ptr;

    /* Update indices and set up the validate list. */
    dri_ttm_bo_process_reloc(batch_buf);

    /* Add the batch buffer to the validation list.  There are no relocations
     * pointing to it.
     */
    intel_add_validate_buffer(batch_buf,
			      DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_EXE);

    ptr = intel_setup_validate_list(bufmgr_ttm, count);

    return ptr;
}

static void
intel_update_buffer_offsets (dri_bufmgr_ttm *bufmgr_ttm)
{
    struct intel_bo_list *list = &bufmgr_ttm->list;
    struct intel_bo_node *node;
    drmMMListHead *l;
    struct drm_i915_op_arg *arg;
    struct drm_bo_arg_rep *rep;
    
    for (l = list->list.next; l != &list->list; l = l->next) {
        node = DRMLISTENTRY(struct intel_bo_node, l, head);
	arg = &node->bo_arg;
	rep = &arg->d.rep;
	node->bo->offset = rep->bo_info.offset;
    }
}

static void
dri_ttm_post_submit(dri_bo *batch_buf, dri_fence **last_fence)
{
    dri_bufmgr_ttm *bufmgr_ttm = (dri_bufmgr_ttm *)batch_buf->bufmgr;

    intel_update_buffer_offsets (bufmgr_ttm);

    if (bufmgr_ttm->bufmgr.debug)
	dri_ttm_dump_validation_list(bufmgr_ttm);

    intel_free_validate_list(bufmgr_ttm);
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

    bufmgr_ttm = malloc(sizeof(*bufmgr_ttm));
    bufmgr_ttm->fd = fd;
    bufmgr_ttm->fence_type = fence_type;
    bufmgr_ttm->fence_type_flush = fence_type_flush;

    /* lets go with one relocation per every four dwords - purely heuristic */
    bufmgr_ttm->max_relocs = batch_size / sizeof(uint32_t) / 4;

    intel_init_validate_list(&bufmgr_ttm->list);

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

    return &bufmgr_ttm->bufmgr;
}

