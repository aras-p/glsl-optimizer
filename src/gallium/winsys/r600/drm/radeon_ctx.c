/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 *      Jerome Glisse
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "radeon_priv.h"
#include "radeon_drm.h"
#include "bof.h"

static int radeon_ctx_set_bo_new(struct radeon_ctx *ctx, struct radeon_bo *bo, unsigned state_id)
{
	ctx->bo[ctx->nbo].bo = bo;
	ctx->bo[ctx->nbo].bo_flushed = 0;
	ctx->bo[ctx->nbo].state_id = state_id;
	ctx->nbo++;
	return 0;
}

void radeon_ctx_clear(struct radeon_ctx *ctx)
{
	unsigned i;

	/* FIXME somethings is wrong, it should be safe to
	 * delete bo here, kernel should postpone bo deletion
	 * until bo is no longer referenced by cs (through the
	 * fence association)
	 */
	for (i = 0; i < 50; i++) {
		usleep(10);
	}
	for (i = 0; i < ctx->nbo; i++) {
		ctx->bo[i].bo = radeon_bo_decref(ctx->radeon, ctx->bo[i].bo);
	}
	ctx->id = 0;
	ctx->npm4 = RADEON_CTX_MAX_PM4;
	ctx->nreloc = 0;
	ctx->nbo = 0;
	memset(ctx->state_crc32, 0, ctx->radeon->nstate * 4);
}

struct radeon_ctx *radeon_ctx(struct radeon *radeon)
{
	struct radeon_ctx *ctx;

	if (radeon == NULL)
		return NULL;
	ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL)
		return NULL;
	ctx->radeon = radeon_incref(radeon);
	ctx->max_bo = 4096;
	ctx->max_reloc = 4096;
	ctx->pm4 = malloc(RADEON_CTX_MAX_PM4 * 4);
	if (ctx->pm4 == NULL) {
		return radeon_ctx_decref(ctx);
	}
	ctx->state_crc32 = malloc(ctx->radeon->nstate * 4);
	if (ctx->state_crc32 == NULL) {
		return radeon_ctx_decref(ctx);
	}
	ctx->bo = malloc(ctx->max_bo * sizeof(struct radeon_ctx_bo));
	if (ctx->bo == NULL) {
		return radeon_ctx_decref(ctx);
	}
	ctx->reloc = malloc(ctx->max_reloc * sizeof(struct radeon_cs_reloc));
	if (ctx->reloc == NULL) {
		return radeon_ctx_decref(ctx);
	}
	radeon_ctx_clear(ctx);
	return ctx;
}

struct radeon_ctx *radeon_ctx_incref(struct radeon_ctx *ctx)
{
	ctx->refcount++;
	return ctx;
}

struct radeon_ctx *radeon_ctx_decref(struct radeon_ctx *ctx)
{
	if (ctx == NULL)
		return NULL;
	if (--ctx->refcount > 0) {
		return NULL;
	}

	ctx->radeon = radeon_decref(ctx->radeon);
	free(ctx->bo);
	free(ctx->pm4);
	free(ctx->reloc);
	free(ctx->state_crc32);
	memset(ctx, 0, sizeof(*ctx));
	free(ctx);
	return NULL;
}

static int radeon_ctx_bo_id(struct radeon_ctx *ctx, struct radeon_bo *bo)
{
	unsigned i;

	for (i = 0; i < ctx->nbo; i++) {
		if (bo == ctx->bo[i].bo)
			return i;
	}
	return -1;
}

static int radeon_ctx_state_bo(struct radeon_ctx *ctx, struct radeon_state *state)
{
	unsigned i, j;
	int r;

	if (state == NULL)
		return 0;
	for (i = 0; i < state->nbo; i++) {
		for (j = 0; j < ctx->nbo; j++) {
			if (state->bo[i] == ctx->bo[j].bo)
				break;
		}
		if (j == ctx->nbo) {
			if (ctx->nbo >= ctx->max_bo) {
				return -EBUSY;
			}
			radeon_bo_incref(ctx->radeon, state->bo[i]);
			r = radeon_ctx_set_bo_new(ctx, state->bo[i], state->id);
			if (r)
				return r;
		}
	}
	return 0;
}

