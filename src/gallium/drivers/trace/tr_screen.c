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

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "tr_buffer.h"
#include "tr_dump.h"
#include "tr_dump_state.h"
#include "tr_texture.h"
#include "tr_context.h"
#include "tr_screen.h"

#include "util/u_inlines.h"
#include "pipe/p_format.h"


static boolean trace = FALSE;
static boolean rbug = FALSE;

static const char *
trace_screen_get_name(struct pipe_screen *_screen)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   const char *result;

   trace_dump_call_begin("pipe_screen", "get_name");

   trace_dump_arg(ptr, screen);

   result = screen->get_name(screen);

   trace_dump_ret(string, result);

   trace_dump_call_end();

   return result;
}


static const char *
trace_screen_get_vendor(struct pipe_screen *_screen)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   const char *result;

   trace_dump_call_begin("pipe_screen", "get_vendor");

   trace_dump_arg(ptr, screen);

   result = screen->get_vendor(screen);

   trace_dump_ret(string, result);

   trace_dump_call_end();

   return result;
}


static int
trace_screen_get_param(struct pipe_screen *_screen,
                       int param)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   int result;

   trace_dump_call_begin("pipe_screen", "get_param");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(int, param);

   result = screen->get_param(screen, param);

   trace_dump_ret(int, result);

   trace_dump_call_end();

   return result;
}


static float
trace_screen_get_paramf(struct pipe_screen *_screen,
                        int param)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   float result;

   trace_dump_call_begin("pipe_screen", "get_paramf");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(int, param);

   result = screen->get_paramf(screen, param);

   trace_dump_ret(float, result);

   trace_dump_call_end();

   return result;
}


static boolean
trace_screen_is_format_supported(struct pipe_screen *_screen,
                                 enum pipe_format format,
                                 enum pipe_texture_target target,
                                 unsigned tex_usage,
                                 unsigned geom_flags)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   boolean result;

   trace_dump_call_begin("pipe_screen", "is_format_supported");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(format, format);
   trace_dump_arg(int, target);
   trace_dump_arg(uint, tex_usage);
   trace_dump_arg(uint, geom_flags);

   result = screen->is_format_supported(screen, format, target, tex_usage, geom_flags);

   trace_dump_ret(bool, result);

   trace_dump_call_end();

   return result;
}


static struct pipe_context *
trace_screen_context_create(struct pipe_screen *_screen, void *priv)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_context *result;

   trace_dump_call_begin("pipe_screen", "context_create");

   trace_dump_arg(ptr, screen);

   result = screen->context_create(screen, priv);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   result = trace_context_create(tr_scr, result);

   return result;
}


static void
trace_screen_flush_frontbuffer(struct pipe_screen *_screen,
                               struct pipe_surface *_surface,
                               void *context_private)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_surface *tr_surf = trace_surface(_surface);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_surface *surface = tr_surf->surface;

   trace_dump_call_begin("pipe_screen", "flush_frontbuffer");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, surface);
   /* XXX: hide, as there is nothing we can do with this
   trace_dump_arg(ptr, context_private);
   */

   screen->flush_frontbuffer(screen, surface, context_private);

   trace_dump_call_end();
}


/********************************************************************
 * texture
 */


static struct pipe_texture *
trace_screen_texture_create(struct pipe_screen *_screen,
                            const struct pipe_texture *templat)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_texture *result;

   trace_dump_call_begin("pipe_screen", "texture_create");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(template, templat);

   result = screen->texture_create(screen, templat);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   result = trace_texture_create(tr_scr, result);

   return result;
}


static struct pipe_texture *
trace_screen_texture_blanket(struct pipe_screen *_screen,
                             const struct pipe_texture *templat,
                             const unsigned *ppitch,
                             struct pipe_buffer *_buffer)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_buffer *tr_buf = trace_buffer(_buffer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *buffer = tr_buf->buffer;
   unsigned pitch = *ppitch;
   struct pipe_texture *result;

   trace_dump_call_begin("pipe_screen", "texture_blanket");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(template, templat);
   trace_dump_arg(uint, pitch);
   trace_dump_arg(ptr, buffer);

   result = screen->texture_blanket(screen, templat, ppitch, buffer);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   result = trace_texture_create(tr_scr, result);

   return result;
}


