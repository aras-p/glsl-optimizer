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

#include <math.h>
#include <assert.h>

#include <util/u_memory.h>
#include <util/u_rect.h>
#include <util/u_video.h>

#include "vl_mpeg12_decoder.h"
#include "vl_defines.h"

#define SCALE_FACTOR_SNORM (32768.0f / 256.0f)
#define SCALE_FACTOR_SSCALED (1.0f / 256.0f)

static const unsigned const_empty_block_mask_420[3][2][2] = {
   { { 0x20, 0x10 },  { 0x08, 0x04 } },
   { { 0x02, 0x02 },  { 0x02, 0x02 } },
   { { 0x01, 0x01 },  { 0x01, 0x01 } }
};

static const enum pipe_format const_idct_source_formats[] = {
   PIPE_FORMAT_R16G16B16A16_SNORM,
   PIPE_FORMAT_R16G16B16A16_SSCALED
};

static const unsigned num_idct_source_formats =
   sizeof(const_idct_source_formats) / sizeof(enum pipe_format);

static const enum pipe_format const_idct_intermediate_formats[] = {
   PIPE_FORMAT_R16G16B16A16_FLOAT,
   PIPE_FORMAT_R16G16B16A16_SNORM,
   PIPE_FORMAT_R16G16B16A16_SSCALED,
   PIPE_FORMAT_R32G32B32A32_FLOAT
};

static const unsigned num_idct_intermediate_formats =
   sizeof(const_idct_intermediate_formats) / sizeof(enum pipe_format);

static const enum pipe_format const_mc_source_formats[] = {
   PIPE_FORMAT_R16_SNORM,
   PIPE_FORMAT_R16_SSCALED
};

static const unsigned num_mc_source_formats =
   sizeof(const_mc_source_formats) / sizeof(enum pipe_format);

static void
map_buffers(struct vl_mpeg12_decoder *ctx, struct vl_mpeg12_buffer *buffer)
{
   struct pipe_sampler_view **sampler_views;
   struct pipe_resource *tex;
   unsigned i;

   assert(ctx && buffer);

   if (ctx->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      sampler_views = buffer->idct_source->get_sampler_views(buffer->idct_source);
   else
      sampler_views = buffer->mc_source->get_sampler_views(buffer->mc_source);
   assert(sampler_views);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      tex = sampler_views[i]->texture;

      struct pipe_box rect =
      {
         0, 0, 0,
         tex->width0,
         tex->height0,
         1
      };

      buffer->tex_transfer[i] = ctx->pipe->get_transfer
      (
         ctx->pipe, tex,
         0, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
         &rect
      );

      buffer->texels[i] = ctx->pipe->transfer_map(ctx->pipe, buffer->tex_transfer[i]);
   }
}

static void
upload_block(struct vl_mpeg12_buffer *buffer, unsigned plane, unsigned x, unsigned y, short *block)
{
   unsigned tex_pitch;
   short *texels;

   unsigned i;

   assert(buffer);
   assert(block);

   tex_pitch = buffer->tex_transfer[plane]->stride / sizeof(short);
   texels = buffer->texels[plane] + y * tex_pitch * BLOCK_HEIGHT + x * BLOCK_WIDTH;

   for (i = 0; i < BLOCK_HEIGHT; ++i)
      memcpy(texels + i * tex_pitch, block + i * BLOCK_WIDTH, BLOCK_WIDTH * sizeof(short));
}

static void
upload_buffer(struct vl_mpeg12_decoder *ctx,
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
            upload_block(buffer, 0, mb->mbx * 2 + x, mb->mby * 2 + y, blocks);
            blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
         }
      }
   }

   /* TODO: Implement 422, 444 */
   assert(ctx->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   for (tb = 1; tb < 3; ++tb) {
      if (mb->cbp & (*ctx->empty_block_mask)[tb][0][0]) {
         upload_block(buffer, tb, mb->mbx, mb->mby, blocks);
         blocks += BLOCK_WIDTH * BLOCK_HEIGHT;
      }
   }
}

