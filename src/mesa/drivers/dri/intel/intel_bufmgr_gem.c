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
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "errno.h"
#include "mtypes.h"
#include "dri_bufmgr.h"
#include "string.h"
#include "imports.h"

#include "i915_drm.h"

#include "intel_bufmgr_gem.h"

#define DBG(...) do {					\
   if (bufmgr_gem->bufmgr.debug)			\
      fprintf(stderr, __VA_ARGS__);			\
} while (0)

struct intel_validate_entry {
    dri_bo *bo;
    struct drm_i915_op_arg bo_arg;
};

struct dri_gem_bo_bucket_entry {
   uint32_t gem_handle;
   uint32_t last_offset;
   struct dri_gem_bo_bucket_entry *next;
};

struct dri_gem_bo_bucket {
   struct dri_gem_bo_bucket_entry *head;
   struct dri_gem_bo_bucket_entry **tail;
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
#define INTEL_GEM_BO_BUCKETS	16
typedef struct _dri_bufmgr_gem {
    dri_bufmgr bufmgr;

    int fd;

    uint32_t max_relocs;

    struct drm_i915_gem_exec_object *exec_objects;
    dri_bo **exec_bos;
    int exec_size;
    int exec_count;

    /** Array of lists of cached gem objects of power-of-two sizes */
    struct dri_gem_bo_bucket cache_bucket[INTEL_GEM_BO_BUCKETS];

    struct drm_i915_gem_execbuffer exec_arg;
} dri_bufmgr_gem;

typedef struct _dri_bo_gem {
    dri_bo bo;

    int refcount;
    GLboolean mapped;
    uint32_t gem_handle;
    const char *name;

    /**
     * Index of the buffer within the validation list while preparing a
     * batchbuffer execution.
     */
    int validate_index;

    /**
     * Tracks whether set_domain to CPU is current
     * Set when set_domain has been called
     * Cleared when a batch has been submitted
     */
    GLboolean cpu_domain_set;

    /** Array passed to the DRM containing relocation information. */
    struct drm_i915_gem_relocation_entry *relocs;
    /** Array of bos corresponding to relocs[i].target_handle */
    dri_bo **reloc_target_bo;
    /** Number of entries in relocs */
    int reloc_count;
    /** Mapped address for the buffer */
    void *virtual;
} dri_bo_gem;

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

static struct dri_gem_bo_bucket *
dri_gem_bo_bucket_for_size(dri_bufmgr_gem *bufmgr_gem, unsigned long size)
{
    int i;

    /* We only do buckets in power of two increments */
    if ((size & (size - 1)) != 0)
	return NULL;

    /* We should only see sizes rounded to pages. */
    assert((size % 4096) == 0);

    /* We always allocate in units of pages */
    i = ffs(size / 4096) - 1;
    if (i >= INTEL_GEM_BO_BUCKETS)
	return NULL;

    return &bufmgr_gem->cache_bucket[i];
}


static void dri_gem_dump_validation_list(dri_bufmgr_gem *bufmgr_gem)
{
    int i, j;

    for (i = 0; i < bufmgr_gem->exec_count; i++) {
	dri_bo *bo = bufmgr_gem->exec_bos[i];
	dri_bo_gem *bo_gem = (dri_bo_gem *)bo;

	if (bo_gem->relocs == NULL) {
	    DBG("%2d: %d (%s)\n", i, bo_gem->gem_handle, bo_gem->name);
	    continue;
	}

	for (j = 0; j < bo_gem->reloc_count; j++) {
	    dri_bo *target_bo = bo_gem->reloc_target_bo[j];
	    dri_bo_gem *target_gem = (dri_bo_gem *)target_bo;

	    DBG("%2d: %d (%s)@0x%08llx -> %d (%s)@0x%08lx + 0x%08x\n",
		i,
		bo_gem->gem_handle, bo_gem->name, bo_gem->relocs[j].offset,
		target_gem->gem_handle, target_gem->name, target_bo->offset,
		bo_gem->relocs[j].delta);
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
intel_add_validate_buffer(dri_bo *bo)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    int index;

    if (bo_gem->validate_index != -1)
	return;

    /* Extend the array of validation entries as necessary. */
    if (bufmgr_gem->exec_count == bufmgr_gem->exec_size) {
	int new_size = bufmgr_gem->exec_size * 2;

	if (new_size == 0)
	    new_size = 5;

	bufmgr_gem->exec_objects =
	    realloc(bufmgr_gem->exec_objects,
		    sizeof(*bufmgr_gem->exec_objects) * new_size);
	bufmgr_gem->exec_bos =
	    realloc(bufmgr_gem->exec_bos,
		    sizeof(*bufmgr_gem->exec_bos) * new_size);
	bufmgr_gem->exec_size = new_size;
    }

    index = bufmgr_gem->exec_count;
    bo_gem->validate_index = index;
    /* Fill in array entry */
    bufmgr_gem->exec_objects[index].handle = bo_gem->gem_handle;
    bufmgr_gem->exec_objects[index].relocation_count = bo_gem->reloc_count;
    bufmgr_gem->exec_objects[index].relocs_ptr = (uintptr_t)bo_gem->relocs;
    bufmgr_gem->exec_objects[index].alignment = 0;
    bufmgr_gem->exec_objects[index].offset = 0;
    bufmgr_gem->exec_bos[index] = bo;
    dri_bo_reference(bo);
    bufmgr_gem->exec_count++;
}


#define RELOC_BUF_SIZE(x) ((I915_RELOC_HEADER + x * I915_RELOC0_STRIDE) * \
	sizeof(uint32_t))

static int
intel_setup_reloc_list(dri_bo *bo)
{
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;

    bo_gem->relocs = malloc(bufmgr_gem->max_relocs *
			    sizeof(struct drm_i915_gem_relocation_entry));
    bo_gem->reloc_target_bo = malloc(bufmgr_gem->max_relocs * sizeof(dri_bo *));

    return 0;
}

static dri_bo *
dri_gem_bo_alloc(dri_bufmgr *bufmgr, const char *name,
		 unsigned long size, unsigned int alignment,
		 uint64_t location_mask)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bufmgr;
    dri_bo_gem *bo_gem;
    unsigned int page_size = getpagesize();
    int ret;
    struct dri_gem_bo_bucket *bucket;
    GLboolean alloc_from_cache = GL_FALSE;

    bo_gem = calloc(1, sizeof(*bo_gem));
    if (!bo_gem)
	return NULL;

    /* Round the allocated size up to a power of two number of pages. */
    bo_gem->bo.size = 1 << logbase2(size);
    if (bo_gem->bo.size < page_size)
	bo_gem->bo.size = page_size;
    bucket = dri_gem_bo_bucket_for_size(bufmgr_gem, bo_gem->bo.size);

    /* If we don't have caching at this size, don't actually round the
     * allocation up.
     */
    if (bucket == NULL || bucket->max_entries == 0) {
	bo_gem->bo.size = size;
	if (bo_gem->bo.size < page_size)
	    bo_gem->bo.size = page_size;
    }

    /* Get a buffer out of the cache if available */
    if (bucket != NULL && bucket->num_entries > 0) {
	struct dri_gem_bo_bucket_entry *entry = bucket->head;
	struct drm_i915_gem_busy busy;
	
        busy.handle = entry->gem_handle;
        ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_I915_GEM_BUSY, &busy);
        alloc_from_cache = (ret == 0 && busy.busy == 0);

	if (alloc_from_cache) {
	    bucket->head = entry->next;
	    if (entry->next == NULL)
		bucket->tail = &bucket->head;
	    bucket->num_entries--;

	    bo_gem->gem_handle = entry->gem_handle;
	    bo_gem->bo.offset = entry->last_offset;
	    free(entry);
	}
    }

