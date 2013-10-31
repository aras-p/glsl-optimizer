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
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "util/u_blitter.h"
#include "util/u_double_list.h"
#include "util/u_format.h"
#include "util/u_transfer.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_simple_shaders.h"
#include "util/u_upload_mgr.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"
#include "os/os_time.h"
#include "pipebuffer/pb_buffer.h"
#include "radeonsi_pipe.h"
#include "radeon/radeon_uvd.h"
#include "r600.h"
#include "sid.h"
#include "r600_resource.h"
#include "radeonsi_pipe.h"
#include "si_state.h"
#include "../radeon/r600_cs.h"

/*
 * pipe_context
 */
void radeonsi_flush(struct pipe_context *ctx, struct pipe_fence_handle **fence,
		    unsigned flags)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct pipe_query *render_cond = NULL;
	boolean render_cond_cond = FALSE;
	unsigned render_cond_mode = 0;

	if (fence) {
		*fence = rctx->b.ws->cs_create_fence(rctx->b.rings.gfx.cs);
	}

	/* Disable render condition. */
	if (rctx->current_render_cond) {
		render_cond = rctx->current_render_cond;
		render_cond_cond = rctx->current_render_cond_cond;
		render_cond_mode = rctx->current_render_cond_mode;
		ctx->render_condition(ctx, NULL, FALSE, 0);
	}

	si_context_flush(rctx, flags);

	/* Re-enable render condition. */
	if (render_cond) {
		ctx->render_condition(ctx, render_cond, render_cond_cond, render_cond_mode);
	}
}

static void r600_flush_from_st(struct pipe_context *ctx,
			       struct pipe_fence_handle **fence,
                               unsigned flags)
{
	radeonsi_flush(ctx, fence,
                       flags & PIPE_FLUSH_END_OF_FRAME ? RADEON_FLUSH_END_OF_FRAME : 0);
}

static void r600_flush_from_winsys(void *ctx, unsigned flags)
{
	radeonsi_flush((struct pipe_context*)ctx, NULL, flags);
}

static void r600_destroy_context(struct pipe_context *context)
{
	struct r600_context *rctx = (struct r600_context *)context;

	si_release_all_descriptors(rctx);

	pipe_resource_reference(&rctx->null_const_buf.buffer, NULL);
	r600_resource_reference(&rctx->border_color_table, NULL);

	if (rctx->dummy_pixel_shader) {
		rctx->b.b.delete_fs_state(&rctx->b.b, rctx->dummy_pixel_shader);
	}
	for (int i = 0; i < 8; i++) {
		rctx->b.b.delete_depth_stencil_alpha_state(&rctx->b.b, rctx->custom_dsa_flush_depth_stencil[i]);
		rctx->b.b.delete_depth_stencil_alpha_state(&rctx->b.b, rctx->custom_dsa_flush_depth[i]);
		rctx->b.b.delete_depth_stencil_alpha_state(&rctx->b.b, rctx->custom_dsa_flush_stencil[i]);
	}
	rctx->b.b.delete_depth_stencil_alpha_state(&rctx->b.b, rctx->custom_dsa_flush_inplace);
	rctx->b.b.delete_blend_state(&rctx->b.b, rctx->custom_blend_resolve);
	rctx->b.b.delete_blend_state(&rctx->b.b, rctx->custom_blend_decompress);
	util_unreference_framebuffer_state(&rctx->framebuffer);

	util_blitter_destroy(rctx->blitter);

	if (rctx->uploader) {
		u_upload_destroy(rctx->uploader);
	}
	util_slab_destroy(&rctx->pool_transfers);

	r600_common_context_cleanup(&rctx->b);
	FREE(rctx);
}

static struct pipe_context *r600_create_context(struct pipe_screen *screen, void *priv)
{
	struct r600_context *rctx = CALLOC_STRUCT(r600_context);
	struct r600_screen* rscreen = (struct r600_screen *)screen;
	int shader, i;

	if (rctx == NULL)
		return NULL;

	if (!r600_common_context_init(&rctx->b, &rscreen->b))
		goto fail;

