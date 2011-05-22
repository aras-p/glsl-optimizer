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

struct format_config {
   enum pipe_format zscan_source_format;
   enum pipe_format idct_source_format;
   enum pipe_format mc_source_format;

   float idct_scale;
   float mc_scale;
};

static const struct format_config bitstream_format_config[] = {
   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, 1.0f, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SNORM },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, 1.0f, SCALE_FACTOR_SNORM }
};

static const unsigned num_bitstream_format_configs =
   sizeof(bitstream_format_config) / sizeof(struct format_config);

static const struct format_config idct_format_config[] = {
   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, PIPE_FORMAT_R16G16B16A16_SSCALED, 1.0f, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_FLOAT, 1.0f, SCALE_FACTOR_SNORM },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, PIPE_FORMAT_R16G16B16A16_SNORM, 1.0f, SCALE_FACTOR_SNORM }
};

static const unsigned num_idct_format_configs =
   sizeof(idct_format_config) / sizeof(struct format_config);

static const struct format_config mc_format_config[] = {
   //{ PIPE_FORMAT_R16_SSCALED, PIPE_FORMAT_NONE, PIPE_FORMAT_R16_SSCALED, 0.0f, SCALE_FACTOR_SSCALED },
   { PIPE_FORMAT_R16_SNORM, PIPE_FORMAT_NONE, PIPE_FORMAT_R16_SNORM, 0.0f, SCALE_FACTOR_SNORM }
};

static const unsigned num_mc_format_configs =
   sizeof(mc_format_config) / sizeof(struct format_config);

static bool
init_zscan_buffer(struct vl_mpeg12_buffer *buffer)
{
   enum pipe_format formats[3];

   struct pipe_sampler_view **source;
   struct pipe_surface **destination;

   struct vl_mpeg12_decoder *dec;

   unsigned i;

   assert(buffer);

   dec = (struct vl_mpeg12_decoder*)buffer->base.decoder;

   formats[0] = formats[1] = formats[2] = dec->zscan_source_format;
   buffer->zscan_source = vl_video_buffer_init(dec->base.context, dec->pipe,
                                               dec->blocks_per_line * BLOCK_WIDTH * BLOCK_HEIGHT,
                                               dec->max_blocks / dec->blocks_per_line,
                                               1, PIPE_VIDEO_CHROMA_FORMAT_444,
                                               formats, PIPE_USAGE_STATIC);
   if (!buffer->zscan_source)
      goto error_source;

   source = buffer->zscan_source->get_sampler_view_planes(buffer->zscan_source);
   if (!source)
      goto error_sampler;

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      destination = dec->idct_source->get_surfaces(dec->idct_source);
   else
      destination = dec->mc_source->get_surfaces(dec->mc_source);

   if (!destination)
      goto error_surface;

   for (i = 0; i < VL_MAX_PLANES; ++i)
      if (!vl_zscan_init_buffer(i == 0 ? &dec->zscan_y : &dec->zscan_c,
                                &buffer->zscan[i], source[i], destination[i]))
         goto error_plane;

   return true;

error_plane:
   for (; i > 0; --i)
      vl_zscan_cleanup_buffer(&buffer->zscan[i - 1]);

error_surface:
error_sampler:
   buffer->zscan_source->destroy(buffer->zscan_source);

error_source:
   return false;
}

static void
cleanup_zscan_buffer(struct vl_mpeg12_buffer *buffer)
{
   unsigned i;

   assert(buffer);

   for (i = 0; i < VL_MAX_PLANES; ++i)
      vl_zscan_cleanup_buffer(&buffer->zscan[i]);
   buffer->zscan_source->destroy(buffer->zscan_source);
}

