/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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

#include "freedreno_state.h"
#include "freedreno_resource.h"

#include "fd3_draw.h"
#include "fd3_context.h"
#include "fd3_emit.h"
#include "fd3_program.h"
#include "fd3_util.h"
#include "fd3_zsa.h"


static void
emit_vertexbufs(struct fd_context *ctx)
{
	struct fd_vertex_stateobj *vtx = ctx->vtx;
	struct fd_vertexbuf_stateobj *vertexbuf = &ctx->vertexbuf;
	struct fd3_vertex_buf bufs[PIPE_MAX_ATTRIBS];
	unsigned i;

	if (!vtx->num_elements)
		return;

	for (i = 0; i < vtx->num_elements; i++) {
		struct pipe_vertex_element *elem = &vtx->pipe[i];
		struct pipe_vertex_buffer *vb =
				&vertexbuf->vb[elem->vertex_buffer_index];
		bufs[i].offset = vb->buffer_offset + elem->src_offset;
		bufs[i].stride = vb->stride;
		bufs[i].prsc   = vb->buffer;
		bufs[i].format = elem->src_format;
	}

	fd3_emit_vertex_bufs(ctx->ring, &ctx->prog, bufs, vtx->num_elements);
}

static void
fd3_draw(struct fd_context *ctx, const struct pipe_draw_info *info)
{
	struct fd_ringbuffer *ring = ctx->ring;
	unsigned dirty = ctx->dirty;

	fd3_emit_state(ctx, dirty);

	if (dirty & FD_DIRTY_VTXBUF)
		emit_vertexbufs(ctx);

	OUT_PKT0(ring, REG_A3XX_PC_VERTEX_REUSE_BLOCK_CNTL, 1);
	OUT_RING(ring, 0x0000000b);                  /* PC_VERTEX_REUSE_BLOCK_CNTL */

	OUT_PKT3(ring, CP_WAIT_FOR_IDLE, 1);
	OUT_RING(ring, 0x0000000);

	OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
	OUT_RING(ring, info->min_index);        /* VFD_INDEX_MIN */
	OUT_RING(ring, info->max_index + 1);    /* VFD_INDEX_MAX */
	OUT_RING(ring, info->start_instance);   /* VFD_INSTANCEID_OFFSET */
	OUT_RING(ring, info->start);            /* VFD_INDEX_OFFSET */

	OUT_PKT0(ring, REG_A3XX_PC_RESTART_INDEX, 1);
	OUT_RING(ring, info->primitive_restart ? /* PC_RESTART_INDEX */
			info->restart_index : 0xffffffff);

	fd_draw_emit(ctx, info);
}

