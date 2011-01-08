#include "radeon_drm_buffer.h"
#include "radeon_drm_cs.h"

#include "util/u_hash_table.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "pipebuffer/pb_bufmgr.h"
#include "os/os_thread.h"

#include "state_tracker/drm_driver.h"

#include <radeon_drm.h>
#include <radeon_bo_gem.h>
#include <sys/ioctl.h>

struct radeon_drm_bufmgr;

struct radeon_drm_buffer {
    struct pb_buffer base;
    struct radeon_drm_bufmgr *mgr;

    struct radeon_bo *bo;

    boolean flinked;
    uint32_t flink;

    struct radeon_drm_buffer *next, *prev;
};

extern const struct pb_vtbl radeon_drm_buffer_vtbl;


static INLINE struct radeon_drm_buffer *
radeon_drm_buffer(struct pb_buffer *buf)
{
    assert(buf);
    assert(buf->vtbl == &radeon_drm_buffer_vtbl);
    return (struct radeon_drm_buffer *)buf;
}

struct radeon_drm_bufmgr {
    /* Base class. */
    struct pb_manager base;

    /* Winsys. */
    struct radeon_drm_winsys *rws;

    /* List of mapped buffers and its mutex. */
    struct radeon_drm_buffer buffer_map_list;
    pipe_mutex buffer_map_list_mutex;

    /* List of buffer handles and its mutex. */
    struct util_hash_table *buffer_handles;
    pipe_mutex buffer_handles_mutex;
};

static INLINE struct radeon_drm_bufmgr *
radeon_drm_bufmgr(struct pb_manager *mgr)
{
    assert(mgr);
    return (struct radeon_drm_bufmgr *)mgr;
}

static void
radeon_drm_buffer_destroy(struct pb_buffer *_buf)
{
    struct radeon_drm_buffer *buf = radeon_drm_buffer(_buf);
    int name;

    if (buf->bo->ptr != NULL) {
        pipe_mutex_lock(buf->mgr->buffer_map_list_mutex);
        /* Now test it again inside the mutex. */
        if (buf->bo->ptr != NULL) {
            remove_from_list(buf);
            radeon_bo_unmap(buf->bo);
            buf->bo->ptr = NULL;
        }
        pipe_mutex_unlock(buf->mgr->buffer_map_list_mutex);
    }
    name = radeon_gem_name_bo(buf->bo);
    if (name) {
        pipe_mutex_lock(buf->mgr->buffer_handles_mutex);
	util_hash_table_remove(buf->mgr->buffer_handles,
			       (void*)(uintptr_t)name);
        pipe_mutex_unlock(buf->mgr->buffer_handles_mutex);
    }
    radeon_bo_unref(buf->bo);

    FREE(buf);
}

static unsigned get_pb_usage_from_transfer_flags(enum pipe_transfer_usage usage)
{
    unsigned res = 0;

    if (usage & PIPE_TRANSFER_READ)
        res |= PB_USAGE_CPU_READ;

    if (usage & PIPE_TRANSFER_WRITE)
        res |= PB_USAGE_CPU_WRITE;

    if (usage & PIPE_TRANSFER_DONTBLOCK)
        res |= PB_USAGE_DONTBLOCK;

    if (usage & PIPE_TRANSFER_UNSYNCHRONIZED)
        res |= PB_USAGE_UNSYNCHRONIZED;

    return res;
}