static void
trace_screen_texture_destroy(struct pipe_texture *_texture)
{
   struct trace_screen *tr_scr = trace_screen(_texture->screen);
   struct trace_texture *tr_tex = trace_texture(_texture);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_texture *texture = tr_tex->texture;

   assert(texture->screen == screen);

   trace_dump_call_begin("pipe_screen", "texture_destroy");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, texture);

   trace_dump_call_end();

   trace_texture_destroy(tr_tex);
}


/********************************************************************
 * surface
 */


static struct pipe_surface *
trace_screen_get_tex_surface(struct pipe_screen *_screen,
                             struct pipe_texture *_texture,
                             unsigned face, unsigned level,
                             unsigned zslice,
                             unsigned usage)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_texture *tr_tex = trace_texture(_texture);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_texture *texture = tr_tex->texture;
   struct pipe_surface *result = NULL;

   assert(texture->screen == screen);

   trace_dump_call_begin("pipe_screen", "get_tex_surface");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, texture);
   trace_dump_arg(uint, face);
   trace_dump_arg(uint, level);
   trace_dump_arg(uint, zslice);
   trace_dump_arg(uint, usage);

   result = screen->get_tex_surface(screen, texture, face, level, zslice, usage);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   result = trace_surface_create(tr_tex, result);

   return result;
}


static void
trace_screen_tex_surface_destroy(struct pipe_surface *_surface)
{
   struct trace_screen *tr_scr = trace_screen(_surface->texture->screen);
   struct trace_surface *tr_surf = trace_surface(_surface);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_surface *surface = tr_surf->surface;

   trace_dump_call_begin("pipe_screen", "tex_surface_destroy");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, surface);

   trace_dump_call_end();

   trace_surface_destroy(tr_surf);
}


/********************************************************************
 * transfer
 */


static struct pipe_transfer *
trace_screen_get_tex_transfer(struct pipe_screen *_screen,
                              struct pipe_texture *_texture,
                              unsigned face, unsigned level,
                              unsigned zslice,
                              enum pipe_transfer_usage usage,
                              unsigned x, unsigned y, unsigned w, unsigned h)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_texture *tr_tex = trace_texture(_texture);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_texture *texture = tr_tex->texture;
   struct pipe_transfer *result = NULL;

   assert(texture->screen == screen);

   trace_dump_call_begin("pipe_screen", "get_tex_transfer");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, texture);
   trace_dump_arg(uint, face);
   trace_dump_arg(uint, level);
   trace_dump_arg(uint, zslice);
   trace_dump_arg(uint, usage);

   trace_dump_arg(uint, x);
   trace_dump_arg(uint, y);
   trace_dump_arg(uint, w);
   trace_dump_arg(uint, h);

   result = screen->get_tex_transfer(screen, texture, face, level, zslice, usage,
                                     x, y, w, h);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   if (result)
      result = trace_transfer_create(tr_tex, result);

   return result;
}


static void
trace_screen_tex_transfer_destroy(struct pipe_transfer *_transfer)
{
   struct trace_screen *tr_scr = trace_screen(_transfer->texture->screen);
   struct trace_transfer *tr_trans = trace_transfer(_transfer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_transfer *transfer = tr_trans->transfer;

   trace_dump_call_begin("pipe_screen", "tex_transfer_destroy");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, transfer);

   trace_dump_call_end();

   trace_transfer_destroy(tr_trans);
}


static void *
trace_screen_transfer_map(struct pipe_screen *_screen,
                          struct pipe_transfer *_transfer)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_transfer *tr_trans = trace_transfer(_transfer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_transfer *transfer = tr_trans->transfer;
   void *map;

   map = screen->transfer_map(screen, transfer);
   if(map) {
      if(transfer->usage & PIPE_TRANSFER_WRITE) {
         assert(!tr_trans->map);
         tr_trans->map = map;
      }
   }

   return map;
}


static void
trace_screen_transfer_unmap(struct pipe_screen *_screen,
                           struct pipe_transfer *_transfer)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_transfer *tr_trans = trace_transfer(_transfer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_transfer *transfer = tr_trans->transfer;

   if(tr_trans->map) {
      size_t size = util_format_get_nblocksy(transfer->texture->format, transfer->height) * transfer->stride;

      trace_dump_call_begin("pipe_screen", "transfer_write");

      trace_dump_arg(ptr, screen);

      trace_dump_arg(ptr, transfer);

      trace_dump_arg_begin("stride");
      trace_dump_uint(transfer->stride);
      trace_dump_arg_end();

      trace_dump_arg_begin("data");
      trace_dump_bytes(tr_trans->map, size);
      trace_dump_arg_end();

      trace_dump_arg_begin("size");
      trace_dump_uint(size);
      trace_dump_arg_end();

      trace_dump_call_end();

      tr_trans->map = NULL;
   }

   screen->transfer_unmap(screen, transfer);
}


