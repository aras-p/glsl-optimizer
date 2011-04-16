/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <errno.h>
#include <pipe/p_defines.h>
#include <pipe/p_state.h>
#include <pipe/p_context.h>
#include <tgsi/tgsi_scan.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_util.h>
#include <util/u_blitter.h>
#include <util/u_double_list.h>
#include "util/u_format.h"
#include <util/u_format_s3tc.h>
#include <util/u_transfer.h>
#include <util/u_surface.h>
#include <util/u_pack_color.h>
#include <util/u_memory.h>
#include <util/u_inlines.h>
#include "util/u_upload_mgr.h"
#include "os/os_time.h"
#include <pipebuffer/pb_buffer.h>
#include "r600.h"
#include "r600d.h"
#include "r600_resource.h"
#include "r600_shader.h"
#include "r600_pipe.h"
#include "r600_state_inlines.h"

/*
 * pipe_context
 */
static struct r600_fence *r600_create_fence(struct r600_pipe_context *ctx)
{
	struct r600_fence *fence = NULL;

	if (!ctx->fences.bo) {
		/* Create the shared buffer object */
		ctx->fences.bo = r600_bo(ctx->radeon, 4096, 0, 0, 0);
		if (!ctx->fences.bo) {
			R600_ERR("r600: failed to create bo for fence objects\n");
			return NULL;
		}
		ctx->fences.data = r600_bo_map(ctx->radeon, ctx->fences.bo, PB_USAGE_UNSYNCHRONIZED, NULL);
	}

	if (!LIST_IS_EMPTY(&ctx->fences.pool)) {
		struct r600_fence *entry;

		/* Try to find a freed fence that has been signalled */
		LIST_FOR_EACH_ENTRY(entry, &ctx->fences.pool, head) {
			if (ctx->fences.data[entry->index] != 0) {
				LIST_DELINIT(&entry->head);
				fence = entry;
				break;
			}
		}
	}

	if (!fence) {
		/* Allocate a new fence */
		struct r600_fence_block *block;
		unsigned index;

		if ((ctx->fences.next_index + 1) >= 1024) {
			R600_ERR("r600: too many concurrent fences\n");
			return NULL;
		}

		index = ctx->fences.next_index++;

		if (!(index % FENCE_BLOCK_SIZE)) {
			/* Allocate a new block */
			block = CALLOC_STRUCT(r600_fence_block);
			if (block == NULL)
				return NULL;

			LIST_ADD(&block->head, &ctx->fences.blocks);
		} else {
			block = LIST_ENTRY(struct r600_fence_block, ctx->fences.blocks.next, head);
		}

		fence = &block->fences[index % FENCE_BLOCK_SIZE];
		fence->ctx = ctx;
		fence->index = index;
	}

	pipe_reference_init(&fence->reference, 1);

	ctx->fences.data[fence->index] = 0;
	r600_context_emit_fence(&ctx->ctx, ctx->fences.bo, fence->index, 1);
	return fence;
}

static void r600_flush(struct pipe_context *ctx,
			struct pipe_fence_handle **fence)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_fence **rfence = (struct r600_fence**)fence;

#if 0
	static int dc = 0;
	char dname[256];
#endif

	if (rfence)
		*rfence = r600_create_fence(rctx);

	if (!rctx->ctx.pm4_cdwords)
		return;

#if 0
	sprintf(dname, "gallium-%08d.bof", dc);
	if (dc < 20) {
		r600_context_dump_bof(&rctx->ctx, dname);
		R600_ERR("dumped %s\n", dname);
	}
	dc++;
#endif
	r600_context_flush(&rctx->ctx);

	/* XXX This shouldn't be really necessary, but removing it breaks some tests.
	 * Needless buffer reallocations may significantly increase memory consumption,
	 * so getting rid of this call is important. */
	u_upload_flush(rctx->vbuf_mgr->uploader);
}

