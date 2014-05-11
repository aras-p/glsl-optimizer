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
#include "util/u_helpers.h"

#include "freedreno_resource.h"

#include "fd2_emit.h"
#include "fd2_blend.h"
#include "fd2_context.h"
#include "fd2_program.h"
#include "fd2_rasterizer.h"
#include "fd2_texture.h"
#include "fd2_util.h"
#include "fd2_zsa.h"

/* NOTE: just define the position for const regs statically.. the blob
 * driver doesn't seem to change these dynamically, and I can't really
 * think of a good reason to so..
 */
#define VS_CONST_BASE 0x20
#define PS_CONST_BASE 0x120

static void
emit_constants(struct fd_ringbuffer *ring, uint32_t base,
		struct fd_constbuf_stateobj *constbuf,
		struct fd2_shader_stateobj *shader)
{
	uint32_t enabled_mask = constbuf->enabled_mask;
	uint32_t start_base = base;
	unsigned i;

	// XXX TODO only emit dirty consts.. but we need to keep track if
	// they are clobbered by a clear, gmem2mem, or mem2gmem..
	constbuf->dirty_mask = enabled_mask;

	/* emit user constants: */
	while (enabled_mask) {
		unsigned index = ffs(enabled_mask) - 1;
		struct pipe_constant_buffer *cb = &constbuf->cb[index];
		unsigned size = align(cb->buffer_size, 4) / 4; /* size in dwords */

		// I expect that size should be a multiple of vec4's:
		assert(size == align(size, 4));

		/* hmm, sometimes we still seem to end up with consts bound,
		 * even if shader isn't using them, which ends up overwriting
		 * const reg's used for immediates.. this is a hack to work
		 * around that:
		 */
		if (shader && ((base - start_base) >= (shader->first_immediate * 4)))
			break;

		if (constbuf->dirty_mask & (1 << index)) {
			const uint32_t *dwords;

			if (cb->user_buffer) {
				dwords = cb->user_buffer;
			} else {
				struct fd_resource *rsc = fd_resource(cb->buffer);
				dwords = fd_bo_map(rsc->bo);
			}

			dwords = (uint32_t *)(((uint8_t *)dwords) + cb->buffer_offset);

			OUT_PKT3(ring, CP_SET_CONSTANT, size + 1);
			OUT_RING(ring, base);
			for (i = 0; i < size; i++)
				OUT_RING(ring, *(dwords++));

			constbuf->dirty_mask &= ~(1 << index);
		}

		base += size;
		enabled_mask &= ~(1 << index);
	}

	/* emit shader immediates: */
	if (shader) {
		for (i = 0; i < shader->num_immediates; i++) {
			OUT_PKT3(ring, CP_SET_CONSTANT, 5);
			OUT_RING(ring, start_base + (4 * (shader->first_immediate + i)));
			OUT_RING(ring, shader->immediates[i].val[0]);
			OUT_RING(ring, shader->immediates[i].val[1]);
			OUT_RING(ring, shader->immediates[i].val[2]);
			OUT_RING(ring, shader->immediates[i].val[3]);
			base += 4;
		}
	}
}

typedef uint32_t texmask;

static texmask
emit_texture(struct fd_ringbuffer *ring, struct fd_context *ctx,
		struct fd_texture_stateobj *tex, unsigned samp_id, texmask emitted)
{
	unsigned const_idx = fd2_get_const_idx(ctx, tex, samp_id);
	static const struct fd2_sampler_stateobj dummy_sampler = {};
	const struct fd2_sampler_stateobj *sampler;
	struct fd2_pipe_sampler_view *view;

	if (emitted & (1 << const_idx))
		return 0;

	sampler = tex->samplers[samp_id] ?
			fd2_sampler_stateobj(tex->samplers[samp_id]) :
			&dummy_sampler;
	view = fd2_pipe_sampler_view(tex->textures[samp_id]);

	OUT_PKT3(ring, CP_SET_CONSTANT, 7);
	OUT_RING(ring, 0x00010000 + (0x6 * const_idx));

	OUT_RING(ring, sampler->tex0 | view->tex0);
	OUT_RELOC(ring, view->tex_resource->bo, 0, view->fmt, 0);
	OUT_RING(ring, view->tex2);
	OUT_RING(ring, sampler->tex3 | view->tex3);
	OUT_RING(ring, sampler->tex4);
	OUT_RING(ring, sampler->tex5);