/********************************************************************
 * buffer
 */


static struct pipe_buffer *
trace_screen_surface_buffer_create(struct pipe_screen *_screen,
                                   unsigned width, unsigned height,
                                   enum pipe_format format,
                                   unsigned usage,
                                   unsigned tex_usage,
                                   unsigned *pstride)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   unsigned stride;
   struct pipe_buffer *result;

   trace_dump_call_begin("pipe_screen", "surface_buffer_create");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);
   trace_dump_arg(format, format);
   trace_dump_arg(uint, usage);
   trace_dump_arg(uint, tex_usage);

   result = screen->surface_buffer_create(screen,
                                          width, height,
                                          format,
                                          usage,
                                          tex_usage,
                                          pstride);

   stride = *pstride;

   trace_dump_arg(uint, stride);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   return trace_buffer_create(tr_scr, result);
}


static struct pipe_buffer *
trace_screen_buffer_create(struct pipe_screen *_screen,
                           unsigned alignment,
                           unsigned usage,
                           unsigned size)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *result;

   trace_dump_call_begin("pipe_screen", "buffer_create");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(uint, alignment);
   trace_dump_arg(uint, usage);
   trace_dump_arg(uint, size);

   result = screen->buffer_create(screen, alignment, usage, size);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   /* Zero the buffer to avoid dumping uninitialized memory */
   if(result->usage & PIPE_BUFFER_USAGE_CPU_WRITE) {
      void *map;
      map = pipe_buffer_map(screen, result, PIPE_BUFFER_USAGE_CPU_WRITE);
      if(map) {
         memset(map, 0, result->size);
         screen->buffer_unmap(screen, result);
      }
   }

   return trace_buffer_create(tr_scr, result);
}


static struct pipe_buffer *
trace_screen_user_buffer_create(struct pipe_screen *_screen,
                                void *data,
                                unsigned size)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *result;

   trace_dump_call_begin("pipe_screen", "user_buffer_create");

   trace_dump_arg(ptr, screen);
   trace_dump_arg_begin("data");
   trace_dump_bytes(data, size);
   trace_dump_arg_end();
   trace_dump_arg(uint, size);

   result = screen->user_buffer_create(screen, data, size);

   trace_dump_ret(ptr, result);

   trace_dump_call_end();

   if(result) {
      assert(!(result->usage & TRACE_BUFFER_USAGE_USER));
      result->usage |= TRACE_BUFFER_USAGE_USER;
   }

   return trace_buffer_create(tr_scr, result);
}


/**
 * This function is used to track if data has been changed on a user buffer
 * without map/unmap being called.
 */
void
trace_screen_user_buffer_update(struct pipe_screen *_screen,
                                struct pipe_buffer *_buffer)
{
#if 0
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   const void *map;

   if(buffer && buffer->usage & TRACE_BUFFER_USAGE_USER) {
      map = screen->buffer_map(screen, buffer, PIPE_BUFFER_USAGE_CPU_READ);
      if(map) {
         trace_dump_call_begin("pipe_winsys", "buffer_write");

         trace_dump_arg(ptr, screen);

         trace_dump_arg(ptr, buffer);

         trace_dump_arg_begin("data");
         trace_dump_bytes(map, buffer->size);
         trace_dump_arg_end();

         trace_dump_arg_begin("size");
         trace_dump_uint(buffer->size);
         trace_dump_arg_end();

         trace_dump_call_end();

         screen->buffer_unmap(screen, buffer);
      }
   }
#endif
}


static void *
trace_screen_buffer_map(struct pipe_screen *_screen,
                        struct pipe_buffer *_buffer,
                        unsigned usage)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_buffer *tr_buf = trace_buffer(_buffer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *buffer = tr_buf->buffer;
   void *map;

   assert(screen->buffer_map);
   map = screen->buffer_map(screen, buffer, usage);
   if(map) {
      if(usage & PIPE_BUFFER_USAGE_CPU_WRITE) {
         tr_buf->map = map;
      }
   }

   return map;
}


