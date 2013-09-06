/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef FREEDRENO_DRAW_H_
#define FREEDRENO_DRAW_H_

#include "pipe/p_state.h"
#include "pipe/p_context.h"

#include "freedreno_context.h"
#include "freedreno_screen.h"
#include "freedreno_util.h"

struct fd_ringbuffer;

void fd_draw_emit(struct fd_context *ctx, const struct pipe_draw_info *info);

void fd_draw_init(struct pipe_context *pctx);

static inline void
fd_draw(struct fd_context *ctx, enum pc_di_primtype primtype,
		enum pc_di_src_sel src_sel, uint32_t count,
		enum pc_di_index_size idx_type,
		uint32_t idx_size, uint32_t idx_offset,
		struct fd_bo *idx_bo)
{
	struct fd_ringbuffer *ring = ctx->ring;

	/* for debug after a lock up, write a unique counter value
	 * to scratch7 for each draw, to make it easier to match up
	 * register dumps to cmdstream.  The combination of IB
	 * (scratch6) and DRAW is enough to "triangulate" the
	 * particular draw that caused lockup.
	 */
	emit_marker(ring, 7);

	OUT_PKT3(ring, CP_DRAW_INDX, idx_bo ? 5 : 3);
	OUT_RING(ring, 0x00000000);        /* viz query info. */
	OUT_RING(ring, DRAW(primtype, src_sel,
			idx_type, IGNORE_VISIBILITY));
	OUT_RING(ring, count);             /* NumIndices */
	if (idx_bo) {
		OUT_RELOC(ring, idx_bo, idx_offset, 0, 0);
		OUT_RING (ring, idx_size);
	}

	emit_marker(ring, 7);
}

#endif /* FREEDRENO_DRAW_H_ */
