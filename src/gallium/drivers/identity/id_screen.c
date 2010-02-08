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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_memory.h"

#include "id_public.h"
#include "id_screen.h"
#include "id_context.h"
#include "id_objects.h"


static void
identity_screen_destroy(struct pipe_screen *_screen)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   screen->destroy(screen);

   FREE(id_screen);
}

static const char *
identity_screen_get_name(struct pipe_screen *_screen)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   return screen->get_name(screen);
}

static const char *
identity_screen_get_vendor(struct pipe_screen *_screen)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   return screen->get_vendor(screen);
}

static int
identity_screen_get_param(struct pipe_screen *_screen,
                          int param)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   return screen->get_param(screen,
                            param);
}

static float
identity_screen_get_paramf(struct pipe_screen *_screen,
                           int param)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   return screen->get_paramf(screen,
                             param);
}

static boolean
identity_screen_is_format_supported(struct pipe_screen *_screen,
                                    enum pipe_format format,
                                    enum pipe_texture_target target,
                                    unsigned tex_usage,
                                    unsigned geom_flags)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   return screen->is_format_supported(screen,
                                      format,
                                      target,
                                      tex_usage,
                                      geom_flags);
}

static struct pipe_context *
identity_screen_context_create(struct pipe_screen *_screen,
                               void *priv)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_context *result;

   result = screen->context_create(screen, priv);
   if (result)
      return identity_context_create(_screen, result);
   return NULL;
}

static struct pipe_texture *
identity_screen_texture_create(struct pipe_screen *_screen,
                               const struct pipe_texture *templat)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_texture *result;

   result = screen->texture_create(screen,
                                   templat);

   if (result)
      return identity_texture_create(id_screen, result);
   return NULL;
}

static struct pipe_texture *
identity_screen_texture_blanket(struct pipe_screen *_screen,
                                const struct pipe_texture *templat,
                                const unsigned *stride,
                                struct pipe_buffer *_buffer)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_buffer *id_buffer = identity_buffer(_buffer);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *buffer = id_buffer->buffer;
   struct pipe_texture *result;

   result = screen->texture_blanket(screen,
                                    templat,
                                    stride,
                                    buffer);

   if (result)
      return identity_texture_create(id_screen, result);
   return NULL;
}

static void
identity_screen_texture_destroy(struct pipe_texture *_texture)
{
   identity_texture_destroy(identity_texture(_texture));
}

static struct pipe_surface *
identity_screen_get_tex_surface(struct pipe_screen *_screen,
                                struct pipe_texture *_texture,
                                unsigned face,
                                unsigned level,
                                unsigned zslice,
                                unsigned usage)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_texture *id_texture = identity_texture(_texture);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_texture *texture = id_texture->texture;
   struct pipe_surface *result;

   result = screen->get_tex_surface(screen,
                                    texture,
                                    face,
                                    level,
                                    zslice,
                                    usage);

   if (result)
      return identity_surface_create(id_texture, result);
   return NULL;
}

static void
identity_screen_tex_surface_destroy(struct pipe_surface *_surface)
{
   identity_surface_destroy(identity_surface(_surface));
}

static struct pipe_transfer *
identity_screen_get_tex_transfer(struct pipe_screen *_screen,
                                 struct pipe_texture *_texture,
                                 unsigned face,
                                 unsigned level,
                                 unsigned zslice,
                                 enum pipe_transfer_usage usage,
                                 unsigned x,
                                 unsigned y,
                                 unsigned w,
                                 unsigned h)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_texture *id_texture = identity_texture(_texture);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_texture *texture = id_texture->texture;
   struct pipe_transfer *result;

   result = screen->get_tex_transfer(screen,
                                     texture,
                                     face,
                                     level,
                                     zslice,
                                     usage,
                                     x,
                                     y,
                                     w,
                                     h);

   if (result)
      return identity_transfer_create(id_texture, result);
   return NULL;
}

static void
identity_screen_tex_transfer_destroy(struct pipe_transfer *_transfer)
{
   identity_transfer_destroy(identity_transfer(_transfer));
}

