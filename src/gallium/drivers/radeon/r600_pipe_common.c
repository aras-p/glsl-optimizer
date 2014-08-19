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

#include "r600_pipe_common.h"
#include "r600_cs.h"
#include "tgsi/tgsi_parse.h"
#include "util/u_draw_quad.h"
#include "util/u_memory.h"
#include "util/u_format_s3tc.h"
#include "util/u_upload_mgr.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"
#include "radeon/radeon_video.h"
#include <inttypes.h>

/*
 * pipe_context
 */

void r600_draw_rectangle(struct blitter_context *blitter,
			 int x1, int y1, int x2, int y2, float depth,
			 enum blitter_attrib_type type,
			 const union pipe_color_union *attrib)
{
	struct r600_common_context *rctx =
		(struct r600_common_context*)util_blitter_get_pipe(blitter);
	struct pipe_viewport_state viewport;
	struct pipe_resource *buf = NULL;
	unsigned offset = 0;
	float *vb;

	if (type == UTIL_BLITTER_ATTRIB_TEXCOORD) {
		util_blitter_draw_rectangle(blitter, x1, y1, x2, y2, depth, type, attrib);
		return;
	}

	/* Some operations (like color resolve on r6xx) don't work
	 * with the conventional primitive types.
	 * One that works is PT_RECTLIST, which we use here. */

	/* setup viewport */
	viewport.scale[0] = 1.0f;
	viewport.scale[1] = 1.0f;
	viewport.scale[2] = 1.0f;
	viewport.scale[3] = 1.0f;
	viewport.translate[0] = 0.0f;
	viewport.translate[1] = 0.0f;
	viewport.translate[2] = 0.0f;
	viewport.translate[3] = 0.0f;
	rctx->b.set_viewport_states(&rctx->b, 0, 1, &viewport);

	/* Upload vertices. The hw rectangle has only 3 vertices,
	 * I guess the 4th one is derived from the first 3.
	 * The vertex specification should match u_blitter's vertex element state. */
	u_upload_alloc(rctx->uploader, 0, sizeof(float) * 24, &offset, &buf, (void**)&vb);
	vb[0] = x1;
	vb[1] = y1;
	vb[2] = depth;
	vb[3] = 1;

	vb[8] = x1;
	vb[9] = y2;
	vb[10] = depth;
	vb[11] = 1;

	vb[16] = x2;
	vb[17] = y1;
	vb[18] = depth;
	vb[19] = 1;

	if (attrib) {
		memcpy(vb+4, attrib->f, sizeof(float)*4);
		memcpy(vb+12, attrib->f, sizeof(float)*4);
		memcpy(vb+20, attrib->f, sizeof(float)*4);
	}

	/* draw */
	util_draw_vertex_buffer(&rctx->b, NULL, buf, blitter->vb_slot, offset,
				R600_PRIM_RECTANGLE_LIST, 3, 2);
	pipe_resource_reference(&buf, NULL);
}

void r600_need_dma_space(struct r600_common_context *ctx, unsigned num_dw)
{
	/* The number of dwords we already used in the DMA so far. */
	num_dw += ctx->rings.dma.cs->cdw;
	/* Flush if there's not enough space. */
	if (num_dw > RADEON_MAX_CMDBUF_DWORDS) {
		ctx->rings.dma.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
	}
}

static void r600_memory_barrier(struct pipe_context *ctx, unsigned flags)
{
}

