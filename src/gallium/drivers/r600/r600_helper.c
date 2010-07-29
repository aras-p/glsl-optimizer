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
 */
#include <stdio.h>
#include <errno.h>
#include <util/u_inlines.h>
#include "r600_screen.h"
#include "r600_context.h"
#include "r600d.h"

int r600_conv_pipe_format(unsigned pformat, unsigned *format)
{
	switch (pformat) {
	case PIPE_FORMAT_R32G32B32_FLOAT:
		*format = 0x30;
		return 0;
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		*format = V_0280A0_COLOR_32_32_32_32_FLOAT;
		return 0;
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_X8R8G8B8_UNORM:
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case PIPE_FORMAT_B8G8R8X8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R8G8B8X8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_USCALED:
	case PIPE_FORMAT_R8G8B8A8_SNORM:
	case PIPE_FORMAT_R8G8B8A8_SSCALED:
		*format = V_0280A0_COLOR_8_8_8_8;
		return 0;
	case PIPE_FORMAT_R32_FLOAT:
		*format = V_0280A0_COLOR_32_FLOAT;
		return 0;
	case PIPE_FORMAT_R32G32_FLOAT:
		*format = V_0280A0_COLOR_32_32_FLOAT;
		return 0;
	case PIPE_FORMAT_L8_UNORM:
	case PIPE_FORMAT_A8_UNORM:
	case PIPE_FORMAT_I8_UNORM:
		*format = V_0280A0_COLOR_8;
		return 0;
	case PIPE_FORMAT_L16_UNORM:
	case PIPE_FORMAT_Z16_UNORM:
	case PIPE_FORMAT_Z32_UNORM:
	case PIPE_FORMAT_Z32_FLOAT:
	case PIPE_FORMAT_R64_FLOAT:
	case PIPE_FORMAT_R64G64_FLOAT:
	case PIPE_FORMAT_R64G64B64_FLOAT:
	case PIPE_FORMAT_R64G64B64A64_FLOAT:
	case PIPE_FORMAT_R32_UNORM:
	case PIPE_FORMAT_R32G32_UNORM:
	case PIPE_FORMAT_R32G32B32_UNORM:
	case PIPE_FORMAT_R32G32B32A32_UNORM:
	case PIPE_FORMAT_R32_USCALED:
	case PIPE_FORMAT_R32G32_USCALED:
	case PIPE_FORMAT_R32G32B32_USCALED:
	case PIPE_FORMAT_R32G32B32A32_USCALED:
	case PIPE_FORMAT_R32_SNORM:
	case PIPE_FORMAT_R32G32_SNORM:
	case PIPE_FORMAT_R32G32B32_SNORM:
	case PIPE_FORMAT_R32G32B32A32_SNORM:
	case PIPE_FORMAT_R32_SSCALED:
	case PIPE_FORMAT_R32G32_SSCALED:
	case PIPE_FORMAT_R32G32B32_SSCALED:
	case PIPE_FORMAT_R32G32B32A32_SSCALED:
	case PIPE_FORMAT_R16_UNORM:
	case PIPE_FORMAT_R16G16_UNORM:
	case PIPE_FORMAT_R16G16B16_UNORM:
	case PIPE_FORMAT_R16G16B16A16_UNORM:
	case PIPE_FORMAT_R16_USCALED:
	case PIPE_FORMAT_R16G16_USCALED:
	case PIPE_FORMAT_R16G16B16_USCALED:
	case PIPE_FORMAT_R16G16B16A16_USCALED:
	case PIPE_FORMAT_R16_SNORM:
	case PIPE_FORMAT_R16G16_SNORM:
	case PIPE_FORMAT_R16G16B16_SNORM:
	case PIPE_FORMAT_R16G16B16A16_SNORM:
	case PIPE_FORMAT_R16_SSCALED:
	case PIPE_FORMAT_R16G16_SSCALED:
	case PIPE_FORMAT_R16G16B16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8B8_UNORM:
	case PIPE_FORMAT_R8_USCALED:
	case PIPE_FORMAT_R8G8_USCALED:
	case PIPE_FORMAT_R8G8B8_USCALED:
	case PIPE_FORMAT_R8_SNORM:
	case PIPE_FORMAT_R8G8_SNORM:
	case PIPE_FORMAT_R8G8B8_SNORM:
	case PIPE_FORMAT_R8_SSCALED:
	case PIPE_FORMAT_R8G8_SSCALED:
	case PIPE_FORMAT_R8G8B8_SSCALED:
	case PIPE_FORMAT_R32_FIXED:
	case PIPE_FORMAT_R32G32_FIXED:
	case PIPE_FORMAT_R32G32B32_FIXED:
	case PIPE_FORMAT_R32G32B32A32_FIXED:
	default:
		R600_ERR("unsupported %d\n", pformat);
		return -EINVAL;
	}
}

int r600_conv_pipe_prim(unsigned pprim, unsigned *prim)
{
	switch (pprim) {
	case PIPE_PRIM_POINTS:
		*prim = V_008958_DI_PT_POINTLIST;
		return 0;
	case PIPE_PRIM_LINES:
		*prim = V_008958_DI_PT_LINELIST;
		return 0;
	case PIPE_PRIM_LINE_STRIP:
		*prim = V_008958_DI_PT_LINESTRIP;
		return 0;
	case PIPE_PRIM_LINE_LOOP:
		*prim = V_008958_DI_PT_LINELOOP;
		return 0;
	case PIPE_PRIM_TRIANGLES:
		*prim = V_008958_DI_PT_TRILIST;
		return 0;
	case PIPE_PRIM_TRIANGLE_STRIP:
		*prim = V_008958_DI_PT_TRISTRIP;
		return 0;
	case PIPE_PRIM_TRIANGLE_FAN:
		*prim = V_008958_DI_PT_TRIFAN;
		return 0;
	case PIPE_PRIM_POLYGON:
		*prim = V_008958_DI_PT_POLYGON;
		return 0;
	case PIPE_PRIM_QUADS:
		*prim = V_008958_DI_PT_QUADLIST;
		return 0;
	case PIPE_PRIM_QUAD_STRIP:
		*prim = V_008958_DI_PT_QUADSTRIP;
		return 0;
	default:
		fprintf(stderr, "%s:%d unsupported %d\n", __func__, __LINE__, pprim);
		return -EINVAL;
	}
}