	return (1 << const_idx);
}

static void
emit_textures(struct fd_ringbuffer *ring, struct fd_context *ctx)
{
	texmask emitted = 0;
	unsigned i;

	for (i = 0; i < ctx->verttex.num_samplers; i++)
		if (ctx->verttex.samplers[i])
			emitted |= emit_texture(ring, ctx, &ctx->verttex, i, emitted);

	for (i = 0; i < ctx->fragtex.num_samplers; i++)
		if (ctx->fragtex.samplers[i])
			emitted |= emit_texture(ring, ctx, &ctx->fragtex, i, emitted);
}

void
fd2_emit_vertex_bufs(struct fd_ringbuffer *ring, uint32_t val,
		struct fd2_vertex_buf *vbufs, uint32_t n)
{
	unsigned i;

	OUT_PKT3(ring, CP_SET_CONSTANT, 1 + (2 * n));
	OUT_RING(ring, (0x1 << 16) | (val & 0xffff));
	for (i = 0; i < n; i++) {
		struct fd_resource *rsc = fd_resource(vbufs[i].prsc);
		OUT_RELOC(ring, rsc->bo, vbufs[i].offset, 3, 0);
		OUT_RING (ring, vbufs[i].size);
	}
}

void
fd2_emit_state(struct fd_context *ctx, uint32_t dirty)
{
	struct fd2_blend_stateobj *blend = fd2_blend_stateobj(ctx->blend);
	struct fd2_zsa_stateobj *zsa = fd2_zsa_stateobj(ctx->zsa);
	struct fd_ringbuffer *ring = ctx->ring;

	/* NOTE: we probably want to eventually refactor this so each state
	 * object handles emitting it's own state..  although the mapping of
	 * state to registers is not always orthogonal, sometimes a single
	 * register contains bitfields coming from multiple state objects,
	 * so not sure the best way to deal with that yet.
	 */

	if (dirty & FD_DIRTY_SAMPLE_MASK) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_MASK));
		OUT_RING(ring, ctx->sample_mask);
	}

	if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_STENCIL_REF)) {
		struct pipe_stencil_ref *sr = &ctx->stencil_ref;

		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTHCONTROL));
		OUT_RING(ring, zsa->rb_depthcontrol);

		OUT_PKT3(ring, CP_SET_CONSTANT, 4);
		OUT_RING(ring, CP_REG(REG_A2XX_RB_STENCILREFMASK_BF));
		OUT_RING(ring, zsa->rb_stencilrefmask_bf |
				A2XX_RB_STENCILREFMASK_STENCILREF(sr->ref_value[1]));
		OUT_RING(ring, zsa->rb_stencilrefmask |
				A2XX_RB_STENCILREFMASK_STENCILREF(sr->ref_value[0]));
		OUT_RING(ring, zsa->rb_alpha_ref);
	}

	if (dirty & (FD_DIRTY_RASTERIZER | FD_DIRTY_FRAMEBUFFER)) {
		struct fd2_rasterizer_stateobj *rasterizer =
				fd2_rasterizer_stateobj(ctx->rasterizer);
		OUT_PKT3(ring, CP_SET_CONSTANT, 3);
		OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_CLIP_CNTL));
		OUT_RING(ring, rasterizer->pa_cl_clip_cntl);
		OUT_RING(ring, rasterizer->pa_su_sc_mode_cntl |
				A2XX_PA_SU_SC_MODE_CNTL_VTX_WINDOW_OFFSET_ENABLE);

		OUT_PKT3(ring, CP_SET_CONSTANT, 5);
		OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_POINT_SIZE));
		OUT_RING(ring, rasterizer->pa_su_point_size);
		OUT_RING(ring, rasterizer->pa_su_point_minmax);
		OUT_RING(ring, rasterizer->pa_su_line_cntl);
		OUT_RING(ring, rasterizer->pa_sc_line_stipple);

		OUT_PKT3(ring, CP_SET_CONSTANT, 6);
		OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_VTX_CNTL));
		OUT_RING(ring, rasterizer->pa_su_vtx_cntl);
		OUT_RING(ring, fui(1.0));                /* PA_CL_GB_VERT_CLIP_ADJ */
		OUT_RING(ring, fui(1.0));                /* PA_CL_GB_VERT_DISC_ADJ */
		OUT_RING(ring, fui(1.0));                /* PA_CL_GB_HORZ_CLIP_ADJ */
		OUT_RING(ring, fui(1.0));                /* PA_CL_GB_HORZ_DISC_ADJ */
	}

	if (dirty & FD_DIRTY_SCISSOR) {
		struct pipe_scissor_state *scissor = fd_context_get_scissor(ctx);

		OUT_PKT3(ring, CP_SET_CONSTANT, 3);
		OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
		OUT_RING(ring, xy2d(scissor->minx,       /* PA_SC_WINDOW_SCISSOR_TL */
				scissor->miny));
		OUT_RING(ring, xy2d(scissor->maxx,       /* PA_SC_WINDOW_SCISSOR_BR */
				scissor->maxy));

		ctx->max_scissor.minx = MIN2(ctx->max_scissor.minx, scissor->minx);
		ctx->max_scissor.miny = MIN2(ctx->max_scissor.miny, scissor->miny);
		ctx->max_scissor.maxx = MAX2(ctx->max_scissor.maxx, scissor->maxx);
		ctx->max_scissor.maxy = MAX2(ctx->max_scissor.maxy, scissor->maxy);
	}

	if (dirty & FD_DIRTY_VIEWPORT) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 7);
		OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_XSCALE));
		OUT_RING(ring, fui(ctx->viewport.scale[0]));       /* PA_CL_VPORT_XSCALE */
		OUT_RING(ring, fui(ctx->viewport.translate[0]));   /* PA_CL_VPORT_XOFFSET */
		OUT_RING(ring, fui(ctx->viewport.scale[1]));       /* PA_CL_VPORT_YSCALE */
		OUT_RING(ring, fui(ctx->viewport.translate[1]));   /* PA_CL_VPORT_YOFFSET */
		OUT_RING(ring, fui(ctx->viewport.scale[2]));       /* PA_CL_VPORT_ZSCALE */
		OUT_RING(ring, fui(ctx->viewport.translate[2]));   /* PA_CL_VPORT_ZOFFSET */

		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VTE_CNTL));
		OUT_RING(ring, A2XX_PA_CL_VTE_CNTL_VTX_W0_FMT |
				A2XX_PA_CL_VTE_CNTL_VPORT_X_SCALE_ENA |
				A2XX_PA_CL_VTE_CNTL_VPORT_X_OFFSET_ENA |
				A2XX_PA_CL_VTE_CNTL_VPORT_Y_SCALE_ENA |
				A2XX_PA_CL_VTE_CNTL_VPORT_Y_OFFSET_ENA |
				A2XX_PA_CL_VTE_CNTL_VPORT_Z_SCALE_ENA |
				A2XX_PA_CL_VTE_CNTL_VPORT_Z_OFFSET_ENA);
	}

	if (dirty & (FD_DIRTY_PROG | FD_DIRTY_VTXSTATE | FD_DIRTY_TEXSTATE)) {
		fd2_program_validate(ctx);
		fd2_program_emit(ring, &ctx->prog);
	}

	if (dirty & (FD_DIRTY_PROG | FD_DIRTY_CONSTBUF)) {
		emit_constants(ring,  VS_CONST_BASE * 4,
				&ctx->constbuf[PIPE_SHADER_VERTEX],
				(dirty & FD_DIRTY_PROG) ? ctx->prog.vp : NULL);
		emit_constants(ring, PS_CONST_BASE * 4,
				&ctx->constbuf[PIPE_SHADER_FRAGMENT],
				(dirty & FD_DIRTY_PROG) ? ctx->prog.fp : NULL);
	}

	if (dirty & (FD_DIRTY_BLEND | FD_DIRTY_ZSA)) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_A2XX_RB_COLORCONTROL));
		OUT_RING(ring, zsa->rb_colorcontrol | blend->rb_colorcontrol);
	}

	if (dirty & FD_DIRTY_BLEND) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_CONTROL));
		OUT_RING(ring, blend->rb_blendcontrol);

		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_MASK));
		OUT_RING(ring, blend->rb_colormask);
	}

	if (dirty & (FD_DIRTY_VERTTEX | FD_DIRTY_FRAGTEX | FD_DIRTY_PROG))
		emit_textures(ring, ctx);

	ctx->dirty &= ~dirty;
}

