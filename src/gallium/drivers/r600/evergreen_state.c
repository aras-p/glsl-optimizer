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
#include "r600_formats.h"
#include "r600_shader.h"
#include "evergreend.h"

#include "pipe/p_shader_tokens.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_framebuffer.h"
#include "util/u_dual_blend.h"
#include "evergreen_compute.h"
#include "util/u_math.h"

static INLINE unsigned evergreen_array_mode(unsigned mode)
{
	switch (mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:	return V_028C70_ARRAY_LINEAR_ALIGNED;
		break;
	case RADEON_SURF_MODE_1D:		return V_028C70_ARRAY_1D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_2D:		return V_028C70_ARRAY_2D_TILED_THIN1;
	default:
	case RADEON_SURF_MODE_LINEAR:		return V_028C70_ARRAY_LINEAR_GENERAL;
	}
}

static uint32_t eg_num_banks(uint32_t nbanks)
{
	switch (nbanks) {
	case 2:
		return 0;
	case 4:
		return 1;
	case 8:
	default:
		return 2;
	case 16:
		return 3;
	}
}


static unsigned eg_tile_split(unsigned tile_split)
{
	switch (tile_split) {
	case 64:	tile_split = 0;	break;
	case 128:	tile_split = 1;	break;
	case 256:	tile_split = 2;	break;
	case 512:	tile_split = 3;	break;
	default:
	case 1024:	tile_split = 4;	break;
	case 2048:	tile_split = 5;	break;
	case 4096:	tile_split = 6;	break;
	}
	return tile_split;
}

static unsigned eg_macro_tile_aspect(unsigned macro_tile_aspect)
{
	switch (macro_tile_aspect) {
	default:
	case 1:	macro_tile_aspect = 0;	break;
	case 2:	macro_tile_aspect = 1;	break;
	case 4:	macro_tile_aspect = 2;	break;
	case 8:	macro_tile_aspect = 3;	break;
	}
	return macro_tile_aspect;
}

static unsigned eg_bank_wh(unsigned bankwh)
{
	switch (bankwh) {
	default:
	case 1:	bankwh = 0;	break;
	case 2:	bankwh = 1;	break;
	case 4:	bankwh = 2;	break;
	case 8:	bankwh = 3;	break;
	}
	return bankwh;
}

static uint32_t r600_translate_blend_function(int blend_func)
{
	switch (blend_func) {
	case PIPE_BLEND_ADD:
		return V_028780_COMB_DST_PLUS_SRC;
	case PIPE_BLEND_SUBTRACT:
		return V_028780_COMB_SRC_MINUS_DST;
	case PIPE_BLEND_REVERSE_SUBTRACT:
		return V_028780_COMB_DST_MINUS_SRC;
	case PIPE_BLEND_MIN:
		return V_028780_COMB_MIN_DST_SRC;
	case PIPE_BLEND_MAX:
		return V_028780_COMB_MAX_DST_SRC;
	default:
		R600_ERR("Unknown blend function %d\n", blend_func);
		assert(0);
		break;
	}
	return 0;
}

static uint32_t r600_translate_blend_factor(int blend_fact)
{
	switch (blend_fact) {
	case PIPE_BLENDFACTOR_ONE:
		return V_028780_BLEND_ONE;
	case PIPE_BLENDFACTOR_SRC_COLOR:
		return V_028780_BLEND_SRC_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA:
		return V_028780_BLEND_SRC_ALPHA;
	case PIPE_BLENDFACTOR_DST_ALPHA:
		return V_028780_BLEND_DST_ALPHA;
	case PIPE_BLENDFACTOR_DST_COLOR:
		return V_028780_BLEND_DST_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return V_028780_BLEND_SRC_ALPHA_SATURATE;
	case PIPE_BLENDFACTOR_CONST_COLOR:
		return V_028780_BLEND_CONST_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return V_028780_BLEND_CONST_ALPHA;
	case PIPE_BLENDFACTOR_ZERO:
		return V_028780_BLEND_ZERO;
	case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return V_028780_BLEND_ONE_MINUS_SRC_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return V_028780_BLEND_ONE_MINUS_SRC_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return V_028780_BLEND_ONE_MINUS_DST_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return V_028780_BLEND_ONE_MINUS_DST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return V_028780_BLEND_ONE_MINUS_CONST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return V_028780_BLEND_ONE_MINUS_CONST_ALPHA;
	case PIPE_BLENDFACTOR_SRC1_COLOR:
		return V_028780_BLEND_SRC1_COLOR;
	case PIPE_BLENDFACTOR_SRC1_ALPHA:
		return V_028780_BLEND_SRC1_ALPHA;
	case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
		return V_028780_BLEND_INV_SRC1_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
		return V_028780_BLEND_INV_SRC1_ALPHA;
	default:
		R600_ERR("Bad blend factor %d not supported!\n", blend_fact);
		assert(0);
		break;
	}
	return 0;
}

static unsigned r600_tex_dim(unsigned dim, unsigned nr_samples)
{
	switch (dim) {
	default:
	case PIPE_TEXTURE_1D:
		return V_030000_SQ_TEX_DIM_1D;
	case PIPE_TEXTURE_1D_ARRAY:
		return V_030000_SQ_TEX_DIM_1D_ARRAY;
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_RECT:
		return nr_samples > 1 ? V_030000_SQ_TEX_DIM_2D_MSAA :
					V_030000_SQ_TEX_DIM_2D;
	case PIPE_TEXTURE_2D_ARRAY:
		return nr_samples > 1 ? V_030000_SQ_TEX_DIM_2D_ARRAY_MSAA :
					V_030000_SQ_TEX_DIM_2D_ARRAY;
	case PIPE_TEXTURE_3D:
		return V_030000_SQ_TEX_DIM_3D;
	case PIPE_TEXTURE_CUBE:
	case PIPE_TEXTURE_CUBE_ARRAY:
		return V_030000_SQ_TEX_DIM_CUBEMAP;
	}
}

static uint32_t r600_translate_dbformat(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		return V_028040_Z_16;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		return V_028040_Z_24;
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		return V_028040_Z_32_FLOAT;
	default:
		return ~0U;
	}
}

static uint32_t r600_translate_colorswap(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_L4A4_UNORM:
	case PIPE_FORMAT_A4R4_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_SNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_A16_UNORM:
	case PIPE_FORMAT_A16_SNORM:
	case PIPE_FORMAT_A16_UINT:
	case PIPE_FORMAT_A16_SINT:
	case PIPE_FORMAT_A16_FLOAT:
	case PIPE_FORMAT_A32_UINT:
	case PIPE_FORMAT_A32_SINT:
	case PIPE_FORMAT_A32_FLOAT:
	case PIPE_FORMAT_R4A4_UNORM:
		return V_028C70_SWAP_ALT_REV;
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_SNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_I16_UNORM:
	case PIPE_FORMAT_I16_SNORM:
	case PIPE_FORMAT_I16_UINT:
	case PIPE_FORMAT_I16_SINT:
	case PIPE_FORMAT_I16_FLOAT:
	case PIPE_FORMAT_I32_UINT:
	case PIPE_FORMAT_I32_SINT:
	case PIPE_FORMAT_I32_FLOAT:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_L8_SNORM:
	case PIPE_FORMAT_L8_UINT:
	case PIPE_FORMAT_L8_SINT:
	case PIPE_FORMAT_L8_SRGB:
	case PIPE_FORMAT_L16_UNORM:
	case PIPE_FORMAT_L16_SNORM:
	case PIPE_FORMAT_L16_UINT:
	case PIPE_FORMAT_L16_SINT:
	case PIPE_FORMAT_L16_FLOAT:
	case PIPE_FORMAT_L32_UINT:
	case PIPE_FORMAT_L32_SINT:
	case PIPE_FORMAT_L32_FLOAT:
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
	case PIPE_FORMAT_L8A8_SNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_L16A16_UNORM:
	case PIPE_FORMAT_L16A16_SNORM:
	case PIPE_FORMAT_L16A16_UINT:
	case PIPE_FORMAT_L16A16_SINT:
	case PIPE_FORMAT_L16A16_FLOAT:
	case PIPE_FORMAT_L32A32_UINT:
	case PIPE_FORMAT_L32A32_SINT:
	case PIPE_FORMAT_L32A32_FLOAT:
        case PIPE_FORMAT_R8A8_UNORM:
	case PIPE_FORMAT_R8A8_SNORM:
	case PIPE_FORMAT_R8A8_UINT:
	case PIPE_FORMAT_R8A8_SINT:
	case PIPE_FORMAT_R16A16_UNORM:
	case PIPE_FORMAT_R16A16_SNORM:
	case PIPE_FORMAT_R16A16_UINT:
	case PIPE_FORMAT_R16A16_SINT:
	case PIPE_FORMAT_R16A16_FLOAT:
	case PIPE_FORMAT_R32A32_UINT:
	case PIPE_FORMAT_R32A32_SINT:
	case PIPE_FORMAT_R32A32_FLOAT:
		return V_028C70_SWAP_ALT;
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_SNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_SNORM:
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
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
	case PIPE_FORMAT_R8G8B8X8_SNORM:
	case PIPE_FORMAT_R8G8B8X8_SRGB:
	case PIPE_FORMAT_R8G8B8X8_UINT:
	case PIPE_FORMAT_R8G8B8X8_SINT:
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
		return V_028C70_SWAP_STD_REV;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
		return V_028C70_SWAP_STD;

	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
	case PIPE_FORMAT_B10G10R10X2_UNORM:
		return V_028C70_SWAP_ALT;

	case PIPE_FORMAT_R11G11B10_FLOAT:
	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_SNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
		return V_028C70_SWAP_STD;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_FLOAT:
	case PIPE_FORMAT_R16G16B16X16_UNORM:
	case PIPE_FORMAT_R16G16B16X16_SNORM:
	case PIPE_FORMAT_R16G16B16X16_FLOAT:
	case PIPE_FORMAT_R16G16B16X16_UINT:
	case PIPE_FORMAT_R16G16B16X16_SINT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
	case PIPE_FORMAT_R32G32B32X32_FLOAT:
	case PIPE_FORMAT_R32G32B32X32_UINT:
	case PIPE_FORMAT_R32G32B32X32_SINT:
		return V_028C70_SWAP_STD;
	default:
		R600_ERR("unsupported colorswap format %d\n", format);
		return ~0U;
	}
	return ~0U;
}

static uint32_t r600_translate_colorformat(enum pipe_format format)
{
	switch (format) {
	/* 8-bit buffers. */
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_A8_SNORM:
	case PIPE_FORMAT_A8_UINT:
	case PIPE_FORMAT_A8_SINT:
	case PIPE_FORMAT_I8_UNORM:
	case PIPE_FORMAT_I8_SNORM:
	case PIPE_FORMAT_I8_UINT:
	case PIPE_FORMAT_I8_SINT:
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_L8_SNORM:
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

	case PIPE_FORMAT_Z16_UNORM:
		return V_028C70_COLOR_16;

	case PIPE_FORMAT_L8A8_UNORM:
	case PIPE_FORMAT_L8A8_SNORM:
	case PIPE_FORMAT_L8A8_UINT:
	case PIPE_FORMAT_L8A8_SINT:
	case PIPE_FORMAT_L8A8_SRGB:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8_SNORM:
	case PIPE_FORMAT_R8G8_UINT:
	case PIPE_FORMAT_R8G8_SINT:
        case PIPE_FORMAT_R8A8_UNORM:
	case PIPE_FORMAT_R8A8_SNORM:
	case PIPE_FORMAT_R8A8_UINT:
	case PIPE_FORMAT_R8A8_SINT:
		return V_028C70_COLOR_8_8;

	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16_SNORM:
	case PIPE_FORMAT_R16_UINT:
	case PIPE_FORMAT_R16_SINT:
	case PIPE_FORMAT_A16_UNORM:
	case PIPE_FORMAT_A16_SNORM:
	case PIPE_FORMAT_A16_UINT:
	case PIPE_FORMAT_A16_SINT:
	case PIPE_FORMAT_L16_UNORM:
	case PIPE_FORMAT_L16_SNORM:
	case PIPE_FORMAT_L16_UINT:
	case PIPE_FORMAT_L16_SINT:
	case PIPE_FORMAT_I16_UNORM:
	case PIPE_FORMAT_I16_SNORM:
	case PIPE_FORMAT_I16_UINT:
	case PIPE_FORMAT_I16_SINT:
		return V_028C70_COLOR_16;

	case PIPE_FORMAT_R16_FLOAT:
	case PIPE_FORMAT_A16_FLOAT:
	case PIPE_FORMAT_L16_FLOAT:
	case PIPE_FORMAT_I16_FLOAT:
		return V_028C70_COLOR_16_FLOAT;

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
	case PIPE_FORMAT_R8G8B8X8_SNORM:
	case PIPE_FORMAT_R8G8B8X8_SRGB:
	case PIPE_FORMAT_R8G8B8X8_UINT:
	case PIPE_FORMAT_R8G8B8X8_SINT:
	case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
	case PIPE_FORMAT_X8B8G8R8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_SINT:
	case PIPE_FORMAT_R8G8B8A8_UINT:
		return V_028C70_COLOR_8_8_8_8;

	case PIPE_FORMAT_R10G10B10A2_UNORM:
	case PIPE_FORMAT_R10G10B10X2_SNORM:
	case PIPE_FORMAT_B10G10R10A2_UNORM:
	case PIPE_FORMAT_B10G10R10A2_UINT:
	case PIPE_FORMAT_B10G10R10X2_UNORM:
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

	case PIPE_FORMAT_R32_UINT:
	case PIPE_FORMAT_R32_SINT:
	case PIPE_FORMAT_A32_UINT:
	case PIPE_FORMAT_A32_SINT:
	case PIPE_FORMAT_L32_UINT:
	case PIPE_FORMAT_L32_SINT:
	case PIPE_FORMAT_I32_UINT:
	case PIPE_FORMAT_I32_SINT:
		return V_028C70_COLOR_32;

	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_A32_FLOAT:
	case PIPE_FORMAT_L32_FLOAT:
	case PIPE_FORMAT_I32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT:
		return V_028C70_COLOR_32_FLOAT;

	case PIPE_FORMAT_R16G16_FLOAT:
	case PIPE_FORMAT_L16A16_FLOAT:
        case PIPE_FORMAT_R16A16_FLOAT:
		return V_028C70_COLOR_16_16_FLOAT;

	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16_SNORM:
	case PIPE_FORMAT_R16G16_UINT:
	case PIPE_FORMAT_R16G16_SINT:
	case PIPE_FORMAT_L16A16_UNORM:
	case PIPE_FORMAT_L16A16_SNORM:
	case PIPE_FORMAT_L16A16_UINT:
	case PIPE_FORMAT_L16A16_SINT:
        case PIPE_FORMAT_R16A16_UNORM:
	case PIPE_FORMAT_R16A16_SNORM:
	case PIPE_FORMAT_R16A16_UINT:
	case PIPE_FORMAT_R16A16_SINT:
		return V_028C70_COLOR_16_16;

	case PIPE_FORMAT_R11G11B10_FLOAT:
		return V_028C70_COLOR_10_11_11_FLOAT;

	/* 64-bit buffers. */
	case PIPE_FORMAT_R16G16B16A16_UINT:
	case PIPE_FORMAT_R16G16B16A16_SINT:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16G16B16X16_UNORM:
	case PIPE_FORMAT_R16G16B16X16_SNORM:
	case PIPE_FORMAT_R16G16B16X16_UINT:
	case PIPE_FORMAT_R16G16B16X16_SINT:
		return V_028C70_COLOR_16_16_16_16;

	case PIPE_FORMAT_R16G16B16A16_FLOAT:
	case PIPE_FORMAT_R16G16B16X16_FLOAT:
		return V_028C70_COLOR_16_16_16_16_FLOAT;

	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_L32A32_FLOAT:
        case PIPE_FORMAT_R32A32_FLOAT:
		return V_028C70_COLOR_32_32_FLOAT;

	case PIPE_FORMAT_R32G32_SINT:
	case PIPE_FORMAT_R32G32_UINT:
	case PIPE_FORMAT_L32A32_UINT:
	case PIPE_FORMAT_L32A32_SINT:
		return V_028C70_COLOR_32_32;

	/* 128-bit buffers. */
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_SINT:
	case PIPE_FORMAT_R32G32B32A32_UINT:
	case PIPE_FORMAT_R32G32B32X32_UINT:
	case PIPE_FORMAT_R32G32B32X32_SINT:
		return V_028C70_COLOR_32_32_32_32;
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32X32_FLOAT:
		return V_028C70_COLOR_32_32_32_32_FLOAT;

	/* YUV buffers. */
	case PIPE_FORMAT_UYVY:
	case PIPE_FORMAT_YUYV:
	default:
		return ~0U; /* Unsupported. */
	}
}

static uint32_t r600_colorformat_endian_swap(uint32_t colorformat)
{
	if (R600_BIG_ENDIAN) {
		switch(colorformat) {

		/* 8-bit buffers. */
		case V_028C70_COLOR_8:
			return ENDIAN_NONE;

		/* 16-bit buffers. */
		case V_028C70_COLOR_5_6_5:
		case V_028C70_COLOR_1_5_5_5:
		case V_028C70_COLOR_4_4_4_4:
		case V_028C70_COLOR_16:
		case V_028C70_COLOR_8_8:
			return ENDIAN_8IN16;

		/* 32-bit buffers. */
		case V_028C70_COLOR_8_8_8_8:
		case V_028C70_COLOR_2_10_10_10:
		case V_028C70_COLOR_8_24:
		case V_028C70_COLOR_24_8:
		case V_028C70_COLOR_32_FLOAT:
		case V_028C70_COLOR_16_16_FLOAT:
		case V_028C70_COLOR_16_16:
			return ENDIAN_8IN32;

		/* 64-bit buffers. */
		case V_028C70_COLOR_16_16_16_16:
		case V_028C70_COLOR_16_16_16_16_FLOAT:
			return ENDIAN_8IN16;

		case V_028C70_COLOR_32_32_FLOAT:
		case V_028C70_COLOR_32_32:
		case V_028C70_COLOR_X24_8_32_FLOAT:
			return ENDIAN_8IN32;

		/* 96-bit buffers. */
		case V_028C70_COLOR_32_32_32_FLOAT:
		/* 128-bit buffers. */
		case V_028C70_COLOR_32_32_32_32_FLOAT:
		case V_028C70_COLOR_32_32_32_32:
			return ENDIAN_8IN32;
		default:
			return ENDIAN_NONE; /* Unsupported. */
		}
	} else {
		return ENDIAN_NONE;
	}
}

static bool r600_is_sampler_format_supported(struct pipe_screen *screen, enum pipe_format format)
{
	return r600_translate_texformat(screen, format, NULL, NULL, NULL) != ~0U;
}

static bool r600_is_colorbuffer_format_supported(enum pipe_format format)
{
	return r600_translate_colorformat(format) != ~0U &&
		r600_translate_colorswap(format) != ~0U;
}

static bool r600_is_zs_format_supported(enum pipe_format format)
{
	return r600_translate_dbformat(format) != ~0U;
}

boolean evergreen_is_format_supported(struct pipe_screen *screen,
				      enum pipe_format format,
				      enum pipe_texture_target target,
				      unsigned sample_count,
				      unsigned usage)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	unsigned retval = 0;

	if (target >= PIPE_MAX_TEXTURE_TYPES) {
		R600_ERR("r600: unsupported texture type %d\n", target);
		return FALSE;
	}

	if (!util_format_is_supported(format, usage))
		return FALSE;

	if (sample_count > 1) {
		if (!rscreen->has_msaa)
			return FALSE;

		switch (sample_count) {
		case 2:
		case 4:
		case 8:
			break;
		default:
			return FALSE;
		}
	}

	if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
	    r600_is_sampler_format_supported(screen, format)) {
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
	    r600_is_vertex_format_supported(format)) {
		retval |= PIPE_BIND_VERTEX_BUFFER;
	}

	if (usage & PIPE_BIND_TRANSFER_READ)
		retval |= PIPE_BIND_TRANSFER_READ;
	if (usage & PIPE_BIND_TRANSFER_WRITE)
		retval |= PIPE_BIND_TRANSFER_WRITE;

	return retval == usage;
}

