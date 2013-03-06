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

#include "freedreno_gmem.h"
#include "freedreno_context.h"
#include "freedreno_state.h"
#include "freedreno_program.h"
#include "freedreno_resource.h"
#include "freedreno_zsa.h"
#include "freedreno_util.h"

/*
 * GMEM is the small (ie. 256KiB for a200, 512KiB for a220, etc) tile buffer
 * inside the GPU.  All rendering happens to GMEM.  Larger render targets
 * are split into tiles that are small enough for the color (and depth and/or
 * stencil, if enabled) buffers to fit within GMEM.  Before rendering a tile,
 * if there was not a clear invalidating the previous tile contents, we need
 * to restore the previous tiles contents (system mem -> GMEM), and after all
 * the draw calls, before moving to the next tile, we need to save the tile
 * contents (GMEM -> system mem).
 *
 * The code in this file handles dealing with GMEM and tiling.
 *
 * The structure of the ringbuffer ends up being:
 *
 *     +--<---<-- IB ---<---+---<---+---<---<---<--+
 *     |                    |       |              |
 *     v                    ^       ^              ^
 *   ------------------------------------------------------
 *     | clear/draw cmds | Tile0 | Tile1 | .... | TileN |
 *   ------------------------------------------------------
 *                       ^
 *                       |
 *                       address submitted in issueibcmds
 *
 * Where the per-tile section handles scissor setup, mem2gmem restore (if
 * needed), IB to draw cmds earlier in the ringbuffer, and then gmem2mem
 * resolve.
 */

/* transfer from gmem to system memory (ie. normal RAM) */

static void
emit_gmem2mem_surf(struct fd_ringbuffer *ring, uint32_t swap, uint32_t base,
		struct pipe_surface *psurf)
{
	struct fd_resource *rsc = fd_resource(psurf->texture);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COLOR_INFO));
	OUT_RING(ring, RB_COLOR_INFO_COLOR_SWAP(swap) |
			RB_COLOR_INFO_COLOR_BASE(base / 1024) |
			RB_COLOR_INFO_COLOR_FORMAT(fd_pipe2color(psurf->format)));

	OUT_PKT3(ring, CP_SET_CONSTANT, 5);
	OUT_RING(ring, CP_REG(REG_RB_COPY_CONTROL));
	OUT_RING(ring, 0x00000000);             /* RB_COPY_CONTROL */
	OUT_RELOC(ring, rsc->bo, 0, 0);         /* RB_COPY_DEST_BASE */
	OUT_RING(ring, rsc->pitch >> 5);        /* RB_COPY_DEST_PITCH */
	OUT_RING(ring, RB_COPY_DEST_INFO_FORMAT(fd_pipe2color(psurf->format)) |
			RB_COPY_DEST_INFO_LINEAR |      /* RB_COPY_DEST_INFO */
			RB_COPY_DEST_INFO_SWAP(swap) |
			RB_COPY_DEST_INFO_WRITE_RED |
			RB_COPY_DEST_INFO_WRITE_GREEN |
			RB_COPY_DEST_INFO_WRITE_BLUE |
			RB_COPY_DEST_INFO_WRITE_ALPHA);

	OUT_PKT3(ring, CP_WAIT_FOR_IDLE, 1);
	OUT_RING(ring, 0x0000000);

	OUT_PKT3(ring, CP_DRAW_INDX, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, DRAW(DI_PT_RECTLIST, DI_SRC_SEL_AUTO_INDEX,
			INDEX_SIZE_IGN, IGNORE_VISIBILITY));
	OUT_RING(ring, 3);					/* NumIndices */
}

