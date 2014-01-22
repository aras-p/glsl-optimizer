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
#include "util/u_format_s3tc.h"
#include "util/u_upload_mgr.h"
#include <inttypes.h>

/*
 * pipe_context
 */

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
	rctx->max_db = rscreen->chip_class >= EVERGREEN ? 8 : 4;

	rctx->b.transfer_map = u_transfer_map_vtbl;
	rctx->b.transfer_flush_region = u_default_transfer_flush_region;
	rctx->b.transfer_unmap = u_transfer_unmap_vtbl;
	rctx->b.transfer_inline_write = u_default_transfer_inline_write;

	r600_streamout_init(rctx);
	r600_query_init(rctx);

	rctx->allocator_so_filled_size = u_suballocator_create(&rctx->b, 4096, 4,
							       0, PIPE_USAGE_STATIC, TRUE);
	if (!rctx->allocator_so_filled_size)
		return false;

	rctx->uploader = u_upload_create(&rctx->b, 1024 * 1024, 256,
					PIPE_BIND_INDEX_BUFFER |
					PIPE_BIND_CONSTANT_BUFFER);
	if (!rctx->uploader)
		return false;

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

	{ "nohyperz", DBG_NO_HYPERZ, "Disable Hyper-Z" },
	/* GL uses the word INVALIDATE, gallium uses the word DISCARD */
	{ "noinvalrange", DBG_NO_DISCARD_RANGE, "Disable handling of INVALIDATE_RANGE map flags" },

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
	default: return "AMD unknown";
	}
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
	rscreen->b.fence_finish = r600_fence_finish;
	rscreen->b.fence_reference = r600_fence_reference;
	rscreen->b.fence_signalled = r600_fence_signalled;
	rscreen->b.resource_create = r600_resource_create_common;
	rscreen->b.resource_destroy = u_resource_destroy_vtbl;

	r600_init_texture_functions(rscreen);

	rscreen->ws = ws;
	rscreen->family = rscreen->info.family;
	rscreen->chip_class = rscreen->info.chip_class;
	rscreen->debug_flags = debug_get_flags_option("R600_DEBUG", common_debug_options, 0);

	if (!r600_init_tiling(rscreen)) {
		return false;
	}

	util_format_s3tc_init();

	pipe_mutex_init(rscreen->aux_context_lock);
	return true;
}

void r600_common_screen_cleanup(struct r600_common_screen *rscreen)
{
	pipe_mutex_destroy(rscreen->aux_context_lock);
	rscreen->aux_context->destroy(rscreen->aux_context);
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
