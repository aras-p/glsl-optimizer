/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */


#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_string.h"
#include "util/u_debug.h"

#include "os/os_time.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "freedreno_context.h"
#include "freedreno_screen.h"
#include "freedreno_resource.h"
#include "freedreno_fence.h"
#include "freedreno_util.h"

/* XXX this should go away */
#include "state_tracker/drm_driver.h"

static const struct debug_named_value debug_options[] = {
		{"msgs",      FD_DBG_MSGS,   "Print debug messages"},
		{"disasm",    FD_DBG_DISASM, "Dump TGSI and adreno shader disassembly"},
		DEBUG_NAMED_VALUE_END
};

DEBUG_GET_ONCE_FLAGS_OPTION(fd_mesa_debug, "FD_MESA_DEBUG", debug_options, 0)

int fd_mesa_debug = 0;

static const char *
fd_screen_get_name(struct pipe_screen *pscreen)
{
	static char buffer[128];
	util_snprintf(buffer, sizeof(buffer), "FD%03d",
			fd_screen(pscreen)->device_id);
	return buffer;
}

static const char *
fd_screen_get_vendor(struct pipe_screen *pscreen)
{
	return "freedreno";
}

static uint64_t
fd_screen_get_timestamp(struct pipe_screen *pscreen)
{
	int64_t cpu_time = os_time_get() * 1000;
	return cpu_time + fd_screen(pscreen)->cpu_gpu_time_delta;
}

static void
fd_screen_fence_ref(struct pipe_screen *pscreen,
		struct pipe_fence_handle **ptr,
		struct pipe_fence_handle *pfence)
{
	fd_fence_ref(fd_fence(pfence), (struct fd_fence **)ptr);
}

static boolean
fd_screen_fence_signalled(struct pipe_screen *screen,
		struct pipe_fence_handle *pfence)
{
	return fd_fence_signalled(fd_fence(pfence));
}

static boolean
fd_screen_fence_finish(struct pipe_screen *screen,
		struct pipe_fence_handle *pfence,
		uint64_t timeout)
{
	return fd_fence_wait(fd_fence(pfence));
}

static void
fd_screen_destroy(struct pipe_screen *pscreen)
{
	// TODO
	DBG("TODO");
}

/*
EGL Version 1.4
EGL Vendor Qualcomm, Inc
EGL Extensions EGL_QUALCOMM_shared_image EGL_KHR_image EGL_AMD_create_image EGL_KHR_lock_surface EGL_KHR_lock_surface2 EGL_KHR_fence_sync EGL_IMG_context_priorityEGL_ANDROID_image_native_buffer
GL extensions: GL_AMD_compressed_ATC_texture GL_AMD_performance_monitor GL_AMD_program_binary_Z400 GL_EXT_texture_filter_anisotropic GL_EXT_texture_format_BGRA8888 GL_EXT_texture_type_2_10_10_10_REV GL_NV_fence GL_OES_compressed_ETC1_RGB8_texture GL_OES_depth_texture GL_OES_depth24 GL_OES_EGL_image GL_OES_EGL_image_external GL_OES_element_index_uint GL_OES_fbo_render_mipmap GL_OES_fragment_precision_high GL_OES_get_program_binary GL_OES_packed_depth_stencil GL_OES_rgb8_rgba8 GL_OES_standard_derivatives GL_OES_texture_3D GL_OES_texture_float GL_OES_texture_half_float GL_OES_texture_half_float_linear GL_OES_texture_npot GL_OES_vertex_half_float GL_OES_vertex_type_10_10_10_2 GL_QCOM_alpha_test GL_QCOM_binning_control GL_QCOM_driver_control GL_QCOM_perfmon_global_mode GL_QCOM_extended_get GL_QCOM_extended_get2 GL_QCOM_tiled_rendering GL_QCOM_writeonly_rendering GL_AMD_compressed_3DC_texture
GL_MAX_3D_TEXTURE_SIZE_OES: 1024 0 0 0
no GL_MAX_SAMPLES_ANGLE: GL_INVALID_ENUM
no GL_MAX_SAMPLES_APPLE: GL_INVALID_ENUM
GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: 16 0 0 0
no GL_MAX_SAMPLES_IMG: GL_INVALID_ENUM
GL_MAX_TEXTURE_SIZE: 4096 0 0 0
GL_MAX_VIEWPORT_DIMS: 4096 4096 0 0
GL_MAX_VERTEX_ATTRIBS: 16 0 0 0
GL_MAX_VERTEX_UNIFORM_VECTORS: 251 0 0 0
GL_MAX_VARYING_VECTORS: 8 0 0 0
GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: 20 0 0 0
GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS: 4 0 0 0
GL_MAX_TEXTURE_IMAGE_UNITS: 16 0 0 0
GL_MAX_FRAGMENT_UNIFORM_VECTORS: 221 0 0 0
GL_MAX_CUBE_MAP_TEXTURE_SIZE: 4096 0 0 0
GL_MAX_RENDERBUFFER_SIZE: 4096 0 0 0
no GL_TEXTURE_NUM_LEVELS_QCOM: GL_INVALID_ENUM
 */
