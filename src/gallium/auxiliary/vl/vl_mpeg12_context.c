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

#include "util/u_inlines.h"
#include "util/u_memory.h"

#include "vl_mpeg12_context.h"
#include "vl_defines.h"
#include <pipe/p_shader_tokens.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include <util/u_keymap.h>
#include <util/u_rect.h>
#include <util/u_video.h>
#include <util/u_surface.h>
#include <util/u_sampler.h>

static const unsigned const_empty_block_mask_420[3][2][2] = {
        { { 0x20, 0x10 },  { 0x08, 0x04 } },
        { { 0x02, 0x02 },  { 0x02, 0x02 } },
        { { 0x01, 0x01 },  { 0x01, 0x01 } }
};

static void
upload_buffer(struct vl_mpeg12_context *ctx,
              struct vl_mpeg12_buffer *buffer,
              struct pipe_mpeg12_macroblock *mb)
{
   short *blocks;
   unsigned tb, x, y;

   assert(ctx);
   assert(buffer);
   assert(mb);

   blocks = mb->blocks;

   for (y = 0; y < 2; ++y) {
      for (x = 0; x < 2; ++x, ++tb) {
         if (mb->cbp & (*ctx->empty_block_mask)[0][y][x]) {
            vl_idct_add_block(&buffer->idct[0], mb->mbx * 2 + x, mb->mby * 2 + y, blocks);
            blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
         }
      }
   }

   /* TODO: Implement 422, 444 */
   assert(ctx->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   for (tb = 1; tb < 3; ++tb) {
      if (mb->cbp & (*ctx->empty_block_mask)[tb][0][0]) {
         vl_idct_add_block(&buffer->idct[tb], mb->mbx, mb->mby, blocks);
         blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
      }
   }
}

static void
vl_mpeg12_buffer_destroy(struct pipe_video_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)buf->base.context;
   assert(buf && ctx);

   vl_video_buffer_cleanup(&buf->idct_source);
   vl_video_buffer_cleanup(&buf->idct_2_mc);
   vl_video_buffer_cleanup(&buf->render_result);
   vl_vb_cleanup(&buf->vertex_stream);
   vl_idct_cleanup_buffer(&ctx->idct_y, &buf->idct[0]);
   vl_idct_cleanup_buffer(&ctx->idct_c, &buf->idct[1]);
   vl_idct_cleanup_buffer(&ctx->idct_c, &buf->idct[2]);
   vl_mpeg12_mc_cleanup_buffer(&buf->mc[0]);
   vl_mpeg12_mc_cleanup_buffer(&buf->mc[1]);
   vl_mpeg12_mc_cleanup_buffer(&buf->mc[2]);

   FREE(buf);
}

static void
vl_mpeg12_buffer_map(struct pipe_video_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_context *ctx;
   assert(buf);

   ctx = (struct vl_mpeg12_context *)buf->base.context;
   assert(ctx);

   vl_vb_map(&buf->vertex_stream, ctx->pipe);
   vl_idct_map_buffers(&ctx->idct_y, &buf->idct[0]);
   vl_idct_map_buffers(&ctx->idct_c, &buf->idct[1]);
   vl_idct_map_buffers(&ctx->idct_c, &buf->idct[2]);
}

static void
vl_mpeg12_buffer_add_macroblocks(struct pipe_video_buffer *buffer,
                                 unsigned num_macroblocks,
                                 struct pipe_macroblock *macroblocks)
{
   struct pipe_mpeg12_macroblock *mpeg12_macroblocks = (struct pipe_mpeg12_macroblock*)macroblocks;
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_context *ctx;
   unsigned i;

   assert(buf);

   ctx =  (struct vl_mpeg12_context*)buf->base.context;
   assert(ctx);

   assert(num_macroblocks);
   assert(macroblocks);
   assert(macroblocks->codec == PIPE_VIDEO_CODEC_MPEG12);

