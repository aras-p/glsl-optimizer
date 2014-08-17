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

#include "si_pipe.h"
#include "si_public.h"
#include "sid.h"

#include "radeon/radeon_uvd.h"
#include "util/u_memory.h"
#include "util/u_simple_shaders.h"
#include "vl/vl_decoder.h"

/*
 * pipe_context
 */
static void si_destroy_context(struct pipe_context *context)
{
	struct si_context *sctx = (struct si_context *)context;

	si_release_all_descriptors(sctx);

	pipe_resource_reference(&sctx->null_const_buf.buffer, NULL);
	r600_resource_reference(&sctx->border_color_table, NULL);

	si_pm4_delete_state(sctx, gs_rings, sctx->gs_rings);
	si_pm4_delete_state(sctx, gs_onoff, sctx->gs_on);
	si_pm4_delete_state(sctx, gs_onoff, sctx->gs_off);

	if (sctx->dummy_pixel_shader) {
		sctx->b.b.delete_fs_state(&sctx->b.b, sctx->dummy_pixel_shader);
	}
	for (int i = 0; i < 8; i++) {
		sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_depth_stencil[i]);
		sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_depth[i]);
		sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_stencil[i]);
	}
	sctx->b.b.delete_depth_stencil_alpha_state(&sctx->b.b, sctx->custom_dsa_flush_inplace);
	sctx->b.b.delete_blend_state(&sctx->b.b, sctx->custom_blend_resolve);
	sctx->b.b.delete_blend_state(&sctx->b.b, sctx->custom_blend_decompress);
	sctx->b.b.delete_blend_state(&sctx->b.b, sctx->custom_blend_fastclear);
	util_unreference_framebuffer_state(&sctx->framebuffer.state);

	util_blitter_destroy(sctx->blitter);

	si_pm4_cleanup(sctx);

	r600_common_context_cleanup(&sctx->b);
	FREE(sctx);
}

static struct pipe_context *si_create_context(struct pipe_screen *screen, void *priv)
{
	struct si_context *sctx = CALLOC_STRUCT(si_context);
	struct si_screen* sscreen = (struct si_screen *)screen;
	struct radeon_winsys *ws = sscreen->b.ws;
	int shader, i;

	if (sctx == NULL)
		return NULL;

	sctx->b.b.screen = screen; /* this must be set first */
	sctx->b.b.priv = priv;
	sctx->b.b.destroy = si_destroy_context;
	sctx->screen = sscreen; /* Easy accessing of screen/winsys. */

	if (!r600_common_context_init(&sctx->b, &sscreen->b))
		goto fail;

	si_init_blit_functions(sctx);
	si_init_compute_functions(sctx);

	if (sscreen->b.info.has_uvd) {
		sctx->b.b.create_video_codec = si_uvd_create_decoder;
		sctx->b.b.create_video_buffer = si_video_buffer_create;
	} else {
		sctx->b.b.create_video_codec = vl_create_decoder;
		sctx->b.b.create_video_buffer = vl_video_buffer_create;
	}

	sctx->b.rings.gfx.cs = ws->cs_create(ws, RING_GFX, si_context_gfx_flush,
					     sctx, NULL);
	sctx->b.rings.gfx.flush = si_context_gfx_flush;

	si_init_all_descriptors(sctx);

	/* Initialize cache_flush. */
	sctx->cache_flush = si_atom_cache_flush;
	sctx->atoms.s.cache_flush = &sctx->cache_flush;

	sctx->msaa_config = si_atom_msaa_config;
	sctx->atoms.s.msaa_config = &sctx->msaa_config;

	sctx->atoms.s.streamout_begin = &sctx->b.streamout.begin_atom;
	sctx->atoms.s.streamout_enable = &sctx->b.streamout.enable_atom;

