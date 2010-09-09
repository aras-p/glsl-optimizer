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
#include "radeon_priv.h"
#include "radeon_drm.h"
#include "bof.h"

static int radeon_ctx_set_bo_new(struct radeon_ctx *ctx, struct radeon_bo *bo)
{
	if (ctx->nbo >= RADEON_CTX_MAX_PM4)
		return -EBUSY;
	ctx->bo[ctx->nbo] = radeon_bo_incref(ctx->radeon, bo);
	ctx->nbo++;
	return 0;
}

static struct radeon_bo *radeon_ctx_get_bo(struct radeon_ctx *ctx, unsigned reloc)
{
	struct radeon_cs_reloc *greloc;
	unsigned i;

	greloc = (void *)(((u8 *)ctx->reloc) + reloc * 4);
	for (i = 0; i < ctx->nbo; i++) {
		if (ctx->bo[i]->handle == greloc->handle) {
			return radeon_bo_incref(ctx->radeon, ctx->bo[i]);
		}
	}
	fprintf(stderr, "%s no bo for reloc[%d 0x%08X] %d\n", __func__, reloc, greloc->handle, ctx->nbo);
	return NULL;
}

static void radeon_ctx_get_placement(struct radeon_ctx *ctx, unsigned reloc, u32 *placement)
{
	struct radeon_cs_reloc *greloc;
	unsigned i;

	placement[0] = 0;
	placement[1] = 0;
	greloc = (void *)(((u8 *)ctx->reloc) + reloc * 4);
	for (i = 0; i < ctx->nbo; i++) {
		if (ctx->bo[i]->handle == greloc->handle) {
			placement[0] = greloc->read_domain | greloc->write_domain;
			placement[1] = placement[0];
			return;
		}
	}
}

void radeon_ctx_clear(struct radeon_ctx *ctx)
{
	for (int i = 0; i < ctx->nbo; i++) {
		ctx->bo[i] = radeon_bo_decref(ctx->radeon, ctx->bo[i]);
	}
	ctx->ndwords = RADEON_CTX_MAX_PM4;
	ctx->cdwords = 0;
	ctx->nreloc = 0;
	ctx->nbo = 0;
}

int radeon_ctx_init(struct radeon_ctx *ctx, struct radeon *radeon)
{
	if (radeon == NULL)
		return -EINVAL;
	memset(ctx, 0, sizeof(struct radeon_ctx));
	ctx->radeon = radeon_incref(radeon);
	radeon_ctx_clear(ctx);
	ctx->pm4 = malloc(RADEON_CTX_MAX_PM4 * 4);
	if (ctx->pm4 == NULL) {
		radeon_ctx_fini(ctx);
		return -ENOMEM;
	}
	ctx->reloc = malloc(sizeof(struct radeon_cs_reloc) * RADEON_CTX_MAX_PM4);
	if (ctx->reloc == NULL) {
		radeon_ctx_fini(ctx);
		return -ENOMEM;
	}
	ctx->bo = malloc(sizeof(void *) * RADEON_CTX_MAX_PM4);
	if (ctx->bo == NULL) {
		radeon_ctx_fini(ctx);
		return -ENOMEM;
	}
	return 0;
}

void radeon_ctx_fini(struct radeon_ctx *ctx)
{
	unsigned i;

	if (ctx == NULL)
		return;

	for (i = 0; i < ctx->nbo; i++) {
		ctx->bo[i] = radeon_bo_decref(ctx->radeon, ctx->bo[i]);
	}
	ctx->radeon = radeon_decref(ctx->radeon);
	free(ctx->bo);
	free(ctx->pm4);
	free(ctx->reloc);
	memset(ctx, 0, sizeof(struct radeon_ctx));
}

