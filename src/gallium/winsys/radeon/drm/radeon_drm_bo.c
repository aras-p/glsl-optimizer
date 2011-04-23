/*
 * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */

#define _FILE_OFFSET_BITS 64
#include "radeon_drm_cs.h"

#include "util/u_hash_table.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "os/os_thread.h"

#include "state_tracker/drm_driver.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <errno.h>

#define RADEON_BO_FLAGS_MACRO_TILE  1
#define RADEON_BO_FLAGS_MICRO_TILE  2
#define RADEON_BO_FLAGS_MICRO_TILE_SQUARE 0x20

extern const struct pb_vtbl radeon_bo_vtbl;


static INLINE struct radeon_bo *radeon_bo(struct pb_buffer *bo)
{
    assert(bo->vtbl == &radeon_bo_vtbl);
    return (struct radeon_bo *)bo;
}

struct radeon_bomgr {
    /* Base class. */
    struct pb_manager base;

    /* Winsys. */
    struct radeon_drm_winsys *rws;

    /* List of buffer handles and its mutex. */
    struct util_hash_table *bo_handles;
    pipe_mutex bo_handles_mutex;
};

static INLINE struct radeon_bomgr *radeon_bomgr(struct pb_manager *mgr)
{
    return (struct radeon_bomgr *)mgr;
}

static struct radeon_bo *get_radeon_bo(struct pb_buffer *_buf)
{
    struct radeon_bo *bo = NULL;

    if (_buf->vtbl == &radeon_bo_vtbl) {
        bo = radeon_bo(_buf);
    } else {
	struct pb_buffer *base_buf;
	pb_size offset;
	pb_get_base_buffer(_buf, &base_buf, &offset);

        if (base_buf->vtbl == &radeon_bo_vtbl)
            bo = radeon_bo(base_buf);
    }

    return bo;
}

static void radeon_bo_wait(struct pb_buffer *_buf)
{
    struct radeon_bo *bo = get_radeon_bo(pb_buffer(_buf));
    struct drm_radeon_gem_wait_idle args = {};

    while (p_atomic_read(&bo->num_active_ioctls)) {
        sched_yield();
    }

    args.handle = bo->handle;
    while (drmCommandWriteRead(bo->rws->fd, DRM_RADEON_GEM_WAIT_IDLE,
                               &args, sizeof(args)) == -EBUSY);

    bo->busy_for_write = FALSE;
}

static boolean radeon_bo_is_busy(struct pb_buffer *_buf)
{
    struct radeon_bo *bo = get_radeon_bo(pb_buffer(_buf));
    struct drm_radeon_gem_busy args = {};
    boolean busy;

    if (p_atomic_read(&bo->num_active_ioctls)) {
        return TRUE;
    }

    args.handle = bo->handle;
    busy = drmCommandWriteRead(bo->rws->fd, DRM_RADEON_GEM_BUSY,
                               &args, sizeof(args)) != 0;

    if (!busy)
        bo->busy_for_write = FALSE;
    return busy;
}

static void radeon_bo_destroy(struct pb_buffer *_buf)
{
    struct radeon_bo *bo = radeon_bo(_buf);
    struct drm_gem_close args = {};

    if (bo->name) {
        pipe_mutex_lock(bo->mgr->bo_handles_mutex);
        util_hash_table_remove(bo->mgr->bo_handles,
			       (void*)(uintptr_t)bo->name);
        pipe_mutex_unlock(bo->mgr->bo_handles_mutex);
    }

    if (bo->ptr)
        munmap(bo->ptr, bo->size);

    /* Close object. */
    args.handle = bo->handle;
    drmIoctl(bo->rws->fd, DRM_IOCTL_GEM_CLOSE, &args);
    pipe_mutex_destroy(bo->map_mutex);
    FREE(bo);
}

static unsigned get_pb_usage_from_transfer_flags(enum pipe_transfer_usage usage)
{
    unsigned res = 0;

    if (usage & PIPE_TRANSFER_WRITE)
        res |= PB_USAGE_CPU_WRITE;

    if (usage & PIPE_TRANSFER_DONTBLOCK)
        res |= PB_USAGE_DONTBLOCK;

    if (usage & PIPE_TRANSFER_UNSYNCHRONIZED)
        res |= PB_USAGE_UNSYNCHRONIZED;

    return res;
}