static void *evergreen_create_blend_state_mode(struct pipe_context *ctx,
					       const struct pipe_blend_state *state, int mode)
{
	uint32_t color_control = 0, target_mask = 0;
	struct r600_blend_state *blend = CALLOC_STRUCT(r600_blend_state);

	if (!blend) {
		return NULL;
	}

	r600_init_command_buffer(&blend->buffer, 20);
	r600_init_command_buffer(&blend->buffer_no_blend, 20);

	if (state->logicop_enable) {
		color_control |= (state->logicop_func << 16) | (state->logicop_func << 20);
	} else {
		color_control |= (0xcc << 16);
	}
	/* we pretend 8 buffer are used, CB_SHADER_MASK will disable unused one */
	if (state->independent_blend_enable) {
		for (int i = 0; i < 8; i++) {
			target_mask |= (state->rt[i].colormask << (4 * i));
		}
	} else {
		for (int i = 0; i < 8; i++) {
			target_mask |= (state->rt[0].colormask << (4 * i));
		}
	}

	/* only have dual source on MRT0 */
	blend->dual_src_blend = util_blend_state_is_dual(state, 0);
	blend->cb_target_mask = target_mask;
	blend->alpha_to_one = state->alpha_to_one;

	if (target_mask)
		color_control |= S_028808_MODE(mode);
	else
		color_control |= S_028808_MODE(V_028808_CB_DISABLE);


	r600_store_context_reg(&blend->buffer, R_028808_CB_COLOR_CONTROL, color_control);
	r600_store_context_reg(&blend->buffer, R_028B70_DB_ALPHA_TO_MASK,
			       S_028B70_ALPHA_TO_MASK_ENABLE(state->alpha_to_coverage) |
			       S_028B70_ALPHA_TO_MASK_OFFSET0(2) |
			       S_028B70_ALPHA_TO_MASK_OFFSET1(2) |
			       S_028B70_ALPHA_TO_MASK_OFFSET2(2) |
			       S_028B70_ALPHA_TO_MASK_OFFSET3(2));
	r600_store_context_reg_seq(&blend->buffer, R_028780_CB_BLEND0_CONTROL, 8);

	/* Copy over the dwords set so far into buffer_no_blend.
	 * Only the CB_BLENDi_CONTROL registers must be set after this. */
	memcpy(blend->buffer_no_blend.buf, blend->buffer.buf, blend->buffer.num_dw * 4);
	blend->buffer_no_blend.num_dw = blend->buffer.num_dw;

	for (int i = 0; i < 8; i++) {
		/* state->rt entries > 0 only written if independent blending */
		const int j = state->independent_blend_enable ? i : 0;

		unsigned eqRGB = state->rt[j].rgb_func;
		unsigned srcRGB = state->rt[j].rgb_src_factor;
		unsigned dstRGB = state->rt[j].rgb_dst_factor;
		unsigned eqA = state->rt[j].alpha_func;
		unsigned srcA = state->rt[j].alpha_src_factor;
		unsigned dstA = state->rt[j].alpha_dst_factor;
		uint32_t bc = 0;

		r600_store_value(&blend->buffer_no_blend, 0);

		if (!state->rt[j].blend_enable) {
			r600_store_value(&blend->buffer, 0);
			continue;
		}

		bc |= S_028780_BLEND_CONTROL_ENABLE(1);
		bc |= S_028780_COLOR_COMB_FCN(r600_translate_blend_function(eqRGB));
		bc |= S_028780_COLOR_SRCBLEND(r600_translate_blend_factor(srcRGB));
		bc |= S_028780_COLOR_DESTBLEND(r600_translate_blend_factor(dstRGB));

		if (srcA != srcRGB || dstA != dstRGB || eqA != eqRGB) {
			bc |= S_028780_SEPARATE_ALPHA_BLEND(1);
			bc |= S_028780_ALPHA_COMB_FCN(r600_translate_blend_function(eqA));
			bc |= S_028780_ALPHA_SRCBLEND(r600_translate_blend_factor(srcA));
			bc |= S_028780_ALPHA_DESTBLEND(r600_translate_blend_factor(dstA));
		}
		r600_store_value(&blend->buffer, bc);
	}
	return blend;
}

static void *evergreen_create_blend_state(struct pipe_context *ctx,
					const struct pipe_blend_state *state)
{

	return evergreen_create_blend_state_mode(ctx, state, V_028808_CB_NORMAL);
}

static void *evergreen_create_dsa_state(struct pipe_context *ctx,
				   const struct pipe_depth_stencil_alpha_state *state)
{
	unsigned db_depth_control, alpha_test_control, alpha_ref;
	struct r600_dsa_state *dsa = CALLOC_STRUCT(r600_dsa_state);

	if (dsa == NULL) {
		return NULL;
	}

	r600_init_command_buffer(&dsa->buffer, 3);

	dsa->valuemask[0] = state->stencil[0].valuemask;
	dsa->valuemask[1] = state->stencil[1].valuemask;
	dsa->writemask[0] = state->stencil[0].writemask;
	dsa->writemask[1] = state->stencil[1].writemask;
	dsa->zwritemask = state->depth.writemask;

	db_depth_control = S_028800_Z_ENABLE(state->depth.enabled) |
		S_028800_Z_WRITE_ENABLE(state->depth.writemask) |
		S_028800_ZFUNC(state->depth.func);

	/* stencil */
	if (state->stencil[0].enabled) {
		db_depth_control |= S_028800_STENCIL_ENABLE(1);
		db_depth_control |= S_028800_STENCILFUNC(state->stencil[0].func); /* translates straight */
		db_depth_control |= S_028800_STENCILFAIL(r600_translate_stencil_op(state->stencil[0].fail_op));
		db_depth_control |= S_028800_STENCILZPASS(r600_translate_stencil_op(state->stencil[0].zpass_op));
		db_depth_control |= S_028800_STENCILZFAIL(r600_translate_stencil_op(state->stencil[0].zfail_op));

		if (state->stencil[1].enabled) {
			db_depth_control |= S_028800_BACKFACE_ENABLE(1);
			db_depth_control |= S_028800_STENCILFUNC_BF(state->stencil[1].func); /* translates straight */
			db_depth_control |= S_028800_STENCILFAIL_BF(r600_translate_stencil_op(state->stencil[1].fail_op));
			db_depth_control |= S_028800_STENCILZPASS_BF(r600_translate_stencil_op(state->stencil[1].zpass_op));
			db_depth_control |= S_028800_STENCILZFAIL_BF(r600_translate_stencil_op(state->stencil[1].zfail_op));
		}
	}

	/* alpha */
	alpha_test_control = 0;
	alpha_ref = 0;
	if (state->alpha.enabled) {
		alpha_test_control = S_028410_ALPHA_FUNC(state->alpha.func);
		alpha_test_control |= S_028410_ALPHA_TEST_ENABLE(1);
		alpha_ref = fui(state->alpha.ref_value);
	}
	dsa->sx_alpha_test_control = alpha_test_control & 0xff;
	dsa->alpha_ref = alpha_ref;

	/* misc */
	r600_store_context_reg(&dsa->buffer, R_028800_DB_DEPTH_CONTROL, db_depth_control);
	return dsa;
}

static void *evergreen_create_rs_state(struct pipe_context *ctx,
					const struct pipe_rasterizer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	unsigned tmp, spi_interp;
	float psize_min, psize_max;
	struct r600_rasterizer_state *rs = CALLOC_STRUCT(r600_rasterizer_state);

	if (rs == NULL) {
		return NULL;
	}

	r600_init_command_buffer(&rs->buffer, 30);

	rs->flatshade = state->flatshade;
	rs->sprite_coord_enable = state->sprite_coord_enable;
	rs->two_side = state->light_twoside;
	rs->clip_plane_enable = state->clip_plane_enable;
	rs->pa_sc_line_stipple = state->line_stipple_enable ?
				S_028A0C_LINE_PATTERN(state->line_stipple_pattern) |
				S_028A0C_REPEAT_COUNT(state->line_stipple_factor) : 0;
	rs->pa_cl_clip_cntl =
		S_028810_PS_UCP_MODE(3) |
		S_028810_ZCLIP_NEAR_DISABLE(!state->depth_clip) |
		S_028810_ZCLIP_FAR_DISABLE(!state->depth_clip) |
		S_028810_DX_LINEAR_ATTR_CLIP_ENA(1);
	rs->multisample_enable = state->multisample;

	/* offset */
	rs->offset_units = state->offset_units;
	rs->offset_scale = state->offset_scale * 12.0f;
	rs->offset_enable = state->offset_point || state->offset_line || state->offset_tri;

	if (state->point_size_per_vertex) {
		psize_min = util_get_min_point_size(state);
		psize_max = 8192;
	} else {
		/* Force the point size to be as if the vertex output was disabled. */
		psize_min = state->point_size;
		psize_max = state->point_size;
	}

	spi_interp = S_0286D4_FLAT_SHADE_ENA(1);
	if (state->sprite_coord_enable) {
		spi_interp |= S_0286D4_PNT_SPRITE_ENA(1) |
			      S_0286D4_PNT_SPRITE_OVRD_X(2) |
			      S_0286D4_PNT_SPRITE_OVRD_Y(3) |
			      S_0286D4_PNT_SPRITE_OVRD_Z(0) |
			      S_0286D4_PNT_SPRITE_OVRD_W(1);
		if (state->sprite_coord_mode != PIPE_SPRITE_COORD_UPPER_LEFT) {
			spi_interp |= S_0286D4_PNT_SPRITE_TOP_1(1);
		}
	}

	r600_store_context_reg_seq(&rs->buffer, R_028A00_PA_SU_POINT_SIZE, 3);
	/* point size 12.4 fixed point (divide by two, because 0.5 = 1 pixel) */
	tmp = r600_pack_float_12p4(state->point_size/2);
	r600_store_value(&rs->buffer, /* R_028A00_PA_SU_POINT_SIZE */
			 S_028A00_HEIGHT(tmp) | S_028A00_WIDTH(tmp));
	r600_store_value(&rs->buffer, /* R_028A04_PA_SU_POINT_MINMAX */
			 S_028A04_MIN_SIZE(r600_pack_float_12p4(psize_min/2)) |
			 S_028A04_MAX_SIZE(r600_pack_float_12p4(psize_max/2)));
	r600_store_value(&rs->buffer, /* R_028A08_PA_SU_LINE_CNTL */
			 S_028A08_WIDTH((unsigned)(state->line_width * 8)));

	r600_store_context_reg(&rs->buffer, R_0286D4_SPI_INTERP_CONTROL_0, spi_interp);
	r600_store_context_reg(&rs->buffer, R_028A48_PA_SC_MODE_CNTL_0,
			       S_028A48_MSAA_ENABLE(state->multisample) |
			       S_028A48_VPORT_SCISSOR_ENABLE(state->scissor) |
			       S_028A48_LINE_STIPPLE_ENABLE(state->line_stipple_enable));

	if (rctx->chip_class == CAYMAN) {
		r600_store_context_reg(&rs->buffer, CM_R_028BE4_PA_SU_VTX_CNTL,
				       S_028C08_PIX_CENTER_HALF(state->gl_rasterization_rules) |
				       S_028C08_QUANT_MODE(V_028C08_X_1_256TH));
	} else {
		r600_store_context_reg(&rs->buffer, R_028C08_PA_SU_VTX_CNTL,
				       S_028C08_PIX_CENTER_HALF(state->gl_rasterization_rules) |
				       S_028C08_QUANT_MODE(V_028C08_X_1_256TH));
	}

	r600_store_context_reg(&rs->buffer, R_028B7C_PA_SU_POLY_OFFSET_CLAMP, fui(state->offset_clamp));
	r600_store_context_reg(&rs->buffer, R_028814_PA_SU_SC_MODE_CNTL,
			       S_028814_PROVOKING_VTX_LAST(!state->flatshade_first) |
			       S_028814_CULL_FRONT((state->cull_face & PIPE_FACE_FRONT) ? 1 : 0) |
			       S_028814_CULL_BACK((state->cull_face & PIPE_FACE_BACK) ? 1 : 0) |
			       S_028814_FACE(!state->front_ccw) |
			       S_028814_POLY_OFFSET_FRONT_ENABLE(state->offset_tri) |
			       S_028814_POLY_OFFSET_BACK_ENABLE(state->offset_tri) |
			       S_028814_POLY_OFFSET_PARA_ENABLE(state->offset_tri) |
			       S_028814_POLY_MODE(state->fill_front != PIPE_POLYGON_MODE_FILL ||
						  state->fill_back != PIPE_POLYGON_MODE_FILL) |
			       S_028814_POLYMODE_FRONT_PTYPE(r600_translate_fill(state->fill_front)) |
			       S_028814_POLYMODE_BACK_PTYPE(r600_translate_fill(state->fill_back)));
	r600_store_context_reg(&rs->buffer, R_028350_SX_MISC, S_028350_MULTIPASS(state->rasterizer_discard));
	return rs;
}

static void *evergreen_create_sampler_state(struct pipe_context *ctx,
					const struct pipe_sampler_state *state)
{
	struct r600_pipe_sampler_state *ss = CALLOC_STRUCT(r600_pipe_sampler_state);
	unsigned aniso_flag_offset = state->max_anisotropy > 1 ? 2 : 0;

	if (ss == NULL) {
		return NULL;
	}

	ss->border_color_use = sampler_state_needs_border_color(state);

	/* R_03C000_SQ_TEX_SAMPLER_WORD0_0 */
	ss->tex_sampler_words[0] =
		S_03C000_CLAMP_X(r600_tex_wrap(state->wrap_s)) |
		S_03C000_CLAMP_Y(r600_tex_wrap(state->wrap_t)) |
		S_03C000_CLAMP_Z(r600_tex_wrap(state->wrap_r)) |
		S_03C000_XY_MAG_FILTER(r600_tex_filter(state->mag_img_filter) | aniso_flag_offset) |
		S_03C000_XY_MIN_FILTER(r600_tex_filter(state->min_img_filter) | aniso_flag_offset) |
		S_03C000_MIP_FILTER(r600_tex_mipfilter(state->min_mip_filter)) |
		S_03C000_MAX_ANISO(r600_tex_aniso_filter(state->max_anisotropy)) |
		S_03C000_DEPTH_COMPARE_FUNCTION(r600_tex_compare(state->compare_func)) |
		S_03C000_BORDER_COLOR_TYPE(ss->border_color_use ? V_03C000_SQ_TEX_BORDER_COLOR_REGISTER : 0);
	/* R_03C004_SQ_TEX_SAMPLER_WORD1_0 */
	ss->tex_sampler_words[1] =
		S_03C004_MIN_LOD(S_FIXED(CLAMP(state->min_lod, 0, 15), 8)) |
		S_03C004_MAX_LOD(S_FIXED(CLAMP(state->max_lod, 0, 15), 8));
	/* R_03C008_SQ_TEX_SAMPLER_WORD2_0 */
	ss->tex_sampler_words[2] =
		S_03C008_LOD_BIAS(S_FIXED(CLAMP(state->lod_bias, -16, 16), 8)) |
		(state->seamless_cube_map ? 0 : S_03C008_DISABLE_CUBE_WRAP(1)) |
		S_03C008_TYPE(1);

	if (ss->border_color_use) {
		memcpy(&ss->border_color, &state->border_color, sizeof(state->border_color));
	}
	return ss;
}

static struct pipe_sampler_view *
texture_buffer_sampler_view(struct r600_pipe_sampler_view *view,
			    unsigned width0, unsigned height0)
			    
{
	struct pipe_context *ctx = view->base.context;
	struct r600_texture *tmp = (struct r600_texture*)view->base.texture;
	uint64_t va;
	int stride = util_format_get_blocksize(view->base.format);
	unsigned format, num_format, format_comp, endian;
	unsigned swizzle_res;
	unsigned char swizzle[4];
	const struct util_format_description *desc;

	swizzle[0] = view->base.swizzle_r;
	swizzle[1] = view->base.swizzle_g;
	swizzle[2] = view->base.swizzle_b;
	swizzle[3] = view->base.swizzle_a;

	r600_vertex_data_type(view->base.format,
			      &format, &num_format, &format_comp,
			      &endian);

	desc = util_format_description(view->base.format);

	swizzle_res = r600_get_swizzle_combined(desc->swizzle, swizzle, TRUE);

	va = r600_resource_va(ctx->screen, view->base.texture);
	view->tex_resource = &tmp->resource;

	view->skip_mip_address_reloc = true;
	view->tex_resource_words[0] = va;
	view->tex_resource_words[1] = width0 - 1;
	view->tex_resource_words[2] = S_030008_BASE_ADDRESS_HI(va >> 32UL) |
		S_030008_STRIDE(stride) |
		S_030008_DATA_FORMAT(format) |
		S_030008_NUM_FORMAT_ALL(num_format) |
		S_030008_FORMAT_COMP_ALL(format_comp) |
		S_030008_SRF_MODE_ALL(1) |
		S_030008_ENDIAN_SWAP(endian);
	view->tex_resource_words[3] = swizzle_res;
	/*
	 * in theory dword 4 is for number of elements, for use with resinfo,
	 * but it seems to utterly fail to work, the amd gpu shader analyser
	 * uses a const buffer to store the element sizes for buffer txq
	 */
	view->tex_resource_words[4] = 0;
	view->tex_resource_words[5] = view->tex_resource_words[6] = 0;
	view->tex_resource_words[7] = S_03001C_TYPE(V_03001C_SQ_TEX_VTX_VALID_BUFFER);
	return &view->base;
}

