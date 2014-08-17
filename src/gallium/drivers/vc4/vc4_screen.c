/*
 * Copyright © 2014 Broadcom
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>

#include "os/os_misc.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_format.h"

#include "vc4_screen.h"
#include "vc4_context.h"
#include "vc4_resource.h"

static const struct debug_named_value debug_options[] = {
        { "cl",       VC4_DEBUG_CL,
          "Dump command list during creation" },
        { "qpu",      VC4_DEBUG_QPU,
          "Dump generated QPU instructions" },
        { "qir",      VC4_DEBUG_QIR,
          "Dump QPU IR during program compile" },
        { "tgsi",     VC4_DEBUG_TGSI,
          "Dump TGSI during program compile" },
        { "shaderdb", VC4_DEBUG_SHADERDB,
          "Dump program compile information for shader-db analysis" },
        { "perf",     VC4_DEBUG_PERF,
          "Print during performance-related events" },
        { "norast",   VC4_DEBUG_NORAST,
          "Skip actual hardware execution of commands" },
};

DEBUG_GET_ONCE_FLAGS_OPTION(vc4_debug, "VC4_DEBUG", debug_options, 0)
uint32_t vc4_debug;

static const char *
vc4_screen_get_name(struct pipe_screen *pscreen)
{
        return "VC4";
}

static const char *
vc4_screen_get_vendor(struct pipe_screen *pscreen)
{
        return "Broadcom";
}

static void
vc4_screen_destroy(struct pipe_screen *pscreen)
{
        free(pscreen);
}

static int
vc4_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
        switch (param) {
                /* Supported features (boolean caps). */
        case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
        case PIPE_CAP_BUFFER_MAP_PERSISTENT_COHERENT:
        case PIPE_CAP_NPOT_TEXTURES:
        case PIPE_CAP_USER_CONSTANT_BUFFERS:
        case PIPE_CAP_TEXTURE_SHADOW_MAP:
        case PIPE_CAP_BLEND_EQUATION_SEPARATE:
        case PIPE_CAP_TWO_SIDED_STENCIL:
                return 1;

                /* lying for GL 2.0 */
        case PIPE_CAP_OCCLUSION_QUERY:
        case PIPE_CAP_POINT_SPRITE:
                return 1;

        case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
                return 256;

        case PIPE_CAP_GLSL_FEATURE_LEVEL:
                return 120;

        case PIPE_CAP_MAX_VIEWPORTS:
                return 1;

        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
                return 1;

                /* Unsupported features. */
        case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
        case PIPE_CAP_ANISOTROPIC_FILTER:
        case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
        case PIPE_CAP_CUBE_MAP_ARRAY:
        case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
        case PIPE_CAP_TEXTURE_SWIZZLE:
        case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
        case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
        case PIPE_CAP_SEAMLESS_CUBE_MAP:
        case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
        case PIPE_CAP_TGSI_INSTANCEID:
        case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
        case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
        case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
        case PIPE_CAP_COMPUTE:
        case PIPE_CAP_START_INSTANCE:
        case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
        case PIPE_CAP_SHADER_STENCIL_EXPORT:
        case PIPE_CAP_TGSI_TEXCOORD:
        case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
        case PIPE_CAP_CONDITIONAL_RENDER:
        case PIPE_CAP_PRIMITIVE_RESTART:
        case PIPE_CAP_TEXTURE_MULTISAMPLE:
        case PIPE_CAP_TEXTURE_BARRIER:
        case PIPE_CAP_SM3:
        case PIPE_CAP_INDEP_BLEND_ENABLE:
        case PIPE_CAP_INDEP_BLEND_FUNC:
        case PIPE_CAP_DEPTH_CLIP_DISABLE:
        case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
        case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
        case PIPE_CAP_FRAGMENT_COLOR_CLAMPED:
        case PIPE_CAP_VERTEX_COLOR_CLAMPED:
        case PIPE_CAP_USER_VERTEX_BUFFERS:
        case PIPE_CAP_USER_INDEX_BUFFERS:
        case PIPE_CAP_QUERY_PIPELINE_STATISTICS:
        case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
        case PIPE_CAP_TGSI_VS_LAYER_VIEWPORT:
        case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
        case PIPE_CAP_TEXTURE_GATHER_SM5:
        case PIPE_CAP_FAKE_SW_MSAA:
        case PIPE_CAP_TEXTURE_QUERY_LOD:
        case PIPE_CAP_SAMPLE_SHADING:
        case PIPE_CAP_TEXTURE_GATHER_OFFSETS:
        case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
        case PIPE_CAP_TGSI_VS_WINDOW_SPACE_POSITION:
        case PIPE_CAP_MAX_TEXEL_OFFSET:
        case PIPE_CAP_MAX_VERTEX_STREAMS:
        case PIPE_CAP_DRAW_INDIRECT:
        case PIPE_CAP_TGSI_FS_FINE_DERIVATIVE:
        case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
                return 0;

                /* Stream output. */
        case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
        case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
        case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
        case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
                return 0;

                /* Geometry shader output, unsupported. */
        case PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES:
        case PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS:
                return 0;

                /* Texturing. */
        case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
                return VC4_MAX_MIP_LEVELS;
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
                /* Note: Not supported in hardware, just faking it. */
                return 5;
        case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
                return 0;

                /* Render targets. */
        case PIPE_CAP_MAX_RENDER_TARGETS:
                return 1;

                /* Queries. */
        case PIPE_CAP_QUERY_TIME_ELAPSED:
        case PIPE_CAP_QUERY_TIMESTAMP:
                return 0;

        case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
        case PIPE_CAP_MIN_TEXEL_OFFSET:
                return 0;

        case PIPE_CAP_ENDIANNESS:
                return PIPE_ENDIAN_LITTLE;

        case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
                return 64;

        case PIPE_CAP_VENDOR_ID:
                return 0x14E4;
        case PIPE_CAP_DEVICE_ID:
                return 0xFFFFFFFF;
        case PIPE_CAP_ACCELERATED:
                return 1;
        case PIPE_CAP_VIDEO_MEMORY: {
                uint64_t system_memory;

                if (!os_get_total_physical_memory(&system_memory))
                        return 0;

                return (int)(system_memory >> 20);
        }
        case PIPE_CAP_UMA:
                return 1;

        default:
                fprintf(stderr, "unknown param %d\n", param);
                return 0;
        }
}

