/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
#include "util/u_format.h"

#include "fd3_util.h"

/* convert pipe format to vertex buffer format: */
enum a3xx_vtx_fmt
fd3_pipe2vtx(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_L8_SRGB:
		return VFMT_NORM_UBYTE_8;

	case PIPE_FORMAT_A8_SNORM:
	case PIPE_FORMAT_I8_SNORM:
	case PIPE_FORMAT_L8_SNORM:
	case PIPE_FORMAT_R8_SNORM:
		return VFMT_NORM_BYTE_8;

	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_R8_UINT:
		return VFMT_UBYTE_8;

	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_R8_SINT:
		return VFMT_BYTE_8;

	/* 16-bit buffers. */
	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_A16_UNORM:
	case PIPE_FORMAT_L16_UNORM:
	case PIPE_FORMAT_I16_UNORM:
	case PIPE_FORMAT_Z16_UNORM:
		return VFMT_NORM_USHORT_16;

	case PIPE_FORMAT_R16_SNORM:
	case PIPE_FORMAT_A16_SNORM:
	case PIPE_FORMAT_L16_SNORM:
	case PIPE_FORMAT_I16_SNORM:
		return VFMT_NORM_SHORT_16;

	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_A16_UINT:
	case PIPE_FORMAT_L16_UINT:
	case PIPE_FORMAT_I16_UINT:
		return VFMT_USHORT_16;

	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_A16_SINT:
	case PIPE_FORMAT_L16_SINT:
	case PIPE_FORMAT_I16_SINT:
		return VFMT_SHORT_16;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_R8G8_UNORM:
		return VFMT_NORM_UBYTE_8_8;

	case PIPE_FORMAT_L8A8_SNORM:
	case PIPE_FORMAT_R8G8_SNORM:
		return VFMT_NORM_BYTE_8_8;

	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_R8G8_UINT:
		return VFMT_UBYTE_8_8;

	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_R8G8_SINT:
		return VFMT_BYTE_8_8;

	/* 24-bit buffers. */
	case PIPE_FORMAT_R8G8B8_UNORM:
		return VFMT_NORM_UBYTE_8_8_8;

	case PIPE_FORMAT_R8G8B8_SNORM:
		return VFMT_NORM_BYTE_8_8_8;

	case PIPE_FORMAT_R8G8B8_UINT:
		return VFMT_UBYTE_8_8_8;

	case PIPE_FORMAT_R8G8B8_SINT:
		return VFMT_BYTE_8_8_8;

	/* 32-bit buffers. */
	case PIPE_FORMAT_A8B8G8R8_UNORM:
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
	case PIPE_FORMAT_A8B8G8R8_SRGB:
	case PIPE_FORMAT_B8G8R8A8_SRGB:
		return VFMT_NORM_UBYTE_8_8_8_8;

	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8X8_SNORM:
		return VFMT_NORM_BYTE_8_8_8_8;

	case PIPE_FORMAT_R8G8B8A8_UINT:
	case PIPE_FORMAT_R8G8B8X8_UINT:
		return VFMT_UBYTE_8_8_8_8;

	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8X8_SINT:
		return VFMT_BYTE_8_8_8_8;

/* TODO probably need gles3 blob drivers to find the 32bit int formats:
	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_A32_UINT:
	case PIPE_FORMAT_A32_SINT:
	case PIPE_FORMAT_L32_UINT:
	case PIPE_FORMAT_L32_SINT:
	case PIPE_FORMAT_I32_UINT:
	case PIPE_FORMAT_I32_SINT:
*/

	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_A32_FLOAT:
	case PIPE_FORMAT_L32_FLOAT:
	case PIPE_FORMAT_I32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return VFMT_FLOAT_32;

	case PIPE_FORMAT_R32_FIXED:
		return VFMT_FIXED_32;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16A16_UNORM:
		return VFMT_NORM_USHORT_16_16_16_16;

	case PIPE_FORMAT_R16G16B16A16_SNORM:
		return VFMT_NORM_SHORT_16_16_16_16;

	case PIPE_FORMAT_R16G16B16A16_UINT:
		return VFMT_USHORT_16_16_16_16;

	case PIPE_FORMAT_R16G16B16A16_SINT:
		return VFMT_SHORT_16_16_16_16;

	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_L32A32_FLOAT:
		return VFMT_FLOAT_32_32;

	case PIPE_FORMAT_R32G32_FIXED:
		return VFMT_FIXED_32_32;

/* TODO probably need gles3 blob drivers to find the 32bit int formats:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_L32A32_UINT:
	case PIPE_FORMAT_L32A32_SINT:
*/

	/* 96-bit buffers. */
	case PIPE_FORMAT_R32G32B32_FLOAT:
		return VFMT_FLOAT_32_32_32;

	case PIPE_FORMAT_R32G32B32_FIXED:
		return VFMT_FIXED_32_32_32;

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		return VFMT_FLOAT_32_32_32_32;

	case PIPE_FORMAT_R32G32B32A32_FIXED:
		return VFMT_FIXED_32_32_32_32;

/* TODO probably need gles3 blob drivers to find the 32bit int formats:
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
*/

	default:
		return ~0;
	}
}

