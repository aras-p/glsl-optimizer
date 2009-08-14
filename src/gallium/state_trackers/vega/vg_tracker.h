/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef VG_TRACKER_H
#define VG_TRACKER_H

#include "VG/openvg.h"

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"

#define ST_SURFACE_FRONT_LEFT   0
#define ST_SURFACE_BACK_LEFT    1
#define ST_SURFACE_FRONT_RIGHT  2
#define ST_SURFACE_BACK_RIGHT   3
#define ST_SURFACE_DEPTH        8

struct vg_context;
struct st_framebuffer;
struct pipe_context;
struct pipe_fence_handle;
struct pipe_surface;


struct vg_context *st_create_context(struct pipe_context *pipe,
                                     const void *visual,
                                     struct vg_context *share);

void st_destroy_context( struct vg_context *st );

void st_copy_context_state(struct vg_context *dst, struct vg_context *src,
                           uint mask);

struct st_framebuffer *st_create_framebuffer(const void *visual,
                                             enum pipe_format colorFormat,
                                             enum pipe_format depthFormat,
                                             enum pipe_format stencilFormat,
                                             uint width, uint height,
                                             void *privateData);

void st_resize_framebuffer(struct st_framebuffer *stfb,
                           uint width, uint height);

void st_set_framebuffer_surface(struct st_framebuffer *stfb,
                                uint surfIndex, struct pipe_surface *surf);

void st_get_framebuffer_dimensions( struct st_framebuffer *stfb,
				    uint *width, uint *height);

int st_bind_texture_surface(struct pipe_surface *ps, int target, int level,
                            enum pipe_format format);

int st_unbind_texture_surface(struct pipe_surface *ps, int target, int level);

int st_get_framebuffer_surface(struct st_framebuffer *stfb,
                               uint surfIndex, struct pipe_surface **surf);

int st_get_framebuffer_texture(struct st_framebuffer *stfb,
                               uint surfIndex, struct pipe_texture **tex);

void *st_framebuffer_private(struct st_framebuffer *stfb);

void st_unreference_framebuffer(struct st_framebuffer *stfb);

void st_make_current(struct vg_context *st,
                     struct st_framebuffer *draw,
                     struct st_framebuffer *read);

struct vg_context *st_get_current(void);

void st_flush(struct vg_context *st, uint pipeFlushFlags,
               struct pipe_fence_handle **fence);
void st_finish(struct vg_context *st);

void st_notify_swapbuffers(struct st_framebuffer *stfb);
void st_notify_swapbuffers_complete(struct st_framebuffer *stfb);


/** Generic function type */
typedef void (*st_proc)();

st_proc st_get_proc_address(const char *procname);

#endif
