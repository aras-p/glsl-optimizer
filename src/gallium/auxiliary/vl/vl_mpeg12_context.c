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

#define NUM_BUFFERS 2

static const unsigned const_empty_block_mask_420[3][2][2] = {
        { { 0x20, 0x10 },  { 0x08, 0x04 } },
        { { 0x02, 0x02 },  { 0x02, 0x02 } },
        { { 0x01, 0x01 },  { 0x01, 0x01 } }
};

static void
flush_buffer(struct vl_mpeg12_context *ctx)
{
   unsigned ne_start, ne_num, e_start, e_num;
   assert(ctx);

   if(ctx->cur_buffer != NULL) {

      vl_vb_unmap(&ctx->cur_buffer->vertex_stream, ctx->pipe);
      vl_idct_unmap_buffers(&ctx->idct_y, &ctx->cur_buffer->idct_y);
      vl_idct_unmap_buffers(&ctx->idct_cr, &ctx->cur_buffer->idct_cr);
      vl_idct_unmap_buffers(&ctx->idct_cb, &ctx->cur_buffer->idct_cb);
      vl_vb_restart(&ctx->cur_buffer->vertex_stream,
		    &ne_start, &ne_num, &e_start, &e_num);

      ctx->pipe->set_vertex_buffers(ctx->pipe, 2, ctx->cur_buffer->vertex_bufs.all);
      ctx->pipe->bind_vertex_elements_state(ctx->pipe, ctx->vertex_elems_state);
      vl_idct_flush(&ctx->idct_y, &ctx->cur_buffer->idct_y, ne_num);
      vl_idct_flush(&ctx->idct_cr, &ctx->cur_buffer->idct_cr, ne_num);
      vl_idct_flush(&ctx->idct_cb, &ctx->cur_buffer->idct_cb, ne_num);
      vl_mpeg12_mc_renderer_flush(&ctx->mc_renderer, &ctx->cur_buffer->mc,
                                  ne_start, ne_num, e_start, e_num);

      ctx->cur_buffer = NULL;
   }
}

static void
rotate_buffer(struct vl_mpeg12_context *ctx)
{
   struct pipe_resource *y, *cr, *cb;
   static unsigned key = 0;
   struct vl_mpeg12_buffer *buffer;

   assert(ctx);

   flush_buffer(ctx);

   buffer = (struct vl_mpeg12_buffer*)util_keymap_lookup(ctx->buffer_map, &key);
   if (!buffer) {
      boolean added_to_map;

      buffer = CALLOC_STRUCT(vl_mpeg12_buffer);
      if (buffer == NULL)
         return;

      buffer->vertex_bufs.individual.quad.stride = ctx->quads.stride;
      buffer->vertex_bufs.individual.quad.buffer_offset = ctx->quads.buffer_offset;
      pipe_resource_reference(&buffer->vertex_bufs.individual.quad.buffer, ctx->quads.buffer);

      buffer->vertex_bufs.individual.stream = vl_vb_init(&buffer->vertex_stream, ctx->pipe,
                                                         ctx->vertex_buffer_size);
      if (!(y = vl_idct_init_buffer(&ctx->idct_y, &buffer->idct_y))) {
         FREE(buffer);
         return;
      }

      if (!(cr = vl_idct_init_buffer(&ctx->idct_cr, &buffer->idct_cr))) {
         FREE(buffer);
         return;
      }

      if (!(cb = vl_idct_init_buffer(&ctx->idct_cb, &buffer->idct_cb))) {
         FREE(buffer);
         return;
      }

      if(!vl_mpeg12_mc_init_buffer(&ctx->mc_renderer, &buffer->mc, y, cr, cb)) {
         FREE(buffer);
         return;
      }

      added_to_map = util_keymap_insert(ctx->buffer_map, &key, buffer, ctx);
      assert(added_to_map);
   }
   ++key;
   key %= NUM_BUFFERS;
   ctx->cur_buffer = buffer;

   vl_vb_map(&ctx->cur_buffer->vertex_stream, ctx->pipe);
   vl_idct_map_buffers(&ctx->idct_y, &ctx->cur_buffer->idct_y);
   vl_idct_map_buffers(&ctx->idct_cr, &ctx->cur_buffer->idct_cr);
   vl_idct_map_buffers(&ctx->idct_cb, &ctx->cur_buffer->idct_cb);
}