static int
fd_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
	/* this is probably not totally correct.. but it's a start: */
	switch (param) {
	/* Supported features (boolean caps). */
	case PIPE_CAP_NPOT_TEXTURES:
	case PIPE_CAP_TWO_SIDED_STENCIL:
	case PIPE_CAP_ANISOTROPIC_FILTER:
	case PIPE_CAP_POINT_SPRITE:
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
	case PIPE_CAP_TEXTURE_SWIZZLE:
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
	case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
	case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
	case PIPE_CAP_TGSI_INSTANCEID:
	case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
	case PIPE_CAP_COMPUTE:
	case PIPE_CAP_START_INSTANCE:
	case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
	case PIPE_CAP_TEXTURE_MULTISAMPLE:
	case PIPE_CAP_USER_CONSTANT_BUFFERS:
		return 1;

	case PIPE_CAP_TGSI_TEXCOORD:
	case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
		return 0;

	case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
		return 256;

	case PIPE_CAP_GLSL_FEATURE_LEVEL:
		return 120;

	/* Unsupported features. */
	case PIPE_CAP_INDEP_BLEND_ENABLE:
	case PIPE_CAP_INDEP_BLEND_FUNC:
	case PIPE_CAP_DEPTH_CLIP_DISABLE:
	case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
	case PIPE_CAP_SCALED_RESOLVE:
	case PIPE_CAP_TGSI_CAN_COMPACT_VARYINGS:
	case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
	case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
	case PIPE_CAP_VERTEX_COLOR_CLAMPED:
	case PIPE_CAP_USER_VERTEX_BUFFERS:
	case PIPE_CAP_USER_INDEX_BUFFERS:
		return 0;

	/* Stream output. */
	case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
	case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
	case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
	case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
		return 0;

	/* Texturing. */
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 14;
	case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
		return 9192;
	case PIPE_CAP_MAX_COMBINED_SAMPLERS:
		return 20;

	/* Render targets. */
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 1;

	/* Timer queries. */
	case PIPE_CAP_QUERY_TIME_ELAPSED:
	case PIPE_CAP_OCCLUSION_QUERY:
	case PIPE_CAP_QUERY_TIMESTAMP:
		return 0;

	case PIPE_CAP_MIN_TEXEL_OFFSET:
		return -8;

	case PIPE_CAP_MAX_TEXEL_OFFSET:
		return 7;

	default:
		DBG("unknown param %d", param);
		return 0;
	}
}

static float
fd_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
{
	switch (param) {
	case PIPE_CAPF_MAX_LINE_WIDTH:
	case PIPE_CAPF_MAX_LINE_WIDTH_AA:
	case PIPE_CAPF_MAX_POINT_WIDTH:
	case PIPE_CAPF_MAX_POINT_WIDTH_AA:
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
	default:
		DBG("unknown paramf %d", param);
		return 0;
	}
}

static int
fd_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
		enum pipe_shader_cap param)
{
	switch(shader)
	{
	case PIPE_SHADER_FRAGMENT:
	case PIPE_SHADER_VERTEX:
		break;
	case PIPE_SHADER_COMPUTE:
	case PIPE_SHADER_GEOMETRY:
		/* maye we could emulate.. */
		return 0;
	default:
		DBG("unknown shader type %d", shader);
		return 0;
	}

	/* this is probably not totally correct.. but it's a start: */
	switch (param) {
	case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
		return 16384;
	case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
		return 8; /* XXX */
	case PIPE_SHADER_CAP_MAX_INPUTS:
		return 32;
	case PIPE_SHADER_CAP_MAX_TEMPS:
		return 256; /* Max native temporaries. */
	case PIPE_SHADER_CAP_MAX_ADDRS:
		/* XXX Isn't this equal to TEMPS? */
		return 1; /* Max native address registers */
	case PIPE_SHADER_CAP_MAX_CONSTS:
	case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
		return 64;
	case PIPE_SHADER_CAP_MAX_PREDS:
		return 0; /* nothing uses this */
	case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		return 1;
	case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
		return 1;
	case PIPE_SHADER_CAP_SUBROUTINES:
		return 0;
	case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
	case PIPE_SHADER_CAP_INTEGERS:
		return 0;
	case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
		return 16;
	case PIPE_SHADER_CAP_PREFERRED_IR:
		return PIPE_SHADER_IR_TGSI;
	default:
		DBG("unknown shader param %d", param);
		return 0;
	}
	return 0;
}

