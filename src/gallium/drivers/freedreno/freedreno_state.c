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
#include "util/u_helpers.h"

#include "freedreno_state.h"
#include "freedreno_context.h"
#include "freedreno_zsa.h"
#include "freedreno_rasterizer.h"
#include "freedreno_blend.h"
#include "freedreno_program.h"
#include "freedreno_resource.h"
#include "freedreno_texture.h"
#include "freedreno_gmem.h"
#include "freedreno_util.h"

static void
fd_set_blend_color(struct pipe_context *pctx,
		const struct pipe_blend_color *blend_color)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->blend_color = *blend_color;
	ctx->dirty |= FD_DIRTY_BLEND_COLOR;
}

static void
fd_set_stencil_ref(struct pipe_context *pctx,
		const struct pipe_stencil_ref *stencil_ref)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->stencil_ref =* stencil_ref;
	ctx->dirty |= FD_DIRTY_STENCIL_REF;
}

static void
fd_set_clip_state(struct pipe_context *pctx,
		const struct pipe_clip_state *clip)
{
	DBG("TODO: ");
}

static void
fd_set_sample_mask(struct pipe_context *pctx, unsigned sample_mask)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->sample_mask = (uint16_t)sample_mask;
	ctx->dirty |= FD_DIRTY_SAMPLE_MASK;
}

/* notes from calim on #dri-devel:
 * index==0 will be non-UBO (ie. glUniformXYZ()) all packed together padded
 * out to vec4's
 * I should be able to consider that I own the user_ptr until the next
 * set_constant_buffer() call, at which point I don't really care about the
 * previous values.
 * index>0 will be UBO's.. well, I'll worry about that later
 */
static void
fd_set_constant_buffer(struct pipe_context *pctx, uint shader, uint index,
		struct pipe_constant_buffer *cb)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_constbuf_stateobj *so = &ctx->constbuf[shader];

	/* Note that the state tracker can unbind constant buffers by
	 * passing NULL here.
	 */
	if (unlikely(!cb)) {
		so->enabled_mask &= ~(1 << index);
		so->dirty_mask &= ~(1 << index);
		pipe_resource_reference(&so->cb[index].buffer, NULL);
		return;
	}

	pipe_resource_reference(&so->cb[index].buffer, cb->buffer);
	so->cb[index].buffer_offset = cb->buffer_offset;
	so->cb[index].buffer_size   = cb->buffer_size;
	so->cb[index].user_buffer   = cb->user_buffer;

	so->enabled_mask |= 1 << index;
	so->dirty_mask |= 1 << index;
	ctx->dirty |= FD_DIRTY_CONSTBUF;
}

static void
fd_set_framebuffer_state(struct pipe_context *pctx,
		const struct pipe_framebuffer_state *framebuffer)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_framebuffer_state *cso = &ctx->framebuffer;
	unsigned i;

	DBG("%d: cbufs[0]=%p, zsbuf=%p", ctx->needs_flush,
			cso->cbufs[0], cso->zsbuf);

	fd_context_render(pctx);

	for (i = 0; i < framebuffer->nr_cbufs; i++)
		pipe_surface_reference(&cso->cbufs[i], framebuffer->cbufs[i]);
	for (; i < ctx->framebuffer.nr_cbufs; i++)
		pipe_surface_reference(&cso->cbufs[i], NULL);

	cso->nr_cbufs = framebuffer->nr_cbufs;
	cso->width = framebuffer->width;
	cso->height = framebuffer->height;

	pipe_surface_reference(&cso->zsbuf, framebuffer->zsbuf);

	ctx->dirty |= FD_DIRTY_FRAMEBUFFER;
}

static void
fd_set_polygon_stipple(struct pipe_context *pctx,
		const struct pipe_poly_stipple *stipple)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->stipple = *stipple;
	ctx->dirty |= FD_DIRTY_STIPPLE;
}

