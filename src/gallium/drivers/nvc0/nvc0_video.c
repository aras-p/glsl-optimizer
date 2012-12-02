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

static void
nvc0_decoder_decode_bitstream(struct pipe_video_decoder *decoder,
                              struct pipe_video_buffer *video_target,
                              struct pipe_picture_desc *picture,
                              unsigned num_buffers,
                              const void *const *data,
                              const unsigned *num_bytes)
{
   struct nvc0_decoder *dec = (struct nvc0_decoder *)decoder;
   struct nvc0_video_buffer *target = (struct nvc0_video_buffer *)video_target;
   uint32_t comm_seq = ++dec->fence_seq;
   union pipe_desc desc;

   unsigned vp_caps, is_ref, ret;
   struct nvc0_video_buffer *refs[16] = {};

   desc.base = picture;

   assert(target->base.buffer_format == PIPE_FORMAT_NV12);

   ret = nvc0_decoder_bsp(dec, desc, target, comm_seq,
                          num_buffers, data, num_bytes,
                          &vp_caps, &is_ref, refs);

   /* did we decode bitstream correctly? */
   assert(ret == 2);

   nvc0_decoder_vp(dec, desc, target, comm_seq, vp_caps, is_ref, refs);
   nvc0_decoder_ppp(dec, desc, target, comm_seq);
}

static void
nvc0_decoder_flush(struct pipe_video_decoder *decoder)
{
   struct nvc0_decoder *dec = (struct nvc0_decoder *)decoder;
   (void)dec;
}

static void
nvc0_decoder_begin_frame(struct pipe_video_decoder *decoder,
                         struct pipe_video_buffer *target,
                         struct pipe_picture_desc *picture)
{
}

static void
nvc0_decoder_end_frame(struct pipe_video_decoder *decoder,
                       struct pipe_video_buffer *target,
                       struct pipe_picture_desc *picture)
{
}