static void
emit_gmem2mem(struct fd_context *ctx, struct fd_ringbuffer *ring,
		uint32_t xoff, uint32_t yoff, uint32_t bin_w, uint32_t bin_h)
{
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;

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

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SC_AA_MASK));
	OUT_RING(ring, 0x0000ffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_DEPTHCONTROL));
	OUT_RING(ring, RB_DEPTHCONTROL_EARLY_Z_ENABLE);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SU_SC_MODE_CNTL));
	OUT_RING(ring, PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST |  /* PA_SU_SC_MODE_CNTL */
			PA_SU_SC_MODE_CNTL_POLYMODE_FRONT_PTYPE(DRAW_TRIANGLES) |
			PA_SU_SC_MODE_CNTL_POLYMODE_BACK_PTYPE(DRAW_TRIANGLES));

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_PA_SC_WINDOW_SCISSOR_TL));
	OUT_RING(ring, xy2d(0, 0));                       /* PA_SC_WINDOW_SCISSOR_TL */
	OUT_RING(ring, xy2d(pfb->width, pfb->height));    /* PA_SC_WINDOW_SCISSOR_BR */

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_CL_VTE_CNTL));
	OUT_RING(ring, PA_CL_VTE_CNTL_VTX_W0_FMT |
			PA_CL_VTE_CNTL_VPORT_X_SCALE_ENA |
			PA_CL_VTE_CNTL_VPORT_X_OFFSET_ENA |
			PA_CL_VTE_CNTL_VPORT_Y_SCALE_ENA |
			PA_CL_VTE_CNTL_VPORT_Y_OFFSET_ENA);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_CL_CLIP_CNTL));
	OUT_RING(ring, 0x00000000);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_MODECONTROL));
	OUT_RING(ring, RB_MODECONTROL_EDRAM_MODE(EDRAM_COPY));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COPY_DEST_OFFSET));
	OUT_RING(ring, RB_COPY_DEST_OFFSET_X(xoff) | RB_COPY_DEST_OFFSET_Y(yoff));

	if (ctx->resolve & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL))
		emit_gmem2mem_surf(ring, 0, bin_w * bin_h, pfb->zsbuf);

	if (ctx->resolve & FD_BUFFER_COLOR)
		emit_gmem2mem_surf(ring, 1, 0, pfb->cbufs[0]);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_MODECONTROL));
	OUT_RING(ring, RB_MODECONTROL_EDRAM_MODE(COLOR_DEPTH));
}

/* transfer from system memory to gmem */

static void
emit_mem2gmem_surf(struct fd_ringbuffer *ring, uint32_t swap, uint32_t base,
		struct pipe_surface *psurf)
{
	struct fd_resource *rsc = fd_resource(psurf->texture);
	uint32_t swiz;

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COLOR_INFO));
	OUT_RING(ring, RB_COLOR_INFO_COLOR_SWAP(swap) |
			RB_COLOR_INFO_COLOR_BASE(base / 1024) |
			RB_COLOR_INFO_COLOR_FORMAT(fd_pipe2color(psurf->format)));

	swiz = fd_tex_swiz(psurf->format, PIPE_SWIZZLE_RED, PIPE_SWIZZLE_GREEN,
			PIPE_SWIZZLE_BLUE, PIPE_SWIZZLE_ALPHA);

	/* emit fb as a texture: */
	OUT_PKT3(ring, CP_SET_CONSTANT, 7);
	OUT_RING(ring, 0x00010000);
	OUT_RING(ring, SQ_TEX0_CLAMP_X(SQ_TEX_WRAP) |
			SQ_TEX0_CLAMP_Y(SQ_TEX_WRAP) |
			SQ_TEX0_CLAMP_Z(SQ_TEX_WRAP) |
			SQ_TEX0_PITCH(rsc->pitch));
	OUT_RELOC(ring, rsc->bo, 0,
			fd_pipe2surface(psurf->format) | 0x800);
	OUT_RING(ring, SQ_TEX2_WIDTH(psurf->width) |
			SQ_TEX2_HEIGHT(psurf->height));
	OUT_RING(ring, 0x01000000 | // XXX
			swiz |
			SQ_TEX3_XY_MAG_FILTER(SQ_TEX_FILTER_POINT) |
			SQ_TEX3_XY_MIN_FILTER(SQ_TEX_FILTER_POINT));
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, 0x00000200);

	OUT_PKT3(ring, CP_DRAW_INDX, 3);
	OUT_RING(ring, 0x00000000);
	OUT_RING(ring, DRAW(DI_PT_RECTLIST, DI_SRC_SEL_AUTO_INDEX,
			INDEX_SIZE_IGN, IGNORE_VISIBILITY));
	OUT_RING(ring, 3);					/* NumIndices */
}