static void
delete_buffer(const struct keymap *map,
              const void *key, void *data,
              void *user)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)user;
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)data;

   assert(map);
   assert(key);
   assert(data);
   assert(user);

   vl_vb_cleanup(&buf->vertex_stream);
   vl_idct_cleanup_buffer(&ctx->idct_y, &buf->idct_y);
   vl_idct_cleanup_buffer(&ctx->idct_cb, &buf->idct_cb);
   vl_idct_cleanup_buffer(&ctx->idct_cr, &buf->idct_cr);
   vl_mpeg12_mc_cleanup_buffer(&ctx->mc_renderer, &buf->mc);
}

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
            vl_idct_add_block(&buffer->idct_y, mb->mbx * 2 + x, mb->mby * 2 + y, blocks);
            blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
         }
      }
   }

   /* TODO: Implement 422, 444 */
   assert(ctx->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   for (tb = 1; tb < 3; ++tb) {
      if (mb->cbp & (*ctx->empty_block_mask)[tb][0][0]) {
         if(tb == 1)
            vl_idct_add_block(&buffer->idct_cb, mb->mbx, mb->mby, blocks);
         else
            vl_idct_add_block(&buffer->idct_cr, mb->mbx, mb->mby, blocks);
         blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
      }
   }
}

static void
vl_mpeg12_destroy(struct pipe_video_context *vpipe)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   flush_buffer(ctx);

   /* Asserted in softpipe_delete_fs_state() for some reason */
   ctx->pipe->bind_vs_state(ctx->pipe, NULL);
   ctx->pipe->bind_fs_state(ctx->pipe, NULL);

   ctx->pipe->delete_blend_state(ctx->pipe, ctx->blend);
   ctx->pipe->delete_rasterizer_state(ctx->pipe, ctx->rast);
   ctx->pipe->delete_depth_stencil_alpha_state(ctx->pipe, ctx->dsa);

   pipe_surface_reference(&ctx->decode_target, NULL);
   vl_compositor_cleanup(&ctx->compositor);
   util_delete_keymap(ctx->buffer_map, ctx);
   vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
   vl_idct_cleanup(&ctx->idct_y);
   vl_idct_cleanup(&ctx->idct_cr);
   vl_idct_cleanup(&ctx->idct_cb);
   ctx->pipe->delete_vertex_elements_state(ctx->pipe, ctx->vertex_elems_state);
   pipe_resource_reference(&ctx->quads.buffer, NULL);
   ctx->pipe->destroy(ctx->pipe);

   FREE(ctx);
}

static int
vl_mpeg12_get_param(struct pipe_video_context *vpipe, int param)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   switch (param) {
      case PIPE_CAP_NPOT_TEXTURES:
         /* XXX: Temporary; not all paths are NPOT-tested */
#if 0
         return ctx->pipe->screen->get_param(ctx->pipe->screen, param);
#endif
         return FALSE;
      case PIPE_CAP_DECODE_TARGET_PREFERRED_FORMAT:
         return ctx->decode_format;
      default:
      {
         debug_printf("vl_mpeg12_context: Unknown PIPE_CAP %d\n", param);
         return 0;
      }
   }
}

static struct pipe_surface *
vl_mpeg12_create_surface(struct pipe_video_context *vpipe,
                         struct pipe_resource *resource,
                         const struct pipe_surface *templat)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   return ctx->pipe->create_surface(ctx->pipe, resource, templat);
}

static boolean
vl_mpeg12_is_format_supported(struct pipe_video_context *vpipe,
                              enum pipe_format format,
                              unsigned usage,
                              unsigned geom)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   return ctx->pipe->screen->is_format_supported(ctx->pipe->screen, format,
                                                 PIPE_TEXTURE_2D,
                                                 0, usage);
}