   for ( i = 0; i < num_macroblocks; ++i ) {
      vl_vb_add_block(&buf->vertex_stream, &mpeg12_macroblocks[i], ctx->empty_block_mask);
      upload_buffer(ctx, buf, &mpeg12_macroblocks[i]);
   }
}

static void
vl_mpeg12_buffer_unmap(struct pipe_video_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_context *ctx;
   assert(buf);

   ctx = (struct vl_mpeg12_context *)buf->base.context;
   assert(ctx);

   vl_vb_unmap(&buf->vertex_stream, ctx->pipe);
   vl_idct_unmap_buffers(&ctx->idct_y, &buf->idct[0]);
   vl_idct_unmap_buffers(&ctx->idct_c, &buf->idct[1]);
   vl_idct_unmap_buffers(&ctx->idct_c, &buf->idct[2]);
}

static void
vl_mpeg12_buffer_flush(struct pipe_video_buffer *buffer,
                       struct pipe_video_buffer *refs[2],
                       struct pipe_fence_handle **fence)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer *)buffer;
   struct vl_mpeg12_buffer *past = (struct vl_mpeg12_buffer *)refs[0];
   struct vl_mpeg12_buffer *future = (struct vl_mpeg12_buffer *)refs[1];

   vl_surfaces *surfaces;
   vl_sampler_views *sv_past;
   vl_sampler_views *sv_future;

   struct pipe_sampler_view *sv_refs[2];
   unsigned ne_start, ne_num, e_start, e_num;
   struct vl_mpeg12_context *ctx;
   unsigned i;

   assert(buf);

   ctx = (struct vl_mpeg12_context *)buf->base.context;
   assert(ctx);

   surfaces = vl_video_buffer_surfaces(&buf->render_result);

   sv_past = past ? vl_video_buffer_sampler_views(&past->render_result) : NULL;
   sv_future = future ? vl_video_buffer_sampler_views(&future->render_result) : NULL;

   vl_vb_restart(&buf->vertex_stream, &ne_start, &ne_num, &e_start, &e_num);

   ctx->pipe->set_vertex_buffers(ctx->pipe, 2, buf->vertex_bufs.all);
   ctx->pipe->bind_blend_state(ctx->pipe, ctx->blend);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      ctx->pipe->bind_vertex_elements_state(ctx->pipe, ctx->ves[i]);
      vl_idct_flush(i == 0 ? &ctx->idct_y : &ctx->idct_c, &buf->idct[i], ne_num);

      sv_refs[0] = sv_past ? (*sv_past)[i] : NULL;
      sv_refs[1] = sv_future ? (*sv_future)[i] : NULL;

      vl_mpeg12_mc_renderer_flush(&ctx->mc, &buf->mc[i], (*surfaces)[i],
                                  sv_refs, ne_start, ne_num, e_start, e_num, fence);
   }
}

static void
vl_mpeg12_buffer_get_sampler_views(struct pipe_video_buffer *buffer,
                                   struct pipe_sampler_view *sampler_views[3])
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   vl_sampler_views *samplers;
   unsigned i;

   assert(buf);

   samplers = vl_video_buffer_sampler_views(&buf->render_result);

   assert(samplers);

   for (i = 0; i < VL_MAX_PLANES; ++i)
      pipe_sampler_view_reference(&sampler_views[i], (*samplers)[i]);
}

static void
vl_mpeg12_destroy(struct pipe_video_context *vpipe)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   /* Asserted in softpipe_delete_fs_state() for some reason */
   ctx->pipe->bind_vs_state(ctx->pipe, NULL);
   ctx->pipe->bind_fs_state(ctx->pipe, NULL);

   ctx->pipe->delete_blend_state(ctx->pipe, ctx->blend);
   ctx->pipe->delete_rasterizer_state(ctx->pipe, ctx->rast);
   ctx->pipe->delete_depth_stencil_alpha_state(ctx->pipe, ctx->dsa);

   vl_mpeg12_mc_renderer_cleanup(&ctx->mc);
   vl_idct_cleanup(&ctx->idct_y);
   vl_idct_cleanup(&ctx->idct_c);
   ctx->pipe->delete_vertex_elements_state(ctx->pipe, ctx->ves[0]);
   ctx->pipe->delete_vertex_elements_state(ctx->pipe, ctx->ves[1]);
   ctx->pipe->delete_vertex_elements_state(ctx->pipe, ctx->ves[2]);
   pipe_resource_reference(&ctx->quads.buffer, NULL);
   ctx->pipe->destroy(ctx->pipe);

   FREE(ctx);
}