static void
unmap_buffers(struct vl_mpeg12_decoder *ctx, struct vl_mpeg12_buffer *buffer)
{
   unsigned i;

   assert(ctx && buffer);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      ctx->pipe->transfer_unmap(ctx->pipe, buffer->tex_transfer[i]);
      ctx->pipe->transfer_destroy(ctx->pipe, buffer->tex_transfer[i]);
   }
}

static void
cleanup_idct_buffer(struct vl_mpeg12_buffer *buf)
{
   struct vl_mpeg12_decoder *dec;
   assert(buf);

   dec = (struct vl_mpeg12_decoder*)buf->base.decoder;
   assert(dec);

   buf->idct_source->destroy(buf->idct_source);
   buf->idct_intermediate->destroy(buf->idct_intermediate);
   vl_idct_cleanup_buffer(&dec->idct_y, &buf->idct[0]);
   vl_idct_cleanup_buffer(&dec->idct_c, &buf->idct[1]);
   vl_idct_cleanup_buffer(&dec->idct_c, &buf->idct[2]);
}

static void
vl_mpeg12_buffer_destroy(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;
   unsigned i;

   assert(buf);

   dec = (struct vl_mpeg12_decoder*)buf->base.decoder;
   assert(dec);

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      cleanup_idct_buffer(buf);

   buf->mc_source->destroy(buf->mc_source);
   vl_vb_cleanup(&buf->vertex_stream);
   for (i = 0; i < VL_MAX_PLANES; ++i)
      vl_mc_cleanup_buffer(&buf->mc[i]);

   FREE(buf);
}

static void
vl_mpeg12_buffer_map(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;
   assert(buf);

   dec = (struct vl_mpeg12_decoder *)buf->base.decoder;
   assert(dec);

   vl_vb_map(&buf->vertex_stream, dec->pipe);
   map_buffers(dec, buf);
}

static void
vl_mpeg12_buffer_add_macroblocks(struct pipe_video_decode_buffer *buffer,
                                 unsigned num_macroblocks,
                                 struct pipe_macroblock *macroblocks)
{
   struct pipe_mpeg12_macroblock *mb = (struct pipe_mpeg12_macroblock*)macroblocks;
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;
   unsigned i;

   assert(buf);

   dec =  (struct vl_mpeg12_decoder*)buf->base.decoder;
   assert(dec);

   assert(num_macroblocks);
   assert(macroblocks);
   assert(macroblocks->codec == PIPE_VIDEO_CODEC_MPEG12);

   for ( i = 0; i < num_macroblocks; ++i ) {
      vl_vb_add_block(&buf->vertex_stream, &mb[i], dec->empty_block_mask);
      upload_buffer(dec, buf, &mb[i]);
   }
}

static void
vl_mpeg12_buffer_unmap(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;
   assert(buf);

   dec = (struct vl_mpeg12_decoder *)buf->base.decoder;
   assert(dec);

   vl_vb_unmap(&buf->vertex_stream, dec->pipe);
   unmap_buffers(dec, buf);
}

static void
vl_mpeg12_destroy(struct pipe_video_decoder *decoder)
{
   struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder*)decoder;
   unsigned i;

   assert(decoder);

   /* Asserted in softpipe_delete_fs_state() for some reason */
   dec->pipe->bind_vs_state(dec->pipe, NULL);
   dec->pipe->bind_fs_state(dec->pipe, NULL);

   dec->pipe->delete_blend_state(dec->pipe, dec->blend);
   dec->pipe->delete_depth_stencil_alpha_state(dec->pipe, dec->dsa);

   vl_mc_cleanup(&dec->mc);
   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      vl_idct_cleanup(&dec->idct_y);
      vl_idct_cleanup(&dec->idct_c);
   }
   for (i = 0; i < VL_MAX_PLANES; ++i)
      dec->pipe->delete_vertex_elements_state(dec->pipe, dec->ves_eb[i]);

   for (i = 0; i < 2; ++i)
      dec->pipe->delete_vertex_elements_state(dec->pipe, dec->ves_mv[i]);

   pipe_resource_reference(&dec->quads.buffer, NULL);

   FREE(dec);
}

