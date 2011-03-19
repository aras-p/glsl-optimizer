/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * 2010 Corbin Simpson <MostAwesomeDude@gmail.com>
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
#include "util/u_math.h"

#include "glhd_public.h"
#include "glhd_screen.h"
#include "glhd_context.h"
#include "glhd_objects.h"

DEBUG_GET_ONCE_BOOL_OPTION(galahad, "GALLIUM_GALAHAD", FALSE)

static void
galahad_screen_destroy(struct pipe_screen *_screen)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   screen->destroy(screen);

   FREE(glhd_screen);
}

static const char *
galahad_screen_get_name(struct pipe_screen *_screen)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   return screen->get_name(screen);
}

static const char *
galahad_screen_get_vendor(struct pipe_screen *_screen)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   return screen->get_vendor(screen);
}

static int
galahad_screen_get_param(struct pipe_screen *_screen,
                          enum pipe_cap param)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   return screen->get_param(screen,
                            param);
}

static int
galahad_screen_get_shader_param(struct pipe_screen *_screen,
                          unsigned shader, enum pipe_shader_cap param)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   return screen->get_shader_param(screen, shader,
                            param);
}

static float
galahad_screen_get_paramf(struct pipe_screen *_screen,
                           enum pipe_cap param)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   return screen->get_paramf(screen,
                             param);
}

static boolean
galahad_screen_is_format_supported(struct pipe_screen *_screen,
                                    enum pipe_format format,
                                    enum pipe_texture_target target,
                                    unsigned sample_count,
                                    unsigned tex_usage)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   if (target >= PIPE_MAX_TEXTURE_TYPES) {
      glhd_warn("Received bogus texture target %d", target);
   }

   return screen->is_format_supported(screen,
                                      format,
                                      target,
                                      sample_count,
                                      tex_usage);
}

static struct pipe_context *
galahad_screen_context_create(struct pipe_screen *_screen,
                               void *priv)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;
   struct pipe_context *result;

   result = screen->context_create(screen, priv);
   if (result)
      return galahad_context_create(_screen, result);
   return NULL;
}

static struct pipe_resource *
galahad_screen_resource_create(struct pipe_screen *_screen,
                                const struct pipe_resource *templat)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;
   struct pipe_resource *result;

   if (templat->target >= PIPE_MAX_TEXTURE_TYPES)
      glhd_warn("Received bogus resource target %d", templat->target);

   if(templat->target != PIPE_TEXTURE_RECT && templat->target != PIPE_BUFFER && !screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES))
   {
      if(!util_is_power_of_two(templat->width0) || !util_is_power_of_two(templat->height0))
         glhd_warn("Requested NPOT (%ux%u) non-rectangle texture without NPOT support", templat->width0, templat->height0);
   }

   if(templat->target == PIPE_TEXTURE_RECT && templat->last_level)
      glhd_warn("Rectangle textures cannot have mipmaps, but last_level = %u", templat->last_level);

   if(templat->target == PIPE_BUFFER && templat->last_level)
      glhd_warn("Buffers cannot have mipmaps, but last_level = %u", templat->last_level);

   if(templat->target != PIPE_TEXTURE_3D && templat->depth0 != 1)
      glhd_warn("Only 3D textures can have depth != 1, but received target %u and depth %u", templat->target, templat->depth0);

   if(templat->target == PIPE_TEXTURE_1D && templat->height0 != 1)
     glhd_warn("1D textures must have height 1 but got asked for height %u", templat->height0);

   if(templat->target == PIPE_BUFFER && templat->height0 != 1)
     glhd_warn("Buffers must have height 1 but got asked for height %u", templat->height0);

   if(templat->target == PIPE_TEXTURE_CUBE && templat->width0 != templat->height0)
      glhd_warn("Cube maps must be square, but got asked for %ux%u", templat->width0, templat->height0);

   result = screen->resource_create(screen,
                                    templat);

   if (result)
      return galahad_resource_create(glhd_screen, result);
   return NULL;
}

static struct pipe_resource *
galahad_screen_resource_from_handle(struct pipe_screen *_screen,
                                     const struct pipe_resource *templ,
                                     struct winsys_handle *handle)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;
   struct pipe_resource *result;

   /* TODO trace call */

   result = screen->resource_from_handle(screen, templ, handle);

   result = galahad_resource_create(galahad_screen(_screen), result);

   return result;
}