static void
fd_set_scissor_state(struct pipe_context *pctx,
		const struct pipe_scissor_state *scissor)
{
	struct fd_context *ctx = fd_context(pctx);

	ctx->scissor = *scissor;
	ctx->dirty |= FD_DIRTY_SCISSOR;
}

static void
fd_set_viewport_state(struct pipe_context *pctx,
		const struct pipe_viewport_state *viewport)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->viewport = *viewport;
	ctx->dirty |= FD_DIRTY_VIEWPORT;
}

static void
fd_set_vertex_buffers(struct pipe_context *pctx,
		unsigned start_slot, unsigned count,
		const struct pipe_vertex_buffer *vb)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_vertexbuf_stateobj *so = &ctx->vertexbuf;

	util_set_vertex_buffers_mask(so->vb, &so->enabled_mask, vb, start_slot, count);
	so->count = util_last_bit(so->enabled_mask);

	ctx->dirty |= FD_DIRTY_VERTEXBUF;
}

static void
fd_set_index_buffer(struct pipe_context *pctx,
		const struct pipe_index_buffer *ib)
{
	struct fd_context *ctx = fd_context(pctx);

	if (ib) {
		pipe_resource_reference(&ctx->indexbuf.buffer, ib->buffer);
		ctx->indexbuf.index_size = ib->index_size;
		ctx->indexbuf.offset = ib->offset;
		ctx->indexbuf.user_buffer = ib->user_buffer;
	} else {
		pipe_resource_reference(&ctx->indexbuf.buffer, NULL);
	}

	ctx->dirty |= FD_DIRTY_INDEXBUF;
}

void
fd_state_init(struct pipe_context *pctx)
{
	pctx->set_blend_color = fd_set_blend_color;
	pctx->set_stencil_ref = fd_set_stencil_ref;
	pctx->set_clip_state = fd_set_clip_state;
	pctx->set_sample_mask = fd_set_sample_mask;
	pctx->set_constant_buffer = fd_set_constant_buffer;
	pctx->set_framebuffer_state = fd_set_framebuffer_state;
	pctx->set_polygon_stipple = fd_set_polygon_stipple;
	pctx->set_scissor_state = fd_set_scissor_state;
	pctx->set_viewport_state = fd_set_viewport_state;

	pctx->set_vertex_buffers = fd_set_vertex_buffers;
	pctx->set_index_buffer = fd_set_index_buffer;
}

/* NOTE: just define the position for const regs statically.. the blob
 * driver doesn't seem to change these dynamically, and I can't really
 * think of a good reason to so..
 */
#define VS_CONST_BASE 0x20
#define PS_CONST_BASE 0x120

static void
emit_constants(struct fd_ringbuffer *ring, uint32_t base,
		struct fd_constbuf_stateobj *constbuf,
		struct fd_shader_stateobj *shader)
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
		unsigned size = ALIGN(cb->buffer_size, 4) / 4; /* size in dwords */

		// I expect that size should be a multiple of vec4's:
		assert(size == ALIGN(size, 4));

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
			OUT_RING(ring, base);
			OUT_RING(ring, shader->immediates[i].val[0]);
			OUT_RING(ring, shader->immediates[i].val[1]);
			OUT_RING(ring, shader->immediates[i].val[2]);
			OUT_RING(ring, shader->immediates[i].val[3]);
			base += 4;
		}
	}
}

/* this works at least for a220 and earlier.. if later gpu's gain more than
 * 32 texture units, might need to bump this up to uint64_t
 */
typedef uint32_t texmask;

static texmask
emit_texture(struct fd_ringbuffer *ring, struct fd_context *ctx,
		struct fd_texture_stateobj *tex, unsigned samp_id, texmask emitted)
{
	unsigned const_idx = fd_get_const_idx(ctx, tex, samp_id);
	struct fd_sampler_stateobj *sampler;
	struct fd_pipe_sampler_view *view;

	if (emitted & (1 << const_idx))
		return 0;