static float
vc4_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
{
        switch (param) {
        case PIPE_CAPF_MAX_LINE_WIDTH:
        case PIPE_CAPF_MAX_LINE_WIDTH_AA:
        case PIPE_CAPF_MAX_POINT_WIDTH:
        case PIPE_CAPF_MAX_POINT_WIDTH_AA:
                return 8192.0f;
        case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
                return 0.0f;
        case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
                return 0.0f;
        case PIPE_CAPF_GUARD_BAND_LEFT:
        case PIPE_CAPF_GUARD_BAND_TOP:
        case PIPE_CAPF_GUARD_BAND_RIGHT:
        case PIPE_CAPF_GUARD_BAND_BOTTOM:
                return 0.0f;
        default:
                fprintf(stderr, "unknown paramf %d\n", param);
                return 0;
        }
}

static int
vc4_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
                           enum pipe_shader_cap param)
{
        if (shader != PIPE_SHADER_VERTEX &&
            shader != PIPE_SHADER_FRAGMENT) {
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
                return 0;
        case PIPE_SHADER_CAP_MAX_INPUTS:
                return 16;
        case PIPE_SHADER_CAP_MAX_TEMPS:
                return 64; /* Max native temporaries. */
        case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
                return 64 * sizeof(float[4]);
        case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
                return 1;
        case PIPE_SHADER_CAP_MAX_PREDS:
                return 0; /* nothing uses this */
        case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
                return 0;
        case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
                return 0;
        case PIPE_SHADER_CAP_SUBROUTINES:
                return 0;
        case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
                return 0;
        case PIPE_SHADER_CAP_INTEGERS:
        case PIPE_SHADER_CAP_DOUBLES:
                return 0;
        case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
        case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
                return VC4_MAX_TEXTURE_SAMPLERS;
        case PIPE_SHADER_CAP_PREFERRED_IR:
                return PIPE_SHADER_IR_TGSI;
        default:
                fprintf(stderr, "unknown shader param %d\n", param);
                return 0;
        }
        return 0;
}

uint8_t
vc4_get_texture_format(enum pipe_format format)
{
        switch (format) {
        case PIPE_FORMAT_B8G8R8A8_UNORM:
                return 0;
        case PIPE_FORMAT_B8G8R8X8_UNORM:
                return 1;
        case PIPE_FORMAT_R8G8B8A8_UNORM:
                return 0;
        case PIPE_FORMAT_R8G8B8X8_UNORM:
                return 1;
        case PIPE_FORMAT_A8R8G8B8_UNORM:
                return 0;
        case PIPE_FORMAT_X8R8G8B8_UNORM:
                return 1;
        case PIPE_FORMAT_A8B8G8R8_UNORM:
                return 0;
        case PIPE_FORMAT_X8B8G8R8_UNORM:
                return 1;
/*
        case PIPE_FORMAT_R4G4B4A4_UNORM:
                return 2;
        case PIPE_FORMAT_R5G5B5A1_UNORM:
                return 3;
        case PIPE_FORMAT_R5G6B5_UNORM:
                return 4;
*/
        case PIPE_FORMAT_L8_UNORM:
                return 5;
        case PIPE_FORMAT_A8_UNORM:
                return 6;
        case PIPE_FORMAT_L8A8_UNORM:
                return 7;
                /* XXX: ETC1 and more*/
        default:
                return ~0;
        }
}