/* convert pipe format to texture sampler format: */
enum a3xx_tex_fmt
fd3_pipe2tex(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
		return TFMT_NORM_UINT_8;

	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return TFMT_NORM_UINT_8_8_8_8;

	case PIPE_FORMAT_Z24X8_UNORM:
		return TFMT_NORM_UINT_X8Z24;

	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return TFMT_NORM_UINT_8_8_8_8;

	case PIPE_FORMAT_Z16_UNORM:
		return TFMT_NORM_UINT_8_8;

	// TODO add more..

	default:
		return ~0;
	}
}

enum a3xx_tex_fetchsize
fd3_pipe2fetchsize(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
		return TFETCH_1_BYTE;

	case PIPE_FORMAT_Z16_UNORM:
		return TFETCH_2_BYTE;

	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return TFETCH_4_BYTE;

	// TODO add more..

	default:
		return TFETCH_DISABLE;  /* save default */
	}
}

/* convert pipe format to MRT / copydest format used for render-target: */
enum a3xx_color_fmt
fd3_pipe2color(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return RB_R8G8B8A8_UNORM;

	case PIPE_FORMAT_Z16_UNORM:
		return RB_Z16_UNORM;

	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		/* for DEPTHX_24_8, blob driver also seems to use R8G8B8A8 fmt.. */
		return RB_R8G8B8A8_UNORM;

	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_L8_UNORM:
		return RB_A8_UNORM;

	// TODO add more..

	default:
		return ~0;
	}
}

enum a3xx_color_swap
fd3_pipe2swap(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return WXYZ;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
		return WZYX;

	default:
		return WZYX;
	}
}

static inline enum a3xx_tex_swiz
tex_swiz(unsigned swiz)
{
	switch (swiz) {
	default:
	case PIPE_SWIZZLE_RED:   return A3XX_TEX_X;
	case PIPE_SWIZZLE_GREEN: return A3XX_TEX_Y;
	case PIPE_SWIZZLE_BLUE:  return A3XX_TEX_Z;
	case PIPE_SWIZZLE_ALPHA: return A3XX_TEX_W;
	case PIPE_SWIZZLE_ZERO:  return A3XX_TEX_ZERO;
	case PIPE_SWIZZLE_ONE:   return A3XX_TEX_ONE;
	}
}

uint32_t
fd3_tex_swiz(enum pipe_format format, unsigned swizzle_r, unsigned swizzle_g,
		unsigned swizzle_b, unsigned swizzle_a)
{
	const struct util_format_description *desc =
			util_format_description(format);
	uint8_t swiz[] = {
			swizzle_r, swizzle_g, swizzle_b, swizzle_a,
			PIPE_SWIZZLE_ZERO, PIPE_SWIZZLE_ONE,
			PIPE_SWIZZLE_ONE, PIPE_SWIZZLE_ONE,
	};

	return A3XX_TEX_CONST_0_SWIZ_X(tex_swiz(swiz[desc->swizzle[0]])) |
			A3XX_TEX_CONST_0_SWIZ_Y(tex_swiz(swiz[desc->swizzle[1]])) |
			A3XX_TEX_CONST_0_SWIZ_Z(tex_swiz(swiz[desc->swizzle[2]])) |
			A3XX_TEX_CONST_0_SWIZ_W(tex_swiz(swiz[desc->swizzle[3]]));
}