static void
vl_mpeg12_decode_macroblocks(struct pipe_video_context *vpipe,
                             struct pipe_surface *past,
                             struct pipe_surface *future,
                             unsigned num_macroblocks,
                             struct pipe_macroblock *macroblocks,
                             struct pipe_fence_handle **fence)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;
   struct pipe_mpeg12_macroblock *mpeg12_macroblocks = (struct pipe_mpeg12_macroblock*)macroblocks;
   unsigned i;

   assert(vpipe);
   assert(num_macroblocks);
   assert(macroblocks);
   assert(macroblocks->codec == PIPE_VIDEO_CODEC_MPEG12);
   assert(ctx->decode_target);
   assert(ctx->cur_buffer);

   for ( i = 0; i < num_macroblocks; ++i ) {
      vl_vb_add_block(&ctx->cur_buffer->vertex_stream, &mpeg12_macroblocks[i],
                      ctx->empty_block_mask);
      upload_buffer(ctx, ctx->cur_buffer, &mpeg12_macroblocks[i]);
   }

   vl_mpeg12_mc_set_surfaces(&ctx->mc_renderer, &ctx->cur_buffer->mc,
                             ctx->decode_target, past, future, fence);
}

static void
vl_mpeg12_clear_render_target(struct pipe_video_context *vpipe,
                       struct pipe_surface *dst,
                       unsigned dstx, unsigned dsty,
                       const float *rgba,
                       unsigned width, unsigned height)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(dst);

   if (ctx->pipe->clear_render_target)
      ctx->pipe->clear_render_target(ctx->pipe, dst, rgba, dstx, dsty, width, height);
   else
      util_clear_render_target(ctx->pipe, dst, rgba, dstx, dsty, width, height);
}

static void
vl_mpeg12_resource_copy_region(struct pipe_video_context *vpipe,
                               struct pipe_resource *dst,
                               unsigned dstx, unsigned dsty, unsigned dstz,
                               struct pipe_resource *src,
                               unsigned srcx, unsigned srcy, unsigned srcz,
                               unsigned width, unsigned height)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(dst);

   struct pipe_box box;
   box.x = srcx;
   box.y = srcy;
   box.z = srcz;
   box.width = width;
   box.height = height;

   if (ctx->pipe->resource_copy_region)
      ctx->pipe->resource_copy_region(ctx->pipe, dst, 0,
                                      dstx, dsty, dstz,
                                      src, 0, &box);
   else
      util_resource_copy_region(ctx->pipe, dst, 0,
                                dstx, dsty, dstz,
                                src, 0, &box);
}

static struct pipe_transfer*
vl_mpeg12_get_transfer(struct pipe_video_context *vpipe,
                       struct pipe_resource *resource,
                       unsigned level,
                       unsigned usage,  /* a combination of PIPE_TRANSFER_x */
                       const struct pipe_box *box)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(resource);
   assert(box);

   return ctx->pipe->get_transfer(ctx->pipe, resource, level, usage, box);
}

static void
vl_mpeg12_transfer_destroy(struct pipe_video_context *vpipe,
                           struct pipe_transfer *transfer)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(transfer);

   ctx->pipe->transfer_destroy(ctx->pipe, transfer);
}

static void*
vl_mpeg12_transfer_map(struct pipe_video_context *vpipe,
                       struct pipe_transfer *transfer)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(transfer);

   return ctx->pipe->transfer_map(ctx->pipe, transfer);
}

static void
vl_mpeg12_transfer_flush_region(struct pipe_video_context *vpipe,
                                struct pipe_transfer *transfer,
                                const struct pipe_box *box)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(transfer);
   assert(box);

   ctx->pipe->transfer_flush_region(ctx->pipe, transfer, box);
}

static void
vl_mpeg12_transfer_unmap(struct pipe_video_context *vpipe,
                         struct pipe_transfer *transfer)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(transfer);

   ctx->pipe->transfer_unmap(ctx->pipe, transfer);
}

