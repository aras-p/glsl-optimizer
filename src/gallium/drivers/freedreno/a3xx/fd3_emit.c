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
#include "util/u_helpers.h"
#include "util/u_format.h"

#include "freedreno_resource.h"

#include "fd3_emit.h"
#include "fd3_blend.h"
#include "fd3_context.h"
#include "fd3_program.h"
#include "fd3_rasterizer.h"
#include "fd3_texture.h"
#include "fd3_util.h"
#include "fd3_zsa.h"

/* regid:          base const register
 * prsc or dwords: buffer containing constant values
 * sizedwords:     size of const value buffer
 */
void
fd3_emit_constant(struct fd_ringbuffer *ring,
		enum adreno_state_block sb,
		uint32_t regid, uint32_t offset, uint32_t sizedwords,
		const uint32_t *dwords, struct pipe_resource *prsc)
{
	uint32_t i, sz;
	enum adreno_state_src src;

	if (prsc) {
		sz = 0;
		src = SS_INDIRECT;
	} else {
		sz = sizedwords;
		src = SS_DIRECT;
	}

	OUT_PKT3(ring, CP_LOAD_STATE, 2 + sz);
	OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(regid/2) |
			CP_LOAD_STATE_0_STATE_SRC(src) |
			CP_LOAD_STATE_0_STATE_BLOCK(sb) |
			CP_LOAD_STATE_0_NUM_UNIT(sizedwords/2));
	if (prsc) {
		struct fd_bo *bo = fd_resource(prsc)->bo;
		OUT_RELOC(ring, bo, offset,
				CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS), 0);
	} else {
		OUT_RING(ring, CP_LOAD_STATE_1_EXT_SRC_ADDR(0) |
				CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS));
		dwords = (uint32_t *)&((uint8_t *)dwords)[offset];
	}
	for (i = 0; i < sz; i++) {
		OUT_RING(ring, dwords[i]);
	}
}

static void
emit_constants(struct fd_ringbuffer *ring,
		enum adreno_state_block sb,
		struct fd_constbuf_stateobj *constbuf,
		struct fd3_shader_variant *shader)
{
	uint32_t enabled_mask = constbuf->enabled_mask;
	uint32_t first_immediate;
	uint32_t base = 0;
	unsigned i;

	// XXX TODO only emit dirty consts.. but we need to keep track if
	// they are clobbered by a clear, gmem2mem, or mem2gmem..
	constbuf->dirty_mask = enabled_mask;

	/* in particular, with binning shader and a unneeded consts no
	 * longer referenced, we could end up w/ constlen that is smaller
	 * than first_immediate.  In that case truncate the user consts
	 * early to avoid HLSQ lockup caused by writing too many consts
	 */
	first_immediate = MIN2(shader->first_immediate, shader->constlen);

	/* emit user constants: */
	while (enabled_mask) {
		unsigned index = ffs(enabled_mask) - 1;
		struct pipe_constant_buffer *cb = &constbuf->cb[index];
		unsigned size = align(cb->buffer_size, 4) / 4; /* size in dwords */

		// I expect that size should be a multiple of vec4's:
		assert(size == align(size, 4));

		/* gallium could leave const buffers bound above what the
		 * current shader uses.. don't let that confuse us.
		 */
		if (base >= (4 * first_immediate))
			break;

		if (constbuf->dirty_mask & (1 << index)) {
			/* and even if the start of the const buffer is before
			 * first_immediate, the end may not be:
			 */
			size = MIN2(size, (4 * first_immediate) - base);
			fd3_emit_constant(ring, sb, base,
					cb->buffer_offset, size,
					cb->user_buffer, cb->buffer);
			constbuf->dirty_mask &= ~(1 << index);
		}

		base += size;
		enabled_mask &= ~(1 << index);
	}

	/* emit shader immediates: */
	if (shader) {
		for (i = 0; i < shader->immediates_count; i++) {
			base = 4 * (shader->first_immediate + i);
			if (base >= (4 * shader->constlen))
				break;
			fd3_emit_constant(ring, sb, base,
				0, 4, shader->immediates[i].val, NULL);
		}
	}
}

#define VERT_TEX_OFF    0
#define FRAG_TEX_OFF    16
#define BASETABLE_SZ    A3XX_MAX_MIP_LEVELS