struct pipe_sampler_view *
evergreen_create_sampler_view_custom(struct pipe_context *ctx,
				     struct pipe_resource *texture,
				     const struct pipe_sampler_view *state,
				     unsigned width0, unsigned height0)
{
	struct r600_screen *rscreen = (struct r600_screen*)ctx->screen;
	struct r600_pipe_sampler_view *view = CALLOC_STRUCT(r600_pipe_sampler_view);
	struct r600_texture *tmp = (struct r600_texture*)texture;
	unsigned format, endian;
	uint32_t word4 = 0, yuv_format = 0, pitch = 0;
	unsigned char swizzle[4], array_mode = 0, non_disp_tiling = 0;
	unsigned height, depth, width;
	unsigned macro_aspect, tile_split, bankh, bankw, nbanks;
	enum pipe_format pipe_format = state->format;
	struct radeon_surface_level *surflevel;

	if (view == NULL)
		return NULL;

	/* initialize base object */
	view->base = *state;
	view->base.texture = NULL;
	pipe_reference(NULL, &texture->reference);
	view->base.texture = texture;
	view->base.reference.count = 1;
	view->base.context = ctx;

	if (texture->target == PIPE_BUFFER)
		return texture_buffer_sampler_view(view, width0, height0);

	swizzle[0] = state->swizzle_r;
	swizzle[1] = state->swizzle_g;
	swizzle[2] = state->swizzle_b;
	swizzle[3] = state->swizzle_a;

	tile_split = tmp->surface.tile_split;
	surflevel = tmp->surface.level;

	/* Texturing with separate depth and stencil. */
	if (tmp->is_depth && !tmp->is_flushing_texture) {
		switch (pipe_format) {
		case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
			pipe_format = PIPE_FORMAT_Z32_FLOAT;
			break;
		case PIPE_FORMAT_X8Z24_UNORM:
		case PIPE_FORMAT_S8_UINT_Z24_UNORM:
			/* Z24 is always stored like this. */
			pipe_format = PIPE_FORMAT_Z24X8_UNORM;
			break;
		case PIPE_FORMAT_X24S8_UINT:
		case PIPE_FORMAT_S8X24_UINT:
		case PIPE_FORMAT_X32_S8X24_UINT:
			pipe_format = PIPE_FORMAT_S8_UINT;
			tile_split = tmp->surface.stencil_tile_split;
			surflevel = tmp->surface.stencil_level;
			break;
		default:;
		}
	}

	format = r600_translate_texformat(ctx->screen, pipe_format,
					  swizzle,
					  &word4, &yuv_format);
	assert(format != ~0);
	if (format == ~0) {
		FREE(view);
		return NULL;
	}

	endian = r600_colorformat_endian_swap(format);

	width = width0;
	height = height0;
	depth = texture->depth0;
	pitch = surflevel[0].nblk_x * util_format_get_blockwidth(pipe_format);
	non_disp_tiling = tmp->non_disp_tiling;

	switch (surflevel[0].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		array_mode = V_028C70_ARRAY_LINEAR_ALIGNED;
		break;
	case RADEON_SURF_MODE_2D:
		array_mode = V_028C70_ARRAY_2D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_1D:
		array_mode = V_028C70_ARRAY_1D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_LINEAR:
	default:
		array_mode = V_028C70_ARRAY_LINEAR_GENERAL;
		break;
	}
	macro_aspect = tmp->surface.mtilea;
	bankw = tmp->surface.bankw;
	bankh = tmp->surface.bankh;
	tile_split = eg_tile_split(tile_split);
	macro_aspect = eg_macro_tile_aspect(macro_aspect);
	bankw = eg_bank_wh(bankw);
	bankh = eg_bank_wh(bankh);

	/* 128 bit formats require tile type = 1 */
	if (rscreen->chip_class == CAYMAN) {
		if (util_format_get_blocksize(pipe_format) >= 16)
			non_disp_tiling = 1;
	}
	nbanks = eg_num_banks(rscreen->tiling_info.num_banks);

	if (texture->target == PIPE_TEXTURE_1D_ARRAY) {
	        height = 1;
		depth = texture->array_size;
	} else if (texture->target == PIPE_TEXTURE_2D_ARRAY) {
		depth = texture->array_size;
	} else if (texture->target == PIPE_TEXTURE_CUBE_ARRAY)
		depth = texture->array_size / 6;

	view->tex_resource = &tmp->resource;
	view->tex_resource_words[0] = (S_030000_DIM(r600_tex_dim(texture->target, texture->nr_samples)) |
				       S_030000_PITCH((pitch / 8) - 1) |
				       S_030000_TEX_WIDTH(width - 1));
	if (rscreen->chip_class == CAYMAN)
		view->tex_resource_words[0] |= CM_S_030000_NON_DISP_TILING_ORDER(non_disp_tiling);
	else
		view->tex_resource_words[0] |= S_030000_NON_DISP_TILING_ORDER(non_disp_tiling);
	view->tex_resource_words[1] = (S_030004_TEX_HEIGHT(height - 1) |
				       S_030004_TEX_DEPTH(depth - 1) |
				       S_030004_ARRAY_MODE(array_mode));
	view->tex_resource_words[2] = (surflevel[0].offset + r600_resource_va(ctx->screen, texture)) >> 8;

	/* TEX_RESOURCE_WORD3.MIP_ADDRESS */
	if (texture->nr_samples > 1 && rscreen->msaa_texture_support == MSAA_TEXTURE_COMPRESSED) {
		/* XXX the 2x and 4x cases are broken. */
		if (tmp->is_depth || tmp->resource.b.b.nr_samples != 8) {
			/* disable FMASK (0 = disabled) */
			view->tex_resource_words[3] = 0;
			view->skip_mip_address_reloc = true;
		} else {
			/* FMASK should be in MIP_ADDRESS for multisample textures */
			view->tex_resource_words[3] = (tmp->fmask_offset + r600_resource_va(ctx->screen, texture)) >> 8;
		}
	} else if (state->u.tex.last_level && texture->nr_samples <= 1) {
		view->tex_resource_words[3] = (surflevel[1].offset + r600_resource_va(ctx->screen, texture)) >> 8;
	} else {
		view->tex_resource_words[3] = (surflevel[0].offset + r600_resource_va(ctx->screen, texture)) >> 8;
	}

	view->tex_resource_words[4] = (word4 |
				       S_030010_SRF_MODE_ALL(V_030010_SRF_MODE_ZERO_CLAMP_MINUS_ONE) |
				       S_030010_ENDIAN_SWAP(endian));
	view->tex_resource_words[5] = S_030014_BASE_ARRAY(state->u.tex.first_layer) |
				      S_030014_LAST_ARRAY(state->u.tex.last_layer);
	if (texture->nr_samples > 1) {
		unsigned log_samples = util_logbase2(texture->nr_samples);
		if (rscreen->chip_class == CAYMAN) {
			view->tex_resource_words[4] |= S_030010_LOG2_NUM_FRAGMENTS(log_samples);
		}
		/* LAST_LEVEL holds log2(nr_samples) for multisample textures */
		view->tex_resource_words[5] |= S_030014_LAST_LEVEL(log_samples);
	} else {
		view->tex_resource_words[4] |= S_030010_BASE_LEVEL(state->u.tex.first_level);
		view->tex_resource_words[5] |= S_030014_LAST_LEVEL(state->u.tex.last_level);
	}
	/* aniso max 16 samples */
	view->tex_resource_words[6] = (S_030018_MAX_ANISO(4)) |
				      (S_030018_TILE_SPLIT(tile_split));
	view->tex_resource_words[7] = S_03001C_DATA_FORMAT(format) |
				      S_03001C_TYPE(V_03001C_SQ_TEX_VTX_VALID_TEXTURE) |
				      S_03001C_BANK_WIDTH(bankw) |
				      S_03001C_BANK_HEIGHT(bankh) |
				      S_03001C_MACRO_TILE_ASPECT(macro_aspect) |
				      S_03001C_NUM_BANKS(nbanks) |
				      S_03001C_DEPTH_SAMPLE_ORDER(tmp->is_depth && !tmp->is_flushing_texture);
	return &view->base;
}

static struct pipe_sampler_view *
evergreen_create_sampler_view(struct pipe_context *ctx,
			      struct pipe_resource *tex,
			      const struct pipe_sampler_view *state)
{
	return evergreen_create_sampler_view_custom(ctx, tex, state,
						    tex->width0, tex->height0);
}

static void evergreen_emit_clip_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct pipe_clip_state *state = &rctx->clip_state.state;

	r600_write_context_reg_seq(cs, R_0285BC_PA_CL_UCP0_X, 6*4);
	r600_write_array(cs, 6*4, (unsigned*)state);
}

static void evergreen_set_polygon_stipple(struct pipe_context *ctx,
					 const struct pipe_poly_stipple *state)
{
}

static void evergreen_get_scissor_rect(struct r600_context *rctx,
				       unsigned tl_x, unsigned tl_y, unsigned br_x, unsigned br_y,
				       uint32_t *tl, uint32_t *br)
{
	/* EG hw workaround */
	if (br_x == 0)
		tl_x = 1;
	if (br_y == 0)
		tl_y = 1;

	/* cayman hw workaround */
	if (rctx->chip_class == CAYMAN) {
		if (br_x == 1 && br_y == 1)
			br_x = 2;
	}

	*tl = S_028240_TL_X(tl_x) | S_028240_TL_Y(tl_y);
	*br = S_028244_BR_X(br_x) | S_028244_BR_Y(br_y);
}

static void evergreen_set_scissor_state(struct pipe_context *ctx,
					const struct pipe_scissor_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;

	rctx->scissor.scissor = *state;
	rctx->scissor.atom.dirty = true;
}

static void evergreen_emit_scissor_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct pipe_scissor_state *state = &rctx->scissor.scissor;
	uint32_t tl, br;

	evergreen_get_scissor_rect(rctx, state->minx, state->miny, state->maxx, state->maxy, &tl, &br);

	r600_write_context_reg_seq(cs, R_028250_PA_SC_VPORT_SCISSOR_0_TL, 2);
	r600_write_value(cs, tl);
	r600_write_value(cs, br);
}

/**
 * This function intializes the CB* register values for RATs.  It is meant
 * to be used for 1D aligned buffers that do not have an associated
 * radeon_surface.
 */
void evergreen_init_color_surface_rat(struct r600_context *rctx,
					struct r600_surface *surf)
{
	struct pipe_resource *pipe_buffer = surf->base.texture;
	unsigned format = r600_translate_colorformat(surf->base.format);
	unsigned endian = r600_colorformat_endian_swap(format);
	unsigned swap = r600_translate_colorswap(surf->base.format);
	unsigned block_size =
		align(util_format_get_blocksize(pipe_buffer->format), 4);
	unsigned pitch_alignment =
		MAX2(64, rctx->screen->tiling_info.group_bytes / block_size);
	unsigned pitch = align(pipe_buffer->width0, pitch_alignment);

	/* XXX: This is copied from evergreen_init_color_surface().  I don't
	 * know why this is necessary.
	 */
	if (pipe_buffer->usage == PIPE_USAGE_STAGING) {
		endian = ENDIAN_NONE;
	}

	surf->cb_color_base =
		r600_resource_va(rctx->context.screen, pipe_buffer) >> 8;

	surf->cb_color_pitch = (pitch / 8) - 1;

	surf->cb_color_slice = 0;

	surf->cb_color_view = 0;

	surf->cb_color_info =
		  S_028C70_ENDIAN(endian)
		| S_028C70_FORMAT(format)
		| S_028C70_ARRAY_MODE(V_028C70_ARRAY_LINEAR_ALIGNED)
		| S_028C70_NUMBER_TYPE(V_028C70_NUMBER_UINT)
		| S_028C70_COMP_SWAP(swap)
		| S_028C70_BLEND_BYPASS(1) /* We must set this bit because we
					    * are using NUMBER_UINT */
		| S_028C70_RAT(1)
		;

	surf->cb_color_attrib = S_028C74_NON_DISP_TILING_ORDER(1);

	/* For buffers, CB_COLOR0_DIM needs to be set to the number of
	 * elements. */
	surf->cb_color_dim = pipe_buffer->width0;

	/* Set the buffer range the GPU will have access to: */
	util_range_add(&r600_resource(pipe_buffer)->valid_buffer_range,
		       0, pipe_buffer->width0);

	surf->cb_color_cmask = surf->cb_color_base;
	surf->cb_color_cmask_slice = 0;
	surf->cb_color_fmask = surf->cb_color_base;
	surf->cb_color_fmask_slice = 0;
}

void evergreen_init_color_surface(struct r600_context *rctx,
				  struct r600_surface *surf)
{
	struct r600_screen *rscreen = rctx->screen;
	struct r600_texture *rtex = (struct r600_texture*)surf->base.texture;
	struct pipe_resource *pipe_tex = surf->base.texture;
	unsigned level = surf->base.u.tex.level;
	unsigned pitch, slice;
	unsigned color_info, color_attrib, color_dim = 0;
	unsigned format, swap, ntype, endian;
	uint64_t offset, base_offset;
	unsigned non_disp_tiling, macro_aspect, tile_split, bankh, bankw, fmask_bankh, nbanks;
	const struct util_format_description *desc;
	int i;
	bool blend_clamp = 0, blend_bypass = 0;

	offset = rtex->surface.level[level].offset;
	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		offset += rtex->surface.level[level].slice_size *
			  surf->base.u.tex.first_layer;
	}
	pitch = (rtex->surface.level[level].nblk_x) / 8 - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}
	color_info = 0;
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_LINEAR_ALIGNED);
		non_disp_tiling = 1;
		break;
	case RADEON_SURF_MODE_1D:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_1D_TILED_THIN1);
		non_disp_tiling = rtex->non_disp_tiling;
		break;
	case RADEON_SURF_MODE_2D:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_2D_TILED_THIN1);
		non_disp_tiling = rtex->non_disp_tiling;
		break;
	case RADEON_SURF_MODE_LINEAR:
	default:
		color_info = S_028C70_ARRAY_MODE(V_028C70_ARRAY_LINEAR_GENERAL);
		non_disp_tiling = 1;
		break;
	}
	tile_split = rtex->surface.tile_split;
	macro_aspect = rtex->surface.mtilea;
	bankw = rtex->surface.bankw;
	bankh = rtex->surface.bankh;
	fmask_bankh = rtex->fmask_bank_height;
	tile_split = eg_tile_split(tile_split);
	macro_aspect = eg_macro_tile_aspect(macro_aspect);
	bankw = eg_bank_wh(bankw);
	bankh = eg_bank_wh(bankh);
	fmask_bankh = eg_bank_wh(fmask_bankh);

	/* 128 bit formats require tile type = 1 */
	if (rscreen->chip_class == CAYMAN) {
		if (util_format_get_blocksize(surf->base.format) >= 16)
			non_disp_tiling = 1;
	}
	nbanks = eg_num_banks(rscreen->tiling_info.num_banks);
	desc = util_format_description(surf->base.format);
	for (i = 0; i < 4; i++) {
		if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
			break;
		}
	}

	color_attrib = S_028C74_TILE_SPLIT(tile_split)|
			S_028C74_NUM_BANKS(nbanks) |
			S_028C74_BANK_WIDTH(bankw) |
			S_028C74_BANK_HEIGHT(bankh) |
			S_028C74_MACRO_TILE_ASPECT(macro_aspect) |
			S_028C74_NON_DISP_TILING_ORDER(non_disp_tiling) |
		        S_028C74_FMASK_BANK_HEIGHT(fmask_bankh);

	if (rctx->chip_class == CAYMAN) {
		color_attrib |=	S_028C74_FORCE_DST_ALPHA_1(desc->swizzle[3] ==
							   UTIL_FORMAT_SWIZZLE_1);

		if (rtex->resource.b.b.nr_samples > 1) {
			unsigned log_samples = util_logbase2(rtex->resource.b.b.nr_samples);
			color_attrib |= S_028C74_NUM_SAMPLES(log_samples) |
					S_028C74_NUM_FRAGMENTS(log_samples);
		}
	}

	ntype = V_028C70_NUMBER_UNORM;
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
		ntype = V_028C70_NUMBER_SRGB;
	else if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
		if (desc->channel[i].normalized)
			ntype = V_028C70_NUMBER_SNORM;
		else if (desc->channel[i].pure_integer)
			ntype = V_028C70_NUMBER_SINT;
	} else if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
		if (desc->channel[i].normalized)
			ntype = V_028C70_NUMBER_UNORM;
		else if (desc->channel[i].pure_integer)
			ntype = V_028C70_NUMBER_UINT;
	}

	format = r600_translate_colorformat(surf->base.format);
	assert(format != ~0);

	swap = r600_translate_colorswap(surf->base.format);
	assert(swap != ~0);

	if (rtex->resource.b.b.usage == PIPE_USAGE_STAGING) {
		endian = ENDIAN_NONE;
	} else {
		endian = r600_colorformat_endian_swap(format);
	}

	/* blend clamp should be set for all NORM/SRGB types */
	if (ntype == V_028C70_NUMBER_UNORM || ntype == V_028C70_NUMBER_SNORM ||
	    ntype == V_028C70_NUMBER_SRGB)
		blend_clamp = 1;

	/* set blend bypass according to docs if SINT/UINT or
	   8/24 COLOR variants */
	if (ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT ||
	    format == V_028C70_COLOR_8_24 || format == V_028C70_COLOR_24_8 ||
	    format == V_028C70_COLOR_X24_8_32_FLOAT) {
		blend_clamp = 0;
		blend_bypass = 1;
	}

	surf->alphatest_bypass = ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT;

	color_info |= S_028C70_FORMAT(format) |
		S_028C70_COMP_SWAP(swap) |
		S_028C70_BLEND_CLAMP(blend_clamp) |
		S_028C70_BLEND_BYPASS(blend_bypass) |
		S_028C70_NUMBER_TYPE(ntype) |
		S_028C70_ENDIAN(endian);

	if (rtex->is_rat) {
		color_info |= S_028C70_RAT(1);
		color_dim = S_028C78_WIDTH_MAX(pipe_tex->width0 & 0xffff)
			| S_028C78_HEIGHT_MAX((pipe_tex->width0 >> 16) & 0xffff);
	}

	/* EXPORT_NORM is an optimzation that can be enabled for better
	 * performance in certain cases.
	 * EXPORT_NORM can be enabled if:
	 * - 11-bit or smaller UNORM/SNORM/SRGB
	 * - 16-bit or smaller FLOAT
	 */
	if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS &&
	    ((desc->channel[i].size < 12 &&
	      desc->channel[i].type != UTIL_FORMAT_TYPE_FLOAT &&
	      ntype != V_028C70_NUMBER_UINT && ntype != V_028C70_NUMBER_SINT) ||
	     (desc->channel[i].size < 17 &&
	      desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT))) {
		color_info |= S_028C70_SOURCE_FORMAT(V_028C70_EXPORT_4C_16BPC);
		surf->export_16bpc = true;
	}

	if (rtex->fmask_size && rtex->cmask_size) {
		color_info |= S_028C70_COMPRESSION(1) | S_028C70_FAST_CLEAR(1);
	}

	base_offset = r600_resource_va(rctx->context.screen, pipe_tex);

	/* XXX handle enabling of CB beyond BASE8 which has different offset */
	surf->cb_color_base = (base_offset + offset) >> 8;
	surf->cb_color_dim = color_dim;
	surf->cb_color_info = color_info;
	surf->cb_color_pitch = S_028C64_PITCH_TILE_MAX(pitch);
	surf->cb_color_slice = S_028C68_SLICE_TILE_MAX(slice);
	if (rtex->surface.level[level].mode < RADEON_SURF_MODE_1D) {
		surf->cb_color_view = 0;
	} else {
		surf->cb_color_view = S_028C6C_SLICE_START(surf->base.u.tex.first_layer) |
				      S_028C6C_SLICE_MAX(surf->base.u.tex.last_layer);
	}
	surf->cb_color_attrib = color_attrib;
	if (rtex->fmask_size && rtex->cmask_size) {
		surf->cb_color_fmask = (base_offset + rtex->fmask_offset) >> 8;
		surf->cb_color_cmask = (base_offset + rtex->cmask_offset) >> 8;
	} else {
		surf->cb_color_fmask = surf->cb_color_base;
		surf->cb_color_cmask = surf->cb_color_base;
	}
	surf->cb_color_fmask_slice = S_028C88_TILE_MAX(slice);
	surf->cb_color_cmask_slice = S_028C80_TILE_MAX(rtex->cmask_slice_tile_max);

	surf->color_initialized = true;
}

static void evergreen_init_depth_surface(struct r600_context *rctx,
					 struct r600_surface *surf)
{
	struct r600_screen *rscreen = rctx->screen;
	struct pipe_screen *screen = &rscreen->screen;
	struct r600_texture *rtex = (struct r600_texture*)surf->base.texture;
	uint64_t offset;
	unsigned level, pitch, slice, format, array_mode;
	unsigned macro_aspect, tile_split, bankh, bankw, nbanks;

	level = surf->base.u.tex.level;
	format = r600_translate_dbformat(surf->base.format);
	assert(format != ~0);

	offset = r600_resource_va(screen, surf->base.texture);
	offset += rtex->surface.level[level].offset;
	pitch = (rtex->surface.level[level].nblk_x / 8) - 1;
	slice = (rtex->surface.level[level].nblk_x * rtex->surface.level[level].nblk_y) / 64;
	if (slice) {
		slice = slice - 1;
	}
	switch (rtex->surface.level[level].mode) {
	case RADEON_SURF_MODE_2D:
		array_mode = V_028C70_ARRAY_2D_TILED_THIN1;
		break;
	case RADEON_SURF_MODE_1D:
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
	case RADEON_SURF_MODE_LINEAR:
	default:
		array_mode = V_028C70_ARRAY_1D_TILED_THIN1;
		break;
	}
	tile_split = rtex->surface.tile_split;
	macro_aspect = rtex->surface.mtilea;
	bankw = rtex->surface.bankw;
	bankh = rtex->surface.bankh;
	tile_split = eg_tile_split(tile_split);
	macro_aspect = eg_macro_tile_aspect(macro_aspect);
	bankw = eg_bank_wh(bankw);
	bankh = eg_bank_wh(bankh);
	nbanks = eg_num_banks(rscreen->tiling_info.num_banks);
	offset >>= 8;