static int
vl_mpeg12_get_param(struct pipe_video_context *vpipe, int param)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   if (param == PIPE_CAP_NPOT_TEXTURES)
      return !ctx->pot_buffers;

   debug_printf("vl_mpeg12_context: Unknown PIPE_CAP %d\n", param);
   return 0;
}

static struct pipe_surface *
vl_mpeg12_create_surface(struct pipe_video_context *vpipe,
                         struct pipe_resource *resource,
                         const struct pipe_surface *templ)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(ctx);

   return ctx->pipe->create_surface(ctx->pipe, resource, templ);
}

static struct pipe_sampler_view *
vl_mpeg12_create_sampler_view(struct pipe_video_context *vpipe,
                              struct pipe_resource *resource,
                              const struct pipe_sampler_view *templ)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(ctx);

   return ctx->pipe->create_sampler_view(ctx->pipe, resource, templ);
}

static struct pipe_video_buffer *
vl_mpeg12_create_buffer(struct pipe_video_context *vpipe)
{
   const enum pipe_format idct_source_formats[3] = {
      PIPE_FORMAT_R16G16B16A16_SNORM,
      PIPE_FORMAT_R16G16B16A16_SNORM,
      PIPE_FORMAT_R16G16B16A16_SNORM
   };

   const enum pipe_format idct_2_mc_formats[3] = {
      PIPE_FORMAT_R16_SNORM,
      PIPE_FORMAT_R16_SNORM,
      PIPE_FORMAT_R16_SNORM
   };

   const enum pipe_format render_result_formats[3] = {
      PIPE_FORMAT_R8_SNORM,
      PIPE_FORMAT_R8_SNORM,
      PIPE_FORMAT_R8_SNORM
   };

   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;
   struct vl_mpeg12_buffer *buffer;

   vl_sampler_views *idct_views, *mc_views;
   vl_surfaces *idct_surfaces;

   assert(ctx);

   buffer = CALLOC_STRUCT(vl_mpeg12_buffer);
   if (buffer == NULL)
      return NULL;

   buffer->base.context = vpipe;
   buffer->base.destroy = vl_mpeg12_buffer_destroy;
   buffer->base.map = vl_mpeg12_buffer_map;
   buffer->base.add_macroblocks = vl_mpeg12_buffer_add_macroblocks;
   buffer->base.unmap = vl_mpeg12_buffer_unmap;
   buffer->base.flush = vl_mpeg12_buffer_flush;
   buffer->base.get_sampler_views = vl_mpeg12_buffer_get_sampler_views;

   buffer->vertex_bufs.individual.quad.stride = ctx->quads.stride;
   buffer->vertex_bufs.individual.quad.buffer_offset = ctx->quads.buffer_offset;
   pipe_resource_reference(&buffer->vertex_bufs.individual.quad.buffer, ctx->quads.buffer);

   buffer->vertex_bufs.individual.stream = vl_vb_init(&buffer->vertex_stream, ctx->pipe,
                                                      ctx->buffer_width / MACROBLOCK_WIDTH *
                                                      ctx->buffer_height / MACROBLOCK_HEIGHT);
   if (!buffer->vertex_bufs.individual.stream.buffer)
      goto error_vertex_stream;

   if (!vl_video_buffer_init(&buffer->idct_source, ctx->pipe,
                             ctx->buffer_width / 4, ctx->buffer_height, 1,
                             ctx->base.chroma_format, 3,
                             idct_source_formats,
                             PIPE_USAGE_STREAM))
      goto error_idct_source;

   if (!vl_video_buffer_init(&buffer->idct_2_mc, ctx->pipe,
                             ctx->buffer_width, ctx->buffer_height, 1,
                             ctx->base.chroma_format, 3,
                             idct_2_mc_formats,
                             PIPE_USAGE_STATIC))
      goto error_idct_2_mc;

   if (!vl_video_buffer_init(&buffer->render_result, ctx->pipe,
                             ctx->buffer_width, ctx->buffer_height, 1,
                             ctx->base.chroma_format, 3,
                             render_result_formats,
                             PIPE_USAGE_STATIC))
      goto error_render_result;

   idct_views = vl_video_buffer_sampler_views(&buffer->idct_source);
   if (!idct_views)
      goto error_idct_views;

   idct_surfaces = vl_video_buffer_surfaces(&buffer->idct_2_mc);
   if (!idct_surfaces)
      goto error_idct_surfaces;

   if (!vl_idct_init_buffer(&ctx->idct_y, &buffer->idct[0],
                            (*idct_views)[0], (*idct_surfaces)[0]))
      goto error_idct_y;

   if (!vl_idct_init_buffer(&ctx->idct_c, &buffer->idct[1],
                            (*idct_views)[1], (*idct_surfaces)[1]))
      goto error_idct_cb;

   if (!vl_idct_init_buffer(&ctx->idct_c, &buffer->idct[2],
                            (*idct_views)[2], (*idct_surfaces)[2]))
      goto error_idct_cr;

   mc_views = vl_video_buffer_sampler_views(&buffer->idct_2_mc);
   if (!mc_views)
      goto error_mc_views;

   if(!vl_mpeg12_mc_init_buffer(&ctx->mc, &buffer->mc[0], (*mc_views)[0]))
      goto error_mc_y;

   if(!vl_mpeg12_mc_init_buffer(&ctx->mc, &buffer->mc[1], (*mc_views)[1]))
      goto error_mc_cb;

   if(!vl_mpeg12_mc_init_buffer(&ctx->mc, &buffer->mc[2], (*mc_views)[2]))
      goto error_mc_cr;

   return &buffer->base;

error_mc_cr:
   vl_mpeg12_mc_cleanup_buffer(&buffer->mc[1]);

error_mc_cb:
   vl_mpeg12_mc_cleanup_buffer(&buffer->mc[0]);

error_mc_y:
error_mc_views:
   vl_idct_cleanup_buffer(&ctx->idct_c, &buffer->idct[2]);

error_idct_cr:
   vl_idct_cleanup_buffer(&ctx->idct_c, &buffer->idct[1]);

error_idct_cb:
   vl_idct_cleanup_buffer(&ctx->idct_y, &buffer->idct[0]);

error_idct_y:
error_idct_surfaces:
error_idct_views:
   vl_video_buffer_cleanup(&buffer->render_result);

error_render_result:
   vl_video_buffer_cleanup(&buffer->idct_2_mc);

error_idct_2_mc:
   vl_video_buffer_cleanup(&buffer->idct_source);

error_idct_source:
   vl_vb_cleanup(&buffer->vertex_stream);

error_vertex_stream:
   FREE(buffer);
   return NULL;
}

