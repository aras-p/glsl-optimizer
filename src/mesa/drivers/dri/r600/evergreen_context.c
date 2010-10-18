/*
 * Copyright (C) 2008-2010  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#include "main/glheader.h"
#include "main/api_arrayelt.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "main/imports.h"
#include "main/bufferobj.h"
#include "main/texobj.h"

#include "radeon_common_context.h"
#include "evergreen_context.h"
#include "evergreen_state.h"
#include "evergreen_blit.h"
#include "r600_cmdbuf.h"
#include "radeon_queryobj.h"

static void evergreen_get_lock(radeonContextPtr rmesa)
{
	drm_radeon_sarea_t *sarea = rmesa->sarea;

	if (sarea->ctx_owner != rmesa->dri.hwContext) {
		sarea->ctx_owner = rmesa->dri.hwContext;
		if (!rmesa->radeonScreen->kernel_mm)
			radeon_bo_legacy_texture_age(rmesa->radeonScreen->bom);
	}
}

static void evergreen_vtbl_emit_cs_header(struct radeon_cs *cs, radeonContextPtr rmesa)
{
    /* please flush pipe do all pending work */
    /* to be enabled */
}

static void evergreen_vtbl_pre_emit_atoms(radeonContextPtr radeon)
{
	r700Start3D((context_t *)radeon);
}

static void evergreen_fallback(struct gl_context *ctx, GLuint bit, GLboolean mode)
{
	context_t *context = EVERGREEN_CONTEXT(ctx);
	if (mode)
		context->radeon.Fallback |= bit;
	else
		context->radeon.Fallback &= ~bit;
}

static void evergreen_emit_query_finish(radeonContextPtr radeon)
{
	context_t *context = (context_t*) radeon;
	BATCH_LOCALS(&context->radeon);

	struct radeon_query_object *query = radeon->query.current;

	BEGIN_BATCH_NO_AUTOSTATE(4 + 2);
	R600_OUT_BATCH(CP_PACKET3(R600_IT_EVENT_WRITE, 2));
	R600_OUT_BATCH(R600_EVENT_TYPE(ZPASS_DONE) | R600_EVENT_INDEX(1));
	R600_OUT_BATCH(query->curr_offset + 8); /* hw writes qwords */
	R600_OUT_BATCH(0x00000000);
	R600_OUT_BATCH_RELOC(VGT_EVENT_INITIATOR, query->bo, 0, 0, RADEON_GEM_DOMAIN_GTT, 0);
	END_BATCH();
	assert(query->curr_offset < RADEON_QUERY_PAGE_SIZE);
	query->emitted_begin = GL_FALSE;
}

void evergreen_init_vtbl(radeonContextPtr radeon)
{
	radeon->vtbl.get_lock = evergreen_get_lock;
	radeon->vtbl.update_viewport_offset = evergreenUpdateViewportOffset;
	radeon->vtbl.emit_cs_header = evergreen_vtbl_emit_cs_header;
	radeon->vtbl.swtcl_flush = NULL;
	radeon->vtbl.pre_emit_atoms = evergreen_vtbl_pre_emit_atoms;
	radeon->vtbl.fallback = evergreen_fallback;
	radeon->vtbl.emit_query_finish = evergreen_emit_query_finish;
	radeon->vtbl.check_blit = evergreen_check_blit;
	radeon->vtbl.blit = evergreen_blit;
	radeon->vtbl.is_format_renderable = r600IsFormatRenderable;
}



