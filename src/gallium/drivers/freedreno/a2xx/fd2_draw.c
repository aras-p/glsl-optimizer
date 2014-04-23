/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012-2013 Rob Clark <robclark@freedesktop.org>
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
#include "util/u_prim.h"
#include "util/u_pack_color.h"

#include "freedreno_state.h"
#include "freedreno_resource.h"

#include "fd2_draw.h"
#include "fd2_context.h"
#include "fd2_emit.h"
#include "fd2_program.h"
#include "fd2_util.h"
#include "fd2_zsa.h"


static void
emit_cacheflush(struct fd_ringbuffer *ring)
{
	unsigned i;

	for (i = 0; i < 12; i++) {
		OUT_PKT3(ring, CP_EVENT_WRITE, 1);
		OUT_RING(ring, CACHE_FLUSH);
	}
}

static void
emit_vertexbufs(struct fd_context *ctx)
{
	struct fd_vertex_stateobj *vtx = ctx->vtx;
	struct fd_vertexbuf_stateobj *vertexbuf = &ctx->vertexbuf;
	struct fd2_vertex_buf bufs[PIPE_MAX_ATTRIBS];
	unsigned i;

	if (!vtx->num_elements)
		return;

	for (i = 0; i < vtx->num_elements; i++) {
		struct pipe_vertex_element *elem = &vtx->pipe[i];
		struct pipe_vertex_buffer *vb =
				&vertexbuf->vb[elem->vertex_buffer_index];
		bufs[i].offset = vb->buffer_offset;
		bufs[i].size = fd_bo_size(fd_resource(vb->buffer)->bo);
		bufs[i].prsc = vb->buffer;
	}

	// NOTE I believe the 0x78 (or 0x9c in solid_vp) relates to the
	// CONST(20,0) (or CONST(26,0) in soliv_vp)

	fd2_emit_vertex_bufs(ctx->ring, 0x78, bufs, vtx->num_elements);
}

static void
fd2_draw(struct fd_context *ctx, const struct pipe_draw_info *info)
{
	struct fd_ringbuffer *ring = ctx->ring;

	if (ctx->dirty & FD_DIRTY_VTXBUF)
		emit_vertexbufs(ctx);

	fd2_emit_state(ctx, ctx->dirty);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
	OUT_RING(ring, info->start);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
	OUT_RING(ring, 0x0000003b);

	OUT_PKT0(ring, REG_A2XX_TC_CNTL_STATUS, 1);
	OUT_RING(ring, A2XX_TC_CNTL_STATUS_L2_INVALIDATE);

	OUT_WFI (ring);

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
	OUT_RING(ring, info->max_index);        /* VGT_MAX_VTX_INDX */
	OUT_RING(ring, info->min_index);        /* VGT_MIN_VTX_INDX */

	fd_draw_emit(ctx, ring, IGNORE_VISIBILITY, info);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_UNKNOWN_2010));
	OUT_RING(ring, 0x00000000);

	emit_cacheflush(ring);
}


static uint32_t
pack_rgba(enum pipe_format format, const float *rgba)
{
	union util_color uc;
	util_pack_color(rgba, format, &uc);
	return uc.ui[0];
}