	sampler = tex->samplers[samp_id];
	view = fd_pipe_sampler_view(tex->textures[samp_id]);

	OUT_PKT3(ring, CP_SET_CONSTANT, 7);
	OUT_RING(ring, 0x00010000 + (0x6 * const_idx));

	OUT_RING(ring, sampler->tex0 | view->tex0);
	OUT_RELOC(ring, view->tex_resource->bo, 0, view->fmt);
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
fd_emit_vertex_bufs(struct fd_ringbuffer *ring, uint32_t val,
		struct fd_vertex_buf *vbufs, uint32_t n)
{
	unsigned i;

	OUT_PKT3(ring, CP_SET_CONSTANT, 1 + (2 * n));
	OUT_RING(ring, (0x1 << 16) | (val & 0xffff));
	for (i = 0; i < n; i++) {
		struct fd_resource *rsc = fd_resource(vbufs[i].prsc);
		OUT_RELOC(ring, rsc->bo, vbufs[i].offset, 3);
		OUT_RING (ring, vbufs[i].size);
	}
}

void
fd_state_emit(struct pipe_context *pctx, uint32_t dirty)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_ringbuffer *ring = ctx->ring;

	/* NOTE: we probably want to eventually refactor this so each state
	 * object handles emitting it's own state..  although the mapping of
	 * state to registers is not always orthogonal, sometimes a single
	 * register contains bitfields coming from multiple state objects,
	 * so not sure the best way to deal with that yet.
	 */