static void
nvc0_decoder_destroy(struct pipe_video_decoder *decoder)
{
   struct nvc0_decoder *dec = (struct nvc0_decoder *)decoder;
   int i;

   nouveau_bo_ref(NULL, &dec->ref_bo);
   nouveau_bo_ref(NULL, &dec->bitplane_bo);
   nouveau_bo_ref(NULL, &dec->inter_bo[0]);
   nouveau_bo_ref(NULL, &dec->inter_bo[1]);
#ifdef NVC0_DEBUG_FENCE
   nouveau_bo_ref(NULL, &dec->fence_bo);
#endif
   nouveau_bo_ref(NULL, &dec->fw_bo);

   for (i = 0; i < NVC0_VIDEO_QDEPTH; ++i)
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

static void nvc0_video_getpath(enum pipe_video_profile profile, char *path)
{
   switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_CODEC_MPEG12: {
         sprintf(path, "/lib/firmware/nouveau/vuc-mpeg12-0");
         break;
      }
      case PIPE_VIDEO_CODEC_MPEG4: {
         sprintf(path, "/lib/firmware/nouveau/vuc-mpeg4-0");
         break;
      }
      case PIPE_VIDEO_CODEC_VC1: {
         sprintf(path, "/lib/firmware/nouveau/vuc-vc1-%u", profile - PIPE_VIDEO_PROFILE_VC1_SIMPLE);
         break;
      }
      case PIPE_VIDEO_CODEC_MPEG4_AVC: {
         sprintf(path, "/lib/firmware/nouveau/vuc-h264-0");
         break;
      }
      default: assert(0);
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
   struct nouveau_screen *screen = &((struct nvc0_context *)context)->screen->base;
   struct nvc0_decoder *dec;
   struct nouveau_pushbuf **push;
   union nouveau_bo_config cfg;
   bool kepler = screen->device->chipset >= 0xe0;

   cfg.nvc0.tile_mode = 0x10;
   cfg.nvc0.memtype = 0xfe;

   int ret, i;
   uint32_t codec = 1, ppp_codec = 3;
   uint32_t timeout;
   u32 tmp_size = 0;

   if (getenv("XVMC_VL"))
       return vl_create_decoder(context, profile, entrypoint,
                                chroma_format, width, height,
                                max_references, chunked_decode);

   if (entrypoint != PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      debug_printf("%x\n", entrypoint);
      return NULL;
   }

   dec = CALLOC_STRUCT(nvc0_decoder);
   if (!dec)
      return NULL;
   dec->client = screen->client;

   if (!kepler) {
      dec->bsp_idx = 5;
      dec->vp_idx = 6;
      dec->ppp_idx = 7;
   } else {
      dec->bsp_idx = 2;
      dec->vp_idx = 2;
      dec->ppp_idx = 2;
   }

   for (i = 0; i < 3; ++i)
      if (i && !kepler) {
         dec->channel[i] = dec->channel[0];
         dec->pushbuf[i] = dec->pushbuf[0];
      } else {
         void *data;
         u32 size;
         struct nvc0_fifo nvc0_args = {};
         struct nve0_fifo nve0_args = {};

         if (!kepler) {
            size = sizeof(nvc0_args);
            data = &nvc0_args;
         } else {
            unsigned engine[] = {
               NVE0_FIFO_ENGINE_BSP,
               NVE0_FIFO_ENGINE_VP,
               NVE0_FIFO_ENGINE_PPP
            };

            nve0_args.engine = engine[i];
            size = sizeof(nve0_args);
            data = &nve0_args;
         }

         ret = nouveau_object_new(&screen->device->object, 0,
                                  NOUVEAU_FIFO_CHANNEL_CLASS,
                                  data, size, &dec->channel[i]);

         if (!ret)
            ret = nouveau_pushbuf_new(screen->client, dec->channel[i], 4,
                                   32 * 1024, true, &dec->pushbuf[i]);
         if (ret)
            break;
      }
   push = dec->pushbuf;

   if (!kepler) {
      if (!ret)
         ret = nouveau_object_new(dec->channel[0], 0x390b1, 0x90b1, NULL, 0, &dec->bsp);
      if (!ret)
         ret = nouveau_object_new(dec->channel[1], 0x190b2, 0x90b2, NULL, 0, &dec->vp);
      if (!ret)
         ret = nouveau_object_new(dec->channel[2], 0x290b3, 0x90b3, NULL, 0, &dec->ppp);
   } else {
      if (!ret)
         ret = nouveau_object_new(dec->channel[0], 0x95b1, 0x95b1, NULL, 0, &dec->bsp);
      if (!ret)
         ret = nouveau_object_new(dec->channel[1], 0x95b2, 0x95b2, NULL, 0, &dec->vp);
      if (!ret)
         ret = nouveau_object_new(dec->channel[2], 0x90b3, 0x90b3, NULL, 0, &dec->ppp);
   }
   if (ret)
      goto fail;

   BEGIN_NVC0(push[0], SUBC_BSP(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push[0], dec->bsp->handle);

   BEGIN_NVC0(push[1], SUBC_VP(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push[1], dec->vp->handle);

   BEGIN_NVC0(push[2], SUBC_PPP(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push[2], dec->ppp->handle);

   dec->base.context = context;
   dec->base.profile = profile;
   dec->base.entrypoint = entrypoint;
   dec->base.chroma_format = chroma_format;
   dec->base.width = width;
   dec->base.height = height;
   dec->base.max_references = max_references;
   dec->base.destroy = nvc0_decoder_destroy;
   dec->base.flush = nvc0_decoder_flush;
   dec->base.decode_bitstream = nvc0_decoder_decode_bitstream;
   dec->base.begin_frame = nvc0_decoder_begin_frame;
   dec->base.end_frame = nvc0_decoder_end_frame;

   for (i = 0; i < NVC0_VIDEO_QDEPTH && !ret; ++i)
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM,
                           0, 1 << 20, &cfg, &dec->bsp_bo[i]);
   if (!ret)
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM,
                           0x100, 4 << 20, &cfg, &dec->inter_bo[0]);
   if (!ret) {
      if (!kepler)
         nouveau_bo_ref(dec->inter_bo[0], &dec->inter_bo[1]);
      else
         ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM,
                              0x100, dec->inter_bo[0]->size, &cfg,
                              &dec->inter_bo[1]);
   }
   if (ret)
      goto fail;

   switch (u_reduce_video_profile(profile)) {
   case PIPE_VIDEO_CODEC_MPEG12: {
      codec = 1;
      assert(max_references <= 2);
      break;
   }
   case PIPE_VIDEO_CODEC_MPEG4: {
      codec = 4;
      tmp_size = mb(height)*16 * mb(width)*16;
      assert(max_references <= 2);
      break;
   }
   case PIPE_VIDEO_CODEC_VC1: {
      ppp_codec = codec = 2;
      tmp_size = mb(height)*16 * mb(width)*16;
      assert(max_references <= 2);
      break;
   }
   case PIPE_VIDEO_CODEC_MPEG4_AVC: {
      codec = 3;
      dec->tmp_stride = 16 * mb_half(width) * nvc0_video_align(height) * 3 / 2;
      tmp_size = dec->tmp_stride * (max_references + 1);
      assert(max_references <= 16);
      break;
   }
   default:
      fprintf(stderr, "invalid codec\n");
      goto fail;
   }

   if (screen->device->chipset < 0xd0) {
      int fd;
      char path[PATH_MAX];
      ssize_t r;
      uint32_t *end, endval;

      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM, 0,
                           0x4000, &cfg, &dec->fw_bo);
      if (!ret)
         ret = nouveau_bo_map(dec->fw_bo, NOUVEAU_BO_WR, dec->client);
      if (ret)
         goto fail;

      nvc0_video_getpath(profile, path);

      fd = open(path, O_RDONLY | O_CLOEXEC);
      if (fd < 0) {
         fprintf(stderr, "opening firmware file %s failed: %m\n", path);
         goto fw_fail;
      }
      r = read(fd, dec->fw_bo->map, 0x4000);
      if (r < 0) {
         fprintf(stderr, "reading firmware file %s failed: %m\n", path);
         goto fw_fail;
      }

      if (r == 0x4000) {
         close(fd);
         fprintf(stderr, "firmware file %s too large!\n", path);
         goto fw_fail;
      }

      if (r & 0xff) {
         close(fd);
         fprintf(stderr, "firmware file %s wrong size!\n", path);
         goto fw_fail;
      }

      end = dec->fw_bo->map + r - 4;
      endval = *end;
      while (endval == *end)
         end--;

      r = (intptr_t)end - (intptr_t)dec->fw_bo->map + 4;

      switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_CODEC_MPEG12: {
         assert((r & 0xff) == 0xe0);
         dec->fw_sizes = (0x2e0<<16) | (r - 0x2e0);
         break;
      }
      case PIPE_VIDEO_CODEC_MPEG4: {
         assert((r & 0xff) == 0xe0);
         dec->fw_sizes = (0x2e0<<16) | (r - 0x2e0);
         break;
      }
      case PIPE_VIDEO_CODEC_VC1: {
         assert((r & 0xff) == 0xac);
         dec->fw_sizes = (0x3ac<<16) | (r - 0x3ac);
         break;
      }
      case PIPE_VIDEO_CODEC_MPEG4_AVC: {
         assert((r & 0xff) == 0x70);
         dec->fw_sizes = (0x370<<16) | (r - 0x370);
         break;
      }
      default:
         goto fw_fail;
      }
      munmap(dec->fw_bo->map, dec->fw_bo->size);
      dec->fw_bo->map = NULL;
   }

   if (codec != 3) {
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM, 0,
                           0x400, &cfg, &dec->bitplane_bo);
      if (ret)
         goto fail;
   }

   dec->ref_stride = mb(width)*16 * (mb_half(height)*32 + nvc0_video_align(height)/2);
   ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM, 0,
                        dec->ref_stride * (max_references+2) + tmp_size,
                        &cfg, &dec->ref_bo);
   if (ret)
      goto fail;

   timeout = 0;

   BEGIN_NVC0(push[0], SUBC_BSP(0x200), 2);
   PUSH_DATA (push[0], codec);
   PUSH_DATA (push[0], timeout);

   BEGIN_NVC0(push[1], SUBC_VP(0x200), 2);
   PUSH_DATA (push[1], codec);
   PUSH_DATA (push[1], timeout);

   BEGIN_NVC0(push[2], SUBC_PPP(0x200), 2);
   PUSH_DATA (push[2], ppp_codec);
   PUSH_DATA (push[2], timeout);

   ++dec->fence_seq;