static void
fd2_clear(struct fd_context *ctx, unsigned buffers,
		const union pipe_color_union *color, double depth, unsigned stencil)
{
	struct fd2_context *fd2_ctx = fd2_context(ctx);
	struct fd_ringbuffer *ring = ctx->ring;
	struct pipe_framebuffer_state *fb = &ctx->framebuffer;
	uint32_t reg, colr = 0;

	if ((buffers & PIPE_CLEAR_COLOR) && fb->nr_cbufs)
		colr  = pack_rgba(fb->cbufs[0]->format, color->f);

	/* emit generic state now: */
	fd2_emit_state(ctx, ctx->dirty &
			(FD_DIRTY_BLEND | FD_DIRTY_VIEWPORT |
					FD_DIRTY_FRAMEBUFFER | FD_DIRTY_SCISSOR));

	fd2_emit_vertex_bufs(ring, 0x9c, (struct fd2_vertex_buf[]) {
			{ .prsc = fd2_ctx->solid_vertexbuf, .size = 48 },
		}, 1);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
	OUT_RING(ring, 0);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
	OUT_RING(ring, 0x0000028f);

	fd2_program_emit(ring, &ctx->solid_prog);

	OUT_PKT0(ring, REG_A2XX_TC_CNTL_STATUS, 1);
	OUT_RING(ring, A2XX_TC_CNTL_STATUS_L2_INVALIDATE);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_CLEAR_COLOR));
	OUT_RING(ring, colr);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_A220_RB_LRZ_VSC_CONTROL));
	OUT_RING(ring, 0x00000084);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_CONTROL));
	reg = 0;
	if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
		reg |= A2XX_RB_COPY_CONTROL_DEPTH_CLEAR_ENABLE;
		switch (fd_pipe2depth(fb->zsbuf->format)) {
		case DEPTHX_24_8:
			if (buffers & PIPE_CLEAR_DEPTH)
				reg |= A2XX_RB_COPY_CONTROL_CLEAR_MASK(0xe);
			if (buffers & PIPE_CLEAR_STENCIL)
				reg |= A2XX_RB_COPY_CONTROL_CLEAR_MASK(0x1);
			break;
		case DEPTHX_16:
			if (buffers & PIPE_CLEAR_DEPTH)
				reg |= A2XX_RB_COPY_CONTROL_CLEAR_MASK(0xf);
			break;
		default:
			assert(1);
			break;
		}
	}
	OUT_RING(ring, reg);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTH_CLEAR));
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
	OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTHCONTROL));
	reg = 0;
	if (buffers & PIPE_CLEAR_DEPTH) {
		reg |= A2XX_RB_DEPTHCONTROL_ZFUNC(FUNC_ALWAYS) |
				A2XX_RB_DEPTHCONTROL_Z_ENABLE |
				A2XX_RB_DEPTHCONTROL_Z_WRITE_ENABLE |
				A2XX_RB_DEPTHCONTROL_EARLY_Z_ENABLE;
	}
	if (buffers & PIPE_CLEAR_STENCIL) {
		reg |= A2XX_RB_DEPTHCONTROL_STENCILFUNC(FUNC_ALWAYS) |
				A2XX_RB_DEPTHCONTROL_STENCIL_ENABLE |
				A2XX_RB_DEPTHCONTROL_STENCILZPASS(STENCIL_REPLACE);
	}
	OUT_RING(ring, reg);

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_STENCILREFMASK_BF));
	OUT_RING(ring, 0xff000000 | A2XX_RB_STENCILREFMASK_BF_STENCILWRITEMASK(0xff));
	OUT_RING(ring, 0xff000000 | A2XX_RB_STENCILREFMASK_STENCILWRITEMASK(0xff));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_COLORCONTROL));
	OUT_RING(ring, A2XX_RB_COLORCONTROL_ALPHA_FUNC(FUNC_ALWAYS) |
			A2XX_RB_COLORCONTROL_BLEND_DISABLE |
			A2XX_RB_COLORCONTROL_ROP_CODE(12) |
			A2XX_RB_COLORCONTROL_DITHER_MODE(DITHER_DISABLE) |
			A2XX_RB_COLORCONTROL_DITHER_TYPE(DITHER_PIXEL));

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_CLIP_CNTL));
	OUT_RING(ring, 0x00000000);        /* PA_CL_CLIP_CNTL */
	OUT_RING(ring, A2XX_PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST |  /* PA_SU_SC_MODE_CNTL */
			A2XX_PA_SU_SC_MODE_CNTL_FRONT_PTYPE(PC_DRAW_TRIANGLES) |
			A2XX_PA_SU_SC_MODE_CNTL_BACK_PTYPE(PC_DRAW_TRIANGLES));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_MASK));
	OUT_RING(ring, 0x0000ffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
	OUT_RING(ring, xy2d(0,0));	        /* PA_SC_WINDOW_SCISSOR_TL */
	OUT_RING(ring, xy2d(fb->width,      /* PA_SC_WINDOW_SCISSOR_BR */
			fb->height));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_MASK));
	if (buffers & PIPE_CLEAR_COLOR) {
		OUT_RING(ring, A2XX_RB_COLOR_MASK_WRITE_RED |
				A2XX_RB_COLOR_MASK_WRITE_GREEN |
				A2XX_RB_COLOR_MASK_WRITE_BLUE |
				A2XX_RB_COLOR_MASK_WRITE_ALPHA);
	} else {
		OUT_RING(ring, 0x0);
	}

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
	OUT_RING(ring, 3);                 /* VGT_MAX_VTX_INDX */
	OUT_RING(ring, 0);                 /* VGT_MIN_VTX_INDX */

	fd_draw(ctx, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
			DI_SRC_SEL_AUTO_INDEX, 3, INDEX_SIZE_IGN, 0, 0, NULL);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_A220_RB_LRZ_VSC_CONTROL));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_CONTROL));
	OUT_RING(ring, 0x00000000);
}

void
fd2_draw_init(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->draw = fd2_draw;
	ctx->clear = fd2_clear;
}
