/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "util/u_memory.h"
#include "util/u_hash_table.h"

#include "tr_dump.h"
#include "tr_state.h"
#include "tr_screen.h"
#include "tr_texture.h"
#include "tr_winsys.h"


static unsigned trace_buffer_hash(void *buffer)
{
   return (unsigned)(uintptr_t)buffer;
}


static int trace_buffer_compare(void *buffer1, void *buffer2)
{
   return (char *)buffer2 - (char *)buffer1;
}

                  
static const char *
trace_winsys_get_name(struct pipe_winsys *_winsys)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   const char *result;
   
   trace_dump_call_begin("pipe_winsys", "get_name");
   
   trace_dump_arg(ptr, winsys);

   result = winsys->get_name(winsys);
   
   trace_dump_ret(string, result);
   
   trace_dump_call_end();
   
   return result;
}


static void 
trace_winsys_flush_frontbuffer(struct pipe_winsys *_winsys,
                               struct pipe_surface *surface,
                               void *context_private)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;

   assert(surface);
   if(surface->texture) {
      struct trace_screen *tr_scr = trace_screen(surface->texture->screen);
      struct trace_texture *tr_tex = trace_texture(tr_scr, surface->texture);
      struct trace_surface *tr_surf = trace_surface(tr_tex, surface);
      surface = tr_surf->surface;
   }
   
   trace_dump_call_begin("pipe_winsys", "flush_frontbuffer");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(ptr, surface);
   /* XXX: hide, as there is nothing we can do with this
   trace_dump_arg(ptr, context_private);
   */

   winsys->flush_frontbuffer(winsys, surface, context_private);
   
   trace_dump_call_end();
}


static struct pipe_surface *
trace_winsys_surface_alloc(struct pipe_winsys *_winsys)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   struct pipe_surface *result;
   
   trace_dump_call_begin("pipe_winsys", "surface_alloc");
   
   trace_dump_arg(ptr, winsys);

   result = winsys->surface_alloc(winsys);
   
   trace_dump_ret(ptr, result);
   
   trace_dump_call_end();
   
   assert(!result || !result->texture);

   return result;
}


static int
trace_winsys_surface_alloc_storage(struct pipe_winsys *_winsys,
                                   struct pipe_surface *surface,
                                   unsigned width, unsigned height,
                                   enum pipe_format format,
                                   unsigned flags,
                                   unsigned tex_usage)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   int result;
   
   assert(surface && !surface->texture);

   trace_dump_call_begin("pipe_winsys", "surface_alloc_storage");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(ptr, surface);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);
   trace_dump_arg(format, format);
   trace_dump_arg(uint, flags);
   trace_dump_arg(uint, tex_usage);

   result = winsys->surface_alloc_storage(winsys,
                                          surface,
                                          width, height,
                                          format,
                                          flags,
                                          tex_usage);
   
   trace_dump_ret(int, result);
   
   trace_dump_call_end();
   
   return result;
}


static void
trace_winsys_surface_release(struct pipe_winsys *_winsys, 
                             struct pipe_surface **psurface)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   struct pipe_surface *surface = *psurface;
   
   assert(psurface && *psurface && !(*psurface)->texture);
   
   trace_dump_call_begin("pipe_winsys", "surface_release");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(ptr, surface);

   winsys->surface_release(winsys, psurface);
   
   trace_dump_call_end();
}


static struct pipe_buffer *
trace_winsys_buffer_create(struct pipe_winsys *_winsys, 
                           unsigned alignment, 
                           unsigned usage,
                           unsigned size)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   struct pipe_buffer *buffer;
   
   trace_dump_call_begin("pipe_winsys", "buffer_create");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(uint, alignment);
   trace_dump_arg(uint, usage);
   trace_dump_arg(uint, size);

   buffer = winsys->buffer_create(winsys, alignment, usage, size);
   
   trace_dump_ret(ptr, buffer);
   
   trace_dump_call_end();

   /* Zero the buffer to avoid dumping uninitialized memory */
   if(buffer->usage & PIPE_BUFFER_USAGE_CPU_WRITE) {
      void *map;
      map = winsys->buffer_map(winsys, buffer, PIPE_BUFFER_USAGE_CPU_WRITE);
      if(map) {
         memset(map, 0, buffer->size);
         winsys->buffer_unmap(winsys, buffer);
      }
   }
   
   return buffer;
}


