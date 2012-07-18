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

/* TODO:
 *	- fix mask for depth control & cull for query
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
#include "util/u_transfer.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_framebuffer.h"
#include "pipebuffer/pb_buffer.h"
#include "r600.h"
#include "sid.h"
#include "r600_resource.h"
#include "radeonsi_pipe.h"
#include "si_state.h"

#if 0
static uint32_t r600_translate_stencil_op(int s_op)
{
	switch (s_op) {
	case PIPE_STENCIL_OP_KEEP:
		return V_028800_STENCIL_KEEP;
	case PIPE_STENCIL_OP_ZERO:
		return V_028800_STENCIL_ZERO;
	case PIPE_STENCIL_OP_REPLACE:
		return V_028800_STENCIL_REPLACE;
	case PIPE_STENCIL_OP_INCR:
		return V_028800_STENCIL_INCR;
	case PIPE_STENCIL_OP_DECR:
		return V_028800_STENCIL_DECR;
	case PIPE_STENCIL_OP_INCR_WRAP:
		return V_028800_STENCIL_INCR_WRAP;
	case PIPE_STENCIL_OP_DECR_WRAP:
		return V_028800_STENCIL_DECR_WRAP;
	case PIPE_STENCIL_OP_INVERT:
		return V_028800_STENCIL_INVERT;
	default:
		R600_ERR("Unknown stencil op %d", s_op);
		assert(0);
		break;
	}
	return 0;
}
#endif

static unsigned si_tex_wrap(unsigned wrap)
{
	switch (wrap) {
	default:
	case PIPE_TEX_WRAP_REPEAT:
		return V_008F30_SQ_TEX_WRAP;
	case PIPE_TEX_WRAP_CLAMP:
		return V_008F30_SQ_TEX_CLAMP_HALF_BORDER;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		return V_008F30_SQ_TEX_CLAMP_LAST_TEXEL;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		return V_008F30_SQ_TEX_CLAMP_BORDER;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		return V_008F30_SQ_TEX_MIRROR;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		return V_008F30_SQ_TEX_MIRROR_ONCE_HALF_BORDER;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		return V_008F30_SQ_TEX_MIRROR_ONCE_LAST_TEXEL;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		return V_008F30_SQ_TEX_MIRROR_ONCE_BORDER;
	}
}

static unsigned si_tex_filter(unsigned filter)
{
	switch (filter) {
	default:
	case PIPE_TEX_FILTER_NEAREST:
		return V_008F38_SQ_TEX_XY_FILTER_POINT;
	case PIPE_TEX_FILTER_LINEAR:
		return V_008F38_SQ_TEX_XY_FILTER_BILINEAR;
	}
}

static unsigned si_tex_mipfilter(unsigned filter)
{
	switch (filter) {
	case PIPE_TEX_MIPFILTER_NEAREST:
		return V_008F38_SQ_TEX_Z_FILTER_POINT;
	case PIPE_TEX_MIPFILTER_LINEAR:
		return V_008F38_SQ_TEX_Z_FILTER_LINEAR;
	default:
	case PIPE_TEX_MIPFILTER_NONE:
		return V_008F38_SQ_TEX_Z_FILTER_NONE;
	}
}

static unsigned si_tex_compare(unsigned compare)
{
	switch (compare) {
	default:
	case PIPE_FUNC_NEVER:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_NEVER;
	case PIPE_FUNC_LESS:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_LESS;
	case PIPE_FUNC_EQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_EQUAL;
	case PIPE_FUNC_LEQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_LESSEQUAL;
	case PIPE_FUNC_GREATER:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_GREATER;
	case PIPE_FUNC_NOTEQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_NOTEQUAL;
	case PIPE_FUNC_GEQUAL:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_GREATEREQUAL;
	case PIPE_FUNC_ALWAYS:
		return V_008F30_SQ_TEX_DEPTH_COMPARE_ALWAYS;
	}
}

static unsigned si_tex_dim(unsigned dim)
{
	switch (dim) {
	default:
	case PIPE_TEXTURE_1D:
		return V_008F1C_SQ_RSRC_IMG_1D;
	case PIPE_TEXTURE_1D_ARRAY:
		return V_008F1C_SQ_RSRC_IMG_1D_ARRAY;
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_RECT:
		return V_008F1C_SQ_RSRC_IMG_2D;
	case PIPE_TEXTURE_2D_ARRAY:
		return V_008F1C_SQ_RSRC_IMG_2D_ARRAY;
	case PIPE_TEXTURE_3D:
		return V_008F1C_SQ_RSRC_IMG_3D;
	case PIPE_TEXTURE_CUBE:
		return V_008F1C_SQ_RSRC_IMG_CUBE;
	}
}

static uint32_t si_translate_dbformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		return V_028040_Z_16;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028040_Z_24; /* XXX no longer supported on SI */
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028040_Z_32_FLOAT;
	default:
		return ~0U;
	}
}