static void r600_update_num_contexts(struct r600_screen *rscreen, int diff)
{
	pipe_mutex_lock(rscreen->mutex_num_contexts);
	if (diff > 0) {
		rscreen->num_contexts++;

		if (rscreen->num_contexts > 1)
			util_slab_set_thread_safety(&rscreen->pool_buffers,
						    UTIL_SLAB_MULTITHREADED);
	} else {
		rscreen->num_contexts--;

		if (rscreen->num_contexts <= 1)
			util_slab_set_thread_safety(&rscreen->pool_buffers,
						    UTIL_SLAB_SINGLETHREADED);
	}
	pipe_mutex_unlock(rscreen->mutex_num_contexts);
}

static void r600_destroy_context(struct pipe_context *context)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)context;

	rctx->context.delete_depth_stencil_alpha_state(&rctx->context, rctx->custom_dsa_flush);
	util_unreference_framebuffer_state(&rctx->framebuffer);

	r600_context_fini(&rctx->ctx);

	util_blitter_destroy(rctx->blitter);

	for (int i = 0; i < R600_PIPE_NSTATES; i++) {
		free(rctx->states[i]);
	}

	u_vbuf_mgr_destroy(rctx->vbuf_mgr);
	util_slab_destroy(&rctx->pool_transfers);

	if (rctx->fences.bo) {
		struct r600_fence_block *entry, *tmp;

		LIST_FOR_EACH_ENTRY_SAFE(entry, tmp, &rctx->fences.blocks, head) {
			LIST_DEL(&entry->head);
			FREE(entry);
		}

		r600_bo_unmap(rctx->radeon, rctx->fences.bo);
		r600_bo_reference(rctx->radeon, &rctx->fences.bo, NULL);
	}

	r600_update_num_contexts(rctx->screen, -1);

	FREE(rctx);
}

static struct pipe_context *r600_create_context(struct pipe_screen *screen, void *priv)
{
	struct r600_pipe_context *rctx = CALLOC_STRUCT(r600_pipe_context);
	struct r600_screen* rscreen = (struct r600_screen *)screen;
	enum chip_class class;

	if (rctx == NULL)
		return NULL;

	r600_update_num_contexts(rscreen, 1);

	rctx->context.winsys = rscreen->screen.winsys;
	rctx->context.screen = screen;
	rctx->context.priv = priv;
	rctx->context.destroy = r600_destroy_context;
	rctx->context.flush = r600_flush;

	/* Easy accessing of screen/winsys. */
	rctx->screen = rscreen;
	rctx->radeon = rscreen->radeon;
	rctx->family = r600_get_family(rctx->radeon);

	rctx->fences.bo = NULL;
	rctx->fences.data = NULL;
	rctx->fences.next_index = 0;
	LIST_INITHEAD(&rctx->fences.pool);
	LIST_INITHEAD(&rctx->fences.blocks);

	r600_init_blit_functions(rctx);
	r600_init_query_functions(rctx);
	r600_init_context_resource_functions(rctx);
	r600_init_surface_functions(rctx);
	rctx->context.draw_vbo = r600_draw_vbo;

	switch (r600_get_family(rctx->radeon)) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV670:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		r600_init_state_functions(rctx);
		if (r600_context_init(&rctx->ctx, rctx->radeon)) {
			r600_destroy_context(&rctx->context);
			return NULL;
		}
		r600_init_config(rctx);
		break;
	case CHIP_CEDAR:
	case CHIP_REDWOOD:
	case CHIP_JUNIPER:
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
	case CHIP_PALM:
	case CHIP_BARTS:
	case CHIP_TURKS:
	case CHIP_CAICOS:
		evergreen_init_state_functions(rctx);
		if (evergreen_context_init(&rctx->ctx, rctx->radeon)) {
			r600_destroy_context(&rctx->context);
			return NULL;
		}
		evergreen_init_config(rctx);
		break;
	default:
		R600_ERR("unsupported family %d\n", r600_get_family(rctx->radeon));
		r600_destroy_context(&rctx->context);
		return NULL;
	}

	util_slab_create(&rctx->pool_transfers,
			 sizeof(struct pipe_transfer), 64,
			 UTIL_SLAB_SINGLETHREADED);

	rctx->vbuf_mgr = u_vbuf_mgr_create(&rctx->context, 1024 * 1024, 256,
					   PIPE_BIND_VERTEX_BUFFER |
					   PIPE_BIND_INDEX_BUFFER |
					   PIPE_BIND_CONSTANT_BUFFER,
					   U_VERTEX_FETCH_DWORD_ALIGNED);
	if (!rctx->vbuf_mgr) {
		r600_destroy_context(&rctx->context);
		return NULL;
	}

	rctx->blitter = util_blitter_create(&rctx->context);
	if (rctx->blitter == NULL) {
		r600_destroy_context(&rctx->context);
		return NULL;
	}

	class = r600_get_family_class(rctx->radeon);
	if (class == R600 || class == R700)
		rctx->custom_dsa_flush = r600_create_db_flush_dsa(rctx);
	else
		rctx->custom_dsa_flush = evergreen_create_db_flush_dsa(rctx);

	return &rctx->context;
}

