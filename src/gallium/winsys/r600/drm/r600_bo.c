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
#include <pipe/p_compiler.h>
#include <pipe/p_screen.h>
#include <pipebuffer/pb_bufmgr.h>
#include "state_tracker/drm_driver.h"
#include "r600_priv.h"
#include "r600d.h"
#include "drm.h"
#include "radeon_drm.h"

struct r600_bo *r600_bo(struct radeon *radeon,
			unsigned size, unsigned alignment,
			unsigned binding, unsigned usage)
{
	struct r600_bo *ws_bo = calloc(1, sizeof(struct r600_bo));
	struct pb_desc desc;
	struct pb_manager *man;

	desc.alignment = alignment;
	desc.usage = (PB_USAGE_CPU_READ_WRITE | PB_USAGE_GPU_READ_WRITE);
	ws_bo->size = size;

	if (binding & (PIPE_BIND_CONSTANT_BUFFER | PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
		man = radeon->cman;
	else
		man = radeon->kman;

	/* Staging resources particpate in transfers and blits only
	 * and are used for uploads and downloads from regular
	 * resources.  We generate them internally for some transfers.
	 */
	if (usage == PIPE_USAGE_STAGING)
                ws_bo->domains = RADEON_GEM_DOMAIN_CPU | RADEON_GEM_DOMAIN_GTT;
        else
                ws_bo->domains = (RADEON_GEM_DOMAIN_CPU |
                                  RADEON_GEM_DOMAIN_GTT |
                                  RADEON_GEM_DOMAIN_VRAM);


	ws_bo->pb = man->create_buffer(man, size, &desc);
	if (ws_bo->pb == NULL) {
		free(ws_bo);
		return NULL;
	}

	pipe_reference_init(&ws_bo->reference, 1);
	return ws_bo;
}

struct r600_bo *r600_bo_handle(struct radeon *radeon,
			       unsigned handle, unsigned *array_mode)
{
	struct r600_bo *ws_bo = calloc(1, sizeof(struct r600_bo));
	struct radeon_bo *bo;

	ws_bo->pb = radeon_bo_pb_create_buffer_from_handle(radeon->kman, handle);
	if (!ws_bo->pb) {
		free(ws_bo);
		return NULL;
	}
	bo = radeon_bo_pb_get_bo(ws_bo->pb);
	ws_bo->size = bo->size;
	ws_bo->domains = (RADEON_GEM_DOMAIN_CPU |
			  RADEON_GEM_DOMAIN_GTT |
			  RADEON_GEM_DOMAIN_VRAM);

	pipe_reference_init(&ws_bo->reference, 1);

	radeon_bo_get_tiling_flags(radeon, bo, &ws_bo->tiling_flags,
				   &ws_bo->kernel_pitch);
	if (array_mode) {
		if (ws_bo->tiling_flags) {
			if (ws_bo->tiling_flags & RADEON_TILING_MICRO)
				*array_mode = V_0280A0_ARRAY_1D_TILED_THIN1;
			if ((ws_bo->tiling_flags & (RADEON_TILING_MICRO | RADEON_TILING_MACRO)) ==
			    (RADEON_TILING_MICRO | RADEON_TILING_MACRO))
				*array_mode = V_0280A0_ARRAY_2D_TILED_THIN1;
		} else {
			*array_mode = 0;
		}
	}
	return ws_bo;
}

void *r600_bo_map(struct radeon *radeon, struct r600_bo *bo, unsigned usage, void *ctx)
{
	return pb_map(bo->pb, usage, ctx);
}

void r600_bo_unmap(struct radeon *radeon, struct r600_bo *bo)
{
	pb_unmap(bo->pb);
}

static void r600_bo_destroy(struct radeon *radeon, struct r600_bo *bo)
{
	if (bo->pb)
		pb_reference(&bo->pb, NULL);
	free(bo);
}

void r600_bo_reference(struct radeon *radeon, struct r600_bo **dst,
			    struct r600_bo *src)
{
	struct r600_bo *old = *dst;
 		
	if (pipe_reference(&(*dst)->reference, &src->reference)) {
		r600_bo_destroy(radeon, old);
	}
	*dst = src;
}

unsigned r600_bo_get_handle(struct r600_bo *pb_bo)
{
	struct radeon_bo *bo;

	bo = radeon_bo_pb_get_bo(pb_bo->pb);
	if (!bo)
		return 0;

	return bo->handle;
}

unsigned r600_bo_get_size(struct r600_bo *pb_bo)
{
	struct radeon_bo *bo;

	bo = radeon_bo_pb_get_bo(pb_bo->pb);
	if (!bo)
		return 0;

	return bo->size;
}

boolean r600_bo_get_winsys_handle(struct radeon *radeon, struct r600_bo *pb_bo,
				unsigned stride, struct winsys_handle *whandle)
{
	struct radeon_bo *bo;

	bo = radeon_bo_pb_get_bo(pb_bo->pb);
	if (!bo)
		return FALSE;

	whandle->stride = stride;
	switch(whandle->type) {
	case DRM_API_HANDLE_TYPE_KMS:
		whandle->handle = r600_bo_get_handle(pb_bo);
		break;
	case DRM_API_HANDLE_TYPE_SHARED:
		if (radeon_bo_get_name(radeon, bo, &whandle->handle))
			return FALSE;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}