static void
emit_textures(struct fd_ringbuffer *ring,
		enum adreno_state_block sb,
		struct fd_texture_stateobj *tex)
{
	static const unsigned tex_off[] = {
			[SB_VERT_TEX] = VERT_TEX_OFF,
			[SB_FRAG_TEX] = FRAG_TEX_OFF,
	};
	static const enum adreno_state_block mipaddr[] = {
			[SB_VERT_TEX] = SB_VERT_MIPADDR,
			[SB_FRAG_TEX] = SB_FRAG_MIPADDR,
	};
	unsigned i, j;

	if (tex->num_samplers > 0) {
		/* output sampler state: */
		OUT_PKT3(ring, CP_LOAD_STATE, 2 + (2 * tex->num_samplers));
		OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(tex_off[sb]) |
				CP_LOAD_STATE_0_STATE_SRC(SS_DIRECT) |
				CP_LOAD_STATE_0_STATE_BLOCK(sb) |
				CP_LOAD_STATE_0_NUM_UNIT(tex->num_samplers));
		OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_SHADER) |
				CP_LOAD_STATE_1_EXT_SRC_ADDR(0));
		for (i = 0; i < tex->num_samplers; i++) {
			static const struct fd3_sampler_stateobj dummy_sampler = {};
			const struct fd3_sampler_stateobj *sampler = tex->samplers[i] ?
					fd3_sampler_stateobj(tex->samplers[i]) :
					&dummy_sampler;
			OUT_RING(ring, sampler->texsamp0);
			OUT_RING(ring, sampler->texsamp1);
		}
	}

	if (tex->num_textures > 0) {
		/* emit texture state: */
		OUT_PKT3(ring, CP_LOAD_STATE, 2 + (4 * tex->num_textures));
		OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(tex_off[sb]) |
				CP_LOAD_STATE_0_STATE_SRC(SS_DIRECT) |
				CP_LOAD_STATE_0_STATE_BLOCK(sb) |
				CP_LOAD_STATE_0_NUM_UNIT(tex->num_textures));
		OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
				CP_LOAD_STATE_1_EXT_SRC_ADDR(0));
		for (i = 0; i < tex->num_textures; i++) {
			static const struct fd3_pipe_sampler_view dummy_view = {};
			const struct fd3_pipe_sampler_view *view = tex->textures[i] ?
					fd3_pipe_sampler_view(tex->textures[i]) :
					&dummy_view;
			OUT_RING(ring, view->texconst0);
			OUT_RING(ring, view->texconst1);
			OUT_RING(ring, view->texconst2 |
					A3XX_TEX_CONST_2_INDX(BASETABLE_SZ * i));
			OUT_RING(ring, view->texconst3);
		}

		/* emit mipaddrs: */
		OUT_PKT3(ring, CP_LOAD_STATE, 2 + (BASETABLE_SZ * tex->num_textures));
		OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(BASETABLE_SZ * tex_off[sb]) |
				CP_LOAD_STATE_0_STATE_SRC(SS_DIRECT) |
				CP_LOAD_STATE_0_STATE_BLOCK(mipaddr[sb]) |
				CP_LOAD_STATE_0_NUM_UNIT(BASETABLE_SZ * tex->num_textures));
		OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
				CP_LOAD_STATE_1_EXT_SRC_ADDR(0));
		for (i = 0; i < tex->num_textures; i++) {
			static const struct fd3_pipe_sampler_view dummy_view = {};
			const struct fd3_pipe_sampler_view *view = tex->textures[i] ?
					fd3_pipe_sampler_view(tex->textures[i]) :
					&dummy_view;
			struct fd_resource *rsc = view->tex_resource;

			for (j = 0; j < view->mipaddrs; j++) {
				struct fd_resource_slice *slice = fd_resource_slice(rsc, j);
				OUT_RELOC(ring, rsc->bo, slice->offset, 0, 0);
			}

			/* pad the remaining entries w/ null: */
			for (; j < BASETABLE_SZ; j++) {
				OUT_RING(ring, 0x00000000);
			}
		}
	}
}

static void
emit_cache_flush(struct fd_ringbuffer *ring)
{
	OUT_PKT3(ring, CP_EVENT_WRITE, 1);
	OUT_RING(ring, CACHE_FLUSH);

	/* probably only really needed on a320: */
	OUT_PKT3(ring, CP_DRAW_INDX, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, DRAW(1, DI_SRC_SEL_AUTO_INDEX,
			INDEX_SIZE_IGN, IGNORE_VISIBILITY));
	OUT_RING(ring, 0);					/* NumIndices */

	OUT_PKT3(ring, CP_NOP, 4);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000000);
}