/*
 * pipe_screen
 */
static const char* r600_get_vendor(struct pipe_screen* pscreen)
{
	return "X.Org";
}

static const char *r600_get_family_name(enum radeon_family family)
{
	switch(family) {
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
	case CHIP_BARTS: return "AMD BARTS";
	case CHIP_TURKS: return "AMD TURKS";
	case CHIP_CAICOS: return "AMD CAICOS";
	default: return "AMD unknown";
	}
}

static const char* r600_get_name(struct pipe_screen* pscreen)
{
	struct r600_screen *rscreen = (struct r600_screen *)pscreen;
	enum radeon_family family = r600_get_family(rscreen->radeon);

	return r600_get_family_name(family);
}

static int r600_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
	struct r600_screen *rscreen = (struct r600_screen *)pscreen;
	enum radeon_family family = r600_get_family(rscreen->radeon);

	switch (param) {
	/* Supported features (boolean caps). */
	case PIPE_CAP_NPOT_TEXTURES:
	case PIPE_CAP_TWO_SIDED_STENCIL:
	case PIPE_CAP_GLSL:
	case PIPE_CAP_DUAL_SOURCE_BLEND:
	case PIPE_CAP_ANISOTROPIC_FILTER:
	case PIPE_CAP_POINT_SPRITE:
	case PIPE_CAP_OCCLUSION_QUERY:
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
	case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
	case PIPE_CAP_SM3:
	case PIPE_CAP_TEXTURE_SWIZZLE:
	case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
	case PIPE_CAP_DEPTH_CLAMP:
	case PIPE_CAP_SHADER_STENCIL_EXPORT:
	case PIPE_CAP_TGSI_INSTANCEID:
	case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
	case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
		return 1;
	case PIPE_CAP_INDEP_BLEND_ENABLE:
		/* R600 doesn't support per-MRT blends */
		if (family == CHIP_R600)
			return 0;
		else
			return 1;

	/* Unsupported features (boolean caps). */
	case PIPE_CAP_STREAM_OUTPUT:
	case PIPE_CAP_PRIMITIVE_RESTART:
	case PIPE_CAP_INDEP_BLEND_FUNC: /* FIXME allow this */
	case PIPE_CAP_FRAGMENT_COLOR_CLAMP_CONTROL:
		/* R600 doesn't support per-MRT blends */
		if (family == CHIP_R600)
			return 0;
		else
			return 0;

	case PIPE_CAP_ARRAY_TEXTURES:
		/* fix once the CS checker upstream is fixed */
		return debug_get_bool_option("R600_ARRAY_TEXTURE", FALSE);

	/* Texturing. */
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		if (family >= CHIP_CEDAR)
			return 15;
		else
			return 14;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		/* FIXME allow this once infrastructure is there */
		return 16;
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
	case PIPE_CAP_MAX_COMBINED_SAMPLERS:
		return 16;

	/* Render targets. */
	case PIPE_CAP_MAX_RENDER_TARGETS:
		/* FIXME some r6xx are buggy and can only do 4 */
		return 8;

	/* Fragment coordinate conventions. */
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
		return 1;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
		return 0;

	/* Timer queries, present when the clock frequency is non zero. */
	case PIPE_CAP_TIMER_QUERY:
		return r600_get_clock_crystal_freq(rscreen->radeon) != 0;

	default:
		R600_ERR("r600: unknown param %d\n", param);
		return 0;
	}
}

