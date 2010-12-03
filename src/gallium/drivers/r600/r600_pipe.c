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
#include <util/u_transfer.h>
#include <util/u_surface.h>
#include <util/u_pack_color.h>
#include <util/u_memory.h>
#include <util/u_inlines.h>
#include <util/u_upload_mgr.h>
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
static void r600_flush(struct pipe_context *ctx, unsigned flags,
			struct pipe_fence_handle **fence)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
#if 0
	static int dc = 0;
	char dname[256];
#endif

	if (!rctx->ctx.pm4_cdwords)
		return;

	u_upload_flush(rctx->upload_vb);
	u_upload_flush(rctx->upload_ib);

#if 0
	sprintf(dname, "gallium-%08d.bof", dc);
	if (dc < 20) {
		r600_context_dump_bof(&rctx->ctx, dname);
		R600_ERR("dumped %s\n", dname);
	}
	dc++;
#endif
	r600_context_flush(&rctx->ctx);
}

static void r600_destroy_context(struct pipe_context *context)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)context;

	rctx->context.delete_depth_stencil_alpha_state(&rctx->context, rctx->custom_dsa_flush);

	r600_context_fini(&rctx->ctx);

	util_blitter_destroy(rctx->blitter);

	for (int i = 0; i < R600_PIPE_NSTATES; i++) {
		free(rctx->states[i]);
	}

	u_upload_destroy(rctx->upload_vb);
	u_upload_destroy(rctx->upload_ib);

	if (rctx->tran.translate_cache)
		translate_cache_destroy(rctx->tran.translate_cache);

	FREE(rctx->ps_resource);
	FREE(rctx->vs_resource);
	FREE(rctx);
}

static struct pipe_context *r600_create_context(struct pipe_screen *screen, void *priv)
{
	struct r600_pipe_context *rctx = CALLOC_STRUCT(r600_pipe_context);
	struct r600_screen* rscreen = (struct r600_screen *)screen;
	enum chip_class class;

	if (rctx == NULL)
		return NULL;
	rctx->context.winsys = rscreen->screen.winsys;
	rctx->context.screen = screen;
	rctx->context.priv = priv;
	rctx->context.destroy = r600_destroy_context;
	rctx->context.flush = r600_flush;

	/* Easy accessing of screen/winsys. */
	rctx->screen = rscreen;
	rctx->radeon = rscreen->radeon;
	rctx->family = r600_get_family(rctx->radeon);

	r600_init_blit_functions(rctx);
	r600_init_query_functions(rctx);
	r600_init_context_resource_functions(rctx);
	r600_init_surface_functions(rctx);

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
		rctx->context.draw_vbo = r600_draw_vbo;
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
		rctx->context.draw_vbo = evergreen_draw;
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

	rctx->upload_ib = u_upload_create(&rctx->context, 32 * 1024, 16,
					  PIPE_BIND_INDEX_BUFFER);
	if (rctx->upload_ib == NULL) {
		r600_destroy_context(&rctx->context);
		return NULL;
	}

	rctx->upload_vb = u_upload_create(&rctx->context, 128 * 1024, 16,
					  PIPE_BIND_VERTEX_BUFFER);
	if (rctx->upload_vb == NULL) {
		r600_destroy_context(&rctx->context);
		return NULL;
	}

	rctx->blitter = util_blitter_create(&rctx->context);
	if (rctx->blitter == NULL) {
		FREE(rctx);
		return NULL;
	}

	rctx->tran.translate_cache = translate_cache_create();
	if (rctx->tran.translate_cache == NULL) {
		FREE(rctx);
		return NULL;
	}

	rctx->vs_resource = CALLOC(R600_RESOURCE_ARRAY_SIZE, sizeof(struct r600_pipe_state));
	if (!rctx->vs_resource) {
		FREE(rctx);
		return NULL;
	}

	rctx->ps_resource = CALLOC(R600_RESOURCE_ARRAY_SIZE, sizeof(struct r600_pipe_state));
	if (!rctx->ps_resource) {
		FREE(rctx);
		return NULL;
	}

	class = r600_get_family_class(rctx->radeon);
	if (class == R600 || class == R700)
		rctx->custom_dsa_flush = r600_create_db_flush_dsa(rctx);
	else
		rctx->custom_dsa_flush = evergreen_create_db_flush_dsa(rctx);

	r600_blit_uncompress_depth_ptr = r600_blit_uncompress_depth;

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
	case PIPE_CAP_INDEP_BLEND_ENABLE:
	case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
	case PIPE_CAP_DEPTH_CLAMP:
	case PIPE_CAP_SHADER_STENCIL_EXPORT:
		return 1;

	/* Unsupported features (boolean caps). */
	case PIPE_CAP_TIMER_QUERY:
	case PIPE_CAP_STREAM_OUTPUT:
	case PIPE_CAP_PRIMITIVE_RESTART:
	case PIPE_CAP_INDEP_BLEND_FUNC: /* FIXME allow this */
		return 0;

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
		return 256; //max native parameters
	case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
		return 1;
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
					unsigned usage,
					unsigned geom_flags)
{
	unsigned retval = 0;
	if (target >= PIPE_MAX_TEXTURE_TYPES) {
		R600_ERR("r600: unsupported texture type %d\n", target);
		return FALSE;
	}

	/* Multisample */
	if (sample_count > 1)
		return FALSE;

	if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
	    r600_is_sampler_format_supported(format)) {
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

	if ((usage & PIPE_BIND_VERTEX_BUFFER) &&
	    r600_is_vertex_format_supported(format))
		retval |= PIPE_BIND_VERTEX_BUFFER;

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

	FREE(rscreen);
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
	r600_init_screen_resource_functions(&rscreen->screen);

	rscreen->tiling_info = r600_get_tiling_info(radeon);

	return &rscreen->screen;
}