void r600_preflush_suspend_features(struct r600_common_context *ctx)
{
	/* Disable render condition. */
	ctx->saved_render_cond = NULL;
	ctx->saved_render_cond_cond = FALSE;
	ctx->saved_render_cond_mode = 0;
	if (ctx->current_render_cond) {
		ctx->saved_render_cond = ctx->current_render_cond;
		ctx->saved_render_cond_cond = ctx->current_render_cond_cond;
		ctx->saved_render_cond_mode = ctx->current_render_cond_mode;
		ctx->b.render_condition(&ctx->b, NULL, FALSE, 0);
	}

	/* suspend queries */
	ctx->nontimer_queries_suspended = false;
	if (ctx->num_cs_dw_nontimer_queries_suspend) {
		r600_suspend_nontimer_queries(ctx);
		ctx->nontimer_queries_suspended = true;
	}

	ctx->streamout.suspended = false;
	if (ctx->streamout.begin_emitted) {
		r600_emit_streamout_end(ctx);
		ctx->streamout.suspended = true;
	}
}

void r600_postflush_resume_features(struct r600_common_context *ctx)
{
	if (ctx->streamout.suspended) {
		ctx->streamout.append_bitmask = ctx->streamout.enabled_mask;
		r600_streamout_buffers_dirty(ctx);
	}

	/* resume queries */
	if (ctx->nontimer_queries_suspended) {
		r600_resume_nontimer_queries(ctx);
	}

	/* Re-enable render condition. */
	if (ctx->saved_render_cond) {
		ctx->b.render_condition(&ctx->b, ctx->saved_render_cond,
					  ctx->saved_render_cond_cond,
					  ctx->saved_render_cond_mode);
	}
}

static void r600_flush_from_st(struct pipe_context *ctx,
			       struct pipe_fence_handle **fence,
			       unsigned flags)
{
	struct r600_common_context *rctx = (struct r600_common_context *)ctx;
	unsigned rflags = 0;

	if (flags & PIPE_FLUSH_END_OF_FRAME)
		rflags |= RADEON_FLUSH_END_OF_FRAME;

	if (rctx->rings.dma.cs) {
		rctx->rings.dma.flush(rctx, rflags, NULL);
	}
	rctx->rings.gfx.flush(rctx, rflags, fence);
}

static void r600_flush_dma_ring(void *ctx, unsigned flags,
				struct pipe_fence_handle **fence)
{
	struct r600_common_context *rctx = (struct r600_common_context *)ctx;
	struct radeon_winsys_cs *cs = rctx->rings.dma.cs;

	if (!cs->cdw) {
		return;
	}

	rctx->rings.dma.flushing = true;
	rctx->ws->cs_flush(cs, flags, fence, 0);
	rctx->rings.dma.flushing = false;
}

bool r600_common_context_init(struct r600_common_context *rctx,
			      struct r600_common_screen *rscreen)
{
	util_slab_create(&rctx->pool_transfers,
			 sizeof(struct r600_transfer), 64,
			 UTIL_SLAB_SINGLETHREADED);

	rctx->screen = rscreen;
	rctx->ws = rscreen->ws;
	rctx->family = rscreen->family;
	rctx->chip_class = rscreen->chip_class;

	if (rscreen->family == CHIP_HAWAII)
		rctx->max_db = 16;
	else if (rscreen->chip_class >= EVERGREEN)
		rctx->max_db = 8;
	else
		rctx->max_db = 4;

	rctx->b.transfer_map = u_transfer_map_vtbl;
	rctx->b.transfer_flush_region = u_default_transfer_flush_region;
	rctx->b.transfer_unmap = u_transfer_unmap_vtbl;
	rctx->b.transfer_inline_write = u_default_transfer_inline_write;
        rctx->b.memory_barrier = r600_memory_barrier;
	rctx->b.flush = r600_flush_from_st;

	LIST_INITHEAD(&rctx->texture_buffers);

	r600_init_context_texture_functions(rctx);
	r600_streamout_init(rctx);
	r600_query_init(rctx);
	cayman_init_msaa(&rctx->b);

	rctx->allocator_so_filled_size = u_suballocator_create(&rctx->b, 4096, 4,
							       0, PIPE_USAGE_DEFAULT, TRUE);
	if (!rctx->allocator_so_filled_size)
		return false;