static uint32_t si_translate_colorswap(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_R4A4_UNORM:
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_028C70_SWAP_STD;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_028C70_SWAP_STD_REV;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_Z16_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
		return V_028C70_SWAP_ALT;
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_R16_FLOAT:
		return V_028C70_SWAP_STD;

	/* 32-bit buffers. */
	case PIPE_FORMAT_A8B8G8R8_SRGB:
		return V_028C70_SWAP_STD_REV;
	case PIPE_FORMAT_B8G8R8A8_SRGB:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SSCALED:
	case PIPE_FORMAT_R8G8B8A8_USCALED:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	/* case PIPE_FORMAT_R8SG8SB8UX8U_NORM: */
		return V_028C70_SWAP_STD_REV;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_R11G11B10_FLOAT:
	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_SWAP_STD;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16A16_USCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SSCALED:
	case PIPE_FORMAT_R32G32B32A32_USCALED:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
		return V_028C70_SWAP_STD;
	default:
		R600_ERR("unsupported colorswap format %d\n", format);
		return ~0U;
	}
	return ~0U;
}

static uint32_t si_translate_colorformat(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8_UINT:
	case PIPE_FORMAT_R8_SINT:
		return V_028C70_COLOR_8;

	/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_028C70_COLOR_5_6_5;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_028C70_COLOR_1_5_5_5;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_028C70_COLOR_4_4_4_4;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_COLOR_8_8;

	case PIPE_FORMAT_Z16_UNORM:
	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_R16_FLOAT:
	case PIPE_FORMAT_R16G16_FLOAT:
		return V_028C70_COLOR_16;

	/* 32-bit buffers. */
	case PIPE_FORMAT_A8B8G8R8_SRGB:
	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_B8G8R8A8_SRGB:
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
	case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SSCALED:
	case PIPE_FORMAT_R8G8B8A8_USCALED:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
		return V_028C70_COLOR_8_8_8_8;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_028C70_COLOR_2_10_10_10;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return V_028C70_COLOR_8_24;

	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_028C70_COLOR_24_8;

	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028C70_COLOR_X24_8_32_FLOAT;

	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return V_028C70_COLOR_32;

	case PIPE_FORMAT_R16G16_SSCALED:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_COLOR_16_16;

	case PIPE_FORMAT_R11G11B10_FLOAT:
		return V_028C70_COLOR_10_11_11;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16_USCALED:
	case PIPE_FORMAT_R16G16B16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_USCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16_FLOAT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
		return V_028C70_COLOR_16_16_16_16;

	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_USCALED:
	case PIPE_FORMAT_R32G32_SSCALED:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R32G32_UINT:
		return V_028C70_COLOR_32_32;

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SSCALED:
	case PIPE_FORMAT_R32G32B32A32_USCALED:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		return V_028C70_COLOR_32_32_32_32;

	/* YUV buffers. */
	case PIPE_FORMAT_UYVY:
	case PIPE_FORMAT_YUYV:
	/* 96-bit buffers. */
	case PIPE_FORMAT_R32G32B32_FLOAT:
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_R4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
	default:
		return ~0U; /* Unsupported. */
	}
}

static uint32_t si_colorformat_endian_swap(uint32_t colorformat)
{
	if (R600_BIG_ENDIAN) {
		switch(colorformat) {
		/* 8-bit buffers. */
		case V_028C70_COLOR_8:
			return V_028C70_ENDIAN_NONE;

		/* 16-bit buffers. */
		case V_028C70_COLOR_5_6_5:
		case V_028C70_COLOR_1_5_5_5:
		case V_028C70_COLOR_4_4_4_4:
		case V_028C70_COLOR_16:
		case V_028C70_COLOR_8_8:
			return V_028C70_ENDIAN_8IN16;

		/* 32-bit buffers. */
		case V_028C70_COLOR_8_8_8_8:
		case V_028C70_COLOR_2_10_10_10:
		case V_028C70_COLOR_8_24:
		case V_028C70_COLOR_24_8:
		case V_028C70_COLOR_16_16:
			return V_028C70_ENDIAN_8IN32;

		/* 64-bit buffers. */
		case V_028C70_COLOR_16_16_16_16:
			return V_028C70_ENDIAN_8IN16;

		case V_028C70_COLOR_32_32:
			return V_028C70_ENDIAN_8IN32;

		/* 128-bit buffers. */
		case V_028C70_COLOR_32_32_32_32:
			return V_028C70_ENDIAN_8IN32;
		default:
			return V_028C70_ENDIAN_NONE; /* Unsupported. */
		}
	} else {
		return V_028C70_ENDIAN_NONE;
	}
}