	rctx->b.b.screen = screen;
	rctx->b.b.priv = priv;
	rctx->b.b.destroy = r600_destroy_context;
	rctx->b.b.flush = r600_flush_from_st;

	/* Easy accessing of screen/winsys. */
	rctx->screen = rscreen;

	si_init_blit_functions(rctx);
	r600_init_query_functions(rctx);
	r600_init_context_resource_functions(rctx);
	si_init_compute_functions(rctx);

	if (rscreen->b.info.has_uvd) {
		rctx->b.b.create_video_codec = radeonsi_uvd_create_decoder;
		rctx->b.b.create_video_buffer = radeonsi_video_buffer_create;
	} else {
		rctx->b.b.create_video_codec = vl_create_decoder;
		rctx->b.b.create_video_buffer = vl_video_buffer_create;
	}

	rctx->b.rings.gfx.cs = rctx->b.ws->cs_create(rctx->b.ws, RING_GFX, NULL);
	rctx->b.rings.gfx.flush = r600_flush_from_winsys;

	si_init_all_descriptors(rctx);

	/* Initialize cache_flush. */
	rctx->cache_flush = si_atom_cache_flush;
	rctx->atoms.cache_flush = &rctx->cache_flush;

	rctx->atoms.streamout_begin = &rctx->b.streamout.begin_atom;

	switch (rctx->b.chip_class) {
	case SI:
	case CIK:
		si_init_state_functions(rctx);
		LIST_INITHEAD(&rctx->active_nontimer_query_list);
		rctx->max_db = 8;
		si_init_config(rctx);
		break;
	default:
		R600_ERR("Unsupported chip class %d.\n", rctx->b.chip_class);
		goto fail;
	}

	rctx->b.ws->cs_set_flush_callback(rctx->b.rings.gfx.cs, r600_flush_from_winsys, rctx);

	util_slab_create(&rctx->pool_transfers,
			 sizeof(struct pipe_transfer), 64,
			 UTIL_SLAB_SINGLETHREADED);

        rctx->uploader = u_upload_create(&rctx->b.b, 1024 * 1024, 256,
                                         PIPE_BIND_INDEX_BUFFER |
                                         PIPE_BIND_CONSTANT_BUFFER);
        if (!rctx->uploader)
		goto fail;

	rctx->blitter = util_blitter_create(&rctx->b.b);
	if (rctx->blitter == NULL)
		goto fail;

	rctx->dummy_pixel_shader =
		util_make_fragment_cloneinput_shader(&rctx->b.b, 0,
						     TGSI_SEMANTIC_GENERIC,
						     TGSI_INTERPOLATE_CONSTANT);
	rctx->b.b.bind_fs_state(&rctx->b.b, rctx->dummy_pixel_shader);

	/* these must be last */
	si_begin_new_cs(rctx);
	si_get_backend_mask(rctx);

	/* CIK cannot unbind a constant buffer (S_BUFFER_LOAD is buggy
	 * with a NULL buffer). We need to use a dummy buffer instead. */
	if (rctx->b.chip_class == CIK) {
		rctx->null_const_buf.buffer = pipe_buffer_create(screen, PIPE_BIND_CONSTANT_BUFFER,
								 PIPE_USAGE_STATIC, 16);
		rctx->null_const_buf.buffer_size = rctx->null_const_buf.buffer->width0;

		for (shader = 0; shader < SI_NUM_SHADERS; shader++) {
			for (i = 0; i < NUM_CONST_BUFFERS; i++) {
				rctx->b.b.set_constant_buffer(&rctx->b.b, shader, i,
							      &rctx->null_const_buf);
			}
		}

		/* Clear the NULL constant buffer, because loads should return zeros. */
		rctx->b.clear_buffer(&rctx->b.b, rctx->null_const_buf.buffer, 0,
				     rctx->null_const_buf.buffer->width0, 0);
	}

	return &rctx->b.b;
fail:
	r600_destroy_context(&rctx->b.b);
	return NULL;
}

/*
 * pipe_screen
 */
static const char* r600_get_vendor(struct pipe_screen* pscreen)
{
	return "X.Org";
}

