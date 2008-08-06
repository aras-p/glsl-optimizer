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

#include "pipe/p_util.h"

#include "tr_stream.h"
#include "tr_dump.h"
#include "tr_state.h"
#include "tr_screen.h"


static const char *
trace_screen_get_name(struct pipe_screen *_screen)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   const char *result;
   
   trace_dump_call_begin(stream, "pipe_screen", "get_name");
   
   trace_dump_arg(stream, ptr, screen);

   result = screen->get_name(screen);
   
   trace_dump_ret(stream, string, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static const char *
trace_screen_get_vendor(struct pipe_screen *_screen)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   const char *result;
   
   trace_dump_call_begin(stream, "pipe_screen", "get_vendor");
   
   trace_dump_arg(stream, ptr, screen);
  
   result = screen->get_vendor(screen);
   
   trace_dump_ret(stream, string, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static int 
trace_screen_get_param(struct pipe_screen *_screen, 
                       int param)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   int result;
   
   trace_dump_call_begin(stream, "pipe_screen", "get_param");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, int, param);

   result = screen->get_param(screen, param);
   
   trace_dump_ret(stream, int, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static float 
trace_screen_get_paramf(struct pipe_screen *_screen, 
                        int param)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   float result;
   
   trace_dump_call_begin(stream, "pipe_screen", "get_paramf");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, int, param);

   result = screen->get_paramf(screen, param);
   
   trace_dump_ret(stream, float, result);
   
   trace_dump_call_end(stream);
   
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
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   boolean result;
   
   trace_dump_call_begin(stream, "pipe_screen", "is_format_supported");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, int, format);
   trace_dump_arg(stream, int, target);
   trace_dump_arg(stream, uint, tex_usage);
   trace_dump_arg(stream, uint, geom_flags);

   result = screen->is_format_supported(screen, format, target, tex_usage, geom_flags);
   
   trace_dump_ret(stream, bool, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static struct pipe_texture *
trace_screen_texture_create(struct pipe_screen *_screen,
                            const struct pipe_texture *templat)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_texture *result;
   
   trace_dump_call_begin(stream, "pipe_screen", "texture_create");

   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, template, templat);

   result = screen->texture_create(screen, templat);
   
   trace_dump_ret(stream, ptr, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static struct pipe_texture *
trace_screen_texture_blanket(struct pipe_screen *_screen,
                             const struct pipe_texture *templat,
                             const unsigned *ppitch,
                             struct pipe_buffer *buffer)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   unsigned pitch = *ppitch;
   struct pipe_texture *result;

   trace_dump_call_begin(stream, "pipe_screen", "texture_blanket");

   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, template, templat);
   trace_dump_arg(stream, uint, pitch);
   trace_dump_arg(stream, ptr, buffer);

   result = screen->texture_blanket(screen, templat, ppitch, buffer);
   
   trace_dump_ret(stream, ptr, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static void 
trace_screen_texture_release(struct pipe_screen *_screen,
                             struct pipe_texture **ptexture)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_texture *texture = *ptexture;
   
   trace_dump_call_begin(stream, "pipe_screen", "texture_release");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, ptr, texture);

   screen->texture_release(screen, ptexture);
   
   trace_dump_call_end(stream);
}

static struct pipe_surface *
trace_screen_get_tex_surface(struct pipe_screen *_screen,
                             struct pipe_texture *texture,
                             unsigned face, unsigned level,
                             unsigned zslice,
                             unsigned usage)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_surface *result;
   
   trace_dump_call_begin(stream, "pipe_screen", "get_tex_surface");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, ptr, texture);
   trace_dump_arg(stream, uint, face);
   trace_dump_arg(stream, uint, level);
   trace_dump_arg(stream, uint, zslice);
   trace_dump_arg(stream, uint, usage);

   result = screen->get_tex_surface(screen, texture, face, level, zslice, usage);

   trace_dump_ret(stream, ptr, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static void 
trace_screen_tex_surface_release(struct pipe_screen *_screen,
                                 struct pipe_surface **psurface)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_surface *surface = *psurface;
   
   trace_dump_call_begin(stream, "pipe_screen", "tex_surface_release");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, ptr, surface);

   screen->tex_surface_release(screen, psurface);
   
   trace_dump_call_end(stream);
}


static void *
trace_screen_surface_map(struct pipe_screen *_screen,
                         struct pipe_surface *surface,
                         unsigned flags)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_surface *result;
   
   trace_dump_call_begin(stream, "pipe_screen", "surface_map");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, ptr, surface);
   trace_dump_arg(stream, uint, flags);

   result = screen->surface_map(screen, surface, flags);
   
   trace_dump_ret(stream, ptr, result);
   
   trace_dump_call_end(stream);
   
   return result;
}


static void 
trace_screen_surface_unmap(struct pipe_screen *_screen,
                           struct pipe_surface *surface)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   
   trace_dump_call_begin(stream, "pipe_screen", "surface_unmap");
   
   trace_dump_arg(stream, ptr, screen);
   trace_dump_arg(stream, ptr, surface);

   screen->surface_unmap(screen, surface);
   
   trace_dump_call_end(stream);
}


static void
trace_screen_destroy(struct pipe_screen *_screen)
{
   struct trace_screen *tr_scr = trace_screen(_screen);
   struct trace_stream *stream = tr_scr->stream;
   struct pipe_screen *screen = tr_scr->screen;
   
   trace_dump_call_begin(stream, "pipe_screen", "destroy");
   
   trace_dump_arg(stream, ptr, screen);

   screen->destroy(screen);
   
   trace_dump_call_end(stream);
   
   trace_dump_trace_end(stream);
   
   trace_stream_close(tr_scr->stream);
   
   FREE(tr_scr);
}


struct pipe_screen *
trace_screen_create(struct pipe_screen *screen)
{
   struct trace_screen *tr_scr;
   
   if(!debug_get_bool_option("GALLIUM_TRACE", FALSE))
      return screen;
   
   tr_scr = CALLOC_STRUCT(trace_screen);
   if(!tr_scr)
      return NULL;

   tr_scr->base.winsys = screen->winsys;
   tr_scr->base.destroy = trace_screen_destroy;
   tr_scr->base.get_name = trace_screen_get_name;
   tr_scr->base.get_vendor = trace_screen_get_vendor;
   tr_scr->base.get_param = trace_screen_get_param;
   tr_scr->base.get_paramf = trace_screen_get_paramf;
   tr_scr->base.is_format_supported = trace_screen_is_format_supported;
   tr_scr->base.texture_create = trace_screen_texture_create;
   tr_scr->base.texture_blanket = trace_screen_texture_blanket;
   tr_scr->base.texture_release = trace_screen_texture_release;
   tr_scr->base.get_tex_surface = trace_screen_get_tex_surface;
   tr_scr->base.tex_surface_release = trace_screen_tex_surface_release;
   tr_scr->base.surface_map = trace_screen_surface_map;
   tr_scr->base.surface_unmap = trace_screen_surface_unmap;
   
   tr_scr->screen = screen;
      
   tr_scr->stream = trace_stream_create("gallium", "trace");
   if(!tr_scr->stream)
      return NULL;

   trace_dump_trace_begin(tr_scr->stream, 0);
   
   return &tr_scr->base;
}
