
#include <sys/ioctl.h>
#include "radeon_drm.h"
#include "radeon_bo_gem.h"
#include "radeon_cs_gem.h"
#include "radeon_buffer.h"

#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_bufmgr.h"

#include "radeon_winsys.h"
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
    struct pb_manager base;
    struct radeon_libdrm_winsys *rws;
    struct radeon_drm_buffer buffer_map_list;
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

    if (buf->bo->ptr != NULL) {
	remove_from_list(buf);
	radeon_bo_unmap(buf->bo);
	buf->bo->ptr = NULL;
    }
    radeon_bo_unref(buf->bo);

    FREE(buf);
}

static void *
radeon_drm_buffer_map(struct pb_buffer *_buf,
		      unsigned flags)
{
    struct radeon_drm_buffer *buf = radeon_drm_buffer(_buf);
    int write;

    if (flags & PIPE_BUFFER_USAGE_DONTBLOCK) {
	if ((_buf->base.usage & PIPE_BUFFER_USAGE_VERTEX) ||
	    (_buf->base.usage & PIPE_BUFFER_USAGE_INDEX))
	    if (radeon_bo_is_referenced_by_cs(buf->bo, buf->mgr->rws->cs))
		return NULL;
    }

    if (buf->bo->ptr != NULL)
	return buf->bo->ptr;

    if (flags & PIPE_BUFFER_USAGE_DONTBLOCK) {
        uint32_t domain;
        if (radeon_bo_is_busy(buf->bo, &domain))
            return NULL;
    }

    if (radeon_bo_is_referenced_by_cs(buf->bo, buf->mgr->rws->cs)) {
        buf->mgr->rws->flush_cb(buf->mgr->rws->flush_data);
    }

    if (flags & PIPE_BUFFER_USAGE_CPU_WRITE) {
        write = 1;
    }

    if (radeon_bo_map(buf->bo, write)) {
        return NULL;
    }
    insert_at_tail(&buf->mgr->buffer_map_list, buf);
    return buf->bo->ptr;
}

static void
radeon_drm_buffer_unmap(struct pb_buffer *_buf)
{
    (void)_buf;
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
    radeon_drm_buffer_map,
    radeon_drm_buffer_unmap,
    radeon_drm_buffer_validate,
    radeon_drm_buffer_fence,
    radeon_drm_buffer_get_base_buffer,
};


static uint32_t radeon_domain_from_usage(unsigned usage)
{
    uint32_t domain = 0;

    if (usage & PIPE_BUFFER_USAGE_GPU_WRITE) {
        domain |= RADEON_GEM_DOMAIN_VRAM;
    }
    if (usage & PIPE_BUFFER_USAGE_PIXEL) {
        domain |= RADEON_GEM_DOMAIN_VRAM;
    }
    if (usage & PIPE_BUFFER_USAGE_VERTEX) {
        domain |= RADEON_GEM_DOMAIN_GTT;
    }
    if (usage & PIPE_BUFFER_USAGE_INDEX) {
        domain |= RADEON_GEM_DOMAIN_GTT;
    }

    return domain;
}

struct pb_buffer *radeon_drm_bufmgr_create_buffer_from_handle(struct pb_manager *_mgr,
							      uint32_t handle)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    struct radeon_libdrm_winsys *rws = mgr->rws;
    struct radeon_drm_buffer *buf;
    struct radeon_bo *bo;

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
    buf->base.base.usage = PIPE_BUFFER_USAGE_PIXEL;
    buf->base.base.size = 0;
    buf->base.vtbl = &radeon_drm_buffer_vtbl;
    buf->mgr = mgr;

    buf->bo = bo;

    return &buf->base;
}

static struct pb_buffer *
radeon_drm_bufmgr_create_buffer(struct pb_manager *_mgr,
				pb_size size,
				const struct pb_desc *desc)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    struct radeon_libdrm_winsys *rws = mgr->rws;
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
    domain = radeon_domain_from_usage(desc->usage);
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
    FREE(mgr);
}