static bool
init_idct_buffer(struct vl_mpeg12_buffer *buffer)
{
   struct pipe_sampler_view **idct_source_sv, **mc_source_sv;
   struct pipe_surface **idct_surfaces;

   struct vl_mpeg12_decoder *dec;

   unsigned i;

   assert(buffer);

   dec = (struct vl_mpeg12_decoder*)buffer->base.decoder;

   idct_source_sv = dec->idct_source->get_sampler_view_planes(dec->idct_source);
   if (!idct_source_sv)
      goto error_source_sv;

   mc_source_sv = dec->mc_source->get_sampler_view_planes(dec->mc_source);
   if (!mc_source_sv)
      goto error_mc_source_sv;

   idct_surfaces = dec->mc_source->get_surfaces(dec->mc_source);
   if (!idct_surfaces)
      goto error_surfaces;

   for (i = 0; i < 3; ++i)
      if (!vl_idct_init_buffer(i == 0 ? &dec->idct_y : &dec->idct_c,
                               &buffer->idct[i], idct_source_sv[i],
                               mc_source_sv[i], idct_surfaces[i]))
         goto error_plane;

   return true;

error_plane:
   for (; i > 0; --i)
      vl_idct_cleanup_buffer(i == 1 ? &dec->idct_c : &dec->idct_y, &buffer->idct[i - 1]);

error_surfaces:
error_mc_source_sv:
error_source_sv:
   return false;
}

static void
cleanup_idct_buffer(struct vl_mpeg12_buffer *buf)
{
   struct vl_mpeg12_decoder *dec;
   assert(buf);

   dec = (struct vl_mpeg12_decoder*)buf->base.decoder;
   assert(dec);

   vl_idct_cleanup_buffer(&dec->idct_y, &buf->idct[0]);
   vl_idct_cleanup_buffer(&dec->idct_c, &buf->idct[1]);
   vl_idct_cleanup_buffer(&dec->idct_c, &buf->idct[2]);
}

static bool
init_mc_buffer(struct vl_mpeg12_buffer *buf)
{
   struct vl_mpeg12_decoder *dec;

   assert(buf);

   dec = (struct vl_mpeg12_decoder*)buf->base.decoder;
   assert(dec);

   if(!vl_mc_init_buffer(&dec->mc_y, &buf->mc[0]))
      goto error_mc_y;

   if(!vl_mc_init_buffer(&dec->mc_c, &buf->mc[1]))
      goto error_mc_cb;

   if(!vl_mc_init_buffer(&dec->mc_c, &buf->mc[2]))
      goto error_mc_cr;

   return true;

error_mc_cr:
   vl_mc_cleanup_buffer(&buf->mc[1]);

error_mc_cb:
   vl_mc_cleanup_buffer(&buf->mc[0]);

error_mc_y:
   return false;
}

static void
cleanup_mc_buffer(struct vl_mpeg12_buffer *buf)
{
   unsigned i;

   assert(buf);

   for (i = 0; i < VL_MAX_PLANES; ++i)
      vl_mc_cleanup_buffer(&buf->mc[i]);
}

static void
vl_mpeg12_buffer_destroy(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;

   assert(buf);

   dec = (struct vl_mpeg12_decoder*)buf->base.decoder;
   assert(dec);

   cleanup_zscan_buffer(buf);

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      cleanup_idct_buffer(buf);

   cleanup_mc_buffer(buf);

   vl_vb_cleanup(&buf->vertex_stream);

   FREE(buf);
}

static void
vl_mpeg12_buffer_map(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;

   struct pipe_sampler_view **sampler_views;
   unsigned i;

   assert(buf);

   dec = (struct vl_mpeg12_decoder *)buf->base.decoder;
   assert(dec);

   vl_vb_map(&buf->vertex_stream, dec->pipe);

   sampler_views = buf->zscan_source->get_sampler_view_planes(buf->zscan_source);

   assert(sampler_views);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      struct pipe_resource *tex = sampler_views[i]->texture;
      struct pipe_box rect =
      {
         0, 0, 0,
         tex->width0,
         tex->height0,
         1
      };

      buf->tex_transfer[i] = dec->pipe->get_transfer
      (
         dec->pipe, tex,
         0, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
         &rect
      );

      buf->texels[i] = dec->pipe->transfer_map(dec->pipe, buf->tex_transfer[i]);
   }

   if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      struct pipe_ycbcr_block *ycbcr_stream[VL_MAX_PLANES];
      struct pipe_motionvector *mv_stream[VL_MAX_REF_FRAMES];

      for (i = 0; i < VL_MAX_PLANES; ++i)
         ycbcr_stream[i] = vl_vb_get_ycbcr_stream(&buf->vertex_stream, i);

      for (i = 0; i < VL_MAX_REF_FRAMES; ++i)
         mv_stream[i] = vl_vb_get_mv_stream(&buf->vertex_stream, i);

      vl_mpg12_bs_set_buffers(&buf->bs, ycbcr_stream, buf->texels, mv_stream);
   } else {
      for (i = 0; i < VL_MAX_PLANES; ++i)
         vl_zscan_set_layout(&buf->zscan[i], dec->zscan_linear);
   }
}

