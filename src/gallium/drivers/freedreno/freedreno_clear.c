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

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_pack_color.h"

#include "freedreno_clear.h"
#include "freedreno_context.h"
#include "freedreno_resource.h"
#include "freedreno_state.h"
#include "freedreno_program.h"
#include "freedreno_zsa.h"
#include "freedreno_util.h"

static uint32_t
pack_rgba(enum pipe_format format, const float *rgba)
{
	union util_color uc;
	util_pack_color(rgba, format, &uc);
	return uc.ui;
}

static void
fd_clear(struct pipe_context *pctx, unsigned buffers,
		const union pipe_color_union *color, double depth, unsigned stencil)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_ringbuffer *ring = ctx->ring;
	struct pipe_framebuffer_state *fb = &ctx->framebuffer;
	uint32_t reg, colr = 0;

	ctx->cleared |= buffers;
	ctx->resolve |= buffers;
	ctx->needs_flush = true;

	if (buffers & PIPE_CLEAR_COLOR)
		fd_resource(fb->cbufs[0]->texture)->dirty = true;

	if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL))
		fd_resource(fb->zsbuf->texture)->dirty = true;

	DBG("depth=%f, stencil=%u", depth, stencil);

	if ((buffers & PIPE_CLEAR_COLOR) && fb->nr_cbufs)
		colr  = pack_rgba(fb->cbufs[0]->format, color->f);

	/* emit generic state now: */
	fd_state_emit(pctx, ctx->dirty &
			(FD_DIRTY_BLEND | FD_DIRTY_VIEWPORT |
					FD_DIRTY_FRAMEBUFFER | FD_DIRTY_SCISSOR));

	fd_emit_vertex_bufs(ring, 0x9c, (struct fd_vertex_buf[]) {
			{ .prsc = ctx->solid_vertexbuf, .size = 48 },
		}, 1);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_INDX_OFFSET));
	OUT_RING(ring, 0);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_VERTEX_REUSE_BLOCK_CNTL));
	OUT_RING(ring, 0x0000028f);

	fd_program_emit(ring, &ctx->solid_prog);

	OUT_PKT0(ring, REG_TC_CNTL_STATUS, 1);
	OUT_RING(ring, TC_CNTL_STATUS_L2_INVALIDATE);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_CLEAR_COLOR));
	OUT_RING(ring, colr);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A220_RB_LRZ_VSC_CONTROL));
	OUT_RING(ring, 0x00000084);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COPY_CONTROL));
	reg = 0;
	if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
		reg |= RB_COPY_CONTROL_DEPTH_CLEAR_ENABLE;
		switch (fd_pipe2depth(fb->zsbuf->format)) {
		case DEPTHX_24_8:
			if (buffers & PIPE_CLEAR_DEPTH)
				reg |= RB_COPY_CONTROL_CLEAR_MASK(0xe);
			if (buffers & PIPE_CLEAR_STENCIL)
				reg |= RB_COPY_CONTROL_CLEAR_MASK(0x1);
			break;
		case DEPTHX_16:
			if (buffers & PIPE_CLEAR_DEPTH)
				reg |= RB_COPY_CONTROL_CLEAR_MASK(0xf);
			break;
		}
	}
	OUT_RING(ring, reg);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_DEPTH_CLEAR));
	reg = 0;
	if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
		switch (fd_pipe2depth(fb->zsbuf->format)) {
		case DEPTHX_24_8:
			reg = (((uint32_t)(0xffffff * depth)) << 8) |
				(stencil & 0xff);
			break;
		case DEPTHX_16:
			reg = (uint32_t)(0xffffffff * depth);
			break;
		}
	}
	OUT_RING(ring, reg);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_DEPTHCONTROL));
	reg = 0;
	if (buffers & PIPE_CLEAR_DEPTH) {
		reg |= RB_DEPTHCONTROL_ZFUNC(GL_ALWAYS) |
				RB_DEPTHCONTROL_Z_ENABLE |
				RB_DEPTHCONTROL_Z_WRITE_ENABLE |
				RB_DEPTHCONTROL_EARLY_Z_ENABLE;
	}
	if (buffers & PIPE_CLEAR_STENCIL) {
		reg |= RB_DEPTHCONTROL_STENCILFUNC(GL_ALWAYS) |
				RB_DEPTHCONTROL_STENCIL_ENABLE |
				RB_DEPTHCONTROL_STENCILZPASS(STENCIL_REPLACE);
	}
	OUT_RING(ring, reg);

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_PA_CL_CLIP_CNTL));
	OUT_RING(ring, 0x00000000);        /* PA_CL_CLIP_CNTL */
	OUT_RING(ring, PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST |  /* PA_SU_SC_MODE_CNTL */
			PA_SU_SC_MODE_CNTL_POLYMODE_FRONT_PTYPE(DRAW_TRIANGLES) |
			PA_SU_SC_MODE_CNTL_POLYMODE_BACK_PTYPE(DRAW_TRIANGLES));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SC_AA_MASK));
	OUT_RING(ring, 0x0000ffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_PA_SC_WINDOW_SCISSOR_TL));
	OUT_RING(ring, xy2d(0,0));	        /* PA_SC_WINDOW_SCISSOR_TL */
	OUT_RING(ring, xy2d(fb->width,      /* PA_SC_WINDOW_SCISSOR_BR */
			fb->height));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COLOR_MASK));
	if (buffers & PIPE_CLEAR_COLOR) {
		OUT_RING(ring, RB_COLOR_MASK_WRITE_RED |
				RB_COLOR_MASK_WRITE_GREEN |
				RB_COLOR_MASK_WRITE_BLUE |
				RB_COLOR_MASK_WRITE_ALPHA);
	} else {
		OUT_RING(ring, 0x0);
	}

	OUT_PKT3(ring, CP_DRAW_INDX, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, DRAW(DI_PT_RECTLIST, DI_SRC_SEL_AUTO_INDEX,
			INDEX_SIZE_IGN, IGNORE_VISIBILITY));
	OUT_RING(ring, 3);					/* NumIndices */

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A220_RB_LRZ_VSC_CONTROL));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COPY_CONTROL));
	OUT_RING(ring, 0x00000000);

	ctx->dirty |= FD_DIRTY_ZSA |
			FD_DIRTY_RASTERIZER |
			FD_DIRTY_SAMPLE_MASK |
			FD_DIRTY_PROG |
			FD_DIRTY_CONSTBUF |
			FD_DIRTY_BLEND;
}

static void
fd_clear_render_target(struct pipe_context *pctx, struct pipe_surface *ps,
		const union pipe_color_union *color,
		unsigned x, unsigned y, unsigned w, unsigned h)
{
	DBG("TODO: x=%u, y=%u, w=%u, h=%u", x, y, w, h);
}

static void
fd_clear_depth_stencil(struct pipe_context *pctx, struct pipe_surface *ps,
		unsigned buffers, double depth, unsigned stencil,
		unsigned x, unsigned y, unsigned w, unsigned h)
{
	DBG("TODO: buffers=%u, depth=%f, stencil=%u, x=%u, y=%u, w=%u, h=%u",
			buffers, depth, stencil, x, y, w, h);
}

void
fd_clear_init(struct pipe_context *pctx)
{
	pctx->clear = fd_clear;
	pctx->clear_render_target = fd_clear_render_target;
	pctx->clear_depth_stencil = fd_clear_depth_stencil;
}