static bool
init_idct_buffer(struct vl_mpeg12_buffer *buffer)
{
   enum pipe_format formats[3];

   struct pipe_sampler_view **idct_source_sv, **idct_intermediate_sv;
   struct pipe_surface **idct_surfaces;

   struct vl_mpeg12_decoder *dec;

   unsigned i;

   assert(buffer);

   dec = (struct vl_mpeg12_decoder*)buffer->base.decoder;

   formats[0] = formats[1] = formats[2] = dec->idct_source_format;
   buffer->idct_source = vl_video_buffer_init(dec->base.context, dec->pipe,
                                              dec->base.width / 4, dec->base.height, 1,
                                              dec->base.chroma_format,
                                              formats, PIPE_USAGE_STREAM);
   if (!buffer->idct_source)
      goto error_source;

   formats[0] = formats[1] = formats[2] = dec->idct_intermediate_format;
   buffer->idct_intermediate = vl_video_buffer_init(dec->base.context, dec->pipe,
                                                    dec->base.width / dec->nr_of_idct_render_targets,
                                                    dec->base.height / 4, dec->nr_of_idct_render_targets,
                                                    dec->base.chroma_format,
                                                    formats, PIPE_USAGE_STATIC);

   if (!buffer->idct_intermediate)
      goto error_intermediate;

   idct_source_sv = buffer->idct_source->get_sampler_views(buffer->idct_source);
   if (!idct_source_sv)
      goto error_source_sv;

   idct_intermediate_sv = buffer->idct_intermediate->get_sampler_views(buffer->idct_intermediate);
   if (!idct_intermediate_sv)
      goto error_intermediate_sv;

   idct_surfaces = buffer->mc_source->get_surfaces(buffer->mc_source);
   if (!idct_surfaces)
      goto error_surfaces;

   for (i = 0; i < 3; ++i)
      if (!vl_idct_init_buffer(i == 0 ? &dec->idct_y : &dec->idct_c,
                               &buffer->idct[i], idct_source_sv[i],
                               idct_intermediate_sv[i], idct_surfaces[i]))
         goto error_plane;

   return true;

error_plane:
   for (; i > 0; --i)
      vl_idct_cleanup_buffer(i == 1 ? &dec->idct_c : &dec->idct_y, &buffer->idct[i - 1]);

error_surfaces:
error_intermediate_sv:
error_source_sv:
   buffer->idct_intermediate->destroy(buffer->idct_intermediate);

error_intermediate:
   buffer->idct_source->destroy(buffer->idct_source);

error_source:
   return false;
}