static uint32_t si_translate_texformat(struct pipe_screen *screen,
				       enum pipe_format format,
				       const struct util_format_description *desc,
				       int first_non_void)
{
	boolean uniform = TRUE;
	int i;

	/* Colorspace (return non-RGB formats directly). */
	switch (desc->colorspace) {
	/* Depth stencil formats */
	case UTIL_FORMAT_COLORSPACE_ZS:
		switch (format) {
		case PIPE_FORMAT_Z16_UNORM:
			return V_008F14_IMG_DATA_FORMAT_16;
		case PIPE_FORMAT_X24S8_UINT:
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24_UNORM_S8_UINT:
			return V_008F14_IMG_DATA_FORMAT_24_8;
		case PIPE_FORMAT_S8X24_UINT:
		case PIPE_FORMAT_X8Z24_UNORM:
		case PIPE_FORMAT_S8_UINT_Z24_UNORM:
			return V_008F14_IMG_DATA_FORMAT_8_24;
		case PIPE_FORMAT_S8_UINT:
			return V_008F14_IMG_DATA_FORMAT_8;
		case PIPE_FORMAT_Z32_FLOAT:
			return V_008F14_IMG_DATA_FORMAT_32;
		case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
			return V_008F14_IMG_DATA_FORMAT_X24_8_32;
		default:
			goto out_unknown;
		}

	case UTIL_FORMAT_COLORSPACE_YUV:
		goto out_unknown; /* TODO */

	case UTIL_FORMAT_COLORSPACE_SRGB:
		break;

	default:
		break;
	}

	/* TODO compressed formats */

	if (format == PIPE_FORMAT_R9G9B9E5_FLOAT) {
		return V_008F14_IMG_DATA_FORMAT_5_9_9_9;
	} else if (format == PIPE_FORMAT_R11G11B10_FLOAT) {
		return V_008F14_IMG_DATA_FORMAT_10_11_11;
	}

	/* R8G8Bx_SNORM - TODO CxV8U8 */

	/* See whether the components are of the same size. */
	for (i = 1; i < desc->nr_channels; i++) {
		uniform = uniform && desc->channel[0].size == desc->channel[i].size;
	}

	/* Non-uniform formats. */
	if (!uniform) {
		switch(desc->nr_channels) {
		case 3:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 6 &&
			    desc->channel[2].size == 5) {
				return V_008F14_IMG_DATA_FORMAT_5_6_5;
			}
			goto out_unknown;
		case 4:
			if (desc->channel[0].size == 5 &&
			    desc->channel[1].size == 5 &&
			    desc->channel[2].size == 5 &&
			    desc->channel[3].size == 1) {
				return V_008F14_IMG_DATA_FORMAT_1_5_5_5;
			}
			if (desc->channel[0].size == 10 &&
			    desc->channel[1].size == 10 &&
			    desc->channel[2].size == 10 &&
			    desc->channel[3].size == 2) {
				return V_008F14_IMG_DATA_FORMAT_2_10_10_10;
			}
			goto out_unknown;
		}
		goto out_unknown;
	}

	if (first_non_void < 0 || first_non_void > 3)
		goto out_unknown;

	/* uniform formats */
	switch (desc->channel[first_non_void].size) {
	case 4:
		switch (desc->nr_channels) {
		case 2:
			return V_008F14_IMG_DATA_FORMAT_4_4;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_4_4_4_4;
		}
		break;
	case 8:
		switch (desc->nr_channels) {
		case 1:
			return V_008F14_IMG_DATA_FORMAT_8;
		case 2:
			return V_008F14_IMG_DATA_FORMAT_8_8;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_8_8_8_8;
		}
		break;
	case 16:
		switch (desc->nr_channels) {
		case 1:
			return V_008F14_IMG_DATA_FORMAT_16;
		case 2:
			return V_008F14_IMG_DATA_FORMAT_16_16;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_16_16_16_16;
		}
		break;
	case 32:
		switch (desc->nr_channels) {
		case 1:
			return V_008F14_IMG_DATA_FORMAT_32;
		case 2:
			return V_008F14_IMG_DATA_FORMAT_32_32;
		case 3:
			return V_008F14_IMG_DATA_FORMAT_32_32_32;
		case 4:
			return V_008F14_IMG_DATA_FORMAT_32_32_32_32;
		}
	}

out_unknown:
	/* R600_ERR("Unable to handle texformat %d %s\n", format, util_format_name(format)); */
	return ~0;
}

static bool si_is_sampler_format_supported(struct pipe_screen *screen, enum pipe_format format)
{
	return si_translate_texformat(screen, format, util_format_description(format),
				      util_format_get_first_non_void_channel(format)) != ~0U;
}

uint32_t si_translate_vertexformat(struct pipe_screen *screen,
				   enum pipe_format format,
				   const struct util_format_description *desc,
				   int first_non_void)
{
	uint32_t result;

	if (desc->channel[first_non_void].type == UTIL_FORMAT_TYPE_FIXED)
		return ~0;

	result = si_translate_texformat(screen, format, desc, first_non_void);
	if (result == V_008F0C_BUF_DATA_FORMAT_INVALID ||
	    result > V_008F0C_BUF_DATA_FORMAT_32_32_32_32)
		result = ~0;

	return result;
}

static bool si_is_vertex_format_supported(struct pipe_screen *screen, enum pipe_format format)
{
	return si_translate_vertexformat(screen, format, util_format_description(format),
					 util_format_get_first_non_void_channel(format)) != ~0U;
}

static bool r600_is_colorbuffer_format_supported(enum pipe_format format)
{
	return si_translate_colorformat(format) != ~0U &&
		si_translate_colorswap(format) != ~0U;
}

static bool r600_is_zs_format_supported(enum pipe_format format)
{
	return si_translate_dbformat(format) != ~0U;
}

boolean si_is_format_supported(struct pipe_screen *screen,
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
	    si_is_sampler_format_supported(screen, format)) {
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
	    si_is_vertex_format_supported(screen, format)) {
		retval |= PIPE_BIND_VERTEX_BUFFER;
	}

	if (usage & PIPE_BIND_TRANSFER_READ)
		retval |= PIPE_BIND_TRANSFER_READ;
	if (usage & PIPE_BIND_TRANSFER_WRITE)
		retval |= PIPE_BIND_TRANSFER_WRITE;

	return retval == usage;
}