static void
vl_mpeg12_transfer_inline_write(struct pipe_video_context *vpipe,
                                struct pipe_resource *resource,
                                unsigned level,
                                unsigned usage, /* a combination of PIPE_TRANSFER_x */
                                const struct pipe_box *box,
                                const void *data,
                                unsigned stride,
                                unsigned slice_stride)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(resource);
   assert(box);
   assert(data);
   assert(ctx->pipe->transfer_inline_write);

   ctx->pipe->transfer_inline_write(ctx->pipe, resource, level, usage,
                                    box, data, stride, slice_stride);
}

static void
vl_mpeg12_render_picture(struct pipe_video_context     *vpipe,
                         struct pipe_surface           *src_surface,
                         enum pipe_mpeg12_picture_type picture_type,
                         /*unsigned                    num_past_surfaces,
                         struct pipe_surface           *past_surfaces,
                         unsigned                      num_future_surfaces,
                         struct pipe_surface           *future_surfaces,*/
                         struct pipe_video_rect        *src_area,
                         struct pipe_surface           *dst_surface,
                         struct pipe_video_rect        *dst_area,
                         struct pipe_fence_handle      **fence)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(src_surface);
   assert(src_area);
   assert(dst_surface);
   assert(dst_area);

   flush_buffer(ctx);

   vl_compositor_render(&ctx->compositor, src_surface,
                        picture_type, src_area, dst_surface, dst_area, fence);
}

static void
vl_mpeg12_set_picture_background(struct pipe_video_context *vpipe,
                                  struct pipe_surface *bg,
                                  struct pipe_video_rect *bg_src_rect)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(bg);
   assert(bg_src_rect);

   vl_compositor_set_background(&ctx->compositor, bg, bg_src_rect);
}

static void
vl_mpeg12_set_picture_layers(struct pipe_video_context *vpipe,
                             struct pipe_surface *layers[],
                             struct pipe_video_rect *src_rects[],
                             struct pipe_video_rect *dst_rects[],
                             unsigned num_layers)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert((layers && src_rects && dst_rects) ||
          (!layers && !src_rects && !dst_rects));

   vl_compositor_set_layers(&ctx->compositor, layers, src_rects, dst_rects, num_layers);
}

static void
vl_mpeg12_set_decode_target(struct pipe_video_context *vpipe,
                            struct pipe_surface *dt)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);
   assert(dt);

   if (ctx->decode_target != dt || ctx->cur_buffer == NULL) {
      rotate_buffer(ctx);

      pipe_surface_reference(&ctx->decode_target, dt);
   }
}