static void *
identity_screen_transfer_map(struct pipe_screen *_screen,
                             struct pipe_transfer *_transfer)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_transfer *id_transfer = identity_transfer(_transfer);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_transfer *transfer = id_transfer->transfer;

   return screen->transfer_map(screen,
                               transfer);
}

static void
identity_screen_transfer_unmap(struct pipe_screen *_screen,
                               struct pipe_transfer *_transfer)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_transfer *id_transfer = identity_transfer(_transfer);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_transfer *transfer = id_transfer->transfer;

   screen->transfer_unmap(screen,
                          transfer);
}

static struct pipe_buffer *
identity_screen_buffer_create(struct pipe_screen *_screen,
                              unsigned alignment,
                              unsigned usage,
                              unsigned size)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *result;

   result = screen->buffer_create(screen,
                                  alignment,
                                  usage,
                                  size);

   if (result)
      return identity_buffer_create(id_screen, result);
   return NULL;
}

static struct pipe_buffer *
identity_screen_user_buffer_create(struct pipe_screen *_screen,
                                   void *ptr,
                                   unsigned bytes)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *result;

   result = screen->user_buffer_create(screen,
                                       ptr,
                                       bytes);

   if (result)
      return identity_buffer_create(id_screen, result);
   return NULL;
}

static struct pipe_buffer *
identity_screen_surface_buffer_create(struct pipe_screen *_screen,
                                      unsigned width,
                                      unsigned height,
                                      enum pipe_format format,
                                      unsigned usage,
                                      unsigned tex_usage,
                                      unsigned *stride)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *result;

   result = screen->surface_buffer_create(screen,
                                          width,
                                          height,
                                          format,
                                          usage,
                                          tex_usage,
                                          stride);

   if (result)
      return identity_buffer_create(id_screen, result);
   return NULL;
}

static void *
identity_screen_buffer_map(struct pipe_screen *_screen,
                           struct pipe_buffer *_buffer,
                           unsigned usage)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_buffer *id_buffer = identity_buffer(_buffer);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *buffer = id_buffer->buffer;

   return screen->buffer_map(screen,
                             buffer,
                             usage);
}

static void *
identity_screen_buffer_map_range(struct pipe_screen *_screen,
                                 struct pipe_buffer *_buffer,
                                 unsigned offset,
                                 unsigned length,
                                 unsigned usage)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_buffer *id_buffer = identity_buffer(_buffer);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *buffer = id_buffer->buffer;

   return screen->buffer_map_range(screen,
                                   buffer,
                                   offset,
                                   length,
                                   usage);
}

static void
identity_screen_buffer_flush_mapped_range(struct pipe_screen *_screen,
                                          struct pipe_buffer *_buffer,
                                          unsigned offset,
                                          unsigned length)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_buffer *id_buffer = identity_buffer(_buffer);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *buffer = id_buffer->buffer;

   screen->buffer_flush_mapped_range(screen,
                                     buffer,
                                     offset,
                                     length);
}

static void
identity_screen_buffer_unmap(struct pipe_screen *_screen,
                             struct pipe_buffer *_buffer)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_buffer *id_buffer = identity_buffer(_buffer);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_buffer *buffer = id_buffer->buffer;

   screen->buffer_unmap(screen,
                        buffer);
}

static void
identity_screen_buffer_destroy(struct pipe_buffer *_buffer)
{
   identity_buffer_destroy(identity_buffer(_buffer));
}

static struct pipe_video_surface *
identity_screen_video_surface_create(struct pipe_screen *_screen,
                                     enum pipe_video_chroma_format chroma_format,
                                     unsigned width,
                                     unsigned height)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_video_surface *result;

   result = screen->video_surface_create(screen,
                                         chroma_format,
                                         width,
                                         height);

   if (result) {
      return identity_video_surface_create(id_screen, result);
   }
   return NULL;
}

static void
identity_screen_video_surface_destroy(struct pipe_video_surface *_vsfc)
{
   identity_video_surface_destroy(identity_video_surface(_vsfc));
}