	rctx->uploader = u_upload_create(&rctx->b, 1024 * 1024, 256,
					PIPE_BIND_INDEX_BUFFER |
					PIPE_BIND_CONSTANT_BUFFER);
	if (!rctx->uploader)
		return false;

	if (rscreen->info.r600_has_dma && !(rscreen->debug_flags & DBG_NO_ASYNC_DMA)) {
		rctx->rings.dma.cs = rctx->ws->cs_create(rctx->ws, RING_DMA,
							 r600_flush_dma_ring,
							 rctx, NULL);
		rctx->rings.dma.flush = r600_flush_dma_ring;
	}

	return true;
}

void r600_common_context_cleanup(struct r600_common_context *rctx)
{
	if (rctx->rings.gfx.cs) {
		rctx->ws->cs_destroy(rctx->rings.gfx.cs);
	}
	if (rctx->rings.dma.cs) {
		rctx->ws->cs_destroy(rctx->rings.dma.cs);
	}

	if (rctx->uploader) {
		u_upload_destroy(rctx->uploader);
	}

	util_slab_destroy(&rctx->pool_transfers);

	if (rctx->allocator_so_filled_size) {
		u_suballocator_destroy(rctx->allocator_so_filled_size);
	}
}

void r600_context_add_resource_size(struct pipe_context *ctx, struct pipe_resource *r)
{
	struct r600_common_context *rctx = (struct r600_common_context *)ctx;
	struct r600_resource *rr = (struct r600_resource *)r;

	if (r == NULL) {
		return;
	}

	/*
	 * The idea is to compute a gross estimate of memory requirement of
	 * each draw call. After each draw call, memory will be precisely
	 * accounted. So the uncertainty is only on the current draw call.
	 * In practice this gave very good estimate (+/- 10% of the target
	 * memory limit).
	 */
	if (rr->domains & RADEON_DOMAIN_GTT) {
		rctx->gtt += rr->buf->size;
	}
	if (rr->domains & RADEON_DOMAIN_VRAM) {
		rctx->vram += rr->buf->size;
	}
}

/*
 * pipe_screen
 */

static const struct debug_named_value common_debug_options[] = {
	/* logging */
	{ "tex", DBG_TEX, "Print texture info" },
	{ "texmip", DBG_TEXMIP, "Print texture info (mipmapped only)" },
	{ "compute", DBG_COMPUTE, "Print compute info" },
	{ "vm", DBG_VM, "Print virtual addresses when creating resources" },
	{ "trace_cs", DBG_TRACE_CS, "Trace cs and write rlockup_<csid>.c file with faulty cs" },

	/* shaders */
	{ "fs", DBG_FS, "Print fetch shaders" },
	{ "vs", DBG_VS, "Print vertex shaders" },
	{ "gs", DBG_GS, "Print geometry shaders" },
	{ "ps", DBG_PS, "Print pixel shaders" },
	{ "cs", DBG_CS, "Print compute shaders" },

	/* features */
	{ "nodma", DBG_NO_ASYNC_DMA, "Disable asynchronous DMA" },
	{ "hyperz", DBG_HYPERZ, "Enable Hyper-Z" },
	/* GL uses the word INVALIDATE, gallium uses the word DISCARD */
	{ "noinvalrange", DBG_NO_DISCARD_RANGE, "Disable handling of INVALIDATE_RANGE map flags" },
	{ "no2d", DBG_NO_2D_TILING, "Disable 2D tiling" },
	{ "notiling", DBG_NO_TILING, "Disable tiling" },
	{ "switch_on_eop", DBG_SWITCH_ON_EOP, "Program WD/IA to switch on end-of-packet." },

	DEBUG_NAMED_VALUE_END /* must be last */
};

static const char* r600_get_vendor(struct pipe_screen* pscreen)
{
	return "X.Org";
}

