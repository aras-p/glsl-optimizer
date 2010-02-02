/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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

#include "u_simple_screen.h"

#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_simple_screen.h"


static struct pipe_buffer *
pass_buffer_create(struct pipe_screen *screen,
                   unsigned alignment,
                   unsigned usage,
                   unsigned size)
{
   struct pipe_buffer *buffer =
      screen->winsys->buffer_create(screen->winsys, alignment, usage, size);

   buffer->screen = screen;

   return buffer;
}

static struct pipe_buffer *
pass_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes)
{
   struct pipe_buffer *buffer =
      screen->winsys->user_buffer_create(screen->winsys, ptr, bytes);

   buffer->screen = screen;

   return buffer;
}

static struct pipe_buffer *
pass_surface_buffer_create(struct pipe_screen *screen,
                           unsigned width, unsigned height,
                           enum pipe_format format,
                           unsigned usage,
                           unsigned tex_usage,
                           unsigned *stride)
{
   struct pipe_buffer *buffer =
      screen->winsys->surface_buffer_create(screen->winsys, width, height,
                                            format, usage, tex_usage, stride);

   buffer->screen = screen;

   return buffer;
}

static void *
pass_buffer_map(struct pipe_screen *screen,
                struct pipe_buffer *buf,
                unsigned usage)
{
   return screen->winsys->buffer_map(screen->winsys, buf, usage);
}

static void
pass_buffer_unmap(struct pipe_screen *screen,
                  struct pipe_buffer *buf)
{
   screen->winsys->buffer_unmap(screen->winsys, buf);
}

static void
pass_buffer_destroy(struct pipe_buffer *buf)
{
   buf->screen->winsys->buffer_destroy(buf);
}


static void
pass_flush_frontbuffer(struct pipe_screen *screen,
                       struct pipe_surface *surf,
                       void *context_private)
{
   screen->winsys->flush_frontbuffer(screen->winsys, surf, context_private);
}

static void
pass_fence_reference(struct pipe_screen *screen,
                     struct pipe_fence_handle **ptr,
                     struct pipe_fence_handle *fence)
{
   screen->winsys->fence_reference(screen->winsys, ptr, fence);
}

static int
pass_fence_signalled(struct pipe_screen *screen,
                     struct pipe_fence_handle *fence,
                     unsigned flag)
{
   return screen->winsys->fence_signalled(screen->winsys, fence, flag);
}

static int
pass_fence_finish(struct pipe_screen *screen,
                  struct pipe_fence_handle *fence,
                  unsigned flag)
{
   return screen->winsys->fence_finish(screen->winsys, fence, flag);
}

void
u_simple_screen_init(struct pipe_screen *screen)
{
   screen->buffer_create = pass_buffer_create;
   screen->user_buffer_create = pass_user_buffer_create;
   screen->surface_buffer_create = pass_surface_buffer_create;

   screen->buffer_map = pass_buffer_map;
   screen->buffer_unmap = pass_buffer_unmap;
   screen->buffer_destroy = pass_buffer_destroy;
   screen->flush_frontbuffer = pass_flush_frontbuffer;
   screen->fence_reference = pass_fence_reference;
   screen->fence_signalled = pass_fence_signalled;
   screen->fence_finish = pass_fence_finish;
}

const char *
u_simple_screen_winsys_name(struct pipe_screen *screen)
{
   return screen->winsys->get_name(screen->winsys);
}