	switch (sctx->b.chip_class) {
	case SI:
	case CIK:
		si_init_state_functions(sctx);
		si_init_config(sctx);
		break;
	default:
		R600_ERR("Unsupported chip class %d.\n", sctx->b.chip_class);
		goto fail;
	}

	sctx->blitter = util_blitter_create(&sctx->b.b);
	if (sctx->blitter == NULL)
		goto fail;
	sctx->blitter->draw_rectangle = r600_draw_rectangle;

	sctx->dummy_pixel_shader =
		util_make_fragment_cloneinput_shader(&sctx->b.b, 0,
						     TGSI_SEMANTIC_GENERIC,
						     TGSI_INTERPOLATE_CONSTANT);
	sctx->b.b.bind_fs_state(&sctx->b.b, sctx->dummy_pixel_shader);

	/* these must be last */
	si_begin_new_cs(sctx);
	r600_query_init_backend_mask(&sctx->b); /* this emits commands and must be last */

	/* CIK cannot unbind a constant buffer (S_BUFFER_LOAD is buggy
	 * with a NULL buffer). We need to use a dummy buffer instead. */
	if (sctx->b.chip_class == CIK) {
		sctx->null_const_buf.buffer = pipe_buffer_create(screen, PIPE_BIND_CONSTANT_BUFFER,
								 PIPE_USAGE_DEFAULT, 16);
		sctx->null_const_buf.buffer_size = sctx->null_const_buf.buffer->width0;

		for (shader = 0; shader < SI_NUM_SHADERS; shader++) {
			for (i = 0; i < SI_NUM_CONST_BUFFERS; i++) {
				sctx->b.b.set_constant_buffer(&sctx->b.b, shader, i,
							      &sctx->null_const_buf);
			}
		}

		/* Clear the NULL constant buffer, because loads should return zeros. */
		sctx->b.clear_buffer(&sctx->b.b, sctx->null_const_buf.buffer, 0,
				     sctx->null_const_buf.buffer->width0, 0);
	}

	return &sctx->b.b;
fail:
	si_destroy_context(&sctx->b.b);
	return NULL;
}

/*
 * pipe_screen
 */

static int si_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
	struct si_screen *sscreen = (struct si_screen *)pscreen;

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
        case PIPE_CAP_TGSI_VS_LAYER_VIEWPORT:
	case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
	case PIPE_CAP_BUFFER_MAP_PERSISTENT_COHERENT:
	case PIPE_CAP_CUBE_MAP_ARRAY:
	case PIPE_CAP_SAMPLE_SHADING:
	case PIPE_CAP_DRAW_INDIRECT:
		return 1;

	case PIPE_CAP_TEXTURE_MULTISAMPLE:
		/* 2D tiling on CIK is supported since DRM 2.35.0 */
		return sscreen->b.chip_class < CIK ||
		       sscreen->b.info.drm_minor >= 35;

        case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
                return R600_MAP_BUFFER_ALIGNMENT;

	case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
	case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
		return 4;

	case PIPE_CAP_GLSL_FEATURE_LEVEL:
		return 330;

	case PIPE_CAP_MAX_TEXTURE_BUFFER_SIZE:
		return MIN2(sscreen->b.info.vram_size, 0xFFFFFFFF);

	case PIPE_CAP_TEXTURE_QUERY_LOD:
	case PIPE_CAP_TEXTURE_GATHER_SM5:
		return HAVE_LLVM >= 0x0305;
	case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
		return HAVE_LLVM >= 0x0305 ? 4 : 0;

	/* Unsupported features. */
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
	case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
	case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
	case PIPE_CAP_VERTEX_COLOR_CLAMPED:
	case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
	case PIPE_CAP_USER_VERTEX_BUFFERS:
	case PIPE_CAP_TGSI_TEXCOORD:
	case PIPE_CAP_FAKE_SW_MSAA:
	case PIPE_CAP_TEXTURE_GATHER_OFFSETS:
	case PIPE_CAP_TGSI_VS_WINDOW_SPACE_POSITION:
	case PIPE_CAP_TGSI_FS_FINE_DERIVATIVE:
		return 0;

	case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
		return PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_R600;

	/* Stream output. */
	case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
		return sscreen->b.has_streamout ? 4 : 0;
	case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
		return sscreen->b.has_streamout ? 1 : 0;
	case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
	case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
		return sscreen->b.has_streamout ? 32*4 : 0;

	/* Geometry shader output. */
	case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
		return 1024;
	case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
		return 4095;
	case PIPE_CAP_MAX_VERTEX_STREAMS:
		return 1;

	/* Texturing. */
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 15; /* 16384 */
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		/* textures support 8192, but layered rendering supports 2048 */
		return 12;
	case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
		/* textures support 8192, but layered rendering supports 2048 */
		return 2048;

	/* Render targets. */
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 8;

	case PIPE_CAP_MAX_VIEWPORTS:
		return 1;

	/* Timer queries, present when the clock frequency is non zero. */
	case PIPE_CAP_QUERY_TIMESTAMP:
	case PIPE_CAP_QUERY_TIME_ELAPSED:
		return sscreen->b.info.r600_clock_crystal_freq != 0;

 	case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
	case PIPE_CAP_MIN_TEXEL_OFFSET:
		return -32;

 	case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
	case PIPE_CAP_MAX_TEXEL_OFFSET:
		return 31;

	case PIPE_CAP_ENDIANNESS:
		return PIPE_ENDIAN_LITTLE;

	case PIPE_CAP_VENDOR_ID:
		return 0x1002;
	case PIPE_CAP_DEVICE_ID:
		return sscreen->b.info.pci_id;
	case PIPE_CAP_ACCELERATED:
		return 1;
	case PIPE_CAP_VIDEO_MEMORY:
		return sscreen->b.info.vram_size >> 20;
	case PIPE_CAP_UMA:
		return 0;
	}
	return 0;
}

