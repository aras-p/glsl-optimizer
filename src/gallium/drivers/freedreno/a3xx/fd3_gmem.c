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
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "freedreno_state.h"
#include "freedreno_resource.h"

#include "fd3_gmem.h"
#include "fd3_context.h"
#include "fd3_emit.h"
#include "fd3_program.h"
#include "fd3_util.h"
#include "fd3_zsa.h"


static void
emit_mrt(struct fd_ringbuffer *ring, unsigned nr_bufs,
		struct pipe_surface **bufs, uint32_t *bases, uint32_t bin_w)
{
	enum a3xx_tile_mode tile_mode;
	unsigned i;

	if (bin_w) {
		tile_mode = TILE_32X32;
	} else {
		tile_mode = LINEAR;
	}

	for (i = 0; i < 4; i++) {
		enum a3xx_color_fmt format = 0;
		enum a3xx_color_swap swap = WZYX;
		struct fd_resource *res = NULL;
		uint32_t stride = 0;
		uint32_t base = 0;

		if (i < nr_bufs) {
			struct pipe_surface *psurf = bufs[i];

			res = fd_resource(psurf->texture);
			format = fd3_pipe2color(psurf->format);
			swap = fd3_pipe2swap(psurf->format);

			if (bin_w) {
				stride = bin_w * res->cpp;

				if (bases) {
					base = bases[i] * res->cpp;
				}
			} else {
				stride = res->pitch * res->cpp;
			}
		}

		OUT_PKT0(ring, REG_A3XX_RB_MRT_BUF_INFO(i), 2);
		OUT_RING(ring, A3XX_RB_MRT_BUF_INFO_COLOR_FORMAT(format) |
				A3XX_RB_MRT_BUF_INFO_COLOR_TILE_MODE(tile_mode) |
				A3XX_RB_MRT_BUF_INFO_COLOR_BUF_PITCH(stride) |
				A3XX_RB_MRT_BUF_INFO_COLOR_SWAP(swap));
		if (bin_w || (i >= nr_bufs)) {
			OUT_RING(ring, A3XX_RB_MRT_BUF_BASE_COLOR_BUF_BASE(base));
		} else {
			OUT_RELOCS(ring, res->bo, 0, 0, -1);
		}

		OUT_PKT0(ring, REG_A3XX_SP_FS_IMAGE_OUTPUT_REG(i), 1);
		OUT_RING(ring, A3XX_SP_FS_IMAGE_OUTPUT_REG_MRTFORMAT(format));
	}
}

static uint32_t
depth_base(struct fd_gmem_stateobj *gmem)
{
	return align(gmem->bin_w * gmem->bin_h, 0x4000);
}

/* transfer from gmem to system memory (ie. normal RAM) */

static void
emit_gmem2mem_surf(struct fd_ringbuffer *ring,
		enum adreno_rb_copy_control_mode mode,
		uint32_t base, struct pipe_surface *psurf)
{
	struct fd_resource *rsc = fd_resource(psurf->texture);

	OUT_PKT0(ring, REG_A3XX_RB_COPY_CONTROL, 4);
	OUT_RING(ring, A3XX_RB_COPY_CONTROL_MSAA_RESOLVE(MSAA_ONE) |
			A3XX_RB_COPY_CONTROL_MODE(mode) |
			A3XX_RB_COPY_CONTROL_GMEM_BASE(base));
	OUT_RELOCS(ring, rsc->bo, 0, 0, -1);    /* RB_COPY_DEST_BASE */
	OUT_RING(ring, A3XX_RB_COPY_DEST_PITCH_PITCH(rsc->pitch * rsc->cpp));
	OUT_RING(ring, A3XX_RB_COPY_DEST_INFO_TILE(LINEAR) |
			A3XX_RB_COPY_DEST_INFO_FORMAT(fd3_pipe2color(psurf->format)) |
			A3XX_RB_COPY_DEST_INFO_COMPONENT_ENABLE(0xf) |
			A3XX_RB_COPY_DEST_INFO_ENDIAN(ENDIAN_NONE) |
			A3XX_RB_COPY_DEST_INFO_SWAP(fd3_pipe2swap(psurf->format)));

	OUT_PKT3(ring, CP_DRAW_INDX, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, DRAW(DI_PT_RECTLIST, DI_SRC_SEL_AUTO_INDEX,
			INDEX_SIZE_IGN, IGNORE_VISIBILITY));
	OUT_RING(ring, 2);					/* NumIndices */
}