static struct pipe_buffer *
trace_winsys_user_buffer_create(struct pipe_winsys *_winsys, 
                                void *data,
                                unsigned size)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   struct pipe_buffer *result;
   
   trace_dump_call_begin("pipe_winsys", "user_buffer_create");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg_begin("data");
   trace_dump_bytes(data, size);
   trace_dump_arg_end();
   trace_dump_arg(uint, size);

   result = winsys->user_buffer_create(winsys, data, size);
   
   trace_dump_ret(ptr, result);
   
   trace_dump_call_end();
   
   /* XXX: Mark the user buffers. (we should wrap pipe_buffers, but is is 
    * impossible to do so while texture-less surfaces are still around */
   if(result) {
      assert(!(result->usage & TRACE_BUFFER_USAGE_USER));
      result->usage |= TRACE_BUFFER_USAGE_USER;
   }
   
   return result;
}


void
trace_winsys_user_buffer_update(struct pipe_winsys *_winsys, 
                                struct pipe_buffer *buffer)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   const void *map;
   
   if(buffer && buffer->usage & TRACE_BUFFER_USAGE_USER) {
      map = winsys->buffer_map(winsys, buffer, PIPE_BUFFER_USAGE_CPU_READ);
      if(map) {
         trace_dump_call_begin("pipe_winsys", "buffer_write");
         
         trace_dump_arg(ptr, winsys);
         
         trace_dump_arg(ptr, buffer);
         
         trace_dump_arg_begin("data");
         trace_dump_bytes(map, buffer->size);
         trace_dump_arg_end();
      
         trace_dump_arg_begin("size");
         trace_dump_uint(buffer->size);
         trace_dump_arg_end();
      
         trace_dump_call_end();
         
         winsys->buffer_unmap(winsys, buffer);
      }
   }
}


static void *
trace_winsys_buffer_map(struct pipe_winsys *_winsys, 
                        struct pipe_buffer *buffer,
                        unsigned usage)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   void *map;
   
   map = winsys->buffer_map(winsys, buffer, usage);
   if(map) {
      if(usage & PIPE_BUFFER_USAGE_CPU_WRITE) {
         assert(!hash_table_get(tr_ws->buffer_maps, buffer));
         hash_table_set(tr_ws->buffer_maps, buffer, map);
      }
   }
   
   return map;
}


static void
trace_winsys_buffer_unmap(struct pipe_winsys *_winsys, 
                          struct pipe_buffer *buffer)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   const void *map;
   
   map = hash_table_get(tr_ws->buffer_maps, buffer);
   if(map) {
      trace_dump_call_begin("pipe_winsys", "buffer_write");
      
      trace_dump_arg(ptr, winsys);
      
      trace_dump_arg(ptr, buffer);
      
      trace_dump_arg_begin("data");
      trace_dump_bytes(map, buffer->size);
      trace_dump_arg_end();

      trace_dump_arg_begin("size");
      trace_dump_uint(buffer->size);
      trace_dump_arg_end();
   
      trace_dump_call_end();

      hash_table_remove(tr_ws->buffer_maps, buffer);
   }
   
   winsys->buffer_unmap(winsys, buffer);
}


static void
trace_winsys_buffer_destroy(struct pipe_winsys *_winsys,
                            struct pipe_buffer *buffer)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   
   trace_dump_call_begin("pipe_winsys", "buffer_destroy");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(ptr, buffer);

   winsys->buffer_destroy(winsys, buffer);
   
   trace_dump_call_end();
}