    if (!alloc_from_cache) {
	struct drm_gem_create create;

	memset(&create, 0, sizeof(create));
	create.size = bo_gem->bo.size;

	ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_CREATE, &create);
	bo_gem->gem_handle = create.handle;
	if (ret != 0) {
	    free(bo_gem);
	    return NULL;
	}
    }

    bo_gem->bo.virtual = NULL;
    bo_gem->bo.bufmgr = bufmgr;
    bo_gem->name = name;
    bo_gem->refcount = 1;
    bo_gem->validate_index = -1;

    DBG("bo_create: buf %d (%s) %ldb\n",
	bo_gem->gem_handle, bo_gem->name, size);

    return &bo_gem->bo;
}

/* Our GEM backend doesn't allow creation of static buffers, as that requires
 * privelege for the non-fake case, and the lock in the fake case where we were
 * working around the X Server not creating buffers and passing handles to us.
 */
static dri_bo *
dri_gem_bo_alloc_static(dri_bufmgr *bufmgr, const char *name,
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
intel_gem_bo_create_from_handle(dri_bufmgr *bufmgr, const char *name,
			      unsigned int handle)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bufmgr;
    dri_bo_gem *bo_gem;
    int ret;
    struct drm_gem_open open_arg;

    bo_gem = calloc(1, sizeof(*bo_gem));
    if (!bo_gem)
	return NULL;

    memset(&open_arg, 0, sizeof(open_arg));
    open_arg.name = handle;
    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_OPEN, &open_arg);
    if (ret != 0) {
	fprintf(stderr, "Couldn't reference %s handle 0x%08x: %s\n",
	       name, handle, strerror(-ret));
	free(bo_gem);
	return NULL;
    }
    bo_gem->bo.size = open_arg.size;
    bo_gem->bo.offset = 0;
    bo_gem->bo.virtual = NULL;
    bo_gem->bo.bufmgr = bufmgr;
    bo_gem->name = name;
    bo_gem->refcount = 1;
    bo_gem->validate_index = -1;
    bo_gem->gem_handle = open_arg.handle;

    DBG("bo_create_from_handle: %d (%s)\n", handle, bo_gem->name);

    return &bo_gem->bo;
}

