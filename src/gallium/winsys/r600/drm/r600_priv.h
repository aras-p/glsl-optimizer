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
#include <util/u_double_list.h>
#include <util/u_inlines.h>
#include "util/u_hash_table.h"
#include <os/os_thread.h>
#include "r600.h"

#define PKT_COUNT_C                     0xC000FFFF
#define PKT_COUNT_S(x)                  (((x) & 0x3FFF) << 16)

struct r600_bomgr;
struct r600_bo;

struct radeon {
	int				fd;
	int				refcount;
	unsigned			device;
	unsigned			family;
	enum chip_class			chip_class;
	struct r600_tiling_info		tiling_info;
	struct r600_bomgr		*bomgr;
	unsigned			fence;
	unsigned			*cfence;
	struct r600_bo			*fence_bo;
	unsigned			clock_crystal_freq;
	unsigned			num_backends;
	unsigned			num_tile_pipes;
	unsigned			backend_map;
	boolean				backend_map_valid;
	unsigned                        minor_version;

        /* List of buffer handles and its mutex. */
	struct util_hash_table          *bo_handles;
	pipe_mutex bo_handles_mutex;
};

/* these flags are used in register flags and added into block flags */
#define REG_FLAG_NEED_BO 1
#define REG_FLAG_DIRTY_ALWAYS 2
#define REG_FLAG_RV6XX_SBU 4
#define REG_FLAG_NOT_R600 8
#define REG_FLAG_ENABLE_ALWAYS 16
#define BLOCK_FLAG_RESOURCE 32
#define REG_FLAG_FLUSH_CHANGE 64

struct r600_reg {
	unsigned			offset;
	unsigned			flags;
	unsigned			flush_flags;
	unsigned			flush_mask;
};

#define BO_BOUND_TEXTURE 1
struct radeon_bo {
	struct pipe_reference		reference;
	unsigned			handle;
	unsigned			size;
	unsigned			alignment;
	int				map_count;
	void				*data;
	struct list_head		fencedlist;
	unsigned			fence;
	struct r600_context		*ctx;
	boolean				shared;
	struct r600_reloc		*reloc;
	unsigned			reloc_id;
	unsigned			last_flush;
	unsigned                        name;
	unsigned                        binding;
};

struct r600_bo {
	struct pipe_reference		reference; /* this must be the first member for the r600_bo_reference inline to work */
	/* DO NOT MOVE THIS ^ */
	unsigned			size;
	unsigned			tiling_flags;
	unsigned			kernel_pitch;
	unsigned			domains;
	struct radeon_bo		*bo;
	unsigned			fence;
	/* manager data */
	struct list_head		list;
	unsigned			manager_id;
	unsigned			alignment;
	unsigned			offset;
	int64_t				start;
	int64_t				end;
};

struct r600_bomgr {
	struct radeon			*radeon;
	unsigned			usecs;
	pipe_mutex			mutex;
	struct list_head		delayed;
	unsigned			num_delayed;
};

/*
 * r600_drm.c
 */
struct radeon *r600_new(int fd, unsigned device);
void r600_delete(struct radeon *r600);

/*
 * radeon_pciid.c
 */
unsigned radeon_family_from_device(unsigned device);

/*
 * radeon_bo.c
 */
struct radeon_bo *radeon_bo(struct radeon *radeon, unsigned handle,
			    unsigned size, unsigned alignment, unsigned initial_domain);
void radeon_bo_reference(struct radeon *radeon, struct radeon_bo **dst,
			 struct radeon_bo *src);
int radeon_bo_wait(struct radeon *radeon, struct radeon_bo *bo);
int radeon_bo_busy(struct radeon *radeon, struct radeon_bo *bo, uint32_t *domain);
int radeon_bo_fencelist(struct radeon *radeon, struct radeon_bo **bolist, uint32_t num_bo);
int radeon_bo_get_tiling_flags(struct radeon *radeon,
			       struct radeon_bo *bo,
			       uint32_t *tiling_flags,
			       uint32_t *pitch);
int radeon_bo_get_name(struct radeon *radeon,
		       struct radeon_bo *bo,
		       uint32_t *name);
int radeon_bo_fixed_map(struct radeon *radeon, struct radeon_bo *bo);

/*
 * r600_hw_context.c
 */
int r600_context_init_fence(struct r600_context *ctx);
void r600_context_get_reloc(struct r600_context *ctx, struct r600_bo *rbo);
void r600_context_bo_flush(struct r600_context *ctx, unsigned flush_flags,
				unsigned flush_mask, struct r600_bo *rbo);
struct r600_bo *r600_context_reg_bo(struct r600_context *ctx, unsigned offset);
int r600_context_add_block(struct r600_context *ctx, const struct r600_reg *reg, unsigned nreg,
			   unsigned opcode, unsigned offset_base);
void r600_context_pipe_state_set_resource(struct r600_context *ctx, struct r600_pipe_resource_state *state, struct r600_block *block);
void r600_context_block_emit_dirty(struct r600_context *ctx, struct r600_block *block);
void r600_context_block_resource_emit_dirty(struct r600_context *ctx, struct r600_block *block);
void r600_context_dirty_block(struct r600_context *ctx, struct r600_block *block,
			      int dirty, int index);
int r600_setup_block_table(struct r600_context *ctx);
void r600_context_reg(struct r600_context *ctx,
		      unsigned offset, unsigned value,
		      unsigned mask);
void r600_init_cs(struct r600_context *ctx);
int r600_resource_init(struct r600_context *ctx, struct r600_range *range, unsigned offset, unsigned nblocks, unsigned stride, struct r600_reg *reg, int nreg, unsigned offset_base);

static INLINE void r600_context_bo_reloc(struct r600_context *ctx, u32 *pm4, struct r600_bo *rbo)
{
	struct radeon_bo *bo = rbo->bo;

	assert(bo != NULL);

	if (!bo->reloc)
		r600_context_get_reloc(ctx, rbo);

	/* set PKT3 to point to proper reloc */
	*pm4 = bo->reloc_id;
}

/*
 * r600_bo.c
 */
void r600_bo_destroy(struct radeon *radeon, struct r600_bo *bo);

/*
 * r600_bomgr.c
 */
struct r600_bomgr *r600_bomgr_create(struct radeon *radeon, unsigned usecs);
void r600_bomgr_destroy(struct r600_bomgr *mgr);
boolean r600_bomgr_bo_destroy(struct r600_bomgr *mgr, struct r600_bo *bo);
void r600_bomgr_bo_init(struct r600_bomgr *mgr, struct r600_bo *bo);
struct r600_bo *r600_bomgr_bo_create(struct r600_bomgr *mgr,
					unsigned size,
					unsigned alignment,
					unsigned cfence);


/*
 * helpers
 */


/*
 * radeon_bo.c
 */
static inline int radeon_bo_map(struct radeon *radeon, struct radeon_bo *bo)
{
	if (bo->map_count == 0 && !bo->data)
		return radeon_bo_fixed_map(radeon, bo);
	bo->map_count++;
	return 0;
}

static inline void radeon_bo_unmap(struct radeon *radeon, struct radeon_bo *bo)
{
	bo->map_count--;
	assert(bo->map_count >= 0);
}

/*
 * fence
 */
static inline boolean fence_is_after(unsigned fence, unsigned ofence)
{
	/* handle wrap around */
	if (fence < 0x80000000 && ofence > 0x80000000)
		return TRUE;
	if (fence > ofence)
		return TRUE;
	return FALSE;
}

#endif