static float r600_get_paramf(struct pipe_screen* pscreen, enum pipe_cap param)
{
	struct r600_screen *rscreen = (struct r600_screen *)pscreen;
	enum radeon_family family = r600_get_family(rscreen->radeon);

	switch (param) {
	case PIPE_CAP_MAX_LINE_WIDTH:
	case PIPE_CAP_MAX_LINE_WIDTH_AA:
	case PIPE_CAP_MAX_POINT_WIDTH:
	case PIPE_CAP_MAX_POINT_WIDTH_AA:
		if (family >= CHIP_CEDAR)
			return 16384.0f;
		else
			return 8192.0f;
	case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
		return 16.0f;
	case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
		return 16.0f;
	default:
		R600_ERR("r600: unsupported paramf %d\n", param);
		return 0.0f;
	}
}

static int r600_get_shader_param(struct pipe_screen* pscreen, unsigned shader, enum pipe_shader_cap param)
{
	switch(shader)
	{
	case PIPE_SHADER_FRAGMENT:
	case PIPE_SHADER_VERTEX:
		break;
	case PIPE_SHADER_GEOMETRY:
		/* TODO: support and enable geometry programs */
		return 0;
	default:
		/* TODO: support tessellation on Evergreen */
		return 0;
	}

	/* TODO: all these should be fixed, since r600 surely supports much more! */
	switch (param) {
	case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
		return 16384;
	case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
		return 8; /* FIXME */
	case PIPE_SHADER_CAP_MAX_INPUTS:
		if(shader == PIPE_SHADER_FRAGMENT)
			return 10;
		else
			return 16;
	case PIPE_SHADER_CAP_MAX_TEMPS:
		return 256; //max native temporaries
	case PIPE_SHADER_CAP_MAX_ADDRS:
		return 1; //max native address registers/* FIXME Isn't this equal to TEMPS? */
	case PIPE_SHADER_CAP_MAX_CONSTS:
		return R600_MAX_CONST_BUFFER_SIZE;
	case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
		return R600_MAX_CONST_BUFFERS;
	case PIPE_SHADER_CAP_MAX_PREDS:
		return 0; /* FIXME */
	case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		return 1;
	case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
		return 1;
	case PIPE_SHADER_CAP_SUBROUTINES:
		return 0;
	default:
		return 0;
	}
}

static boolean r600_is_format_supported(struct pipe_screen* screen,
					enum pipe_format format,
					enum pipe_texture_target target,
					unsigned sample_count,
                                        unsigned usage)
{
	unsigned retval = 0;
	if (target >= PIPE_MAX_TEXTURE_TYPES) {
		R600_ERR("r600: unsupported texture type %d\n", target);
		return FALSE;
	}

        if (!util_format_is_supported(format, usage))
                return FALSE;

	/* Multisample */
	if (sample_count > 1)
		return FALSE;

	if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
	    r600_is_sampler_format_supported(screen, format)) {
		retval |= PIPE_BIND_SAMPLER_VIEW;
	}

	if ((usage & (PIPE_BIND_RENDER_TARGET |
			PIPE_BIND_DISPLAY_TARGET |
			PIPE_BIND_SCANOUT |
			PIPE_BIND_SHARED)) &&
			r600_is_colorbuffer_format_supported(format)) {
		retval |= usage &
			(PIPE_BIND_RENDER_TARGET |
			 PIPE_BIND_DISPLAY_TARGET |
			 PIPE_BIND_SCANOUT |
			 PIPE_BIND_SHARED);
	}

	if ((usage & PIPE_BIND_DEPTH_STENCIL) &&
	    r600_is_zs_format_supported(format)) {
		retval |= PIPE_BIND_DEPTH_STENCIL;
	}

	if (usage & PIPE_BIND_VERTEX_BUFFER) {
		struct r600_screen *rscreen = (struct r600_screen *)screen;
		enum radeon_family family = r600_get_family(rscreen->radeon);

		if (r600_is_vertex_format_supported(format, family)) {
			retval |= PIPE_BIND_VERTEX_BUFFER;
		}
	}

	if (usage & PIPE_BIND_TRANSFER_READ)
		retval |= PIPE_BIND_TRANSFER_READ;
	if (usage & PIPE_BIND_TRANSFER_WRITE)
		retval |= PIPE_BIND_TRANSFER_WRITE;

	return retval == usage;
}