static void
identity_screen_flush_frontbuffer(struct pipe_screen *_screen,
                                  struct pipe_surface *_surface,
                                  void *context_private)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct identity_surface *id_surface = identity_surface(_surface);
   struct pipe_screen *screen = id_screen->screen;
   struct pipe_surface *surface = id_surface->surface;

   screen->flush_frontbuffer(screen,
                             surface,
                             context_private);
}

static void
identity_screen_fence_reference(struct pipe_screen *_screen,
                                struct pipe_fence_handle **ptr,
                                struct pipe_fence_handle *fence)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   screen->fence_reference(screen,
                           ptr,
                           fence);
}

static int
identity_screen_fence_signalled(struct pipe_screen *_screen,
                                struct pipe_fence_handle *fence,
                                unsigned flags)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   return screen->fence_signalled(screen,
                                  fence,
                                  flags);
}

static int
identity_screen_fence_finish(struct pipe_screen *_screen,
                             struct pipe_fence_handle *fence,
                             unsigned flags)
{
   struct identity_screen *id_screen = identity_screen(_screen);
   struct pipe_screen *screen = id_screen->screen;

   return screen->fence_finish(screen,
                               fence,
                               flags);
}

struct pipe_screen *
identity_screen_create(struct pipe_screen *screen)
{
   struct identity_screen *id_screen;

   id_screen = CALLOC_STRUCT(identity_screen);
   if (!id_screen) {
      return NULL;
   }

   id_screen->base.winsys = NULL;

   id_screen->base.destroy = identity_screen_destroy;
   id_screen->base.get_name = identity_screen_get_name;
   id_screen->base.get_vendor = identity_screen_get_vendor;
   id_screen->base.get_param = identity_screen_get_param;
   id_screen->base.get_paramf = identity_screen_get_paramf;
   id_screen->base.is_format_supported = identity_screen_is_format_supported;
   id_screen->base.context_create = identity_screen_context_create;
   id_screen->base.texture_create = identity_screen_texture_create;
   id_screen->base.texture_blanket = identity_screen_texture_blanket;
   id_screen->base.texture_destroy = identity_screen_texture_destroy;
   id_screen->base.get_tex_surface = identity_screen_get_tex_surface;
   id_screen->base.tex_surface_destroy = identity_screen_tex_surface_destroy;
   id_screen->base.get_tex_transfer = identity_screen_get_tex_transfer;
   id_screen->base.tex_transfer_destroy = identity_screen_tex_transfer_destroy;
   id_screen->base.transfer_map = identity_screen_transfer_map;
   id_screen->base.transfer_unmap = identity_screen_transfer_unmap;
   id_screen->base.buffer_create = identity_screen_buffer_create;
   id_screen->base.user_buffer_create = identity_screen_user_buffer_create;
   id_screen->base.surface_buffer_create = identity_screen_surface_buffer_create;
   if (screen->buffer_map)
      id_screen->base.buffer_map = identity_screen_buffer_map;
   if (screen->buffer_map_range)
      id_screen->base.buffer_map_range = identity_screen_buffer_map_range;
   if (screen->buffer_flush_mapped_range)
      id_screen->base.buffer_flush_mapped_range = identity_screen_buffer_flush_mapped_range;
   if (screen->buffer_unmap)
      id_screen->base.buffer_unmap = identity_screen_buffer_unmap;
   id_screen->base.buffer_destroy = identity_screen_buffer_destroy;
   if (screen->video_surface_create) {
      id_screen->base.video_surface_create = identity_screen_video_surface_create;
   }
   if (screen->video_surface_destroy) {
      id_screen->base.video_surface_destroy = identity_screen_video_surface_destroy;
   }
   id_screen->base.flush_frontbuffer = identity_screen_flush_frontbuffer;
   id_screen->base.fence_reference = identity_screen_fence_reference;
   id_screen->base.fence_signalled = identity_screen_fence_signalled;
   id_screen->base.fence_finish = identity_screen_fence_finish;

   id_screen->screen = screen;

   return &id_screen->base;
}