static void
fd3_emit_tile_gmem2mem(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
		uint32_t bin_w, uint32_t bin_h)
{
	struct fd3_context *fd3_ctx = fd3_context(ctx);
	struct fd_ringbuffer *ring = ctx->ring;
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;

	OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
	OUT_RING(ring, A3XX_RB_DEPTH_CONTROL_ZFUNC(FUNC_NEVER));

	OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
	OUT_RING(ring, A3XX_RB_STENCIL_CONTROL_FUNC(FUNC_NEVER) |
			A3XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_FUNC_BF(FUNC_NEVER) |
			A3XX_RB_STENCIL_CONTROL_FAIL_BF(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZPASS_BF(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZFAIL_BF(STENCIL_KEEP));

	OUT_PKT0(ring, REG_A3XX_RB_STENCILREFMASK, 2);
	OUT_RING(ring, 0xff000000 |
			A3XX_RB_STENCILREFMASK_STENCILREF(0) |
			A3XX_RB_STENCILREFMASK_STENCILMASK(0) |
			A3XX_RB_STENCILREFMASK_STENCILWRITEMASK(0xff));
	OUT_RING(ring, 0xff000000 |
			A3XX_RB_STENCILREFMASK_STENCILREF(0) |
			A3XX_RB_STENCILREFMASK_STENCILMASK(0) |
			A3XX_RB_STENCILREFMASK_STENCILWRITEMASK(0xff));

	OUT_PKT0(ring, REG_A3XX_GRAS_SU_MODE_CONTROL, 1);
	OUT_RING(ring, A3XX_GRAS_SU_MODE_CONTROL_LINEHALFWIDTH(0));

	OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
	OUT_RING(ring, 0x00000000);   /* GRAS_CL_CLIP_CNTL */

	OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
	OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RESOLVE_PASS) |
			A3XX_RB_MODE_CONTROL_MARB_CACHE_SPLIT_MODE);

	fd3_emit_rbrc_draw_state(ring,
			A3XX_RB_RENDER_CONTROL_DISABLE_COLOR_PIPE |
			A3XX_RB_RENDER_CONTROL_ALPHA_TEST_FUNC(FUNC_NEVER));

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
	OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RESOLVE_PASS) |
			A3XX_GRAS_SC_CONTROL_MSAA_SAMPLES(MSAA_ONE) |
			A3XX_GRAS_SC_CONTROL_RASTER_MODE(1));

	OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
	OUT_RING(ring, A3XX_PC_PRIM_VTX_CNTL_STRIDE_IN_VPC(0) |
			A3XX_PC_PRIM_VTX_CNTL_POLYMODE_FRONT_PTYPE(PC_DRAW_TRIANGLES) |
			A3XX_PC_PRIM_VTX_CNTL_POLYMODE_BACK_PTYPE(PC_DRAW_TRIANGLES) |
			A3XX_PC_PRIM_VTX_CNTL_PROVOKING_VTX_LAST);

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
	OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
			A3XX_GRAS_SC_WINDOW_SCISSOR_TL_Y(0));
	OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_BR_X(pfb->width - 1) |
			A3XX_GRAS_SC_WINDOW_SCISSOR_BR_Y(pfb->height - 1));

	OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
	OUT_RING(ring, 0);            /* VFD_INDEX_MIN */
	OUT_RING(ring, 2);            /* VFD_INDEX_MAX */
	OUT_RING(ring, 0);            /* VFD_INSTANCEID_OFFSET */
	OUT_RING(ring, 0);            /* VFD_INDEX_OFFSET */

	fd3_program_emit(ring, &ctx->solid_prog);

	fd3_emit_vertex_bufs(ring, &ctx->solid_prog, (struct fd3_vertex_buf[]) {
			{ .prsc = fd3_ctx->solid_vbuf, .stride = 12, .format = PIPE_FORMAT_R32G32B32_FLOAT },
		}, 1);

	if (ctx->resolve & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL)) {
		uint32_t base = depth_base(&ctx->gmem) *
				fd_resource(pfb->cbufs[0]->texture)->cpp;
		emit_gmem2mem_surf(ring, RB_COPY_DEPTH_STENCIL, base, pfb->zsbuf);
	}

	if (ctx->resolve & FD_BUFFER_COLOR) {
		emit_gmem2mem_surf(ring, RB_COPY_RESOLVE, 0, pfb->cbufs[0]);
	}

	OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
	OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
			A3XX_RB_MODE_CONTROL_MARB_CACHE_SPLIT_MODE);

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
	OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
			A3XX_GRAS_SC_CONTROL_MSAA_SAMPLES(MSAA_ONE) |
			A3XX_GRAS_SC_CONTROL_RASTER_MODE(0));
}

