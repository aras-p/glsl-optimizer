
/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "nv40_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_inlines.h"
#include "util/p_tile.h"

static boolean
nv40_surface_format_supported(struct pipe_context *pipe,
			      enum pipe_format format, uint type)
{
	switch (type) {
	case PIPE_SURFACE:
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM: 
		case PIPE_FORMAT_Z24S8_UNORM:
		case PIPE_FORMAT_Z16_UNORM:
			return TRUE;
		default:
			break;
		}
		break;
	case PIPE_TEXTURE:
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_A1R5G5B5_UNORM:
		case PIPE_FORMAT_A4R4G4B4_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM: 
		case PIPE_FORMAT_U_L8:
		case PIPE_FORMAT_U_A8:
		case PIPE_FORMAT_U_I8:
		case PIPE_FORMAT_U_A8_L8:
		case PIPE_FORMAT_Z16_UNORM:
		case PIPE_FORMAT_Z24S8_UNORM:
			return TRUE;
		default:
			break;
		}
		break;
	default:
		assert(0);
	};

	return FALSE;
}

static void
nv40_surface_copy(struct pipe_context *pipe, unsigned do_flip,
		  struct pipe_surface *dest, unsigned destx, unsigned desty,
		  struct pipe_surface *src, unsigned srcx, unsigned srcy,
		  unsigned width, unsigned height)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->surface_copy(nvws, dest, destx, desty, src, srcx, srcy,
			   width, height);
}

static void
nv40_surface_fill(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, unsigned width,
		  unsigned height, unsigned value)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->surface_fill(nvws, dest, destx, desty, width, height, value);
}

void
nv40_init_surface_functions(struct nv40_context *nv40)
{
	nv40->pipe.is_format_supported = nv40_surface_format_supported;
	nv40->pipe.surface_copy = nv40_surface_copy;
	nv40->pipe.surface_fill = nv40_surface_fill;
}
