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
#include "r600_priv.h"
#include "r600d.h"
#include "state_tracker/drm_driver.h"
#include "radeon_drm.h"

struct r600_bo *r600_bo(struct radeon *radeon,
			unsigned size, unsigned alignment,
			unsigned binding, unsigned usage)
{
	struct r600_bo *bo;
	struct pb_buffer *pb;
	uint32_t initial_domain, domains;
	  
	/* Staging resources particpate in transfers and blits only
	 * and are used for uploads and downloads from regular
	 * resources.  We generate them internally for some transfers.
	 */
	if (usage == PIPE_USAGE_STAGING) {
		domains = RADEON_GEM_DOMAIN_GTT;
		initial_domain = RADEON_GEM_DOMAIN_GTT;
	} else {
		domains = RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM;

		switch(usage) {
		case PIPE_USAGE_DYNAMIC:
		case PIPE_USAGE_STREAM:
		case PIPE_USAGE_STAGING:
			initial_domain = RADEON_GEM_DOMAIN_GTT;
			break;
		case PIPE_USAGE_DEFAULT:
		case PIPE_USAGE_STATIC:
		case PIPE_USAGE_IMMUTABLE:
		default:
			initial_domain = RADEON_GEM_DOMAIN_VRAM;
			break;
		}
	}

	pb = radeon->ws->buffer_create(radeon->ws, size, alignment, binding, initial_domain);
	if (!pb) {
		return NULL;
	}

	bo = calloc(1, sizeof(struct r600_bo));
	bo->domains = domains;
	bo->buf = pb;
	bo->cs_buf = radeon->ws->buffer_get_cs_handle(pb);

	pipe_reference_init(&bo->reference, 1);
	return bo;
}

struct r600_bo *r600_bo_handle(struct radeon *radeon, struct winsys_handle *whandle,
			       unsigned *stride, unsigned *array_mode)
{
	struct pb_buffer *pb;
	struct r600_bo *bo = calloc(1, sizeof(struct r600_bo));

	pb = bo->buf = radeon->ws->buffer_from_handle(radeon->ws, whandle, stride, NULL);
	if (!pb) {
		free(bo);
		return NULL;
	}

	pipe_reference_init(&bo->reference, 1);
	bo->domains = RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM;
	bo->cs_buf = radeon->ws->buffer_get_cs_handle(pb);

	if (stride)
		*stride = whandle->stride;

	if (array_mode) {
		enum radeon_bo_layout micro, macro;

		radeon->ws->buffer_get_tiling(bo->buf, &micro, &macro);

		if (macro == RADEON_LAYOUT_TILED)
			*array_mode = V_0280A0_ARRAY_2D_TILED_THIN1;
		else if (micro == RADEON_LAYOUT_TILED)
			*array_mode = V_0280A0_ARRAY_1D_TILED_THIN1;
		else
			*array_mode = 0;
	}
	return bo;
}

void *r600_bo_map(struct radeon *radeon, struct r600_bo *bo, struct radeon_winsys_cs *cs, unsigned usage)
{
	return radeon->ws->buffer_map(bo->buf, cs, usage);
}

void r600_bo_unmap(struct radeon *radeon, struct r600_bo *bo)
{
	radeon->ws->buffer_unmap(bo->buf);
}

void r600_bo_destroy(struct radeon *radeon, struct r600_bo *bo)
{
	pb_reference(&bo->buf, NULL);
	free(bo);
}

boolean r600_bo_get_winsys_handle(struct radeon *radeon, struct r600_bo *bo,
				  unsigned stride, struct winsys_handle *whandle)
{
	return radeon->ws->buffer_get_handle(bo->buf, stride, whandle);
}