	surf->db_depth_info = S_028040_ARRAY_MODE(array_mode) |
			      S_028040_FORMAT(format) |
			      S_028040_TILE_SPLIT(tile_split)|
			      S_028040_NUM_BANKS(nbanks) |
			      S_028040_BANK_WIDTH(bankw) |
			      S_028040_BANK_HEIGHT(bankh) |
			      S_028040_MACRO_TILE_ASPECT(macro_aspect);
	if (rscreen->chip_class == CAYMAN && rtex->resource.b.b.nr_samples > 1) {
		surf->db_depth_info |= S_028040_NUM_SAMPLES(util_logbase2(rtex->resource.b.b.nr_samples));
	}
	surf->db_depth_base = offset;
	surf->db_depth_view = S_028008_SLICE_START(surf->base.u.tex.first_layer) |
			      S_028008_SLICE_MAX(surf->base.u.tex.last_layer);
	surf->db_depth_size = S_028058_PITCH_TILE_MAX(pitch);
	surf->db_depth_slice = S_02805C_SLICE_TILE_MAX(slice);

	switch (surf->base.format) {
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		surf->pa_su_poly_offset_db_fmt_cntl =
			S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS((char)-24);
		break;
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
		surf->pa_su_poly_offset_db_fmt_cntl =
			S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS((char)-23) |
			S_028B78_POLY_OFFSET_DB_IS_FLOAT_FMT(1);
		break;
	case PIPE_FORMAT_Z16_UNORM:
		surf->pa_su_poly_offset_db_fmt_cntl =
			S_028B78_POLY_OFFSET_NEG_NUM_DB_BITS((char)-16);
		break;
	default:;
	}

	if (rtex->surface.flags & RADEON_SURF_SBUFFER) {
		uint64_t stencil_offset;
		unsigned stile_split = rtex->surface.stencil_tile_split;

		stile_split = eg_tile_split(stile_split);

		stencil_offset = rtex->surface.stencil_level[level].offset;
		stencil_offset += r600_resource_va(screen, surf->base.texture);

		surf->db_stencil_base = stencil_offset >> 8;
		surf->db_stencil_info = S_028044_FORMAT(V_028044_STENCIL_8) |
					S_028044_TILE_SPLIT(stile_split);
	} else {
		surf->db_stencil_base = offset;
		/* DRM 2.6.18 allows the INVALID format to disable stencil.
		 * Older kernels are out of luck. */
		surf->db_stencil_info = rctx->screen->info.drm_minor >= 18 ?
					S_028044_FORMAT(V_028044_STENCIL_INVALID) :
					S_028044_FORMAT(V_028044_STENCIL_8);
	}

	surf->htile_enabled = 0;
	/* use htile only for first level */
	if (rtex->htile && !level) {
		uint64_t va = r600_resource_va(&rctx->screen->screen, &rtex->htile->b.b);
		surf->htile_enabled = 1;
		surf->db_htile_data_base = va >> 8;
		surf->db_htile_surface = S_028ABC_HTILE_WIDTH(1) |
					S_028ABC_HTILE_HEIGHT(1) |
					S_028ABC_LINEAR(1);
		surf->db_depth_info |= S_028040_TILE_SURFACE_ENABLE(1);
		surf->db_preload_control = 0;
	}

	surf->depth_initialized = true;
}

static void evergreen_set_framebuffer_state(struct pipe_context *ctx,
					    const struct pipe_framebuffer_state *state)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_surface *surf;
	struct r600_texture *rtex;
	uint32_t i, log_samples;

	if (rctx->framebuffer.state.nr_cbufs) {
		rctx->flags |= R600_CONTEXT_WAIT_3D_IDLE | R600_CONTEXT_FLUSH_AND_INV;

		if (rctx->framebuffer.state.cbufs[0]->texture->nr_samples > 1) {
			rctx->flags |= R600_CONTEXT_FLUSH_AND_INV_CB_META;
		}
	}
	if (rctx->framebuffer.state.zsbuf) {
		rctx->flags |= R600_CONTEXT_WAIT_3D_IDLE | R600_CONTEXT_FLUSH_AND_INV;

		rtex = (struct r600_texture*)rctx->framebuffer.state.zsbuf->texture;
		if (rtex->htile) {
			rctx->flags |= R600_CONTEXT_FLUSH_AND_INV_DB_META;
		}
	}

	util_copy_framebuffer_state(&rctx->framebuffer.state, state);

	/* Colorbuffers. */
	rctx->framebuffer.export_16bpc = state->nr_cbufs != 0;
	rctx->framebuffer.cb0_is_integer = state->nr_cbufs &&
					   util_format_is_pure_integer(state->cbufs[0]->format);
	rctx->framebuffer.compressed_cb_mask = 0;

	if (state->nr_cbufs)
		rctx->framebuffer.nr_samples = state->cbufs[0]->texture->nr_samples;
	else if (state->zsbuf)
		rctx->framebuffer.nr_samples = state->zsbuf->texture->nr_samples;
	else
		rctx->framebuffer.nr_samples = 0;

	for (i = 0; i < state->nr_cbufs; i++) {
		surf = (struct r600_surface*)state->cbufs[i];
		rtex = (struct r600_texture*)surf->base.texture;

		r600_context_add_resource_size(ctx, state->cbufs[i]->texture);

		if (!surf->color_initialized) {
			evergreen_init_color_surface(rctx, surf);
		}

		if (!surf->export_16bpc) {
			rctx->framebuffer.export_16bpc = false;
		}

		if (rtex->fmask_size && rtex->cmask_size) {
			rctx->framebuffer.compressed_cb_mask |= 1 << i;
		}
	}

	/* Update alpha-test state dependencies.
	 * Alpha-test is done on the first colorbuffer only. */
	if (state->nr_cbufs) {
		surf = (struct r600_surface*)state->cbufs[0];
		if (rctx->alphatest_state.bypass != surf->alphatest_bypass) {
			rctx->alphatest_state.bypass = surf->alphatest_bypass;
			rctx->alphatest_state.atom.dirty = true;
		}
		if (rctx->alphatest_state.cb0_export_16bpc != surf->export_16bpc) {
			rctx->alphatest_state.cb0_export_16bpc = surf->export_16bpc;
			rctx->alphatest_state.atom.dirty = true;
		}
	}

	/* ZS buffer. */
	if (state->zsbuf) {
		surf = (struct r600_surface*)state->zsbuf;

		r600_context_add_resource_size(ctx, state->zsbuf->texture);

		if (!surf->depth_initialized) {
			evergreen_init_depth_surface(rctx, surf);
		}

		if (state->zsbuf->format != rctx->poly_offset_state.zs_format) {
			rctx->poly_offset_state.zs_format = state->zsbuf->format;
			rctx->poly_offset_state.atom.dirty = true;
		}

		if (rctx->db_state.rsurf != surf) {
			rctx->db_state.rsurf = surf;
			rctx->db_state.atom.dirty = true;
			rctx->db_misc_state.atom.dirty = true;
		}
	} else if (rctx->db_state.rsurf) {
		rctx->db_state.rsurf = NULL;
		rctx->db_state.atom.dirty = true;
		rctx->db_misc_state.atom.dirty = true;
	}

	if (rctx->cb_misc_state.nr_cbufs != state->nr_cbufs) {
		rctx->cb_misc_state.nr_cbufs = state->nr_cbufs;
		rctx->cb_misc_state.atom.dirty = true;
	}

	if (state->nr_cbufs == 0 && rctx->alphatest_state.bypass) {
		rctx->alphatest_state.bypass = false;
		rctx->alphatest_state.atom.dirty = true;
	}

	log_samples = util_logbase2(rctx->framebuffer.nr_samples);
	if (rctx->chip_class == CAYMAN && rctx->db_misc_state.log_samples != log_samples) {
		rctx->db_misc_state.log_samples = log_samples;
		rctx->db_misc_state.atom.dirty = true;
	}

	evergreen_update_db_shader_control(rctx);

	/* Calculate the CS size. */
	rctx->framebuffer.atom.num_dw = 4; /* SCISSOR */

	/* MSAA. */
	if (rctx->chip_class == EVERGREEN) {
		switch (rctx->framebuffer.nr_samples) {
		case 2:
		case 4:
			rctx->framebuffer.atom.num_dw += 6;
			break;
		case 8:
			rctx->framebuffer.atom.num_dw += 10;
			break;
		}
		rctx->framebuffer.atom.num_dw += 4;
	} else {
		switch (rctx->framebuffer.nr_samples) {
		case 2:
		case 4:
			rctx->framebuffer.atom.num_dw += 12;
			break;
		case 8:
			rctx->framebuffer.atom.num_dw += 16;
			break;
		case 16:
			rctx->framebuffer.atom.num_dw += 18;
			break;
		}
		rctx->framebuffer.atom.num_dw += 7;
	}

	/* Colorbuffers. */
	rctx->framebuffer.atom.num_dw += state->nr_cbufs * 21;
	if (rctx->keep_tiling_flags)
		rctx->framebuffer.atom.num_dw += state->nr_cbufs * 2;
	rctx->framebuffer.atom.num_dw += (12 - state->nr_cbufs) * 3;

	/* ZS buffer. */
	if (state->zsbuf) {
		rctx->framebuffer.atom.num_dw += 24;
		if (rctx->keep_tiling_flags)
			rctx->framebuffer.atom.num_dw += 2;
	} else if (rctx->screen->info.drm_minor >= 18) {
		rctx->framebuffer.atom.num_dw += 4;
	}

	rctx->framebuffer.atom.dirty = true;
}

#define FILL_SREG(s0x, s0y, s1x, s1y, s2x, s2y, s3x, s3y)  \
	(((s0x) & 0xf) | (((s0y) & 0xf) << 4) |		   \
	(((s1x) & 0xf) << 8) | (((s1y) & 0xf) << 12) |	   \
	(((s2x) & 0xf) << 16) | (((s2y) & 0xf) << 20) |	   \
	 (((s3x) & 0xf) << 24) | (((s3y) & 0xf) << 28))

static void evergreen_emit_msaa_state(struct r600_context *rctx, int nr_samples)
{
	/* 2xMSAA
	 * There are two locations (-4, 4), (4, -4). */
	static uint32_t sample_locs_2x[] = {
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
	};
	static unsigned max_dist_2x = 4;
	/* 4xMSAA
	 * There are 4 locations: (-2, -2), (2, 2), (-6, 6), (6, -6). */
	static uint32_t sample_locs_4x[] = {
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
	};
	static unsigned max_dist_4x = 6;
	/* 8xMSAA */
	static uint32_t sample_locs_8x[] = {
		FILL_SREG(-1,  1,  1,  5,  3, -5,  5,  3),
		FILL_SREG(-7, -1, -3, -7,  7, -3, -5,  7),
		FILL_SREG(-1,  1,  1,  5,  3, -5,  5,  3),
		FILL_SREG(-7, -1, -3, -7,  7, -3, -5,  7),
		FILL_SREG(-1,  1,  1,  5,  3, -5,  5,  3),
		FILL_SREG(-7, -1, -3, -7,  7, -3, -5,  7),
		FILL_SREG(-1,  1,  1,  5,  3, -5,  5,  3),
		FILL_SREG(-7, -1, -3, -7,  7, -3, -5,  7),
	};
	static unsigned max_dist_8x = 7;

	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	unsigned max_dist = 0;

	switch (nr_samples) {
	default:
		nr_samples = 0;
		break;
	case 2:
		r600_write_context_reg_seq(cs, R_028C1C_PA_SC_AA_SAMPLE_LOCS_0, Elements(sample_locs_2x));
		r600_write_array(cs, Elements(sample_locs_2x), sample_locs_2x);
		max_dist = max_dist_2x;
		break;
	case 4:
		r600_write_context_reg_seq(cs, R_028C1C_PA_SC_AA_SAMPLE_LOCS_0, Elements(sample_locs_4x));
		r600_write_array(cs, Elements(sample_locs_4x), sample_locs_4x);
		max_dist = max_dist_4x;
		break;
	case 8:
		r600_write_context_reg_seq(cs, R_028C1C_PA_SC_AA_SAMPLE_LOCS_0, Elements(sample_locs_8x));
		r600_write_array(cs, Elements(sample_locs_8x), sample_locs_8x);
		max_dist = max_dist_8x;
		break;
	}

	if (nr_samples > 1) {
		r600_write_context_reg_seq(cs, R_028C00_PA_SC_LINE_CNTL, 2);
		r600_write_value(cs, S_028C00_LAST_PIXEL(1) |
				     S_028C00_EXPAND_LINE_WIDTH(1)); /* R_028C00_PA_SC_LINE_CNTL */
		r600_write_value(cs, S_028C04_MSAA_NUM_SAMPLES(util_logbase2(nr_samples)) |
				     S_028C04_MAX_SAMPLE_DIST(max_dist)); /* R_028C04_PA_SC_AA_CONFIG */
	} else {
		r600_write_context_reg_seq(cs, R_028C00_PA_SC_LINE_CNTL, 2);
		r600_write_value(cs, S_028C00_LAST_PIXEL(1)); /* R_028C00_PA_SC_LINE_CNTL */
		r600_write_value(cs, 0); /* R_028C04_PA_SC_AA_CONFIG */
	}
}

static void cayman_emit_msaa_state(struct r600_context *rctx, int nr_samples)
{
	/* 2xMSAA
	 * There are two locations (-4, 4), (4, -4). */
	static uint32_t sample_locs_2x[] = {
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
		FILL_SREG(-4, 4, 4, -4, -4, 4, 4, -4),
	};
	static unsigned max_dist_2x = 4;
	/* 4xMSAA
	 * There are 4 locations: (-2, -2), (2, 2), (-6, 6), (6, -6). */
	static uint32_t sample_locs_4x[] = {
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
		FILL_SREG(-2, -2, 2, 2, -6, 6, 6, -6),
	};
	static unsigned max_dist_4x = 6;
	/* 8xMSAA */
	static uint32_t sample_locs_8x[] = {
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG(-2, -5, 3, -4, -1, 5, -6, -2),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
		FILL_SREG( 6,  0, 0,  0, -5, 3,  4,  4),
	};
	static unsigned max_dist_8x = 8;
	/* 16xMSAA */
	static uint32_t sample_locs_16x[] = {
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-7, -3, 7, 3, 1, -5, -5, 5),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-3, -7, 3, 7, 5, -1, -1, 1),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-8, -6, 4, 2, 2, -8, -2, 6),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
		FILL_SREG(-4, -2, 0, 4, 6, -4, -6, 0),
	};
	static unsigned max_dist_16x = 8;

	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	unsigned max_dist = 0;

	switch (nr_samples) {
	default:
		nr_samples = 0;
		break;
	case 2:
		r600_write_context_reg(cs, CM_R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, sample_locs_2x[0]);
		r600_write_context_reg(cs, CM_R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0, sample_locs_2x[1]);
		r600_write_context_reg(cs, CM_R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0, sample_locs_2x[2]);
		r600_write_context_reg(cs, CM_R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0, sample_locs_2x[3]);
		max_dist = max_dist_2x;
		break;
	case 4:
		r600_write_context_reg(cs, CM_R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, sample_locs_4x[0]);
		r600_write_context_reg(cs, CM_R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0, sample_locs_4x[1]);
		r600_write_context_reg(cs, CM_R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0, sample_locs_4x[2]);
		r600_write_context_reg(cs, CM_R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0, sample_locs_4x[3]);
		max_dist = max_dist_4x;
		break;
	case 8:
		r600_write_context_reg_seq(cs, CM_R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, 14);
		r600_write_value(cs, sample_locs_8x[0]);
		r600_write_value(cs, sample_locs_8x[4]);
		r600_write_value(cs, 0);
		r600_write_value(cs, 0);
		r600_write_value(cs, sample_locs_8x[1]);
		r600_write_value(cs, sample_locs_8x[5]);
		r600_write_value(cs, 0);
		r600_write_value(cs, 0);
		r600_write_value(cs, sample_locs_8x[2]);
		r600_write_value(cs, sample_locs_8x[6]);
		r600_write_value(cs, 0);
		r600_write_value(cs, 0);
		r600_write_value(cs, sample_locs_8x[3]);
		r600_write_value(cs, sample_locs_8x[7]);
		max_dist = max_dist_8x;
		break;
	case 16:
		r600_write_context_reg_seq(cs, CM_R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, 16);
		r600_write_value(cs, sample_locs_16x[0]);
		r600_write_value(cs, sample_locs_16x[4]);
		r600_write_value(cs, sample_locs_16x[8]);
		r600_write_value(cs, sample_locs_16x[12]);
		r600_write_value(cs, sample_locs_16x[1]);
		r600_write_value(cs, sample_locs_16x[5]);
		r600_write_value(cs, sample_locs_16x[9]);
		r600_write_value(cs, sample_locs_16x[13]);
		r600_write_value(cs, sample_locs_16x[2]);
		r600_write_value(cs, sample_locs_16x[6]);
		r600_write_value(cs, sample_locs_16x[10]);
		r600_write_value(cs, sample_locs_16x[14]);
		r600_write_value(cs, sample_locs_16x[3]);
		r600_write_value(cs, sample_locs_16x[7]);
		r600_write_value(cs, sample_locs_16x[11]);
		r600_write_value(cs, sample_locs_16x[15]);
		max_dist = max_dist_16x;
		break;
	}

	if (nr_samples > 1) {
		unsigned log_samples = util_logbase2(nr_samples);

		r600_write_context_reg_seq(cs, CM_R_028BDC_PA_SC_LINE_CNTL, 2);
		r600_write_value(cs, S_028C00_LAST_PIXEL(1) |
				     S_028C00_EXPAND_LINE_WIDTH(1)); /* CM_R_028BDC_PA_SC_LINE_CNTL */
		r600_write_value(cs, S_028BE0_MSAA_NUM_SAMPLES(log_samples) |
				     S_028BE0_MAX_SAMPLE_DIST(max_dist) |
				     S_028BE0_MSAA_EXPOSED_SAMPLES(log_samples)); /* CM_R_028BE0_PA_SC_AA_CONFIG */

		r600_write_context_reg(cs, CM_R_028804_DB_EQAA,
				       S_028804_MAX_ANCHOR_SAMPLES(log_samples) |
				       S_028804_PS_ITER_SAMPLES(log_samples) |
				       S_028804_MASK_EXPORT_NUM_SAMPLES(log_samples) |
				       S_028804_ALPHA_TO_MASK_NUM_SAMPLES(log_samples) |
				       S_028804_HIGH_QUALITY_INTERSECTIONS(1) |
				       S_028804_STATIC_ANCHOR_ASSOCIATIONS(1));
	} else {
		r600_write_context_reg_seq(cs, CM_R_028BDC_PA_SC_LINE_CNTL, 2);
		r600_write_value(cs, S_028C00_LAST_PIXEL(1)); /* CM_R_028BDC_PA_SC_LINE_CNTL */
		r600_write_value(cs, 0); /* CM_R_028BE0_PA_SC_AA_CONFIG */

		r600_write_context_reg(cs, CM_R_028804_DB_EQAA,
				       S_028804_HIGH_QUALITY_INTERSECTIONS(1) |
				       S_028804_STATIC_ANCHOR_ASSOCIATIONS(1));
	}
}