/* emit texture state for mem->gmem restore operation.. eventually it would
 * be good to get rid of this and use normal CSO/etc state for more of these
 * special cases, but for now the compiler is not sufficient..
 */
void
fd3_emit_gmem_restore_tex(struct fd_ringbuffer *ring, struct pipe_surface *psurf)
{
	struct fd_resource *rsc = fd_resource(psurf->texture);
	enum pipe_format format = fd3_gmem_restore_format(psurf->format);

	/* output sampler state: */
	OUT_PKT3(ring, CP_LOAD_STATE, 4);
	OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(FRAG_TEX_OFF) |
			CP_LOAD_STATE_0_STATE_SRC(SS_DIRECT) |
			CP_LOAD_STATE_0_STATE_BLOCK(SB_FRAG_TEX) |
			CP_LOAD_STATE_0_NUM_UNIT(1));
	OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_SHADER) |
			CP_LOAD_STATE_1_EXT_SRC_ADDR(0));
	OUT_RING(ring, A3XX_TEX_SAMP_0_XY_MAG(A3XX_TEX_NEAREST) |
			A3XX_TEX_SAMP_0_XY_MIN(A3XX_TEX_NEAREST) |
			A3XX_TEX_SAMP_0_WRAP_S(A3XX_TEX_CLAMP_TO_EDGE) |
			A3XX_TEX_SAMP_0_WRAP_T(A3XX_TEX_CLAMP_TO_EDGE) |
			A3XX_TEX_SAMP_0_WRAP_R(A3XX_TEX_REPEAT));
	OUT_RING(ring, 0x00000000);

	/* emit texture state: */
	OUT_PKT3(ring, CP_LOAD_STATE, 6);
	OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(FRAG_TEX_OFF) |
			CP_LOAD_STATE_0_STATE_SRC(SS_DIRECT) |
			CP_LOAD_STATE_0_STATE_BLOCK(SB_FRAG_TEX) |
			CP_LOAD_STATE_0_NUM_UNIT(1));
	OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
			CP_LOAD_STATE_1_EXT_SRC_ADDR(0));
	OUT_RING(ring, A3XX_TEX_CONST_0_FMT(fd3_pipe2tex(psurf->format)) |
			A3XX_TEX_CONST_0_TYPE(A3XX_TEX_2D) |
			fd3_tex_swiz(format,  PIPE_SWIZZLE_RED, PIPE_SWIZZLE_GREEN,
					PIPE_SWIZZLE_BLUE, PIPE_SWIZZLE_ALPHA));
	OUT_RING(ring, A3XX_TEX_CONST_1_FETCHSIZE(TFETCH_DISABLE) |
			A3XX_TEX_CONST_1_WIDTH(psurf->width) |
			A3XX_TEX_CONST_1_HEIGHT(psurf->height));
	OUT_RING(ring, A3XX_TEX_CONST_2_PITCH(rsc->slices[0].pitch * rsc->cpp) |
			A3XX_TEX_CONST_2_INDX(0));
	OUT_RING(ring, 0x00000000);

	/* emit mipaddrs: */
	OUT_PKT3(ring, CP_LOAD_STATE, 3);
	OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(BASETABLE_SZ * FRAG_TEX_OFF) |
			CP_LOAD_STATE_0_STATE_SRC(SS_DIRECT) |
			CP_LOAD_STATE_0_STATE_BLOCK(SB_FRAG_MIPADDR) |
			CP_LOAD_STATE_0_NUM_UNIT(1));
	OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
			CP_LOAD_STATE_1_EXT_SRC_ADDR(0));
	OUT_RELOC(ring, rsc->bo, 0, 0, 0);
}

void
fd3_emit_vertex_bufs(struct fd_ringbuffer *ring,
		struct fd3_shader_variant *vp,
		struct fd3_vertex_buf *vbufs, uint32_t n)
{
	uint32_t i, j, last = 0;
	uint32_t total_in = 0;

	n = MIN2(n, vp->inputs_count);

	for (i = 0; i < n; i++)
		if (vp->inputs[i].compmask)
			last = i;

