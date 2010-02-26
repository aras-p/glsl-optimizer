/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
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

/**
 * @file
 * Malloc softpipe winsys. Uses malloc for all memory allocations.
 * 
 * @author Keith Whitwell
 * @author Brian Paul
 * @author Jose Fonseca
 */


#include "util/u_simple_screen.h"/* port to just p_screen */
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "sp_winsys.h"


struct st_softpipe_buffer
{
   struct pipe_buffer base;
   boolean userBuffer;  /** Is this a user-space buffer? */
   void *data;
   void *mapped;
};


/** Cast wrapper */
static INLINE struct st_softpipe_buffer *
st_softpipe_buffer( struct pipe_buffer *buf )
{
   return (struct st_softpipe_buffer *)buf;
}


static void *
st_softpipe_buffer_map(struct pipe_winsys *winsys, 
                       struct pipe_buffer *buf,
                       unsigned flags)
{
   struct st_softpipe_buffer *st_softpipe_buf = st_softpipe_buffer(buf);
   st_softpipe_buf->mapped = st_softpipe_buf->data;
   return st_softpipe_buf->mapped;
}


static void
st_softpipe_buffer_unmap(struct pipe_winsys *winsys, 
                         struct pipe_buffer *buf)
{
   struct st_softpipe_buffer *st_softpipe_buf = st_softpipe_buffer(buf);
   st_softpipe_buf->mapped = NULL;
}


static void
st_softpipe_buffer_destroy(struct pipe_buffer *buf)
{
   struct st_softpipe_buffer *oldBuf = st_softpipe_buffer(buf);

   if (oldBuf->data) {
      if (!oldBuf->userBuffer)
         align_free(oldBuf->data);

      oldBuf->data = NULL;
   }

   FREE(oldBuf);
}


static void
st_softpipe_flush_frontbuffer(struct pipe_winsys *winsys,
                              struct pipe_surface *surf,
                              void *context_private)
{
}



static const char *
st_softpipe_get_name(struct pipe_winsys *winsys)
{
   return "softpipe";
}


static struct pipe_buffer *
st_softpipe_buffer_create(struct pipe_winsys *winsys, 
                          unsigned alignment, 
                          unsigned usage,
                          unsigned size)
{
   struct st_softpipe_buffer *buffer = CALLOC_STRUCT(st_softpipe_buffer);

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.alignment = alignment;
   buffer->base.usage = usage;
   buffer->base.size = size;

   buffer->data = align_malloc(size, alignment);

   return &buffer->base;
}


/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_buffer *
st_softpipe_user_buffer_create(struct pipe_winsys *winsys, 
                               void *ptr, 
                               unsigned bytes)
{
   struct st_softpipe_buffer *buffer;
   
   buffer = CALLOC_STRUCT(st_softpipe_buffer);
   if(!buffer)
      return NULL;
   
   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.size = bytes;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;

   return &buffer->base;
}


static struct pipe_buffer *
st_softpipe_surface_buffer_create(struct pipe_winsys *winsys,
                                  unsigned width, unsigned height,
                                  enum pipe_format format,
                                  unsigned usage,
                                  unsigned tex_usage,
                                  unsigned *stride)
{
   const unsigned alignment = 64;
   unsigned nblocksy;

   nblocksy = util_format_get_nblocksy(format, height);
   *stride = align(util_format_get_stride(format, width), alignment);

   return winsys->buffer_create(winsys, alignment,
                                usage,
                                *stride * nblocksy);
}


static void
st_softpipe_fence_reference(struct pipe_winsys *winsys, 
                            struct pipe_fence_handle **ptr,
                            struct pipe_fence_handle *fence)
{
}


static int
st_softpipe_fence_signalled(struct pipe_winsys *winsys, 
                            struct pipe_fence_handle *fence,
                            unsigned flag)
{
   return 0;
}


static int
st_softpipe_fence_finish(struct pipe_winsys *winsys, 
                         struct pipe_fence_handle *fence,
                         unsigned flag)
{
   return 0;
}


static void
st_softpipe_destroy(struct pipe_winsys *winsys)
{
   FREE(winsys);
}


struct pipe_screen *
softpipe_create_screen_malloc(void)
{
   static struct pipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = CALLOC_STRUCT(pipe_winsys);
   if(!winsys)
      return NULL;

   winsys->destroy = st_softpipe_destroy;
   
   winsys->buffer_create = st_softpipe_buffer_create;
   winsys->user_buffer_create = st_softpipe_user_buffer_create;
   winsys->buffer_map = st_softpipe_buffer_map;
   winsys->buffer_unmap = st_softpipe_buffer_unmap;
   winsys->buffer_destroy = st_softpipe_buffer_destroy;

   winsys->surface_buffer_create = st_softpipe_surface_buffer_create;

   winsys->fence_reference = st_softpipe_fence_reference;
   winsys->fence_signalled = st_softpipe_fence_signalled;
   winsys->fence_finish = st_softpipe_fence_finish;

   winsys->flush_frontbuffer = st_softpipe_flush_frontbuffer;
   winsys->get_name = st_softpipe_get_name;

   screen = softpipe_create_screen(winsys);
   if(!screen)
      st_softpipe_destroy(winsys);

   return screen;
}