static const char* r600_get_name(struct pipe_screen* pscreen)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)pscreen;

	switch (rscreen->family) {
	case CHIP_R600: return "AMD R600";
	case CHIP_RV610: return "AMD RV610";
	case CHIP_RV630: return "AMD RV630";
	case CHIP_RV670: return "AMD RV670";
	case CHIP_RV620: return "AMD RV620";
	case CHIP_RV635: return "AMD RV635";
	case CHIP_RS780: return "AMD RS780";
	case CHIP_RS880: return "AMD RS880";
	case CHIP_RV770: return "AMD RV770";
	case CHIP_RV730: return "AMD RV730";
	case CHIP_RV710: return "AMD RV710";
	case CHIP_RV740: return "AMD RV740";
	case CHIP_CEDAR: return "AMD CEDAR";
	case CHIP_REDWOOD: return "AMD REDWOOD";
	case CHIP_JUNIPER: return "AMD JUNIPER";
	case CHIP_CYPRESS: return "AMD CYPRESS";
	case CHIP_HEMLOCK: return "AMD HEMLOCK";
	case CHIP_PALM: return "AMD PALM";
	case CHIP_SUMO: return "AMD SUMO";
	case CHIP_SUMO2: return "AMD SUMO2";
	case CHIP_BARTS: return "AMD BARTS";
	case CHIP_TURKS: return "AMD TURKS";
	case CHIP_CAICOS: return "AMD CAICOS";
	case CHIP_CAYMAN: return "AMD CAYMAN";
	case CHIP_ARUBA: return "AMD ARUBA";
	case CHIP_TAHITI: return "AMD TAHITI";
	case CHIP_PITCAIRN: return "AMD PITCAIRN";
	case CHIP_VERDE: return "AMD CAPE VERDE";
	case CHIP_OLAND: return "AMD OLAND";
	case CHIP_HAINAN: return "AMD HAINAN";
	case CHIP_BONAIRE: return "AMD BONAIRE";
	case CHIP_KAVERI: return "AMD KAVERI";
	case CHIP_KABINI: return "AMD KABINI";
	case CHIP_HAWAII: return "AMD HAWAII";
	case CHIP_MULLINS: return "AMD MULLINS";
	default: return "AMD unknown";
	}
}

static float r600_get_paramf(struct pipe_screen* pscreen,
			     enum pipe_capf param)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen *)pscreen;

	switch (param) {
	case PIPE_CAPF_MAX_LINE_WIDTH:
	case PIPE_CAPF_MAX_LINE_WIDTH_AA:
	case PIPE_CAPF_MAX_POINT_WIDTH:
	case PIPE_CAPF_MAX_POINT_WIDTH_AA:
		if (rscreen->family >= CHIP_CEDAR)
			return 16384.0f;
		else
			return 8192.0f;
	case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
		return 16.0f;
	case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
		return 16.0f;
	case PIPE_CAPF_GUARD_BAND_LEFT:
	case PIPE_CAPF_GUARD_BAND_TOP:
	case PIPE_CAPF_GUARD_BAND_RIGHT:
	case PIPE_CAPF_GUARD_BAND_BOTTOM:
		return 0.0f;
	}
	return 0.0f;
}

static int r600_get_video_param(struct pipe_screen *screen,
				enum pipe_video_profile profile,
				enum pipe_video_entrypoint entrypoint,
				enum pipe_video_cap param)
{
	switch (param) {
	case PIPE_VIDEO_CAP_SUPPORTED:
		return vl_profile_supported(screen, profile, entrypoint);
	case PIPE_VIDEO_CAP_NPOT_TEXTURES:
		return 1;
	case PIPE_VIDEO_CAP_MAX_WIDTH:
	case PIPE_VIDEO_CAP_MAX_HEIGHT:
		return vl_video_buffer_max_size(screen);
	case PIPE_VIDEO_CAP_PREFERED_FORMAT:
		return PIPE_FORMAT_NV12;
	case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
		return false;
	case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
		return false;
	case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
		return true;
	case PIPE_VIDEO_CAP_MAX_LEVEL:
		return vl_level_supported(screen, profile);
	default:
		return 0;
	}
}

