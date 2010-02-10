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

#include "pipe/p_state.h"
#include "util/u_simple_screen.h"
#include "u_timed_winsys.h"
#include "util/u_memory.h"
#include "os/os_time.h"


struct timed_winsys {
   struct pipe_winsys base;
   struct pipe_winsys *backend;
   uint64_t last_dump;
   struct {
      const char *name_key;
      double total;
      unsigned calls;
   } funcs[13];
};


static struct timed_winsys *timed_winsys( struct pipe_winsys *winsys )
{
   return (struct timed_winsys *)winsys;
}


static void time_display( struct pipe_winsys *winsys )
{
   struct timed_winsys *tws = timed_winsys(winsys);
   unsigned i;
   double overall = 0;

   for (i = 0; i < Elements(tws->funcs); i++) {
      if (tws->funcs[i].name_key) {
         debug_printf("*** %-25s %5.3fms (%d calls, avg %.3fms)\n", 
                      tws->funcs[i].name_key,
                      tws->funcs[i].total,
                      tws->funcs[i].calls,
                      tws->funcs[i].total / tws->funcs[i].calls);
         overall += tws->funcs[i].total;
         tws->funcs[i].calls = 0;
         tws->funcs[i].total = 0;
      }
   }

   debug_printf("*** %-25s %5.3fms\n", 
                "OVERALL WINSYS",
                overall);
}

static void time_finish( struct pipe_winsys *winsys,
                         long long startval, 
                         unsigned idx,
                         const char *name ) 
{
   struct timed_winsys *tws = timed_winsys(winsys);
   int64_t endval = os_time_get();
   double elapsed = (endval - startval)/1000.0;

   if (endval - startval > 1000LL) 
      debug_printf("*** %s %.3f\n", name, elapsed );

   assert( tws->funcs[idx].name_key == name ||
           tws->funcs[idx].name_key == NULL);

   tws->funcs[idx].name_key = name;
   tws->funcs[idx].total += elapsed;
   tws->funcs[idx].calls++;

   if (endval - tws->last_dump > 10LL * 1000LL * 1000LL) {
      time_display( winsys );
      tws->last_dump = endval;
   }
}


/* Pipe has no concept of pools, but the psb driver passes a flag that
 * can be mapped onto pools in the backend.
 */
static struct pipe_buffer *
timed_buffer_create(struct pipe_winsys *winsys, 
                    unsigned alignment, 
                    unsigned usage, 
                    unsigned size )
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   struct pipe_buffer *buf =
      backend->buffer_create( backend, alignment, usage, size );

   time_finish(winsys, start, 0, __FUNCTION__);
   
   return buf;
}




static struct pipe_buffer *
timed_user_buffer_create(struct pipe_winsys *winsys,
                             void *data, 
                             unsigned bytes) 
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   struct pipe_buffer *buf = backend->user_buffer_create( backend, data, bytes );

   time_finish(winsys, start, 1, __FUNCTION__);
   
   return buf;
}


static void *
timed_buffer_map(struct pipe_winsys *winsys,
                     struct pipe_buffer *buf,
                     unsigned flags)
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   void *map = backend->buffer_map( backend, buf, flags );

   time_finish(winsys, start, 2, __FUNCTION__);
   
   return map;
}


static void
timed_buffer_unmap(struct pipe_winsys *winsys,
                       struct pipe_buffer *buf)
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   backend->buffer_unmap( backend, buf );

   time_finish(winsys, start, 3, __FUNCTION__);
}


static void
timed_buffer_destroy(struct pipe_buffer *buf)
{
   struct pipe_winsys *winsys = buf->screen->winsys;
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   backend->buffer_destroy( buf );

   time_finish(winsys, start, 4, __FUNCTION__);
}


static void
timed_flush_frontbuffer( struct pipe_winsys *winsys,
                         struct pipe_surface *surf,
                         void *context_private)
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   backend->flush_frontbuffer( backend, surf, context_private );

   time_finish(winsys, start, 5, __FUNCTION__);
}




static struct pipe_buffer *
timed_surface_buffer_create(struct pipe_winsys *winsys,
                              unsigned width, unsigned height,
                              enum pipe_format format, 
                              unsigned usage,
                              unsigned tex_usage,
                              unsigned *stride)
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   struct pipe_buffer *ret = backend->surface_buffer_create( backend, width, height, 
                                                             format, usage, tex_usage, stride );

   time_finish(winsys, start, 7, __FUNCTION__);
   
   return ret;
}


static const char *
timed_get_name( struct pipe_winsys *winsys )
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   const char *ret = backend->get_name( backend );

   time_finish(winsys, start, 9, __FUNCTION__);
   
   return ret;
}

static void
timed_fence_reference(struct pipe_winsys *winsys,
                    struct pipe_fence_handle **ptr,
                    struct pipe_fence_handle *fence)
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   backend->fence_reference( backend, ptr, fence );

   time_finish(winsys, start, 10, __FUNCTION__);
}


static int
timed_fence_signalled( struct pipe_winsys *winsys,
                       struct pipe_fence_handle *fence,
                       unsigned flag )
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   int ret = backend->fence_signalled( backend, fence, flag );

   time_finish(winsys, start, 11, __FUNCTION__);
   
   return ret;
}

static int
timed_fence_finish( struct pipe_winsys *winsys,
                     struct pipe_fence_handle *fence,
                     unsigned flag )
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   int64_t start = os_time_get();

   int ret = backend->fence_finish( backend, fence, flag );

   time_finish(winsys, start, 12, __FUNCTION__);
   
   return ret;
}

static void
timed_winsys_destroy( struct pipe_winsys *winsys )
{
   struct pipe_winsys *backend = timed_winsys(winsys)->backend;
   backend->destroy( backend );
   FREE(winsys);
}



struct pipe_winsys *u_timed_winsys_create( struct pipe_winsys *backend )
{
   struct timed_winsys *ws = CALLOC_STRUCT(timed_winsys);
   
   ws->base.user_buffer_create = timed_user_buffer_create;
   ws->base.buffer_map = timed_buffer_map;
   ws->base.buffer_unmap = timed_buffer_unmap;
   ws->base.buffer_destroy = timed_buffer_destroy;
   ws->base.buffer_create = timed_buffer_create;
   ws->base.surface_buffer_create = timed_surface_buffer_create;
   ws->base.flush_frontbuffer = timed_flush_frontbuffer;
   ws->base.get_name = timed_get_name;
   ws->base.fence_reference = timed_fence_reference;
   ws->base.fence_signalled = timed_fence_signalled;
   ws->base.fence_finish = timed_fence_finish;
   ws->base.destroy = timed_winsys_destroy;
   
   ws->backend = backend;

   return &ws->base;
}