static void
vl_mpeg12_set_csc_matrix(struct pipe_video_context *vpipe, const float *mat)
{
   struct vl_mpeg12_context *ctx = (struct vl_mpeg12_context*)vpipe;

   assert(vpipe);

   vl_compositor_set_csc_matrix(&ctx->compositor, mat);
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
   ctx->pipe->bind_blend_state(ctx->pipe, ctx->blend);

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

struct pipe_video_context *
vl_create_mpeg12_context(struct pipe_context *pipe,
                         enum pipe_video_profile profile,
                         enum pipe_video_chroma_format chroma_format,
                         unsigned width, unsigned height,
                         bool pot_buffers,
                         enum pipe_format decode_format)
{
   struct pipe_resource *idct_matrix;
   unsigned buffer_width, buffer_height;
   unsigned chroma_width, chroma_height, chroma_blocks_x, chroma_blocks_y;
   struct vl_mpeg12_context *ctx;

   assert(u_reduce_video_profile(profile) == PIPE_VIDEO_CODEC_MPEG12);

   ctx = CALLOC_STRUCT(vl_mpeg12_context);

   if (!ctx)
      return NULL;

   /* TODO: Non-pot buffers untested, probably doesn't work without changes to texcoord generation, vert shader, etc */
   assert(pot_buffers);

   buffer_width = pot_buffers ? util_next_power_of_two(width) : width;
   buffer_height = pot_buffers ? util_next_power_of_two(height) : height;

   ctx->base.profile = profile;
   ctx->base.chroma_format = chroma_format;
   ctx->base.width = width;
   ctx->base.height = height;

   ctx->base.screen = pipe->screen;

   ctx->base.destroy = vl_mpeg12_destroy;
   ctx->base.get_param = vl_mpeg12_get_param;
   ctx->base.is_format_supported = vl_mpeg12_is_format_supported;
   ctx->base.create_surface = vl_mpeg12_create_surface;
   ctx->base.decode_macroblocks = vl_mpeg12_decode_macroblocks;
   ctx->base.render_picture = vl_mpeg12_render_picture;
   ctx->base.clear_render_target = vl_mpeg12_clear_render_target;
   ctx->base.resource_copy_region = vl_mpeg12_resource_copy_region;
   ctx->base.get_transfer = vl_mpeg12_get_transfer;
   ctx->base.transfer_destroy = vl_mpeg12_transfer_destroy;
   ctx->base.transfer_map = vl_mpeg12_transfer_map;
   ctx->base.transfer_flush_region = vl_mpeg12_transfer_flush_region;
   ctx->base.transfer_unmap = vl_mpeg12_transfer_unmap;
   if (pipe->transfer_inline_write)
      ctx->base.transfer_inline_write = vl_mpeg12_transfer_inline_write;
   ctx->base.set_picture_background = vl_mpeg12_set_picture_background;
   ctx->base.set_picture_layers = vl_mpeg12_set_picture_layers;
   ctx->base.set_decode_target = vl_mpeg12_set_decode_target;
   ctx->base.set_csc_matrix = vl_mpeg12_set_csc_matrix;

   ctx->pipe = pipe;
   ctx->decode_format = decode_format;

   ctx->quads = vl_vb_upload_quads(ctx->pipe, 2, 2);
   ctx->vertex_buffer_size = width / MACROBLOCK_WIDTH * height / MACROBLOCK_HEIGHT;
   ctx->vertex_elems_state = vl_vb_get_elems_state(ctx->pipe, true);

   if (ctx->vertex_elems_state == NULL) {
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }

   /* TODO: Implement 422, 444 */
   assert(chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);
   ctx->empty_block_mask = &const_empty_block_mask_420;

   if (!(idct_matrix = vl_idct_upload_matrix(ctx->pipe)))
      return false;

   if (!vl_idct_init(&ctx->idct_y, ctx->pipe, buffer_width, buffer_height,
                     2, 2, TGSI_SWIZZLE_X, idct_matrix))
      return false;

   if (chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      chroma_width = buffer_width / 2;
      chroma_height = buffer_height / 2;
      chroma_blocks_x = 1;
      chroma_blocks_y = 1;
   } else if (chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
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

   if(!vl_idct_init(&ctx->idct_cr, ctx->pipe, chroma_width, chroma_height,
                    chroma_blocks_x, chroma_blocks_y, TGSI_SWIZZLE_Z, idct_matrix))
      return false;

   if(!vl_idct_init(&ctx->idct_cb, ctx->pipe, chroma_width, chroma_height,
                    chroma_blocks_x, chroma_blocks_y, TGSI_SWIZZLE_Y, idct_matrix))
      return false;

   if (!vl_mpeg12_mc_renderer_init(&ctx->mc_renderer, ctx->pipe,
                                   buffer_width, buffer_height, chroma_format)) {
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }

   ctx->buffer_map = util_new_keymap(sizeof(unsigned), -1, delete_buffer);
   if (!ctx->buffer_map) {
      vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }

   if (!vl_compositor_init(&ctx->compositor, ctx->pipe)) {
      util_delete_keymap(ctx->buffer_map, ctx);
      vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }

   if (!init_pipe_state(ctx)) {
      vl_compositor_cleanup(&ctx->compositor);
      util_delete_keymap(ctx->buffer_map, ctx);
      vl_mpeg12_mc_renderer_cleanup(&ctx->mc_renderer);
      ctx->pipe->destroy(ctx->pipe);
      FREE(ctx);
      return NULL;
   }

   return &ctx->base;
}
