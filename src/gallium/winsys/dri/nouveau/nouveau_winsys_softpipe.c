/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/
/*
 * Authors: Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#include "imports.h"

#include "pipe/p_defines.h"
#include "pipe/p_format.h"
#include "softpipe/sp_winsys.h"

#include "nouveau_context.h"
#include "nouveau_winsys_pipe.h"

struct nouveau_softpipe_winsys {
   struct softpipe_winsys sws;
   struct nouveau_context *nv;
};

/**
 * Return list of surface formats supported by this driver.
 */
static boolean
nouveau_is_format_supported(struct softpipe_winsys *sws, uint format)
{
	switch (format) {
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case PIPE_FORMAT_R5G6B5_UNORM:
	case PIPE_FORMAT_Z24S8_UNORM:
		return TRUE;
	default:
		break;
	};

	return FALSE;
}

struct pipe_context *
nouveau_create_softpipe(struct nouveau_context *nv)
{
	struct nouveau_softpipe_winsys *nvsws;
	struct pipe_screen *pscreen;
	struct pipe_winsys *ws;

	ws = nouveau_create_pipe_winsys(nv);
	if (!ws)
		return NULL;
	pscreen = softpipe_create_screen(ws);

	nvsws = CALLOC_STRUCT(nouveau_softpipe_winsys);
	if (!nvsws)
		return NULL;

	nvsws->sws.is_format_supported = nouveau_is_format_supported;
	nvsws->nv = nv;

	return softpipe_create(pscreen, ws, &nvsws->sws);
}