/* transfer from system memory to gmem */

static void
emit_mem2gmem_surf(struct fd_ringbuffer *ring, uint32_t base,
		struct pipe_surface *psurf, uint32_t bin_w)
{
	emit_mrt(ring, 1, &psurf, &base, bin_w);

	fd3_emit_gmem_restore_tex(ring, psurf);

	OUT_PKT3(ring, CP_DRAW_INDX, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, DRAW(DI_PT_RECTLIST, DI_SRC_SEL_AUTO_INDEX,
			INDEX_SIZE_IGN, IGNORE_VISIBILITY));
	OUT_RING(ring, 2);					/* NumIndices */
}

static void
fd3_emit_tile_mem2gmem(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
		uint32_t bin_w, uint32_t bin_h)
{
	struct fd3_context *fd3_ctx = fd3_context(ctx);
	struct fd_gmem_stateobj *gmem = &ctx->gmem;
	struct fd_ringbuffer *ring = ctx->ring;
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;
	float x0, y0, x1, y1;
	unsigned i;

	/* write texture coordinates to vertexbuf: */
	x0 = ((float)xoff) / ((float)pfb->width);
	x1 = ((float)xoff + bin_w) / ((float)pfb->width);
	y0 = ((float)yoff) / ((float)pfb->height);
	y1 = ((float)yoff + bin_h) / ((float)pfb->height);

	OUT_PKT3(ring, CP_MEM_WRITE, 5);
	OUT_RELOC(ring, fd_resource(fd3_ctx->blit_texcoord_vbuf)->bo, 0, 0);
	OUT_RING(ring, fui(x0));
	OUT_RING(ring, fui(y0));
	OUT_RING(ring, fui(x1));
	OUT_RING(ring, fui(y1));

	for (i = 0; i < 4; i++) {
		OUT_PKT0(ring, REG_A3XX_RB_MRT_CONTROL(i), 1);
		OUT_RING(ring, A3XX_RB_MRT_CONTROL_ROP_CODE(12) |
				A3XX_RB_MRT_CONTROL_DITHER_MODE(DITHER_DISABLE) |
				A3XX_RB_MRT_CONTROL_COMPONENT_ENABLE(0xf));

		OUT_PKT0(ring, REG_A3XX_RB_MRT_BLEND_CONTROL(i), 1);
		OUT_RING(ring, A3XX_RB_MRT_BLEND_CONTROL_RGB_SRC_FACTOR(FACTOR_ONE) |
				A3XX_RB_MRT_BLEND_CONTROL_RGB_BLEND_OPCODE(BLEND_DST_PLUS_SRC) |
				A3XX_RB_MRT_BLEND_CONTROL_RGB_DEST_FACTOR(FACTOR_ZERO) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_SRC_FACTOR(FACTOR_ONE) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_BLEND_OPCODE(BLEND_DST_PLUS_SRC) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_DEST_FACTOR(FACTOR_ZERO) |
				A3XX_RB_MRT_BLEND_CONTROL_CLAMP_ENABLE);
	}

	fd3_emit_rbrc_tile_state(ring,
			A3XX_RB_RENDER_CONTROL_BIN_WIDTH(gmem->bin_w));

	OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
	OUT_RING(ring, A3XX_RB_DEPTH_CONTROL_ZFUNC(FUNC_LESS));

	OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
	OUT_RING(ring, A3XX_GRAS_CL_CLIP_CNTL_IJ_PERSP_CENTER);   /* GRAS_CL_CLIP_CNTL */

	OUT_PKT0(ring, REG_A3XX_GRAS_CL_VPORT_XOFFSET, 6);
	OUT_RING(ring, A3XX_GRAS_CL_VPORT_XOFFSET((float)bin_w/2.0 - 0.5));
	OUT_RING(ring, A3XX_GRAS_CL_VPORT_XSCALE((float)bin_w/2.0));
	OUT_RING(ring, A3XX_GRAS_CL_VPORT_YOFFSET((float)bin_h/2.0 - 0.5));
	OUT_RING(ring, A3XX_GRAS_CL_VPORT_YSCALE(-(float)bin_h/2.0));
	OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZOFFSET(0.0));
	OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZSCALE(1.0));

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
	OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
			A3XX_GRAS_SC_WINDOW_SCISSOR_TL_Y(0));
	OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_BR_X(bin_w - 1) |
			A3XX_GRAS_SC_WINDOW_SCISSOR_BR_Y(bin_h - 1));

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
	OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(0) |
			A3XX_GRAS_SC_SCREEN_SCISSOR_TL_Y(0));
	OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(bin_w - 1) |
			A3XX_GRAS_SC_SCREEN_SCISSOR_BR_Y(bin_h - 1));

	OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
	OUT_RING(ring, 0x2 |
			A3XX_RB_STENCIL_CONTROL_FUNC(FUNC_ALWAYS) |
			A3XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_FUNC_BF(FUNC_ALWAYS) |
			A3XX_RB_STENCIL_CONTROL_FAIL_BF(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZPASS_BF(STENCIL_KEEP) |
			A3XX_RB_STENCIL_CONTROL_ZFAIL_BF(STENCIL_KEEP));

	fd3_emit_rbrc_draw_state(ring,
			A3XX_RB_RENDER_CONTROL_ALPHA_TEST_FUNC(FUNC_ALWAYS));

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
	OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
			A3XX_GRAS_SC_CONTROL_MSAA_SAMPLES(MSAA_ONE) |
			A3XX_GRAS_SC_CONTROL_RASTER_MODE(1));

	OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
	OUT_RING(ring, A3XX_PC_PRIM_VTX_CNTL_STRIDE_IN_VPC(2) |
			A3XX_PC_PRIM_VTX_CNTL_POLYMODE_FRONT_PTYPE(PC_DRAW_TRIANGLES) |
			A3XX_PC_PRIM_VTX_CNTL_POLYMODE_BACK_PTYPE(PC_DRAW_TRIANGLES) |
			A3XX_PC_PRIM_VTX_CNTL_PROVOKING_VTX_LAST);

	OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
	OUT_RING(ring, 0);            /* VFD_INDEX_MIN */
	OUT_RING(ring, 2);            /* VFD_INDEX_MAX */
	OUT_RING(ring, 0);            /* VFD_INSTANCEID_OFFSET */
	OUT_RING(ring, 0);            /* VFD_INDEX_OFFSET */

	fd3_program_emit(ring, &ctx->blit_prog);

	fd3_emit_vertex_bufs(ring, &ctx->blit_prog, (struct fd3_vertex_buf[]) {
			{ .prsc = fd3_ctx->blit_texcoord_vbuf, .stride = 8, .format = PIPE_FORMAT_R32G32_FLOAT },
			{ .prsc = fd3_ctx->solid_vbuf, .stride = 12, .format = PIPE_FORMAT_R32G32B32_FLOAT },
		}, 2);

	/* for gmem pitch/base calculations, we need to use the non-
	 * truncated tile sizes:
	 */
	bin_w = gmem->bin_w;
	bin_h = gmem->bin_h;

	if (ctx->restore & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL))
		emit_mem2gmem_surf(ring, depth_base(gmem), pfb->zsbuf, bin_w);

	if (ctx->restore & FD_BUFFER_COLOR)
		emit_mem2gmem_surf(ring, 0, pfb->cbufs[0], bin_w);

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
	OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
			A3XX_GRAS_SC_CONTROL_MSAA_SAMPLES(MSAA_ONE) |
			A3XX_GRAS_SC_CONTROL_RASTER_MODE(0));
}