static void evergreen_emit_framebuffer_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct pipe_framebuffer_state *state = &rctx->framebuffer.state;
	unsigned nr_cbufs = state->nr_cbufs;
	unsigned i, tl, br;

	/* XXX support more colorbuffers once we need them */
	assert(nr_cbufs <= 8);
	if (nr_cbufs > 8)
		nr_cbufs = 8;

	/* Colorbuffers. */
	for (i = 0; i < nr_cbufs; i++) {
		struct r600_surface *cb = (struct r600_surface*)state->cbufs[i];
		unsigned reloc = r600_context_bo_reloc(rctx,
						       &rctx->rings.gfx,
						       (struct r600_resource*)cb->base.texture,
						       RADEON_USAGE_READWRITE);

		r600_write_context_reg_seq(cs, R_028C60_CB_COLOR0_BASE + i * 0x3C, 11);
		r600_write_value(cs, cb->cb_color_base);	/* R_028C60_CB_COLOR0_BASE */
		r600_write_value(cs, cb->cb_color_pitch);	/* R_028C64_CB_COLOR0_PITCH */
		r600_write_value(cs, cb->cb_color_slice);	/* R_028C68_CB_COLOR0_SLICE */
		r600_write_value(cs, cb->cb_color_view);	/* R_028C6C_CB_COLOR0_VIEW */
		r600_write_value(cs, cb->cb_color_info);	/* R_028C70_CB_COLOR0_INFO */
		r600_write_value(cs, cb->cb_color_attrib);	/* R_028C74_CB_COLOR0_ATTRIB */
		r600_write_value(cs, cb->cb_color_dim);		/* R_028C78_CB_COLOR0_DIM */
		r600_write_value(cs, cb->cb_color_cmask);	/* R_028C7C_CB_COLOR0_CMASK */
		r600_write_value(cs, cb->cb_color_cmask_slice);	/* R_028C80_CB_COLOR0_CMASK_SLICE */
		r600_write_value(cs, cb->cb_color_fmask);	/* R_028C84_CB_COLOR0_FMASK */
		r600_write_value(cs, cb->cb_color_fmask_slice); /* R_028C88_CB_COLOR0_FMASK_SLICE */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C60_CB_COLOR0_BASE */
		r600_write_value(cs, reloc);

		if (!rctx->keep_tiling_flags) {
			r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C70_CB_COLOR0_INFO */
			r600_write_value(cs, reloc);
		}

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C74_CB_COLOR0_ATTRIB */
		r600_write_value(cs, reloc);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C7C_CB_COLOR0_CMASK */
		r600_write_value(cs, reloc);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C84_CB_COLOR0_FMASK */
		r600_write_value(cs, reloc);
	}
	/* set CB_COLOR1_INFO for possible dual-src blending */
	if (i == 1 && !((struct r600_texture*)state->cbufs[0]->texture)->is_rat) {
		r600_write_context_reg(cs, R_028C70_CB_COLOR0_INFO + 1 * 0x3C,
				       ((struct r600_surface*)state->cbufs[0])->cb_color_info);

		if (!rctx->keep_tiling_flags) {
			unsigned reloc = r600_context_bo_reloc(rctx,
							       &rctx->rings.gfx,
							       (struct r600_resource*)state->cbufs[0]->texture,
							       RADEON_USAGE_READWRITE);

			r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C70_CB_COLOR0_INFO */
			r600_write_value(cs, reloc);
		}
		i++;
	}
	if (rctx->keep_tiling_flags) {
		for (; i < 8 ; i++) {
			r600_write_context_reg(cs, R_028C70_CB_COLOR0_INFO + i * 0x3C, 0);
		}
		for (; i < 12; i++) {
			r600_write_context_reg(cs, R_028E50_CB_COLOR8_INFO + (i - 8) * 0x1C, 0);
		}
	}

	/* ZS buffer. */
	if (state->zsbuf) {
		struct r600_surface *zb = (struct r600_surface*)state->zsbuf;
		unsigned reloc = r600_context_bo_reloc(rctx,
						       &rctx->rings.gfx,
						       (struct r600_resource*)state->zsbuf->texture,
						       RADEON_USAGE_READWRITE);

		r600_write_context_reg(cs, R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL,
				       zb->pa_su_poly_offset_db_fmt_cntl);
		r600_write_context_reg(cs, R_028008_DB_DEPTH_VIEW, zb->db_depth_view);

		r600_write_context_reg_seq(cs, R_028040_DB_Z_INFO, 8);
		r600_write_value(cs, zb->db_depth_info);	/* R_028040_DB_Z_INFO */
		r600_write_value(cs, zb->db_stencil_info);	/* R_028044_DB_STENCIL_INFO */
		r600_write_value(cs, zb->db_depth_base);	/* R_028048_DB_Z_READ_BASE */
		r600_write_value(cs, zb->db_stencil_base);	/* R_02804C_DB_STENCIL_READ_BASE */
		r600_write_value(cs, zb->db_depth_base);	/* R_028050_DB_Z_WRITE_BASE */
		r600_write_value(cs, zb->db_stencil_base);	/* R_028054_DB_STENCIL_WRITE_BASE */
		r600_write_value(cs, zb->db_depth_size);	/* R_028058_DB_DEPTH_SIZE */
		r600_write_value(cs, zb->db_depth_slice);	/* R_02805C_DB_DEPTH_SLICE */

		if (!rctx->keep_tiling_flags) {
			r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028040_DB_Z_INFO */
			r600_write_value(cs, reloc);
		}

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028048_DB_Z_READ_BASE */
		r600_write_value(cs, reloc);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_02804C_DB_STENCIL_READ_BASE */
		r600_write_value(cs, reloc);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028050_DB_Z_WRITE_BASE */
		r600_write_value(cs, reloc);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028054_DB_STENCIL_WRITE_BASE */
		r600_write_value(cs, reloc);
	} else if (rctx->screen->info.drm_minor >= 18) {
		/* DRM 2.6.18 allows the INVALID format to disable depth/stencil.
		 * Older kernels are out of luck. */
		r600_write_context_reg_seq(cs, R_028040_DB_Z_INFO, 2);
		r600_write_value(cs, S_028040_FORMAT(V_028040_Z_INVALID)); /* R_028040_DB_Z_INFO */
		r600_write_value(cs, S_028044_FORMAT(V_028044_STENCIL_INVALID)); /* R_028044_DB_STENCIL_INFO */
	}

	/* Framebuffer dimensions. */
	evergreen_get_scissor_rect(rctx, 0, 0, state->width, state->height, &tl, &br);

	r600_write_context_reg_seq(cs, R_028204_PA_SC_WINDOW_SCISSOR_TL, 2);
	r600_write_value(cs, tl); /* R_028204_PA_SC_WINDOW_SCISSOR_TL */
	r600_write_value(cs, br); /* R_028208_PA_SC_WINDOW_SCISSOR_BR */

	if (rctx->chip_class == EVERGREEN) {
		evergreen_emit_msaa_state(rctx, rctx->framebuffer.nr_samples);
	} else {
		cayman_emit_msaa_state(rctx, rctx->framebuffer.nr_samples);
	}
}

static void evergreen_emit_polygon_offset(struct r600_context *rctx, struct r600_atom *a)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_poly_offset_state *state = (struct r600_poly_offset_state*)a;
	float offset_units = state->offset_units;
	float offset_scale = state->offset_scale;

	switch (state->zs_format) {
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_UINT:
	case PIPE_FORMAT_X8Z24_UNORM:
	case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		offset_units *= 2.0f;
		break;
	case PIPE_FORMAT_Z16_UNORM:
		offset_units *= 4.0f;
		break;
	default:;
	}

	r600_write_context_reg_seq(cs, R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE, 4);
	r600_write_value(cs, fui(offset_scale));
	r600_write_value(cs, fui(offset_units));
	r600_write_value(cs, fui(offset_scale));
	r600_write_value(cs, fui(offset_units));
}

static void evergreen_emit_cb_misc_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_cb_misc_state *a = (struct r600_cb_misc_state*)atom;
	unsigned fb_colormask = (1ULL << ((unsigned)a->nr_cbufs * 4)) - 1;
	unsigned ps_colormask = (1ULL << ((unsigned)a->nr_ps_color_outputs * 4)) - 1;

	r600_write_context_reg_seq(cs, R_028238_CB_TARGET_MASK, 2);
	r600_write_value(cs, a->blend_colormask & fb_colormask); /* R_028238_CB_TARGET_MASK */
	/* Always enable the first colorbuffer in CB_SHADER_MASK. This
	 * will assure that the alpha-test will work even if there is
	 * no colorbuffer bound. */
	r600_write_value(cs, 0xf | (a->dual_src_blend ? ps_colormask : 0) | fb_colormask); /* R_02823C_CB_SHADER_MASK */
}

static void evergreen_emit_db_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_db_state *a = (struct r600_db_state*)atom;

	if (a->rsurf && a->rsurf->htile_enabled) {
		struct r600_texture *rtex = (struct r600_texture *)a->rsurf->base.texture;
		unsigned reloc_idx;

		r600_write_context_reg(cs, R_02802C_DB_DEPTH_CLEAR, fui(rtex->depth_clear));
		r600_write_context_reg(cs, R_028ABC_DB_HTILE_SURFACE, a->rsurf->db_htile_surface);
		r600_write_context_reg(cs, R_028AC8_DB_PRELOAD_CONTROL, a->rsurf->db_preload_control);
		r600_write_context_reg(cs, R_028014_DB_HTILE_DATA_BASE, a->rsurf->db_htile_data_base);
		reloc_idx = r600_context_bo_reloc(rctx, &rctx->rings.gfx, rtex->htile, RADEON_USAGE_READWRITE);
		cs->buf[cs->cdw++] = PKT3(PKT3_NOP, 0, 0);
		cs->buf[cs->cdw++] = reloc_idx;
	} else {
		r600_write_context_reg(cs, R_028ABC_DB_HTILE_SURFACE, 0);
		r600_write_context_reg(cs, R_028AC8_DB_PRELOAD_CONTROL, 0);
	}
}

static void evergreen_emit_db_misc_state(struct r600_context *rctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_db_misc_state *a = (struct r600_db_misc_state*)atom;
	unsigned db_render_control = 0;
	unsigned db_count_control = 0;
	unsigned db_render_override =
		S_02800C_FORCE_HIS_ENABLE0(V_02800C_FORCE_DISABLE) |
		S_02800C_FORCE_HIS_ENABLE1(V_02800C_FORCE_DISABLE);

	if (a->occlusion_query_enabled) {
		db_count_control |= S_028004_PERFECT_ZPASS_COUNTS(1);
		if (rctx->chip_class == CAYMAN) {
			db_count_control |= S_028004_SAMPLE_RATE(a->log_samples);
		}
		db_render_override |= S_02800C_NOOP_CULL_DISABLE(1);
	}
	/* FIXME we should be able to use hyperz even if we are not writing to
	 * zbuffer but somehow this trigger GPU lockup. See :
	 *
	 * https://bugs.freedesktop.org/show_bug.cgi?id=60848
	 *
	 * Disable hyperz for now if not writing to zbuffer.
	 */
	if (rctx->db_state.rsurf && rctx->db_state.rsurf->htile_enabled && rctx->zwritemask) {
		/* FORCE_OFF means HiZ/HiS are determined by DB_SHADER_CONTROL */
		db_render_override |= S_02800C_FORCE_HIZ_ENABLE(V_02800C_FORCE_OFF);
		/* This is to fix a lockup when hyperz and alpha test are enabled at
		 * the same time somehow GPU get confuse on which order to pick for
		 * z test
		 */
		if (rctx->alphatest_state.sx_alpha_test_control) {
			db_render_override |= S_02800C_FORCE_SHADER_Z_ORDER(1);
		}
	} else {
		db_render_override |= S_02800C_FORCE_HIZ_ENABLE(V_02800C_FORCE_DISABLE);
	}
	if (a->flush_depthstencil_through_cb) {
		assert(a->copy_depth || a->copy_stencil);

		db_render_control |= S_028000_DEPTH_COPY_ENABLE(a->copy_depth) |
				     S_028000_STENCIL_COPY_ENABLE(a->copy_stencil) |
				     S_028000_COPY_CENTROID(1) |
				     S_028000_COPY_SAMPLE(a->copy_sample);
	} else if (a->flush_depthstencil_in_place) {
		db_render_control |= S_028000_DEPTH_COMPRESS_DISABLE(1) |
				     S_028000_STENCIL_COMPRESS_DISABLE(1);
		db_render_override |= S_02800C_DISABLE_PIXEL_RATE_TILES(1);
	}
	if (a->htile_clear) {
		/* FIXME we might want to disable cliprect here */
		db_render_control |= S_028000_DEPTH_CLEAR_ENABLE(1);
	}

	r600_write_context_reg_seq(cs, R_028000_DB_RENDER_CONTROL, 2);
	r600_write_value(cs, db_render_control); /* R_028000_DB_RENDER_CONTROL */
	r600_write_value(cs, db_count_control); /* R_028004_DB_COUNT_CONTROL */
	r600_write_context_reg(cs, R_02800C_DB_RENDER_OVERRIDE, db_render_override);
	r600_write_context_reg(cs, R_02880C_DB_SHADER_CONTROL, a->db_shader_control);
}

static void evergreen_emit_vertex_buffers(struct r600_context *rctx,
					  struct r600_vertexbuf_state *state,
					  unsigned resource_offset,
					  unsigned pkt_flags)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	uint32_t dirty_mask = state->dirty_mask;

	while (dirty_mask) {
		struct pipe_vertex_buffer *vb;
		struct r600_resource *rbuffer;
		uint64_t va;
		unsigned buffer_index = u_bit_scan(&dirty_mask);

		vb = &state->vb[buffer_index];
		rbuffer = (struct r600_resource*)vb->buffer;
		assert(rbuffer);

		va = r600_resource_va(&rctx->screen->screen, &rbuffer->b.b);
		va += vb->buffer_offset;

		/* fetch resources start at index 992 */
		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 8, 0) | pkt_flags);
		r600_write_value(cs, (resource_offset + buffer_index) * 8);
		r600_write_value(cs, va); /* RESOURCEi_WORD0 */
		r600_write_value(cs, rbuffer->buf->size - vb->buffer_offset - 1); /* RESOURCEi_WORD1 */
		r600_write_value(cs, /* RESOURCEi_WORD2 */
				 S_030008_ENDIAN_SWAP(r600_endian_swap(32)) |
				 S_030008_STRIDE(vb->stride) |
				 S_030008_BASE_ADDRESS_HI(va >> 32UL));
		r600_write_value(cs, /* RESOURCEi_WORD3 */
				 S_03000C_DST_SEL_X(V_03000C_SQ_SEL_X) |
				 S_03000C_DST_SEL_Y(V_03000C_SQ_SEL_Y) |
				 S_03000C_DST_SEL_Z(V_03000C_SQ_SEL_Z) |
				 S_03000C_DST_SEL_W(V_03000C_SQ_SEL_W));
		r600_write_value(cs, 0); /* RESOURCEi_WORD4 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD5 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD6 */
		r600_write_value(cs, 0xc0000000); /* RESOURCEi_WORD7 */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0) | pkt_flags);
		r600_write_value(cs, r600_context_bo_reloc(rctx, &rctx->rings.gfx, rbuffer, RADEON_USAGE_READ));
	}
	state->dirty_mask = 0;
}

static void evergreen_fs_emit_vertex_buffers(struct r600_context *rctx, struct r600_atom * atom)
{
	evergreen_emit_vertex_buffers(rctx, &rctx->vertex_buffer_state, 992, 0);
}

static void evergreen_cs_emit_vertex_buffers(struct r600_context *rctx, struct r600_atom * atom)
{
	evergreen_emit_vertex_buffers(rctx, &rctx->cs_vertex_buffer_state, 816,
				      RADEON_CP_PACKET3_COMPUTE_MODE);
}

static void evergreen_emit_constant_buffers(struct r600_context *rctx,
					    struct r600_constbuf_state *state,
					    unsigned buffer_id_base,
					    unsigned reg_alu_constbuf_size,
					    unsigned reg_alu_const_cache)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	uint32_t dirty_mask = state->dirty_mask;

	while (dirty_mask) {
		struct pipe_constant_buffer *cb;
		struct r600_resource *rbuffer;
		uint64_t va;
		unsigned buffer_index = ffs(dirty_mask) - 1;

		cb = &state->cb[buffer_index];
		rbuffer = (struct r600_resource*)cb->buffer;
		assert(rbuffer);

		va = r600_resource_va(&rctx->screen->screen, &rbuffer->b.b);
		va += cb->buffer_offset;

		r600_write_context_reg(cs, reg_alu_constbuf_size + buffer_index * 4,
				       ALIGN_DIVUP(cb->buffer_size >> 4, 16));
		r600_write_context_reg(cs, reg_alu_const_cache + buffer_index * 4, va >> 8);

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, r600_context_bo_reloc(rctx, &rctx->rings.gfx, rbuffer, RADEON_USAGE_READ));

		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 8, 0));
		r600_write_value(cs, (buffer_id_base + buffer_index) * 8);
		r600_write_value(cs, va); /* RESOURCEi_WORD0 */
		r600_write_value(cs, rbuffer->buf->size - cb->buffer_offset - 1); /* RESOURCEi_WORD1 */
		r600_write_value(cs, /* RESOURCEi_WORD2 */
				 S_030008_ENDIAN_SWAP(r600_endian_swap(32)) |
				 S_030008_STRIDE(16) |
				 S_030008_BASE_ADDRESS_HI(va >> 32UL));
		r600_write_value(cs, /* RESOURCEi_WORD3 */
				 S_03000C_DST_SEL_X(V_03000C_SQ_SEL_X) |
				 S_03000C_DST_SEL_Y(V_03000C_SQ_SEL_Y) |
				 S_03000C_DST_SEL_Z(V_03000C_SQ_SEL_Z) |
				 S_03000C_DST_SEL_W(V_03000C_SQ_SEL_W));
		r600_write_value(cs, 0); /* RESOURCEi_WORD4 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD5 */
		r600_write_value(cs, 0); /* RESOURCEi_WORD6 */
		r600_write_value(cs, 0xc0000000); /* RESOURCEi_WORD7 */

		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, r600_context_bo_reloc(rctx, &rctx->rings.gfx, rbuffer, RADEON_USAGE_READ));

		dirty_mask &= ~(1 << buffer_index);
	}
	state->dirty_mask = 0;
}

static void evergreen_emit_vs_constant_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_constant_buffers(rctx, &rctx->constbuf_state[PIPE_SHADER_VERTEX], 176,
					R_028180_ALU_CONST_BUFFER_SIZE_VS_0,
					R_028980_ALU_CONST_CACHE_VS_0);
}

static void evergreen_emit_gs_constant_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_constant_buffers(rctx, &rctx->constbuf_state[PIPE_SHADER_GEOMETRY], 336,
					R_0281C0_ALU_CONST_BUFFER_SIZE_GS_0,
					R_0289C0_ALU_CONST_CACHE_GS_0);
}

static void evergreen_emit_ps_constant_buffers(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_constant_buffers(rctx, &rctx->constbuf_state[PIPE_SHADER_FRAGMENT], 0,
				       R_028140_ALU_CONST_BUFFER_SIZE_PS_0,
				       R_028940_ALU_CONST_CACHE_PS_0);
}

static void evergreen_emit_sampler_views(struct r600_context *rctx,
					 struct r600_samplerview_state *state,
					 unsigned resource_id_base)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	uint32_t dirty_mask = state->dirty_mask;

	while (dirty_mask) {
		struct r600_pipe_sampler_view *rview;
		unsigned resource_index = u_bit_scan(&dirty_mask);
		unsigned reloc;

		rview = state->views[resource_index];
		assert(rview);

		r600_write_value(cs, PKT3(PKT3_SET_RESOURCE, 8, 0));
		r600_write_value(cs, (resource_id_base + resource_index) * 8);
		r600_write_array(cs, 8, rview->tex_resource_words);

		reloc = r600_context_bo_reloc(rctx, &rctx->rings.gfx, rview->tex_resource,
					      RADEON_USAGE_READ);
		r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
		r600_write_value(cs, reloc);

		if (!rview->skip_mip_address_reloc) {
			r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
			r600_write_value(cs, reloc);
		}
	}
	state->dirty_mask = 0;
}

static void evergreen_emit_vs_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_views(rctx, &rctx->samplers[PIPE_SHADER_VERTEX].views, 176 + R600_MAX_CONST_BUFFERS);
}