static void *radeon_bo_map_internal(struct pb_buffer *_buf,
                                    unsigned flags, void *flush_ctx)
{
    struct radeon_bo *bo = radeon_bo(_buf);
    struct radeon_drm_cs *cs = flush_ctx;
    struct drm_radeon_gem_mmap args = {};
    void *ptr;

    /* If it's not unsynchronized bo_map, flush CS if needed and then wait. */
    if (!(flags & PB_USAGE_UNSYNCHRONIZED)) {
        /* DONTBLOCK doesn't make sense with UNSYNCHRONIZED. */
        if (flags & PB_USAGE_DONTBLOCK) {
            if (radeon_bo_is_referenced_by_cs(cs, bo)) {
                cs->flush_cs(cs->flush_data, RADEON_FLUSH_ASYNC);
                return NULL;
            }

            if (radeon_bo_is_busy((struct pb_buffer*)bo)) {
                return NULL;
            }
        } else {
            if (!(flags & PB_USAGE_CPU_WRITE)) {
                /* Mapping for read.
                 *
                 * Since we are mapping for read, we don't need to wait
                 * if the GPU is using the buffer for read too
                 * (neither one is changing it).
                 *
                 * Only check whether the buffer is being used for write. */
                if (radeon_bo_is_referenced_by_cs_for_write(cs, bo)) {
                    cs->flush_cs(cs->flush_data, 0);
                    radeon_bo_wait((struct pb_buffer*)bo);
                } else if (bo->busy_for_write) {
                    /* Update the busy_for_write field (done by radeon_bo_is_busy)
                     * and wait if needed. */
                    if (radeon_bo_is_busy((struct pb_buffer*)bo)) {
                        radeon_bo_wait((struct pb_buffer*)bo);
                    }
                }
            } else {
                /* Mapping for write. */
                if (radeon_bo_is_referenced_by_cs(cs, bo)) {
                    cs->flush_cs(cs->flush_data, 0);
                } else {
                    /* Try to avoid busy-waiting in radeon_bo_wait. */
                    if (p_atomic_read(&bo->num_active_ioctls))
                        radeon_drm_cs_sync_flush(cs);
                }

                radeon_bo_wait((struct pb_buffer*)bo);
            }
        }
    }

    /* Return the pointer if it's already mapped. */
    if (bo->ptr)
        return bo->ptr;

    /* Map the buffer. */
    pipe_mutex_lock(bo->map_mutex);
    /* Return the pointer if it's already mapped (in case of a race). */
    if (bo->ptr) {
        pipe_mutex_unlock(bo->map_mutex);
        return bo->ptr;
    }
    args.handle = bo->handle;
    args.offset = 0;
    args.size = (uint64_t)bo->size;
    if (drmCommandWriteRead(bo->rws->fd,
                            DRM_RADEON_GEM_MMAP,
                            &args,
                            sizeof(args))) {
        pipe_mutex_unlock(bo->map_mutex);
        fprintf(stderr, "radeon: gem_mmap failed: %p 0x%08X\n",
                bo, bo->handle);
        return NULL;
    }

    ptr = mmap(0, args.size, PROT_READ|PROT_WRITE, MAP_SHARED,
               bo->rws->fd, args.addr_ptr);
    if (ptr == MAP_FAILED) {
        pipe_mutex_unlock(bo->map_mutex);
        fprintf(stderr, "radeon: mmap failed, errno: %i\n", errno);
        return NULL;
    }
    bo->ptr = ptr;
    pipe_mutex_unlock(bo->map_mutex);

    return bo->ptr;
}

static void radeon_bo_unmap_internal(struct pb_buffer *_buf)
{
    /* NOP */
}

static void radeon_bo_get_base_buffer(struct pb_buffer *buf,
				      struct pb_buffer **base_buf,
				      unsigned *offset)
{
    *base_buf = buf;
    *offset = 0;
}

static enum pipe_error radeon_bo_validate(struct pb_buffer *_buf,
					  struct pb_validate *vl,
					  unsigned flags)
{
    /* Always pinned */
    return PIPE_OK;
}