static void
update_vsc_pipe(struct fd_context *ctx)
{
	struct fd_ringbuffer *ring = ctx->ring;
	struct fd_gmem_stateobj *gmem = &ctx->gmem;
	struct fd_bo *bo = fd3_context(ctx)->vsc_pipe_mem;
	int i;

	/* since we aren't using binning, just try to assign all bins
	 * to same pipe for now:
	 */
	OUT_PKT0(ring, REG_A3XX_VSC_PIPE(0), 3);
	OUT_RING(ring, A3XX_VSC_PIPE_CONFIG_X(0) |
			A3XX_VSC_PIPE_CONFIG_Y(0) |
			A3XX_VSC_PIPE_CONFIG_W(gmem->nbins_x) |
			A3XX_VSC_PIPE_CONFIG_H(gmem->nbins_y));
	OUT_RELOC(ring, bo, 0, 0);              /* VSC_PIPE[0].DATA_ADDRESS */
	OUT_RING(ring, fd_bo_size(bo) - 32);    /* VSC_PIPE[0].DATA_LENGTH */

	for (i = 1; i < 8; i++) {
		OUT_PKT0(ring, REG_A3XX_VSC_PIPE(i), 3);
		OUT_RING(ring, A3XX_VSC_PIPE_CONFIG_X(0) |
				A3XX_VSC_PIPE_CONFIG_Y(0) |
				A3XX_VSC_PIPE_CONFIG_W(0) |
				A3XX_VSC_PIPE_CONFIG_H(0));
		OUT_RING(ring, 0x00000000);         /* VSC_PIPE[i].DATA_ADDRESS */
		OUT_RING(ring, 0x00000000);         /* VSC_PIPE[i].DATA_LENGTH */
	}
}