static struct pipe_video_decode_buffer *
vl_mpeg12_create_buffer(struct pipe_video_decoder *decoder)
{
   enum pipe_format formats[3];

   struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder*)decoder;
   struct vl_mpeg12_buffer *buffer;

   struct pipe_sampler_view **mc_source_sv;

   assert(dec);

   buffer = CALLOC_STRUCT(vl_mpeg12_buffer);
   if (buffer == NULL)
      return NULL;

   buffer->base.decoder = decoder;
   buffer->base.destroy = vl_mpeg12_buffer_destroy;
   buffer->base.map = vl_mpeg12_buffer_map;
   buffer->base.add_macroblocks = vl_mpeg12_buffer_add_macroblocks;
   buffer->base.unmap = vl_mpeg12_buffer_unmap;

   buffer->vertex_bufs.individual.quad.stride = dec->quads.stride;
   buffer->vertex_bufs.individual.quad.buffer_offset = dec->quads.buffer_offset;
   pipe_resource_reference(&buffer->vertex_bufs.individual.quad.buffer, dec->quads.buffer);

   buffer->vertex_bufs.individual.stream = vl_vb_init(&buffer->vertex_stream, dec->pipe,
                                                      dec->base.width / MACROBLOCK_WIDTH *
                                                      dec->base.height / MACROBLOCK_HEIGHT);
   if (!buffer->vertex_bufs.individual.stream.buffer)
      goto error_vertex_stream;

   formats[0] = formats[1] = formats[2] =dec->mc_source_format;
   buffer->mc_source = vl_video_buffer_init(dec->base.context, dec->pipe,
                                            dec->base.width, dec->base.height, 1,
                                            dec->base.chroma_format,
                                            formats, PIPE_USAGE_STATIC);

   if (!buffer->mc_source)
      goto error_mc_source;

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      if (!init_idct_buffer(buffer))
         goto error_idct;

   mc_source_sv = buffer->mc_source->get_sampler_views(buffer->mc_source);
   if (!mc_source_sv)
      goto error_mc_source_sv;

   if(!vl_mc_init_buffer(&dec->mc, &buffer->mc[0], mc_source_sv[0]))
      goto error_mc_y;

   if(!vl_mc_init_buffer(&dec->mc, &buffer->mc[1], mc_source_sv[1]))
      goto error_mc_cb;

   if(!vl_mc_init_buffer(&dec->mc, &buffer->mc[2], mc_source_sv[2]))
      goto error_mc_cr;

   return &buffer->base;

error_mc_cr:
   vl_mc_cleanup_buffer(&buffer->mc[1]);

error_mc_cb:
   vl_mc_cleanup_buffer(&buffer->mc[0]);

error_mc_y:
error_mc_source_sv:
   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      cleanup_idct_buffer(buffer);

error_idct:
   buffer->mc_source->destroy(buffer->mc_source);

error_mc_source:
   vl_vb_cleanup(&buffer->vertex_stream);

error_vertex_stream:
   FREE(buffer);
   return NULL;
}

static void
vl_mpeg12_decoder_flush_buffer(struct pipe_video_decode_buffer *buffer,
                               struct pipe_video_buffer *refs[2],
                               struct pipe_video_buffer *dst,
                               struct pipe_fence_handle **fence)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer *)buffer;
   struct vl_mpeg12_decoder *dec;

   struct pipe_sampler_view **sv[2];
   struct pipe_surface **surfaces;

   unsigned ne_start, ne_num, e_start, e_num;
   unsigned i, j;

   assert(buf);

   dec = (struct vl_mpeg12_decoder *)buf->base.decoder;
   assert(dec);

   for (i = 0; i < 2; ++i)
      sv[i] = refs[i] ? refs[i]->get_sampler_views(refs[i]) : NULL;

   surfaces = dst->get_surfaces(dst);

   vl_vb_restart(&buf->vertex_stream, &ne_start, &ne_num, &e_start, &e_num);

   dec->pipe->set_vertex_buffers(dec->pipe, 2, buf->vertex_bufs.all);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      bool first = true;

      vl_mc_set_surface(&dec->mc, surfaces[i]);

      for (j = 0; j < 2; ++j) {
         if (sv[j] == NULL) continue;

         dec->pipe->bind_vertex_elements_state(dec->pipe, dec->ves_mv[j]);
         vl_mc_render_ref(&buf->mc[i], sv[j][i], first, ne_start, ne_num, e_start, e_num);
         first = false;
      }

      dec->pipe->bind_blend_state(dec->pipe, dec->blend);
      dec->pipe->bind_vertex_elements_state(dec->pipe, dec->ves_eb[i]);

      if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
         vl_idct_flush(i == 0 ? &dec->idct_y : &dec->idct_c, &buf->idct[i], ne_num);

      vl_mc_render_ycbcr(&buf->mc[i], first, ne_start, ne_num);

   }
   dec->pipe->flush(dec->pipe, fence);
}