int radeon_ctx_submit(struct radeon_ctx *ctx)
{
	struct drm_radeon_cs drmib;
	struct drm_radeon_cs_chunk chunks[2];
	uint64_t chunk_array[2];
	int r = 0;

	if (!ctx->id)
		return 0;
#if 0
	for (r = 0; r < ctx->id; r++) {
		fprintf(stderr, "0x%08X\n", ctx->pm4[r]);
	}
#endif
	drmib.num_chunks = 2;
	drmib.chunks = (uint64_t)(uintptr_t)chunk_array;
	chunks[0].chunk_id = RADEON_CHUNK_ID_IB;
	chunks[0].length_dw = ctx->id;
	chunks[0].chunk_data = (uint64_t)(uintptr_t)ctx->pm4;
	chunks[1].chunk_id = RADEON_CHUNK_ID_RELOCS;
	chunks[1].length_dw = ctx->nreloc * sizeof(struct radeon_cs_reloc) / 4;
	chunks[1].chunk_data = (uint64_t)(uintptr_t)ctx->reloc;
	chunk_array[0] = (uint64_t)(uintptr_t)&chunks[0];
	chunk_array[1] = (uint64_t)(uintptr_t)&chunks[1];
#if 1
	r = drmCommandWriteRead(ctx->radeon->fd, DRM_RADEON_CS, &drmib,
				sizeof(struct drm_radeon_cs));
#endif
	return r;
}

int radeon_ctx_reloc(struct radeon_ctx *ctx, struct radeon_bo *bo,
			unsigned id, unsigned *placement)
{
	unsigned i;

	for (i = 0; i < ctx->nreloc; i++) {
		if (ctx->reloc[i].handle == bo->handle) {
			ctx->pm4[id] = i * sizeof(struct radeon_cs_reloc) / 4;
			return 0;
		}
	}
	if (ctx->nreloc >= ctx->max_reloc) {
		return -EBUSY;
	}
	ctx->reloc[ctx->nreloc].handle = bo->handle;
	ctx->reloc[ctx->nreloc].read_domain = placement[0] | placement [1];
	ctx->reloc[ctx->nreloc].write_domain = placement[0] | placement [1];
	ctx->reloc[ctx->nreloc].flags = 0;
	ctx->pm4[id] = ctx->nreloc * sizeof(struct radeon_cs_reloc) / 4;
	ctx->nreloc++;
	return 0;
}

static int radeon_ctx_state_schedule(struct radeon_ctx *ctx, struct radeon_state *state)
{
	unsigned i, rid, cid;
	u32 flags;
	int r, bo_id[4];

	if (state == NULL)
		return 0;
	for (i = 0; i < state->nbo; i++) {
		bo_id[i] = radeon_ctx_bo_id(ctx, state->bo[i]);
		if (bo_id[i] < 0) {
			return -EINVAL;
		}
		flags = (~ctx->bo[bo_id[i]].bo_flushed) & ctx->radeon->type[state->id].flush_flags;
		if (flags) {
			r = ctx->radeon->bo_flush(ctx, state->bo[i], flags, &state->placement[i * 2]);
			if (r) {
				return r;
			}
		}
		ctx->bo[bo_id[i]].bo_flushed |= ctx->radeon->type[state->id].flush_flags;
	}
	if ((ctx->radeon->type[state->id].header_cpm4 + state->cpm4) > ctx->npm4) {
		/* need to flush */
		return -EBUSY;
	}
	memcpy(&ctx->pm4[ctx->id], ctx->radeon->type[state->id].header_pm4, ctx->radeon->type[state->id].header_cpm4 * 4);
	ctx->id += ctx->radeon->type[state->id].header_cpm4;
	ctx->npm4 -= ctx->radeon->type[state->id].header_cpm4;
	memcpy(&ctx->pm4[ctx->id], state->states, state->cpm4 * 4);
	for (i = 0; i < state->nbo; i++) {
		rid = state->reloc_pm4_id[i];
		cid = ctx->id + rid;
		r = radeon_ctx_reloc(ctx, state->bo[i], cid,
					&state->placement[i * 2]);
		if (r) {
			fprintf(stderr, "%s state %d failed to reloc\n", __func__, state->id);
			return r;
		}
	}
	ctx->id += state->cpm4;
	ctx->npm4 -= state->cpm4;
	for (i = 0; i < state->nbo; i++) {
		ctx->bo[bo_id[i]].bo_flushed &= ~ctx->radeon->type[state->id].dirty_flags;
	}
	return 0;
}