static void *
trace_screen_buffer_map_range(struct pipe_screen *_screen,
                              struct pipe_buffer *_buffer,
                              unsigned offset,
                              unsigned length,
                              unsigned usage)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_buffer *tr_buf = trace_buffer(_buffer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *buffer = tr_buf->buffer;
   void *map;

   assert(screen->buffer_map_range);
   map = screen->buffer_map_range(screen, buffer, offset, length, usage);
   if(map) {
      if(usage & PIPE_BUFFER_USAGE_CPU_WRITE) {
         tr_buf->map = map;
      }
   }

   return map;
}


static void
buffer_write(struct pipe_screen *screen,
             struct pipe_buffer *buffer,
             unsigned offset,
             const char *map,
             unsigned size)
{
   assert(map);

   trace_dump_call_begin("pipe_screen", "buffer_write");

   trace_dump_arg(ptr, screen);

   trace_dump_arg(ptr, buffer);

   trace_dump_arg(uint, offset);

   trace_dump_arg_begin("data");
   trace_dump_bytes(map + offset, size);
   trace_dump_arg_end();

   trace_dump_arg(uint, size);

   trace_dump_call_end();

}


static void
trace_screen_buffer_flush_mapped_range(struct pipe_screen *_screen,
                                       struct pipe_buffer *_buffer,
                                       unsigned offset,
                                       unsigned length)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_buffer *tr_buf = trace_buffer(_buffer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *buffer = tr_buf->buffer;

   assert(tr_buf->map);
   buffer_write(screen, buffer, offset, tr_buf->map, length);
   tr_buf->range_flushed = TRUE;
   screen->buffer_flush_mapped_range(screen, buffer, offset, length);
}


static void
trace_screen_buffer_unmap(struct pipe_screen *_screen,
                          struct pipe_buffer *_buffer)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_buffer *tr_buf = trace_buffer(_buffer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *buffer = tr_buf->buffer;

   if (tr_buf->map && !tr_buf->range_flushed)
      buffer_write(screen, buffer, 0, tr_buf->map, buffer->size);
   tr_buf->map = NULL;
   tr_buf->range_flushed = FALSE;
   screen->buffer_unmap(screen, buffer);
}


static void
trace_screen_buffer_destroy(struct pipe_buffer *_buffer)
{
   struct trace_screen *tr_scr = trace_screen(_buffer->screen);
   struct trace_buffer *tr_buf = trace_buffer(_buffer);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_buffer *buffer = tr_buf->buffer;

   trace_dump_call_begin("pipe_screen", "buffer_destroy");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, buffer);

   trace_dump_call_end();

   trace_buffer_destroy(tr_scr, _buffer);
}


/********************************************************************
 * fence
 */


static void
trace_screen_fence_reference(struct pipe_screen *_screen,
                             struct pipe_fence_handle **pdst,
                             struct pipe_fence_handle *src)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_fence_handle *dst;

   assert(pdst);
   dst = *pdst;
   
   trace_dump_call_begin("pipe_screen", "fence_reference");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(ptr, src);

   screen->fence_reference(screen, pdst, src);

   trace_dump_call_end();
}


static int
trace_screen_fence_signalled(struct pipe_screen *_screen,
                             struct pipe_fence_handle *fence,
                             unsigned flags)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   int result;

   trace_dump_call_begin("pipe_screen", "fence_signalled");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, fence);
   trace_dump_arg(uint, flags);

   result = screen->fence_signalled(screen, fence, flags);

   trace_dump_ret(int, result);

   trace_dump_call_end();

   return result;
}


static int
trace_screen_fence_finish(struct pipe_screen *_screen,
                          struct pipe_fence_handle *fence,
                          unsigned flags)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   int result;

   trace_dump_call_begin("pipe_screen", "fence_finish");

   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, fence);
   trace_dump_arg(uint, flags);

   result = screen->fence_finish(screen, fence, flags);

   trace_dump_ret(int, result);

   trace_dump_call_end();

   return result;
}


/********************************************************************
 * screen
 */

static void
trace_screen_destroy(struct pipe_screen *_screen)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;

   trace_dump_call_begin("pipe_screen", "destroy");
   trace_dump_arg(ptr, screen);
   trace_dump_call_end();
   trace_dump_trace_end();

   if (tr_scr->rbug)
      trace_rbug_stop(tr_scr->rbug);

   screen->destroy(screen);

   FREE(tr_scr);
}