static struct pipe_ycbcr_block *
vl_mpeg12_buffer_get_ycbcr_stream(struct pipe_video_decode_buffer *buffer, int component)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;

   assert(buf);

   return vl_vb_get_ycbcr_stream(&buf->vertex_stream, component);
}

static short *
vl_mpeg12_buffer_get_ycbcr_buffer(struct pipe_video_decode_buffer *buffer, int component)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;

   assert(buf);
   assert(component < VL_MAX_PLANES);

   return buf->texels[component];
}

static unsigned
vl_mpeg12_buffer_get_mv_stream_stride(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;

   assert(buf);

   return vl_vb_get_mv_stream_stride(&buf->vertex_stream);
}

static struct pipe_motionvector *
vl_mpeg12_buffer_get_mv_stream(struct pipe_video_decode_buffer *buffer, int ref_frame)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;

   assert(buf);

   return vl_vb_get_mv_stream(&buf->vertex_stream, ref_frame);
}

static void
vl_mpeg12_buffer_decode_bitstream(struct pipe_video_decode_buffer *buffer,
                                  unsigned num_bytes, const void *data,
                                  struct pipe_mpeg12_picture_desc *picture,
                                  unsigned num_ycbcr_blocks[3])
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;
   unsigned i;

   assert(buf);

   dec = (struct vl_mpeg12_decoder *)buf->base.decoder;
   assert(dec);

   for (i = 0; i < VL_MAX_PLANES; ++i)
      vl_zscan_set_layout(&buf->zscan[i], picture->alternate_scan ? dec->zscan_alternate : dec->zscan_normal);

   vl_mpg12_bs_decode(&buf->bs, num_bytes, data, picture, num_ycbcr_blocks);
}

static void
vl_mpeg12_buffer_unmap(struct pipe_video_decode_buffer *buffer)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer*)buffer;
   struct vl_mpeg12_decoder *dec;
   unsigned i;

   assert(buf);

   dec = (struct vl_mpeg12_decoder *)buf->base.decoder;
   assert(dec);

   vl_vb_unmap(&buf->vertex_stream, dec->pipe);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      dec->pipe->transfer_unmap(dec->pipe, buf->tex_transfer[i]);
      dec->pipe->transfer_destroy(dec->pipe, buf->tex_transfer[i]);
   }
}

static void
vl_mpeg12_destroy(struct pipe_video_decoder *decoder)
{
   struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder*)decoder;

   assert(decoder);

   /* Asserted in softpipe_delete_fs_state() for some reason */
   dec->pipe->bind_vs_state(dec->pipe, NULL);
   dec->pipe->bind_fs_state(dec->pipe, NULL);

   dec->pipe->delete_depth_stencil_alpha_state(dec->pipe, dec->dsa);
   dec->pipe->delete_sampler_state(dec->pipe, dec->sampler_ycbcr);

   vl_mc_cleanup(&dec->mc_y);
   vl_mc_cleanup(&dec->mc_c);
   dec->mc_source->destroy(dec->mc_source);

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      vl_idct_cleanup(&dec->idct_y);
      vl_idct_cleanup(&dec->idct_c);
      dec->idct_source->destroy(dec->idct_source);
   }

   vl_zscan_cleanup(&dec->zscan_y);
   vl_zscan_cleanup(&dec->zscan_c);

   dec->pipe->delete_vertex_elements_state(dec->pipe, dec->ves_ycbcr);
   dec->pipe->delete_vertex_elements_state(dec->pipe, dec->ves_mv);

   pipe_resource_reference(&dec->quads.buffer, NULL);
   pipe_resource_reference(&dec->pos.buffer, NULL);

   pipe_sampler_view_reference(&dec->zscan_linear, NULL);
   pipe_sampler_view_reference(&dec->zscan_normal, NULL);
   pipe_sampler_view_reference(&dec->zscan_alternate, NULL);

   FREE(dec);
}