int radeon_ctx_set_query_state(struct radeon_ctx *ctx, struct radeon_state *state)
{
	unsigned ndw = 0;
	int r = 0;

	r = radeon_state_pm4(state);
	if (r)
		return r;

	/* !!! ONLY ACCEPT QUERY STATE HERE !!! */
	ndw = state->cpm4 + ctx->radeon->type[state->id].header_cpm4;
	switch (state->id) {
	case R600_QUERY_BEGIN:
		/* account QUERY_END at same time of QUERY_BEGIN so we know we
		 * have room left for QUERY_END
		 */
		if ((ndw * 2) > ctx->npm4) {
			/* need to flush */
			return -EBUSY;
		}
		ctx->npm4 -= ndw;
		break;
	case R600_QUERY_END:
		/* add again ndw from previous accounting */
		ctx->npm4 += ndw;
		break;
	default:
		return -EINVAL;
	}

	return radeon_ctx_state_schedule(ctx, state);
}

int radeon_ctx_set_draw(struct radeon_ctx *ctx, struct radeon_draw *draw)
{
	unsigned i, previous_id;
	int r = 0;

	for (i = 0; i < draw->nstate; i++) {
		r = radeon_ctx_state_bo(ctx, draw->state[i]);
		if (r)
			return r;
	}
	r = radeon_draw_check(draw);
	if (r)
		return r;
	if (draw->cpm4 >= RADEON_CTX_MAX_PM4) {
		fprintf(stderr, "%s single draw too big %d, max %d\n",
			__func__, draw->cpm4, RADEON_CTX_MAX_PM4);
		return -EINVAL;
	}
	previous_id = ctx->id;
	for (i = 0; i < draw->nstate; i++) {
		/* FIXME always force draw state to schedule */
		if (draw->state[i] && draw->state[i]->pm4_crc != ctx->state_crc32[draw->state[i]->id]) {
			r = radeon_ctx_state_schedule(ctx, draw->state[i]);
			if (r) {
				ctx->id = previous_id;
				return r;
			}
		}
	}
	return 0;
}

void radeon_ctx_dump_bof(struct radeon_ctx *ctx, const char *file)
{
	bof_t *bcs, *blob, *array, *bo, *size, *handle, *device_id, *root;
	unsigned i;

	root = device_id = bcs = blob = array = bo = size = handle = NULL;
	root = bof_object();
	if (root == NULL)
		goto out_err;
	device_id = bof_int32(ctx->radeon->device);
	if (device_id == NULL)
		return;
	if (bof_object_set(root, "device_id", device_id))
		goto out_err;
	bof_decref(device_id);
	device_id = NULL;
	/* dump relocs */
printf("%d relocs\n", ctx->nreloc);
	blob = bof_blob(ctx->nreloc * 16, ctx->reloc);
	if (blob == NULL)
		goto out_err;
	if (bof_object_set(root, "reloc", blob))
		goto out_err;
	bof_decref(blob);
	blob = NULL;
	/* dump cs */
printf("%d pm4\n", ctx->id);
	blob = bof_blob(ctx->id * 4, ctx->pm4);
	if (blob == NULL)
		goto out_err;
	if (bof_object_set(root, "pm4", blob))
		goto out_err;
	bof_decref(blob);
	blob = NULL;
	/* dump bo */
	array = bof_array();
	if (array == NULL)
		goto out_err;
	for (i = 0; i < ctx->nbo; i++) {
		bo = bof_object();
		if (bo == NULL)
			goto out_err;
		size = bof_int32(ctx->bo[i].bo->size);
		if (size == NULL)
			goto out_err;
		if (bof_object_set(bo, "size", size))
			goto out_err;
		bof_decref(size);
		size = NULL;
		handle = bof_int32(ctx->bo[i].bo->handle);
		if (handle == NULL)
			goto out_err;
		if (bof_object_set(bo, "handle", handle))
			goto out_err;
		bof_decref(handle);
		handle = NULL;
		radeon_bo_map(ctx->radeon, ctx->bo[i].bo);
		blob = bof_blob(ctx->bo[i].bo->size, ctx->bo[i].bo->data);
		radeon_bo_unmap(ctx->radeon, ctx->bo[i].bo);
		if (blob == NULL)
			goto out_err;
		if (bof_object_set(bo, "data", blob))
			goto out_err;
		bof_decref(blob);
		blob = NULL;
		if (bof_array_append(array, bo))
			goto out_err;
		bof_decref(bo);
		bo = NULL;
	}
	if (bof_object_set(root, "bo", array))
		goto out_err;
	bof_dump_file(root, file);
printf("done dump\n");
out_err:
	bof_decref(blob);
	bof_decref(array);
	bof_decref(bo);
	bof_decref(size);
	bof_decref(handle);
	bof_decref(device_id);
	bof_decref(root);
}