static void
dri_gem_bo_reference(dri_bo *bo)
{
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;

    bo_gem->refcount++;
}

static void
dri_gem_bo_unreference(dri_bo *bo)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;

    if (!bo)
	return;

    if (--bo_gem->refcount == 0) {
	struct dri_gem_bo_bucket *bucket;
	int ret;

	if (bo_gem->mapped)
	    munmap (bo_gem->virtual, bo->size);

	if (bo_gem->relocs != NULL) {
	    int i;

	    /* Unreference all the target buffers */
	    for (i = 0; i < bo_gem->reloc_count; i++)
		 dri_bo_unreference(bo_gem->reloc_target_bo[i]);
	    free(bo_gem->reloc_target_bo);
	    free(bo_gem->relocs);
	}

	bucket = dri_gem_bo_bucket_for_size(bufmgr_gem, bo->size);
	/* Put the buffer into our internal cache for reuse if we can. */
	if (bucket != NULL &&
	    (bucket->max_entries == -1 ||
	     (bucket->max_entries > 0 &&
	      bucket->num_entries < bucket->max_entries)))
	{
	    struct dri_gem_bo_bucket_entry *entry;

	    entry = calloc(1, sizeof(*entry));
	    entry->gem_handle = bo_gem->gem_handle;
	    entry->last_offset = bo->offset;

	    entry->next = NULL;
	    *bucket->tail = entry;
	    bucket->tail = &entry->next;
	    bucket->num_entries++;
	} else {
	    struct drm_gem_close close;

	    /* Close this object */
	    close.handle = bo_gem->gem_handle;
	    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_CLOSE, &close);
	    if (ret != 0) {
	       fprintf(stderr,
		       "DRM_IOCTL_GEM_CLOSE %d failed (%s): %s\n",
		       bo_gem->gem_handle, bo_gem->name, strerror(-ret));
	    }
	}

	DBG("bo_unreference final: %d (%s)\n",
	    bo_gem->gem_handle, bo_gem->name);

	free(bo);
	return;
    }
}