static boolean
galahad_screen_resource_get_handle(struct pipe_screen *_screen,
                                    struct pipe_resource *_resource,
                                    struct winsys_handle *handle)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct galahad_resource *glhd_resource = galahad_resource(_resource);
   struct pipe_screen *screen = glhd_screen->screen;
   struct pipe_resource *resource = glhd_resource->resource;

   /* TODO trace call */

   return screen->resource_get_handle(screen, resource, handle);
}



static void
galahad_screen_resource_destroy(struct pipe_screen *screen,
                                 struct pipe_resource *_resource)
{
   galahad_resource_destroy(galahad_resource(_resource));
}


static struct pipe_resource *
galahad_screen_user_buffer_create(struct pipe_screen *_screen,
                                   void *ptr,
                                   unsigned bytes,
                                   unsigned usage)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;
   struct pipe_resource *result;

   result = screen->user_buffer_create(screen,
                                       ptr,
                                       bytes,
                                       usage);

   if (result)
      return galahad_resource_create(glhd_screen, result);
   return NULL;
}



static void
galahad_screen_flush_frontbuffer(struct pipe_screen *_screen,
                                  struct pipe_resource *_resource,
                                  unsigned level, unsigned layer,
                                  void *context_private)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct galahad_resource *glhd_resource = galahad_resource(_resource);
   struct pipe_screen *screen = glhd_screen->screen;
   struct pipe_resource *resource = glhd_resource->resource;

   screen->flush_frontbuffer(screen,
                             resource,
                             level, layer,
                             context_private);
}

static void
galahad_screen_fence_reference(struct pipe_screen *_screen,
                                struct pipe_fence_handle **ptr,
                                struct pipe_fence_handle *fence)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   screen->fence_reference(screen,
                           ptr,
                           fence);
}

static boolean
galahad_screen_fence_signalled(struct pipe_screen *_screen,
                                struct pipe_fence_handle *fence)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   return screen->fence_signalled(screen,
                                  fence);
}

static boolean
galahad_screen_fence_finish(struct pipe_screen *_screen,
                             struct pipe_fence_handle *fence,
                             uint64_t timeout)
{
   struct galahad_screen *glhd_screen = galahad_screen(_screen);
   struct pipe_screen *screen = glhd_screen->screen;

   return screen->fence_finish(screen,
                               fence,
                               timeout);
}

struct pipe_screen *
galahad_screen_create(struct pipe_screen *screen)
{
   struct galahad_screen *glhd_screen;

   if (!debug_get_option_galahad())
      return screen;

   glhd_screen = CALLOC_STRUCT(galahad_screen);
   if (!glhd_screen) {
      return screen;
   }

   glhd_screen->base.winsys = NULL;

   glhd_screen->base.destroy = galahad_screen_destroy;
   glhd_screen->base.get_name = galahad_screen_get_name;
   glhd_screen->base.get_vendor = galahad_screen_get_vendor;
   glhd_screen->base.get_param = galahad_screen_get_param;
   glhd_screen->base.get_shader_param = galahad_screen_get_shader_param;
   glhd_screen->base.get_paramf = galahad_screen_get_paramf;
   glhd_screen->base.is_format_supported = galahad_screen_is_format_supported;
   glhd_screen->base.context_create = galahad_screen_context_create;
   glhd_screen->base.resource_create = galahad_screen_resource_create;
   glhd_screen->base.resource_from_handle = galahad_screen_resource_from_handle;
   glhd_screen->base.resource_get_handle = galahad_screen_resource_get_handle;
   glhd_screen->base.resource_destroy = galahad_screen_resource_destroy;
   glhd_screen->base.user_buffer_create = galahad_screen_user_buffer_create;
   glhd_screen->base.flush_frontbuffer = galahad_screen_flush_frontbuffer;
   glhd_screen->base.fence_reference = galahad_screen_fence_reference;
   glhd_screen->base.fence_signalled = galahad_screen_fence_signalled;
   glhd_screen->base.fence_finish = galahad_screen_fence_finish;

   glhd_screen->screen = screen;

   glhd_warn("Created screen %p", (void *) glhd_screen);

   return &glhd_screen->base;
}
