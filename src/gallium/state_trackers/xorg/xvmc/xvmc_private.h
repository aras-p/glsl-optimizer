/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#ifndef xvmc_private_h
#define xvmc_private_h

#include <X11/Xlib.h>
#include <X11/extensions/XvMClib.h>

#define BLOCK_SIZE_SAMPLES 64
#define BLOCK_SIZE_BYTES (BLOCK_SIZE_SAMPLES * 2)

struct pipe_video_context;
struct pipe_surface;
struct pipe_fence_handle;

typedef struct
{
	struct pipe_video_context *vpipe;
	struct pipe_surface *backbuffer;
} XvMCContextPrivate;

typedef struct
{
	struct pipe_video_surface *pipe_vsfc;
	struct pipe_fence_handle *render_fence;
	struct pipe_fence_handle *disp_fence;
	
	/* Some XvMC functions take a surface but not a context,
	   so we keep track of which context each surface belongs to. */
	XvMCContext *context;
} XvMCSurfacePrivate;

#endif /* xvmc_private_h */
