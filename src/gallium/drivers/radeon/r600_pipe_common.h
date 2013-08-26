/*
 * Copyright 2013 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Marek Olšák <maraeo@gmail.com>
 *
 */

/**
 * This file contains common screen and context structures and functions
 * for r600g and radeonsi.
 */

#ifndef R600_PIPE_COMMON_H
#define R600_PIPE_COMMON_H

#include "../../winsys/radeon/drm/radeon_winsys.h"

#include "util/u_range.h"
#include "util/u_suballoc.h"
#include "util/u_transfer.h"

/* read caches */
#define R600_CONTEXT_INV_VERTEX_CACHE		(1 << 0)
#define R600_CONTEXT_INV_TEX_CACHE		(1 << 1)
#define R600_CONTEXT_INV_CONST_CACHE		(1 << 2)
#define R600_CONTEXT_INV_SHADER_CACHE		(1 << 3)
/* read-write caches */
#define R600_CONTEXT_STREAMOUT_FLUSH		(1 << 8)
#define R600_CONTEXT_FLUSH_AND_INV		(1 << 9)
#define R600_CONTEXT_FLUSH_AND_INV_CB_META	(1 << 10)
#define R600_CONTEXT_FLUSH_AND_INV_DB_META	(1 << 11)
#define R600_CONTEXT_FLUSH_AND_INV_DB		(1 << 12)
#define R600_CONTEXT_FLUSH_AND_INV_CB		(1 << 13)
/* engine synchronization */
#define R600_CONTEXT_PS_PARTIAL_FLUSH		(1 << 16)
#define R600_CONTEXT_WAIT_3D_IDLE		(1 << 17)
#define R600_CONTEXT_WAIT_CP_DMA_IDLE		(1 << 18)

struct r600_common_context;

struct r600_resource {
	struct u_resource		b;

	/* Winsys objects. */
	struct pb_buffer		*buf;
	struct radeon_winsys_cs_handle	*cs_buf;

	/* Resource state. */
	enum radeon_bo_domain		domains;

	/* The buffer range which is initialized (with a write transfer,
	 * streamout, DMA, or as a random access target). The rest of
	 * the buffer is considered invalid and can be mapped unsynchronized.
	 *
	 * This allows unsychronized mapping of a buffer range which hasn't
	 * been used yet. It's for applications which forget to use
	 * the unsynchronized map flag and expect the driver to figure it out.
         */
	struct util_range		valid_buffer_range;
};

struct r600_common_screen {
	struct pipe_screen		b;
	struct radeon_winsys		*ws;
	enum radeon_family		family;
	enum chip_class			chip_class;
	struct radeon_info		info;
};

/* This encapsulates a state or an operation which can emitted into the GPU
 * command stream. */
struct r600_atom {
	void (*emit)(struct r600_common_context *ctx, struct r600_atom *state);
	unsigned		num_dw;
	bool			dirty;
};

struct r600_so_target {
	struct pipe_stream_output_target b;

	/* The buffer where BUFFER_FILLED_SIZE is stored. */
	struct r600_resource	*buf_filled_size;
	unsigned		buf_filled_size_offset;

	unsigned		stride_in_dw;
};

struct r600_streamout {
	struct r600_atom		begin_atom;
	bool				begin_emitted;
	unsigned			num_dw_for_end;

	unsigned			enabled_mask;
	unsigned			num_targets;
	struct r600_so_target		*targets[PIPE_MAX_SO_BUFFERS];

	unsigned			append_bitmask;
	bool				suspended;

	/* External state which comes from the vertex shader,
	 * it must be set explicitly when binding a shader. */
	unsigned			*stride_in_dw;
};

struct r600_ring {
	struct radeon_winsys_cs		*cs;
	bool				flushing;
	void (*flush)(void *ctx, unsigned flags);
};

struct r600_rings {
	struct r600_ring		gfx;
	struct r600_ring		dma;
};

struct r600_common_context {
	struct pipe_context b; /* base class */

	struct radeon_winsys		*ws;
	enum radeon_family		family;
	enum chip_class			chip_class;
	struct r600_rings		rings;

	struct u_suballocator		*allocator_so_filled_size;

	/* Current unaccounted memory usage. */
	uint64_t			vram;
	uint64_t			gtt;

	/* States. */
	struct r600_streamout		streamout;

	/* Additional context states. */
	unsigned flags; /* flush flags */
};

/* r600_common_pipe.c */
void r600_common_screen_init(struct r600_common_screen *rscreen,
			     struct radeon_winsys *ws);
bool r600_common_context_init(struct r600_common_context *rctx,
			      struct r600_common_screen *rscreen);
void r600_common_context_cleanup(struct r600_common_context *rctx);
void r600_context_add_resource_size(struct pipe_context *ctx, struct pipe_resource *r);

/* r600_streamout.c */
void r600_streamout_buffers_dirty(struct r600_common_context *rctx);
void r600_set_streamout_targets(struct pipe_context *ctx,
				unsigned num_targets,
				struct pipe_stream_output_target **targets,
				unsigned append_bitmask);
void r600_emit_streamout_end(struct r600_common_context *rctx);
void r600_streamout_init(struct r600_common_context *rctx);

/* Inline helpers. */

static INLINE struct r600_resource *r600_resource(struct pipe_resource *r)
{
	return (struct r600_resource*)r;
}

static INLINE void
r600_resource_reference(struct r600_resource **ptr, struct r600_resource *res)
{
	pipe_resource_reference((struct pipe_resource **)ptr,
				(struct pipe_resource *)res);
}

#endif