static boolean
vc4_screen_is_format_supported(struct pipe_screen *pscreen,
                               enum pipe_format format,
                               enum pipe_texture_target target,
                               unsigned sample_count,
                               unsigned usage)
{
        unsigned retval = 0;

        if ((target >= PIPE_MAX_TEXTURE_TYPES) ||
            (sample_count > 1) ||
            !util_format_is_supported(format, usage)) {
                return FALSE;
        }

        if (usage & PIPE_BIND_VERTEX_BUFFER &&
            (format == PIPE_FORMAT_R32G32B32A32_FLOAT ||
             format == PIPE_FORMAT_R32G32B32_FLOAT ||
             format == PIPE_FORMAT_R32G32_FLOAT ||
             format == PIPE_FORMAT_R32_FLOAT)) {
                retval |= PIPE_BIND_VERTEX_BUFFER;
        }

        if ((usage & PIPE_BIND_RENDER_TARGET) &&
            (format == PIPE_FORMAT_B8G8R8A8_UNORM ||
             format == PIPE_FORMAT_B8G8R8X8_UNORM || /* XXX: really? */
             format == PIPE_FORMAT_R8G8B8A8_UNORM ||
             format == PIPE_FORMAT_R8G8B8X8_UNORM || /* XXX: really? */
             format == PIPE_FORMAT_A8B8G8R8_UNORM ||
             format == PIPE_FORMAT_X8B8G8R8_UNORM || /* XXX: really? */
             format == PIPE_FORMAT_A8R8G8B8_UNORM ||
             format == PIPE_FORMAT_X8R8G8B8_UNORM || /* XXX: really? */
             format == PIPE_FORMAT_R16G16B16A16_FLOAT)) {
                retval |= PIPE_BIND_RENDER_TARGET;
        }

        if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
            (vc4_get_texture_format(format) != ~0)) {
                retval |= PIPE_BIND_SAMPLER_VIEW;
        }

        if ((usage & PIPE_BIND_DEPTH_STENCIL) &&
            (format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
             format == PIPE_FORMAT_Z24X8_UNORM)) {
                retval |= PIPE_BIND_DEPTH_STENCIL;
        }

        if ((usage & PIPE_BIND_INDEX_BUFFER) &&
            (format == PIPE_FORMAT_I8_UINT ||
             format == PIPE_FORMAT_I16_UINT)) {
                retval |= PIPE_BIND_INDEX_BUFFER;
        }

        if (usage & PIPE_BIND_TRANSFER_READ)
                retval |= PIPE_BIND_TRANSFER_READ;
        if (usage & PIPE_BIND_TRANSFER_WRITE)
                retval |= PIPE_BIND_TRANSFER_WRITE;

#if 0
	if (retval != usage) {
		fprintf(stderr,
                        "not supported: format=%s, target=%d, sample_count=%d, "
                        "usage=0x%x, retval=0x%x\n", util_format_name(format),
                        target, sample_count, usage, retval);
	}
#endif

        return retval == usage;
}

struct pipe_screen *
vc4_screen_create(int fd)
{
        struct vc4_screen *screen = CALLOC_STRUCT(vc4_screen);
        struct pipe_screen *pscreen;

        pscreen = &screen->base;

        pscreen->destroy = vc4_screen_destroy;
        pscreen->get_param = vc4_screen_get_param;
        pscreen->get_paramf = vc4_screen_get_paramf;
        pscreen->get_shader_param = vc4_screen_get_shader_param;
        pscreen->context_create = vc4_context_create;
        pscreen->is_format_supported = vc4_screen_is_format_supported;

        screen->fd = fd;

	vc4_debug = debug_get_option_vc4_debug();
        if (vc4_debug & VC4_DEBUG_SHADERDB)
                vc4_debug |= VC4_DEBUG_NORAST;

#if USE_VC4_SIMULATOR
        vc4_simulator_init(screen);
#endif

        vc4_resource_screen_init(pscreen);

        pscreen->get_name = vc4_screen_get_name;
        pscreen->get_vendor = vc4_screen_get_vendor;

        return pscreen;
}

boolean
vc4_screen_bo_get_handle(struct pipe_screen *pscreen,
                         struct vc4_bo *bo,
                         unsigned stride,
                         struct winsys_handle *whandle)
{
        whandle->stride = stride;

        switch (whandle->type) {
        case DRM_API_HANDLE_TYPE_SHARED:
                return vc4_bo_flink(bo, &whandle->handle);
        case DRM_API_HANDLE_TYPE_KMS:
                whandle->handle = bo->handle;
                return TRUE;
        }

        return FALSE;
}

struct vc4_bo *
vc4_screen_bo_from_handle(struct pipe_screen *pscreen,
                          struct winsys_handle *whandle,
                          unsigned *out_stride)
{
        struct vc4_screen *screen = vc4_screen(pscreen);
        struct vc4_bo *bo;

        if (whandle->type != DRM_API_HANDLE_TYPE_SHARED) {
                fprintf(stderr,
                        "Attempt to import unsupported handle type %d\n",
                        whandle->type);
                return NULL;
        }

        bo = vc4_bo_open_name(screen, whandle->handle, whandle->stride);
        if (!bo) {
                fprintf(stderr, "Open name %d failed\n", whandle->handle);
                return NULL;
        }

        *out_stride = whandle->stride;

        return bo;
}