	if (dirty & FD_DIRTY_SAMPLE_MASK) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_PA_SC_AA_MASK));
		OUT_RING(ring, ctx->sample_mask);
	}

	if (dirty & FD_DIRTY_ZSA) {
		struct pipe_stencil_ref *sr = &ctx->stencil_ref;

		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_RB_DEPTHCONTROL));
		OUT_RING(ring, ctx->zsa->rb_depthcontrol);

		OUT_PKT3(ring, CP_SET_CONSTANT, 4);
		OUT_RING(ring, CP_REG(REG_RB_STENCILREFMASK_BF));
		OUT_RING(ring, ctx->zsa->rb_stencilrefmask_bf |
				RB_STENCILREFMASK_STENCILREF(sr->ref_value[1]));
		OUT_RING(ring, ctx->zsa->rb_stencilrefmask |
				RB_STENCILREFMASK_STENCILREF(sr->ref_value[0]));
		OUT_RING(ring, ctx->zsa->rb_alpha_ref);
	}

	if (dirty & (FD_DIRTY_RASTERIZER | FD_DIRTY_FRAMEBUFFER)) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 3);
		OUT_RING(ring, CP_REG(REG_PA_CL_CLIP_CNTL));
		OUT_RING(ring, ctx->rasterizer->pa_cl_clip_cntl);
		OUT_RING(ring, ctx->rasterizer->pa_su_sc_mode_cntl |
				PA_SU_SC_MODE_CNTL_VTX_WINDOW_OFFSET_ENABLE);

		OUT_PKT3(ring, CP_SET_CONSTANT, 5);
		OUT_RING(ring, CP_REG(REG_PA_SU_POINT_SIZE));
		OUT_RING(ring, ctx->rasterizer->pa_su_point_size);
		OUT_RING(ring, ctx->rasterizer->pa_su_point_minmax);
		OUT_RING(ring, ctx->rasterizer->pa_su_line_cntl);
		OUT_RING(ring, ctx->rasterizer->pa_sc_line_stipple);

		OUT_PKT3(ring, CP_SET_CONSTANT, 6);
		OUT_RING(ring, CP_REG(REG_PA_SU_VTX_CNTL));
		OUT_RING(ring, ctx->rasterizer->pa_su_vtx_cntl);
		OUT_RING(ring, f2d(1.0));                /* PA_CL_GB_VERT_CLIP_ADJ */
		OUT_RING(ring, f2d(1.0));                /* PA_CL_GB_VERT_DISC_ADJ */
		OUT_RING(ring, f2d(1.0));                /* PA_CL_GB_HORZ_CLIP_ADJ */
		OUT_RING(ring, f2d(1.0));                /* PA_CL_GB_HORZ_DISC_ADJ */
	}

	if (dirty & FD_DIRTY_SCISSOR) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 3);
		OUT_RING(ring, CP_REG(REG_PA_SC_WINDOW_SCISSOR_TL));
		OUT_RING(ring, xy2d(ctx->scissor.minx,   /* PA_SC_WINDOW_SCISSOR_TL */
				ctx->scissor.miny));
		OUT_RING(ring, xy2d(ctx->scissor.maxx,   /* PA_SC_WINDOW_SCISSOR_BR */
				ctx->scissor.maxy));

		ctx->max_scissor.minx = min(ctx->max_scissor.minx, ctx->scissor.minx);
		ctx->max_scissor.miny = min(ctx->max_scissor.miny, ctx->scissor.miny);
		ctx->max_scissor.maxx = max(ctx->max_scissor.maxx, ctx->scissor.maxx);
		ctx->max_scissor.maxy = max(ctx->max_scissor.maxy, ctx->scissor.maxy);
	}

	if (dirty & FD_DIRTY_VIEWPORT) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 7);
		OUT_RING(ring, CP_REG(REG_PA_CL_VPORT_XSCALE));
		OUT_RING(ring, f2d(ctx->viewport.scale[0]));       /* PA_CL_VPORT_XSCALE */
		OUT_RING(ring, f2d(ctx->viewport.translate[0]));   /* PA_CL_VPORT_XOFFSET */
		OUT_RING(ring, f2d(ctx->viewport.scale[1]));       /* PA_CL_VPORT_YSCALE */
		OUT_RING(ring, f2d(ctx->viewport.translate[1]));   /* PA_CL_VPORT_YOFFSET */
		OUT_RING(ring, f2d(ctx->viewport.scale[2]));       /* PA_CL_VPORT_ZSCALE */
		OUT_RING(ring, f2d(ctx->viewport.translate[2]));   /* PA_CL_VPORT_ZOFFSET */

		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_PA_CL_VTE_CNTL));
		OUT_RING(ring, PA_CL_VTE_CNTL_VTX_W0_FMT |
				PA_CL_VTE_CNTL_VPORT_X_SCALE_ENA |
				PA_CL_VTE_CNTL_VPORT_X_OFFSET_ENA |
				PA_CL_VTE_CNTL_VPORT_Y_SCALE_ENA |
				PA_CL_VTE_CNTL_VPORT_Y_OFFSET_ENA |
				PA_CL_VTE_CNTL_VPORT_Z_SCALE_ENA |
				PA_CL_VTE_CNTL_VPORT_Z_OFFSET_ENA);
	}

	if (dirty & (FD_DIRTY_PROG | FD_DIRTY_VTX | FD_DIRTY_VERTTEX | FD_DIRTY_FRAGTEX)) {
		fd_program_validate(ctx);
		fd_program_emit(ring, &ctx->prog);
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
		OUT_RING(ring, CP_REG(REG_RB_COLORCONTROL));
		OUT_RING(ring, ctx->zsa->rb_colorcontrol | ctx->blend->rb_colorcontrol);
	}

	if (dirty & FD_DIRTY_BLEND) {
		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_RB_BLEND_CONTROL));
		OUT_RING(ring, ctx->blend->rb_blendcontrol);

		OUT_PKT3(ring, CP_SET_CONSTANT, 2);
		OUT_RING(ring, CP_REG(REG_RB_COLOR_MASK));
		OUT_RING(ring, ctx->blend->rb_colormask);
	}

	if (dirty & (FD_DIRTY_VERTTEX | FD_DIRTY_FRAGTEX | FD_DIRTY_PROG))
		emit_textures(ring, ctx);

	ctx->dirty &= ~dirty;
}