static boolean
vl_mpeg12_is_format_supported(struct pipe_video_context *vpipe,
                              enum pipe_format format,
                              unsigned usage)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   return ctx->pipe->screen->is_format_supported(ctx->pipe->screen, format,
                                                 PIPE_TEXTURE_2D,
                                                 0, usage);
}

static void
vl_mpeg12_clear_sampler(struct pipe_video_context *vpipe,
                        struct pipe_sampler_view *dst,
                        const struct pipe_box *dst_box,
                        const float *rgba)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;
   struct pipe_transfer *transfer;
   union util_color uc;
   void *map;
   unsigned i;

   assert(vpipe);
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

static void
vl_mpeg12_upload_sampler(struct pipe_video_context *vpipe,
                         struct pipe_sampler_view *dst,
                         const struct pipe_box *dst_box,
                         const void *src, unsigned src_stride,
                         unsigned src_x, unsigned src_y)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;
   struct pipe_transfer *transfer;
   void *map;

   assert(vpipe);
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

static struct pipe_video_compositor *
vl_mpeg12_create_compositor(struct pipe_video_context *vpipe)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   return vl_compositor_init(vpipe, ctx->pipe);
}

static bool
init_pipe_state(struct vl_mpeg12_context *ctx)
{
   struct pipe_rasterizer_state rast;
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state dsa;
   unsigned i;

   assert(ctx);

   memset(&rast, 0, sizeof rast);
   rast.flatshade = 1;
   rast.flatshade_first = 0;
   rast.light_twoside = 0;
   rast.front_ccw = 1;
   rast.cull_face = PIPE_FACE_NONE;
   rast.fill_back = PIPE_POLYGON_MODE_FILL;
   rast.fill_front = PIPE_POLYGON_MODE_FILL;
   rast.offset_point = 0;
   rast.offset_line = 0;
   rast.scissor = 0;
   rast.poly_smooth = 0;
   rast.poly_stipple_enable = 0;
   rast.sprite_coord_enable = 0;
   rast.point_size_per_vertex = 0;
   rast.multisample = 0;
   rast.line_smooth = 0;
   rast.line_stipple_enable = 0;
   rast.line_stipple_factor = 0;
   rast.line_stipple_pattern = 0;
   rast.line_last_pixel = 0;
   rast.line_width = 1;
   rast.point_smooth = 0;
   rast.point_quad_rasterization = 0;
   rast.point_size_per_vertex = 1;
   rast.offset_units = 1;
   rast.offset_scale = 1;
   rast.gl_rasterization_rules = 1;

   ctx->rast = ctx->pipe->create_rasterizer_state(ctx->pipe, &rast);
   ctx->pipe->bind_rasterizer_state(ctx->pipe, ctx->rast);

   memset(&blend, 0, sizeof blend);

   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 0;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   /* Needed to allow color writes to FB, even if blending disabled */
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.dither = 0;
   ctx->blend = ctx->pipe->create_blend_state(ctx->pipe, &blend);

   memset(&dsa, 0, sizeof dsa);
   dsa.depth.enabled = 0;
   dsa.depth.writemask = 0;
   dsa.depth.func = PIPE_FUNC_ALWAYS;
   for (i = 0; i < 2; ++i) {
      dsa.stencil[i].enabled = 0;
      dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
      dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
      dsa.stencil[i].valuemask = 0;
      dsa.stencil[i].writemask = 0;
   }
   dsa.alpha.enabled = 0;
   dsa.alpha.func = PIPE_FUNC_ALWAYS;
   dsa.alpha.ref_value = 0;
   ctx->dsa = ctx->pipe->create_depth_stencil_alpha_state(ctx->pipe, &dsa);
   ctx->pipe->bind_depth_stencil_alpha_state(ctx->pipe, ctx->dsa);

   return true;
}