/* for rendering directly to system memory: */
static void
fd3_emit_sysmem_prep(struct fd_context *ctx)
{
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;
	struct fd_resource *rsc = fd_resource(pfb->cbufs[0]->texture);
	struct fd_ringbuffer *ring = ctx->ring;

	fd3_emit_restore(ctx);

	OUT_PKT0(ring, REG_A3XX_RB_WINDOW_SIZE, 1);
	OUT_RING(ring, A3XX_RB_WINDOW_SIZE_WIDTH(pfb->width) |
			A3XX_RB_WINDOW_SIZE_HEIGHT(pfb->height));

	emit_mrt(ring, pfb->nr_cbufs, pfb->cbufs, NULL, 0);

	fd3_emit_rbrc_tile_state(ring,
			A3XX_RB_RENDER_CONTROL_BIN_WIDTH(rsc->pitch));

	/* setup scissor/offset for current tile: */
	OUT_PKT0(ring, REG_A3XX_PA_SC_WINDOW_OFFSET, 1);
	OUT_RING(ring, A3XX_PA_SC_WINDOW_OFFSET_X(0) |
			A3XX_PA_SC_WINDOW_OFFSET_Y(0));

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
	OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(0) |
			A3XX_GRAS_SC_SCREEN_SCISSOR_TL_Y(0));
	OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(pfb->width - 1) |
			A3XX_GRAS_SC_SCREEN_SCISSOR_BR_Y(pfb->height - 1));

	OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
	OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
			A3XX_RB_MODE_CONTROL_GMEM_BYPASS |
			A3XX_RB_MODE_CONTROL_MARB_CACHE_SPLIT_MODE);
}

/* before first tile */
static void
fd3_emit_tile_init(struct fd_context *ctx)
{
	struct fd_ringbuffer *ring = ctx->ring;
	struct fd_gmem_stateobj *gmem = &ctx->gmem;

	fd3_emit_restore(ctx);

	/* note: use gmem->bin_w/h, the bin_w/h parameters may be truncated
	 * at the right and bottom edge tiles
	 */
	OUT_PKT0(ring, REG_A3XX_VSC_BIN_SIZE, 1);
	OUT_RING(ring, A3XX_VSC_BIN_SIZE_WIDTH(gmem->bin_w) |
			A3XX_VSC_BIN_SIZE_HEIGHT(gmem->bin_h));

	/* TODO we only need to do this if gmem stateobj changes.. or in
	 * particular if the # of bins changes..
	 */
	update_vsc_pipe(ctx);
}

