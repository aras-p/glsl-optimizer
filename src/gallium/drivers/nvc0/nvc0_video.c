/*
 * Copyright 2011 Maarten Lankhorst
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nvc0_video.h"

#include "util/u_sampler.h"
#include "util/u_format.h"

#include <sys/mman.h>
#include <fcntl.h>

int
nvc0_screen_get_video_param(struct pipe_screen *pscreen,
                            enum pipe_video_profile profile,
                            enum pipe_video_cap param)
{
   switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
      return profile >= PIPE_VIDEO_PROFILE_MPEG1;
   case PIPE_VIDEO_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
      return nouveau_screen(pscreen)->device->chipset < 0xd0 ? 2048 : 4096;
   case PIPE_VIDEO_CAP_PREFERED_FORMAT:
      return PIPE_FORMAT_NV12;
   case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
   case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
      return true;
   case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
      return false;
   default:
      debug_printf("unknown video param: %d\n", param);
      return 0;
   }
}

struct pipe_video_decoder *
nvc0_create_decoder(struct pipe_context *context,
                    enum pipe_video_profile profile,
                    enum pipe_video_entrypoint entrypoint,
                    enum pipe_video_chroma_format chroma_format,
                    unsigned width, unsigned height, unsigned max_references,
                    bool chunked_decode)
{
   if (getenv("XVMC_VL"))
       return vl_create_decoder(context, profile, entrypoint,
                                chroma_format, width, height,
                                max_references, chunked_decode);

   if (entrypoint != PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      debug_printf("%x\n", entrypoint);
      return NULL;
   }

   return NULL;
}

static struct pipe_sampler_view **
nvc0_video_buffer_sampler_view_planes(struct pipe_video_buffer *buffer)
{
   struct nvc0_video_buffer *buf = (struct nvc0_video_buffer *)buffer;
   return buf->sampler_view_planes;
}

static struct pipe_sampler_view **
nvc0_video_buffer_sampler_view_components(struct pipe_video_buffer *buffer)
{
   struct nvc0_video_buffer *buf = (struct nvc0_video_buffer *)buffer;
   return buf->sampler_view_components;
}

static struct pipe_surface **
nvc0_video_buffer_surfaces(struct pipe_video_buffer *buffer)
{
   struct nvc0_video_buffer *buf = (struct nvc0_video_buffer *)buffer;
   return buf->surfaces;
}

static void
nvc0_video_buffer_destroy(struct pipe_video_buffer *buffer)
{
   struct nvc0_video_buffer *buf = (struct nvc0_video_buffer *)buffer;
   unsigned i;

   assert(buf);

   for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      pipe_resource_reference(&buf->resources[i], NULL);
      pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);
      pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);
      pipe_surface_reference(&buf->surfaces[i * 2], NULL);
      pipe_surface_reference(&buf->surfaces[i * 2 + 1], NULL);
   }
   FREE(buffer);
}

struct pipe_video_buffer *
nvc0_video_buffer_create(struct pipe_context *pipe,
                         const struct pipe_video_buffer *templat)
{
   struct nvc0_video_buffer *buffer;
   struct pipe_resource templ;
   unsigned i, j, component;
   struct pipe_sampler_view sv_templ;
   struct pipe_surface surf_templ;

   assert(templat->interlaced);
   if (getenv("XVMC_VL") || templat->buffer_format != PIPE_FORMAT_NV12)
      return vl_video_buffer_create(pipe, templat);

   assert(templat->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   buffer = CALLOC_STRUCT(nvc0_video_buffer);
   if (!buffer)
      return NULL;
   assert(!(templat->height % 4));
   assert(!(templat->width % 2));

   buffer->base.buffer_format = templat->buffer_format;
   buffer->base.context = pipe;
   buffer->base.destroy = nvc0_video_buffer_destroy;
   buffer->base.chroma_format = templat->chroma_format;
   buffer->base.width = templat->width;
   buffer->base.height = templat->height;
   buffer->base.get_sampler_view_planes = nvc0_video_buffer_sampler_view_planes;
   buffer->base.get_sampler_view_components = nvc0_video_buffer_sampler_view_components;
   buffer->base.get_surfaces = nvc0_video_buffer_surfaces;
   buffer->base.interlaced = true;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_3D;
   templ.depth0 = 2;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ.format = PIPE_FORMAT_R8_UNORM;
   templ.width0 = buffer->base.width;
   templ.height0 = buffer->base.height/2;
   templ.flags = NVC0_RESOURCE_FLAG_VIDEO;
   templ.last_level = 0;
   templ.array_size = 1;

   buffer->resources[0] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[0])
      goto error;

   templ.format = PIPE_FORMAT_R8G8_UNORM;
   buffer->num_planes = 2;
   templ.width0 /= 2;
   templ.height0 /= 2;
   for (i = 1; i < buffer->num_planes; ++i) {
      buffer->resources[i] = pipe->screen->resource_create(pipe->screen, &templ);
      if (!buffer->resources[i])
         goto error;
   }

   memset(&sv_templ, 0, sizeof(sv_templ));
   for (component = 0, i = 0; i < buffer->num_planes; ++i ) {
      struct pipe_resource *res = buffer->resources[i];
      unsigned nr_components = util_format_get_nr_components(res->format);

      u_sampler_view_default_template(&sv_templ, res, res->format);
      buffer->sampler_view_planes[i] = pipe->create_sampler_view(pipe, res, &sv_templ);
      if (!buffer->sampler_view_planes[i])
         goto error;

      for (j = 0; j < nr_components; ++j, ++component) {
         sv_templ.swizzle_r = sv_templ.swizzle_g = sv_templ.swizzle_b = PIPE_SWIZZLE_RED + j;
         sv_templ.swizzle_a = PIPE_SWIZZLE_ONE;

         buffer->sampler_view_components[component] = pipe->create_sampler_view(pipe, res, &sv_templ);
         if (!buffer->sampler_view_components[component])
            goto error;
      }
  }

   memset(&surf_templ, 0, sizeof(surf_templ));
   for (j = 0; j < buffer->num_planes; ++j) {
      surf_templ.format = buffer->resources[j]->format;
      surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = 0;
      buffer->surfaces[j * 2] = pipe->create_surface(pipe, buffer->resources[j], &surf_templ);
      if (!buffer->surfaces[j * 2])
         goto error;

      surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = 1;
      buffer->surfaces[j * 2 + 1] = pipe->create_surface(pipe, buffer->resources[j], &surf_templ);
      if (!buffer->surfaces[j * 2 + 1])
         goto error;
   }

   return &buffer->base;

error:
   nvc0_video_buffer_destroy(&buffer->base);
   return NULL;
}
