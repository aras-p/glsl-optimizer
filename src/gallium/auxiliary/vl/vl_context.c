/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <pipe/p_video_context.h>

#include <util/u_memory.h>
#include <util/u_rect.h>
#include <util/u_video.h>

#include "vl_context.h"
#include "vl_compositor.h"
#include "vl_mpeg12_decoder.h"

static void
vl_context_destroy(struct pipe_video_context *context)
{
   struct vl_context *ctx = (struct vl_context*)context;

   assert(context);

   ctx->pipe->destroy(ctx->pipe);

   FREE(ctx);
}

static int
vl_context_get_param(struct pipe_video_context *context, int param)
{
   struct vl_context *ctx = (struct vl_context*)context;

   assert(context);

   if (param == PIPE_CAP_NPOT_TEXTURES)
      return !ctx->pot_buffers;

   debug_printf("vl_context: Unknown PIPE_CAP %d\n", param);
   return 0;
}

static boolean
vl_context_is_format_supported(struct pipe_video_context *context,
                               enum pipe_format format,
                               unsigned usage)
{
   struct vl_context *ctx = (struct vl_context*)context;

   assert(context);

   return ctx->pipe->screen->is_format_supported(ctx->pipe->screen, format,
                                                 PIPE_TEXTURE_2D,
                                                 0, usage);
}

static struct pipe_surface *
vl_context_create_surface(struct pipe_video_context *context,
                          struct pipe_resource *resource,
                          const struct pipe_surface *templ)
{
   struct vl_context *ctx = (struct vl_context*)context;

   assert(ctx);

   return ctx->pipe->create_surface(ctx->pipe, resource, templ);
}

static struct pipe_sampler_view *
vl_context_create_sampler_view(struct pipe_video_context *context,
                               struct pipe_resource *resource,
                               const struct pipe_sampler_view *templ)
{
   struct vl_context *ctx = (struct vl_context*)context;

   assert(ctx);

   return ctx->pipe->create_sampler_view(ctx->pipe, resource, templ);
}

static void
vl_context_upload_sampler(struct pipe_video_context *context,
                          struct pipe_sampler_view *dst,
                          const struct pipe_box *dst_box,
                          const void *src, unsigned src_stride,
                          unsigned src_x, unsigned src_y)
{
   struct vl_context *ctx = (struct vl_context*)context;
   struct pipe_transfer *transfer;
   void *map;

   assert(context);
   assert(dst);
   assert(dst_box);
   assert(src);

   transfer = ctx->pipe->get_transfer(ctx->pipe, dst->texture, 0, PIPE_TRANSFER_WRITE, dst_box);
   if (!transfer)
      return;

   map = ctx->pipe->transfer_map(ctx->pipe, transfer);
   if (!transfer)
      goto error_map;

   util_copy_rect(map, dst->texture->format, transfer->stride, 0, 0,
                  dst_box->width, dst_box->height,
                  src, src_stride, src_x, src_y);

   ctx->pipe->transfer_unmap(ctx->pipe, transfer);

error_map:
   ctx->pipe->transfer_destroy(ctx->pipe, transfer);
}

static void
vl_context_clear_sampler(struct pipe_video_context *context,
                         struct pipe_sampler_view *dst,
                         const struct pipe_box *dst_box,
                         const float *rgba)
{
   struct vl_context *ctx = (struct vl_context*)context;
   struct pipe_transfer *transfer;
   union util_color uc;
   void *map;
   unsigned i;

   assert(context);
   assert(dst);
   assert(dst_box);
   assert(rgba);

   transfer = ctx->pipe->get_transfer(ctx->pipe, dst->texture, 0, PIPE_TRANSFER_WRITE, dst_box);
   if (!transfer)
      return;

   map = ctx->pipe->transfer_map(ctx->pipe, transfer);
   if (!transfer)
      goto error_map;

   for ( i = 0; i < 4; ++i)
      uc.f[i] = rgba[i];

   util_fill_rect(map, dst->texture->format, transfer->stride, 0, 0,
                  dst_box->width, dst_box->height, &uc);

   ctx->pipe->transfer_unmap(ctx->pipe, transfer);

error_map:
   ctx->pipe->transfer_destroy(ctx->pipe, transfer);
}

static struct pipe_video_decoder *
vl_context_create_decoder(struct pipe_video_context *context,
                          enum pipe_video_profile profile,
                          enum pipe_video_chroma_format chroma_format,
                          unsigned width, unsigned height)
{
   struct vl_context *ctx = (struct vl_context*)context;
   unsigned buffer_width, buffer_height;

   assert(context);
   assert(width > 0 && height > 0);

   buffer_width = ctx->pot_buffers ? util_next_power_of_two(width) : width;
   buffer_height = ctx->pot_buffers ? util_next_power_of_two(height) : height;

   switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_CODEC_MPEG12:
         return vl_create_mpeg12_decoder(context, ctx->pipe, profile, chroma_format,
                                         buffer_width, buffer_height);
      default:
         return NULL;
   }
   return NULL;
}

static struct pipe_video_buffer *
vl_context_create_buffer(struct pipe_video_context *context,
                         enum pipe_format buffer_format,
                         enum pipe_video_chroma_format chroma_format,
                         unsigned width, unsigned height)
{
   const enum pipe_format resource_formats[3] = {
      PIPE_FORMAT_R8_SNORM,
      PIPE_FORMAT_R8_SNORM,
      PIPE_FORMAT_R8_SNORM
   };

   struct vl_context *ctx = (struct vl_context*)context;
   struct pipe_video_buffer *result;
   unsigned buffer_width, buffer_height;

   assert(context);
   assert(width > 0 && height > 0);
   assert(buffer_format == PIPE_FORMAT_YV12);

   buffer_width = ctx->pot_buffers ? util_next_power_of_two(width) : width;
   buffer_height = ctx->pot_buffers ? util_next_power_of_two(height) : height;

   result = vl_video_buffer_init(context, ctx->pipe,
                                 buffer_width, buffer_height, 1,
                                 chroma_format, 3,
                                 resource_formats,
                                 PIPE_USAGE_STATIC);
   if (result) // TODO move format handling into vl_video_buffer
      result->buffer_format = buffer_format;

   return result;
}

static struct pipe_video_compositor *
vl_context_create_compositor(struct pipe_video_context *context)
{
   struct vl_context *ctx = (struct vl_context*)context;

   assert(context);

   return vl_compositor_init(context, ctx->pipe);
}

struct pipe_video_context *
vl_create_context(struct pipe_context *pipe, bool pot_buffers)
{
   struct vl_context *ctx;

   ctx = CALLOC_STRUCT(vl_context);

   if (!ctx)
      return NULL;

   ctx->base.screen = pipe->screen;

   ctx->base.destroy = vl_context_destroy;
   ctx->base.get_param = vl_context_get_param;
   ctx->base.is_format_supported = vl_context_is_format_supported;
   ctx->base.create_surface = vl_context_create_surface;
   ctx->base.create_sampler_view = vl_context_create_sampler_view;
   ctx->base.clear_sampler = vl_context_clear_sampler;
   ctx->base.upload_sampler = vl_context_upload_sampler;
   ctx->base.create_decoder = vl_context_create_decoder;
   ctx->base.create_buffer = vl_context_create_buffer;
   ctx->base.create_compositor = vl_context_create_compositor;

   ctx->pipe = pipe;
   ctx->pot_buffers = pot_buffers;

   return &ctx->base;
}