static void
trace_winsys_fence_reference(struct pipe_winsys *_winsys,
                             struct pipe_fence_handle **pdst,
                             struct pipe_fence_handle *src)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   struct pipe_fence_handle *dst = *pdst;
   
   trace_dump_call_begin("pipe_winsys", "fence_reference");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(ptr, src);

   winsys->fence_reference(winsys, pdst, src);
   
   trace_dump_call_end();
}


static int
trace_winsys_fence_signalled(struct pipe_winsys *_winsys,
                             struct pipe_fence_handle *fence,
                             unsigned flag)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   int result;
   
   trace_dump_call_begin("pipe_winsys", "fence_signalled");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(ptr, fence);
   trace_dump_arg(uint, flag);

   result = winsys->fence_signalled(winsys, fence, flag);
   
   trace_dump_ret(int, result);
   
   trace_dump_call_end();
   
   return result;
}


static int
trace_winsys_fence_finish(struct pipe_winsys *_winsys,
                          struct pipe_fence_handle *fence,
                          unsigned flag)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   int result;
   
   trace_dump_call_begin("pipe_winsys", "fence_finish");
   
   trace_dump_arg(ptr, winsys);
   trace_dump_arg(ptr, fence);
   trace_dump_arg(uint, flag);

   result = winsys->fence_finish(winsys, fence, flag);
   
   trace_dump_ret(int, result);
   
   trace_dump_call_end();
   
   return result;
}


static void
trace_winsys_destroy(struct pipe_winsys *_winsys)
{
   struct trace_winsys *tr_ws = trace_winsys(_winsys);
   struct pipe_winsys *winsys = tr_ws->winsys;
   
   trace_dump_call_begin("pipe_winsys", "destroy");
   
   trace_dump_arg(ptr, winsys);

   /* 
   winsys->destroy(winsys); 
   */
   
   trace_dump_call_end();
   
   hash_table_destroy(tr_ws->buffer_maps);

   FREE(tr_ws);
}


struct pipe_winsys *
trace_winsys_create(struct pipe_winsys *winsys)
{
   struct trace_winsys *tr_ws;
   
   if(!winsys)
      goto error1;
   
   tr_ws = CALLOC_STRUCT(trace_winsys);
   if(!tr_ws)
      goto error1;

   tr_ws->base.destroy = trace_winsys_destroy;
   tr_ws->base.get_name = trace_winsys_get_name;
   tr_ws->base.flush_frontbuffer = trace_winsys_flush_frontbuffer;
   tr_ws->base.surface_alloc = trace_winsys_surface_alloc;
   tr_ws->base.surface_alloc_storage = trace_winsys_surface_alloc_storage;
   tr_ws->base.surface_release = trace_winsys_surface_release;
   tr_ws->base.buffer_create = trace_winsys_buffer_create;
   tr_ws->base.user_buffer_create = trace_winsys_user_buffer_create;
   tr_ws->base.buffer_map = trace_winsys_buffer_map;
   tr_ws->base.buffer_unmap = trace_winsys_buffer_unmap;
   tr_ws->base.buffer_destroy = trace_winsys_buffer_destroy;
   tr_ws->base.fence_reference = trace_winsys_fence_reference;
   tr_ws->base.fence_signalled = trace_winsys_fence_signalled;
   tr_ws->base.fence_finish = trace_winsys_fence_finish;
   
   tr_ws->winsys = winsys;

   tr_ws->buffer_maps = hash_table_create(trace_buffer_hash, 
                                          trace_buffer_compare);
   if(!tr_ws->buffer_maps)
      goto error2;
   
   trace_dump_call_begin("", "pipe_winsys_create");
   trace_dump_ret(ptr, winsys);
   trace_dump_call_end();

   return &tr_ws->base;
   
error2:
   FREE(tr_ws);
error1:
   return winsys;
}
