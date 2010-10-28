/*
 * Copyright 2010 Dave Airlie
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Dave Airlie
 */
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include <util/u_double_list.h>
#include <pipebuffer/pb_buffer.h>
#include <pipebuffer/pb_bufmgr.h>
#include "r600_priv.h"

struct radeon_bo_pb {
	struct pb_buffer b;
	struct radeon_bo *bo;

	struct radeon_bo_pbmgr *mgr;
	struct list_head maplist;
};

extern const struct pb_vtbl radeon_bo_pb_vtbl;

static INLINE struct radeon_bo_pb *radeon_bo_pb(struct pb_buffer *buf)
{
	assert(buf);
	assert(buf->vtbl == &radeon_bo_pb_vtbl);
	return (struct radeon_bo_pb *)buf;
}

struct radeon_bo_pbmgr {
	struct pb_manager b;
	struct radeon *radeon;
	struct list_head buffer_map_list;
};

static INLINE struct radeon_bo_pbmgr *radeon_bo_pbmgr(struct pb_manager *mgr)
{
	assert(mgr);
	return (struct radeon_bo_pbmgr *)mgr;
}

static void radeon_bo_pb_destroy(struct pb_buffer *_buf)
{
	struct radeon_bo_pb *buf = radeon_bo_pb(_buf);

	LIST_DEL(&buf->maplist);

	if (buf->bo->data != NULL) {
		radeon_bo_unmap(buf->mgr->radeon, buf->bo);
	}
	radeon_bo_reference(buf->mgr->radeon, &buf->bo, NULL);
	FREE(buf);
}

static void *
radeon_bo_pb_map_internal(struct pb_buffer *_buf,
			  unsigned flags, void *ctx)
{
	struct radeon_bo_pb *buf = radeon_bo_pb(_buf);
	struct pipe_context *pctx = ctx;

	if (flags & PB_USAGE_UNSYNCHRONIZED) {
		if (!buf->bo->data && radeon_bo_map(buf->mgr->radeon, buf->bo)) {
			return NULL;
		}
		LIST_DELINIT(&buf->maplist);
		return buf->bo->data;
	}

	if (p_atomic_read(&buf->bo->reference.count) > 1) {
		if (flags & PB_USAGE_DONTBLOCK) {
			return NULL;
		}
		if (ctx) {
			pctx->flush(pctx, 0, NULL);
		}
	}

	if (flags & PB_USAGE_DONTBLOCK) {
		uint32_t domain;
		if (radeon_bo_busy(buf->mgr->radeon, buf->bo, &domain))
			return NULL;
		if (radeon_bo_map(buf->mgr->radeon, buf->bo)) {
			return NULL;
		}
		goto out;
	}

	if (buf->bo->data != NULL) {
		if (radeon_bo_wait(buf->mgr->radeon, buf->bo)) {
			return NULL;
		}
	} else {
		if (radeon_bo_map(buf->mgr->radeon, buf->bo)) {
			return NULL;
		}
		if (radeon_bo_wait(buf->mgr->radeon, buf->bo)) {
			radeon_bo_unmap(buf->mgr->radeon, buf->bo);
			return NULL;
		}
	}
out:
	LIST_DELINIT(&buf->maplist);
	return buf->bo->data;
}

static void radeon_bo_pb_unmap_internal(struct pb_buffer *_buf)
{
	struct radeon_bo_pb *buf = radeon_bo_pb(_buf);
	LIST_ADDTAIL(&buf->maplist, &buf->mgr->buffer_map_list);
}

static void
radeon_bo_pb_get_base_buffer(struct pb_buffer *buf,
			     struct pb_buffer **base_buf,
			     unsigned *offset)
{
	*base_buf = buf;
	*offset = 0;
}

static enum pipe_error
radeon_bo_pb_validate(struct pb_buffer *_buf, 
		      struct pb_validate *vl,
		      unsigned flags)
{
	/* Always pinned */
	return PIPE_OK;
}

static void
radeon_bo_pb_fence(struct pb_buffer *buf,
		   struct pipe_fence_handle *fence)
{
}

const struct pb_vtbl radeon_bo_pb_vtbl = {
    radeon_bo_pb_destroy,
    radeon_bo_pb_map_internal,
    radeon_bo_pb_unmap_internal,
    radeon_bo_pb_validate,
    radeon_bo_pb_fence,
    radeon_bo_pb_get_base_buffer,
};