static void *si_create_sampler_state(struct pipe_context *ctx,
				     const struct pipe_sampler_state *state)
{
	struct si_pipe_sampler_state *rstate = CALLOC_STRUCT(si_pipe_sampler_state);
	union util_color uc;
	unsigned aniso_flag_offset = state->max_anisotropy > 1 ? 2 : 0;
	unsigned border_color_type;

	if (rstate == NULL) {
		return NULL;
	}

	util_pack_color(state->border_color.f, PIPE_FORMAT_B8G8R8A8_UNORM, &uc);
	switch (uc.ui) {
	case 0x000000FF:
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_OPAQUE_BLACK;
		break;
	case 0x00000000:
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_TRANS_BLACK;
		break;
	case 0xFFFFFFFF:
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_OPAQUE_WHITE;
		break;
	default: /* Use border color pointer */
		border_color_type = V_008F3C_SQ_TEX_BORDER_COLOR_REGISTER;
	}

	rstate->val[0] = (S_008F30_CLAMP_X(si_tex_wrap(state->wrap_s)) |
			  S_008F30_CLAMP_Y(si_tex_wrap(state->wrap_t)) |
			  S_008F30_CLAMP_Z(si_tex_wrap(state->wrap_r)) |
			  (state->max_anisotropy & 0x7) << 9 | /* XXX */
			  S_008F30_DEPTH_COMPARE_FUNC(si_tex_compare(state->compare_func)) |
			  S_008F30_FORCE_UNNORMALIZED(!state->normalized_coords) |
			  aniso_flag_offset << 16 | /* XXX */
			  S_008F30_DISABLE_CUBE_WRAP(!state->seamless_cube_map));
	rstate->val[1] = (S_008F34_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 8)) |
			  S_008F34_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 8)));
	rstate->val[2] = (S_008F38_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 8)) |
			  S_008F38_XY_MAG_FILTER(si_tex_filter(state->mag_img_filter)) |
			  S_008F38_XY_MIN_FILTER(si_tex_filter(state->min_img_filter)) |
			  S_008F38_MIP_FILTER(si_tex_mipfilter(state->min_mip_filter)));
	rstate->val[3] = S_008F3C_BORDER_COLOR_TYPE(border_color_type);

#if 0
	if (border_color_type == 3) {
		r600_pipe_state_add_reg_noblock(rstate, R_00A404_TD_PS_SAMPLER0_BORDER_RED, fui(state->border_color.f[0]), NULL, 0);
		r600_pipe_state_add_reg_noblock(rstate, R_00A408_TD_PS_SAMPLER0_BORDER_GREEN, fui(state->border_color.f[1]), NULL, 0);
		r600_pipe_state_add_reg_noblock(rstate, R_00A40C_TD_PS_SAMPLER0_BORDER_BLUE, fui(state->border_color.f[2]), NULL, 0);
		r600_pipe_state_add_reg_noblock(rstate, R_00A410_TD_PS_SAMPLER0_BORDER_ALPHA, fui(state->border_color.f[3]), NULL, 0);
	}
#endif
	return rstate;
}

static void si_delete_sampler_state(struct pipe_context *ctx,
				    void *state)
{
	free(state);
}