static void
emit_mem2gmem(struct fd_context *ctx, struct fd_ringbuffer *ring,
		uint32_t xoff, uint32_t yoff, uint32_t bin_w, uint32_t bin_h)
{
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;
	float x0, y0, x1, y1;

	fd_emit_vertex_bufs(ring, 0x9c, (struct fd_vertex_buf[]) {
			{ .prsc = ctx->solid_vertexbuf, .size = 48, .offset = 0x30 },
			{ .prsc = ctx->solid_vertexbuf, .size = 32, .offset = 0x60 },
		}, 2);

	/* write texture coordinates to vertexbuf: */
	x0 = ((float)xoff) / ((float)pfb->width);
	x1 = ((float)xoff + bin_w) / ((float)pfb->width);
	y0 = ((float)yoff) / ((float)pfb->height);
	y1 = ((float)yoff + bin_h) / ((float)pfb->height);
	OUT_PKT3(ring, CP_MEM_WRITE, 9);
	OUT_RELOC(ring, fd_resource(ctx->solid_vertexbuf)->bo, 0x60, 0);
	OUT_RING(ring, f2d(x0));
	OUT_RING(ring, f2d(y0));
	OUT_RING(ring, f2d(x1));
	OUT_RING(ring, f2d(y0));
	OUT_RING(ring, f2d(x0));
	OUT_RING(ring, f2d(y1));
	OUT_RING(ring, f2d(x1));
	OUT_RING(ring, f2d(y1));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_INDX_OFFSET));
	OUT_RING(ring, 0);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_VGT_VERTEX_REUSE_BLOCK_CNTL));
	OUT_RING(ring, 0x0000003b);

	fd_program_emit(ring, &ctx->blit_prog);

	OUT_PKT0(ring, REG_TC_CNTL_STATUS, 1);
	OUT_RING(ring, TC_CNTL_STATUS_L2_INVALIDATE);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_DEPTHCONTROL));
	OUT_RING(ring, RB_DEPTHCONTROL_EARLY_Z_ENABLE);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SU_SC_MODE_CNTL));
	OUT_RING(ring, PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST |
			PA_SU_SC_MODE_CNTL_POLYMODE_FRONT_PTYPE(DRAW_TRIANGLES) |
			PA_SU_SC_MODE_CNTL_POLYMODE_BACK_PTYPE(DRAW_TRIANGLES));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_SC_AA_MASK));
	OUT_RING(ring, 0x0000ffff);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_COLORCONTROL));
	OUT_RING(ring, RB_COLORCONTROL_ALPHA_FUNC(PIPE_FUNC_ALWAYS) |
			RB_COLORCONTROL_BLEND_DISABLE |
			RB_COLORCONTROL_ROP_CODE(12) |
			RB_COLORCONTROL_DITHER_MODE(DITHER_DISABLE) |
			RB_COLORCONTROL_DITHER_TYPE(DITHER_PIXEL));

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_RB_BLEND_CONTROL));
	OUT_RING(ring, RB_BLENDCONTROL_COLOR_SRCBLEND(RB_BLEND_ONE) |
			RB_BLENDCONTROL_COLOR_COMB_FCN(COMB_DST_PLUS_SRC) |
			RB_BLENDCONTROL_COLOR_DESTBLEND(RB_BLEND_ZERO) |
			RB_BLENDCONTROL_ALPHA_SRCBLEND(RB_BLEND_ONE) |
			RB_BLENDCONTROL_ALPHA_COMB_FCN(COMB_DST_PLUS_SRC) |
			RB_BLENDCONTROL_ALPHA_DESTBLEND(RB_BLEND_ZERO));

	OUT_PKT3(ring, CP_SET_CONSTANT, 3);
	OUT_RING(ring, CP_REG(REG_PA_SC_WINDOW_SCISSOR_TL));
	OUT_RING(ring, PA_SC_WINDOW_OFFSET_DISABLE |
			xy2d(0,0));                     /* PA_SC_WINDOW_SCISSOR_TL */
	OUT_RING(ring, xy2d(bin_w, bin_h));     /* PA_SC_WINDOW_SCISSOR_BR */

	OUT_PKT3(ring, CP_SET_CONSTANT, 5);
	OUT_RING(ring, CP_REG(REG_PA_CL_VPORT_XSCALE));
	OUT_RING(ring, f2d((float)bin_w/2.0));  /* PA_CL_VPORT_XSCALE */
	OUT_RING(ring, f2d((float)bin_w/2.0));  /* PA_CL_VPORT_XOFFSET */
	OUT_RING(ring, f2d(-(float)bin_h/2.0)); /* PA_CL_VPORT_YSCALE */
	OUT_RING(ring, f2d((float)bin_h/2.0));  /* PA_CL_VPORT_YOFFSET */

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_CL_VTE_CNTL));
	OUT_RING(ring, PA_CL_VTE_CNTL_VTX_XY_FMT |
			PA_CL_VTE_CNTL_VTX_Z_FMT |       // XXX check this???
			PA_CL_VTE_CNTL_VPORT_X_SCALE_ENA |
			PA_CL_VTE_CNTL_VPORT_X_OFFSET_ENA |
			PA_CL_VTE_CNTL_VPORT_Y_SCALE_ENA |
			PA_CL_VTE_CNTL_VPORT_Y_OFFSET_ENA);

	OUT_PKT3(ring, CP_SET_CONSTANT, 2);
	OUT_RING(ring, CP_REG(REG_PA_CL_CLIP_CNTL));
	OUT_RING(ring, 0x00000000);

	if (ctx->restore & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL))
		emit_mem2gmem_surf(ring, 0, bin_w * bin_h, pfb->zsbuf);

	if (ctx->restore & FD_BUFFER_COLOR)
		emit_mem2gmem_surf(ring, 1, 0, pfb->cbufs[0]);

	/* TODO blob driver seems to toss in a CACHE_FLUSH after each DRAW_INDX.. */
}