struct pb_buffer *
radeon_bo_pb_create_buffer_from_handle(struct pb_manager *_mgr,
				       uint32_t handle)
{
	struct radeon_bo_pbmgr *mgr = radeon_bo_pbmgr(_mgr);
	struct radeon *radeon = mgr->radeon;
	struct radeon_bo_pb *bo;
	struct radeon_bo *hw_bo;

	hw_bo = radeon_bo(radeon, handle, 0, 0);
	if (hw_bo == NULL)
		return NULL;

	bo = CALLOC_STRUCT(radeon_bo_pb);
	if (!bo) {
		radeon_bo_reference(radeon, &hw_bo, NULL);
		return NULL;
	}

	LIST_INITHEAD(&bo->maplist);
	pipe_reference_init(&bo->b.base.reference, 1);
	bo->b.base.alignment = 0;
	bo->b.base.usage = PB_USAGE_GPU_WRITE | PB_USAGE_GPU_READ;
	bo->b.base.size = hw_bo->size;
	bo->b.vtbl = &radeon_bo_pb_vtbl;
	bo->mgr = mgr;

	bo->bo = hw_bo;

	return &bo->b;
}

static struct pb_buffer *
radeon_bo_pb_create_buffer(struct pb_manager *_mgr,
			   pb_size size,
			   const struct pb_desc *desc)
{
	struct radeon_bo_pbmgr *mgr = radeon_bo_pbmgr(_mgr);
	struct radeon *radeon = mgr->radeon;
	struct radeon_bo_pb *bo;

	bo = CALLOC_STRUCT(radeon_bo_pb);
	if (!bo)
		goto error1;

	pipe_reference_init(&bo->b.base.reference, 1);
	bo->b.base.alignment = desc->alignment;
	bo->b.base.usage = desc->usage;
	bo->b.base.size = size;
	bo->b.vtbl = &radeon_bo_pb_vtbl;
	bo->mgr = mgr;

	LIST_INITHEAD(&bo->maplist);

	bo->bo = radeon_bo(radeon, 0, size, desc->alignment);
	if (bo->bo == NULL)
		goto error2;
	return &bo->b;

error2:
	FREE(bo);
error1:
	return NULL;
}

static void
radeon_bo_pbmgr_flush(struct pb_manager *mgr)
{
    /* NOP */
}

static void
radeon_bo_pbmgr_destroy(struct pb_manager *_mgr)
{
	struct radeon_bo_pbmgr *mgr = radeon_bo_pbmgr(_mgr);
	FREE(mgr);
}

struct pb_manager *radeon_bo_pbmgr_create(struct radeon *radeon)
{
	struct radeon_bo_pbmgr *mgr;

	mgr = CALLOC_STRUCT(radeon_bo_pbmgr);
	if (!mgr)
		return NULL;

	mgr->b.destroy = radeon_bo_pbmgr_destroy;
	mgr->b.create_buffer = radeon_bo_pb_create_buffer;
	mgr->b.flush = radeon_bo_pbmgr_flush;

	mgr->radeon = radeon;
	LIST_INITHEAD(&mgr->buffer_map_list);
	return &mgr->b;
}

void radeon_bo_pbmgr_flush_maps(struct pb_manager *_mgr)
{
	struct radeon_bo_pbmgr *mgr = radeon_bo_pbmgr(_mgr);
	struct radeon_bo_pb *rpb = NULL;
	struct radeon_bo_pb *t_rpb;

	LIST_FOR_EACH_ENTRY_SAFE(rpb, t_rpb, &mgr->buffer_map_list, maplist) {
		radeon_bo_unmap(mgr->radeon, rpb->bo);
		LIST_DELINIT(&rpb->maplist);
	}

	LIST_INITHEAD(&mgr->buffer_map_list);
}

struct radeon_bo *radeon_bo_pb_get_bo(struct pb_buffer *_buf)
{
	struct radeon_bo_pb *buf;
	if (_buf->vtbl == &radeon_bo_pb_vtbl) {
		buf = radeon_bo_pb(_buf);
		return buf->bo;
	} else {
		struct pb_buffer *base_buf;
		pb_size offset;
		pb_get_base_buffer(_buf, &base_buf, &offset);
		if (base_buf->vtbl == &radeon_bo_pb_vtbl) {
			buf = radeon_bo_pb(base_buf);
			return buf->bo;
		}
	}
	return NULL;
}