static void evergreen_emit_gs_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_views(rctx, &rctx->samplers[PIPE_SHADER_GEOMETRY].views, 336 + R600_MAX_CONST_BUFFERS);
}

static void evergreen_emit_ps_sampler_views(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_views(rctx, &rctx->samplers[PIPE_SHADER_FRAGMENT].views, R600_MAX_CONST_BUFFERS);
}

static void evergreen_emit_sampler_states(struct r600_context *rctx,
				struct r600_textures_info *texinfo,
				unsigned resource_id_base,
				unsigned border_index_reg)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	uint32_t dirty_mask = texinfo->states.dirty_mask;

	while (dirty_mask) {
		struct r600_pipe_sampler_state *rstate;
		unsigned i = u_bit_scan(&dirty_mask);

		rstate = texinfo->states.states[i];
		assert(rstate);

		r600_write_value(cs, PKT3(PKT3_SET_SAMPLER, 3, 0));
		r600_write_value(cs, (resource_id_base + i) * 3);
		r600_write_array(cs, 3, rstate->tex_sampler_words);

		if (rstate->border_color_use) {
			r600_write_config_reg_seq(cs, border_index_reg, 5);
			r600_write_value(cs, i);
			r600_write_array(cs, 4, rstate->border_color.ui);
		}
	}
	texinfo->states.dirty_mask = 0;
}

static void evergreen_emit_vs_sampler_states(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_states(rctx, &rctx->samplers[PIPE_SHADER_VERTEX], 18, R_00A414_TD_VS_SAMPLER0_BORDER_INDEX);
}

static void evergreen_emit_gs_sampler_states(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_states(rctx, &rctx->samplers[PIPE_SHADER_GEOMETRY], 36, R_00A428_TD_GS_SAMPLER0_BORDER_INDEX);
}

static void evergreen_emit_ps_sampler_states(struct r600_context *rctx, struct r600_atom *atom)
{
	evergreen_emit_sampler_states(rctx, &rctx->samplers[PIPE_SHADER_FRAGMENT], 0, R_00A400_TD_PS_SAMPLER0_BORDER_INDEX);
}

static void evergreen_emit_sample_mask(struct r600_context *rctx, struct r600_atom *a)
{
	struct r600_sample_mask *s = (struct r600_sample_mask*)a;
	uint8_t mask = s->sample_mask;

	r600_write_context_reg(rctx->rings.gfx.cs, R_028C3C_PA_SC_AA_MASK,
			       mask | (mask << 8) | (mask << 16) | (mask << 24));
}

static void cayman_emit_sample_mask(struct r600_context *rctx, struct r600_atom *a)
{
	struct r600_sample_mask *s = (struct r600_sample_mask*)a;
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	uint16_t mask = s->sample_mask;

	r600_write_context_reg_seq(cs, CM_R_028C38_PA_SC_AA_MASK_X0Y0_X1Y0, 2);
	r600_write_value(cs, mask | (mask << 16)); /* X0Y0_X1Y0 */
	r600_write_value(cs, mask | (mask << 16)); /* X0Y1_X1Y1 */
}

static void evergreen_emit_vertex_fetch_shader(struct r600_context *rctx, struct r600_atom *a)
{
	struct radeon_winsys_cs *cs = rctx->rings.gfx.cs;
	struct r600_cso_state *state = (struct r600_cso_state*)a;
	struct r600_fetch_shader *shader = (struct r600_fetch_shader*)state->cso;

	r600_write_context_reg(cs, R_0288A4_SQ_PGM_START_FS,
			       (r600_resource_va(rctx->context.screen, &shader->buffer->b.b) + shader->offset) >> 8);
	r600_write_value(cs, PKT3(PKT3_NOP, 0, 0));
	r600_write_value(cs, r600_context_bo_reloc(rctx, &rctx->rings.gfx, shader->buffer, RADEON_USAGE_READ));
}

void cayman_init_common_regs(struct r600_command_buffer *cb,
			     enum chip_class ctx_chip_class,
			     enum radeon_family ctx_family,
			     int ctx_drm_minor)
{
	r600_store_config_reg_seq(cb, R_008C00_SQ_CONFIG, 2);
	r600_store_value(cb, S_008C00_EXPORT_SRC_C(1)); /* R_008C00_SQ_CONFIG */
	/* always set the temp clauses */
	r600_store_value(cb, S_008C04_NUM_CLAUSE_TEMP_GPRS(4)); /* R_008C04_SQ_GPR_RESOURCE_MGMT_1 */

	r600_store_config_reg_seq(cb, R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1, 2);
	r600_store_value(cb, 0); /* R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1 */
	r600_store_value(cb, 0); /* R_008C14_SQ_GLOBAL_GPR_RESOURCE_MGMT_2 */

	r600_store_config_reg(cb, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, (1 << 8));

	r600_store_context_reg(cb, R_028A4C_PA_SC_MODE_CNTL_1, 0);

	r600_store_context_reg(cb, R_028354_SX_SURFACE_SYNC, S_028354_SURFACE_SYNC_MASK(0xf));

	r600_store_context_reg(cb, R_028800_DB_DEPTH_CONTROL, 0);
}

static void cayman_init_atom_start_cs(struct r600_context *rctx)
{
	struct r600_command_buffer *cb = &rctx->start_cs_cmd;

	r600_init_command_buffer(cb, 256);

	/* This must be first. */
	r600_store_value(cb, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
	r600_store_value(cb, 0x80000000);
	r600_store_value(cb, 0x80000000);

	/* We're setting config registers here. */
	r600_store_value(cb, PKT3(PKT3_EVENT_WRITE, 0, 0));
	r600_store_value(cb, EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4));

	cayman_init_common_regs(cb, rctx->chip_class,
				rctx->family, rctx->screen->info.drm_minor);

	r600_store_config_reg(cb, R_009100_SPI_CONFIG_CNTL, 0);
	r600_store_config_reg(cb, R_00913C_SPI_CONFIG_CNTL_1, S_00913C_VTX_DONE_DELAY(4));

	r600_store_context_reg_seq(cb, R_028900_SQ_ESGS_RING_ITEMSIZE, 6);
	r600_store_value(cb, 0); /* R_028900_SQ_ESGS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028904_SQ_GSVS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028908_SQ_ESTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_02890C_SQ_GSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028910_SQ_VSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028914_SQ_PSTMP_RING_ITEMSIZE */

	r600_store_context_reg_seq(cb, R_02891C_SQ_GS_VERT_ITEMSIZE, 4);
	r600_store_value(cb, 0); /* R_02891C_SQ_GS_VERT_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028920_SQ_GS_VERT_ITEMSIZE_1 */
	r600_store_value(cb, 0); /* R_028924_SQ_GS_VERT_ITEMSIZE_2 */
	r600_store_value(cb, 0); /* R_028928_SQ_GS_VERT_ITEMSIZE_3 */

	r600_store_context_reg_seq(cb, R_028A10_VGT_OUTPUT_PATH_CNTL, 13);
	r600_store_value(cb, 0); /* R_028A10_VGT_OUTPUT_PATH_CNTL */
	r600_store_value(cb, 0); /* R_028A14_VGT_HOS_CNTL */
	r600_store_value(cb, 0); /* R_028A18_VGT_HOS_MAX_TESS_LEVEL */
	r600_store_value(cb, 0); /* R_028A1C_VGT_HOS_MIN_TESS_LEVEL */
	r600_store_value(cb, 0); /* R_028A20_VGT_HOS_REUSE_DEPTH */
	r600_store_value(cb, 0); /* R_028A24_VGT_GROUP_PRIM_TYPE */
	r600_store_value(cb, 0); /* R_028A28_VGT_GROUP_FIRST_DECR */
	r600_store_value(cb, 0); /* R_028A2C_VGT_GROUP_DECR */
	r600_store_value(cb, 0); /* R_028A30_VGT_GROUP_VECT_0_CNTL */
	r600_store_value(cb, 0); /* R_028A34_VGT_GROUP_VECT_1_CNTL */
	r600_store_value(cb, 0); /* R_028A38_VGT_GROUP_VECT_0_FMT_CNTL */
	r600_store_value(cb, 0); /* R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL */
	r600_store_value(cb, 0); /* R_028A40_VGT_GS_MODE */

	r600_store_context_reg_seq(cb, R_028B94_VGT_STRMOUT_CONFIG, 2);
	r600_store_value(cb, 0); /* R_028B94_VGT_STRMOUT_CONFIG */
	r600_store_value(cb, 0); /* R_028B98_VGT_STRMOUT_BUFFER_CONFIG */

	r600_store_context_reg_seq(cb, R_028AB4_VGT_REUSE_OFF, 2);
	r600_store_value(cb, 0); /* R_028AB4_VGT_REUSE_OFF */
	r600_store_value(cb, 0); /* R_028AB8_VGT_VTX_CNT_EN */

	r600_store_config_reg(cb, R_008A14_PA_CL_ENHANCE, (3 << 1) | 1);

	r600_store_context_reg(cb, CM_R_028AA8_IA_MULTI_VGT_PARAM, S_028AA8_SWITCH_ON_EOP(1) | S_028AA8_PARTIAL_VS_WAVE_ON(1) | S_028AA8_PRIMGROUP_SIZE(63));

	r600_store_context_reg_seq(cb, CM_R_028BD4_PA_SC_CENTROID_PRIORITY_0, 2);
	r600_store_value(cb, 0x76543210); /* CM_R_028BD4_PA_SC_CENTROID_PRIORITY_0 */
	r600_store_value(cb, 0xfedcba98); /* CM_R_028BD8_PA_SC_CENTROID_PRIORITY_1 */

	r600_store_context_reg_seq(cb, CM_R_0288E8_SQ_LDS_ALLOC, 2);
	r600_store_value(cb, 0); /* CM_R_0288E8_SQ_LDS_ALLOC */
	r600_store_value(cb, 0); /* R_0288EC_SQ_LDS_ALLOC_PS */

        r600_store_context_reg(cb, R_0288F0_SQ_VTX_SEMANTIC_CLEAR, ~0);

        r600_store_context_reg_seq(cb, R_028400_VGT_MAX_VTX_INDX, 2);
	r600_store_value(cb, ~0); /* R_028400_VGT_MAX_VTX_INDX */
	r600_store_value(cb, 0); /* R_028404_VGT_MIN_VTX_INDX */

	r600_store_ctl_const(cb, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0);

	r600_store_context_reg(cb, R_028028_DB_STENCIL_CLEAR, 0);

	r600_store_context_reg(cb, R_0286DC_SPI_FOG_CNTL, 0);

	r600_store_context_reg_seq(cb, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 3);
	r600_store_value(cb, 0); /* R_028AC0_DB_SRESULTS_COMPARE_STATE0 */
	r600_store_value(cb, 0); /* R_028AC4_DB_SRESULTS_COMPARE_STATE1 */
	r600_store_value(cb, 0); /* R_028AC8_DB_PRELOAD_CONTROL */

	r600_store_context_reg(cb, R_028200_PA_SC_WINDOW_OFFSET, 0);
	r600_store_context_reg(cb, R_02820C_PA_SC_CLIPRECT_RULE, 0xFFFF);

	r600_store_context_reg_seq(cb, R_0282D0_PA_SC_VPORT_ZMIN_0, 2);
	r600_store_value(cb, 0); /* R_0282D0_PA_SC_VPORT_ZMIN_0 */
	r600_store_value(cb, 0x3F800000); /* R_0282D4_PA_SC_VPORT_ZMAX_0 */

	r600_store_context_reg(cb, R_028230_PA_SC_EDGERULE, 0xAAAAAAAA);
	r600_store_context_reg(cb, R_028818_PA_CL_VTE_CNTL, 0x0000043F);
	r600_store_context_reg(cb, R_028820_PA_CL_NANINF_CNTL, 0);

	r600_store_context_reg_seq(cb, CM_R_028BE8_PA_CL_GB_VERT_CLIP_ADJ, 4);
	r600_store_value(cb, 0x3F800000); /* CM_R_028BE8_PA_CL_GB_VERT_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* CM_R_028BEC_PA_CL_GB_VERT_DISC_ADJ */
	r600_store_value(cb, 0x3F800000); /* CM_R_028BF0_PA_CL_GB_HORZ_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* CM_R_028BF4_PA_CL_GB_HORZ_DISC_ADJ */

	r600_store_context_reg_seq(cb, R_028240_PA_SC_GENERIC_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028240_PA_SC_GENERIC_SCISSOR_TL */
	r600_store_value(cb, S_028244_BR_X(16384) | S_028244_BR_Y(16384)); /* R_028244_PA_SC_GENERIC_SCISSOR_BR */

	r600_store_context_reg_seq(cb, R_028030_PA_SC_SCREEN_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028030_PA_SC_SCREEN_SCISSOR_TL */
	r600_store_value(cb, S_028034_BR_X(16384) | S_028034_BR_Y(16384)); /* R_028034_PA_SC_SCREEN_SCISSOR_BR */

	r600_store_context_reg(cb, R_028848_SQ_PGM_RESOURCES_2_PS, S_028848_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));
	r600_store_context_reg(cb, R_028864_SQ_PGM_RESOURCES_2_VS, S_028864_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));
	r600_store_context_reg(cb, R_0288A8_SQ_PGM_RESOURCES_FS, 0);

	/* to avoid GPU doing any preloading of constant from random address */
	r600_store_context_reg_seq(cb, R_028140_ALU_CONST_BUFFER_SIZE_PS_0, 16);
	r600_store_value(cb, 0); /* R_028140_ALU_CONST_BUFFER_SIZE_PS_0 */
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);

	r600_store_context_reg_seq(cb, R_028180_ALU_CONST_BUFFER_SIZE_VS_0, 16);
	r600_store_value(cb, 0); /* R_028180_ALU_CONST_BUFFER_SIZE_VS_0 */
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);

	if (rctx->screen->has_streamout) {
		r600_store_context_reg(cb, R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET, 0);
	}

	r600_store_context_reg(cb, R_028010_DB_RENDER_OVERRIDE2, 0);
	r600_store_context_reg(cb, R_028234_PA_SU_HARDWARE_SCREEN_OFFSET, 0);
	r600_store_context_reg(cb, R_0286C8_SPI_THREAD_GROUPING, 0);
	r600_store_context_reg_seq(cb, R_0286E4_SPI_PS_IN_CONTROL_2, 2);
	r600_store_value(cb, 0); /* R_0286E4_SPI_PS_IN_CONTROL_2 */
	r600_store_value(cb, 0); /* R_0286E8_SPI_COMPUTE_INPUT_CNTL */
	r600_store_context_reg(cb, R_028B54_VGT_SHADER_STAGES_EN, 0);

	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0, 0x01000FFF);
	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0 + (32 * 4), 0x01000FFF);
}

void evergreen_init_common_regs(struct r600_command_buffer *cb,
	enum chip_class ctx_chip_class,
	enum radeon_family ctx_family,
	int ctx_drm_minor)
{
	int ps_prio;
	int vs_prio;
	int gs_prio;
	int es_prio;

	int hs_prio;
	int cs_prio;
	int ls_prio;

	int num_ps_gprs;
	int num_vs_gprs;
	int num_gs_gprs;
	int num_es_gprs;
	int num_hs_gprs;
	int num_ls_gprs;
	int num_temp_gprs;

	unsigned tmp;

	ps_prio = 0;
	vs_prio = 1;
	gs_prio = 2;
	es_prio = 3;
	hs_prio = 0;
	ls_prio = 0;
	cs_prio = 0;

	num_ps_gprs = 93;
	num_vs_gprs = 46;
	num_temp_gprs = 4;
	num_gs_gprs = 31;
	num_es_gprs = 31;
	num_hs_gprs = 23;
	num_ls_gprs = 23;

	tmp = 0;
	switch (ctx_family) {
	case CHIP_CEDAR:
	case CHIP_PALM:
	case CHIP_SUMO:
	case CHIP_SUMO2:
	case CHIP_CAICOS:
		break;
	default:
		tmp |= S_008C00_VC_ENABLE(1);
		break;
	}
	tmp |= S_008C00_EXPORT_SRC_C(1);
	tmp |= S_008C00_CS_PRIO(cs_prio);
	tmp |= S_008C00_LS_PRIO(ls_prio);
	tmp |= S_008C00_HS_PRIO(hs_prio);
	tmp |= S_008C00_PS_PRIO(ps_prio);
	tmp |= S_008C00_VS_PRIO(vs_prio);
	tmp |= S_008C00_GS_PRIO(gs_prio);
	tmp |= S_008C00_ES_PRIO(es_prio);

	/* enable dynamic GPR resource management */
	if (ctx_drm_minor >= 7) {
		r600_store_config_reg_seq(cb, R_008C00_SQ_CONFIG, 2);
		r600_store_value(cb, tmp); /* R_008C00_SQ_CONFIG */
		/* always set temp clauses */
		r600_store_value(cb, S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs)); /* R_008C04_SQ_GPR_RESOURCE_MGMT_1 */
		r600_store_config_reg_seq(cb, R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1, 2);
		r600_store_value(cb, 0); /* R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1 */
		r600_store_value(cb, 0); /* R_008C14_SQ_GLOBAL_GPR_RESOURCE_MGMT_2 */
		r600_store_config_reg(cb, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, (1 << 8));
		r600_store_context_reg(cb, R_028838_SQ_DYN_GPR_RESOURCE_LIMIT_1,
					S_028838_PS_GPRS(0x1e) |
					S_028838_VS_GPRS(0x1e) |
					S_028838_GS_GPRS(0x1e) |
					S_028838_ES_GPRS(0x1e) |
					S_028838_HS_GPRS(0x1e) |
					S_028838_LS_GPRS(0x1e)); /* workaround for hw issues with dyn gpr - must set all limits to 240 instead of 0, 0x1e == 240 / 8*/
	} else {
		r600_store_config_reg_seq(cb, R_008C00_SQ_CONFIG, 4);
		r600_store_value(cb, tmp); /* R_008C00_SQ_CONFIG */

		tmp = S_008C04_NUM_PS_GPRS(num_ps_gprs);
		tmp |= S_008C04_NUM_VS_GPRS(num_vs_gprs);
		tmp |= S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs);
		r600_store_value(cb, tmp); /* R_008C04_SQ_GPR_RESOURCE_MGMT_1 */

		tmp = S_008C08_NUM_GS_GPRS(num_gs_gprs);
		tmp |= S_008C08_NUM_ES_GPRS(num_es_gprs);
		r600_store_value(cb, tmp); /* R_008C08_SQ_GPR_RESOURCE_MGMT_2 */

		tmp = S_008C0C_NUM_HS_GPRS(num_hs_gprs);
		tmp |= S_008C0C_NUM_HS_GPRS(num_ls_gprs);
		r600_store_value(cb, tmp); /* R_008C0C_SQ_GPR_RESOURCE_MGMT_3 */
	}

	r600_store_config_reg(cb, R_008E2C_SQ_LDS_RESOURCE_MGMT,
			      S_008E2C_NUM_PS_LDS(0x1000) | S_008E2C_NUM_LS_LDS(0x1000));

	r600_store_context_reg(cb, R_028A4C_PA_SC_MODE_CNTL_1, 0);

	/* The cs checker requires this register to be set. */
	r600_store_context_reg(cb, R_028800_DB_DEPTH_CONTROL, 0);

	r600_store_context_reg(cb, R_028354_SX_SURFACE_SYNC, S_028354_SURFACE_SYNC_MASK(0xf));

	return;
}

