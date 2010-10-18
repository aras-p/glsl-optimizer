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
#include "r600_priv.h"

struct r600_bo *r600_bo(struct radeon *radeon,
				  unsigned size, unsigned alignment, unsigned usage)
{
	struct r600_bo *ws_bo = calloc(1, sizeof(struct r600_bo));
	struct pb_desc desc;
	struct pb_manager *man;

	desc.alignment = alignment;
	desc.usage = usage;
	ws_bo->size = size;

	if (usage & (PIPE_BIND_CONSTANT_BUFFER | PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
		man = radeon->cman;
	else
		man = radeon->kman;

	ws_bo->pb = man->create_buffer(man, size, &desc);
	if (ws_bo->pb == NULL) {
		free(ws_bo);
		return NULL;
	}

	pipe_reference_init(&ws_bo->reference, 1);
	return ws_bo;
}

struct r600_bo *r600_bo_handle(struct radeon *radeon,
					 unsigned handle)
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
	pipe_reference_init(&ws_bo->reference, 1);
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