boolean
trace_enabled(void)
{
   static boolean firstrun = TRUE;

   if (!firstrun)
      return trace;
   firstrun = FALSE;

   trace_dump_init();

   if(trace_dump_trace_begin()) {
      trace_dumping_start();
      trace = TRUE;
   }

   if (debug_get_bool_option("GALLIUM_RBUG", FALSE)) {
      trace = TRUE;
      rbug = TRUE;
   }

   return trace;
}

struct pipe_screen *
trace_screen_create(struct pipe_screen *screen)
{
   struct trace_screen *tr_scr;
   struct pipe_winsys *winsys;

   if(!screen)
      goto error1;

   if (!trace_enabled())
      goto error1;

   trace_dump_call_begin("", "pipe_screen_create");

   tr_scr = CALLOC_STRUCT(trace_screen);
   if(!tr_scr)
      goto error2;

#if 0
   winsys = trace_winsys_create(screen->winsys);
   if(!winsys)
      goto error3;
#else
   winsys = screen->winsys;
#endif
   pipe_mutex_init(tr_scr->list_mutex);
   make_empty_list(&tr_scr->buffers);
   make_empty_list(&tr_scr->contexts);
   make_empty_list(&tr_scr->textures);
   make_empty_list(&tr_scr->surfaces);
   make_empty_list(&tr_scr->transfers);

   tr_scr->base.winsys = winsys;
   tr_scr->base.destroy = trace_screen_destroy;
   tr_scr->base.get_name = trace_screen_get_name;
   tr_scr->base.get_vendor = trace_screen_get_vendor;
   tr_scr->base.get_param = trace_screen_get_param;
   tr_scr->base.get_paramf = trace_screen_get_paramf;
   tr_scr->base.is_format_supported = trace_screen_is_format_supported;
   assert(screen->context_create);
   tr_scr->base.context_create = trace_screen_context_create;
   tr_scr->base.texture_create = trace_screen_texture_create;
   tr_scr->base.texture_blanket = trace_screen_texture_blanket;
   tr_scr->base.texture_destroy = trace_screen_texture_destroy;
   tr_scr->base.get_tex_surface = trace_screen_get_tex_surface;
   tr_scr->base.tex_surface_destroy = trace_screen_tex_surface_destroy;
   tr_scr->base.get_tex_transfer = trace_screen_get_tex_transfer;
   tr_scr->base.tex_transfer_destroy = trace_screen_tex_transfer_destroy;
   tr_scr->base.transfer_map = trace_screen_transfer_map;
   tr_scr->base.transfer_unmap = trace_screen_transfer_unmap;
   tr_scr->base.buffer_create = trace_screen_buffer_create;
   tr_scr->base.user_buffer_create = trace_screen_user_buffer_create;
   tr_scr->base.surface_buffer_create = trace_screen_surface_buffer_create;
   if (screen->buffer_map)
      tr_scr->base.buffer_map = trace_screen_buffer_map;
   if (screen->buffer_map_range)
      tr_scr->base.buffer_map_range = trace_screen_buffer_map_range;
   if (screen->buffer_flush_mapped_range)
      tr_scr->base.buffer_flush_mapped_range = trace_screen_buffer_flush_mapped_range;
   if (screen->buffer_unmap)
      tr_scr->base.buffer_unmap = trace_screen_buffer_unmap;
   tr_scr->base.buffer_destroy = trace_screen_buffer_destroy;
   tr_scr->base.fence_reference = trace_screen_fence_reference;
   tr_scr->base.fence_signalled = trace_screen_fence_signalled;
   tr_scr->base.fence_finish = trace_screen_fence_finish;
   tr_scr->base.flush_frontbuffer = trace_screen_flush_frontbuffer;
   tr_scr->screen = screen;

   trace_dump_ret(ptr, screen);
   trace_dump_call_end();

   if (rbug)
      tr_scr->rbug = trace_rbug_start(tr_scr);

   return &tr_scr->base;

#if 0
error3:
   FREE(tr_scr);
#endif
error2:
   trace_dump_ret(ptr, screen);
   trace_dump_call_end();
   trace_dump_trace_end();
error1:
   return screen;
}


struct trace_screen *
trace_screen(struct pipe_screen *screen)
{
   assert(screen);
   assert(screen->destroy == trace_screen_destroy);
   return (struct trace_screen *)screen;
}