static int
dri_gem_bo_map(dri_bo *bo, GLboolean write_enable)
{
    dri_bufmgr_gem *bufmgr_gem;
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    struct drm_gem_set_domain set_domain;
    int ret;

    bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;

    /* Allow recursive mapping. Mesa may recursively map buffers with
     * nested display loops.
     */
    if (!bo_gem->mapped) {
    
	assert(bo->virtual == NULL);
    
	DBG("bo_map: %d (%s)\n", bo_gem->gem_handle, bo_gem->name);
    
	if (bo_gem->virtual == NULL) {
	    struct drm_gem_mmap mmap_arg;
    
	    memset(&mmap_arg, 0, sizeof(mmap_arg));
	    mmap_arg.handle = bo_gem->gem_handle;
	    mmap_arg.offset = 0;
	    mmap_arg.size = bo->size;
	    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_MMAP, &mmap_arg);
	    if (ret != 0) {
		fprintf(stderr, "%s:%d: Error mapping buffer %d (%s): %s .\n",
			__FILE__, __LINE__,
			bo_gem->gem_handle, bo_gem->name, strerror(errno));
	    }
	    bo_gem->virtual = (void *)(uintptr_t)mmap_arg.addr_ptr;
	}
	bo->virtual = bo_gem->virtual;
	bo_gem->mapped = GL_TRUE;
	DBG("bo_map: %d (%s) -> %p\n", bo_gem->gem_handle, bo_gem->name, bo_gem->virtual);
    }

    if (!bo_gem->cpu_domain_set) {
	set_domain.handle = bo_gem->gem_handle;
	set_domain.read_domains = DRM_GEM_DOMAIN_CPU;
	set_domain.write_domain = write_enable ? DRM_GEM_DOMAIN_CPU : 0;
	ret = ioctl (bufmgr_gem->fd, DRM_IOCTL_GEM_SET_DOMAIN, &set_domain);
	if (ret != 0) {
	    fprintf (stderr, "%s:%d: Error setting memory domains %d (%08x %08x): %s .\n",
		     __FILE__, __LINE__,
		     bo_gem->gem_handle, set_domain.read_domains, set_domain.write_domain,
		     strerror (errno));
	}
	bo_gem->cpu_domain_set = GL_TRUE;
    }

    return 0;
}

static int
dri_gem_bo_unmap(dri_bo *bo)
{
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;

    if (bo == NULL)
	return 0;

    assert(bo_gem->mapped);

    return 0;
}

static int
dri_gem_bo_subdata (dri_bo *bo, unsigned long offset,
		    unsigned long size, const void *data)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    struct drm_gem_pwrite pwrite;
    int ret;

    memset (&pwrite, 0, sizeof (pwrite));
    pwrite.handle = bo_gem->gem_handle;
    pwrite.offset = offset;
    pwrite.size = size;
    pwrite.data_ptr = (uint64_t) (uintptr_t) data;
    ret = ioctl (bufmgr_gem->fd, DRM_IOCTL_GEM_PWRITE, &pwrite);
    if (ret != 0) {
	fprintf (stderr, "%s:%d: Error writing data to buffer %d: (%d %d) %s .\n",
		 __FILE__, __LINE__,
		 bo_gem->gem_handle, (int) offset, (int) size,
		 strerror (errno));
    }
    return 0;
}

static int
dri_gem_bo_get_subdata (dri_bo *bo, unsigned long offset,
			unsigned long size, void *data)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    struct drm_gem_pread pread;
    int ret;

    memset (&pread, 0, sizeof (pread));
    pread.handle = bo_gem->gem_handle;
    pread.offset = offset;
    pread.size = size;
    pread.data_ptr = (uint64_t) (uintptr_t) data;
    ret = ioctl (bufmgr_gem->fd, DRM_IOCTL_GEM_PREAD, &pread);
    if (ret != 0) {
	fprintf (stderr, "%s:%d: Error reading data from buffer %d: (%d %d) %s .\n",
		 __FILE__, __LINE__,
		 bo_gem->gem_handle, (int) offset, (int) size,
		 strerror (errno));
    }
    return 0;
}

static void
dri_gem_bo_wait_rendering(dri_bo *bo)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    struct drm_gem_set_domain set_domain;
    int ret;

    set_domain.handle = bo_gem->gem_handle;
    set_domain.read_domains = DRM_GEM_DOMAIN_CPU;
    set_domain.write_domain = 0;
    ret = ioctl (bufmgr_gem->fd, DRM_IOCTL_GEM_SET_DOMAIN, &set_domain);
    if (ret != 0) {
	fprintf (stderr, "%s:%d: Error setting memory domains %d (%08x %08x): %s .\n",
		 __FILE__, __LINE__,
		 bo_gem->gem_handle, set_domain.read_domains, set_domain.write_domain,
		 strerror (errno));
    }
}