const char *r600_get_llvm_processor_name(enum radeon_family family)
{
	switch (family) {
	case CHIP_R600:
	case CHIP_RV630:
	case CHIP_RV635:
	case CHIP_RV670:
		return "r600";
	case CHIP_RV610:
	case CHIP_RV620:
	case CHIP_RS780:
	case CHIP_RS880:
		return "rs880";
	case CHIP_RV710:
		return "rv710";
	case CHIP_RV730:
		return "rv730";
	case CHIP_RV740:
	case CHIP_RV770:
		return "rv770";
	case CHIP_PALM:
	case CHIP_CEDAR:
		return "cedar";
	case CHIP_SUMO:
	case CHIP_SUMO2:
		return "sumo";
	case CHIP_REDWOOD:
		return "redwood";
	case CHIP_JUNIPER:
		return "juniper";
	case CHIP_HEMLOCK:
	case CHIP_CYPRESS:
		return "cypress";
	case CHIP_BARTS:
		return "barts";
	case CHIP_TURKS:
		return "turks";
	case CHIP_CAICOS:
		return "caicos";
	case CHIP_CAYMAN:
        case CHIP_ARUBA:
		return "cayman";

	case CHIP_TAHITI: return "tahiti";
	case CHIP_PITCAIRN: return "pitcairn";
	case CHIP_VERDE: return "verde";
	case CHIP_OLAND: return "oland";
	case CHIP_HAINAN: return "hainan";
	case CHIP_BONAIRE: return "bonaire";
	case CHIP_KABINI: return "kabini";
	case CHIP_KAVERI: return "kaveri";
	case CHIP_HAWAII: return "hawaii";
	case CHIP_MULLINS:
#if HAVE_LLVM >= 0x0305
		return "mullins";
#else
		return "kabini";
#endif
	default: return "";
	}
}

static int r600_get_compute_param(struct pipe_screen *screen,
        enum pipe_compute_cap param,
        void *ret)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen *)screen;

	//TODO: select these params by asic
	switch (param) {
	case PIPE_COMPUTE_CAP_IR_TARGET: {
		const char *gpu;
		switch(rscreen->family) {
		/* Clang < 3.6 is missing Hainan in its list of
		 * GPUs, so we need to use the name of a similar GPU.
		 */
#if HAVE_LLVM < 0x0306
		case CHIP_HAINAN:
			gpu = "oland";
			break;
#endif
		default:
			gpu = r600_get_llvm_processor_name(rscreen->family);
			break;
		}
		if (ret) {
			sprintf(ret, "%s-r600--", gpu);
		}
		return (8 + strlen(gpu)) * sizeof(char);
	}
	case PIPE_COMPUTE_CAP_GRID_DIMENSION:
		if (ret) {
			uint64_t *grid_dimension = ret;
			grid_dimension[0] = 3;
		}
		return 1 * sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
		if (ret) {
			uint64_t *grid_size = ret;
			grid_size[0] = 65535;
			grid_size[1] = 65535;
			grid_size[2] = 1;
		}
		return 3 * sizeof(uint64_t) ;

	case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
		if (ret) {
			uint64_t *block_size = ret;
			block_size[0] = 256;
			block_size[1] = 256;
			block_size[2] = 256;
		}
		return 3 * sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
		if (ret) {
			uint64_t *max_threads_per_block = ret;
			*max_threads_per_block = 256;
		}
		return sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
		if (ret) {
			uint64_t *max_global_size = ret;
			uint64_t max_mem_alloc_size;

			r600_get_compute_param(screen,
				PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE,
				&max_mem_alloc_size);

			/* In OpenCL, the MAX_MEM_ALLOC_SIZE must be at least
			 * 1/4 of the MAX_GLOBAL_SIZE.  Since the
			 * MAX_MEM_ALLOC_SIZE is fixed for older kernels,
			 * make sure we never report more than
			 * 4 * MAX_MEM_ALLOC_SIZE.
			 */
			*max_global_size = MIN2(4 * max_mem_alloc_size,
				rscreen->info.gart_size +
				rscreen->info.vram_size);
		}
		return sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
		if (ret) {
			uint64_t *max_local_size = ret;
			/* Value reported by the closed source driver. */
			*max_local_size = 32768;
		}
		return sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
		if (ret) {
			uint64_t *max_input_size = ret;
			/* Value reported by the closed source driver. */
			*max_input_size = 1024;
		}
		return sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE:
		if (ret) {
			uint64_t max_global_size;
			uint64_t *max_mem_alloc_size = ret;

			/* XXX: The limit in older kernels is 256 MB.  We
			 * should add a query here for newer kernels.
			 */
			*max_mem_alloc_size = 256 * 1024 * 1024;
		}
		return sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
		if (ret) {
			uint32_t *max_clock_frequency = ret;
			*max_clock_frequency = rscreen->info.max_sclk;
		}
		return sizeof(uint32_t);

	case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
		if (ret) {
			uint32_t *max_compute_units = ret;
			*max_compute_units = MAX2(rscreen->info.max_compute_units, 1);
		}
		return sizeof(uint32_t);

	case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
		if (ret) {
			uint32_t *images_supported = ret;
			*images_supported = 0;
		}
		return sizeof(uint32_t);
	}

        fprintf(stderr, "unknown PIPE_COMPUTE_CAP %d\n", param);
        return 0;
}