static struct pipe_sampler_view *evergreen_create_sampler_view(struct pipe_context *ctx,
							struct pipe_resource *texture,
							const struct pipe_sampler_view *state)
{
	struct si_pipe_sampler_view *view = CALLOC_STRUCT(si_pipe_sampler_view);
	struct r600_resource_texture *tmp = (struct r600_resource_texture*)texture;
	const struct util_format_description *desc = util_format_description(state->format);
	unsigned blocksize = util_format_get_blocksize(tmp->real_format);
	unsigned format, num_format, endian, tiling_index;
	uint32_t pitch = 0;
	unsigned char state_swizzle[4], swizzle[4];
	unsigned height, depth, width;
	int first_non_void;
	uint64_t va;

	if (view == NULL)
		return NULL;

	/* initialize base object */
	view->base = *state;
	view->base.texture = NULL;
	pipe_reference(NULL, &texture->reference);
	view->base.texture = texture;
	view->base.reference.count = 1;
	view->base.context = ctx;

	state_swizzle[0] = state->swizzle_r;
	state_swizzle[1] = state->swizzle_g;
	state_swizzle[2] = state->swizzle_b;
	state_swizzle[3] = state->swizzle_a;
	util_format_compose_swizzles(desc->swizzle, state_swizzle, swizzle);

	first_non_void = util_format_get_first_non_void_channel(state->format);
	switch (desc->channel[first_non_void].type) {
	case UTIL_FORMAT_TYPE_FLOAT:
		num_format = V_008F14_IMG_NUM_FORMAT_FLOAT;
		break;
	case UTIL_FORMAT_TYPE_SIGNED:
		num_format = V_008F14_IMG_NUM_FORMAT_SNORM;
		break;
	case UTIL_FORMAT_TYPE_UNSIGNED:
	default:
		num_format = V_008F14_IMG_NUM_FORMAT_UNORM;
	}

	format = si_translate_texformat(ctx->screen, state->format, desc, first_non_void);
	if (format == ~0) {
		format = 0;
	}

	if (tmp->depth && !tmp->is_flushing_texture) {
		r600_texture_depth_flush(ctx, texture, TRUE);
		tmp = tmp->flushed_depth_texture;
	}

	endian = si_colorformat_endian_swap(format);

	height = texture->height0;
	depth = texture->depth0;
	width = texture->width0;
	pitch = align(tmp->pitch_in_blocks[0] *
		      util_format_get_blockwidth(state->format), 8);

	if (texture->target == PIPE_TEXTURE_1D_ARRAY) {
	        height = 1;
		depth = texture->array_size;
	} else if (texture->target == PIPE_TEXTURE_2D_ARRAY) {
		depth = texture->array_size;
	}

	tiling_index = 8;
	switch (tmp->surface.level[state->u.tex.first_level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		tiling_index = 8;
		break;
	case RADEON_SURF_MODE_1D:
		tiling_index = 9;
		break;
	case RADEON_SURF_MODE_2D:
		if (tmp->resource.b.b.bind & PIPE_BIND_SCANOUT) {
			switch (blocksize) {
			case 1:
				tiling_index = 10;
				break;
			case 2:
				tiling_index = 11;
				break;
			case 4:
				tiling_index = 12;
				break;
			}
			break;
		} else switch (blocksize) {
		case 1:
			tiling_index = 14;
			break;
		case 2:
			tiling_index = 15;
			break;
		case 4:
			tiling_index = 16;
			break;
		case 8:
			tiling_index = 17;
			break;
		default:
			tiling_index = 13;
		}
		break;
	}

	va = r600_resource_va(ctx->screen, texture);
	if (state->u.tex.last_level) {
		view->state[0] = (va + tmp->offset[1]) >> 8;
	} else {
		view->state[0] = (va + tmp->offset[0]) >> 8;
	}
	view->state[1] = (S_008F14_BASE_ADDRESS_HI((va + tmp->offset[0]) >> 40) |
			  S_008F14_DATA_FORMAT(format) |
			  S_008F14_NUM_FORMAT(num_format));
	view->state[2] = (S_008F18_WIDTH(width - 1) |
			  S_008F18_HEIGHT(height - 1));
	view->state[3] = (S_008F1C_DST_SEL_X(si_map_swizzle(swizzle[0])) |
			  S_008F1C_DST_SEL_Y(si_map_swizzle(swizzle[1])) |
			  S_008F1C_DST_SEL_Z(si_map_swizzle(swizzle[2])) |
			  S_008F1C_DST_SEL_W(si_map_swizzle(swizzle[3])) |
			  S_008F1C_BASE_LEVEL(state->u.tex.first_level) |
			  S_008F1C_LAST_LEVEL(state->u.tex.last_level) |
			  S_008F1C_TILING_INDEX(tiling_index) |
			  S_008F1C_TYPE(si_tex_dim(texture->target)));
	view->state[4] = (S_008F20_DEPTH(depth - 1) | S_008F20_PITCH(pitch - 1));
	view->state[5] = (S_008F24_BASE_ARRAY(state->u.tex.first_layer) |
			  S_008F24_LAST_ARRAY(state->u.tex.last_layer));
	view->state[6] = 0;
	view->state[7] = 0;

	return &view->base;
}

static void evergreen_set_vs_sampler_view(struct pipe_context *ctx, unsigned count,
					struct pipe_sampler_view **views)
{
}

static void evergreen_set_ps_sampler_view(struct pipe_context *ctx, unsigned count,
					struct pipe_sampler_view **views)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_sampler_view **resource = (struct si_pipe_sampler_view **)views;
	struct r600_pipe_state *rstate = &rctx->ps_samplers.views_state;
	struct r600_resource *bo;
	int i;
	int has_depth = 0;
	uint64_t va;
	char *ptr;

	if (!count)
		goto out;

	r600_inval_texture_cache(rctx);

	bo = (struct r600_resource*)
		pipe_buffer_create(ctx->screen, PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE,
				   count * sizeof(resource[0]->state));
	ptr = rctx->ws->buffer_map(bo->cs_buf, rctx->cs, PIPE_TRANSFER_WRITE);

	for (i = 0; i < count; i++, ptr += sizeof(resource[0]->state)) {
		pipe_sampler_view_reference(
			(struct pipe_sampler_view **)&rctx->ps_samplers.views[i],
			views[i]);

		if (resource[i]) {
			if (((struct r600_resource_texture *)resource[i]->base.texture)->depth)
				has_depth = 1;

			memcpy(ptr, resource[i]->state, sizeof(resource[0]->state));
		} else
			memset(ptr, 0, sizeof(resource[0]->state));
	}

	rctx->ws->buffer_unmap(bo->cs_buf);

	for (i = count; i < NUM_TEX_UNITS; i++) {
		if (rctx->ps_samplers.views[i])
			pipe_sampler_view_reference((struct pipe_sampler_view **)&rctx->ps_samplers.views[i], NULL);
	}

	rstate->nregs = 0;
	va = r600_resource_va(ctx->screen, (void *)bo);
	r600_pipe_state_add_reg(rstate, R_00B040_SPI_SHADER_USER_DATA_PS_4, va, bo, RADEON_USAGE_READ);
	r600_pipe_state_add_reg(rstate, R_00B044_SPI_SHADER_USER_DATA_PS_5, va >> 32, NULL, 0);
	r600_context_pipe_state_set(rctx, rstate);

out:
	rctx->have_depth_texture = has_depth;
	rctx->ps_samplers.n_views = count;
}