static bool
init_idct(struct vl_mpeg12_context *ctx, unsigned buffer_width, unsigned buffer_height)
{
   unsigned chroma_width, chroma_height, chroma_blocks_x, chroma_blocks_y;
   struct pipe_sampler_view *idct_matrix;

   /* TODO: Implement 422, 444 */
   assert(ctx->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);
   ctx->empty_block_mask = &const_empty_block_mask_420;

   if (!(idct_matrix = vl_idct_upload_matrix(ctx->pipe)))
      goto error_idct_matrix;

   if (!vl_idct_init(&ctx->idct_y, ctx->pipe, buffer_width, buffer_height,
                     2, 2, idct_matrix))
      goto error_idct_y;

   if (ctx->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      chroma_width = buffer_width / 2;
      chroma_height = buffer_height / 2;
      chroma_blocks_x = 1;
      chroma_blocks_y = 1;
   } else if (ctx->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
      chroma_width = buffer_width;
      chroma_height = buffer_height / 2;
      chroma_blocks_x = 2;
      chroma_blocks_y = 1;
   } else {
      chroma_width = buffer_width;
      chroma_height = buffer_height;
      chroma_blocks_x = 2;
      chroma_blocks_y = 2;
   }

   if(!vl_idct_init(&ctx->idct_c, ctx->pipe, chroma_width, chroma_height,
                    chroma_blocks_x, chroma_blocks_y, idct_matrix))
      goto error_idct_c;

   pipe_sampler_view_reference(&idct_matrix, NULL);
   return true;

error_idct_c:
   vl_idct_cleanup(&ctx->idct_y);

error_idct_y:
   pipe_sampler_view_reference(&idct_matrix, NULL);

error_idct_matrix:
   return false;
}

