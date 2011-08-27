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

#ifndef vl_winsys_h
#define vl_winsys_h

#include <X11/Xlib.h>
#include "pipe/p_defines.h"
#include "pipe/p_format.h"

struct pipe_screen;
struct pipe_surface;

struct vl_screen
{
   struct pipe_screen *pscreen;
};

struct vl_context
{
   struct vl_screen *vscreen;
   struct pipe_context *pipe;
};

struct vl_screen*
vl_screen_create(Display *display, int screen);

void vl_screen_destroy(struct vl_screen *vscreen);

struct vl_context*
vl_video_create(struct vl_screen *vscreen);

void vl_video_destroy(struct vl_context *vctx);

struct pipe_surface*
vl_drawable_surface_get(struct vl_context *vctx, Drawable drawable);

void*
vl_contextprivate_get(struct vl_context *vctx, struct pipe_surface *drawable_surface);

#endif