/* emit per-context initialization:
 */
void
fd2_emit_setup(struct fd_context *ctx)
{
	struct fd_ringbuffer *ring = ctx->ring;

	OUT_PKT0(ring, REG_A2XX_TP0_CHICKEN, 1);
	OUT_RING(ring, 0x00000002);

	OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
	OUT_RING(ring, 0x00007fff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_SQ_VS_CONST));
	OUT_RING(ring, A2XX_SQ_VS_CONST_BASE(VS_CONST_BASE) |
			A2XX_SQ_VS_CONST_SIZE(0x100));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_SQ_PS_CONST));
	OUT_RING(ring, A2XX_SQ_PS_CONST_BASE(PS_CONST_BASE) |
			A2XX_SQ_PS_CONST_SIZE(0xe0));

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
	OUT_RING(ring, 0xffffffff);        /* VGT_MAX_VTX_INDX */
	OUT_RING(ring, 0x00000000);        /* VGT_MIN_VTX_INDX */

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
	OUT_RING(ring, 0x0000003b);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_SQ_CONTEXT_MISC));
	OUT_RING(ring, A2XX_SQ_CONTEXT_MISC_SC_SAMPLE_CNTL(CENTERS_ONLY));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_SQ_INTERPOLATOR_CNTL));
	OUT_RING(ring, 0xffffffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_CONFIG));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_LINE_CNTL));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_OFFSET));
	OUT_RING(ring, 0x00000000);

	// XXX we change this dynamically for draw/clear.. vs gmem<->mem..
	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_MODECONTROL));
	OUT_RING(ring, A2XX_RB_MODECONTROL_EDRAM_MODE(COLOR_DEPTH));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_SAMPLE_POS));
	OUT_RING(ring, 0x88888888);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_DEST_MASK));
	OUT_RING(ring, 0xffffffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_DEST_INFO));
	OUT_RING(ring, A2XX_RB_COPY_DEST_INFO_FORMAT(COLORX_4_4_4_4) |
			A2XX_RB_COPY_DEST_INFO_WRITE_RED |
			A2XX_RB_COPY_DEST_INFO_WRITE_GREEN |
			A2XX_RB_COPY_DEST_INFO_WRITE_BLUE |
			A2XX_RB_COPY_DEST_INFO_WRITE_ALPHA);

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_A2XX_SQ_WRAPPING_0));
	OUT_RING(ring, 0x00000000);        /* SQ_WRAPPING_0 */
	OUT_RING(ring, 0x00000000);        /* SQ_WRAPPING_1 */

	OUT_PKT3(ring, CP_SET_DRAW_INIT_FLAGS, 1);
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_WAIT_REG_EQ, 4);
	OUT_RING(ring, 0x000005d0);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x5f601000);
	OUT_RING(ring, 0x00000001);

	OUT_PKT0(ring, REG_A2XX_SQ_INST_STORE_MANAGMENT, 1);
	OUT_RING(ring, 0x00000180);

	OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
	OUT_RING(ring, 0x00000300);

	OUT_PKT3(ring, CP_SET_SHADER_BASES, 1);
	OUT_RING(ring, 0x80000180);

	/* not sure what this form of CP_SET_CONSTANT is.. */
	OUT_PKT3(ring, CP_SET_CONSTANT, 13);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x469c4000);
	OUT_RING(ring, 0x3f800000);
	OUT_RING(ring, 0x3f000000);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x40000000);
	OUT_RING(ring, 0x3f400000);
	OUT_RING(ring, 0x3ec00000);
	OUT_RING(ring, 0x3e800000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_MASK));
	OUT_RING(ring, A2XX_RB_COLOR_MASK_WRITE_RED |
			A2XX_RB_COLOR_MASK_WRITE_GREEN |
			A2XX_RB_COLOR_MASK_WRITE_BLUE |
			A2XX_RB_COLOR_MASK_WRITE_ALPHA);

	OUT_PKT3(ring, CP_SET_CONSTANT, 5);
	OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_RED));
	OUT_RING(ring, 0x00000000);        /* RB_BLEND_RED */
	OUT_RING(ring, 0x00000000);        /* RB_BLEND_GREEN */
	OUT_RING(ring, 0x00000000);        /* RB_BLEND_BLUE */
	OUT_RING(ring, 0x000000ff);        /* RB_BLEND_ALPHA */

	fd_ringbuffer_flush(ring);
	fd_ringmarker_mark(ctx->draw_start);
}