static uint64_t r600_get_timestamp(struct pipe_screen *screen)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;

	return 1000000 * rscreen->ws->query_value(rscreen->ws, RADEON_TIMESTAMP) /
			rscreen->info.r600_clock_crystal_freq;
}

static int r600_get_driver_query_info(struct pipe_screen *screen,
				      unsigned index,
				      struct pipe_driver_query_info *info)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;
	struct pipe_driver_query_info list[] = {
		{"draw-calls", R600_QUERY_DRAW_CALLS, 0},
		{"requested-VRAM", R600_QUERY_REQUESTED_VRAM, rscreen->info.vram_size, TRUE},
		{"requested-GTT", R600_QUERY_REQUESTED_GTT, rscreen->info.gart_size, TRUE},
		{"buffer-wait-time", R600_QUERY_BUFFER_WAIT_TIME, 0, FALSE},
		{"num-cs-flushes", R600_QUERY_NUM_CS_FLUSHES, 0, FALSE},
		{"num-bytes-moved", R600_QUERY_NUM_BYTES_MOVED, 0, TRUE},
		{"VRAM-usage", R600_QUERY_VRAM_USAGE, rscreen->info.vram_size, TRUE},
		{"GTT-usage", R600_QUERY_GTT_USAGE, rscreen->info.gart_size, TRUE},
	};

	if (!info)
		return Elements(list);

	if (index >= Elements(list))
		return 0;

	*info = list[index];
	return 1;
}

static void r600_fence_reference(struct pipe_screen *screen,
				 struct pipe_fence_handle **ptr,
				 struct pipe_fence_handle *fence)
{
	struct radeon_winsys *rws = ((struct r600_common_screen*)screen)->ws;

	rws->fence_reference(ptr, fence);
}

static boolean r600_fence_signalled(struct pipe_screen *screen,
				    struct pipe_fence_handle *fence)
{
	struct radeon_winsys *rws = ((struct r600_common_screen*)screen)->ws;

	return rws->fence_wait(rws, fence, 0);
}

static boolean r600_fence_finish(struct pipe_screen *screen,
				 struct pipe_fence_handle *fence,
				 uint64_t timeout)
{
	struct radeon_winsys *rws = ((struct r600_common_screen*)screen)->ws;

	return rws->fence_wait(rws, fence, timeout);
}