static struct pipe_video_decode_buffer *
vl_mpeg12_create_buffer(struct pipe_video_decoder *decoder)
{
   struct vl_mpeg12_decoder *dec = (struct vl_mpeg12_decoder*)decoder;
   struct vl_mpeg12_buffer *buffer;

   assert(dec);

   buffer = CALLOC_STRUCT(vl_mpeg12_buffer);
   if (buffer == NULL)
      return NULL;

   buffer->base.decoder = decoder;
   buffer->base.destroy = vl_mpeg12_buffer_destroy;
   buffer->base.map = vl_mpeg12_buffer_map;
   buffer->base.get_ycbcr_stream = vl_mpeg12_buffer_get_ycbcr_stream;
   buffer->base.get_ycbcr_buffer = vl_mpeg12_buffer_get_ycbcr_buffer;
   buffer->base.get_mv_stream_stride = vl_mpeg12_buffer_get_mv_stream_stride;
   buffer->base.get_mv_stream = vl_mpeg12_buffer_get_mv_stream;
   buffer->base.decode_bitstream = vl_mpeg12_buffer_decode_bitstream;
   buffer->base.unmap = vl_mpeg12_buffer_unmap;

   if (!vl_vb_init(&buffer->vertex_stream, dec->pipe,
                   dec->base.width / MACROBLOCK_WIDTH,
                   dec->base.height / MACROBLOCK_HEIGHT))
      goto error_vertex_buffer;

   if (!init_mc_buffer(buffer))
      goto error_mc;

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      if (!init_idct_buffer(buffer))
         goto error_idct;

   if (!init_zscan_buffer(buffer))
      goto error_zscan;

   if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
      vl_mpg12_bs_init(&buffer->bs,
                       dec->base.width / MACROBLOCK_WIDTH,
                       dec->base.height / MACROBLOCK_HEIGHT);

   return &buffer->base;

error_zscan:
   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
      cleanup_idct_buffer(buffer);

error_idct:
   cleanup_mc_buffer(buffer);

error_mc:
   vl_vb_cleanup(&buffer->vertex_stream);

error_vertex_buffer:
   FREE(buffer);
   return NULL;
}

static void
vl_mpeg12_decoder_flush_buffer(struct pipe_video_decode_buffer *buffer,
                               unsigned num_ycbcr_blocks[3],
                               struct pipe_video_buffer *refs[2],
                               struct pipe_video_buffer *dst)
{
   struct vl_mpeg12_buffer *buf = (struct vl_mpeg12_buffer *)buffer;
   struct vl_mpeg12_decoder *dec;

   struct pipe_sampler_view **sv[VL_MAX_REF_FRAMES], **mc_source_sv;
   struct pipe_surface **surfaces;

   struct pipe_vertex_buffer vb[3];

   unsigned i, j, component;
   unsigned nr_components;

   assert(buf);

   dec = (struct vl_mpeg12_decoder *)buf->base.decoder;
   assert(dec);

   for (i = 0; i < 2; ++i)
      sv[i] = refs[i] ? refs[i]->get_sampler_view_planes(refs[i]) : NULL;

   vb[0] = dec->quads;
   vb[1] = dec->pos;

   surfaces = dst->get_surfaces(dst);

   dec->pipe->bind_vertex_elements_state(dec->pipe, dec->ves_mv);
   for (i = 0; i < VL_MAX_PLANES; ++i) {
      if (!surfaces[i]) continue;

      vl_mc_set_surface(&buf->mc[i], surfaces[i]);

      for (j = 0; j < VL_MAX_REF_FRAMES; ++j) {
         if (!sv[j]) continue;

         vb[2] = vl_vb_get_mv(&buf->vertex_stream, j);;
         dec->pipe->set_vertex_buffers(dec->pipe, 3, vb);

         vl_mc_render_ref(&buf->mc[i], sv[j][i]);
      }
   }

