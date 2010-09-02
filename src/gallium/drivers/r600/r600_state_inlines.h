/*
 * Copyright 2010 Red Hat Inc.
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
#ifndef R600_STATE_INLINES_H
#define R600_STATE_INLINES_H

#include "util/u_format.h"
#include "r600d.h"

static INLINE uint32_t r600_translate_blend_function(int blend_func)
{
	switch (blend_func) {
	case PIPE_BLEND_ADD:
		return V_028804_COMB_DST_PLUS_SRC;
	case PIPE_BLEND_SUBTRACT:
		return V_028804_COMB_SRC_MINUS_DST;
	case PIPE_BLEND_REVERSE_SUBTRACT:
		return V_028804_COMB_DST_MINUS_SRC;
	case PIPE_BLEND_MIN:
		return V_028804_COMB_MIN_DST_SRC;
	case PIPE_BLEND_MAX:
		return V_028804_COMB_MAX_DST_SRC;
	default:
		R600_ERR("Unknown blend function %d\n", blend_func);
		assert(0);
		break;
	}
	return 0;
}

static INLINE uint32_t r600_translate_blend_factor(int blend_fact)
{
	switch (blend_fact) {
	case PIPE_BLENDFACTOR_ONE:
		return V_028804_BLEND_ONE;
	case PIPE_BLENDFACTOR_SRC_COLOR:
		return V_028804_BLEND_SRC_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA:
		return V_028804_BLEND_SRC_ALPHA;
	case PIPE_BLENDFACTOR_DST_ALPHA:
		return V_028804_BLEND_DST_ALPHA;
	case PIPE_BLENDFACTOR_DST_COLOR:
		return V_028804_BLEND_DST_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return V_028804_BLEND_SRC_ALPHA_SATURATE;
	case PIPE_BLENDFACTOR_CONST_COLOR:
		return V_028804_BLEND_CONST_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return V_028804_BLEND_CONST_ALPHA;
	case PIPE_BLENDFACTOR_ZERO:
		return V_028804_BLEND_ZERO;
	case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return V_028804_BLEND_ONE_MINUS_SRC_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return V_028804_BLEND_ONE_MINUS_SRC_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return V_028804_BLEND_ONE_MINUS_DST_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return V_028804_BLEND_ONE_MINUS_DST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return V_028804_BLEND_ONE_MINUS_CONST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return V_028804_BLEND_ONE_MINUS_CONST_ALPHA;
	case PIPE_BLENDFACTOR_SRC1_COLOR:
		return V_028804_BLEND_SRC1_COLOR;
	case PIPE_BLENDFACTOR_SRC1_ALPHA:
		return V_028804_BLEND_SRC1_ALPHA;
	case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
		return V_028804_BLEND_INV_SRC1_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
		return V_028804_BLEND_INV_SRC1_ALPHA;
	default:
		R600_ERR("Bad blend factor %d not supported!\n", blend_fact);
		assert(0);
		break;
	}
	return 0;
}

static INLINE uint32_t r600_translate_stencil_op(int s_op)
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

/* translates straight */
static INLINE uint32_t r600_translate_ds_func(int func)
{
	return func;
}

static uint32_t r600_translate_dbformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		return V_028010_DEPTH_16;
	case PIPE_FORMAT_Z24X8_UNORM:
		return V_028010_DEPTH_X8_24;
	case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
		return V_028010_DEPTH_8_24;
	default:
		return ~0;
	}
}

static uint32_t r600_translate_colorswap(enum pipe_format format)
{
	switch (format) {
		/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
		return V_0280A0_SWAP_STD;

		/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_0280A0_SWAP_STD_REV;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_0280A0_SWAP_ALT;
		/* 32-bit buffers. */

	case PIPE_FORMAT_A8B8G8R8_SRGB:
		return V_0280A0_SWAP_STD_REV;
	case PIPE_FORMAT_B8G8R8A8_SRGB:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return V_0280A0_SWAP_ALT;

	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		return V_0280A0_SWAP_ALT_REV;
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
		//        case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
		return V_0280A0_SWAP_STD_REV;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
		return V_0280A0_SWAP_STD;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_0280A0_SWAP_STD_REV;

		/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
		//		return V_0280A0_COLOR_16_16_16_16;
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
		//		return V_0280A0_COLOR_16_16_16_16_FLOAT;

		/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		//		return V_0280A0_COLOR_32_32_32_32_FLOAT;
		return 0;
	default:
		R600_ERR("unsupported colorswap format %d\n", format);
		return ~0;
	}
	return ~0;
}

static INLINE uint32_t r600_translate_colorformat(enum pipe_format format)
{
	switch (format) {
		/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8_SNORM:
		return V_0280A0_COLOR_8;

		/* 16-bit buffers. */
	case PIPE_FORMAT_B5G6R5_UNORM:
		return V_0280A0_COLOR_5_6_5;

	case PIPE_FORMAT_B5G5R5A1_UNORM:
	case PIPE_FORMAT_B5G5R5X1_UNORM:
		return V_0280A0_COLOR_1_5_5_5;

	case PIPE_FORMAT_B4G4R4A4_UNORM:
	case PIPE_FORMAT_B4G4R4X4_UNORM:
		return V_0280A0_COLOR_4_4_4_4;

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
		return V_0280A0_COLOR_8_8_8_8;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_0280A0_COLOR_10_10_10_2;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
		return V_0280A0_COLOR_8_24;

	case PIPE_FORMAT_R32_FLOAT:
		return V_0280A0_COLOR_32_FLOAT;

		/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
		return V_0280A0_COLOR_16_16_16_16;
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
		return V_0280A0_COLOR_16_16_16_16_FLOAT;
	case PIPE_FORMAT_R32G32_FLOAT:
		return V_0280A0_COLOR_32_32_FLOAT;

		/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32_FLOAT:
	  	return V_0280A0_COLOR_32_32_32_FLOAT;
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		return V_0280A0_COLOR_32_32_32_32_FLOAT;

		/* YUV buffers. */
	case PIPE_FORMAT_UYVY:
	case PIPE_FORMAT_YUYV:
	default:
		R600_ERR("unsupported color format %d\n", format);
		return ~0; /* Unsupported. */
	}
}

static INLINE boolean r600_is_sampler_format_supported(enum pipe_format format)
{
	return r600_translate_texformat(format, NULL, NULL, NULL) != ~0;
}

static INLINE boolean r600_is_colorbuffer_format_supported(enum pipe_format format)
{
	return r600_translate_colorformat(format) != ~0 &&
		r600_translate_colorswap(format) != ~0;
}

static INLINE boolean r600_is_zs_format_supported(enum pipe_format format)
{
	return r600_translate_dbformat(format) != ~0;
}

static INLINE boolean r600_is_vertex_format_supported(enum pipe_format format)
{
	return r600_translate_colorformat(format) != ~0;
}

#endif
