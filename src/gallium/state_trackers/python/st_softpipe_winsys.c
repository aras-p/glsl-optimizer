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
 * Softpipe support. 
 * 
 * @author Keith Whitwell
 * @author Brian Paul
 * @author Jose Fonseca
 */


#include "pipe/p_winsys.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "softpipe/sp_winsys.h"
#include "st_winsys.h"


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
st_softpipe_buffer_destroy(struct pipe_winsys *winsys,
                           struct pipe_buffer *buf)
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

   buffer->base.refcount = 1;
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
   
   buffer->base.refcount = 1;
   buffer->base.size = bytes;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;

   return &buffer->base;
}


/**
 * Round n up to next multiple.
 */
static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
   return (n + multiple - 1) & ~(multiple - 1);
}


static int
st_softpipe_surface_alloc_storage(struct pipe_winsys *winsys,
                                  struct pipe_surface *surf,
                                  unsigned width, unsigned height,
                                  enum pipe_format format, 
                                  unsigned flags,
                                  unsigned tex_usage)
{
   const unsigned alignment = 64;

   surf->width = width;
   surf->height = height;
   surf->format = format;
   pf_get_block(format, &surf->block);
   surf->nblocksx = pf_get_nblocksx(&surf->block, width);
   surf->nblocksy = pf_get_nblocksy(&surf->block, height);
   surf->stride = round_up(surf->nblocksx * surf->block.size, alignment);
   surf->usage = flags;

   assert(!surf->buffer);
   surf->buffer = winsys->buffer_create(winsys, alignment,
                                        PIPE_BUFFER_USAGE_PIXEL,
                                        surf->stride * surf->nblocksy);
   if(!surf->buffer)
      return -1;
   
   return 0;
}


static struct pipe_surface *
st_softpipe_surface_alloc(struct pipe_winsys *winsys)
{
   struct pipe_surface *surface = CALLOC_STRUCT(pipe_surface);

   assert(winsys);

   surface->refcount = 1;
   surface->winsys = winsys;

   return surface;
}


static void
st_softpipe_surface_release(struct pipe_winsys *winsys, 
                            struct pipe_surface **s)
{
   struct pipe_surface *surf = *s;
   assert(!surf->texture);
   surf->refcount--;
   if (surf->refcount == 0) {
      if (surf->buffer)
	winsys_buffer_reference(winsys, &surf->buffer, NULL);
      free(surf);
   }
   *s = NULL;
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


static struct pipe_screen *
st_softpipe_screen_create(void)
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

   winsys->surface_alloc = st_softpipe_surface_alloc;
   winsys->surface_alloc_storage = st_softpipe_surface_alloc_storage;
   winsys->surface_release = st_softpipe_surface_release;

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


static struct pipe_context *
st_softpipe_context_create(struct pipe_screen *screen)
{
   return softpipe_create(screen, screen->winsys, NULL);
}


const struct st_winsys st_softpipe_winsys = {
   &st_softpipe_screen_create,
   &st_softpipe_context_create,
};