static void *
radeon_drm_buffer_map_internal(struct pb_buffer *_buf,
			       unsigned flags, void *flush_ctx)
{
    struct radeon_drm_buffer *buf = radeon_drm_buffer(_buf);
    struct radeon_drm_cs *cs = flush_ctx;
    int write = 0;

    /* Note how we use radeon_bo_is_referenced_by_cs here. There are
     * basically two places this map function can be called from:
     * - pb_map
     * - create_buffer (in the buffer reuse case)
     *
     * Since pb managers are per-winsys managers, not per-context managers,
     * and we shouldn't reuse buffers if they are in-use in any context,
     * we simply ask: is this buffer referenced by *any* CS?
     *
     * The problem with buffer_create is that it comes from pipe_screen,
     * so we have no CS to look at, though luckily the following code
     * is sufficient to tell whether the buffer is in use. */
    if (flags & PB_USAGE_DONTBLOCK) {
        if (_buf->base.usage & RADEON_PB_USAGE_VERTEX)
            if (radeon_bo_is_referenced_by_cs(buf->bo, NULL))
		return NULL;
    }

    if (buf->bo->ptr != NULL) {
        pipe_mutex_lock(buf->mgr->buffer_map_list_mutex);
        /* Now test ptr again inside the mutex. We might have gotten a race
         * during the first test. */
        if (buf->bo->ptr != NULL) {
            remove_from_list(buf);
        }
        pipe_mutex_unlock(buf->mgr->buffer_map_list_mutex);
	return buf->bo->ptr;
    }

    if (flags & PB_USAGE_DONTBLOCK) {
        uint32_t domain;
        if (radeon_bo_is_busy(buf->bo, &domain))
            return NULL;
    }

    /* If we don't have any CS and the buffer is referenced,
     * we cannot flush. */
    assert(cs || !radeon_bo_is_referenced_by_cs(buf->bo, NULL));

    if (cs && radeon_bo_is_referenced_by_cs(buf->bo, NULL)) {
        cs->flush_cs(cs->flush_data);
    }

    if (flags & PB_USAGE_CPU_WRITE) {
        write = 1;
    }

    if (radeon_bo_map(buf->bo, write)) {
        return NULL;
    }

    pipe_mutex_lock(buf->mgr->buffer_map_list_mutex);
    remove_from_list(buf);
    pipe_mutex_unlock(buf->mgr->buffer_map_list_mutex);
    return buf->bo->ptr;
}

static void
radeon_drm_buffer_unmap_internal(struct pb_buffer *_buf)
{
    struct radeon_drm_buffer *buf = radeon_drm_buffer(_buf);
    pipe_mutex_lock(buf->mgr->buffer_map_list_mutex);
    if (is_empty_list(buf)) { /* = is not inserted... */
        insert_at_tail(&buf->mgr->buffer_map_list, buf);
    }
    pipe_mutex_unlock(buf->mgr->buffer_map_list_mutex);
}

static void
radeon_drm_buffer_get_base_buffer(struct pb_buffer *buf,
				  struct pb_buffer **base_buf,
				  unsigned *offset)
{
    *base_buf = buf;
    *offset = 0;
}


static enum pipe_error
radeon_drm_buffer_validate(struct pb_buffer *_buf,
			   struct pb_validate *vl,
			   unsigned flags)
{
   /* Always pinned */
   return PIPE_OK;
}

static void
radeon_drm_buffer_fence(struct pb_buffer *buf,
			struct pipe_fence_handle *fence)
{
}

const struct pb_vtbl radeon_drm_buffer_vtbl = {
    radeon_drm_buffer_destroy,
    radeon_drm_buffer_map_internal,
    radeon_drm_buffer_unmap_internal,
    radeon_drm_buffer_validate,
    radeon_drm_buffer_fence,
    radeon_drm_buffer_get_base_buffer,
};

static struct pb_buffer *
radeon_drm_bufmgr_create_buffer_from_handle_unsafe(struct pb_manager *_mgr,
                                                   uint32_t handle)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    struct radeon_drm_winsys *rws = mgr->rws;
    struct radeon_drm_buffer *buf;
    struct radeon_bo *bo;

    buf = util_hash_table_get(mgr->buffer_handles, (void*)(uintptr_t)handle);

    if (buf) {
        struct pb_buffer *b = NULL;
        pb_reference(&b, &buf->base);
        return b;
    }

    bo = radeon_bo_open(rws->bom, handle, 0,
			0, 0, 0);
    if (bo == NULL)
	return NULL;

    buf = CALLOC_STRUCT(radeon_drm_buffer);
    if (!buf) {
	radeon_bo_unref(bo);
	return NULL;
    }

    make_empty_list(buf);

    pipe_reference_init(&buf->base.base.reference, 1);
    buf->base.base.alignment = 0;
    buf->base.base.usage = PB_USAGE_GPU_WRITE | PB_USAGE_GPU_READ;
    buf->base.base.size = bo->size;
    buf->base.vtbl = &radeon_drm_buffer_vtbl;
    buf->mgr = mgr;

    buf->bo = bo;

    util_hash_table_set(mgr->buffer_handles, (void*)(uintptr_t)handle, buf);

    return &buf->base;
}

struct pb_buffer *
radeon_drm_bufmgr_create_buffer_from_handle(struct pb_manager *_mgr,
                                            uint32_t handle)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    struct pb_buffer *pb;

    pipe_mutex_lock(mgr->buffer_handles_mutex);
    pb = radeon_drm_bufmgr_create_buffer_from_handle_unsafe(_mgr, handle);
    pipe_mutex_unlock(mgr->buffer_handles_mutex);

    return pb;
}