static bool r600_interpret_tiling(struct r600_common_screen *rscreen,
				  uint32_t tiling_config)
{
	switch ((tiling_config & 0xe) >> 1) {
	case 0:
		rscreen->tiling_info.num_channels = 1;
		break;
	case 1:
		rscreen->tiling_info.num_channels = 2;
		break;
	case 2:
		rscreen->tiling_info.num_channels = 4;
		break;
	case 3:
		rscreen->tiling_info.num_channels = 8;
		break;
	default:
		return false;
	}

	switch ((tiling_config & 0x30) >> 4) {
	case 0:
		rscreen->tiling_info.num_banks = 4;
		break;
	case 1:
		rscreen->tiling_info.num_banks = 8;
		break;
	default:
		return false;

	}
	switch ((tiling_config & 0xc0) >> 6) {
	case 0:
		rscreen->tiling_info.group_bytes = 256;
		break;
	case 1:
		rscreen->tiling_info.group_bytes = 512;
		break;
	default:
		return false;
	}
	return true;
}

static bool evergreen_interpret_tiling(struct r600_common_screen *rscreen,
				       uint32_t tiling_config)
{
	switch (tiling_config & 0xf) {
	case 0:
		rscreen->tiling_info.num_channels = 1;
		break;
	case 1:
		rscreen->tiling_info.num_channels = 2;
		break;
	case 2:
		rscreen->tiling_info.num_channels = 4;
		break;
	case 3:
		rscreen->tiling_info.num_channels = 8;
		break;
	default:
		return false;
	}

	switch ((tiling_config & 0xf0) >> 4) {
	case 0:
		rscreen->tiling_info.num_banks = 4;
		break;
	case 1:
		rscreen->tiling_info.num_banks = 8;
		break;
	case 2:
		rscreen->tiling_info.num_banks = 16;
		break;
	default:
		return false;
	}

	switch ((tiling_config & 0xf00) >> 8) {
	case 0:
		rscreen->tiling_info.group_bytes = 256;
		break;
	case 1:
		rscreen->tiling_info.group_bytes = 512;
		break;
	default:
		return false;
	}
	return true;
}

static bool r600_init_tiling(struct r600_common_screen *rscreen)
{
	uint32_t tiling_config = rscreen->info.r600_tiling_config;

	/* set default group bytes, overridden by tiling info ioctl */
	if (rscreen->chip_class <= R700) {
		rscreen->tiling_info.group_bytes = 256;
	} else {
		rscreen->tiling_info.group_bytes = 512;
	}

	if (!tiling_config)
		return true;

	if (rscreen->chip_class <= R700) {
		return r600_interpret_tiling(rscreen, tiling_config);
	} else {
		return evergreen_interpret_tiling(rscreen, tiling_config);
	}
}

struct pipe_resource *r600_resource_create_common(struct pipe_screen *screen,
						  const struct pipe_resource *templ)
{
	if (templ->target == PIPE_BUFFER) {
		return r600_buffer_create(screen, templ, 4096);
	} else {
		return r600_texture_create(screen, templ);
	}
}

bool r600_common_screen_init(struct r600_common_screen *rscreen,
			     struct radeon_winsys *ws)
{
	ws->query_info(ws, &rscreen->info);

	rscreen->b.get_name = r600_get_name;
	rscreen->b.get_vendor = r600_get_vendor;
	rscreen->b.get_compute_param = r600_get_compute_param;
	rscreen->b.get_paramf = r600_get_paramf;
	rscreen->b.get_driver_query_info = r600_get_driver_query_info;
	rscreen->b.get_timestamp = r600_get_timestamp;
	rscreen->b.fence_finish = r600_fence_finish;
	rscreen->b.fence_reference = r600_fence_reference;
	rscreen->b.fence_signalled = r600_fence_signalled;
	rscreen->b.resource_destroy = u_resource_destroy_vtbl;