   dec->pipe->bind_vertex_elements_state(dec->pipe, dec->ves_ycbcr);
   for (i = 0; i < VL_MAX_PLANES; ++i) {
      if (!num_ycbcr_blocks[i]) continue;

      vb[1] = vl_vb_get_ycbcr(&buf->vertex_stream, i);
      dec->pipe->set_vertex_buffers(dec->pipe, 2, vb);

      vl_zscan_render(&buf->zscan[i] , num_ycbcr_blocks[i]);

      if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
         vl_idct_flush(i == 0 ? &dec->idct_y : &dec->idct_c, &buf->idct[i], num_ycbcr_blocks[i]);
   }

   mc_source_sv = dec->mc_source->get_sampler_view_planes(dec->mc_source);
   for (i = 0, component = 0; i < VL_MAX_PLANES; ++i) {
      if (!surfaces[i]) continue;

      nr_components = util_format_get_nr_components(surfaces[i]->texture->format);
      for (j = 0; j < nr_components; ++j, ++component) {
         if (!num_ycbcr_blocks[i]) continue;

         vb[1] = vl_vb_get_ycbcr(&buf->vertex_stream, component);
         dec->pipe->set_vertex_buffers(dec->pipe, 2, vb);

         if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
            vl_idct_prepare_stage2(component == 0 ? &dec->idct_y : &dec->idct_c, &buf->idct[component]);
         else {
            dec->pipe->set_fragment_sampler_views(dec->pipe, 1, &mc_source_sv[component]);
            dec->pipe->bind_fragment_sampler_states(dec->pipe, 1, &dec->sampler_ycbcr);
         }
         vl_mc_render_ycbcr(&buf->mc[i], j, num_ycbcr_blocks[component]);
      }
   }
}

static bool
init_pipe_state(struct vl_mpeg12_decoder *dec)
{
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_sampler_state sampler;
   unsigned i;

   assert(dec);

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

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   sampler.normalized_coords = 1;
   dec->sampler_ycbcr = dec->pipe->create_sampler_state(dec->pipe, &sampler);
   if (!dec->sampler_ycbcr)
      return false;

   return true;
}

static const struct format_config*
find_format_config(struct vl_mpeg12_decoder *dec, const struct format_config configs[], unsigned num_configs)
{
   struct pipe_screen *screen;
   unsigned i;

   assert(dec);

   screen = dec->pipe->screen;

   for (i = 0; i < num_configs; ++i) {
      if (!screen->is_format_supported(screen, configs[i].zscan_source_format, PIPE_TEXTURE_2D,
                                       1, PIPE_BIND_SAMPLER_VIEW))
         continue;

      if (configs[i].idct_source_format != PIPE_FORMAT_NONE) {
         if (!screen->is_format_supported(screen, configs[i].idct_source_format, PIPE_TEXTURE_2D,
                                          1, PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET))
            continue;

         if (!screen->is_format_supported(screen, configs[i].mc_source_format, PIPE_TEXTURE_3D,
                                          1, PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET))
            continue;
      } else {
         if (!screen->is_format_supported(screen, configs[i].mc_source_format, PIPE_TEXTURE_2D,
                                          1, PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET))
            continue;
      }
      return &configs[i];
   }

   return NULL;
}

static bool
init_zscan(struct vl_mpeg12_decoder *dec, const struct format_config* format_config)
{
   unsigned num_channels;

   assert(dec);

   dec->blocks_per_line = 4;
   dec->max_blocks =
      (dec->base.width * dec->base.height) /
      (BLOCK_WIDTH * BLOCK_HEIGHT);

   dec->zscan_source_format = format_config->zscan_source_format;
   dec->zscan_linear = vl_zscan_layout(dec->pipe, vl_zscan_linear, dec->blocks_per_line);
   dec->zscan_normal = vl_zscan_layout(dec->pipe, vl_zscan_normal, dec->blocks_per_line);
   dec->zscan_alternate = vl_zscan_layout(dec->pipe, vl_zscan_alternate, dec->blocks_per_line);

   num_channels = dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT ? 4 : 1;

   if (!vl_zscan_init(&dec->zscan_y, dec->pipe, dec->base.width, dec->base.height,
                      dec->blocks_per_line, dec->max_blocks, num_channels))
      return false;

   if (!vl_zscan_init(&dec->zscan_c, dec->pipe, dec->chroma_width, dec->chroma_height,
                      dec->blocks_per_line, dec->max_blocks, num_channels))
      return false;

   return true;
}