static void radeon_bo_fence(struct pb_buffer *buf,
                            struct pipe_fence_handle *fence)
{
}

const struct pb_vtbl radeon_bo_vtbl = {
    radeon_bo_destroy,
    radeon_bo_map_internal,
    radeon_bo_unmap_internal,
    radeon_bo_validate,
    radeon_bo_fence,
    radeon_bo_get_base_buffer,
};

static struct pb_buffer *radeon_bomgr_create_bo(struct pb_manager *_mgr,
						pb_size size,
						const struct pb_desc *desc)
{
    struct radeon_bomgr *mgr = radeon_bomgr(_mgr);
    struct radeon_drm_winsys *rws = mgr->rws;
    struct radeon_bo *bo;
    struct drm_radeon_gem_create args = {};

    args.size = size;
    args.alignment = desc->alignment;
    args.initial_domain =
        (desc->usage & RADEON_PB_USAGE_DOMAIN_GTT  ?
         RADEON_GEM_DOMAIN_GTT  : 0) |
        (desc->usage & RADEON_PB_USAGE_DOMAIN_VRAM ?
         RADEON_GEM_DOMAIN_VRAM : 0);

    if (drmCommandWriteRead(rws->fd, DRM_RADEON_GEM_CREATE,
                            &args, sizeof(args))) {
        fprintf(stderr, "radeon: Failed to allocate a buffer:\n");
        fprintf(stderr, "radeon:    size      : %d bytes\n", size);
        fprintf(stderr, "radeon:    alignment : %d bytes\n", desc->alignment);
        fprintf(stderr, "radeon:    domains   : %d\n", args.initial_domain);
        return NULL;
    }

    bo = CALLOC_STRUCT(radeon_bo);
    if (!bo)
	return NULL;

    pipe_reference_init(&bo->base.base.reference, 1);
    bo->base.base.alignment = desc->alignment;
    bo->base.base.usage = desc->usage;
    bo->base.base.size = size;
    bo->base.vtbl = &radeon_bo_vtbl;
    bo->mgr = mgr;
    bo->rws = mgr->rws;
    bo->handle = args.handle;
    bo->size = size;
    pipe_mutex_init(bo->map_mutex);

    return &bo->base;
}

static void radeon_bomgr_flush(struct pb_manager *mgr)
{
    /* NOP */
}

/* This is for the cache bufmgr. */
static boolean radeon_bomgr_is_buffer_busy(struct pb_manager *_mgr,
                                           struct pb_buffer *_buf)
{
   struct radeon_bo *bo = radeon_bo(_buf);

   if (radeon_bo_is_referenced_by_any_cs(bo)) {
       return TRUE;
   }

   if (radeon_bo_is_busy((struct pb_buffer*)bo)) {
       return TRUE;
   }

   return FALSE;
}

static void radeon_bomgr_destroy(struct pb_manager *_mgr)
{
    struct radeon_bomgr *mgr = radeon_bomgr(_mgr);
    util_hash_table_destroy(mgr->bo_handles);
    pipe_mutex_destroy(mgr->bo_handles_mutex);
    FREE(mgr);
}

#define PTR_TO_UINT(x) ((unsigned)((intptr_t)(x)))

static unsigned handle_hash(void *key)
{
    return PTR_TO_UINT(key);
}

static int handle_compare(void *key1, void *key2)
{
    return PTR_TO_UINT(key1) != PTR_TO_UINT(key2);
}

struct pb_manager *radeon_bomgr_create(struct radeon_drm_winsys *rws)
{
    struct radeon_bomgr *mgr;

    mgr = CALLOC_STRUCT(radeon_bomgr);
    if (!mgr)
	return NULL;

    mgr->base.destroy = radeon_bomgr_destroy;
    mgr->base.create_buffer = radeon_bomgr_create_bo;
    mgr->base.flush = radeon_bomgr_flush;
    mgr->base.is_buffer_busy = radeon_bomgr_is_buffer_busy;

    mgr->rws = rws;
    mgr->bo_handles = util_hash_table_create(handle_hash, handle_compare);
    pipe_mutex_init(mgr->bo_handles_mutex);
    return &mgr->base;
}