	for (i = 0, j = 0; i <= last; i++) {
		if (vp->inputs[i].compmask) {
			struct pipe_resource *prsc = vbufs[i].prsc;
			struct fd_resource *rsc = fd_resource(prsc);
			enum pipe_format pfmt = vbufs[i].format;
			enum a3xx_vtx_fmt fmt = fd3_pipe2vtx(pfmt);
			bool switchnext = (i != last);
			uint32_t fs = util_format_get_blocksize(pfmt);

			debug_assert(fmt != ~0);

			OUT_PKT0(ring, REG_A3XX_VFD_FETCH(j), 2);
			OUT_RING(ring, A3XX_VFD_FETCH_INSTR_0_FETCHSIZE(fs - 1) |
					A3XX_VFD_FETCH_INSTR_0_BUFSTRIDE(vbufs[i].stride) |
					COND(switchnext, A3XX_VFD_FETCH_INSTR_0_SWITCHNEXT) |
					A3XX_VFD_FETCH_INSTR_0_INDEXCODE(j) |
					A3XX_VFD_FETCH_INSTR_0_STEPRATE(1));
			OUT_RELOC(ring, rsc->bo, vbufs[i].offset, 0, 0);

			OUT_PKT0(ring, REG_A3XX_VFD_DECODE_INSTR(j), 1);
			OUT_RING(ring, A3XX_VFD_DECODE_INSTR_CONSTFILL |
					A3XX_VFD_DECODE_INSTR_WRITEMASK(vp->inputs[i].compmask) |
					A3XX_VFD_DECODE_INSTR_FORMAT(fmt) |
					A3XX_VFD_DECODE_INSTR_SWAP(fd3_pipe2swap(pfmt)) |
					A3XX_VFD_DECODE_INSTR_REGID(vp->inputs[i].regid) |
					A3XX_VFD_DECODE_INSTR_SHIFTCNT(fs) |
					A3XX_VFD_DECODE_INSTR_LASTCOMPVALID |
					COND(switchnext, A3XX_VFD_DECODE_INSTR_SWITCHNEXT));

			total_in += vp->inputs[i].ncomp;
			j++;
		}
	}

	OUT_PKT0(ring, REG_A3XX_VFD_CONTROL_0, 2);
	OUT_RING(ring, A3XX_VFD_CONTROL_0_TOTALATTRTOVS(total_in) |
			A3XX_VFD_CONTROL_0_PACKETSIZE(2) |
			A3XX_VFD_CONTROL_0_STRMDECINSTRCNT(j) |
			A3XX_VFD_CONTROL_0_STRMFETCHINSTRCNT(j));
	OUT_RING(ring, A3XX_VFD_CONTROL_1_MAXSTORAGE(1) | // XXX
			A3XX_VFD_CONTROL_1_REGID4VTX(regid(63,0)) |
			A3XX_VFD_CONTROL_1_REGID4INST(regid(63,0)));
}

void
fd3_emit_state(struct fd_context *ctx, struct fd_ringbuffer *ring,
		struct fd_program_stateobj *prog, uint32_t dirty,
		struct fd3_shader_key key)
{
	struct fd3_shader_variant *vp;
	struct fd3_shader_variant *fp;

	fp = fd3_shader_variant(prog->fp, key);
	vp = fd3_shader_variant(prog->vp, key);

	emit_marker(ring, 5);

	if (dirty & FD_DIRTY_SAMPLE_MASK) {
		OUT_PKT0(ring, REG_A3XX_RB_MSAA_CONTROL, 1);
		OUT_RING(ring, A3XX_RB_MSAA_CONTROL_DISABLE |
				A3XX_RB_MSAA_CONTROL_SAMPLES(MSAA_ONE) |
				A3XX_RB_MSAA_CONTROL_SAMPLE_MASK(ctx->sample_mask));
	}

	if ((dirty & (FD_DIRTY_ZSA | FD_DIRTY_PROG)) && !key.binning_pass) {
		uint32_t val = fd3_zsa_stateobj(ctx->zsa)->rb_render_control;

		val |= COND(fp->frag_face, A3XX_RB_RENDER_CONTROL_FACENESS);
		val |= COND(fp->frag_coord, A3XX_RB_RENDER_CONTROL_XCOORD |
				A3XX_RB_RENDER_CONTROL_YCOORD |
				A3XX_RB_RENDER_CONTROL_ZCOORD |
				A3XX_RB_RENDER_CONTROL_WCOORD);

		/* I suppose if we needed to (which I don't *think* we need
		 * to), we could emit this for binning pass too.  But we
		 * would need to keep a different patch-list for binning
		 * vs render pass.
		 */

		OUT_PKT0(ring, REG_A3XX_RB_RENDER_CONTROL, 1);
		OUT_RINGP(ring, val, &fd3_context(ctx)->rbrc_patches);
	}