const char *r600_get_llvm_processor_name(enum radeon_family family)
{
	switch (family) {
		case CHIP_TAHITI: return "tahiti";
		case CHIP_PITCAIRN: return "pitcairn";
		case CHIP_VERDE: return "verde";
		case CHIP_OLAND: return "oland";
#if HAVE_LLVM <= 0x0303
		default: return "SI";
#else
		case CHIP_HAINAN: return "hainan";
		case CHIP_BONAIRE: return "bonaire";
		case CHIP_KABINI: return "kabini";
		case CHIP_KAVERI: return "kaveri";
		default: return "";
#endif
	}
}

static const char *r600_get_family_name(enum radeon_family family)
{
	switch(family) {
	case CHIP_TAHITI: return "AMD TAHITI";
	case CHIP_PITCAIRN: return "AMD PITCAIRN";
	case CHIP_VERDE: return "AMD CAPE VERDE";
	case CHIP_OLAND: return "AMD OLAND";
	case CHIP_HAINAN: return "AMD HAINAN";
	case CHIP_BONAIRE: return "AMD BONAIRE";
	case CHIP_KAVERI: return "AMD KAVERI";
	case CHIP_KABINI: return "AMD KABINI";
	default: return "AMD unknown";
	}
}

static const char* r600_get_name(struct pipe_screen* pscreen)
{
	struct r600_screen *rscreen = (struct r600_screen *)pscreen;

	return r600_get_family_name(rscreen->b.family);
}

static int r600_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
	struct r600_screen *rscreen = (struct r600_screen *)pscreen;
	bool has_streamout = HAVE_LLVM >= 0x0304;

	switch (param) {
	/* Supported features (boolean caps). */
	case PIPE_CAP_TWO_SIDED_STENCIL:
	case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
	case PIPE_CAP_ANISOTROPIC_FILTER:
	case PIPE_CAP_POINT_SPRITE:
	case PIPE_CAP_OCCLUSION_QUERY:
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
	case PIPE_CAP_TEXTURE_SWIZZLE:
	case PIPE_CAP_DEPTH_CLIP_DISABLE:
	case PIPE_CAP_SHADER_STENCIL_EXPORT:
	case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
	case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
	case PIPE_CAP_SM3:
	case PIPE_CAP_SEAMLESS_CUBE_MAP:
	case PIPE_CAP_PRIMITIVE_RESTART:
	case PIPE_CAP_CONDITIONAL_RENDER:
	case PIPE_CAP_TEXTURE_BARRIER:
	case PIPE_CAP_INDEP_BLEND_ENABLE:
	case PIPE_CAP_INDEP_BLEND_FUNC:
	case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
	case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
	case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_USER_INDEX_BUFFERS:
	case PIPE_CAP_USER_CONSTANT_BUFFERS:
	case PIPE_CAP_START_INSTANCE:
	case PIPE_CAP_NPOT_TEXTURES:
	case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
        case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
	case PIPE_CAP_TGSI_INSTANCEID:
	case PIPE_CAP_COMPUTE:
	case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
		return 1;

	case PIPE_CAP_TEXTURE_MULTISAMPLE:
		return HAVE_LLVM >= 0x0304 && rscreen->b.chip_class == SI;

	case PIPE_CAP_TGSI_TEXCOORD:
		return 0;

        case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
                return 64;

	case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
		return 256;

	case PIPE_CAP_GLSL_FEATURE_LEVEL:
		return 140;

	case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
		return 1;
	case PIPE_CAP_MAX_TEXTURE_BUFFER_SIZE:
		return MIN2(rscreen->b.info.vram_size, 0xFFFFFFFF);

	/* Unsupported features. */
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
	case PIPE_CAP_SCALED_RESOLVE:
	case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
	case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
	case PIPE_CAP_VERTEX_COLOR_CLAMPED:
	case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
	case PIPE_CAP_USER_VERTEX_BUFFERS:
	case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
	case PIPE_CAP_CUBE_MAP_ARRAY:
		return 0;

	case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
		return PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_R600;

	/* Stream output. */
	case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
		return has_streamout ? 4 : 0;
	case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
		return has_streamout ? 1 : 0;
	case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
	case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
		return has_streamout ? 32*4 : 0;

	/* Texturing. */
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
			return 15;
	case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
		return 16384;
	case PIPE_CAP_MAX_COMBINED_SAMPLERS:
		return 32;

	/* Render targets. */
	case PIPE_CAP_MAX_RENDER_TARGETS:
		/* FIXME some r6xx are buggy and can only do 4 */
		return 8;

	case PIPE_CAP_MAX_VIEWPORTS:
		return 1;

	/* Timer queries, present when the clock frequency is non zero. */
	case PIPE_CAP_QUERY_TIMESTAMP:
	case PIPE_CAP_QUERY_TIME_ELAPSED:
		return rscreen->b.info.r600_clock_crystal_freq != 0;

	case PIPE_CAP_MIN_TEXEL_OFFSET:
		return -8;

	case PIPE_CAP_MAX_TEXEL_OFFSET:
		return 7;
	case PIPE_CAP_ENDIANNESS:
		return PIPE_ENDIAN_LITTLE;
	}
	return 0;
}