void evergreen_init_atom_start_cs(struct r600_context *rctx)
{
	struct r600_command_buffer *cb = &rctx->start_cs_cmd;
	int num_ps_threads;
	int num_vs_threads;
	int num_gs_threads;
	int num_es_threads;
	int num_hs_threads;
	int num_ls_threads;

	int num_ps_stack_entries;
	int num_vs_stack_entries;
	int num_gs_stack_entries;
	int num_es_stack_entries;
	int num_hs_stack_entries;
	int num_ls_stack_entries;
	enum radeon_family family;
	unsigned tmp;

	if (rctx->chip_class == CAYMAN) {
		cayman_init_atom_start_cs(rctx);
		return;
	}

	r600_init_command_buffer(cb, 256);

	/* This must be first. */
	r600_store_value(cb, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
	r600_store_value(cb, 0x80000000);
	r600_store_value(cb, 0x80000000);

	/* We're setting config registers here. */
	r600_store_value(cb, PKT3(PKT3_EVENT_WRITE, 0, 0));
	r600_store_value(cb, EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4));

	evergreen_init_common_regs(cb, rctx->chip_class,
				   rctx->family, rctx->screen->info.drm_minor);

	family = rctx->family;
	switch (family) {
	case CHIP_CEDAR:
	default:
		num_ps_threads = 96;
		num_vs_threads = 16;
		num_gs_threads = 16;
		num_es_threads = 16;
		num_hs_threads = 16;
		num_ls_threads = 16;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_REDWOOD:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_JUNIPER:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_PALM:
		num_ps_threads = 96;
		num_vs_threads = 16;
		num_gs_threads = 16;
		num_es_threads = 16;
		num_hs_threads = 16;
		num_ls_threads = 16;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_SUMO:
		num_ps_threads = 96;
		num_vs_threads = 25;
		num_gs_threads = 25;
		num_es_threads = 25;
		num_hs_threads = 25;
		num_ls_threads = 25;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_SUMO2:
		num_ps_threads = 96;
		num_vs_threads = 25;
		num_gs_threads = 25;
		num_es_threads = 25;
		num_hs_threads = 25;
		num_ls_threads = 25;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_BARTS:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 85;
		num_vs_stack_entries = 85;
		num_gs_stack_entries = 85;
		num_es_stack_entries = 85;
		num_hs_stack_entries = 85;
		num_ls_stack_entries = 85;
		break;
	case CHIP_TURKS:
		num_ps_threads = 128;
		num_vs_threads = 20;
		num_gs_threads = 20;
		num_es_threads = 20;
		num_hs_threads = 20;
		num_ls_threads = 20;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	case CHIP_CAICOS:
		num_ps_threads = 128;
		num_vs_threads = 10;
		num_gs_threads = 10;
		num_es_threads = 10;
		num_hs_threads = 10;
		num_ls_threads = 10;
		num_ps_stack_entries = 42;
		num_vs_stack_entries = 42;
		num_gs_stack_entries = 42;
		num_es_stack_entries = 42;
		num_hs_stack_entries = 42;
		num_ls_stack_entries = 42;
		break;
	}

	tmp = S_008C18_NUM_PS_THREADS(num_ps_threads);
	tmp |= S_008C18_NUM_VS_THREADS(num_vs_threads);
	tmp |= S_008C18_NUM_GS_THREADS(num_gs_threads);
	tmp |= S_008C18_NUM_ES_THREADS(num_es_threads);

	r600_store_config_reg_seq(cb, R_008C18_SQ_THREAD_RESOURCE_MGMT_1, 5);
	r600_store_value(cb, tmp); /* R_008C18_SQ_THREAD_RESOURCE_MGMT_1 */

	tmp = S_008C1C_NUM_HS_THREADS(num_hs_threads);
	tmp |= S_008C1C_NUM_LS_THREADS(num_ls_threads);
	r600_store_value(cb, tmp); /* R_008C1C_SQ_THREAD_RESOURCE_MGMT_2 */

	tmp = S_008C20_NUM_PS_STACK_ENTRIES(num_ps_stack_entries);
	tmp |= S_008C20_NUM_VS_STACK_ENTRIES(num_vs_stack_entries);
	r600_store_value(cb, tmp); /* R_008C20_SQ_STACK_RESOURCE_MGMT_1 */

	tmp = S_008C24_NUM_GS_STACK_ENTRIES(num_gs_stack_entries);
	tmp |= S_008C24_NUM_ES_STACK_ENTRIES(num_es_stack_entries);
	r600_store_value(cb, tmp); /* R_008C24_SQ_STACK_RESOURCE_MGMT_2 */

	tmp = S_008C28_NUM_HS_STACK_ENTRIES(num_hs_stack_entries);
	tmp |= S_008C28_NUM_LS_STACK_ENTRIES(num_ls_stack_entries);
	r600_store_value(cb, tmp); /* R_008C28_SQ_STACK_RESOURCE_MGMT_3 */

	r600_store_config_reg(cb, R_009100_SPI_CONFIG_CNTL, 0);
	r600_store_config_reg(cb, R_00913C_SPI_CONFIG_CNTL_1, S_00913C_VTX_DONE_DELAY(4));

	r600_store_context_reg_seq(cb, R_028900_SQ_ESGS_RING_ITEMSIZE, 6);
	r600_store_value(cb, 0); /* R_028900_SQ_ESGS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028904_SQ_GSVS_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028908_SQ_ESTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_02890C_SQ_GSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028910_SQ_VSTMP_RING_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028914_SQ_PSTMP_RING_ITEMSIZE */

	r600_store_context_reg_seq(cb, R_02891C_SQ_GS_VERT_ITEMSIZE, 4);
	r600_store_value(cb, 0); /* R_02891C_SQ_GS_VERT_ITEMSIZE */
	r600_store_value(cb, 0); /* R_028920_SQ_GS_VERT_ITEMSIZE_1 */
	r600_store_value(cb, 0); /* R_028924_SQ_GS_VERT_ITEMSIZE_2 */
	r600_store_value(cb, 0); /* R_028928_SQ_GS_VERT_ITEMSIZE_3 */

	r600_store_context_reg_seq(cb, R_028A10_VGT_OUTPUT_PATH_CNTL, 13);
	r600_store_value(cb, 0); /* R_028A10_VGT_OUTPUT_PATH_CNTL */
	r600_store_value(cb, 0); /* R_028A14_VGT_HOS_CNTL */
	r600_store_value(cb, 0); /* R_028A18_VGT_HOS_MAX_TESS_LEVEL */
	r600_store_value(cb, 0); /* R_028A1C_VGT_HOS_MIN_TESS_LEVEL */
	r600_store_value(cb, 0); /* R_028A20_VGT_HOS_REUSE_DEPTH */
	r600_store_value(cb, 0); /* R_028A24_VGT_GROUP_PRIM_TYPE */
	r600_store_value(cb, 0); /* R_028A28_VGT_GROUP_FIRST_DECR */
	r600_store_value(cb, 0); /* R_028A2C_VGT_GROUP_DECR */
	r600_store_value(cb, 0); /* R_028A30_VGT_GROUP_VECT_0_CNTL */
	r600_store_value(cb, 0); /* R_028A34_VGT_GROUP_VECT_1_CNTL */
	r600_store_value(cb, 0); /* R_028A38_VGT_GROUP_VECT_0_FMT_CNTL */
	r600_store_value(cb, 0); /* R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL */
	r600_store_value(cb, 0); /* R_028A40_VGT_GS_MODE */

	r600_store_context_reg_seq(cb, R_028AB4_VGT_REUSE_OFF, 2);
	r600_store_value(cb, 0); /* R_028AB4_VGT_REUSE_OFF */
	r600_store_value(cb, 0); /* R_028AB8_VGT_VTX_CNT_EN */

	r600_store_config_reg(cb, R_008A14_PA_CL_ENHANCE, (3 << 1) | 1);

        r600_store_context_reg(cb, R_0288F0_SQ_VTX_SEMANTIC_CLEAR, ~0);

        r600_store_context_reg_seq(cb, R_028400_VGT_MAX_VTX_INDX, 2);
	r600_store_value(cb, ~0); /* R_028400_VGT_MAX_VTX_INDX */
	r600_store_value(cb, 0); /* R_028404_VGT_MIN_VTX_INDX */

	r600_store_ctl_const(cb, R_03CFF0_SQ_VTX_BASE_VTX_LOC, 0);

	r600_store_context_reg(cb, R_028028_DB_STENCIL_CLEAR, 0);

	r600_store_context_reg(cb, R_028200_PA_SC_WINDOW_OFFSET, 0);
	r600_store_context_reg(cb, R_02820C_PA_SC_CLIPRECT_RULE, 0xFFFF);
	r600_store_context_reg(cb, R_028230_PA_SC_EDGERULE, 0xAAAAAAAA);

	r600_store_context_reg_seq(cb, R_0282D0_PA_SC_VPORT_ZMIN_0, 2);
	r600_store_value(cb, 0); /* R_0282D0_PA_SC_VPORT_ZMIN_0 */
	r600_store_value(cb, 0x3F800000); /* R_0282D4_PA_SC_VPORT_ZMAX_0 */

	r600_store_context_reg(cb, R_0286DC_SPI_FOG_CNTL, 0);
	r600_store_context_reg(cb, R_028818_PA_CL_VTE_CNTL, 0x0000043F);
	r600_store_context_reg(cb, R_028820_PA_CL_NANINF_CNTL, 0);

	r600_store_context_reg_seq(cb, R_028AC0_DB_SRESULTS_COMPARE_STATE0, 3);
	r600_store_value(cb, 0); /* R_028AC0_DB_SRESULTS_COMPARE_STATE0 */
	r600_store_value(cb, 0); /* R_028AC4_DB_SRESULTS_COMPARE_STATE1 */
	r600_store_value(cb, 0); /* R_028AC8_DB_PRELOAD_CONTROL */

	r600_store_context_reg_seq(cb, R_028C0C_PA_CL_GB_VERT_CLIP_ADJ, 4);
	r600_store_value(cb, 0x3F800000); /* R_028C0C_PA_CL_GB_VERT_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C10_PA_CL_GB_VERT_DISC_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C14_PA_CL_GB_HORZ_CLIP_ADJ */
	r600_store_value(cb, 0x3F800000); /* R_028C18_PA_CL_GB_HORZ_DISC_ADJ */

	r600_store_context_reg_seq(cb, R_028240_PA_SC_GENERIC_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028240_PA_SC_GENERIC_SCISSOR_TL */
	r600_store_value(cb, S_028244_BR_X(16384) | S_028244_BR_Y(16384)); /* R_028244_PA_SC_GENERIC_SCISSOR_BR */

	r600_store_context_reg_seq(cb, R_028030_PA_SC_SCREEN_SCISSOR_TL, 2);
	r600_store_value(cb, 0); /* R_028030_PA_SC_SCREEN_SCISSOR_TL */
	r600_store_value(cb, S_028034_BR_X(16384) | S_028034_BR_Y(16384)); /* R_028034_PA_SC_SCREEN_SCISSOR_BR */

	r600_store_context_reg(cb, R_028848_SQ_PGM_RESOURCES_2_PS, S_028848_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));
	r600_store_context_reg(cb, R_028864_SQ_PGM_RESOURCES_2_VS, S_028864_SINGLE_ROUND(V_SQ_ROUND_NEAREST_EVEN));
	r600_store_context_reg(cb, R_0288A8_SQ_PGM_RESOURCES_FS, 0);

	/* to avoid GPU doing any preloading of constant from random address */
	r600_store_context_reg_seq(cb, R_028140_ALU_CONST_BUFFER_SIZE_PS_0, 16);
	r600_store_value(cb, 0); /* R_028140_ALU_CONST_BUFFER_SIZE_PS_0 */
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);

	r600_store_context_reg_seq(cb, R_028180_ALU_CONST_BUFFER_SIZE_VS_0, 16);
	r600_store_value(cb, 0); /* R_028180_ALU_CONST_BUFFER_SIZE_VS_0 */
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);
	r600_store_value(cb, 0);

	r600_store_context_reg_seq(cb, R_028B94_VGT_STRMOUT_CONFIG, 2);
	r600_store_value(cb, 0); /* R_028B94_VGT_STRMOUT_CONFIG */
	r600_store_value(cb, 0); /* R_028B98_VGT_STRMOUT_BUFFER_CONFIG */

	if (rctx->screen->has_streamout) {
		r600_store_context_reg(cb, R_028B28_VGT_STRMOUT_DRAW_OPAQUE_OFFSET, 0);
	}

	r600_store_context_reg(cb, R_028010_DB_RENDER_OVERRIDE2, 0);
	r600_store_context_reg(cb, R_028234_PA_SU_HARDWARE_SCREEN_OFFSET, 0);
	r600_store_context_reg(cb, R_0286C8_SPI_THREAD_GROUPING, 0);
	r600_store_context_reg_seq(cb, R_0286E4_SPI_PS_IN_CONTROL_2, 2);
	r600_store_value(cb, 0); /* R_0286E4_SPI_PS_IN_CONTROL_2 */
	r600_store_value(cb, 0); /* R_0286E8_SPI_COMPUTE_INPUT_CNTL */
	r600_store_context_reg(cb, R_0288EC_SQ_LDS_ALLOC_PS, 0);
	r600_store_context_reg(cb, R_028B54_VGT_SHADER_STAGES_EN, 0);

	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0, 0x01000FFF);
	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0 + (32 * 4), 0x01000FFF);
}

void evergreen_update_ps_state(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_command_buffer *cb = &shader->command_buffer;
	struct r600_shader *rshader = &shader->shader;
	unsigned i, exports_ps, num_cout, spi_ps_in_control_0, spi_input_z, spi_ps_in_control_1, db_shader_control = 0;
	int pos_index = -1, face_index = -1;
	int ninterp = 0;
	boolean have_linear = FALSE, have_centroid = FALSE, have_perspective = FALSE;
	unsigned spi_baryc_cntl, sid, tmp, num = 0;
	unsigned z_export = 0, stencil_export = 0;
	unsigned sprite_coord_enable = rctx->rasterizer ? rctx->rasterizer->sprite_coord_enable : 0;
	uint32_t spi_ps_input_cntl[32];

	if (!cb->buf) {
		r600_init_command_buffer(cb, 64);
	} else {
		cb->num_dw = 0;
	}

	for (i = 0; i < rshader->ninput; i++) {
		/* evergreen NUM_INTERP only contains values interpolated into the LDS,
		   POSITION goes via GPRs from the SC so isn't counted */
		if (rshader->input[i].name == TGSI_SEMANTIC_POSITION)
			pos_index = i;
		else if (rshader->input[i].name == TGSI_SEMANTIC_FACE)
			face_index = i;
		else {
			ninterp++;
			if (rshader->input[i].interpolate == TGSI_INTERPOLATE_LINEAR)
				have_linear = TRUE;
			if (rshader->input[i].interpolate == TGSI_INTERPOLATE_PERSPECTIVE)
				have_perspective = TRUE;
			if (rshader->input[i].centroid)
				have_centroid = TRUE;
		}

		sid = rshader->input[i].spi_sid;

		if (sid) {
			tmp = S_028644_SEMANTIC(sid);

			if (rshader->input[i].name == TGSI_SEMANTIC_POSITION ||
				rshader->input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
				(rshader->input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
					rctx->rasterizer && rctx->rasterizer->flatshade)) {
				tmp |= S_028644_FLAT_SHADE(1);
			}

			if (rshader->input[i].name == TGSI_SEMANTIC_GENERIC &&
			    (sprite_coord_enable & (1 << rshader->input[i].sid))) {
				tmp |= S_028644_PT_SPRITE_TEX(1);
			}

			spi_ps_input_cntl[num++] = tmp;
		}
	}

	r600_store_context_reg_seq(cb, R_028644_SPI_PS_INPUT_CNTL_0, num);
	r600_store_array(cb, num, spi_ps_input_cntl);

	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION)
			z_export = 1;
		if (rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			stencil_export = 1;
	}
	if (rshader->uses_kill)
		db_shader_control |= S_02880C_KILL_ENABLE(1);

	db_shader_control |= S_02880C_Z_EXPORT_ENABLE(z_export);
	db_shader_control |= S_02880C_STENCIL_EXPORT_ENABLE(stencil_export);

	exports_ps = 0;
	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].name == TGSI_SEMANTIC_POSITION ||
		    rshader->output[i].name == TGSI_SEMANTIC_STENCIL)
			exports_ps |= 1;
	}

	num_cout = rshader->nr_ps_color_exports;

	exports_ps |= S_02884C_EXPORT_COLORS(num_cout);
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}
	shader->nr_ps_color_outputs = num_cout;
	if (ninterp == 0) {
		ninterp = 1;
		have_perspective = TRUE;
	}

	if (!have_perspective && !have_linear)
		have_perspective = TRUE;

	spi_ps_in_control_0 = S_0286CC_NUM_INTERP(ninterp) |
		              S_0286CC_PERSP_GRADIENT_ENA(have_perspective) |
		              S_0286CC_LINEAR_GRADIENT_ENA(have_linear);
	spi_input_z = 0;
	if (pos_index != -1) {
		spi_ps_in_control_0 |=  S_0286CC_POSITION_ENA(1) |
			S_0286CC_POSITION_CENTROID(rshader->input[pos_index].centroid) |
			S_0286CC_POSITION_ADDR(rshader->input[pos_index].gpr);
		spi_input_z |= S_0286D8_PROVIDE_Z_TO_SPI(1);
	}

	spi_ps_in_control_1 = 0;
	if (face_index != -1) {
		spi_ps_in_control_1 |= S_0286D0_FRONT_FACE_ENA(1) |
			S_0286D0_FRONT_FACE_ADDR(rshader->input[face_index].gpr);
	}

	spi_baryc_cntl = 0;
	if (have_perspective)
		spi_baryc_cntl |= S_0286E0_PERSP_CENTER_ENA(1) |
				  S_0286E0_PERSP_CENTROID_ENA(have_centroid);
	if (have_linear)
		spi_baryc_cntl |= S_0286E0_LINEAR_CENTER_ENA(1) |
				  S_0286E0_LINEAR_CENTROID_ENA(have_centroid);

	r600_store_context_reg_seq(cb, R_0286CC_SPI_PS_IN_CONTROL_0, 2);
	r600_store_value(cb, spi_ps_in_control_0); /* R_0286CC_SPI_PS_IN_CONTROL_0 */
	r600_store_value(cb, spi_ps_in_control_1); /* R_0286D0_SPI_PS_IN_CONTROL_1 */

	r600_store_context_reg(cb, R_0286E0_SPI_BARYC_CNTL, spi_baryc_cntl);
	r600_store_context_reg(cb, R_0286D8_SPI_INPUT_Z, spi_input_z);
	r600_store_context_reg(cb, R_02884C_SQ_PGM_EXPORTS_PS, exports_ps);

	r600_store_context_reg_seq(cb, R_028840_SQ_PGM_START_PS, 2);
	r600_store_value(cb, r600_resource_va(ctx->screen, (void *)shader->bo) >> 8);
	r600_store_value(cb, /* R_028844_SQ_PGM_RESOURCES_PS */
			 S_028844_NUM_GPRS(rshader->bc.ngpr) |
			 S_028844_PRIME_CACHE_ON_DRAW(1) |
			 S_028844_STACK_SIZE(rshader->bc.nstack));
	/* After that, the NOP relocation packet must be emitted (shader->bo, RADEON_USAGE_READ). */

	shader->db_shader_control = db_shader_control;
	shader->ps_depth_export = z_export | stencil_export;

	shader->sprite_coord_enable = sprite_coord_enable;
	if (rctx->rasterizer)
		shader->flatshade = rctx->rasterizer->flatshade;
}

