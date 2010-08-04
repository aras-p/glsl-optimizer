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

int radeon_ctx_set_bo_new(struct radeon_ctx *ctx, struct radeon_bo *bo)
{
	if (ctx->nbo >= 2048)
		return -EBUSY;
	ctx->bo[ctx->nbo] = bo;
	ctx->nbo++;
	return 0;
}

struct radeon_bo *radeon_ctx_get_bo(struct radeon_ctx *ctx, unsigned reloc)
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

void radeon_ctx_get_placement(struct radeon_ctx *ctx, unsigned reloc, u32 *placement)
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

static void radeon_ctx_clear(struct radeon_ctx *ctx)
{
	ctx->draw_cpm4 = 0;
	ctx->cpm4 = 0;
	ctx->ndraw = 0;
	ctx->nbo = 0;
	ctx->nreloc = 0;
}

int radeon_ctx_init(struct radeon_ctx *ctx, struct radeon *radeon)
{
	memset(ctx, 0, sizeof(struct radeon_ctx));
	ctx->radeon = radeon_incref(radeon);
	radeon_ctx_clear(ctx);
	free(ctx->pm4);
	ctx->cpm4 = 0;
	ctx->pm4 = malloc(64 * 1024);
	if (ctx->pm4 == NULL)
		return -ENOMEM;
	return 0;
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
			radeon_bo_incref(ctx->radeon, state->bo[i]);
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

#if 0
	for (r = 0; r < ctx->cpm4; r++) {
		fprintf(stderr, "0x%08X\n", ctx->pm4[r]);
	}
#endif
	drmib.num_chunks = 2;
	drmib.chunks = (uint64_t)(uintptr_t)chunk_array;
	chunks[0].chunk_id = RADEON_CHUNK_ID_IB;
	chunks[0].length_dw = ctx->cpm4;
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
	radeon_ctx_clear(ctx);
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
	if (ctx->nreloc >= 2048)
		return -EINVAL;
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
	memcpy(&ctx->pm4[ctx->id], state->pm4, state->cpm4 * 4);
	for (i = 0; i < state->nreloc; i++) {
		rid = state->reloc_pm4_id[i];
		bid = state->reloc_bo_id[i];
		cid = ctx->id + rid;
		r = radeon_ctx_reloc(ctx, state->bo[bid], cid,
					&state->placement[bid * 2]);
		if (r) {
			fprintf(stderr, "%s state %d failed to reloc\n", __func__, state->type);
			return r;
		}
	}
	ctx->id += state->cpm4;
	return 0;
}

int radeon_ctx_set_draw(struct radeon_ctx *ctx, struct radeon_draw *draw)
{
	unsigned cpm4, i;
	int r = 0;

	for (i = 0; i < draw->nstate; i++) {
		r = radeon_ctx_state_bo(ctx, &draw->state[i]);
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
	ctx->draw[ctx->ndraw] = *draw;
	for (i = 0, cpm4 = 0; i < draw->nstate - 1; i++) {
		ctx->draw[ctx->ndraw].state[i].valid &= ~2;
		if (ctx->draw[ctx->ndraw].state[i].valid) {
			if (ctx->ndraw > 1 && ctx->draw[ctx->ndraw - 1].state[i].valid) {
				if (ctx->draw[ctx->ndraw - 1].state[i].pm4_crc == draw->state[i].pm4_crc)
					continue;
			}
			ctx->draw[ctx->ndraw].state[i].valid |= 2;
			cpm4 += ctx->draw[ctx->ndraw].state[i].cpm4;
		}
	}
	/* The last state is the draw state always add it */
	if (!draw->state[i].valid) {
		fprintf(stderr, "%s no draw command\n", __func__);
		return -EINVAL;
	}
	ctx->draw[ctx->ndraw].state[i].valid |= 2;
	cpm4 += ctx->draw[ctx->ndraw].state[i].cpm4;
	if ((ctx->draw_cpm4 + cpm4) > RADEON_CTX_MAX_PM4) {
		/* need to flush */
		return -EBUSY;
	}
	ctx->draw_cpm4 += cpm4;
	ctx->ndraw++;
	return 0;
}

int radeon_ctx_pm4(struct radeon_ctx *ctx)
{
	unsigned i, j, c;
	int r;

	for (i = 0, c = 0, ctx->id = 0; i < ctx->ndraw; i++) {
		for (j = 0; j < ctx->draw[i].nstate; j++) {
			if (ctx->draw[i].state[j].valid & 2) {
				r = radeon_ctx_state_schedule(ctx, &ctx->draw[i].state[j]);
				if (r)
					return r;
				c += ctx->draw[i].state[j].cpm4;
			}
		}
	}
	if (ctx->id != ctx->draw_cpm4) {
		fprintf(stderr, "%s miss predicted pm4 size %d for %d\n",
			__func__, ctx->draw_cpm4, ctx->id);
		return -EINVAL;
	}
	ctx->cpm4 = ctx->draw_cpm4;
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
printf("%d pm4\n", ctx->cpm4);
	blob = bof_blob(ctx->cpm4 * 4, ctx->pm4);
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
printf("[%d] %d bo\n", i, size);
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