	if (rscreen->info.has_uvd) {
		rscreen->b.get_video_param = rvid_get_video_param;
		rscreen->b.is_video_format_supported = rvid_is_format_supported;
	} else {
		rscreen->b.get_video_param = r600_get_video_param;
		rscreen->b.is_video_format_supported = vl_video_buffer_is_format_supported;
	}

	r600_init_screen_texture_functions(rscreen);

	rscreen->ws = ws;
	rscreen->family = rscreen->info.family;
	rscreen->chip_class = rscreen->info.chip_class;
	rscreen->debug_flags = debug_get_flags_option("R600_DEBUG", common_debug_options, 0);

	if (!r600_init_tiling(rscreen)) {
		return false;
	}
	util_format_s3tc_init();
	pipe_mutex_init(rscreen->aux_context_lock);

	if (rscreen->info.drm_minor >= 28 && (rscreen->debug_flags & DBG_TRACE_CS)) {
		rscreen->trace_bo = (struct r600_resource*)pipe_buffer_create(&rscreen->b,
										PIPE_BIND_CUSTOM,
										PIPE_USAGE_STAGING,
										4096);
		if (rscreen->trace_bo) {
			rscreen->trace_ptr = rscreen->ws->buffer_map(rscreen->trace_bo->cs_buf, NULL,
									PIPE_TRANSFER_UNSYNCHRONIZED);
		}
	}

	return true;
}

void r600_destroy_common_screen(struct r600_common_screen *rscreen)
{
	pipe_mutex_destroy(rscreen->aux_context_lock);
	rscreen->aux_context->destroy(rscreen->aux_context);

	if (rscreen->trace_bo) {
		rscreen->ws->buffer_unmap(rscreen->trace_bo->cs_buf);
		pipe_resource_reference((struct pipe_resource**)&rscreen->trace_bo, NULL);
	}

	rscreen->ws->destroy(rscreen->ws);
	FREE(rscreen);
}

static unsigned tgsi_get_processor_type(const struct tgsi_token *tokens)
{
	struct tgsi_parse_context parse;

	if (tgsi_parse_init( &parse, tokens ) != TGSI_PARSE_OK) {
		debug_printf("tgsi_parse_init() failed in %s:%i!\n", __func__, __LINE__);
		return ~0;
	}
	return parse.FullHeader.Processor.Processor;
}

bool r600_can_dump_shader(struct r600_common_screen *rscreen,
			  const struct tgsi_token *tokens)
{
	/* Compute shader don't have tgsi_tokens */
	if (!tokens)
		return (rscreen->debug_flags & DBG_CS) != 0;

	switch (tgsi_get_processor_type(tokens)) {
	case TGSI_PROCESSOR_VERTEX:
		return (rscreen->debug_flags & DBG_VS) != 0;
	case TGSI_PROCESSOR_GEOMETRY:
		return (rscreen->debug_flags & DBG_GS) != 0;
	case TGSI_PROCESSOR_FRAGMENT:
		return (rscreen->debug_flags & DBG_PS) != 0;
	case TGSI_PROCESSOR_COMPUTE:
		return (rscreen->debug_flags & DBG_CS) != 0;
	default:
		return false;
	}
}

void r600_screen_clear_buffer(struct r600_common_screen *rscreen, struct pipe_resource *dst,
			      unsigned offset, unsigned size, unsigned value)
{
	struct r600_common_context *rctx = (struct r600_common_context*)rscreen->aux_context;

	pipe_mutex_lock(rscreen->aux_context_lock);
	rctx->clear_buffer(&rctx->b, dst, offset, size, value);
	rscreen->aux_context->flush(rscreen->aux_context, NULL, 0);
	pipe_mutex_unlock(rscreen->aux_context_lock);
}
