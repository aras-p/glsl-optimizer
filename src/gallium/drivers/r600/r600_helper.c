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
#include "r600_pipe.h"
#include "r600d.h"

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