static void
dri_bufmgr_gem_destroy(dri_bufmgr *bufmgr)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bufmgr;
    int i;

    free(bufmgr_gem->exec_objects);
    free(bufmgr_gem->exec_bos);

    /* Free any cached buffer objects we were going to reuse */
    for (i = 0; i < INTEL_GEM_BO_BUCKETS; i++) {
	struct dri_gem_bo_bucket *bucket = &bufmgr_gem->cache_bucket[i];
	struct dri_gem_bo_bucket_entry *entry;

	while ((entry = bucket->head) != NULL) {
	    struct drm_gem_close close;
	    int ret;

	    bucket->head = entry->next;
	    if (entry->next == NULL)
		bucket->tail = &bucket->head;
	    bucket->num_entries--;

	    /* Close this object */
	    close.handle = entry->gem_handle;
	    ret = ioctl(bufmgr_gem->fd, DRM_IOCTL_GEM_CLOSE, &close);
	    if (ret != 0) {
	       fprintf(stderr, "DRM_IOCTL_GEM_CLOSE failed: %s\n",
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
 * last known offset in target_bo.
 */
static int
dri_gem_emit_reloc(dri_bo *bo, uint32_t read_domains, uint32_t write_domain,
		   uint32_t delta, uint32_t offset, dri_bo *target_bo)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bo->bufmgr;
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    dri_bo_gem *target_bo_gem = (dri_bo_gem *)target_bo;

    /* Create a new relocation list if needed */
    if (bo_gem->relocs == NULL)
	intel_setup_reloc_list(bo);

    /* Check overflow */
    assert(bo_gem->reloc_count < bufmgr_gem->max_relocs);

    /* Check args */
    assert (offset <= bo->size - 4);
    assert ((write_domain & (write_domain-1)) == 0);

    bo_gem->relocs[bo_gem->reloc_count].offset = offset;
    bo_gem->relocs[bo_gem->reloc_count].delta = delta;
    bo_gem->relocs[bo_gem->reloc_count].target_handle =
	target_bo_gem->gem_handle;
    bo_gem->relocs[bo_gem->reloc_count].read_domains = read_domains;
    bo_gem->relocs[bo_gem->reloc_count].write_domain = write_domain;
    bo_gem->relocs[bo_gem->reloc_count].presumed_offset = target_bo->offset;

    bo_gem->reloc_target_bo[bo_gem->reloc_count] = target_bo;
    dri_bo_reference(target_bo);

    bo_gem->reloc_count++;
    return 0;
}

/**
 * Walk the tree of relocations rooted at BO and accumulate the list of
 * validations to be performed and update the relocation buffers with
 * index values into the validation list.
 */
static void
dri_gem_bo_process_reloc(dri_bo *bo)
{
    dri_bo_gem *bo_gem = (dri_bo_gem *)bo;
    int i;

    if (bo_gem->relocs == NULL)
	return;

    for (i = 0; i < bo_gem->reloc_count; i++) {
	dri_bo *target_bo = bo_gem->reloc_target_bo[i];

	/* Continue walking the tree depth-first. */
	dri_gem_bo_process_reloc(target_bo);

	/* Add the target to the validate list */
	intel_add_validate_buffer(target_bo);
    }
}

static void *
dri_gem_process_reloc(dri_bo *batch_buf)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *) batch_buf->bufmgr;

    /* Update indices and set up the validate list. */
    dri_gem_bo_process_reloc(batch_buf);

    /* Add the batch buffer to the validation list.  There are no relocations
     * pointing to it.
     */
    intel_add_validate_buffer(batch_buf);

    bufmgr_gem->exec_arg.buffers_ptr = (uintptr_t)bufmgr_gem->exec_objects;
    bufmgr_gem->exec_arg.buffer_count = bufmgr_gem->exec_count;
    bufmgr_gem->exec_arg.batch_start_offset = 0;
    bufmgr_gem->exec_arg.batch_len = 0;	/* written in intel_exec_ioctl */

    return &bufmgr_gem->exec_arg;
}

static void
intel_update_buffer_offsets (dri_bufmgr_gem *bufmgr_gem)
{
    int i;

    for (i = 0; i < bufmgr_gem->exec_count; i++) {
	dri_bo *bo = bufmgr_gem->exec_bos[i];
	dri_bo_gem *bo_gem = (dri_bo_gem *)bo;

	/* Update the buffer offset */
	if (bufmgr_gem->exec_objects[i].offset != bo->offset) {
	    DBG("BO %d (%s) migrated: 0x%08lx -> 0x%08llx\n",
		bo_gem->gem_handle, bo_gem->name, bo->offset,
		bufmgr_gem->exec_objects[i].offset);
	    bo->offset = bufmgr_gem->exec_objects[i].offset;
	}
    }
}

static void
dri_gem_post_submit(dri_bo *batch_buf)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)batch_buf->bufmgr;
    int i;

    intel_update_buffer_offsets (bufmgr_gem);

    if (bufmgr_gem->bufmgr.debug)
	dri_gem_dump_validation_list(bufmgr_gem);

    for (i = 0; i < bufmgr_gem->exec_count; i++) {
	dri_bo *bo = bufmgr_gem->exec_bos[i];
	dri_bo_gem *bo_gem = (dri_bo_gem *)bo;

	/* Need to call set_domain on next bo_map */
	bo_gem->cpu_domain_set = GL_FALSE;

	/* Disconnect the buffer from the validate list */
	bo_gem->validate_index = -1;
	dri_bo_unreference(bo);
	bufmgr_gem->exec_bos[i] = NULL;
    }
    bufmgr_gem->exec_count = 0;
}