static void evergreen_bind_ps_sampler(struct pipe_context *ctx, unsigned count, void **states)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_sampler_state **rstates = (struct si_pipe_sampler_state **)states;
	struct r600_pipe_state *rstate = &rctx->ps_samplers.samplers_state;
	struct r600_resource *bo;
	uint64_t va;
	char *ptr;
	int i;

	if (!count)
		goto out;

	r600_inval_texture_cache(rctx);

	bo = (struct r600_resource*)
		pipe_buffer_create(ctx->screen, PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE,
				   count * sizeof(rstates[0]->val));
	ptr = rctx->ws->buffer_map(bo->cs_buf, rctx->cs, PIPE_TRANSFER_WRITE);

	for (i = 0; i < count; i++, ptr += sizeof(rstates[0]->val)) {
		memcpy(ptr, rstates[i]->val, sizeof(rstates[0]->val));
	}

	rctx->ws->buffer_unmap(bo->cs_buf);

	memcpy(rctx->ps_samplers.samplers, states, sizeof(void*) * count);

	rstate->nregs = 0;
	va = r600_resource_va(ctx->screen, (void *)bo);
	r600_pipe_state_add_reg(rstate, R_00B038_SPI_SHADER_USER_DATA_PS_2, va, bo, RADEON_USAGE_READ);
	r600_pipe_state_add_reg(rstate, R_00B03C_SPI_SHADER_USER_DATA_PS_3, va >> 32, NULL, 0);
	r600_context_pipe_state_set(rctx, rstate);

out:
	rctx->ps_samplers.n_samplers = count;
}

static void evergreen_bind_vs_sampler(struct pipe_context *ctx, unsigned count, void **states)
{
}

static void evergreen_set_polygon_stipple(struct pipe_context *ctx,
					 const struct pipe_poly_stipple *state)
{
}

static void evergreen_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
{
}

void cayman_init_state_functions(struct r600_context *rctx)
{
	si_init_state_functions(rctx);
	rctx->context.create_fs_state = si_create_shader_state;
	rctx->context.create_sampler_state = si_create_sampler_state;
	rctx->context.create_sampler_view = evergreen_create_sampler_view;
	rctx->context.create_vertex_elements_state = si_create_vertex_elements;
	rctx->context.create_vs_state = si_create_shader_state;
	rctx->context.bind_fragment_sampler_states = evergreen_bind_ps_sampler;
	rctx->context.bind_fs_state = r600_bind_ps_shader;
	rctx->context.bind_vertex_elements_state = r600_bind_vertex_elements;
	rctx->context.bind_vertex_sampler_states = evergreen_bind_vs_sampler;
	rctx->context.bind_vs_state = r600_bind_vs_shader;
	rctx->context.delete_fs_state = r600_delete_ps_shader;
	rctx->context.delete_sampler_state = si_delete_sampler_state;
	rctx->context.delete_vertex_elements_state = r600_delete_vertex_element;
	rctx->context.delete_vs_state = r600_delete_vs_shader;
	rctx->context.set_constant_buffer = r600_set_constant_buffer;
	rctx->context.set_fragment_sampler_views = evergreen_set_ps_sampler_view;
	rctx->context.set_polygon_stipple = evergreen_set_polygon_stipple;
	rctx->context.set_sample_mask = evergreen_set_sample_mask;
	rctx->context.set_vertex_buffers = r600_set_vertex_buffers;
	rctx->context.set_index_buffer = r600_set_index_buffer;
	rctx->context.set_vertex_sampler_views = evergreen_set_vs_sampler_view;
	rctx->context.sampler_view_destroy = r600_sampler_view_destroy;
	rctx->context.texture_barrier = r600_texture_barrier;
	rctx->context.create_stream_output_target = r600_create_so_target;
	rctx->context.stream_output_target_destroy = r600_so_target_destroy;
	rctx->context.set_stream_output_targets = r600_set_so_targets;
}

void si_init_config(struct r600_context *rctx)
{
  	struct r600_pipe_state *rstate = &rctx->config;
	unsigned tmp;

	r600_pipe_state_add_reg(rstate, R_028A4C_PA_SC_MODE_CNTL_1, 0x0, NULL, 0);

	r600_pipe_state_add_reg(rstate, R_028A10_VGT_OUTPUT_PATH_CNTL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A14_VGT_HOS_CNTL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A18_VGT_HOS_MAX_TESS_LEVEL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A1C_VGT_HOS_MIN_TESS_LEVEL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A20_VGT_HOS_REUSE_DEPTH, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A24_VGT_GROUP_PRIM_TYPE, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A28_VGT_GROUP_FIRST_DECR, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A2C_VGT_GROUP_DECR, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A30_VGT_GROUP_VECT_0_CNTL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A34_VGT_GROUP_VECT_1_CNTL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A38_VGT_GROUP_VECT_0_FMT_CNTL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A40_VGT_GS_MODE, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A84_VGT_PRIMITIVEID_EN, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028A8C_VGT_PRIMITIVEID_RESET, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028B94_VGT_STRMOUT_CONFIG, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028B98_VGT_STRMOUT_BUFFER_CONFIG, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028AA8_IA_MULTI_VGT_PARAM, S_028AA8_SWITCH_ON_EOP(1) | S_028AA8_PARTIAL_VS_WAVE_ON(1) | S_028AA8_PRIMGROUP_SIZE(63), NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028AB4_VGT_REUSE_OFF, 0x00000000, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028AB8_VGT_VTX_CNT_EN, 0x0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_008A14_PA_CL_ENHANCE, (3 << 1) | 1, NULL, 0);

	r600_pipe_state_add_reg(rstate, R_028B54_VGT_SHADER_STAGES_EN, 0, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028BD4_PA_SC_CENTROID_PRIORITY_0, 0x76543210, NULL, 0);
	r600_pipe_state_add_reg(rstate, R_028BD8_PA_SC_CENTROID_PRIORITY_1, 0xfedcba98, NULL, 0);

	r600_pipe_state_add_reg(rstate, R_028804_DB_EQAA, 0x110000, NULL, 0);
	r600_context_pipe_state_set(rctx, rstate);
}