/* before mem2gmem */
static void
fd3_emit_tile_prep(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
		uint32_t bin_w, uint32_t bin_h)
{
	struct fd_ringbuffer *ring = ctx->ring;
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;
	struct fd_gmem_stateobj *gmem = &ctx->gmem;
	uint32_t reg;


	OUT_PKT0(ring, REG_A3XX_RB_DEPTH_INFO, 2);
	reg = A3XX_RB_DEPTH_INFO_DEPTH_BASE(depth_base(gmem));
	if (pfb->zsbuf) {
		reg |= A3XX_RB_DEPTH_INFO_DEPTH_FORMAT(fd_pipe2depth(pfb->zsbuf->format));
	}
	OUT_RING(ring, reg);
	if (pfb->zsbuf) {
		uint32_t cpp = util_format_get_blocksize(pfb->zsbuf->format);
		OUT_RING(ring, A3XX_RB_DEPTH_PITCH(cpp * gmem->bin_w));
	} else {
		OUT_RING(ring, 0x00000000);
	}

	OUT_PKT0(ring, REG_A3XX_RB_WINDOW_SIZE, 1);
	OUT_RING(ring, A3XX_RB_WINDOW_SIZE_WIDTH(pfb->width) |
			A3XX_RB_WINDOW_SIZE_HEIGHT(pfb->height));

	OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
	OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
			A3XX_RB_MODE_CONTROL_MARB_CACHE_SPLIT_MODE);
}

/* before IB to rendering cmds: */
static void
fd3_emit_tile_renderprep(struct fd_context *ctx, uint32_t xoff, uint32_t yoff,
		uint32_t bin_w, uint32_t bin_h)
{
	struct fd_ringbuffer *ring = ctx->ring;
	struct fd_gmem_stateobj *gmem = &ctx->gmem;
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;

	uint32_t x1 = xoff;
	uint32_t y1 = yoff;
	uint32_t x2 = xoff + bin_w - 1;
	uint32_t y2 = yoff + bin_h - 1;

	OUT_PKT3(ring, CP_SET_BIN, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, CP_SET_BIN_1_X1(x1) | CP_SET_BIN_1_Y1(y1));
	OUT_RING(ring, CP_SET_BIN_2_X2(x2) | CP_SET_BIN_2_Y2(y2));

	emit_mrt(ring, pfb->nr_cbufs, pfb->cbufs, NULL, gmem->bin_w);

	fd3_emit_rbrc_tile_state(ring,
			A3XX_RB_RENDER_CONTROL_ENABLE_GMEM |
			A3XX_RB_RENDER_CONTROL_BIN_WIDTH(gmem->bin_w));

	/* setup scissor/offset for current tile: */
	OUT_PKT0(ring, REG_A3XX_PA_SC_WINDOW_OFFSET, 1);
	OUT_RING(ring, A3XX_PA_SC_WINDOW_OFFSET_X(xoff) |
			A3XX_PA_SC_WINDOW_OFFSET_Y(yoff));

	OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
	OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(x1) |
			A3XX_GRAS_SC_SCREEN_SCISSOR_TL_Y(y1));
	OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(x2) |
			A3XX_GRAS_SC_SCREEN_SCISSOR_BR_Y(y2));
}

void
fd3_gmem_init(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	ctx->emit_sysmem_prep = fd3_emit_sysmem_prep;
	ctx->emit_tile_init = fd3_emit_tile_init;
	ctx->emit_tile_prep = fd3_emit_tile_prep;
	ctx->emit_tile_mem2gmem = fd3_emit_tile_mem2gmem;
	ctx->emit_tile_renderprep = fd3_emit_tile_renderprep;
	ctx->emit_tile_gmem2mem = fd3_emit_tile_gmem2mem;
}