static int radeon_ctx_state_bo(struct radeon_ctx *ctx, struct radeon_state *state)
{
	unsigned i, j;
	int r;

	if (state == NULL)
		return 0;
	for (i = 0; i < state->nbo; i++) {
		for (j = 0; j < ctx->nbo; j++) {
			if (state->bo[i] == ctx->bo[j])
				break;
		}
		if (j == ctx->nbo) {
			r = radeon_ctx_set_bo_new(ctx, state->bo[i]);
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

	if (!ctx->cdwords)
		return 0;
#if 0
	for (r = 0; r < ctx->cdwords; r++) {
		fprintf(stderr, "0x%08X\n", ctx->pm4[r]);
	}
#endif
	drmib.num_chunks = 2;
	drmib.chunks = (uint64_t)(uintptr_t)chunk_array;
	chunks[0].chunk_id = RADEON_CHUNK_ID_IB;
	chunks[0].length_dw = ctx->cdwords;
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

static int radeon_ctx_reloc(struct radeon_ctx *ctx, struct radeon_bo *bo,
			unsigned id, unsigned *placement)
{
	unsigned i;

	for (i = 0; i < ctx->nreloc; i++) {
		if (ctx->reloc[i].handle == bo->handle) {
			ctx->pm4[id] = i * sizeof(struct radeon_cs_reloc) / 4;
			return 0;
		}
	}
	if (ctx->nreloc >= RADEON_CTX_MAX_PM4) {
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
	unsigned i, rid, bid, cid;
	int r;

	if (state == NULL)
		return 0;
	if (state->cpm4 > ctx->ndwords) {
		return -EBUSY;
	}
	memcpy(&ctx->pm4[ctx->cdwords], state->pm4, state->cpm4 * 4);
	for (i = 0; i < state->nreloc; i++) {
		rid = state->reloc_pm4_id[i];
		bid = state->reloc_bo_id[i];
		cid = ctx->cdwords + rid;
		r = radeon_ctx_reloc(ctx, state->bo[bid], cid,
					&state->placement[bid * 2]);
		if (r) {
			fprintf(stderr, "%s state %d failed to reloc\n", __func__, state->stype->stype);
			return r;
		}
	}
	ctx->cdwords += state->cpm4;
	ctx->ndwords -= state->cpm4;
	return 0;
}

int radeon_ctx_set_query_state(struct radeon_ctx *ctx, struct radeon_state *state)
{
	int r = 0;

	/* !!! ONLY ACCEPT QUERY STATE HERE !!! */
	r = radeon_state_pm4(state);
	if (r)
		return r;
	/* BEGIN/END query are balanced in the same cs so account for END
	 * END query when scheduling BEGIN query
	 */
	switch (state->stype->stype) {
	case R600_STATE_QUERY_BEGIN:
		/* is there enough place for begin & end */
		if ((state->cpm4 * 2) > ctx->ndwords)
			return -EBUSY;
		ctx->ndwords -= state->cpm4;
		break;
	case R600_STATE_QUERY_END:
		ctx->ndwords += state->cpm4;
		break;
	default:
		return -EINVAL;
	}
	return radeon_ctx_state_schedule(ctx, state);
}

int radeon_ctx_set_draw(struct radeon_ctx *ctx, struct radeon_draw *draw)
{
	unsigned previous_cdwords;
	int r = 0;
	int i;

	for (i = 0; i < ctx->radeon->max_states; i++) {
		r = radeon_ctx_state_bo(ctx, draw->state[i]);
		if (r)
			return r;
	}
	previous_cdwords = ctx->cdwords;
	for (i = 0; i < ctx->radeon->max_states; i++) {
		if (draw->state[i]) {
			r = radeon_ctx_state_schedule(ctx, draw->state[i]);
			if (r) {
				ctx->cdwords = previous_cdwords;
				return r;
			}
		}
	}

	return 0;
}

#if 0
int radeon_ctx_pm4(struct radeon_ctx *ctx)
{
	unsigned i;
	int r;

	free(ctx->pm4);
	ctx->cpm4 = 0;
	ctx->pm4 = malloc(ctx->draw_cpm4 * 4);
	if (ctx->pm4 == NULL)
		return -EINVAL;
	for (i = 0, ctx->id = 0; i < ctx->nstate; i++) {
	}
	if (ctx->id != ctx->draw_cpm4) {
		fprintf(stderr, "%s miss predicted pm4 size %d for %d\n",
			__func__, ctx->draw_cpm4, ctx->id);
		return -EINVAL;
	}
	ctx->cpm4 = ctx->draw_cpm4;
	return 0;
}
#endif

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
	blob = bof_blob(ctx->nreloc * 16, ctx->reloc);
	if (blob == NULL)
		goto out_err;
	if (bof_object_set(root, "reloc", blob))
		goto out_err;
	bof_decref(blob);
	blob = NULL;
	/* dump cs */
	blob = bof_blob(ctx->cdwords * 4, ctx->pm4);
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
		size = bof_int32(ctx->bo[i]->size);
		if (size == NULL)
			goto out_err;
		if (bof_object_set(bo, "size", size))
			goto out_err;
		bof_decref(size);
		size = NULL;
		handle = bof_int32(ctx->bo[i]->handle);
		if (handle == NULL)
			goto out_err;
		if (bof_object_set(bo, "handle", handle))
			goto out_err;
		bof_decref(handle);
		handle = NULL;
		radeon_bo_map(ctx->radeon, ctx->bo[i]);
		blob = bof_blob(ctx->bo[i]->size, ctx->bo[i]->data);
		radeon_bo_unmap(ctx->radeon, ctx->bo[i]);
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
out_err:
	bof_decref(blob);
	bof_decref(array);
	bof_decref(bo);
	bof_decref(size);
	bof_decref(handle);
	bof_decref(device_id);
	bof_decref(root);
}
