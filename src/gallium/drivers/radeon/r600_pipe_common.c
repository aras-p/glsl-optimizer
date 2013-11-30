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
#include <inttypes.h>

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

	DEBUG_NAMED_VALUE_END /* must be last */
};

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

bool r600_common_screen_init(struct r600_common_screen *rscreen,
			     struct radeon_winsys *ws)
{
	ws->query_info(ws, &rscreen->info);

	rscreen->b.fence_finish = r600_fence_finish;
	rscreen->b.fence_reference = r600_fence_reference;
	rscreen->b.fence_signalled = r600_fence_signalled;

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

bool r600_common_context_init(struct r600_common_context *rctx,
			      struct r600_common_screen *rscreen)
{
	rctx->ws = rscreen->ws;
	rctx->family = rscreen->family;
	rctx->chip_class = rscreen->chip_class;

	r600_streamout_init(rctx);

	rctx->allocator_so_filled_size = u_suballocator_create(&rctx->b, 4096, 4,
							       0, PIPE_USAGE_STATIC, TRUE);
	if (!rctx->allocator_so_filled_size)
		return false;

	return true;
}

void r600_common_context_cleanup(struct r600_common_context *rctx)
{
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

boolean r600_rings_is_buffer_referenced(struct r600_common_context *ctx,
					struct radeon_winsys_cs_handle *buf,
					enum radeon_bo_usage usage)
{
	if (ctx->ws->cs_is_buffer_referenced(ctx->rings.gfx.cs, buf, usage)) {
		return TRUE;
	}
	if (ctx->rings.dma.cs &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.dma.cs, buf, usage)) {
		return TRUE;
	}
	return FALSE;
}

void *r600_buffer_map_sync_with_rings(struct r600_common_context *ctx,
                                      struct r600_resource *resource,
                                      unsigned usage)
{
	enum radeon_bo_usage rusage = RADEON_USAGE_READWRITE;

	if (usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
		return ctx->ws->buffer_map(resource->cs_buf, NULL, usage);
	}

	if (!(usage & PIPE_TRANSFER_WRITE)) {
		/* have to wait for the last write */
		rusage = RADEON_USAGE_WRITE;
	}

	if (ctx->rings.gfx.cs->cdw &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.gfx.cs,
					     resource->cs_buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			ctx->rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC);
			return NULL;
		} else {
			ctx->rings.gfx.flush(ctx, 0);
		}
	}
	if (ctx->rings.dma.cs &&
	    ctx->rings.dma.cs->cdw &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.dma.cs,
					     resource->cs_buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			ctx->rings.dma.flush(ctx, RADEON_FLUSH_ASYNC);
			return NULL;
		} else {
			ctx->rings.dma.flush(ctx, 0);
		}
	}

	if (ctx->ws->buffer_is_busy(resource->buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			return NULL;
		} else {
			/* We will be wait for the GPU. Wait for any offloaded
			 * CS flush to complete to avoid busy-waiting in the winsys. */
			ctx->ws->cs_sync_flush(ctx->rings.gfx.cs);
			if (ctx->rings.dma.cs)
				ctx->ws->cs_sync_flush(ctx->rings.dma.cs);
		}
	}

	return ctx->ws->buffer_map(resource->cs_buf, NULL, usage);
}

bool r600_init_resource(struct r600_common_screen *rscreen,
			struct r600_resource *res,
			unsigned size, unsigned alignment,
			bool use_reusable_pool, unsigned usage)
{
	uint32_t initial_domain, domains;

	switch(usage) {
	case PIPE_USAGE_STAGING:
		/* Staging resources participate in transfers, i.e. are used
		 * for uploads and downloads from regular resources.
		 * We generate them internally for some transfers.
		 */
		initial_domain = RADEON_DOMAIN_GTT;
		domains = RADEON_DOMAIN_GTT;
		break;
	case PIPE_USAGE_DYNAMIC:
	case PIPE_USAGE_STREAM:
		/* Default to GTT, but allow the memory manager to move it to VRAM. */
		initial_domain = RADEON_DOMAIN_GTT;
		domains = RADEON_DOMAIN_GTT | RADEON_DOMAIN_VRAM;
		break;
	case PIPE_USAGE_DEFAULT:
	case PIPE_USAGE_STATIC:
	case PIPE_USAGE_IMMUTABLE:
	default:
		/* Don't list GTT here, because the memory manager would put some
		 * resources to GTT no matter what the initial domain is.
		 * Not listing GTT in the domains improves performance a lot. */
		initial_domain = RADEON_DOMAIN_VRAM;
		domains = RADEON_DOMAIN_VRAM;
		break;
	}

	res->buf = rscreen->ws->buffer_create(rscreen->ws, size, alignment,
                                              use_reusable_pool,
                                              initial_domain);
	if (!res->buf) {
		return false;
	}

	res->cs_buf = rscreen->ws->buffer_get_cs_handle(res->buf);
	res->domains = domains;
	util_range_set_empty(&res->valid_buffer_range);

	if (rscreen->debug_flags & DBG_VM && res->b.b.target == PIPE_BUFFER) {
		fprintf(stderr, "VM start=0x%"PRIu64"  end=0x%"PRIu64" | Buffer %u bytes\n",
			r600_resource_va(&rscreen->b, &res->b.b),
			r600_resource_va(&rscreen->b, &res->b.b) + res->buf->size,
			res->buf->size);
	}
	return true;
}