#if NVC0_DEBUG_FENCE
   ret = nouveau_bo_new(screen->device, NOUVEAU_BO_GART|NOUVEAU_BO_MAP,
                        0, 0x1000, &cfg, &dec->fence_bo);
   if (ret)
      goto fail;

   nouveau_bo_map(dec->fence_bo, NOUVEAU_BO_RDWR, screen->client);
   dec->fence_map = dec->fence_bo->map;
   dec->fence_map[0] = dec->fence_map[4] = dec->fence_map[8] = 0;
   dec->comm = (struct comm *)(dec->fence_map + (COMM_OFFSET/sizeof(*dec->fence_map)));

   /* So lets test if the fence is working? */
   BEGIN_NVC0(push[0], SUBC_BSP(0x240), 3);
   PUSH_DATAh(push[0], dec->fence_bo->offset);
   PUSH_DATA (push[0], dec->fence_bo->offset);
   PUSH_DATA (push[0], dec->fence_seq);

   BEGIN_NVC0(push[0], SUBC_BSP(0x304), 1);
   PUSH_DATA (push[0], 1);
   PUSH_KICK (push[0]);

   BEGIN_NVC0(push[1], SUBC_VP(0x240), 3);
   PUSH_DATAh(push[1], (dec->fence_bo->offset + 0x10));
   PUSH_DATA (push[1], (dec->fence_bo->offset + 0x10));
   PUSH_DATA (push[1], dec->fence_seq);

   BEGIN_NVC0(push[1], SUBC_VP(0x304), 1);
   PUSH_DATA (push[1], 1);
   PUSH_KICK (push[1]);

   BEGIN_NVC0(push[2], SUBC_PPP(0x240), 3);
   PUSH_DATAh(push[2], (dec->fence_bo->offset + 0x20));
   PUSH_DATA (push[2], (dec->fence_bo->offset + 0x20));
   PUSH_DATA (push[2], dec->fence_seq);

   BEGIN_NVC0(push[2], SUBC_PPP(0x304), 1);
   PUSH_DATA (push[2], 1);
   PUSH_KICK (push[2]);

   usleep(100);
   while (dec->fence_seq > dec->fence_map[0] &&
          dec->fence_seq > dec->fence_map[4] &&
          dec->fence_seq > dec->fence_map[8]) {
      debug_printf("%u: %u %u %u\n", dec->fence_seq, dec->fence_map[0], dec->fence_map[4], dec->fence_map[8]);
      usleep(100);
   }
   debug_printf("%u: %u %u %u\n", dec->fence_seq, dec->fence_map[0], dec->fence_map[4], dec->fence_map[8]);
#endif

   return &dec->base;

fw_fail:
   debug_printf("Cannot create decoder without firmware..\n");
   nvc0_decoder_destroy(&dec->base);
   return NULL;

fail:
   debug_printf("Creation failed: %s (%i)\n", strerror(-ret), ret);
   nvc0_decoder_destroy(&dec->base);
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
