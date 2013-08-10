/*
 * Copyright 2011-2013 Maarten Lankhorst
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nouveau_screen.h"
#include "nouveau_context.h"
#include "nouveau_vp3_video.h"

#include "util/u_video.h"
#include "util/u_format.h"
#include "util/u_sampler.h"

static struct pipe_sampler_view **
nouveau_vp3_video_buffer_sampler_view_planes(struct pipe_video_buffer *buffer)
{
   struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
   return buf->sampler_view_planes;
}

static struct pipe_sampler_view **
nouveau_vp3_video_buffer_sampler_view_components(struct pipe_video_buffer *buffer)
{
   struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
   return buf->sampler_view_components;
}

static struct pipe_surface **
nouveau_vp3_video_buffer_surfaces(struct pipe_video_buffer *buffer)
{
   struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
   return buf->surfaces;
}

static void
nouveau_vp3_video_buffer_destroy(struct pipe_video_buffer *buffer)
{
   struct nouveau_vp3_video_buffer *buf = (struct nouveau_vp3_video_buffer *)buffer;
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
nouveau_vp3_video_buffer_create(struct pipe_context *pipe,
                         const struct pipe_video_buffer *templat,
                         int flags)
{
   struct nouveau_vp3_video_buffer *buffer;
   struct pipe_resource templ;
   unsigned i, j, component;
   struct pipe_sampler_view sv_templ;
   struct pipe_surface surf_templ;

   assert(templat->interlaced);
   if (getenv("XVMC_VL") || templat->buffer_format != PIPE_FORMAT_NV12)
      return vl_video_buffer_create(pipe, templat);

   assert(templat->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   buffer = CALLOC_STRUCT(nouveau_vp3_video_buffer);
   if (!buffer)
      return NULL;

   buffer->base.buffer_format = templat->buffer_format;
   buffer->base.context = pipe;
   buffer->base.destroy = nouveau_vp3_video_buffer_destroy;
   buffer->base.chroma_format = templat->chroma_format;
   buffer->base.width = templat->width;
   buffer->base.height = templat->height;
   buffer->base.get_sampler_view_planes = nouveau_vp3_video_buffer_sampler_view_planes;
   buffer->base.get_sampler_view_components = nouveau_vp3_video_buffer_sampler_view_components;
   buffer->base.get_surfaces = nouveau_vp3_video_buffer_surfaces;
   buffer->base.interlaced = true;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D_ARRAY;
   templ.depth0 = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ.format = PIPE_FORMAT_R8_UNORM;
   templ.width0 = buffer->base.width;
   templ.height0 = (buffer->base.height + 1)/2;
   templ.flags = flags;
   templ.array_size = 2;

   buffer->resources[0] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[0])
      goto error;

   templ.format = PIPE_FORMAT_R8G8_UNORM;
   buffer->num_planes = 2;
   templ.width0 = (templ.width0 + 1) / 2;
   templ.height0 = (templ.height0 + 1) / 2;
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
   nouveau_vp3_video_buffer_destroy(&buffer->base);
   return NULL;
}

static void
nouveau_vp3_decoder_flush(struct pipe_video_decoder *decoder)
{
}

static void
nouveau_vp3_decoder_begin_frame(struct pipe_video_decoder *decoder,
                                struct pipe_video_buffer *target,
                                struct pipe_picture_desc *picture)
{
}

static void
nouveau_vp3_decoder_end_frame(struct pipe_video_decoder *decoder,
                              struct pipe_video_buffer *target,
                              struct pipe_picture_desc *picture)
{
}

static void
nouveau_vp3_decoder_destroy(struct pipe_video_decoder *decoder)
{
   struct nouveau_vp3_decoder *dec = (struct nouveau_vp3_decoder *)decoder;
   int i;

   nouveau_bo_ref(NULL, &dec->ref_bo);
   nouveau_bo_ref(NULL, &dec->bitplane_bo);
   nouveau_bo_ref(NULL, &dec->inter_bo[0]);
   nouveau_bo_ref(NULL, &dec->inter_bo[1]);
#if NOUVEAU_VP3_DEBUG_FENCE
   nouveau_bo_ref(NULL, &dec->fence_bo);
#endif
   nouveau_bo_ref(NULL, &dec->fw_bo);

   for (i = 0; i < NOUVEAU_VP3_VIDEO_QDEPTH; ++i)
      nouveau_bo_ref(NULL, &dec->bsp_bo[i]);

   nouveau_object_del(&dec->bsp);
   nouveau_object_del(&dec->vp);
   nouveau_object_del(&dec->ppp);

   if (dec->channel[0] != dec->channel[1]) {
      for (i = 0; i < 3; ++i) {
         nouveau_pushbuf_del(&dec->pushbuf[i]);
         nouveau_object_del(&dec->channel[i]);
      }
   } else {
      nouveau_pushbuf_del(dec->pushbuf);
      nouveau_object_del(dec->channel);
   }

   FREE(dec);
}

void
nouveau_vp3_decoder_init_common(struct pipe_video_decoder *dec)
{
   dec->destroy = nouveau_vp3_decoder_destroy;
   dec->flush = nouveau_vp3_decoder_flush;
   dec->begin_frame = nouveau_vp3_decoder_begin_frame;
   dec->end_frame = nouveau_vp3_decoder_end_frame;
}
