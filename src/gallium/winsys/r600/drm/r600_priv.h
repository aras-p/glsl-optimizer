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
#ifndef R600_PRIV_H
#define R600_PRIV_H

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <pipebuffer/pb_bufmgr.h>
#include "r600.h"


struct radeon {
	int				fd;
	int				refcount;
	unsigned			device;
	unsigned			family;
	enum chip_class			chip_class;
	boolean				use_mem_constant; /* true for evergreen */
	struct pb_manager *mman; /* malloc manager */
	struct pb_manager *kman; /* kernel bo manager */
	struct pb_manager *cman; /* cached bo manager */
};

struct radeon *r600_new(int fd, unsigned device);
void r600_delete(struct radeon *r600);

struct r600_reg {
	unsigned			opcode;
	unsigned			offset_base;
	unsigned			offset;
	unsigned			need_bo;
	unsigned			flush_flags;
};

struct radeon_bo {
	struct pipe_reference		reference;
	unsigned			handle;
	unsigned			size;
	unsigned			alignment;
	unsigned			map_count;
	void				*data;
};

struct radeon_ws_bo {
	struct pipe_reference		reference;
	struct pb_buffer		*pb;
};


/* radeon_pciid.c */
unsigned radeon_family_from_device(unsigned device);

/* r600_drm.c */
struct radeon *radeon_decref(struct radeon *radeon);

/* radeon_bo.c */
struct radeon_bo *radeon_bo_pb_get_bo(struct pb_buffer *_buf);
void r600_context_bo_reloc(struct r600_context *ctx, u32 *pm4, struct radeon_bo *bo);
struct radeon_bo *radeon_bo(struct radeon *radeon, unsigned handle,
			    unsigned size, unsigned alignment, void *ptr);
int radeon_bo_map(struct radeon *radeon, struct radeon_bo *bo);
void radeon_bo_unmap(struct radeon *radeon, struct radeon_bo *bo);
void radeon_bo_reference(struct radeon *radeon, struct radeon_bo **dst,
			 struct radeon_bo *src);
int radeon_bo_wait(struct radeon *radeon, struct radeon_bo *bo);
int radeon_bo_busy(struct radeon *radeon, struct radeon_bo *bo, uint32_t *domain);
void radeon_bo_pbmgr_flush_maps(struct pb_manager *_mgr);

/* radeon_bo_pb.c */
struct pb_manager *radeon_bo_pbmgr_create(struct radeon *radeon);
struct pb_buffer *radeon_bo_pb_create_buffer_from_handle(struct pb_manager *_mgr,
							 uint32_t handle);

/* radeon_ws_bo.c */
unsigned radeon_ws_bo_get_handle(struct radeon_ws_bo *bo);
unsigned radeon_ws_bo_get_size(struct radeon_ws_bo *bo);

#define CTX_RANGE_ID(ctx, offset) (((offset) >> (ctx)->hash_shift) & 255)
#define CTX_BLOCK_ID(ctx, offset) ((offset) & ((1 << (ctx)->hash_shift) - 1))

static void inline r600_context_reg(struct r600_context *ctx,
					unsigned offset, unsigned value,
					unsigned mask)
{
	struct r600_range *range;
	struct r600_block *block;
	unsigned id;

	range = &ctx->range[CTX_RANGE_ID(ctx, offset)];
	block = range->blocks[CTX_BLOCK_ID(ctx, offset)];
	id = (offset - block->start_offset) >> 2;
	block->reg[id] &= ~mask;
	block->reg[id] |= value;
	if (!(block->status & R600_BLOCK_STATUS_DIRTY)) {
		ctx->pm4_dirty_cdwords += block->pm4_ndwords;
		block->status |= R600_BLOCK_STATUS_ENABLED;
		block->status |= R600_BLOCK_STATUS_DIRTY;
	}
}

static inline void r600_context_block_emit_dirty(struct r600_context *ctx, struct r600_block *block)
{
	struct radeon_bo *bo;
	int id;

	for (int j = 0; j < block->nreg; j++) {
		if (block->pm4_bo_index[j]) {
			/* find relocation */
			id = block->pm4_bo_index[j];
			bo = radeon_bo_pb_get_bo(block->reloc[id].bo->pb);
			for (int k = 0; k < block->reloc[id].nreloc; k++) {
				r600_context_bo_reloc(ctx,
					&block->pm4[block->reloc[id].bo_pm4_index[k]],
					bo);
			}
		}
	}
	memcpy(&ctx->pm4[ctx->pm4_cdwords], block->pm4, block->pm4_ndwords * 4);
	ctx->pm4_cdwords += block->pm4_ndwords;
	block->status ^= R600_BLOCK_STATUS_DIRTY;
}

#endif