static void
vl_mpeg12_decoder_clear_buffer(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer *)buffer;
   unsigned ne_start, ne_num, e_start, e_num;

   assert(buf);

   vl_vb_restart(&buf->vertex_stream, &ne_start, &ne_num, &e_start, &e_num);
}

static bool
init_pipe_state(struct vl_mpeg12_decoder *dec)
{
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state dsa;
   unsigned i;

   assert(dec);

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
   dec->blend = dec->pipe->create_blend_state(dec->pipe, &blend);

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
   dec->dsa = dec->pipe->create_depth_stencil_alpha_state(dec->pipe, &dsa);
   dec->pipe->bind_depth_stencil_alpha_state(dec->pipe, dec->dsa);

   return true;
}

static enum pipe_format
find_first_supported_format(struct vl_mpeg12_decoder *dec,
                            const enum pipe_format formats[],
                            unsigned num_formats,
                            enum pipe_texture_target target)
{
   struct pipe_screen *screen;
   unsigned i;

   assert(dec);

   screen = dec->pipe->screen;

   for (i = 0; i < num_formats; ++i)
      if (screen->is_format_supported(dec->pipe->screen, formats[i], target, 1,
                                      PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET))
         return formats[i];

   return PIPE_FORMAT_NONE;
}

static bool
init_idct(struct vl_mpeg12_decoder *dec, unsigned buffer_width, unsigned buffer_height)
{
   unsigned chroma_width, chroma_height, chroma_blocks_x, chroma_blocks_y;
   struct pipe_sampler_view *matrix, *transpose;
   float matrix_scale, transpose_scale;

   dec->nr_of_idct_render_targets = dec->pipe->screen->get_param(dec->pipe->screen, PIPE_CAP_MAX_RENDER_TARGETS);

   // more than 4 render targets usually doesn't makes any seens
   dec->nr_of_idct_render_targets = MIN2(dec->nr_of_idct_render_targets, 4);

   dec->idct_source_format = find_first_supported_format(dec, const_idct_source_formats,
                                                         num_idct_source_formats, PIPE_TEXTURE_2D);

   if (dec->idct_source_format == PIPE_FORMAT_NONE)
      return false;

   dec->idct_intermediate_format = find_first_supported_format(dec, const_idct_intermediate_formats,
                                                               num_idct_intermediate_formats, PIPE_TEXTURE_3D);

   if (dec->idct_intermediate_format == PIPE_FORMAT_NONE)
      return false;

   switch (dec->idct_source_format) {
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
      matrix_scale = SCALE_FACTOR_SSCALED;
      break;

   case PIPE_FORMAT_R16G16B16A16_SNORM:
      matrix_scale = SCALE_FACTOR_SNORM;
      break;

   default:
      assert(0);
      return false;
   }

   if (dec->idct_intermediate_format == PIPE_FORMAT_R16G16B16A16_FLOAT ||
       dec->idct_intermediate_format == PIPE_FORMAT_R32G32B32A32_FLOAT)
      transpose_scale = 1.0f;
   else
      transpose_scale = matrix_scale = sqrt(matrix_scale);

   if (dec->mc_source_format == PIPE_FORMAT_R16_SSCALED)
      transpose_scale /= SCALE_FACTOR_SSCALED;

   if (!(matrix = vl_idct_upload_matrix(dec->pipe, matrix_scale)))
      goto error_matrix;

   if (matrix_scale != transpose_scale) {
      if (!(transpose = vl_idct_upload_matrix(dec->pipe, transpose_scale)))
         goto error_transpose;
   } else
      pipe_sampler_view_reference(&transpose, matrix);

   if (!vl_idct_init(&dec->idct_y, dec->pipe, buffer_width, buffer_height,
                     2, 2, dec->nr_of_idct_render_targets, matrix, transpose))
      goto error_y;

   if (dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      chroma_width = buffer_width / 2;
      chroma_height = buffer_height / 2;
      chroma_blocks_x = 1;
      chroma_blocks_y = 1;
   } else if (dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
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

   if(!vl_idct_init(&dec->idct_c, dec->pipe, chroma_width, chroma_height,
                    chroma_blocks_x, chroma_blocks_y,
                    dec->nr_of_idct_render_targets, matrix, transpose))
      goto error_c;

   pipe_sampler_view_reference(&matrix, NULL);
   pipe_sampler_view_reference(&transpose, NULL);
   return true;

error_c:
   vl_idct_cleanup(&dec->idct_y);

error_y:
   pipe_sampler_view_reference(&transpose, NULL);

error_transpose:
   pipe_sampler_view_reference(&matrix, NULL);

error_matrix:
   return false;
}

struct pipe_video_decoder *
vl_create_mpeg12_decoder(struct pipe_video_context *context,
                         struct pipe_context *pipe,
                         enum pipe_video_profile profile,
                         enum pipe_video_entrypoint entrypoint,
                         enum pipe_video_chroma_format chroma_format,
                         unsigned width, unsigned height)
{
   struct vl_mpeg12_decoder *dec;
   float mc_scale;
   unsigned i;

   assert(u_reduce_video_profile(profile) == PIPE_VIDEO_CODEC_MPEG12);

   dec = CALLOC_STRUCT(vl_mpeg12_decoder);

   if (!dec)
      return NULL;

   dec->base.context = context;
   dec->base.profile = profile;
   dec->base.entrypoint = entrypoint;
   dec->base.chroma_format = chroma_format;
   dec->base.width = width;
   dec->base.height = height;

   dec->base.destroy = vl_mpeg12_destroy;
   dec->base.create_buffer = vl_mpeg12_create_buffer;
   dec->base.flush_buffer = vl_mpeg12_decoder_flush_buffer;
   dec->base.clear_buffer = vl_mpeg12_decoder_clear_buffer;

   dec->pipe = pipe;

   dec->quads = vl_vb_upload_quads(dec->pipe, 2, 2);
   for (i = 0; i < VL_MAX_PLANES; ++i)
      dec->ves_eb[i] = vl_vb_get_elems_state(dec->pipe, i, 0);

   for (i = 0; i < 2; ++i)
      dec->ves_mv[i] = vl_vb_get_elems_state(dec->pipe, 0, i);

   dec->base.width = align(width, MACROBLOCK_WIDTH);
   dec->base.height = align(height, MACROBLOCK_HEIGHT);

   /* TODO: Implement 422, 444 */
   assert(dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);
   dec->empty_block_mask = &const_empty_block_mask_420;

   dec->mc_source_format = find_first_supported_format(dec, const_mc_source_formats,
                                                       num_mc_source_formats, PIPE_TEXTURE_3D);

   if (dec->mc_source_format == PIPE_FORMAT_NONE)
      return NULL;

   if (entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      if (!init_idct(dec, dec->base.width, dec->base.height))
         goto error_idct;
      if (dec->mc_source_format == PIPE_FORMAT_R16_SSCALED)
         mc_scale = SCALE_FACTOR_SSCALED;
      else
         mc_scale = 1.0f;
   } else {
      switch (dec->mc_source_format) {
      case PIPE_FORMAT_R16_SNORM:
         mc_scale = SCALE_FACTOR_SNORM;
         break;

      case PIPE_FORMAT_R16_SSCALED:
         mc_scale = SCALE_FACTOR_SSCALED;
         break;

      default:
         assert(0);
         return NULL;
      }
   }

   if (!vl_mc_init(&dec->mc, dec->pipe, dec->base.width, dec->base.height, mc_scale))
      goto error_mc;

   if (!init_pipe_state(dec))
      goto error_pipe_state;

   return &dec->base;

error_pipe_state:
   vl_mc_cleanup(&dec->mc);

error_mc:
   if (entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      vl_idct_cleanup(&dec->idct_y);
      vl_idct_cleanup(&dec->idct_c);
   }

error_idct:
   FREE(dec);
   return NULL;
}