static void r600_destroy_screen(struct pipe_screen* pscreen)
{
	struct r600_screen *rscreen = (struct r600_screen *)pscreen;

	if (rscreen == NULL)
		return;

	radeon_decref(rscreen->radeon);

	util_slab_destroy(&rscreen->pool_buffers);
	pipe_mutex_destroy(rscreen->mutex_num_contexts);
	FREE(rscreen);
}

static void r600_fence_reference(struct pipe_screen *pscreen,
                                 struct pipe_fence_handle **ptr,
                                 struct pipe_fence_handle *fence)
{
	struct r600_fence **oldf = (struct r600_fence**)ptr;
	struct r600_fence *newf = (struct r600_fence*)fence;

	if (pipe_reference(&(*oldf)->reference, &newf->reference)) {
		struct r600_pipe_context *ctx = (*oldf)->ctx;
		LIST_ADDTAIL(&(*oldf)->head, &ctx->fences.pool);
	}

	*ptr = fence;
}

static boolean r600_fence_signalled(struct pipe_screen *pscreen,
                                    struct pipe_fence_handle *fence)
{
	struct r600_fence *rfence = (struct r600_fence*)fence;
	struct r600_pipe_context *ctx = rfence->ctx;

	return ctx->fences.data[rfence->index];
}

static boolean r600_fence_finish(struct pipe_screen *pscreen,
                                 struct pipe_fence_handle *fence,
                                 uint64_t timeout)
{
	struct r600_fence *rfence = (struct r600_fence*)fence;
	struct r600_pipe_context *ctx = rfence->ctx;
	int64_t start_time = 0;
	unsigned spins = 0;

	if (timeout != PIPE_TIMEOUT_INFINITE) {
		start_time = os_time_get();

		/* Convert to microseconds. */
		timeout /= 1000;
	}

	while (ctx->fences.data[rfence->index] == 0) {
		if (++spins % 256)
			continue;
#ifdef PIPE_OS_UNIX
		sched_yield();
#else
		os_time_sleep(10);
#endif
		if (timeout != PIPE_TIMEOUT_INFINITE &&
		    os_time_get() - start_time >= timeout) {
			return FALSE;
		}
	}

	return TRUE;
}

struct pipe_screen *r600_screen_create(struct radeon *radeon)
{
	struct r600_screen *rscreen;

	rscreen = CALLOC_STRUCT(r600_screen);
	if (rscreen == NULL) {
		return NULL;
	}

	rscreen->radeon = radeon;
	rscreen->screen.winsys = (struct pipe_winsys*)radeon;
	rscreen->screen.destroy = r600_destroy_screen;
	rscreen->screen.get_name = r600_get_name;
	rscreen->screen.get_vendor = r600_get_vendor;
	rscreen->screen.get_param = r600_get_param;
	rscreen->screen.get_shader_param = r600_get_shader_param;
	rscreen->screen.get_paramf = r600_get_paramf;
	rscreen->screen.is_format_supported = r600_is_format_supported;
	rscreen->screen.context_create = r600_create_context;
	rscreen->screen.fence_reference = r600_fence_reference;
	rscreen->screen.fence_signalled = r600_fence_signalled;
	rscreen->screen.fence_finish = r600_fence_finish;
	r600_init_screen_resource_functions(&rscreen->screen);

	rscreen->tiling_info = r600_get_tiling_info(radeon);
	util_format_s3tc_init();

	util_slab_create(&rscreen->pool_buffers,
			 sizeof(struct r600_resource_buffer), 64,
			 UTIL_SLAB_SINGLETHREADED);

	pipe_mutex_init(rscreen->mutex_num_contexts);

	return &rscreen->screen;
}
