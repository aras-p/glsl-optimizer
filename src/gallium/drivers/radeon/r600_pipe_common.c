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

static const struct debug_named_value common_debug_options[] = {
	/* logging */
	{ "texdepth", DBG_TEX_DEPTH, "Print texture depth info" },
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

void r600_common_screen_init(struct r600_common_screen *rscreen,
			     struct radeon_winsys *ws)
{
	ws->query_info(ws, &rscreen->info);

	rscreen->ws = ws;
	rscreen->family = rscreen->info.family;
	rscreen->chip_class = rscreen->info.chip_class;
	rscreen->debug_flags = debug_get_flags_option("R600_DEBUG", common_debug_options, 0);
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