static void *radeon_bo_map(struct pb_buffer *buf,
                           struct radeon_winsys_cs *cs,
                           enum pipe_transfer_usage usage)
{
    struct pb_buffer *_buf = pb_buffer(buf);

    return pb_map(_buf, get_pb_usage_from_transfer_flags(usage), cs);
}

static void radeon_bo_get_tiling(struct pb_buffer *_buf,
                                 enum radeon_bo_layout *microtiled,
                                 enum radeon_bo_layout *macrotiled)
{
    struct radeon_bo *bo = get_radeon_bo(pb_buffer(_buf));
    struct drm_radeon_gem_set_tiling args = {};

    args.handle = bo->handle;

    drmCommandWriteRead(bo->rws->fd,
                        DRM_RADEON_GEM_GET_TILING,
                        &args,
                        sizeof(args));

    *microtiled = RADEON_LAYOUT_LINEAR;
    *macrotiled = RADEON_LAYOUT_LINEAR;
    if (args.tiling_flags & RADEON_BO_FLAGS_MICRO_TILE)
	*microtiled = RADEON_LAYOUT_TILED;

    if (args.tiling_flags & RADEON_BO_FLAGS_MACRO_TILE)
	*macrotiled = RADEON_LAYOUT_TILED;
}

static void radeon_bo_set_tiling(struct pb_buffer *_buf,
                                 struct radeon_winsys_cs *rcs,
                                 enum radeon_bo_layout microtiled,
                                 enum radeon_bo_layout macrotiled,
                                 uint32_t pitch)
{
    struct radeon_bo *bo = get_radeon_bo(pb_buffer(_buf));
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct drm_radeon_gem_set_tiling args = {};

    /* Tiling determines how DRM treats the buffer data.
     * We must flush CS when changing it if the buffer is referenced. */
    if (cs && radeon_bo_is_referenced_by_cs(cs, bo)) {
        cs->flush_cs(cs->flush_data, 0);
    }

    while (p_atomic_read(&bo->num_active_ioctls)) {
        sched_yield();
    }

    if (microtiled == RADEON_LAYOUT_TILED)
        args.tiling_flags |= RADEON_BO_FLAGS_MICRO_TILE;
    else if (microtiled == RADEON_LAYOUT_SQUARETILED)
        args.tiling_flags |= RADEON_BO_FLAGS_MICRO_TILE_SQUARE;

    if (macrotiled == RADEON_LAYOUT_TILED)
        args.tiling_flags |= RADEON_BO_FLAGS_MACRO_TILE;

    args.handle = bo->handle;
    args.pitch = pitch;

    drmCommandWriteRead(bo->rws->fd,
                        DRM_RADEON_GEM_SET_TILING,
                        &args,
                        sizeof(args));
}

static struct radeon_winsys_cs_handle *radeon_drm_get_cs_handle(
        struct pb_buffer *_buf)
{
    /* return radeon_bo. */
    return (struct radeon_winsys_cs_handle*)
            get_radeon_bo(pb_buffer(_buf));
}

static unsigned get_pb_usage_from_create_flags(unsigned bind, unsigned usage,
                                               enum radeon_bo_domain domain)
{
    unsigned res = 0;

    if (domain & RADEON_DOMAIN_GTT)
        res |= RADEON_PB_USAGE_DOMAIN_GTT;

    if (domain & RADEON_DOMAIN_VRAM)
        res |= RADEON_PB_USAGE_DOMAIN_VRAM;

    return res;
}

static struct pb_buffer *
radeon_winsys_bo_create(struct radeon_winsys *rws,
                        unsigned size,
                        unsigned alignment,
                        unsigned bind,
                        unsigned usage,
                        enum radeon_bo_domain domain)
{
    struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
    struct pb_desc desc;
    struct pb_manager *provider;
    struct pb_buffer *buffer;

    memset(&desc, 0, sizeof(desc));
    desc.alignment = alignment;
    desc.usage = get_pb_usage_from_create_flags(bind, usage, domain);

