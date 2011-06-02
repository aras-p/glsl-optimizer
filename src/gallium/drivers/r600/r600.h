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
#ifndef R600_H
#define R600_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <util/u_double_list.h>
#include <pipe/p_compiler.h>

#define RADEON_CTX_MAX_PM4	(64 * 1024 / 4)

#define R600_ERR(fmt, args...) \
	fprintf(stderr, "EE %s:%d %s - "fmt, __FILE__, __LINE__, __func__, ##args)

typedef uint64_t		u64;
typedef uint32_t		u32;
typedef uint16_t		u16;
typedef uint8_t			u8;

struct radeon;
struct winsys_handle;

enum radeon_family {
	CHIP_UNKNOWN,
	CHIP_R100,
	CHIP_RV100,
	CHIP_RS100,
	CHIP_RV200,
	CHIP_RS200,
	CHIP_R200,
	CHIP_RV250,
	CHIP_RS300,
	CHIP_RV280,
	CHIP_R300,
	CHIP_R350,
	CHIP_RV350,
	CHIP_RV380,
	CHIP_R420,
	CHIP_R423,
	CHIP_RV410,
	CHIP_RS400,
	CHIP_RS480,
	CHIP_RS600,
	CHIP_RS690,
	CHIP_RS740,
	CHIP_RV515,
	CHIP_R520,
	CHIP_RV530,
	CHIP_RV560,
	CHIP_RV570,
	CHIP_R580,
	CHIP_R600,
	CHIP_RV610,
	CHIP_RV630,
	CHIP_RV670,
	CHIP_RV620,
	CHIP_RV635,
	CHIP_RS780,
	CHIP_RS880,
	CHIP_RV770,
	CHIP_RV730,
	CHIP_RV710,
	CHIP_RV740,
	CHIP_CEDAR,
	CHIP_REDWOOD,
	CHIP_JUNIPER,
	CHIP_CYPRESS,
	CHIP_HEMLOCK,
	CHIP_PALM,
	CHIP_SUMO,
	CHIP_SUMO2,
	CHIP_BARTS,
	CHIP_TURKS,
	CHIP_CAICOS,
	CHIP_CAYMAN,
	CHIP_LAST,
};

enum chip_class {
	R600,
	R700,
	EVERGREEN,
	CAYMAN,
};

struct r600_tiling_info {
	unsigned num_channels;
	unsigned num_banks;
	unsigned group_bytes;
};

enum radeon_family r600_get_family(struct radeon *rw);
enum chip_class r600_get_family_class(struct radeon *radeon);
struct r600_tiling_info *r600_get_tiling_info(struct radeon *radeon);
unsigned r600_get_clock_crystal_freq(struct radeon *radeon);
unsigned r600_get_minor_version(struct radeon *radeon);
unsigned r600_get_num_backends(struct radeon *radeon);

/* r600_bo.c */
struct r600_bo;
struct r600_bo *r600_bo(struct radeon *radeon,
			unsigned size, unsigned alignment,
			unsigned binding, unsigned usage);
struct r600_bo *r600_bo_handle(struct radeon *radeon,
				unsigned handle, unsigned *array_mode);
void *r600_bo_map(struct radeon *radeon, struct r600_bo *bo, unsigned usage, void *ctx);
void r600_bo_unmap(struct radeon *radeon, struct r600_bo *bo);
void r600_bo_reference(struct radeon *radeon, struct r600_bo **dst,
			    struct r600_bo *src);
boolean r600_bo_get_winsys_handle(struct radeon *radeon, struct r600_bo *pb_bo,
				unsigned stride, struct winsys_handle *whandle);
static INLINE unsigned r600_bo_offset(struct r600_bo *bo)
{
	return 0;
}


/* R600/R700 STATES */
#define R600_GROUP_MAX			16
#define R600_BLOCK_MAX_BO		32
#define R600_BLOCK_MAX_REG		128

/* each range covers 9 bits of dword space = 512 dwords = 2k bytes */
/* there is a block entry for each register so 512 blocks */
/* we have no registers to read/write below 0x8000 (0x2000 in dw space) */
/* we use some fake offsets at 0x40000 to do evergreen sampler borders so take 0x42000 as a max bound*/
#define RANGE_OFFSET_START 0x8000
#define HASH_SHIFT 9
#define NUM_RANGES (0x42000 - RANGE_OFFSET_START) / (4 << HASH_SHIFT) /* 128 << 9 = 64k */

#define CTX_RANGE_ID(offset) ((((offset - RANGE_OFFSET_START) >> 2) >> HASH_SHIFT) & 255)
#define CTX_BLOCK_ID(offset) (((offset - RANGE_OFFSET_START) >> 2) & ((1 << HASH_SHIFT) - 1))

struct r600_pipe_reg {
	u32				value;
	u32				mask;
	struct r600_block 		*block;
	struct r600_bo			*bo;
	u32				id;
};

struct r600_pipe_state {
	unsigned			id;
	unsigned			nregs;
	struct r600_pipe_reg		regs[R600_BLOCK_MAX_REG];
};

#define R600_BLOCK_STATUS_ENABLED	(1 << 0)
#define R600_BLOCK_STATUS_DIRTY		(1 << 1)

struct r600_block_reloc {
	struct r600_bo		*bo;
	unsigned		flush_flags;
	unsigned		flush_mask;
	unsigned		bo_pm4_index;
};

struct r600_block {
	struct list_head	list;
	unsigned		status;
	unsigned                flags;
	unsigned		start_offset;
	unsigned		pm4_ndwords;
	unsigned		pm4_flush_ndwords;
	unsigned		nbo;
	u16 		        nreg;
	u16                     nreg_dirty;
	u32			*reg;
	u32			pm4[R600_BLOCK_MAX_REG];
	unsigned		pm4_bo_index[R600_BLOCK_MAX_REG];
	struct r600_block_reloc	reloc[R600_BLOCK_MAX_BO];
};

struct r600_range {
	struct r600_block	**blocks;
};

/*
 * relocation
 */
#pragma pack(1)
struct r600_reloc {
	uint32_t	handle;
	uint32_t	read_domain;
	uint32_t	write_domain;
	uint32_t	flags;
};
#pragma pack()

/*
 * query
 */
struct r600_query {
	u64					result;
	/* The kind of query. Currently only OQ is supported. */
	unsigned				type;
	/* How many results have been written, in dwords. It's incremented
	 * after end_query and flush. */
	unsigned				num_results;
	/* if we've flushed the query */
	unsigned				state;
	/* The buffer where query results are stored. */
	struct r600_bo			*buffer;
	unsigned				buffer_size;
	/* linked list of queries */
	struct list_head			list;
};

#define R600_QUERY_STATE_STARTED	(1 << 0)
#define R600_QUERY_STATE_ENDED		(1 << 1)
#define R600_QUERY_STATE_SUSPENDED	(1 << 2)

#define R600_CONTEXT_DRAW_PENDING	(1 << 0)
#define R600_CONTEXT_DST_CACHES_DIRTY	(1 << 1)
#define R600_CONTEXT_CHECK_EVENT_FLUSH	(1 << 2)

struct r600_context {
	struct radeon		*radeon;
	struct r600_range	*range;
	unsigned		nblocks;
	struct r600_block	**blocks;
	struct list_head	dirty;
	unsigned		pm4_ndwords;
	unsigned		pm4_cdwords;
	unsigned		pm4_dirty_cdwords;
	unsigned		ctx_pm4_ndwords;
	unsigned		nreloc;
	unsigned		creloc;
	struct r600_reloc	*reloc;
	struct radeon_bo	**bo;
	u32			*pm4;
	struct list_head	query_list;
	unsigned		num_query_running;
	struct list_head	fenced_bo;
	unsigned                max_db; /* for OQ */
	unsigned                num_dest_buffers;
	unsigned		flags;
	boolean                 predicate_drawing;
};

struct r600_draw {
	u32			vgt_num_indices;
	u32			vgt_num_instances;
	u32			vgt_index_type;
	u32			vgt_draw_initiator;
	u32			indices_bo_offset;
	struct r600_bo		*indices;
};

int r600_context_init(struct r600_context *ctx, struct radeon *radeon);
void r600_context_fini(struct r600_context *ctx);
void r600_context_pipe_state_set(struct r600_context *ctx, struct r600_pipe_state *state);
void r600_context_pipe_state_set_ps_resource(struct r600_context *ctx, struct r600_pipe_state *state, unsigned rid);
void r600_context_pipe_state_set_vs_resource(struct r600_context *ctx, struct r600_pipe_state *state, unsigned rid);
void r600_context_pipe_state_set_fs_resource(struct r600_context *ctx, struct r600_pipe_state *state, unsigned rid);
void r600_context_pipe_state_set_ps_sampler(struct r600_context *ctx, struct r600_pipe_state *state, unsigned id);
void r600_context_pipe_state_set_vs_sampler(struct r600_context *ctx, struct r600_pipe_state *state, unsigned id);
void r600_context_flush(struct r600_context *ctx);
void r600_context_dump_bof(struct r600_context *ctx, const char *file);
void r600_context_draw(struct r600_context *ctx, const struct r600_draw *draw);

struct r600_query *r600_context_query_create(struct r600_context *ctx, unsigned query_type);
void r600_context_query_destroy(struct r600_context *ctx, struct r600_query *query);
boolean r600_context_query_result(struct r600_context *ctx,
				struct r600_query *query,
				boolean wait, void *vresult);
void r600_query_begin(struct r600_context *ctx, struct r600_query *query);
void r600_query_end(struct r600_context *ctx, struct r600_query *query);
void r600_context_queries_suspend(struct r600_context *ctx);
void r600_context_queries_resume(struct r600_context *ctx);
void r600_query_predication(struct r600_context *ctx, struct r600_query *query, int operation,
			    int flag_wait);
void r600_context_emit_fence(struct r600_context *ctx, struct r600_bo *fence,
                             unsigned offset, unsigned value);
void r600_context_flush_all(struct r600_context *ctx, unsigned flush_flags);
void r600_context_flush_dest_caches(struct r600_context *ctx);

int evergreen_context_init(struct r600_context *ctx, struct radeon *radeon);
void evergreen_context_draw(struct r600_context *ctx, const struct r600_draw *draw);
void evergreen_context_flush_dest_caches(struct r600_context *ctx);
void evergreen_context_pipe_state_set_ps_resource(struct r600_context *ctx, struct r600_pipe_state *state, unsigned rid);
void evergreen_context_pipe_state_set_vs_resource(struct r600_context *ctx, struct r600_pipe_state *state, unsigned rid);
void evergreen_context_pipe_state_set_fs_resource(struct r600_context *ctx, struct r600_pipe_state *state, unsigned rid);
void evergreen_context_pipe_state_set_ps_sampler(struct r600_context *ctx, struct r600_pipe_state *state, unsigned id);
void evergreen_context_pipe_state_set_vs_sampler(struct r600_context *ctx, struct r600_pipe_state *state, unsigned id);

struct radeon *radeon_decref(struct radeon *radeon);

void _r600_pipe_state_add_reg(struct r600_context *ctx,
			      struct r600_pipe_state *state,
			      u32 offset, u32 value, u32 mask,
			      u32 range_id, u32 block_id,
			      struct r600_bo *bo);

#define r600_pipe_state_add_reg(state, offset, value, mask, bo) _r600_pipe_state_add_reg(&rctx->ctx, state, offset, value, mask, CTX_RANGE_ID(offset), CTX_BLOCK_ID(offset), bo)

#endif