/**
 * Enables unlimited caching of buffer objects for reuse.
 *
 * This is potentially very memory expensive, as the cache at each bucket
 * size is only bounded by how many buffers of that size we've managed to have
 * in flight at once.
 */
void
intel_gem_enable_bo_reuse(dri_bufmgr *bufmgr)
{
    dri_bufmgr_gem *bufmgr_gem = (dri_bufmgr_gem *)bufmgr;
    int i;

    for (i = 0; i < INTEL_GEM_BO_BUCKETS; i++) {
	bufmgr_gem->cache_bucket[i].max_entries = -1;
    }
}

/*
 *
 */
static int
dri_gem_check_aperture_space(dri_bo *bo)
{
    return 0;
}

/**
 * Initializes the GEM buffer manager, which uses the kernel to allocate, map,
 * and manage map buffer objections.
 *
 * \param fd File descriptor of the opened DRM device.
 */
dri_bufmgr *
intel_bufmgr_gem_init(int fd, int batch_size)
{
    dri_bufmgr_gem *bufmgr_gem;
    int i;

    bufmgr_gem = calloc(1, sizeof(*bufmgr_gem));
    bufmgr_gem->fd = fd;

    /* Let's go with one relocation per every 2 dwords (but round down a bit
     * since a power of two will mean an extra page allocation for the reloc
     * buffer).
     *
     * Every 4 was too few for the blender benchmark.
     */
    bufmgr_gem->max_relocs = batch_size / sizeof(uint32_t) / 2 - 2;

    bufmgr_gem->bufmgr.bo_alloc = dri_gem_bo_alloc;
    bufmgr_gem->bufmgr.bo_alloc_static = dri_gem_bo_alloc_static;
    bufmgr_gem->bufmgr.bo_reference = dri_gem_bo_reference;
    bufmgr_gem->bufmgr.bo_unreference = dri_gem_bo_unreference;
    bufmgr_gem->bufmgr.bo_map = dri_gem_bo_map;
    bufmgr_gem->bufmgr.bo_unmap = dri_gem_bo_unmap;
    bufmgr_gem->bufmgr.bo_subdata = dri_gem_bo_subdata;
    bufmgr_gem->bufmgr.bo_get_subdata = dri_gem_bo_get_subdata;
    bufmgr_gem->bufmgr.bo_wait_rendering = dri_gem_bo_wait_rendering;
    bufmgr_gem->bufmgr.destroy = dri_bufmgr_gem_destroy;
    bufmgr_gem->bufmgr.emit_reloc = dri_gem_emit_reloc;
    bufmgr_gem->bufmgr.process_relocs = dri_gem_process_reloc;
    bufmgr_gem->bufmgr.post_submit = dri_gem_post_submit;
    bufmgr_gem->bufmgr.debug = GL_FALSE;
    bufmgr_gem->bufmgr.check_aperture_space = dri_gem_check_aperture_space;
    /* Initialize the linked lists for BO reuse cache. */
    for (i = 0; i < INTEL_GEM_BO_BUCKETS; i++)
	bufmgr_gem->cache_bucket[i].tail = &bufmgr_gem->cache_bucket[i].head;

    return &bufmgr_gem->bufmgr;
}