struct pipe_video_context *
vl_create_mpeg12_context(struct pipe_context *pipe,
                         enum pipe_video_profile profile,
                         enum pipe_video_chroma_format chroma_format,
                         unsigned width, unsigned height,
                         bool pot_buffers)
{
   struct vl_mpeg12_context *ctx;

   assert(u_reduce_video_profile(profile) == PIPE_VIDEO_CODEC_MPEG12);

   ctx = CALLOC_STRUCT(vl_mpeg12_context);

   if (!ctx)
      return NULL;

   ctx->base.profile = profile;
   ctx->base.chroma_format = chroma_format;
   ctx->base.width = width;
   ctx->base.height = height;

   ctx->base.screen = pipe->screen;

   ctx->base.destroy = vl_mpeg12_destroy;
   ctx->base.get_param = vl_mpeg12_get_param;
   ctx->base.is_format_supported = vl_mpeg12_is_format_supported;
   ctx->base.create_surface = vl_mpeg12_create_surface;
   ctx->base.create_sampler_view = vl_mpeg12_create_sampler_view;
   ctx->base.create_buffer = vl_mpeg12_create_buffer;
   ctx->base.clear_sampler = vl_mpeg12_clear_sampler;
   ctx->base.upload_sampler = vl_mpeg12_upload_sampler;
   ctx->base.create_compositor = vl_mpeg12_create_compositor;

   ctx->pipe = pipe;
   ctx->pot_buffers = pot_buffers;

   ctx->quads = vl_vb_upload_quads(ctx->pipe, 2, 2);
   ctx->ves[0] = vl_vb_get_elems_state(ctx->pipe, TGSI_SWIZZLE_X);
   ctx->ves[1] = vl_vb_get_elems_state(ctx->pipe, TGSI_SWIZZLE_Y);
   ctx->ves[2] = vl_vb_get_elems_state(ctx->pipe, TGSI_SWIZZLE_Z);

   ctx->buffer_width = pot_buffers ? util_next_power_of_two(width) : align(width, MACROBLOCK_WIDTH);
   ctx->buffer_height = pot_buffers ? util_next_power_of_two(height) : align(height, MACROBLOCK_HEIGHT);

   if (!init_idct(ctx, ctx->buffer_width, ctx->buffer_height))
      goto error_idct;

   if (!vl_mpeg12_mc_renderer_init(&ctx->mc, ctx->pipe, ctx->buffer_width, ctx->buffer_height))
      goto error_mc;

   if (!init_pipe_state(ctx))
      goto error_pipe_state;

   return &ctx->base;

error_pipe_state:
   vl_mpeg12_mc_renderer_cleanup(&ctx->mc);

error_mc:
   vl_idct_cleanup(&ctx->idct_y);
   vl_idct_cleanup(&ctx->idct_c);

error_idct:
   ctx->pipe->destroy(ctx->pipe);
   FREE(ctx);
   return NULL;
}
