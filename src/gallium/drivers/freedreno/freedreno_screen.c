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

#include "freedreno_screen.h"
#include "freedreno_resource.h"
#include "freedreno_fence.h"
#include "freedreno_util.h"

#include "fd2_screen.h"
#include "fd3_screen.h"

/* XXX this should go away */
#include "state_tracker/drm_driver.h"

static const struct debug_named_value debug_options[] = {
		{"msgs",      FD_DBG_MSGS,   "Print debug messages"},
		{"disasm",    FD_DBG_DISASM, "Dump TGSI and adreno shader disassembly"},
		{"dclear",    FD_DBG_DCLEAR, "Mark all state dirty after clear"},
		{"dgmem",     FD_DBG_DGMEM,  "Mark all state dirty after GMEM tile pass"},
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
	struct fd_screen *screen = fd_screen(pscreen);

	if (screen->pipe)
		fd_pipe_del(screen->pipe);

	if (screen->dev)
		fd_device_del(screen->dev);

	free(screen);
}

/*
TODO either move caps to a2xx/a3xx specific code, or maybe have some
tables for things that differ if the delta is not too much..
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
	case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
	case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
	case PIPE_CAP_VERTEX_COLOR_CLAMPED:
	case PIPE_CAP_USER_VERTEX_BUFFERS:
	case PIPE_CAP_USER_INDEX_BUFFERS:
	case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
	case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
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

	case PIPE_CAP_ENDIANNESS:
		return PIPE_ENDIAN_LITTLE;

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

	pscreen = &screen->base;

	screen->dev = dev;

	// maybe this should be in context?
	screen->pipe = fd_pipe_new(screen->dev, FD_PIPE_3D);
	if (!screen->pipe) {
		DBG("could not create 3d pipe");
		goto fail;
	}

	if (fd_pipe_get_param(screen->pipe, FD_GMEM_SIZE, &val)) {
		DBG("could not get GMEM size");
		goto fail;
	}
	screen->gmemsize_bytes = val;

	if (fd_pipe_get_param(screen->pipe, FD_DEVICE_ID, &val)) {
		DBG("could not get device-id");
		goto fail;
	}
	screen->device_id = val;

	if (fd_pipe_get_param(screen->pipe, FD_GPU_ID, &val)) {
		DBG("could not get gpu-id");
		goto fail;
	}
	screen->gpu_id = val;

	/* explicitly checking for GPU revisions that are known to work.  This
	 * may be overly conservative for a3xx, where spoofing the gpu_id with
	 * the blob driver seems to generate identical cmdstream dumps.  But
	 * on a2xx, there seem to be small differences between the GPU revs
	 * so it is probably better to actually test first on real hardware
	 * before enabling:
	 *
	 * If you have a different adreno version, feel free to add it to one
	 * of the two cases below and see what happens.  And if it works, please
	 * send a patch ;-)
	 */
	switch (screen->gpu_id) {
	case 220:
		fd2_screen_init(pscreen);
		break;
	case 320:
		fd3_screen_init(pscreen);
		break;
	default:
		debug_printf("unsupported GPU: a%03d\n", screen->gpu_id);
		goto fail;
	}

	pscreen->destroy = fd_screen_destroy;
	pscreen->get_param = fd_screen_get_param;
	pscreen->get_paramf = fd_screen_get_paramf;
	pscreen->get_shader_param = fd_screen_get_shader_param;

	fd_resource_screen_init(pscreen);

	pscreen->get_name = fd_screen_get_name;
	pscreen->get_vendor = fd_screen_get_vendor;

	pscreen->get_timestamp = fd_screen_get_timestamp;

	pscreen->fence_reference = fd_screen_fence_ref;
	pscreen->fence_signalled = fd_screen_fence_signalled;
	pscreen->fence_finish = fd_screen_fence_finish;

	util_format_s3tc_init();

	return pscreen;

fail:
	fd_screen_destroy(pscreen);
	return NULL;
}