void si_pipe_shader_ps(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, exports_ps, num_cout, spi_ps_in_control, db_shader_control;
	unsigned num_sgprs, num_user_sgprs;
	int pos_index = -1, face_index = -1;
	int ninterp = 0;
	boolean have_linear = FALSE, have_centroid = FALSE, have_perspective = FALSE;
	unsigned spi_baryc_cntl;
	uint64_t va;

	if (si_pipe_shader_create(ctx, shader))
		return;

	rstate->nregs = 0;

	db_shader_control = S_02880C_Z_ORDER(V_02880C_EARLY_Z_THEN_LATE_Z);
	for (i = 0; i < rshader->ninput; i++) {
		ninterp++;
		/* XXX: Flat shading hangs the GPU */
		if (rshader->input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
		    (rshader->input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
		     rctx->queued.named.rasterizer->flatshade))
			have_linear = TRUE;
		if (rshader->input[i].interpolate == TGSI_INTERPOLATE_LINEAR)
			have_linear = TRUE;
		if (rshader->input[i].interpolate == TGSI_INTERPOLATE_PERSPECTIVE)
			have_perspective = TRUE;
		if (rshader->input[i].centroid)
			have_centroid = TRUE;
	}

	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			db_shader_control |= S_02880C_Z_EXPORT_ENABLE(1);
		if (rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			db_shader_control |= 0; // XXX OP_VAL or TEST_VAL?
	}
	if (rshader->uses_kill)
		db_shader_control |= S_02880C_KILL_ENABLE(1);

	exports_ps = 0;
	num_cout = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION ||
		    rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			exports_ps |= 1;
		else if (rshader->output[i].name == TGSI_SEMANTIC_COLOR) {
			if (rshader->fs_write_all)
				num_cout = rshader->nr_cbufs;
			else
				num_cout++;
		}
	}
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}

	spi_ps_in_control = S_0286D8_NUM_INTERP(ninterp);

	spi_baryc_cntl = 0;
	if (have_perspective)
		spi_baryc_cntl |= have_centroid ?
			S_0286E0_PERSP_CENTROID_CNTL(1) : S_0286E0_PERSP_CENTER_CNTL(1);
	if (have_linear)
		spi_baryc_cntl |= have_centroid ?
			S_0286E0_LINEAR_CENTROID_CNTL(1) : S_0286E0_LINEAR_CENTER_CNTL(1);

	r600_pipe_state_add_reg(rstate,
				R_0286E0_SPI_BARYC_CNTL,
				spi_baryc_cntl,
				NULL, 0);

	r600_pipe_state_add_reg(rstate,
				R_0286CC_SPI_PS_INPUT_ENA,
				shader->spi_ps_input_ena,
				NULL, 0);

	r600_pipe_state_add_reg(rstate,
				R_0286D0_SPI_PS_INPUT_ADDR,
				shader->spi_ps_input_ena,
				NULL, 0);

	r600_pipe_state_add_reg(rstate,
				R_0286D8_SPI_PS_IN_CONTROL,
				spi_ps_in_control,
				NULL, 0);

	/* XXX: Depends on Z buffer format? */
	r600_pipe_state_add_reg(rstate,
				R_028710_SPI_SHADER_Z_FORMAT,
				0,
				NULL, 0);

	/* XXX: Depends on color buffer format? */
	r600_pipe_state_add_reg(rstate,
				R_028714_SPI_SHADER_COL_FORMAT,
				S_028714_COL0_EXPORT_FORMAT(V_028714_SPI_SHADER_32_ABGR),
				NULL, 0);

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	r600_pipe_state_add_reg(rstate,
				R_00B020_SPI_SHADER_PGM_LO_PS,
				va >> 8,
				shader->bo, RADEON_USAGE_READ);
	r600_pipe_state_add_reg(rstate,
				R_00B024_SPI_SHADER_PGM_HI_PS,
				va >> 40,
				shader->bo, RADEON_USAGE_READ);

	num_user_sgprs = 6;
	num_sgprs = shader->num_sgprs;
	if (num_user_sgprs > num_sgprs)
		num_sgprs = num_user_sgprs;
	/* Last 2 reserved SGPRs are used for VCC */
	num_sgprs += 2;
	assert(num_sgprs <= 104);

	r600_pipe_state_add_reg(rstate,
				R_00B028_SPI_SHADER_PGM_RSRC1_PS,
				S_00B028_VGPRS((shader->num_vgprs - 1) / 4) |
				S_00B028_SGPRS((num_sgprs - 1) / 8),
				NULL, 0);
	r600_pipe_state_add_reg(rstate,
				R_00B02C_SPI_SHADER_PGM_RSRC2_PS,
				S_00B02C_USER_SGPR(num_user_sgprs),
				NULL, 0);

	r600_pipe_state_add_reg(rstate, R_02880C_DB_SHADER_CONTROL,
				db_shader_control,
				NULL, 0);

	shader->sprite_coord_enable = rctx->sprite_coord_enable;
}

