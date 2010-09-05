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
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson
 */
#include <stdio.h>
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_public.h"
#include "r600_resource.h"
#include "r600_state_inlines.h"

static const char* r600_get_vendor(struct pipe_screen* pscreen)
{
	return "X.Org";
}

static const char* r600_get_name(struct pipe_screen* pscreen)
{
	struct r600_screen *screen = r600_screen(pscreen);
	enum radeon_family family = radeon_get_family(screen->rw);

	if (family >= CHIP_R600 && family < CHIP_RV770)
		return "R600 (HD2XXX,HD3XXX)";
	else if (family < CHIP_CEDAR)
		return "R700 (HD4XXX)";
	else
		return "EVERGREEN";
}

static int r600_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
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
		return 1;

	/* Unsupported features (boolean caps). */
	case PIPE_CAP_TIMER_QUERY:
	case PIPE_CAP_STREAM_OUTPUT:
	case PIPE_CAP_INDEP_BLEND_FUNC: /* FIXME allow this */
		return 0;

	/* Texturing. */
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 14;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		/* FIXME allow this once infrastructure is there */
		return 0;
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
                /* TODO: support this! */
                return 0;
        default:
                return 0;
        }
}

static float r600_get_paramf(struct pipe_screen* pscreen, enum pipe_cap param)
{
	switch (param) {
	case PIPE_CAP_MAX_LINE_WIDTH:
	case PIPE_CAP_MAX_LINE_WIDTH_AA:
	case PIPE_CAP_MAX_POINT_WIDTH:
	case PIPE_CAP_MAX_POINT_WIDTH_AA:
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
	struct r600_screen* rscreen = r600_screen(pscreen);

	if (rscreen == NULL)
		return;
	FREE(rscreen);
}

struct pipe_screen *r600_screen_create(struct radeon *rw)
{
	struct r600_screen* rscreen;
	enum radeon_family family = radeon_get_family(rw);

	rscreen = CALLOC_STRUCT(r600_screen);
	if (rscreen == NULL) {
		return NULL;
	}
	
	/* don't enable mem constant for r600 yet */
	rscreen->use_mem_constant = FALSE;

	switch (family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV670:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
		rscreen->chip_class = R600;
		break;
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		rscreen->chip_class = R700;
		break;
	case CHIP_CEDAR:
	case CHIP_REDWOOD:
	case CHIP_JUNIPER:
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		rscreen->chip_class = EVERGREEN;
		rscreen->use_mem_constant = TRUE;
		break;
	default:
		FREE(rscreen);
		return NULL;
	}
	rscreen->rw = rw;
	rscreen->screen.winsys = (struct pipe_winsys*)rw;
	rscreen->screen.destroy = r600_destroy_screen;
	rscreen->screen.get_name = r600_get_name;
	rscreen->screen.get_vendor = r600_get_vendor;
	rscreen->screen.get_param = r600_get_param;
	rscreen->screen.get_shader_param = r600_get_shader_param;
	rscreen->screen.get_paramf = r600_get_paramf;
	rscreen->screen.is_format_supported = r600_is_format_supported;
	rscreen->screen.context_create = r600_create_context;
	r600_init_screen_texture_functions(&rscreen->screen);
	r600_init_screen_resource_functions(rscreen);
	return &rscreen->screen;
}