static struct pb_buffer *
radeon_drm_bufmgr_create_buffer(struct pb_manager *_mgr,
				pb_size size,
				const struct pb_desc *desc)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    struct radeon_drm_winsys *rws = mgr->rws;
    struct radeon_drm_buffer *buf;
    uint32_t domain;

    buf = CALLOC_STRUCT(radeon_drm_buffer);
    if (!buf)
	goto error1;

    pipe_reference_init(&buf->base.base.reference, 1);
    buf->base.base.alignment = desc->alignment;
    buf->base.base.usage = desc->usage;
    buf->base.base.size = size;
    buf->base.vtbl = &radeon_drm_buffer_vtbl;
    buf->mgr = mgr;

    make_empty_list(buf);

    domain =
        (desc->usage & RADEON_PB_USAGE_DOMAIN_GTT  ? RADEON_GEM_DOMAIN_GTT  : 0) |
        (desc->usage & RADEON_PB_USAGE_DOMAIN_VRAM ? RADEON_GEM_DOMAIN_VRAM : 0);

    buf->bo = radeon_bo_open(rws->bom, 0, size,
			     desc->alignment, domain, 0);
    if (buf->bo == NULL)
	goto error2;

    return &buf->base;

 error2:
    FREE(buf);
 error1:
    return NULL; 
}

static void
radeon_drm_bufmgr_flush(struct pb_manager *mgr)
{
    /* NOP */
}

static void
radeon_drm_bufmgr_destroy(struct pb_manager *_mgr)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    util_hash_table_destroy(mgr->buffer_handles);
    pipe_mutex_destroy(mgr->buffer_map_list_mutex);
    pipe_mutex_destroy(mgr->buffer_handles_mutex);
    FREE(mgr);
}

static unsigned handle_hash(void *key)
{
    return (unsigned)key;
}

static int handle_compare(void *key1, void *key2)
{
    return !((int)key1 == (int)key2);
}

struct pb_manager *
radeon_drm_bufmgr_create(struct radeon_drm_winsys *rws)
{
    struct radeon_drm_bufmgr *mgr;

    mgr = CALLOC_STRUCT(radeon_drm_bufmgr);
    if (!mgr)
	return NULL;

    mgr->base.destroy = radeon_drm_bufmgr_destroy;
    mgr->base.create_buffer = radeon_drm_bufmgr_create_buffer;
    mgr->base.flush = radeon_drm_bufmgr_flush;

    mgr->rws = rws;
    make_empty_list(&mgr->buffer_map_list);
    mgr->buffer_handles = util_hash_table_create(handle_hash, handle_compare);
    pipe_mutex_init(mgr->buffer_map_list_mutex);
    pipe_mutex_init(mgr->buffer_handles_mutex);
    return &mgr->base;
}

static struct radeon_drm_buffer *get_drm_buffer(struct pb_buffer *_buf)
{
    struct radeon_drm_buffer *buf = NULL;

    if (_buf->vtbl == &radeon_drm_buffer_vtbl) {
        buf = radeon_drm_buffer(_buf);
    } else {
	struct pb_buffer *base_buf;
	pb_size offset;
	pb_get_base_buffer(_buf, &base_buf, &offset);

        if (base_buf->vtbl == &radeon_drm_buffer_vtbl)
            buf = radeon_drm_buffer(base_buf);
    }

    return buf;
}

static void *radeon_drm_buffer_map(struct r300_winsys_screen *ws,
                                   struct r300_winsys_buffer *buf,
                                   struct r300_winsys_cs *cs,
                                   enum pipe_transfer_usage usage)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);

    return pb_map(_buf, get_pb_usage_from_transfer_flags(usage), radeon_drm_cs(cs));
}

static void radeon_drm_buffer_unmap(struct r300_winsys_screen *ws,
                                    struct r300_winsys_buffer *buf)
{
    struct pb_buffer *_buf = radeon_pb_buffer(buf);

    pb_unmap(_buf);
}

boolean radeon_drm_bufmgr_get_handle(struct pb_buffer *_buf,
				     struct winsys_handle *whandle)
{
    struct drm_gem_flink flink;
    struct radeon_drm_buffer *buf = get_drm_buffer(_buf);

    if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) {
	if (!buf->flinked) {
	    flink.handle = buf->bo->handle;

            if (ioctl(buf->mgr->rws->fd, DRM_IOCTL_GEM_FLINK, &flink)) {
		return FALSE;
	    }

	    buf->flinked = TRUE;
	    buf->flink = flink.name;
	}
	whandle->handle = buf->flink;
    } else if (whandle->type == DRM_API_HANDLE_TYPE_KMS) {
	whandle->handle = buf->bo->handle;
    }
    return TRUE;
}

