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
#include "util/u_double_list.h"
#include "r600.h"

struct radeon {
	int				fd;
	int				refcount;
	unsigned			device;
	unsigned			family;
	enum chip_class			chip_class;
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
	struct list_head		fencedlist;
	boolean				shared;
	int64_t				last_busy;
	boolean				set_busy;
	struct r600_reloc		*reloc;
	unsigned			reloc_id;
};

struct r600_bo {
	struct pipe_reference		reference;
	struct pb_buffer		*pb;
	unsigned			size;
};


/* radeon_pciid.c */
unsigned radeon_family_from_device(unsigned device);

/* r600_drm.c */
struct radeon *radeon_decref(struct radeon *radeon);

/* radeon_bo.c */
struct radeon_bo *radeon_bo(struct radeon *radeon, unsigned handle,
			    unsigned size, unsigned alignment, void *ptr);
void radeon_bo_reference(struct radeon *radeon, struct radeon_bo **dst,
			 struct radeon_bo *src);
int radeon_bo_wait(struct radeon *radeon, struct radeon_bo *bo);
int radeon_bo_busy(struct radeon *radeon, struct radeon_bo *bo, uint32_t *domain);
void radeon_bo_pbmgr_flush_maps(struct pb_manager *_mgr);
int radeon_bo_fencelist(struct radeon *radeon, struct radeon_bo **bolist, uint32_t num_bo);


/* radeon_bo_pb.c */
struct radeon_bo *radeon_bo_pb_get_bo(struct pb_buffer *_buf);
struct pb_manager *radeon_bo_pbmgr_create(struct radeon *radeon);
struct pb_buffer *radeon_bo_pb_create_buffer_from_handle(struct pb_manager *_mgr,
							 uint32_t handle);

/* r600_hw_context.c */
void r600_context_bo_reloc(struct r600_context *ctx, u32 *pm4, struct r600_bo *rbo);
struct r600_bo *r600_context_reg_bo(struct r600_context *ctx, unsigned offset);
int r600_context_add_block(struct r600_context *ctx, const struct r600_reg *reg, unsigned nreg);

/* r600_bo.c */
unsigned r600_bo_get_handle(struct r600_bo *bo);
unsigned r600_bo_get_size(struct r600_bo *bo);
static INLINE struct radeon_bo *r600_bo_get_bo(struct r600_bo *bo)
{
	return radeon_bo_pb_get_bo(bo->pb);
}

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
	int id;

	for (int j = 0; j < block->nreg; j++) {
		if (block->pm4_bo_index[j]) {
			/* find relocation */
			id = block->pm4_bo_index[j];
			for (int k = 0; k < block->reloc[id].nreloc; k++) {
				r600_context_bo_reloc(ctx,
					&block->pm4[block->reloc[id].bo_pm4_index[k]],
					block->reloc[id].bo);
			}
		}
	}
	memcpy(&ctx->pm4[ctx->pm4_cdwords], block->pm4, block->pm4_ndwords * 4);
	ctx->pm4_cdwords += block->pm4_ndwords;
	block->status ^= R600_BLOCK_STATUS_DIRTY;
}

static inline int radeon_bo_map(struct radeon *radeon, struct radeon_bo *bo)
{
	bo->map_count++;
	return 0;
}

static inline void radeon_bo_unmap(struct radeon *radeon, struct radeon_bo *bo)
{
	bo->map_count--;
	assert(bo->map_count >= 0);
}

#endif