static float r600_get_paramf(struct pipe_screen* pscreen,
			     enum pipe_capf param)
{
	switch (param) {
	case PIPE_CAPF_MAX_LINE_WIDTH:
	case PIPE_CAPF_MAX_LINE_WIDTH_AA:
	case PIPE_CAPF_MAX_POINT_WIDTH:
	case PIPE_CAPF_MAX_POINT_WIDTH_AA:
		return 16384.0f;
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
	case PIPE_SHADER_COMPUTE:
		switch (param) {
		case PIPE_SHADER_CAP_PREFERRED_IR:
			return PIPE_SHADER_IR_LLVM;
		default:
			return 0;
		}
	default:
		/* TODO: support tessellation */
		return 0;
	}

	switch (param) {
	case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
		return 16384;
	case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
		return 32;
	case PIPE_SHADER_CAP_MAX_INPUTS:
		return 32;
	case PIPE_SHADER_CAP_MAX_TEMPS:
		return 256; /* Max native temporaries. */
	case PIPE_SHADER_CAP_MAX_ADDRS:
		/* FIXME Isn't this equal to TEMPS? */
		return 1; /* Max native address registers */
	case PIPE_SHADER_CAP_MAX_CONSTS:
		return 4096; /* actually only memory limits this */
	case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
		return NUM_PIPE_CONST_BUFFERS;
	case PIPE_SHADER_CAP_MAX_PREDS:
		return 0; /* FIXME */
	case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		return 1;
	case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
		return 0;
	case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
		return 1;
	case PIPE_SHADER_CAP_INTEGERS:
		return 1;
	case PIPE_SHADER_CAP_SUBROUTINES:
		return 0;
	case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
		return 16;
	case PIPE_SHADER_CAP_PREFERRED_IR:
		return PIPE_SHADER_IR_TGSI;
	}
	return 0;
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
	case PIPE_VIDEO_CAP_MAX_LEVEL:
		return vl_level_supported(screen, profile);
	default:
		return 0;
	}
}

static int r600_get_compute_param(struct pipe_screen *screen,
        enum pipe_compute_cap param,
        void *ret)
{
	struct r600_screen *rscreen = (struct r600_screen *)screen;
	//TODO: select these params by asic
	switch (param) {
	case PIPE_COMPUTE_CAP_IR_TARGET: {
		const char *gpu = r600_get_llvm_processor_name(rscreen->b.family);
	        if (ret) {
			sprintf(ret, "%s-r600--", gpu);
		}
		return (8 + strlen(gpu)) * sizeof(char);
	}
	case PIPE_COMPUTE_CAP_GRID_DIMENSION:
		if (ret) {
			uint64_t * grid_dimension = ret;
			grid_dimension[0] = 3;
		}
		return 1 * sizeof(uint64_t);
	case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
		if (ret) {
			uint64_t * grid_size = ret;
			grid_size[0] = 65535;
			grid_size[1] = 65535;
			grid_size[2] = 1;
		}
		return 3 * sizeof(uint64_t) ;

	case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
		if (ret) {
			uint64_t * block_size = ret;
			block_size[0] = 256;
			block_size[1] = 256;
			block_size[2] = 256;
		}
		return 3 * sizeof(uint64_t);
	case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
		if (ret) {
			uint64_t * max_threads_per_block = ret;
			*max_threads_per_block = 256;
		}
		return sizeof(uint64_t);

	case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
		if (ret) {
			uint64_t *max_global_size = ret;
			/* XXX: Not sure what to put here. */
			*max_global_size = 2000000000;
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
			r600_get_compute_param(screen, PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE, &max_global_size);
			*max_mem_alloc_size = max_global_size / 4;
		}
		return sizeof(uint64_t);
	default:
		fprintf(stderr, "unknown PIPE_COMPUTE_CAP %d\n", param);
		return 0;
	}
}