static void radeon_drm_buffer_get_tiling(struct r300_winsys_screen *ws,
                                         struct r300_winsys_buffer *_buf,
                                         enum r300_buffer_tiling *microtiled,
                                         enum r300_buffer_tiling *macrotiled)
{
    struct radeon_drm_buffer *buf = get_drm_buffer(radeon_pb_buffer(_buf));
    uint32_t flags = 0, pitch;

    radeon_bo_get_tiling(buf->bo, &flags, &pitch);

    *microtiled = R300_BUFFER_LINEAR;
    *macrotiled = R300_BUFFER_LINEAR;
    if (flags & RADEON_BO_FLAGS_MICRO_TILE)
	*microtiled = R300_BUFFER_TILED;

    if (flags & RADEON_BO_FLAGS_MACRO_TILE)
	*macrotiled = R300_BUFFER_TILED;
}

static void radeon_drm_buffer_set_tiling(struct r300_winsys_screen *ws,
                                         struct r300_winsys_buffer *_buf,
                                         enum r300_buffer_tiling microtiled,
                                         enum r300_buffer_tiling macrotiled,
                                         uint32_t pitch)
{
    struct radeon_drm_buffer *buf = get_drm_buffer(radeon_pb_buffer(_buf));
    uint32_t flags = 0;
    if (microtiled == R300_BUFFER_TILED)
        flags |= RADEON_BO_FLAGS_MICRO_TILE;
/* XXX Remove this ifdef when libdrm version 2.4.19 becomes mandatory. */
#ifdef RADEON_BO_FLAGS_MICRO_TILE_SQUARE
    else if (microtiled == R300_BUFFER_SQUARETILED)
        flags |= RADEON_BO_FLAGS_MICRO_TILE_SQUARE;
#endif
    if (macrotiled == R300_BUFFER_TILED)
        flags |= RADEON_BO_FLAGS_MACRO_TILE;

    radeon_bo_set_tiling(buf->bo, flags, pitch);
}

static struct r300_winsys_cs_buffer *radeon_drm_get_cs_handle(
        struct r300_winsys_screen *rws,
        struct r300_winsys_buffer *_buf)
{
    /* return pure radeon_bo. */
    return (struct r300_winsys_cs_buffer*)
            get_drm_buffer(radeon_pb_buffer(_buf))->bo;
}

static boolean radeon_drm_is_buffer_referenced(struct r300_winsys_cs *rcs,
                                               struct r300_winsys_cs_buffer *_buf,
                                               enum r300_reference_domain domain)
{
    struct radeon_bo *bo = (struct radeon_bo*)_buf;
    uint32_t tmp;

    if (domain & R300_REF_CS) {
        if (radeon_bo_is_referenced_by_cs(bo, NULL)) {
            return TRUE;
        }
    }

    if (domain & R300_REF_HW) {
        if (radeon_bo_is_busy(bo, &tmp)) {
            return TRUE;
        }
    }

    return FALSE;
}

void radeon_drm_bufmgr_flush_maps(struct pb_manager *_mgr)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    struct radeon_drm_buffer *rpb, *t_rpb;

    pipe_mutex_lock(mgr->buffer_map_list_mutex);

    foreach_s(rpb, t_rpb, &mgr->buffer_map_list) {
	radeon_bo_unmap(rpb->bo);
	rpb->bo->ptr = NULL;
	remove_from_list(rpb);
    }

    make_empty_list(&mgr->buffer_map_list);

    pipe_mutex_unlock(mgr->buffer_map_list_mutex);
}

static void radeon_drm_buffer_wait(struct r300_winsys_screen *ws,
                                   struct r300_winsys_buffer *_buf)
{
    struct radeon_drm_buffer *buf = get_drm_buffer(radeon_pb_buffer(_buf));

    radeon_bo_wait(buf->bo);
}

void radeon_drm_bufmgr_init_functions(struct radeon_drm_winsys *ws)
{
    ws->base.buffer_get_cs_handle = radeon_drm_get_cs_handle;
    ws->base.buffer_set_tiling = radeon_drm_buffer_set_tiling;
    ws->base.buffer_get_tiling = radeon_drm_buffer_get_tiling;
    ws->base.buffer_map = radeon_drm_buffer_map;
    ws->base.buffer_unmap = radeon_drm_buffer_unmap;
    ws->base.buffer_wait = radeon_drm_buffer_wait;
    ws->base.cs_is_buffer_referenced = radeon_drm_is_buffer_referenced;
}