static void
fd3_clear(struct fd_context *ctx, unsigned buffers,
		const union pipe_color_union *color, double depth, unsigned stencil)
{
	struct fd3_context *fd3_ctx = fd3_context(ctx);
	struct fd_ringbuffer *ring = ctx->ring;
	unsigned ce, i;

	/* emit generic state now: */
	fd3_emit_state(ctx, ctx->dirty & (FD_DIRTY_VIEWPORT |
			FD_DIRTY_FRAMEBUFFER | FD_DIRTY_SCISSOR));

	OUT_PKT0(ring, REG_A3XX_RB_BLEND_ALPHA, 1);
	OUT_RING(ring, 0X3c0000ff);

	fd3_emit_rbrc_draw_state(ring,
			A3XX_RB_RENDER_CONTROL_ALPHA_TEST_FUNC(FUNC_NEVER));

	if (buffers & PIPE_CLEAR_DEPTH) {
		OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
		OUT_RING(ring, A3XX_RB_DEPTH_CONTROL_Z_WRITE_ENABLE |
				A3XX_RB_DEPTH_CONTROL_Z_ENABLE |
				A3XX_RB_DEPTH_CONTROL_ZFUNC(FUNC_ALWAYS));

		OUT_PKT0(ring, REG_A3XX_GRAS_CL_VPORT_ZOFFSET, 2);
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZOFFSET(0.0));
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZSCALE(depth));
	} else {
		OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
		OUT_RING(ring, A3XX_RB_DEPTH_CONTROL_ZFUNC(FUNC_NEVER));
	}

	if (buffers & PIPE_CLEAR_STENCIL) {
		OUT_PKT0(ring, REG_A3XX_RB_STENCILREFMASK, 2);
		OUT_RING(ring, A3XX_RB_STENCILREFMASK_STENCILREF(stencil) |
				A3XX_RB_STENCILREFMASK_STENCILMASK(stencil) |
				A3XX_RB_STENCILREFMASK_STENCILWRITEMASK(0xff));
		OUT_RING(ring, A3XX_RB_STENCILREFMASK_STENCILREF(0) |
				A3XX_RB_STENCILREFMASK_STENCILMASK(0) |
				0xff000000 | // XXX ???
				A3XX_RB_STENCILREFMASK_STENCILWRITEMASK(0xff));

		OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
		OUT_RING(ring, A3XX_RB_STENCIL_CONTROL_STENCIL_ENABLE |
				A3XX_RB_STENCIL_CONTROL_FUNC(FUNC_ALWAYS) |
				A3XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_REPLACE) |
				A3XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_FUNC_BF(FUNC_NEVER) |
				A3XX_RB_STENCIL_CONTROL_FAIL_BF(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_ZPASS_BF(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_ZFAIL_BF(STENCIL_KEEP));
	} else {
		OUT_PKT0(ring, REG_A3XX_RB_STENCILREFMASK, 2);
		OUT_RING(ring, A3XX_RB_STENCILREFMASK_STENCILREF(0) |
				A3XX_RB_STENCILREFMASK_STENCILMASK(0) |
				A3XX_RB_STENCILREFMASK_STENCILWRITEMASK(0));
		OUT_RING(ring, A3XX_RB_STENCILREFMASK_BF_STENCILREF(0) |
				A3XX_RB_STENCILREFMASK_BF_STENCILMASK(0) |
				A3XX_RB_STENCILREFMASK_BF_STENCILWRITEMASK(0));

		OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
		OUT_RING(ring, A3XX_RB_STENCIL_CONTROL_FUNC(FUNC_NEVER) |
				A3XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_FUNC_BF(FUNC_NEVER) |
				A3XX_RB_STENCIL_CONTROL_FAIL_BF(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_ZPASS_BF(STENCIL_KEEP) |
				A3XX_RB_STENCIL_CONTROL_ZFAIL_BF(STENCIL_KEEP));
	}

	if (buffers & PIPE_CLEAR_COLOR) {
		ce = 0xf;
	} else {
		ce = 0x0;
	}

	for (i = 0; i < 4; i++) {
		OUT_PKT0(ring, REG_A3XX_RB_MRT_CONTROL(i), 1);
		OUT_RING(ring, A3XX_RB_MRT_CONTROL_ROP_CODE(12) |
				A3XX_RB_MRT_CONTROL_DITHER_MODE(DITHER_ALWAYS) |
				A3XX_RB_MRT_CONTROL_COMPONENT_ENABLE(ce));

		OUT_PKT0(ring, REG_A3XX_RB_MRT_BLEND_CONTROL(i), 1);
		OUT_RING(ring, A3XX_RB_MRT_BLEND_CONTROL_RGB_SRC_FACTOR(FACTOR_ONE) |
				A3XX_RB_MRT_BLEND_CONTROL_RGB_BLEND_OPCODE(BLEND_DST_PLUS_SRC) |
				A3XX_RB_MRT_BLEND_CONTROL_RGB_DEST_FACTOR(FACTOR_ZERO) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_SRC_FACTOR(FACTOR_ONE) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_BLEND_OPCODE(BLEND_DST_PLUS_SRC) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_DEST_FACTOR(FACTOR_ZERO) |
				A3XX_RB_MRT_BLEND_CONTROL_CLAMP_ENABLE);
	}

	OUT_PKT0(ring, REG_A3XX_GRAS_SU_MODE_CONTROL, 1);
	OUT_RING(ring, A3XX_GRAS_SU_MODE_CONTROL_LINEHALFWIDTH(0));

	fd3_program_emit(ring, &ctx->solid_prog);

	fd3_emit_vertex_bufs(ring, &ctx->solid_prog, (struct fd3_vertex_buf[]) {
			{ .prsc = fd3_ctx->solid_vbuf, .stride = 12, .format = PIPE_FORMAT_R32G32B32_FLOAT },
		}, 1);

	fd3_emit_constant(ring, SB_FRAG_SHADER, 0, 0, 4, color->ui, NULL);

	OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
	OUT_RING(ring, A3XX_PC_PRIM_VTX_CNTL_STRIDE_IN_VPC(0) |
			A3XX_PC_PRIM_VTX_CNTL_POLYMODE_FRONT_PTYPE(PC_DRAW_TRIANGLES) |
			A3XX_PC_PRIM_VTX_CNTL_POLYMODE_BACK_PTYPE(PC_DRAW_TRIANGLES) |
			A3XX_PC_PRIM_VTX_CNTL_PROVOKING_VTX_LAST);
	OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
	OUT_RING(ring, 0);            /* VFD_INDEX_MIN */
	OUT_RING(ring, 2);            /* VFD_INDEX_MAX */
	OUT_RING(ring, 0);            /* VFD_INSTANCEID_OFFSET */
	OUT_RING(ring, 0);            /* VFD_INDEX_OFFSET */
	OUT_PKT0(ring, REG_A3XX_PC_RESTART_INDEX, 1);
	OUT_RING(ring, 0xffffffff);   /* PC_RESTART_INDEX */

	OUT_PKT3(ring, CP_EVENT_WRITE, 1);
	OUT_RING(ring, PERFCOUNTER_STOP);

	OUT_PKT3(ring, CP_DRAW_INDX, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, DRAW(DI_PT_RECTLIST, DI_SRC_SEL_AUTO_INDEX,
			INDEX_SIZE_IGN, IGNORE_VISIBILITY));
	OUT_RING(ring, 2);					/* NumIndices */

	OUT_PKT3(ring, CP_WAIT_FOR_IDLE, 1);
	OUT_RING(ring, 0x00000000);
}

void
fd3_draw_init(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->draw = fd3_draw;
	ctx->clear = fd3_clear;
}