static void r600_destroy_screen(struct pipe_screen* pscreen)
{
	struct r600_screen *rscreen = (struct r600_screen *)pscreen;

	if (rscreen == NULL)
		return;

	if (!radeon_winsys_unref(rscreen->b.ws))
		return;

	r600_common_screen_cleanup(&rscreen->b);

#if R600_TRACE_CS
	if (rscreen->trace_bo) {
		rscreen->ws->buffer_unmap(rscreen->trace_bo->cs_buf);
		pipe_resource_reference((struct pipe_resource**)&rscreen->trace_bo, NULL);
	}
#endif

	rscreen->b.ws->destroy(rscreen->b.ws);
	FREE(rscreen);
}

static uint64_t r600_get_timestamp(struct pipe_screen *screen)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;

	return 1000000 * rscreen->b.ws->query_value(rscreen->b.ws, RADEON_TIMESTAMP) /
		rscreen->b.info.r600_clock_crystal_freq;
}

struct pipe_screen *radeonsi_screen_create(struct radeon_winsys *ws)
{
	struct r600_screen *rscreen = CALLOC_STRUCT(r600_screen);
	if (rscreen == NULL) {
		return NULL;
	}

	ws->query_info(ws, &rscreen->b.info);

	/* Set functions first. */
	rscreen->b.b.context_create = r600_create_context;
	rscreen->b.b.destroy = r600_destroy_screen;
	rscreen->b.b.get_name = r600_get_name;
	rscreen->b.b.get_vendor = r600_get_vendor;
	rscreen->b.b.get_param = r600_get_param;
	rscreen->b.b.get_shader_param = r600_get_shader_param;
	rscreen->b.b.get_paramf = r600_get_paramf;
	rscreen->b.b.get_compute_param = r600_get_compute_param;
	rscreen->b.b.get_timestamp = r600_get_timestamp;
	rscreen->b.b.is_format_supported = si_is_format_supported;
	if (rscreen->b.info.has_uvd) {
		rscreen->b.b.get_video_param = ruvd_get_video_param;
		rscreen->b.b.is_video_format_supported = ruvd_is_format_supported;
	} else {
		rscreen->b.b.get_video_param = r600_get_video_param;
		rscreen->b.b.is_video_format_supported = vl_video_buffer_is_format_supported;
	}
	r600_init_screen_resource_functions(&rscreen->b.b);

	if (!r600_common_screen_init(&rscreen->b, ws)) {
		FREE(rscreen);
		return NULL;
	}

	if (debug_get_bool_option("RADEON_DUMP_SHADERS", FALSE))
		rscreen->b.debug_flags |= DBG_FS | DBG_VS | DBG_GS | DBG_PS | DBG_CS;

#if R600_TRACE_CS
	rscreen->cs_count = 0;
	if (rscreen->info.drm_minor >= 28) {
		rscreen->trace_bo = (struct r600_resource*)pipe_buffer_create(&rscreen->screen,
										PIPE_BIND_CUSTOM,
										PIPE_USAGE_STAGING,
										4096);
		if (rscreen->trace_bo) {
			rscreen->trace_ptr = rscreen->ws->buffer_map(rscreen->trace_bo->cs_buf, NULL,
									PIPE_TRANSFER_UNSYNCHRONIZED);
		}
	}
#endif

	/* Create the auxiliary context. This must be done last. */
	rscreen->b.aux_context = rscreen->b.b.context_create(&rscreen->b.b, NULL);

	return &rscreen->b.b;
}