/* emit per-context initialization:
 */
void
fd_state_emit_setup(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_ringbuffer *ring = ctx->ring;

	OUT_PKT0(ring, REG_TP0_CHICKEN, 1);
	OUT_RING(ring, 0x00000002);

	OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
	OUT_RING(ring, 0x00007fff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_SQ_VS_CONST));
	OUT_RING(ring, SQ_VS_CONST_BASE(VS_CONST_BASE) |
			SQ_VS_CONST_SIZE(0x100));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_SQ_PS_CONST));
	OUT_RING(ring, SQ_PS_CONST_BASE(PS_CONST_BASE) |
			SQ_PS_CONST_SIZE(0xe0));

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_VGT_MAX_VTX_INDX));
	OUT_RING(ring, 0xffffffff);        /* VGT_MAX_VTX_INDX */
	OUT_RING(ring, 0x00000000);        /* VGT_MIN_VTX_INDX */

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_INDX_OFFSET));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_VERTEX_REUSE_BLOCK_CNTL));
	OUT_RING(ring, 0x0000003b);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_SQ_CONTEXT_MISC));
	OUT_RING(ring, SQ_CONTEXT_MISC_SC_SAMPLE_CNTL(CENTERS_ONLY));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_SQ_INTERPOLATOR_CNTL));
	OUT_RING(ring, 0xffffffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SC_AA_CONFIG));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SC_LINE_CNTL));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SC_WINDOW_OFFSET));
	OUT_RING(ring, 0x00000000);

	// XXX we change this dynamically for draw/clear.. vs gmem<->mem..
	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_MODECONTROL));
	OUT_RING(ring, RB_MODECONTROL_EDRAM_MODE(COLOR_DEPTH));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_SAMPLE_POS));
	OUT_RING(ring, 0x88888888);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COLOR_DEST_MASK));
	OUT_RING(ring, 0xffffffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COPY_DEST_INFO));
	OUT_RING(ring, RB_COPY_DEST_INFO_FORMAT(COLORX_4_4_4_4) |
			RB_COPY_DEST_INFO_WRITE_RED |
			RB_COPY_DEST_INFO_WRITE_GREEN |
			RB_COPY_DEST_INFO_WRITE_BLUE |
			RB_COPY_DEST_INFO_WRITE_ALPHA);

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_SQ_WRAPPING_0));
	OUT_RING(ring, 0x00000000);        /* SQ_WRAPPING_0 */
	OUT_RING(ring, 0x00000000);        /* SQ_WRAPPING_1 */

	OUT_PKT3(ring, CP_SET_DRAW_INIT_FLAGS, 1);
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_WAIT_REG_EQ, 4);
	OUT_RING(ring, 0x000005d0);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x5f601000);
	OUT_RING(ring, 0x00000001);

	OUT_PKT0(ring, REG_SQ_INST_STORE_MANAGMENT, 1);
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
	OUT_RING(ring, CP_REG(REG_RB_COLOR_MASK));
	OUT_RING(ring, RB_COLOR_MASK_WRITE_RED |
			RB_COLOR_MASK_WRITE_GREEN |
			RB_COLOR_MASK_WRITE_BLUE |
			RB_COLOR_MASK_WRITE_ALPHA);

	OUT_PKT3(ring, CP_SET_CONSTANT, 5);
	OUT_RING(ring, CP_REG(REG_RB_BLEND_RED));
	OUT_RING(ring, 0x00000000);        /* RB_BLEND_RED */
	OUT_RING(ring, 0x00000000);        /* RB_BLEND_GREEN */
	OUT_RING(ring, 0x00000000);        /* RB_BLEND_BLUE */
	OUT_RING(ring, 0x000000ff);        /* RB_BLEND_ALPHA */

	fd_ringbuffer_flush(ring);
	fd_ringmarker_mark(ctx->draw_start);
}