void si_pipe_shader_vs(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_pipe_state *rstate = &shader->rstate;
	struct r600_shader *rshader = &shader->shader;
	unsigned num_sgprs, num_user_sgprs;
	unsigned nparams, i;
	uint64_t va;

	if (si_pipe_shader_create(ctx, shader))
		return;

	/* clear previous register */
	rstate->nregs = 0;

	/* Certain attributes (position, psize, etc.) don't count as params.
	 * VS is required to export at least one param and r600_shader_from_tgsi()
	 * takes care of adding a dummy export.
	 */
	for (nparams = 0, i = 0 ; i < rshader->noutput; i++) {
		if (rshader->output[i].name != TGSI_SEMANTIC_POSITION)
			nparams++;
	}
	if (nparams < 1)
		nparams = 1;

	r600_pipe_state_add_reg(rstate,
			R_0286C4_SPI_VS_OUT_CONFIG,
			S_0286C4_VS_EXPORT_COUNT(nparams - 1),
			NULL, 0);

	r600_pipe_state_add_reg(rstate,
				R_02870C_SPI_SHADER_POS_FORMAT,
				S_02870C_POS0_EXPORT_FORMAT(V_02870C_SPI_SHADER_4COMP) |
				S_02870C_POS1_EXPORT_FORMAT(V_02870C_SPI_SHADER_NONE) |
				S_02870C_POS2_EXPORT_FORMAT(V_02870C_SPI_SHADER_NONE) |
				S_02870C_POS3_EXPORT_FORMAT(V_02870C_SPI_SHADER_NONE),
				NULL, 0);

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	r600_pipe_state_add_reg(rstate,
				R_00B120_SPI_SHADER_PGM_LO_VS,
				va >> 8,
				shader->bo, RADEON_USAGE_READ);
	r600_pipe_state_add_reg(rstate,
				R_00B124_SPI_SHADER_PGM_HI_VS,
				va >> 40,
				shader->bo, RADEON_USAGE_READ);

	num_user_sgprs = 8;
	num_sgprs = shader->num_sgprs;
	if (num_user_sgprs > num_sgprs)
		num_sgprs = num_user_sgprs;
	/* Last 2 reserved SGPRs are used for VCC */
	num_sgprs += 2;
	assert(num_sgprs <= 104);

	r600_pipe_state_add_reg(rstate,
				R_00B128_SPI_SHADER_PGM_RSRC1_VS,
				S_00B128_VGPRS((shader->num_vgprs - 1) / 4) |
				S_00B128_SGPRS((num_sgprs - 1) / 8),
				NULL, 0);
	r600_pipe_state_add_reg(rstate,
				R_00B12C_SPI_SHADER_PGM_RSRC2_VS,
				S_00B12C_USER_SGPR(num_user_sgprs),
				NULL, 0);
}

void si_update_spi_map(struct r600_context *rctx)
{
	struct r600_shader *ps = &rctx->ps_shader->shader;
	struct r600_shader *vs = &rctx->vs_shader->shader;
	struct r600_pipe_state *rstate = &rctx->spi;
	unsigned i, j, tmp;

	rstate->nregs = 0;

	for (i = 0; i < ps->ninput; i++) {
		tmp = 0;

#if 0
		/* XXX: Flat shading hangs the GPU */
		if (ps->input[i].name == TGSI_SEMANTIC_POSITION ||
		    ps->input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
		    (ps->input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
		     rctx->rasterizer && rctx->rasterizer->flatshade)) {
			tmp |= S_028644_FLAT_SHADE(1);
		}
#endif

		if (ps->input[i].name == TGSI_SEMANTIC_GENERIC &&
		    rctx->sprite_coord_enable & (1 << ps->input[i].sid)) {
			tmp |= S_028644_PT_SPRITE_TEX(1);
		}

		for (j = 0; j < vs->noutput; j++) {
			if (ps->input[i].name == vs->output[j].name &&
			    ps->input[i].sid == vs->output[j].sid) {
				tmp |= S_028644_OFFSET(vs->output[j].param_offset);
				break;
			}
		}

		if (j == vs->noutput) {
			/* No corresponding output found, load defaults into input */
			tmp |= S_028644_OFFSET(0x20);
		}

		r600_pipe_state_add_reg(rstate, R_028644_SPI_PS_INPUT_CNTL_0 + i * 4,
					tmp, NULL, 0);
	}

	if (rstate->nregs > 0)
		r600_context_pipe_state_set(rctx, rstate);
}