static bool
init_idct(struct vl_mpeg12_decoder *dec, const struct format_config* format_config)
{
   unsigned nr_of_idct_render_targets;
   enum pipe_format formats[3];

   struct pipe_sampler_view *matrix = NULL;

   nr_of_idct_render_targets = dec->pipe->screen->get_param(dec->pipe->screen, PIPE_CAP_MAX_RENDER_TARGETS);

   // more than 4 render targets usually doesn't makes any seens
   nr_of_idct_render_targets = MIN2(nr_of_idct_render_targets, 4);

   formats[0] = formats[1] = formats[2] = format_config->idct_source_format;
   dec->idct_source = vl_video_buffer_init(dec->base.context, dec->pipe,
                                           dec->base.width / 4, dec->base.height, 1,
                                           dec->base.chroma_format,
                                           formats, PIPE_USAGE_STATIC);
   if (!dec->idct_source)
      goto error_idct_source;

   formats[0] = formats[1] = formats[2] = format_config->mc_source_format;
   dec->mc_source = vl_video_buffer_init(dec->base.context, dec->pipe,
                                         dec->base.width / nr_of_idct_render_targets,
                                         dec->base.height / 4, nr_of_idct_render_targets,
                                         dec->base.chroma_format,
                                         formats, PIPE_USAGE_STATIC);

   if (!dec->mc_source)
      goto error_mc_source;

   if (!(matrix = vl_idct_upload_matrix(dec->pipe, format_config->idct_scale)))
      goto error_matrix;

   if (!vl_idct_init(&dec->idct_y, dec->pipe, dec->base.width, dec->base.height,
                     nr_of_idct_render_targets, matrix, matrix))
      goto error_y;

   if(!vl_idct_init(&dec->idct_c, dec->pipe, dec->chroma_width, dec->chroma_height,
                    nr_of_idct_render_targets, matrix, matrix))
      goto error_c;

   pipe_sampler_view_reference(&matrix, NULL);

   return true;

error_c:
   vl_idct_cleanup(&dec->idct_y);

error_y:
   pipe_sampler_view_reference(&matrix, NULL);

error_matrix:
   dec->mc_source->destroy(dec->mc_source);

error_mc_source:
   dec->idct_source->destroy(dec->idct_source);

error_idct_source:
   return false;
}

static bool
init_mc_source_widthout_idct(struct vl_mpeg12_decoder *dec, const struct format_config* format_config)
{
   enum pipe_format formats[3];

   formats[0] = formats[1] = formats[2] = format_config->mc_source_format;
   dec->mc_source = vl_video_buffer_init(dec->base.context, dec->pipe,
                                         dec->base.width, dec->base.height, 1,
                                         dec->base.chroma_format,
                                         formats, PIPE_USAGE_STATIC);

   return dec->mc_source != NULL;
}

static void
mc_vert_shader_callback(void *priv, struct vl_mc *mc,
                        struct ureg_program *shader,
                        unsigned first_output,
                        struct ureg_dst tex)
{
   struct vl_mpeg12_decoder *dec = priv;
   struct ureg_dst o_vtex;

   assert(priv && mc);
   assert(shader);

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      struct vl_idct *idct = mc == &dec->mc_y ? &dec->idct_y : &dec->idct_c;
      vl_idct_stage2_vert_shader(idct, shader, first_output, tex);
   } else {
      o_vtex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output);
      ureg_MOV(shader, ureg_writemask(o_vtex, TGSI_WRITEMASK_XY), ureg_src(tex));
   }
}

static void
mc_frag_shader_callback(void *priv, struct vl_mc *mc,
                        struct ureg_program *shader,
                        unsigned first_input,
                        struct ureg_dst dst)
{
   struct vl_mpeg12_decoder *dec = priv;
   struct ureg_src src, sampler;