static void
calculate_tiles(struct fd_context *ctx)
{
	struct fd_gmem_stateobj *gmem = &ctx->gmem;
	struct pipe_scissor_state *scissor = &ctx->max_scissor;
	uint32_t cpp = util_format_get_blocksize(ctx->framebuffer.cbufs[0]->format);
	uint32_t gmem_size = ctx->screen->gmemsize_bytes;
	uint32_t minx, miny, width, height;
	uint32_t nbins_x = 1, nbins_y = 1;
	uint32_t bin_w, bin_h;
	uint32_t max_width = 992;

	if ((gmem->cpp == cpp) &&
			!memcmp(&gmem->scissor, scissor, sizeof(gmem->scissor))) {
		/* everything is up-to-date */
		return;
	}

	minx = scissor->minx & ~31; /* round down to multiple of 32 */
	miny = scissor->miny & ~31;
	width = scissor->maxx - minx;
	height = scissor->maxy - miny;

// TODO we probably could optimize this a bit if we know that
// Z or stencil is not enabled for any of the draw calls..
//	if (fd_stencil_enabled(ctx->zsa) || fd_depth_enabled(ctx->zsa)) {
		gmem_size /= 2;
		max_width = 256;
//	}

	bin_w = ALIGN(width, 32);
	bin_h = ALIGN(height, 32);

	/* first, find a bin width that satisfies the maximum width
	 * restrictions:
	 */
	while (bin_w > max_width) {
		nbins_x++;
		bin_w = ALIGN(width / nbins_x, 32);
	}

	/* then find a bin height that satisfies the memory constraints:
	 */
	while ((bin_w * bin_h * cpp) > gmem_size) {
		nbins_y++;
		bin_h = ALIGN(height / nbins_y, 32);
	}

	DBG("using %d bins of size %dx%d", nbins_x*nbins_y, bin_w, bin_h);

	gmem->scissor = *scissor;
	gmem->cpp = cpp;
	gmem->minx = minx;
	gmem->miny = miny;
	gmem->bin_h = bin_h;
	gmem->bin_w = bin_w;
	gmem->nbins_x = nbins_x;
	gmem->nbins_y = nbins_y;
	gmem->width = width;
	gmem->height = height;
}