static int si_get_shader_param(struct pipe_screen* pscreen, unsigned shader, enum pipe_shader_cap param)
{
	switch(shader)
	{
	case PIPE_SHADER_FRAGMENT:
	case PIPE_SHADER_VERTEX:
	case PIPE_SHADER_GEOMETRY:
		break;
	case PIPE_SHADER_COMPUTE:
		switch (param) {
		case PIPE_SHADER_CAP_PREFERRED_IR:
			return PIPE_SHADER_IR_LLVM;
		case PIPE_SHADER_CAP_DOUBLES:
			return 0; /* XXX: Enable doubles once the compiler can
			             handle them. */
		case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE: {
			uint64_t max_const_buffer_size;
			pscreen->get_compute_param(pscreen,
				PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE,
				&max_const_buffer_size);
			return max_const_buffer_size;
		}
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
		return shader == PIPE_SHADER_VERTEX ? SI_NUM_VERTEX_BUFFERS : 32;
	case PIPE_SHADER_CAP_MAX_TEMPS:
		return 256; /* Max native temporaries. */
	case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
		return 4096 * sizeof(float[4]); /* actually only memory limits this */
	case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
		return SI_NUM_USER_CONST_BUFFERS;
	case PIPE_SHADER_CAP_MAX_PREDS:
		return 0; /* FIXME */
	case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		return 1;
	case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
		return 0;
	case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
		/* Indirection of geometry shader input dimension is not
		 * handled yet
		 */
		return shader < PIPE_SHADER_GEOMETRY;
	case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
		return 1;
	case PIPE_SHADER_CAP_INTEGERS:
		return 1;
	case PIPE_SHADER_CAP_SUBROUTINES:
		return 0;
	case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
	case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
		return 16;
	case PIPE_SHADER_CAP_PREFERRED_IR:
		return PIPE_SHADER_IR_TGSI;
	case PIPE_SHADER_CAP_DOUBLES:
		return 0;
	}
	return 0;
}

static void si_destroy_screen(struct pipe_screen* pscreen)
{
	struct si_screen *sscreen = (struct si_screen *)pscreen;

	if (sscreen == NULL)
		return;

	if (!sscreen->b.ws->unref(sscreen->b.ws))
		return;

	r600_destroy_common_screen(&sscreen->b);
}

#define SI_TILE_MODE_COLOR_2D_8BPP  14

/* Initialize pipe config. This is especially important for GPUs
 * with 16 pipes and more where it's initialized incorrectly by
 * the TILING_CONFIG ioctl. */
static bool si_initialize_pipe_config(struct si_screen *sscreen)
{
	unsigned mode2d;

	/* This is okay, because there can be no 2D tiling without
	 * the tile mode array, so we won't need the pipe config.
	 * Return "success".
	 */
	if (!sscreen->b.info.si_tile_mode_array_valid)
		return true;

	/* The same index is used for the 2D mode on CIK too. */
	mode2d = sscreen->b.info.si_tile_mode_array[SI_TILE_MODE_COLOR_2D_8BPP];

	switch (G_009910_PIPE_CONFIG(mode2d)) {
	case V_02803C_ADDR_SURF_P2:
		sscreen->b.tiling_info.num_channels = 2;
		break;
	case V_02803C_X_ADDR_SURF_P4_8X16:
	case V_02803C_X_ADDR_SURF_P4_16X16:
	case V_02803C_X_ADDR_SURF_P4_16X32:
	case V_02803C_X_ADDR_SURF_P4_32X32:
		sscreen->b.tiling_info.num_channels = 4;
		break;
	case V_02803C_X_ADDR_SURF_P8_16X16_8X16:
	case V_02803C_X_ADDR_SURF_P8_16X32_8X16:
	case V_02803C_X_ADDR_SURF_P8_32X32_8X16:
	case V_02803C_X_ADDR_SURF_P8_16X32_16X16:
	case V_02803C_X_ADDR_SURF_P8_32X32_16X16:
	case V_02803C_X_ADDR_SURF_P8_32X32_16X32:
	case V_02803C_X_ADDR_SURF_P8_32X64_32X32:
		sscreen->b.tiling_info.num_channels = 8;
		break;
	case V_02803C_X_ADDR_SURF_P16_32X32_8X16:
	case V_02803C_X_ADDR_SURF_P16_32X32_16X16:
		sscreen->b.tiling_info.num_channels = 16;
		break;
	default:
		assert(0);
		fprintf(stderr, "radeonsi: Unknown pipe config %i.\n",
			G_009910_PIPE_CONFIG(mode2d));
		return false;
	}
	return true;
}

struct pipe_screen *radeonsi_screen_create(struct radeon_winsys *ws)
{
	struct si_screen *sscreen = CALLOC_STRUCT(si_screen);
	if (sscreen == NULL) {
		return NULL;
	}

	/* Set functions first. */
	sscreen->b.b.context_create = si_create_context;
	sscreen->b.b.destroy = si_destroy_screen;
	sscreen->b.b.get_param = si_get_param;
	sscreen->b.b.get_shader_param = si_get_shader_param;
	sscreen->b.b.is_format_supported = si_is_format_supported;
	sscreen->b.b.resource_create = r600_resource_create_common;

	if (!r600_common_screen_init(&sscreen->b, ws) ||
	    !si_initialize_pipe_config(sscreen)) {
		FREE(sscreen);
		return NULL;
	}

	sscreen->b.has_cp_dma = true;
	sscreen->b.has_streamout = true;

	if (debug_get_bool_option("RADEON_DUMP_SHADERS", FALSE))
		sscreen->b.debug_flags |= DBG_FS | DBG_VS | DBG_GS | DBG_PS | DBG_CS;

	/* Create the auxiliary context. This must be done last. */
	sscreen->b.aux_context = sscreen->b.b.context_create(&sscreen->b.b, NULL);

	return &sscreen->b.b;
}