	if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_STENCIL_REF)) {
		struct fd3_zsa_stateobj *zsa = fd3_zsa_stateobj(ctx->zsa);
		struct pipe_stencil_ref *sr = &ctx->stencil_ref;

		OUT_PKT0(ring, REG_A3XX_RB_ALPHA_REF, 1);
		OUT_RING(ring, zsa->rb_alpha_ref);

		OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
		OUT_RING(ring, zsa->rb_stencil_control);

		OUT_PKT0(ring, REG_A3XX_RB_STENCILREFMASK, 2);
		OUT_RING(ring, zsa->rb_stencilrefmask |
				A3XX_RB_STENCILREFMASK_STENCILREF(sr->ref_value[0]));
		OUT_RING(ring, zsa->rb_stencilrefmask_bf |
				A3XX_RB_STENCILREFMASK_BF_STENCILREF(sr->ref_value[1]));
	}

	if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_PROG)) {
		uint32_t val = fd3_zsa_stateobj(ctx->zsa)->rb_depth_control;
		if (fp->writes_pos) {
			val |= A3XX_RB_DEPTH_CONTROL_FRAG_WRITES_Z;
			val |= A3XX_RB_DEPTH_CONTROL_EARLY_Z_DISABLE;
		}
		OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
		OUT_RING(ring, val);
	}

	if (dirty & FD_DIRTY_RASTERIZER) {
		struct fd3_rasterizer_stateobj *rasterizer =
				fd3_rasterizer_stateobj(ctx->rasterizer);

		OUT_PKT0(ring, REG_A3XX_GRAS_SU_MODE_CONTROL, 1);
		OUT_RING(ring, rasterizer->gras_su_mode_control);

		OUT_PKT0(ring, REG_A3XX_GRAS_SU_POINT_MINMAX, 2);
		OUT_RING(ring, rasterizer->gras_su_point_minmax);
		OUT_RING(ring, rasterizer->gras_su_point_size);

		OUT_PKT0(ring, REG_A3XX_GRAS_SU_POLY_OFFSET_SCALE, 2);
		OUT_RING(ring, rasterizer->gras_su_poly_offset_scale);
		OUT_RING(ring, rasterizer->gras_su_poly_offset_offset);
	}

	if (dirty & (FD_DIRTY_RASTERIZER | FD_DIRTY_PROG)) {
		uint32_t val = fd3_rasterizer_stateobj(ctx->rasterizer)
				->gras_cl_clip_cntl;
		val |= COND(fp->writes_pos, A3XX_GRAS_CL_CLIP_CNTL_ZCLIP_DISABLE);
		val |= COND(fp->frag_coord, A3XX_GRAS_CL_CLIP_CNTL_ZCOORD |
				A3XX_GRAS_CL_CLIP_CNTL_WCOORD);
		OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
		OUT_RING(ring, val);
	}

	if (dirty & (FD_DIRTY_RASTERIZER | FD_DIRTY_PROG)) {
		uint32_t val = fd3_rasterizer_stateobj(ctx->rasterizer)
				->pc_prim_vtx_cntl;

		if (!key.binning_pass) {
			uint32_t stride_in_vpc = align(fp->total_in, 4) / 4;
			if (stride_in_vpc > 0)
				stride_in_vpc = MAX2(stride_in_vpc, 2);
			val |= A3XX_PC_PRIM_VTX_CNTL_STRIDE_IN_VPC(stride_in_vpc);
		}

		val |= COND(vp->writes_psize, A3XX_PC_PRIM_VTX_CNTL_PSIZE);

		OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
		OUT_RING(ring, val);
	}

	if (dirty & FD_DIRTY_SCISSOR) {
		struct pipe_scissor_state *scissor = fd_context_get_scissor(ctx);

		OUT_PKT0(ring, REG_A3XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
		OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_TL_X(scissor->minx) |
				A3XX_GRAS_SC_WINDOW_SCISSOR_TL_Y(scissor->miny));
		OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_BR_X(scissor->maxx - 1) |
				A3XX_GRAS_SC_WINDOW_SCISSOR_BR_Y(scissor->maxy - 1));

		ctx->max_scissor.minx = MIN2(ctx->max_scissor.minx, scissor->minx);
		ctx->max_scissor.miny = MIN2(ctx->max_scissor.miny, scissor->miny);
		ctx->max_scissor.maxx = MAX2(ctx->max_scissor.maxx, scissor->maxx);
		ctx->max_scissor.maxy = MAX2(ctx->max_scissor.maxy, scissor->maxy);
	}

	if (dirty & FD_DIRTY_VIEWPORT) {
		OUT_PKT0(ring, REG_A3XX_GRAS_CL_VPORT_XOFFSET, 6);
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_XOFFSET(ctx->viewport.translate[0] - 0.5));
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_XSCALE(ctx->viewport.scale[0]));
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_YOFFSET(ctx->viewport.translate[1] - 0.5));
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_YSCALE(ctx->viewport.scale[1]));
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZOFFSET(ctx->viewport.translate[2]));
		OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZSCALE(ctx->viewport.scale[2]));
	}

	if (dirty & FD_DIRTY_PROG) {
		fd_wfi(ctx, ring);
		fd3_program_emit(ring, prog, key);
	}

	OUT_PKT3(ring, CP_EVENT_WRITE, 1);
	OUT_RING(ring, HLSQ_FLUSH);

	if ((dirty & (FD_DIRTY_PROG | FD_DIRTY_CONSTBUF)) &&
			/* evil hack to deal sanely with clear path: */
			(prog == &ctx->prog)) {
		fd_wfi(ctx, ring);
		emit_constants(ring,  SB_VERT_SHADER,
				&ctx->constbuf[PIPE_SHADER_VERTEX],
				(prog->dirty & FD_SHADER_DIRTY_VP) ? vp : NULL);
		if (!key.binning_pass) {
			emit_constants(ring, SB_FRAG_SHADER,
					&ctx->constbuf[PIPE_SHADER_FRAGMENT],
					(prog->dirty & FD_SHADER_DIRTY_FP) ? fp : NULL);
		}
	}

	if ((dirty & FD_DIRTY_BLEND) && ctx->blend) {
		struct fd3_blend_stateobj *blend = fd3_blend_stateobj(ctx->blend);
		uint32_t i;

		for (i = 0; i < ARRAY_SIZE(blend->rb_mrt); i++) {
			OUT_PKT0(ring, REG_A3XX_RB_MRT_CONTROL(i), 1);
			OUT_RING(ring, blend->rb_mrt[i].control);

			OUT_PKT0(ring, REG_A3XX_RB_MRT_BLEND_CONTROL(i), 1);
			OUT_RING(ring, blend->rb_mrt[i].blend_control);
		}
	}

	if (dirty & FD_DIRTY_BLEND_COLOR) {
		struct pipe_blend_color *bcolor = &ctx->blend_color;
		OUT_PKT0(ring, REG_A3XX_RB_BLEND_RED, 4);
		OUT_RING(ring, A3XX_RB_BLEND_RED_UINT(bcolor->color[0] * 255.0) |
				A3XX_RB_BLEND_RED_FLOAT(bcolor->color[0]));
		OUT_RING(ring, A3XX_RB_BLEND_GREEN_UINT(bcolor->color[1] * 255.0) |
				A3XX_RB_BLEND_GREEN_FLOAT(bcolor->color[1]));
		OUT_RING(ring, A3XX_RB_BLEND_BLUE_UINT(bcolor->color[2] * 255.0) |
				A3XX_RB_BLEND_BLUE_FLOAT(bcolor->color[2]));
		OUT_RING(ring, A3XX_RB_BLEND_ALPHA_UINT(bcolor->color[3] * 255.0) |
				A3XX_RB_BLEND_ALPHA_FLOAT(bcolor->color[3]));
	}

	if (dirty & (FD_DIRTY_VERTTEX | FD_DIRTY_FRAGTEX))
		fd_wfi(ctx, ring);

	if (dirty & FD_DIRTY_VERTTEX) {
		if (vp->has_samp)
			emit_textures(ring, SB_VERT_TEX, &ctx->verttex);
		else
			dirty &= ~FD_DIRTY_VERTTEX;
	}

	if (dirty & FD_DIRTY_FRAGTEX) {
		if (fp->has_samp)
			emit_textures(ring, SB_FRAG_TEX, &ctx->fragtex);
		else
			dirty &= ~FD_DIRTY_FRAGTEX;
	}

	ctx->dirty &= ~dirty;
}