    /* Assign a buffer manager. */
    if (bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
	provider = ws->cman;
    else
        provider = ws->kman;

    buffer = provider->create_buffer(provider, size, &desc);
    if (!buffer)
	return NULL;

    return (struct pb_buffer*)buffer;
}

static struct pb_buffer *radeon_winsys_bo_from_handle(struct radeon_winsys *rws,
                                                           struct winsys_handle *whandle,
                                                           unsigned *stride,
                                                           unsigned *size)
{
    struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
    struct radeon_bo *bo;
    struct radeon_bomgr *mgr = radeon_bomgr(ws->kman);
    struct drm_gem_open open_arg = {};

    /* We must maintain a list of pairs <handle, bo>, so that we always return
     * the same BO for one particular handle. If we didn't do that and created
     * more than one BO for the same handle and then relocated them in a CS,
     * we would hit a deadlock in the kernel.
     *
     * The list of pairs is guarded by a mutex, of course. */
    pipe_mutex_lock(mgr->bo_handles_mutex);

    /* First check if there already is an existing bo for the handle. */
    bo = util_hash_table_get(mgr->bo_handles, (void*)(uintptr_t)whandle->handle);
    if (bo) {
        /* Increase the refcount. */
        struct pb_buffer *b = NULL;
        pb_reference(&b, &bo->base);
        goto done;
    }

    /* There isn't, create a new one. */
    bo = CALLOC_STRUCT(radeon_bo);
    if (!bo) {
        goto fail;
    }

    /* Open the BO. */
    open_arg.name = whandle->handle;
    if (drmIoctl(ws->fd, DRM_IOCTL_GEM_OPEN, &open_arg)) {
        FREE(bo);
        goto fail;
    }
    bo->handle = open_arg.handle;
    bo->size = open_arg.size;
    bo->name = whandle->handle;

    /* Initialize it. */
    pipe_reference_init(&bo->base.base.reference, 1);
    bo->base.base.alignment = 0;
    bo->base.base.usage = PB_USAGE_GPU_WRITE | PB_USAGE_GPU_READ;
    bo->base.base.size = bo->size;
    bo->base.vtbl = &radeon_bo_vtbl;
    bo->mgr = mgr;
    bo->rws = mgr->rws;
    pipe_mutex_init(bo->map_mutex);

    util_hash_table_set(mgr->bo_handles, (void*)(uintptr_t)whandle->handle, bo);

done:
    pipe_mutex_unlock(mgr->bo_handles_mutex);

    if (stride)
        *stride = whandle->stride;
    if (size)
        *size = bo->base.base.size;

    return (struct pb_buffer*)bo;

fail:
    pipe_mutex_unlock(mgr->bo_handles_mutex);
    return NULL;
}

static boolean radeon_winsys_bo_get_handle(struct pb_buffer *buffer,
                                           unsigned stride,
                                           struct winsys_handle *whandle)
{
    struct drm_gem_flink flink = {};
    struct radeon_bo *bo = get_radeon_bo(pb_buffer(buffer));

    if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) {
        if (!bo->flinked) {
            flink.handle = bo->handle;

            if (ioctl(bo->rws->fd, DRM_IOCTL_GEM_FLINK, &flink)) {
                return FALSE;
            }

            bo->flinked = TRUE;
            bo->flink = flink.name;
        }
        whandle->handle = bo->flink;
    } else if (whandle->type == DRM_API_HANDLE_TYPE_KMS) {
        whandle->handle = bo->handle;
    }

    whandle->stride = stride;
    return TRUE;
}

void radeon_bomgr_init_functions(struct radeon_drm_winsys *ws)
{
    ws->base.buffer_get_cs_handle = radeon_drm_get_cs_handle;
    ws->base.buffer_set_tiling = radeon_bo_set_tiling;
    ws->base.buffer_get_tiling = radeon_bo_get_tiling;
    ws->base.buffer_map = radeon_bo_map;
    ws->base.buffer_unmap = pb_unmap;
    ws->base.buffer_wait = radeon_bo_wait;
    ws->base.buffer_is_busy = radeon_bo_is_busy;
    ws->base.buffer_create = radeon_winsys_bo_create;
    ws->base.buffer_from_handle = radeon_winsys_bo_from_handle;
    ws->base.buffer_get_handle = radeon_winsys_bo_get_handle;
}