struct pb_manager *
radeon_drm_bufmgr_create(struct radeon_libdrm_winsys *rws)
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
    return &mgr->base;
}

static struct radeon_drm_buffer *get_drm_buffer(struct pb_buffer *_buf)
{
    struct radeon_drm_buffer *buf;
    if (_buf->vtbl == &radeon_drm_buffer_vtbl) {
        buf = radeon_drm_buffer(_buf);
    } else {
	struct pb_buffer *base_buf;
	pb_size offset;
	pb_get_base_buffer(_buf, &base_buf, &offset);

	buf = radeon_drm_buffer(base_buf);
    }
    return buf;
}

boolean radeon_drm_bufmgr_get_handle(struct pb_buffer *_buf,
				     struct winsys_handle *whandle)
{
    int retval, fd;
    struct drm_gem_flink flink;
    struct radeon_drm_buffer *buf = get_drm_buffer(_buf);
    if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) {
	if (!buf->flinked) {
	    fd = buf->mgr->rws->fd;
	    flink.handle = buf->bo->handle;

	    retval = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
	    if (retval) {
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
					   

void radeon_drm_bufmgr_set_tiling(struct pb_buffer *_buf,
                                  enum r300_buffer_tiling microtiled,
                                  enum r300_buffer_tiling macrotiled,
                                  uint32_t pitch)
{
    struct radeon_drm_buffer *buf = get_drm_buffer(_buf);
    uint32_t flags = 0, old_flags, old_pitch;
    if (microtiled == R300_BUFFER_TILED)
	flags |= RADEON_BO_FLAGS_MICRO_TILE;
    if (macrotiled == R300_BUFFER_TILED)
	flags |= RADEON_BO_FLAGS_MACRO_TILE;

    radeon_bo_get_tiling(buf->bo, &old_flags, &old_pitch);

    if (flags != old_flags || pitch != old_pitch) {
        /* Tiling determines how DRM treats the buffer data.
         * We must flush CS when changing it if the buffer is referenced. */
        if (radeon_bo_is_referenced_by_cs(buf->bo,  buf->mgr->rws->cs)) {
	    buf->mgr->rws->flush_cb(buf->mgr->rws->flush_data);
        }
    }
    radeon_bo_set_tiling(buf->bo, flags, pitch);

}

boolean radeon_drm_bufmgr_add_buffer(struct pb_buffer *_buf,
				     uint32_t rd, uint32_t wd)
{
    struct radeon_drm_buffer *buf = get_drm_buffer(_buf);
    radeon_cs_space_add_persistent_bo(buf->mgr->rws->cs, buf->bo,
					  rd, wd);
    return TRUE;
}

void radeon_drm_bufmgr_write_reloc(struct pb_buffer *_buf,
				   uint32_t rd, uint32_t wd,
				   uint32_t flags)
{
    struct radeon_drm_buffer *buf = get_drm_buffer(_buf);
    int retval;

    retval = radeon_cs_write_reloc(buf->mgr->rws->cs,
				   buf->bo, rd, wd, flags);
    if (retval) {
        debug_printf("radeon: Relocation of %p (%d, %d, %d) failed!\n",
		     buf, rd, wd, flags);
    }
}

boolean radeon_drm_bufmgr_is_buffer_referenced(struct pb_buffer *_buf)
{
    struct radeon_drm_buffer *buf = get_drm_buffer(_buf);
    uint32_t domain;

    return (radeon_bo_is_referenced_by_cs(buf->bo, buf->mgr->rws->cs) ||
	    radeon_bo_is_busy(buf->bo, &domain));
}
					    

void radeon_drm_bufmgr_flush_maps(struct pb_manager *_mgr)
{
    struct radeon_drm_bufmgr *mgr = radeon_drm_bufmgr(_mgr);
    struct radeon_drm_buffer *rpb, *t_rpb;

    foreach_s(rpb, t_rpb, &mgr->buffer_map_list) {
	radeon_bo_unmap(rpb->bo);
	rpb->bo->ptr = NULL;
	remove_from_list(rpb);
    }

    make_empty_list(&mgr->buffer_map_list);
}