static boolean
fd_screen_is_format_supported(struct pipe_screen *pscreen,
		enum pipe_format format,
		enum pipe_texture_target target,
		unsigned sample_count,
		unsigned usage)
{
	unsigned retval = 0;

	if ((target >= PIPE_MAX_TEXTURE_TYPES) ||
			(sample_count > 1) || /* TODO add MSAA */
			!util_format_is_supported(format, usage)) {
		DBG("not supported: format=%s, target=%d, sample_count=%d, usage=%x",
				util_format_name(format), target, sample_count, usage);
		return FALSE;
	}

	/* TODO figure out how to render to other formats.. */
	if ((usage & PIPE_BIND_RENDER_TARGET) &&
			((format != PIPE_FORMAT_B8G8R8A8_UNORM) &&
			 (format != PIPE_FORMAT_B8G8R8X8_UNORM))) {
		DBG("not supported render target: format=%s, target=%d, sample_count=%d, usage=%x",
				util_format_name(format), target, sample_count, usage);
		return FALSE;
	}

	if ((usage & (PIPE_BIND_SAMPLER_VIEW |
				PIPE_BIND_VERTEX_BUFFER)) &&
			(fd_pipe2surface(format) != FMT_INVALID)) {
		retval |= usage & (PIPE_BIND_SAMPLER_VIEW |
				PIPE_BIND_VERTEX_BUFFER);
	}

	if ((usage & (PIPE_BIND_RENDER_TARGET |
				PIPE_BIND_DISPLAY_TARGET |
				PIPE_BIND_SCANOUT |
				PIPE_BIND_SHARED)) &&
			(fd_pipe2color(format) != COLORX_INVALID)) {
		retval |= usage & (PIPE_BIND_RENDER_TARGET |
				PIPE_BIND_DISPLAY_TARGET |
				PIPE_BIND_SCANOUT |
				PIPE_BIND_SHARED);
	}

	if ((usage & PIPE_BIND_DEPTH_STENCIL) &&
			(fd_pipe2depth(format) != DEPTHX_INVALID)) {
		retval |= PIPE_BIND_DEPTH_STENCIL;
	}

	if ((usage & PIPE_BIND_INDEX_BUFFER) &&
			(fd_pipe2index(format) != INDEX_SIZE_INVALID)) {
		retval |= PIPE_BIND_INDEX_BUFFER;
	}

	if (usage & PIPE_BIND_TRANSFER_READ)
		retval |= PIPE_BIND_TRANSFER_READ;
	if (usage & PIPE_BIND_TRANSFER_WRITE)
		retval |= PIPE_BIND_TRANSFER_WRITE;

	if (retval != usage) {
		DBG("not supported: format=%s, target=%d, sample_count=%d, "
				"usage=%x, retval=%x", util_format_name(format),
				target, sample_count, usage, retval);
	}

	return retval == usage;
}

boolean
fd_screen_bo_get_handle(struct pipe_screen *pscreen,
		struct fd_bo *bo,
		unsigned stride,
		struct winsys_handle *whandle)
{
	whandle->stride = stride;

	if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) {
		return fd_bo_get_name(bo, &whandle->handle) == 0;
	} else if (whandle->type == DRM_API_HANDLE_TYPE_KMS) {
		whandle->handle = fd_bo_handle(bo);
		return TRUE;
	} else {
		return FALSE;
	}
}

struct fd_bo *
fd_screen_bo_from_handle(struct pipe_screen *pscreen,
		struct winsys_handle *whandle,
		unsigned *out_stride)
{
	struct fd_screen *screen = fd_screen(pscreen);
	struct fd_bo *bo;

	bo = fd_bo_from_name(screen->dev, whandle->handle);
	if (!bo) {
		DBG("ref name 0x%08x failed", whandle->handle);
		return NULL;
	}

	*out_stride = whandle->stride;

	return bo;
}

struct pipe_screen *
fd_screen_create(struct fd_device *dev)
{
	struct fd_screen *screen = CALLOC_STRUCT(fd_screen);
	struct pipe_screen *pscreen;
	uint64_t val;

	fd_mesa_debug = debug_get_option_fd_mesa_debug();

	if (!screen)
		return NULL;

	DBG("");

	screen->dev = dev;

	// maybe this should be in context?
	screen->pipe = fd_pipe_new(screen->dev, FD_PIPE_3D);

	fd_pipe_get_param(screen->pipe, FD_GMEM_SIZE, &val);
	screen->gmemsize_bytes = val;

	fd_pipe_get_param(screen->pipe, FD_DEVICE_ID, &val);
	screen->device_id = val;

	pscreen = &screen->base;

	pscreen->destroy = fd_screen_destroy;
	pscreen->get_param = fd_screen_get_param;
	pscreen->get_paramf = fd_screen_get_paramf;
	pscreen->get_shader_param = fd_screen_get_shader_param;
	pscreen->context_create = fd_context_create;
	pscreen->is_format_supported = fd_screen_is_format_supported;

	fd_resource_screen_init(pscreen);

	pscreen->get_name = fd_screen_get_name;
	pscreen->get_vendor = fd_screen_get_vendor;

	pscreen->get_timestamp = fd_screen_get_timestamp;

	pscreen->fence_reference = fd_screen_fence_ref;
	pscreen->fence_signalled = fd_screen_fence_signalled;
	pscreen->fence_finish = fd_screen_fence_finish;

	util_format_s3tc_init();

	return pscreen;
}