void
fd_gmem_render_tiles(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;
	struct fd_gmem_stateobj *gmem = &ctx->gmem;
	struct fd_ringbuffer *ring = ctx->ring;
	enum rb_colorformatx colorformatx = fd_pipe2color(pfb->cbufs[0]->format);
	uint32_t i, timestamp, yoff = 0;
	uint32_t base, reg;

	calculate_tiles(ctx);

	/* this should be true because bin_w/bin_h should be multiples of 32: */
	assert(((gmem->bin_w * gmem->bin_h) % 1024) == 0);

	/* depth/stencil starts after color buffer in GMEM: */
	base = (gmem->bin_w * gmem->bin_h) / 1024;

	DBG("rendering %dx%d tiles (%s/%s)", gmem->nbins_x, gmem->nbins_y,
			util_format_name(pfb->cbufs[0]->format),
			pfb->zsbuf ? util_format_name(pfb->zsbuf->format) : "none");

	/* mark the end of the clear/draw cmds before emitting per-tile cmds: */
	fd_ringmarker_mark(ctx->draw_end);

	/* RB_SURFACE_INFO / RB_DEPTH_INFO can be emitted once per tile pass,
	 * but RB_COLOR_INFO gets overwritten by gmem2mem and mem2gmem and so
	 * needs to be emitted for each tile:
	 */
	OUT_PKT3(ring, CP_SET_CONSTANT, 4);
	OUT_RING(ring, CP_REG(REG_RB_SURFACE_INFO));
	OUT_RING(ring, gmem->bin_w);                 /* RB_SURFACE_INFO */
	OUT_RING(ring, RB_COLOR_INFO_COLOR_SWAP(1) | /* RB_COLOR_INFO */
			RB_COLOR_INFO_COLOR_FORMAT(colorformatx));
	reg = RB_DEPTH_INFO_DEPTH_BASE(ALIGN(base, 4));
	if (pfb->zsbuf)
		reg |= RB_DEPTH_INFO_DEPTH_FORMAT(fd_pipe2depth(pfb->zsbuf->format));
	OUT_RING(ring, reg);                         /* RB_DEPTH_INFO */

	yoff= gmem->miny;
	for (i = 0; i < gmem->nbins_y; i++) {
		uint32_t j, xoff = gmem->minx;
		uint32_t bh = gmem->bin_h;

		/* clip bin height: */
		bh = min(bh, gmem->height - yoff);

		for (j = 0; j < gmem->nbins_x; j++) {
			uint32_t bw = gmem->bin_w;

			/* clip bin width: */
			bw = min(bw, gmem->width - xoff);

			DBG("bin_h=%d, yoff=%d, bin_w=%d, xoff=%d",
					bh, yoff, bw, xoff);

			if ((i == 0) && (j == 0)) {
				uint32_t reg;


			} else {

			}

			/* setup screen scissor for current tile (same for mem2gmem): */
			OUT_PKT3(ring, CP_SET_CONSTANT, 3);
			OUT_RING(ring, CP_REG(REG_PA_SC_SCREEN_SCISSOR_TL));
			OUT_RING(ring, xy2d(0,0));           /* PA_SC_SCREEN_SCISSOR_TL */
			OUT_RING(ring, xy2d(bw, bh));        /* PA_SC_SCREEN_SCISSOR_BR */

			if (ctx->restore)
				emit_mem2gmem(ctx, ring, xoff, yoff, bw, bh);

			OUT_PKT3(ring, CP_SET_CONSTANT, 2);
			OUT_RING(ring, CP_REG(REG_RB_COLOR_INFO));
			OUT_RING(ring, RB_COLOR_INFO_COLOR_SWAP(1) | /* RB_COLOR_INFO */
					RB_COLOR_INFO_COLOR_FORMAT(colorformatx));

			/* setup window scissor and offset for current tile (different
			 * from mem2gmem):
			 */
			OUT_PKT3(ring, CP_SET_CONSTANT, 2);
			OUT_RING(ring, CP_REG(REG_PA_SC_WINDOW_OFFSET));
			OUT_RING(ring, PA_SC_WINDOW_OFFSET_X(-xoff) |
					PA_SC_WINDOW_OFFSET_Y(-yoff));/* PA_SC_WINDOW_OFFSET */

			/* emit IB to drawcmds: */
			OUT_IB  (ring, ctx->draw_start, ctx->draw_end);

			OUT_PKT3(ring, CP_SET_CONSTANT, 2);
			OUT_RING(ring, CP_REG(REG_PA_SC_WINDOW_OFFSET));
			OUT_RING(ring, 0x00000000);          /* PA_SC_WINDOW_OFFSET */

			/* emit gmem2mem to transfer tile back to system memory: */
			emit_gmem2mem(ctx, ring, xoff, yoff, bw, bh);

			xoff += bw;
		}

		yoff += bh;
	}

	/* GPU executes starting from tile cmds, which IB back to draw cmds: */
	fd_ringmarker_flush(ctx->draw_end);

	/* mark start for next draw cmds: */
	fd_ringmarker_mark(ctx->draw_start);

	/* update timestamps on render targets: */
	fd_pipe_timestamp(ctx->screen->pipe, &timestamp);
	fd_resource(pfb->cbufs[0]->texture)->timestamp = timestamp;
	if (pfb->zsbuf)
		fd_resource(pfb->zsbuf->texture)->timestamp = timestamp;

	/* reset maximal bounds: */
	ctx->max_scissor.minx = ctx->max_scissor.miny = ~0;
	ctx->max_scissor.maxx = ctx->max_scissor.maxy = 0;

	/* Note that because the per-tile setup and mem2gmem/gmem2mem are emitted
	 * after the draw/clear calls, but executed before, we need to preemptively
	 * flag some state as dirty before the first draw/clear call.
	 *
	 * TODO maybe we need to mark all state as dirty to not worry about state
	 * being clobbered by other contexts?
	 */
	ctx->dirty |= FD_DIRTY_ZSA |
			FD_DIRTY_RASTERIZER |
			FD_DIRTY_FRAMEBUFFER |
			FD_DIRTY_SAMPLE_MASK |
			FD_DIRTY_VIEWPORT |
			FD_DIRTY_CONSTBUF |
			FD_DIRTY_PROG |
			FD_DIRTY_SCISSOR |
			/* probably only needed if we need to mem2gmem on the next
			 * draw..  but not sure if there is a good way to know?
			 */
			FD_DIRTY_VERTTEX |
			FD_DIRTY_FRAGTEX |
			FD_DIRTY_BLEND;
}