   assert(priv && mc);
   assert(shader);

   if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      struct vl_idct *idct = mc == &dec->mc_y ? &dec->idct_y : &dec->idct_c;
      vl_idct_stage2_frag_shader(idct, shader, first_input, dst);
   } else {
      src = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input, TGSI_INTERPOLATE_LINEAR);
      sampler = ureg_DECL_sampler(shader, 0);
      ureg_TEX(shader, dst, TGSI_TEXTURE_2D, src, sampler);
   }
}

struct pipe_video_decoder *
vl_create_mpeg12_decoder(struct pipe_video_context *context,
                         struct pipe_context *pipe,
                         enum pipe_video_profile profile,
                         enum pipe_video_entrypoint entrypoint,
                         enum pipe_video_chroma_format chroma_format,
                         unsigned width, unsigned height)
{
   const struct format_config *format_config;
   struct vl_mpeg12_decoder *dec;

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

   dec->pipe = pipe;

   dec->quads = vl_vb_upload_quads(dec->pipe);
   dec->pos = vl_vb_upload_pos(
      dec->pipe,
      dec->base.width / MACROBLOCK_WIDTH,
      dec->base.height / MACROBLOCK_HEIGHT
   );

   dec->ves_ycbcr = vl_vb_get_ves_ycbcr(dec->pipe);
   dec->ves_mv = vl_vb_get_ves_mv(dec->pipe);

   /* TODO: Implement 422, 444 */
   assert(dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);

   if (dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      dec->chroma_width = dec->base.width / 2;
      dec->chroma_height = dec->base.height / 2;
   } else if (dec->base.chroma_format == PIPE_VIDEO_CHROMA_FORMAT_422) {
      dec->chroma_width = dec->base.width;
      dec->chroma_height = dec->base.height / 2;
   } else {
      dec->chroma_width = dec->base.width;
      dec->chroma_height = dec->base.height;
   }

   switch (entrypoint) {
   case PIPE_VIDEO_ENTRYPOINT_BITSTREAM:
      format_config = find_format_config(dec, bitstream_format_config, num_bitstream_format_configs);
      break;

   case PIPE_VIDEO_ENTRYPOINT_IDCT:
      format_config = find_format_config(dec, idct_format_config, num_idct_format_configs);
      break;

   case PIPE_VIDEO_ENTRYPOINT_MC:
      format_config = find_format_config(dec, mc_format_config, num_mc_format_configs);
      break;

   default:
      assert(0);
      return NULL;
   }

   if (!format_config)
      return NULL;

   if (!init_zscan(dec, format_config))
      goto error_zscan;

   if (entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      if (!init_idct(dec, format_config))
         goto error_sources;
   } else {
      if (!init_mc_source_widthout_idct(dec, format_config))
         goto error_sources;
   }

   if (!vl_mc_init(&dec->mc_y, dec->pipe, dec->base.width, dec->base.height, MACROBLOCK_HEIGHT, format_config->mc_scale,
                   mc_vert_shader_callback, mc_frag_shader_callback, dec))
      goto error_mc_y;

   // TODO
   if (!vl_mc_init(&dec->mc_c, dec->pipe, dec->base.width, dec->base.height, BLOCK_HEIGHT, format_config->mc_scale,
                   mc_vert_shader_callback, mc_frag_shader_callback, dec))
      goto error_mc_c;

   if (!init_pipe_state(dec))
      goto error_pipe_state;

   return &dec->base;

error_pipe_state:
   vl_mc_cleanup(&dec->mc_c);

error_mc_c:
   vl_mc_cleanup(&dec->mc_y);

error_mc_y:
   if (entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT) {
      vl_idct_cleanup(&dec->idct_y);
      vl_idct_cleanup(&dec->idct_c);
      dec->idct_source->destroy(dec->idct_source);
   }
   dec->mc_source->destroy(dec->mc_source);

error_sources:
   vl_zscan_cleanup(&dec->zscan_y);
   vl_zscan_cleanup(&dec->zscan_c);

error_zscan:
   FREE(dec);
   return NULL;
}