/* emit setup at begin of new cmdstream buffer (don't rely on previous
 * state, there could have been a context switch between ioctls):
 */
void
fd3_emit_restore(struct fd_context *ctx)
{
	struct fd3_context *fd3_ctx = fd3_context(ctx);
	struct fd_ringbuffer *ring = ctx->ring;
	int i;

	if (ctx->screen->gpu_id == 320) {
		OUT_PKT3(ring, CP_REG_RMW, 3);
		OUT_RING(ring, REG_A3XX_RBBM_CLOCK_CTL);
		OUT_RING(ring, 0xfffcffff);
		OUT_RING(ring, 0x00000000);
	}

	OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
	OUT_RING(ring, 0x00007fff);

	OUT_PKT0(ring, REG_A3XX_SP_VS_PVT_MEM_PARAM_REG, 3);
	OUT_RING(ring, 0x08000001);                  /* SP_VS_PVT_MEM_CTRL_REG */
	OUT_RELOC(ring, fd3_ctx->vs_pvt_mem, 0,0,0); /* SP_VS_PVT_MEM_ADDR_REG */
	OUT_RING(ring, 0x00000000);                  /* SP_VS_PVT_MEM_SIZE_REG */

	OUT_PKT0(ring, REG_A3XX_SP_FS_PVT_MEM_PARAM_REG, 3);
	OUT_RING(ring, 0x08000001);                  /* SP_FS_PVT_MEM_CTRL_REG */
	OUT_RELOC(ring, fd3_ctx->fs_pvt_mem, 0,0,0); /* SP_FS_PVT_MEM_ADDR_REG */
	OUT_RING(ring, 0x00000000);                  /* SP_FS_PVT_MEM_SIZE_REG */

	OUT_PKT0(ring, REG_A3XX_PC_VERTEX_REUSE_BLOCK_CNTL, 1);
	OUT_RING(ring, 0x0000000b);                  /* PC_VERTEX_REUSE_BLOCK_CNTL */

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
	OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
			A3XX_GRAS_SC_CONTROL_MSAA_SAMPLES(MSAA_ONE) |
			A3XX_GRAS_SC_CONTROL_RASTER_MODE(0));

	OUT_PKT0(ring, REG_A3XX_RB_MSAA_CONTROL, 2);
	OUT_RING(ring, A3XX_RB_MSAA_CONTROL_DISABLE |
			A3XX_RB_MSAA_CONTROL_SAMPLES(MSAA_ONE) |
			A3XX_RB_MSAA_CONTROL_SAMPLE_MASK(0xffff));
	OUT_RING(ring, 0x00000000);        /* RB_ALPHA_REF */

	OUT_PKT0(ring, REG_A3XX_GRAS_CL_GB_CLIP_ADJ, 1);
	OUT_RING(ring, A3XX_GRAS_CL_GB_CLIP_ADJ_HORZ(0) |
			A3XX_GRAS_CL_GB_CLIP_ADJ_VERT(0));

	OUT_PKT0(ring, REG_A3XX_GRAS_TSE_DEBUG_ECO, 1);
	OUT_RING(ring, 0x00000001);        /* GRAS_TSE_DEBUG_ECO */

	OUT_PKT0(ring, REG_A3XX_TPL1_TP_VS_TEX_OFFSET, 1);
	OUT_RING(ring, A3XX_TPL1_TP_VS_TEX_OFFSET_SAMPLEROFFSET(VERT_TEX_OFF) |
			A3XX_TPL1_TP_VS_TEX_OFFSET_MEMOBJOFFSET(VERT_TEX_OFF) |
			A3XX_TPL1_TP_VS_TEX_OFFSET_BASETABLEPTR(BASETABLE_SZ * VERT_TEX_OFF));

	OUT_PKT0(ring, REG_A3XX_TPL1_TP_FS_TEX_OFFSET, 1);
	OUT_RING(ring, A3XX_TPL1_TP_FS_TEX_OFFSET_SAMPLEROFFSET(FRAG_TEX_OFF) |
			A3XX_TPL1_TP_FS_TEX_OFFSET_MEMOBJOFFSET(FRAG_TEX_OFF) |
			A3XX_TPL1_TP_FS_TEX_OFFSET_BASETABLEPTR(BASETABLE_SZ * FRAG_TEX_OFF));

	OUT_PKT0(ring, REG_A3XX_VPC_VARY_CYLWRAP_ENABLE_0, 2);
	OUT_RING(ring, 0x00000000);        /* VPC_VARY_CYLWRAP_ENABLE_0 */
	OUT_RING(ring, 0x00000000);        /* VPC_VARY_CYLWRAP_ENABLE_1 */

	OUT_PKT0(ring, REG_A3XX_UNKNOWN_0E43, 1);
	OUT_RING(ring, 0x00000001);        /* UNKNOWN_0E43 */

	OUT_PKT0(ring, REG_A3XX_UNKNOWN_0F03, 1);
	OUT_RING(ring, 0x00000001);        /* UNKNOWN_0F03 */

	OUT_PKT0(ring, REG_A3XX_UNKNOWN_0EE0, 1);
	OUT_RING(ring, 0x00000003);        /* UNKNOWN_0EE0 */

	OUT_PKT0(ring, REG_A3XX_UNKNOWN_0C3D, 1);
	OUT_RING(ring, 0x00000001);        /* UNKNOWN_0C3D */

	OUT_PKT0(ring, REG_A3XX_HLSQ_PERFCOUNTER0_SELECT, 1);
	OUT_RING(ring, 0x00000000);        /* HLSQ_PERFCOUNTER0_SELECT */

	OUT_PKT0(ring, REG_A3XX_HLSQ_CONST_VSPRESV_RANGE_REG, 2);
	OUT_RING(ring, A3XX_HLSQ_CONST_VSPRESV_RANGE_REG_STARTENTRY(0) |
			A3XX_HLSQ_CONST_VSPRESV_RANGE_REG_ENDENTRY(0));
	OUT_RING(ring, A3XX_HLSQ_CONST_FSPRESV_RANGE_REG_STARTENTRY(0) |
			A3XX_HLSQ_CONST_FSPRESV_RANGE_REG_ENDENTRY(0));

	OUT_PKT0(ring, REG_A3XX_UCHE_CACHE_INVALIDATE0_REG, 2);
	OUT_RING(ring, A3XX_UCHE_CACHE_INVALIDATE0_REG_ADDR(0));
	OUT_RING(ring, A3XX_UCHE_CACHE_INVALIDATE1_REG_ADDR(0) |
			A3XX_UCHE_CACHE_INVALIDATE1_REG_OPCODE(INVALIDATE) |
			A3XX_UCHE_CACHE_INVALIDATE1_REG_ENTIRE_CACHE);

	OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
	OUT_RING(ring, 0x00000000);                  /* GRAS_CL_CLIP_CNTL */

	OUT_PKT0(ring, REG_A3XX_GRAS_SU_POINT_MINMAX, 2);
	OUT_RING(ring, 0xffc00010);        /* GRAS_SU_POINT_MINMAX */
	OUT_RING(ring, 0x00000008);        /* GRAS_SU_POINT_SIZE */

	OUT_PKT0(ring, REG_A3XX_PC_RESTART_INDEX, 1);
	OUT_RING(ring, 0xffffffff);        /* PC_RESTART_INDEX */

	OUT_PKT0(ring, REG_A3XX_RB_WINDOW_OFFSET, 1);
	OUT_RING(ring, A3XX_RB_WINDOW_OFFSET_X(0) |
			A3XX_RB_WINDOW_OFFSET_Y(0));

	OUT_PKT0(ring, REG_A3XX_RB_BLEND_RED, 4);
	OUT_RING(ring, A3XX_RB_BLEND_RED_UINT(0) |
			A3XX_RB_BLEND_RED_FLOAT(0.0));
	OUT_RING(ring, A3XX_RB_BLEND_GREEN_UINT(0) |
			A3XX_RB_BLEND_GREEN_FLOAT(0.0));
	OUT_RING(ring, A3XX_RB_BLEND_BLUE_UINT(0) |
			A3XX_RB_BLEND_BLUE_FLOAT(0.0));
	OUT_RING(ring, A3XX_RB_BLEND_ALPHA_UINT(0xff) |
			A3XX_RB_BLEND_ALPHA_FLOAT(1.0));

	for (i = 0; i < 6; i++) {
		OUT_PKT0(ring, REG_A3XX_GRAS_CL_USER_PLANE(i), 4);
		OUT_RING(ring, 0x00000000);    /* GRAS_CL_USER_PLANE[i].X */
		OUT_RING(ring, 0x00000000);    /* GRAS_CL_USER_PLANE[i].Y */
		OUT_RING(ring, 0x00000000);    /* GRAS_CL_USER_PLANE[i].Z */
		OUT_RING(ring, 0x00000000);    /* GRAS_CL_USER_PLANE[i].W */
	}

	OUT_PKT0(ring, REG_A3XX_PC_VSTREAM_CONTROL, 1);
	OUT_RING(ring, 0x00000000);

	emit_cache_flush(ring);
	fd_wfi(ctx, ring);

	ctx->needs_rb_fbd = true;
}