void evergreen_update_vs_state(struct pipe_context *ctx, struct r600_pipe_shader *shader)
{
	struct r600_command_buffer *cb = &shader->command_buffer;
	struct r600_shader *rshader = &shader->shader;
	unsigned spi_vs_out_id[10] = {};
	unsigned i, tmp, nparams = 0;

	for (i = 0; i < rshader->noutput; i++) {
		if (rshader->output[i].spi_sid) {
			tmp = rshader->output[i].spi_sid << ((nparams & 3) * 8);
			spi_vs_out_id[nparams / 4] |= tmp;
			nparams++;
		}
	}

	r600_init_command_buffer(cb, 32);

	r600_store_context_reg_seq(cb, R_02861C_SPI_VS_OUT_ID_0, 10);
	for (i = 0; i < 10; i++) {
		r600_store_value(cb, spi_vs_out_id[i]);
	}

	/* Certain attributes (position, psize, etc.) don't count as params.
	 * VS is required to export at least one param and r600_shader_from_tgsi()
	 * takes care of adding a dummy export.
	 */
	if (nparams < 1)
		nparams = 1;

	r600_store_context_reg(cb, R_0286C4_SPI_VS_OUT_CONFIG,
			       S_0286C4_VS_EXPORT_COUNT(nparams - 1));
	r600_store_context_reg(cb, R_028860_SQ_PGM_RESOURCES_VS,
			       S_028860_NUM_GPRS(rshader->bc.ngpr) |
			       S_028860_STACK_SIZE(rshader->bc.nstack));
	r600_store_context_reg(cb, R_02885C_SQ_PGM_START_VS,
			       r600_resource_va(ctx->screen, (void *)shader->bo) >> 8);
	/* After that, the NOP relocation packet must be emitted (shader->bo, RADEON_USAGE_READ). */

	shader->pa_cl_vs_out_cntl =
		S_02881C_VS_OUT_CCDIST0_VEC_ENA((rshader->clip_dist_write & 0x0F) != 0) |
		S_02881C_VS_OUT_CCDIST1_VEC_ENA((rshader->clip_dist_write & 0xF0) != 0) |
		S_02881C_VS_OUT_MISC_VEC_ENA(rshader->vs_out_misc_write) |
		S_02881C_USE_VTX_POINT_SIZE(rshader->vs_out_point_size);
}

void *evergreen_create_resolve_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	blend.rt[0].colormask = 0xf;
	return evergreen_create_blend_state_mode(&rctx->context, &blend, V_028808_CB_RESOLVE);
}

void *evergreen_create_decompress_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	blend.rt[0].colormask = 0xf;
	return evergreen_create_blend_state_mode(&rctx->context, &blend, V_028808_CB_DECOMPRESS);
}

void *evergreen_create_fmask_decompress_blend(struct r600_context *rctx)
{
	struct pipe_blend_state blend;

	memset(&blend, 0, sizeof(blend));
	blend.independent_blend_enable = true;
	blend.rt[0].colormask = 0xf;
	return evergreen_create_blend_state_mode(&rctx->context, &blend, V_028808_CB_FMASK_DECOMPRESS);
}

void *evergreen_create_db_flush_dsa(struct r600_context *rctx)
{
	struct pipe_depth_stencil_alpha_state dsa = {{0}};

	return rctx->context.create_depth_stencil_alpha_state(&rctx->context, &dsa);
}

void evergreen_update_db_shader_control(struct r600_context * rctx)
{
	bool dual_export = rctx->framebuffer.export_16bpc &&
			   !rctx->ps_shader->current->ps_depth_export;

	unsigned db_shader_control = rctx->ps_shader->current->db_shader_control |
			S_02880C_DUAL_EXPORT_ENABLE(dual_export) |
			S_02880C_DB_SOURCE_FORMAT(dual_export ? V_02880C_EXPORT_DB_TWO :
								V_02880C_EXPORT_DB_FULL) |
			S_02880C_ALPHA_TO_MASK_DISABLE(rctx->framebuffer.cb0_is_integer);

	/* When alpha test is enabled we can't trust the hw to make the proper
	 * decision on the order in which ztest should be run related to fragment
	 * shader execution.
	 *
	 * If alpha test is enabled perform early z rejection (RE_Z) but don't early
	 * write to the zbuffer. Write to zbuffer is delayed after fragment shader
	 * execution and thus after alpha test so if discarded by the alpha test
	 * the z value is not written.
	 * If ReZ is enabled, and the zfunc/zenable/zwrite values change you can
	 * get a hang unless you flush the DB in between.  For now just use
	 * LATE_Z.
	 */
	if (rctx->alphatest_state.sx_alpha_test_control) {
		db_shader_control |= S_02880C_Z_ORDER(V_02880C_LATE_Z);
	} else {
		db_shader_control |= S_02880C_Z_ORDER(V_02880C_EARLY_Z_THEN_LATE_Z);
	}

	if (db_shader_control != rctx->db_misc_state.db_shader_control) {
		rctx->db_misc_state.db_shader_control = db_shader_control;
		rctx->db_misc_state.atom.dirty = true;
	}
}

static void evergreen_dma_copy_tile(struct r600_context *rctx,
				struct pipe_resource *dst,
				unsigned dst_level,
				unsigned dst_x,
				unsigned dst_y,
				unsigned dst_z,
				struct pipe_resource *src,
				unsigned src_level,
				unsigned src_x,
				unsigned src_y,
				unsigned src_z,
				unsigned copy_height,
				unsigned pitch,
				unsigned bpp)
{
	struct radeon_winsys_cs *cs = rctx->rings.dma.cs;
	struct r600_texture *rsrc = (struct r600_texture*)src;
	struct r600_texture *rdst = (struct r600_texture*)dst;
	unsigned array_mode, lbpp, pitch_tile_max, slice_tile_max, size;
	unsigned ncopy, height, cheight, detile, i, x, y, z, src_mode, dst_mode;
	unsigned sub_cmd, bank_h, bank_w, mt_aspect, nbanks, tile_split, non_disp_tiling = 0;
	uint64_t base, addr;

	/* make sure that the dma ring is only one active */
	rctx->rings.gfx.flush(rctx, RADEON_FLUSH_ASYNC);

	dst_mode = rdst->surface.level[dst_level].mode;
	src_mode = rsrc->surface.level[src_level].mode;
	/* downcast linear aligned to linear to simplify test */
	src_mode = src_mode == RADEON_SURF_MODE_LINEAR_ALIGNED ? RADEON_SURF_MODE_LINEAR : src_mode;
	dst_mode = dst_mode == RADEON_SURF_MODE_LINEAR_ALIGNED ? RADEON_SURF_MODE_LINEAR : dst_mode;
	assert(dst_mode != src_mode);

	/* non_disp_tiling bit needs to be set for depth, stencil, and fmask surfaces */
	if (util_format_has_depth(util_format_description(src->format)))
		non_disp_tiling = 1;

	y = 0;
	sub_cmd = 0x8;
	lbpp = util_logbase2(bpp);
	pitch_tile_max = ((pitch / bpp) >> 3) - 1;
	nbanks = eg_num_banks(rctx->screen->tiling_info.num_banks);

	if (dst_mode == RADEON_SURF_MODE_LINEAR) {
		/* T2L */
		array_mode = evergreen_array_mode(src_mode);
		slice_tile_max = (rsrc->surface.level[src_level].nblk_x * rsrc->surface.level[src_level].nblk_y) >> 6;
		slice_tile_max = slice_tile_max ? slice_tile_max - 1 : 0;
		/* linear height must be the same as the slice tile max height, it's ok even
		 * if the linear destination/source have smaller heigh as the size of the
		 * dma packet will be using the copy_height which is always smaller or equal
		 * to the linear height
		 */
		height = rsrc->surface.level[src_level].npix_y;
		detile = 1;
		x = src_x;
		y = src_y;
		z = src_z;
		base = rsrc->surface.level[src_level].offset;
		addr = rdst->surface.level[dst_level].offset;
		addr += rdst->surface.level[dst_level].slice_size * dst_z;
		addr += dst_y * pitch + dst_x * bpp;
		bank_h = eg_bank_wh(rsrc->surface.bankh);
		bank_w = eg_bank_wh(rsrc->surface.bankw);
		mt_aspect = eg_macro_tile_aspect(rsrc->surface.mtilea);
		tile_split = eg_tile_split(rsrc->surface.tile_split);
		base += r600_resource_va(&rctx->screen->screen, src);
		addr += r600_resource_va(&rctx->screen->screen, dst);
	} else {
		/* L2T */
		array_mode = evergreen_array_mode(dst_mode);
		slice_tile_max = (rdst->surface.level[dst_level].nblk_x * rdst->surface.level[dst_level].nblk_y) >> 6;
		slice_tile_max = slice_tile_max ? slice_tile_max - 1 : 0;
		/* linear height must be the same as the slice tile max height, it's ok even
		 * if the linear destination/source have smaller heigh as the size of the
		 * dma packet will be using the copy_height which is always smaller or equal
		 * to the linear height
		 */
		height = rdst->surface.level[dst_level].npix_y;
		detile = 0;
		x = dst_x;
		y = dst_y;
		z = dst_z;
		base = rdst->surface.level[dst_level].offset;
		addr = rsrc->surface.level[src_level].offset;
		addr += rsrc->surface.level[src_level].slice_size * src_z;
		addr += src_y * pitch + src_x * bpp;
		bank_h = eg_bank_wh(rdst->surface.bankh);
		bank_w = eg_bank_wh(rdst->surface.bankw);
		mt_aspect = eg_macro_tile_aspect(rdst->surface.mtilea);
		tile_split = eg_tile_split(rdst->surface.tile_split);
		base += r600_resource_va(&rctx->screen->screen, dst);
		addr += r600_resource_va(&rctx->screen->screen, src);
	}

	size = (copy_height * pitch) >> 2;
	ncopy = (size / 0x000fffff) + !!(size % 0x000fffff);
	r600_need_dma_space(rctx, ncopy * 9);

	for (i = 0; i < ncopy; i++) {
		cheight = copy_height;
		if (((cheight * pitch) >> 2) > 0x000fffff) {
			cheight = (0x000fffff << 2) / pitch;
		}
		size = (cheight * pitch) >> 2;
		/* emit reloc before writting cs so that cs is always in consistent state */
		r600_context_bo_reloc(rctx, &rctx->rings.dma, &rsrc->resource, RADEON_USAGE_READ);
		r600_context_bo_reloc(rctx, &rctx->rings.dma, &rdst->resource, RADEON_USAGE_WRITE);
		cs->buf[cs->cdw++] = DMA_PACKET(DMA_PACKET_COPY, sub_cmd, size);
		cs->buf[cs->cdw++] = base >> 8;
		cs->buf[cs->cdw++] = (detile << 31) | (array_mode << 27) |
					(lbpp << 24) | (bank_h << 21) |
					(bank_w << 18) | (mt_aspect << 16);
		cs->buf[cs->cdw++] = (pitch_tile_max << 0) | ((height - 1) << 16);
		cs->buf[cs->cdw++] = (slice_tile_max << 0);
		cs->buf[cs->cdw++] = (x << 0) | (z << 18);
		cs->buf[cs->cdw++] = (y << 0) | (tile_split << 21) | (nbanks << 25) | (non_disp_tiling << 28);
		cs->buf[cs->cdw++] = addr & 0xfffffffc;
		cs->buf[cs->cdw++] = (addr >> 32UL) & 0xff;
		copy_height -= cheight;
		addr += cheight * pitch;
		y += cheight;
	}
}

boolean evergreen_dma_blit(struct pipe_context *ctx,
			struct pipe_resource *dst,
			unsigned dst_level,
			unsigned dst_x, unsigned dst_y, unsigned dst_z,
			struct pipe_resource *src,
			unsigned src_level,
			const struct pipe_box *src_box)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct r600_texture *rsrc = (struct r600_texture*)src;
	struct r600_texture *rdst = (struct r600_texture*)dst;
	unsigned dst_pitch, src_pitch, bpp, dst_mode, src_mode, copy_height;
	unsigned src_w, dst_w;

	if (rctx->rings.dma.cs == NULL) {
		return FALSE;
	}
	if (src->format != dst->format) {
		return FALSE;
	}

	bpp = rdst->surface.bpe;
	dst_pitch = rdst->surface.level[dst_level].pitch_bytes;
	src_pitch = rsrc->surface.level[src_level].pitch_bytes;
	src_w = rsrc->surface.level[src_level].npix_x;
	dst_w = rdst->surface.level[dst_level].npix_x;
	copy_height = src_box->height / rsrc->surface.blk_h;

	dst_mode = rdst->surface.level[dst_level].mode;
	src_mode = rsrc->surface.level[src_level].mode;
	/* downcast linear aligned to linear to simplify test */
	src_mode = src_mode == RADEON_SURF_MODE_LINEAR_ALIGNED ? RADEON_SURF_MODE_LINEAR : src_mode;
	dst_mode = dst_mode == RADEON_SURF_MODE_LINEAR_ALIGNED ? RADEON_SURF_MODE_LINEAR : dst_mode;

	if (src_pitch != dst_pitch || src_box->x || dst_x || src_w != dst_w) {
		/* FIXME evergreen can do partial blit */
		return FALSE;
	}
	/* the x test here are currently useless (because we don't support partial blit)
	 * but keep them around so we don't forget about those
	 */
	if ((src_pitch & 0x7) || (src_box->x & 0x7) || (dst_x & 0x7) || (src_box->y & 0x7) || (dst_y & 0x7)) {
		return FALSE;
	}

	/* 128 bpp surfaces require non_disp_tiling for both
	 * tiled and linear buffers on cayman.  However, async
	 * DMA only supports it on the tiled side.  As such
	 * the tile order is backwards after a L2T/T2L packet.
	 */
	if ((rctx->chip_class == CAYMAN) &&
	    (src_mode != dst_mode) &&
	    (util_format_get_blocksize(src->format) >= 16)) {
		return FALSE;
	}

	if (src_mode == dst_mode) {
		uint64_t dst_offset, src_offset;
		/* simple dma blit would do NOTE code here assume :
		 *   src_box.x/y == 0
		 *   dst_x/y == 0
		 *   dst_pitch == src_pitch
		 */
		src_offset= rsrc->surface.level[src_level].offset;
		src_offset += rsrc->surface.level[src_level].slice_size * src_box->z;
		src_offset += src_box->y * src_pitch + src_box->x * bpp;
		dst_offset = rdst->surface.level[dst_level].offset;
		dst_offset += rdst->surface.level[dst_level].slice_size * dst_z;
		dst_offset += dst_y * dst_pitch + dst_x * bpp;
		evergreen_dma_copy(rctx, dst, src, dst_offset, src_offset,
					src_box->height * src_pitch);
	} else {
		evergreen_dma_copy_tile(rctx, dst, dst_level, dst_x, dst_y, dst_z,
					src, src_level, src_box->x, src_box->y, src_box->z,
					copy_height, dst_pitch, bpp);
	}
	return TRUE;
}

void evergreen_init_state_functions(struct r600_context *rctx)
{
	unsigned id = 4;

	/* !!!
	 *  To avoid GPU lockup registers must be emited in a specific order
	 * (no kidding ...). The order below is important and have been
	 * partialy infered from analyzing fglrx command stream.
	 *
	 * Don't reorder atom without carefully checking the effect (GPU lockup
	 * or piglit regression).
	 * !!!
	 */

	r600_init_atom(rctx, &rctx->framebuffer.atom, id++, evergreen_emit_framebuffer_state, 0);
	/* shader const */
	r600_init_atom(rctx, &rctx->constbuf_state[PIPE_SHADER_VERTEX].atom, id++, evergreen_emit_vs_constant_buffers, 0);
	r600_init_atom(rctx, &rctx->constbuf_state[PIPE_SHADER_GEOMETRY].atom, id++, evergreen_emit_gs_constant_buffers, 0);
	r600_init_atom(rctx, &rctx->constbuf_state[PIPE_SHADER_FRAGMENT].atom, id++, evergreen_emit_ps_constant_buffers, 0);
	/* shader program */
	r600_init_atom(rctx, &rctx->cs_shader_state.atom, id++, evergreen_emit_cs_shader, 0);
	/* sampler */
	r600_init_atom(rctx, &rctx->samplers[PIPE_SHADER_VERTEX].states.atom, id++, evergreen_emit_vs_sampler_states, 0);
	r600_init_atom(rctx, &rctx->samplers[PIPE_SHADER_GEOMETRY].states.atom, id++, evergreen_emit_gs_sampler_states, 0);
	r600_init_atom(rctx, &rctx->samplers[PIPE_SHADER_FRAGMENT].states.atom, id++, evergreen_emit_ps_sampler_states, 0);
	/* resources */
	r600_init_atom(rctx, &rctx->vertex_buffer_state.atom, id++, evergreen_fs_emit_vertex_buffers, 0);
	r600_init_atom(rctx, &rctx->cs_vertex_buffer_state.atom, id++, evergreen_cs_emit_vertex_buffers, 0);
	r600_init_atom(rctx, &rctx->samplers[PIPE_SHADER_VERTEX].views.atom, id++, evergreen_emit_vs_sampler_views, 0);
	r600_init_atom(rctx, &rctx->samplers[PIPE_SHADER_GEOMETRY].views.atom, id++, evergreen_emit_gs_sampler_views, 0);
	r600_init_atom(rctx, &rctx->samplers[PIPE_SHADER_FRAGMENT].views.atom, id++, evergreen_emit_ps_sampler_views, 0);

	r600_init_atom(rctx, &rctx->vgt_state.atom, id++, r600_emit_vgt_state, 7);

	if (rctx->chip_class == EVERGREEN) {
		r600_init_atom(rctx, &rctx->sample_mask.atom, id++, evergreen_emit_sample_mask, 3);
	} else {
		r600_init_atom(rctx, &rctx->sample_mask.atom, id++, cayman_emit_sample_mask, 4);
	}
	rctx->sample_mask.sample_mask = ~0;

	r600_init_atom(rctx, &rctx->alphatest_state.atom, id++, r600_emit_alphatest_state, 6);
	r600_init_atom(rctx, &rctx->blend_color.atom, id++, r600_emit_blend_color, 6);
	r600_init_atom(rctx, &rctx->blend_state.atom, id++, r600_emit_cso_state, 0);
	r600_init_atom(rctx, &rctx->cb_misc_state.atom, id++, evergreen_emit_cb_misc_state, 4);
	r600_init_atom(rctx, &rctx->clip_misc_state.atom, id++, r600_emit_clip_misc_state, 6);
	r600_init_atom(rctx, &rctx->clip_state.atom, id++, evergreen_emit_clip_state, 26);
	r600_init_atom(rctx, &rctx->db_misc_state.atom, id++, evergreen_emit_db_misc_state, 10);
	r600_init_atom(rctx, &rctx->db_state.atom, id++, evergreen_emit_db_state, 14);
	r600_init_atom(rctx, &rctx->dsa_state.atom, id++, r600_emit_cso_state, 0);
	r600_init_atom(rctx, &rctx->poly_offset_state.atom, id++, evergreen_emit_polygon_offset, 6);
	r600_init_atom(rctx, &rctx->rasterizer_state.atom, id++, r600_emit_cso_state, 0);
	r600_init_atom(rctx, &rctx->scissor.atom, id++, evergreen_emit_scissor_state, 4);
	r600_init_atom(rctx, &rctx->stencil_ref.atom, id++, r600_emit_stencil_ref, 4);
	r600_init_atom(rctx, &rctx->viewport.atom, id++, r600_emit_viewport_state, 8);
	r600_init_atom(rctx, &rctx->vertex_fetch_shader.atom, id++, evergreen_emit_vertex_fetch_shader, 5);
	r600_init_atom(rctx, &rctx->streamout.begin_atom, id++, r600_emit_streamout_begin, 0);
	r600_init_atom(rctx, &rctx->vertex_shader.atom, id++, r600_emit_shader, 23);
	r600_init_atom(rctx, &rctx->pixel_shader.atom, id++, r600_emit_shader, 0);

	rctx->context.create_blend_state = evergreen_create_blend_state;
	rctx->context.create_depth_stencil_alpha_state = evergreen_create_dsa_state;
	rctx->context.create_rasterizer_state = evergreen_create_rs_state;
	rctx->context.create_sampler_state = evergreen_create_sampler_state;
	rctx->context.create_sampler_view = evergreen_create_sampler_view;
	rctx->context.set_framebuffer_state = evergreen_set_framebuffer_state;
	rctx->context.set_polygon_stipple = evergreen_set_polygon_stipple;
	rctx->context.set_scissor_state = evergreen_set_scissor_state;
	evergreen_init_compute_state_functions(rctx);
}
