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

#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "nouveau_screen.h"
#include "nouveau_context.h"
#include "nouveau_video.h"

#include "nvfx/nvfx_context.h"
#include "nvfx/nvfx_resource.h"
#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_buffer.h"
#include "util/u_video.h"
#include "util/u_format.h"
#include "util/u_sampler.h"
#include "nouveau/nouveau_device.h"
#include "nouveau_winsys.h"

static bool
nouveau_video_is_nvfx(struct nouveau_decoder *dec) {
   if (dec->screen->device->chipset < 0x50)
      return true;
   if (dec->screen->device->chipset >= 0x60 && dec->screen->device->chipset < 0x70)
      return true;
   return false;
}

static int
nouveau_vpe_init(struct nouveau_decoder *dec) {
   int ret;
   if (dec->cmds)
      return 0;
   ret = nouveau_bo_map(dec->cmd_bo, NOUVEAU_BO_RDWR);
   if (ret) {
      debug_printf("Mapping cmd bo: %s\n", strerror(-ret));
      return ret;
   }
   ret = nouveau_bo_map(dec->data_bo, NOUVEAU_BO_RDWR);
   if (ret) {
      nouveau_bo_unmap(dec->cmd_bo);
      debug_printf("Mapping data bo: %s\n", strerror(-ret));
      return ret;
   }
   dec->cmds = dec->cmd_bo->map;
   dec->data = dec->data_bo->map;
   return ret;
}

static void
nouveau_vpe_synch(struct nouveau_decoder *dec) {
   struct nouveau_channel *chan = dec->screen->channel;
#if 0
   if (dec->fence_map) {
      BEGIN_RING(chan, dec->mpeg, NV84_MPEG_QUERY_COUNTER, 1);
      OUT_RING(chan, ++dec->fence_seq);
      FIRE_RING(chan);
      while (dec->fence_map[0] != dec->fence_seq)
         usleep(1000);
   } else
#endif
      FIRE_RING(chan);
}

static void
nouveau_vpe_fini(struct nouveau_decoder *dec) {
   struct nouveau_channel *chan = dec->screen->channel;
   if (!dec->cmds)
      return;

   nouveau_bo_unmap(dec->data_bo);
   nouveau_bo_unmap(dec->cmd_bo);

   MARK_RING(chan, 8, 2);
   BEGIN_RING(chan, dec->mpeg, NV31_MPEG_CMD_OFFSET, 2);
   OUT_RELOCl(chan, dec->cmd_bo, 0, NOUVEAU_BO_RD|NOUVEAU_BO_GART);
   OUT_RING(chan, dec->ofs * 4);

   BEGIN_RING(chan, dec->mpeg, NV31_MPEG_DATA_OFFSET, 2);
   OUT_RELOCl(chan, dec->data_bo, 0, NOUVEAU_BO_RD|NOUVEAU_BO_GART);
   OUT_RING(chan, dec->data_pos * 4);

   BEGIN_RING(chan, dec->mpeg, NV31_MPEG_EXEC, 1);
   OUT_RING(chan, 1);

   nouveau_vpe_synch(dec);
   dec->ofs = dec->data_pos = dec->num_surfaces = 0;
   dec->cmds = dec->data = NULL;
   dec->current = dec->future = dec->past = 8;
}

static INLINE void
nouveau_vpe_mb_dct_blocks(struct nouveau_decoder *dec, const struct pipe_mpeg12_macroblock *mb)
{
   int cbb;
   unsigned cbp = mb->coded_block_pattern;
   short *db = mb->blocks;
   for (cbb = 0x20; cbb > 0; cbb >>= 1) {
      if (cbb & cbp) {
         static const int lookup[64] = {
             0, 1, 8,16, 9, 2, 3,10,
            17,24,32,25,18,11, 4, 5,
            12,19,26,33,40,48,41,34,
            27,20,13, 6, 7,14,21,28,
            35,42,49,56,57,50,43,36,
            29,22,15,23,30,37,44,51,
            58,59,52,45,38,31,39,46,
            53,60,61,54,47,55,62,63
         };
         int i, j = 0, found = 0;
         for (i = 0; i < 64; ++i) {
            if (!db[lookup[i]]) { j += 2; continue; }
            dec->data[dec->data_pos++] = (db[lookup[i]] << 16) | j;
            j = 0;
            found = 1;
         }
         if (found)
            dec->data[dec->data_pos - 1] |= 1;
         else
            dec->data[dec->data_pos++] = 1;
         db += 64;
      } else if (mb->macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA) {
         dec->data[dec->data_pos++] = 1;
      }
   }
}

static INLINE void
nouveau_vpe_mb_data_blocks(struct nouveau_decoder *dec, const struct pipe_mpeg12_macroblock *mb)
{
   int cbb;
   unsigned cbp = mb->coded_block_pattern;
   short *db = mb->blocks;
   for (cbb = 0x20; cbb > 0; cbb >>= 1) {
      if (cbb & cbp) {
         memcpy(&dec->data[dec->data_pos], db, 128);
         dec->data_pos += 32;
         db += 64;
      } else if (mb->macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA) {
         memset(&dec->data[dec->data_pos], 0, 128);
         dec->data_pos += 32;
      }
   }
}

static INLINE void
nouveau_vpe_mb_dct_header(struct nouveau_decoder *dec,
                          const struct pipe_mpeg12_macroblock *mb,
                          bool luma)
{
   unsigned base_dct, cbp;
   bool intra = mb->macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA;
   unsigned x = mb->x * 16;
   unsigned y = luma ? mb->y * 16 : mb->y * 8;

   /* Setup the base dct header */
   base_dct = dec->current << NV17_MPEG_CMD_CHROMA_MB_HEADER_SURFACE__SHIFT;
   base_dct |= NV17_MPEG_CMD_CHROMA_MB_HEADER_RUN_SINGLE;

   if (!(mb->x & 1))
      base_dct |= NV17_MPEG_CMD_CHROMA_MB_HEADER_X_COORD_EVEN;
   if (intra)
      cbp = 0x3f;
   else
      cbp = mb->coded_block_pattern;

   if (dec->picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FRAME) {
      base_dct |= NV17_MPEG_CMD_CHROMA_MB_HEADER_TYPE_FRAME;
      if (luma && mb->macroblock_modes.bits.dct_type == PIPE_MPEG12_DCT_TYPE_FIELD)
         base_dct |= NV17_MPEG_CMD_CHROMA_MB_HEADER_FRAME_DCT_TYPE_FIELD;
   } else {
      if (dec->picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FIELD_BOTTOM)
         base_dct |= NV17_MPEG_CMD_CHROMA_MB_HEADER_FIELD_BOTTOM;
      if (!intra)
         y *= 2;
   }

   if (luma) {
      base_dct |= NV17_MPEG_CMD_LUMA_MB_HEADER_OP_LUMA_MB_HEADER;
      base_dct |= (cbp >> 2) << NV17_MPEG_CMD_LUMA_MB_HEADER_CBP__SHIFT;
   } else {
      base_dct |= NV17_MPEG_CMD_CHROMA_MB_HEADER_OP_CHROMA_MB_HEADER;
      base_dct |= (cbp & 3) << NV17_MPEG_CMD_CHROMA_MB_HEADER_CBP__SHIFT;
   }
   nouveau_vpe_write(dec, base_dct);
   nouveau_vpe_write(dec, NV17_MPEG_CMD_MB_COORDS_OP_MB_COORDS |
                     x | (y << NV17_MPEG_CMD_MB_COORDS_Y__SHIFT));
}

static INLINE unsigned int
nouveau_vpe_mb_mv_flags(bool luma, int mv_h, int mv_v, bool forward, bool first, bool vert)
{
   unsigned mc_header = 0;
   if (luma)
      mc_header |= NV17_MPEG_CMD_LUMA_MV_HEADER_OP_LUMA_MV_HEADER;
   else
      mc_header |= NV17_MPEG_CMD_CHROMA_MV_HEADER_OP_CHROMA_MV_HEADER;
   if (mv_h & 1)
      mc_header |= NV17_MPEG_CMD_CHROMA_MV_HEADER_X_HALF;
   if (mv_v & 1)
      mc_header |= NV17_MPEG_CMD_CHROMA_MV_HEADER_Y_HALF;
   if (!forward)
      mc_header |= NV17_MPEG_CMD_CHROMA_MV_HEADER_DIRECTION_BACKWARD;
   if (!first)
      mc_header |= NV17_MPEG_CMD_CHROMA_MV_HEADER_IDX;
   if (vert)
      mc_header |= NV17_MPEG_CMD_LUMA_MV_HEADER_FIELD_BOTTOM;
   return mc_header;
}

static unsigned pos(int pos, int mov, int max) {
   int ret = pos + mov;
   if (pos < 0)
      return 0;
   if (pos >= max)
      return max-1;
   return ret;
}

/* because we want -1 / 2 = -1 */
static int div_down(int val, int mult) {
   val &= ~(mult - 1);
   return val / mult;
}

static int div_up(int val, int mult) {
   val += mult - 1;
   return val / mult;
}

static INLINE void
nouveau_vpe_mb_mv(struct nouveau_decoder *dec, unsigned mc_header,
                   bool luma, bool frame, bool forward, bool vert,
                   int x, int y, const short motions[2],
                   unsigned surface, bool first)
{
   unsigned mc_vector;
   int mv_horizontal = motions[0];
   int mv_vertical = motions[1];
   int mv2 = mc_header & NV17_MPEG_CMD_CHROMA_MV_HEADER_COUNT_2;
   unsigned width = dec->base.width;
   unsigned height = dec->base.height;
   if (mv2)
      mv_vertical = div_down(mv_vertical, 2);
   assert(frame); // Untested for non-frames
   if (!frame)
      height *= 2;

   mc_header |= surface << NV17_MPEG_CMD_CHROMA_MV_HEADER_SURFACE__SHIFT;
   if (!luma) {
      mv_vertical = div_up(mv_vertical, 2);
      mv_horizontal = div_up(mv_horizontal, 2);
      height /= 2;
   }
   mc_header |= nouveau_vpe_mb_mv_flags(luma, mv_horizontal, mv_vertical, forward, first, vert);
   nouveau_vpe_write(dec, mc_header);

   mc_vector = NV17_MPEG_CMD_MV_COORDS_OP_MV_COORDS;
   if (luma)
      mc_vector |= pos(x, div_down(mv_horizontal, 2), width);
   else
      mc_vector |= pos(x, mv_horizontal & ~1, width);
   if (!mv2)
      mc_vector |= pos(y, div_down(mv_vertical, 2), height) << NV17_MPEG_CMD_MV_COORDS_Y__SHIFT;
   else
      mc_vector |= pos(y, mv_vertical & ~1, height) << NV17_MPEG_CMD_MV_COORDS_Y__SHIFT;
   nouveau_vpe_write(dec, mc_vector);
}

static void
nouveau_vpe_mb_mv_header(struct nouveau_decoder *dec,
                         const struct pipe_mpeg12_macroblock *mb,
                         bool luma)
{
   bool frame = dec->picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FRAME;
   unsigned base;
   bool forward, backward;
   int y, y2, x = mb->x * 16;
   if (luma)
      y = mb->y * (frame ? 16 : 32);
   else
      y = mb->y * (frame ? 8 : 16);
   if (frame)
      y2 = y;
   else
      y2 = y + (luma ? 16 : 8);

   forward = mb->macroblock_type & PIPE_MPEG12_MB_TYPE_MOTION_FORWARD;
   backward = mb->macroblock_type & PIPE_MPEG12_MB_TYPE_MOTION_BACKWARD;
   assert(!forward || dec->past < 8);
   assert(!backward || dec->future < 8);
   if (frame) {
      switch (mb->macroblock_modes.bits.frame_motion_type) {
      case PIPE_MPEG12_MO_TYPE_FRAME: goto mv1;
      case PIPE_MPEG12_MO_TYPE_FIELD: goto mv2;
      case PIPE_MPEG12_MO_TYPE_DUAL_PRIME: {
         base = NV17_MPEG_CMD_CHROMA_MV_HEADER_COUNT_2;
         if (forward) {
            nouveau_vpe_mb_mv(dec, base, luma, frame, TRUE, FALSE,
                              x, y, mb->PMV[0][0], dec->past, TRUE);
            nouveau_vpe_mb_mv(dec, base, luma, frame, TRUE, TRUE,
                              x, y2, mb->PMV[0][0], dec->past, FALSE);
         }
         if (backward && forward) {
            nouveau_vpe_mb_mv(dec, base, luma, frame, !forward, TRUE,
                              x, y, mb->PMV[1][0], dec->future, TRUE);
            nouveau_vpe_mb_mv(dec, base, luma, frame, !forward, FALSE,
                              x, y2, mb->PMV[1][1], dec->future, FALSE);
         } else assert(!backward);
         break;
      }
      default: assert(0);
      }
   } else {
      switch (mb->macroblock_modes.bits.field_motion_type) {
      case PIPE_MPEG12_MO_TYPE_FIELD: goto mv1;
      case PIPE_MPEG12_MO_TYPE_16x8: goto mv2;
      case PIPE_MPEG12_MO_TYPE_DUAL_PRIME: {
      base = NV17_MPEG_CMD_CHROMA_MV_HEADER_MV_SPLIT_HALF_MB;
         if (frame)
            base |= NV17_MPEG_CMD_CHROMA_MV_HEADER_TYPE_FRAME;
         if (forward)
            nouveau_vpe_mb_mv(dec, base, luma, frame, TRUE,
                              dec->picture_structure != PIPE_MPEG12_PICTURE_STRUCTURE_FIELD_TOP,
                              x, y, mb->PMV[0][0], dec->past, TRUE);
         if (backward && forward)
            nouveau_vpe_mb_mv(dec, base, luma, frame, FALSE,
                              dec->picture_structure == PIPE_MPEG12_PICTURE_STRUCTURE_FIELD_TOP,
                              x, y, mb->PMV[0][1], dec->future, TRUE);
         else assert(!backward);
         break;
      }
      default: assert(0);
      }
   }
   return;

mv1:
   base = NV17_MPEG_CMD_CHROMA_MV_HEADER_MV_SPLIT_HALF_MB;
   if (frame)
       base |= NV17_MPEG_CMD_CHROMA_MV_HEADER_TYPE_FRAME;
    /* frame 16x16 */
   if (forward)
       nouveau_vpe_mb_mv(dec, base, luma, frame, TRUE, FALSE,
                         x, y, mb->PMV[0][0], dec->past, TRUE);
   if (backward)
       nouveau_vpe_mb_mv(dec, base, luma, frame, !forward, FALSE,
                         x, y, mb->PMV[0][1], dec->future, TRUE);
    return;

mv2:
   base = NV17_MPEG_CMD_CHROMA_MV_HEADER_COUNT_2;
   if (!frame)
      base |= NV17_MPEG_CMD_CHROMA_MV_HEADER_MV_SPLIT_HALF_MB;
   if (forward) {
      nouveau_vpe_mb_mv(dec, base, luma, frame, TRUE,
                        mb->motion_vertical_field_select & PIPE_MPEG12_FS_FIRST_FORWARD,
                        x, y, mb->PMV[0][0], dec->past, TRUE);
      nouveau_vpe_mb_mv(dec, base, luma, frame, TRUE,
                        mb->motion_vertical_field_select & PIPE_MPEG12_FS_SECOND_FORWARD,
                        x, y2, mb->PMV[1][0], dec->past, FALSE);
   }
   if (backward) {
      nouveau_vpe_mb_mv(dec, base, luma, frame, !forward,
                        mb->motion_vertical_field_select & PIPE_MPEG12_FS_FIRST_BACKWARD,
                        x, y, mb->PMV[0][1], dec->future, TRUE);
      nouveau_vpe_mb_mv(dec, base, luma, frame, !forward,
                        mb->motion_vertical_field_select & PIPE_MPEG12_FS_SECOND_BACKWARD,
                        x, y2, mb->PMV[1][1], dec->future, FALSE);
   }
}

static unsigned
nouveau_decoder_surface_index(struct nouveau_decoder *dec,
                              struct pipe_video_buffer *buffer)
{
   struct nouveau_video_buffer *buf = (struct nouveau_video_buffer *)buffer;
   struct nouveau_channel *chan = dec->screen->channel;
   struct nouveau_bo *bo_y, *bo_c;
   unsigned i;

   if (!buf)
      return 8;
   for (i = 0; i < dec->num_surfaces; ++i) {
      if (dec->surfaces[i] == buf)
         return i;
   }
   assert(i < 8);
   dec->surfaces[i] = buf;
   dec->num_surfaces++;

   if (nouveau_video_is_nvfx(dec)) {
      bo_y = ((struct nvfx_resource *)buf->resources[0])->bo;
      bo_c = ((struct nvfx_resource *)buf->resources[1])->bo;
   } else {
      bo_y = ((struct nv04_resource *)buf->resources[0])->bo;
      bo_c = ((struct nv04_resource *)buf->resources[1])->bo;
   }
   MARK_RING(chan, 3, 2);
   BEGIN_RING(chan, dec->mpeg, NV31_MPEG_IMAGE_Y_OFFSET(i), 2);
   OUT_RELOCl(chan, bo_y, 0, NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, bo_c, 0, NOUVEAU_BO_RDWR);
   return i;
}

static void
nouveau_decoder_set_picture_parameters(struct pipe_video_decoder *decoder,
                                       struct pipe_picture_desc *picture_desc)
{
   struct nouveau_decoder *dec = (struct nouveau_decoder *)decoder;
   struct pipe_mpeg12_picture_desc *desc;
   desc = (struct pipe_mpeg12_picture_desc *)picture_desc;
   dec->picture_structure = desc->picture_structure;
}

static void
nouveau_decoder_set_reference_frames(struct pipe_video_decoder *decoder,
                                     struct pipe_video_buffer **buffers,
                                     unsigned count)
{
   struct nouveau_decoder *dec = (struct nouveau_decoder *)decoder;
   if (count >= 1 && buffers[0])
      dec->past = nouveau_decoder_surface_index(dec, buffers[0]);
   if (count >= 2 && buffers[1])
      dec->future = nouveau_decoder_surface_index(dec, buffers[1]);
}

static void
nouveau_decoder_set_decode_target(struct pipe_video_decoder *decoder,
                                  struct pipe_video_buffer *buffer)
{
   struct nouveau_decoder *dec = (struct nouveau_decoder *)decoder;
   dec->current = nouveau_decoder_surface_index(dec, buffer);
}

static void
nouveau_decoder_decode_macroblock(struct pipe_video_decoder *decoder,
                                  const struct pipe_macroblock *pipe_mb,
                                  unsigned num_macroblocks)
{
   struct nouveau_decoder *dec = (struct nouveau_decoder *)decoder;
   const struct pipe_mpeg12_macroblock *mb;
   unsigned i;
   assert(dec->current < 8);

   if (nouveau_vpe_init(dec)) return;
   mb = (const struct pipe_mpeg12_macroblock *)pipe_mb;
   for (i = 0; i < num_macroblocks; ++i, mb++) {
      if (mb->macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA) {
         nouveau_vpe_mb_dct_header(dec, mb, TRUE);
         nouveau_vpe_mb_dct_header(dec, mb, FALSE);
      } else {
         nouveau_vpe_mb_mv_header(dec, mb, TRUE);
         nouveau_vpe_mb_dct_header(dec, mb, TRUE);

         nouveau_vpe_mb_mv_header(dec, mb, FALSE);
         nouveau_vpe_mb_dct_header(dec, mb, FALSE);
      }
      if (dec->base.entrypoint <= PIPE_VIDEO_ENTRYPOINT_IDCT)
         nouveau_vpe_mb_dct_blocks(dec, mb);
      else
         nouveau_vpe_mb_data_blocks(dec, mb);
   }
}

static void
nouveau_decoder_flush(struct pipe_video_decoder *decoder)
{
   struct nouveau_decoder *dec = (struct nouveau_decoder *)decoder;
   if (dec->ofs)
      nouveau_vpe_fini(dec);
}

static void
nouveau_decoder_destroy(struct pipe_video_decoder *decoder)
{
   struct nouveau_decoder *dec = (struct nouveau_decoder*)decoder;

   if (dec->cmds) {
      nouveau_bo_unmap(dec->data_bo);
      nouveau_bo_unmap(dec->cmd_bo);
   }

   if (dec->data_bo)
      nouveau_bo_ref(NULL, &dec->data_bo);
   if (dec->cmd_bo)
      nouveau_bo_ref(NULL, &dec->cmd_bo);
   if (dec->fence_bo)
      nouveau_bo_ref(NULL, &dec->fence_bo);
   nouveau_grobj_free(&dec->mpeg);
   FREE(dec);
}

static void
nouveau_decoder_begin_frame(struct pipe_video_decoder *decoder)
{
}

static void
nouveau_decoder_end_frame(struct pipe_video_decoder *decoder)
{
}

static struct pipe_video_decoder *
nouveau_create_decoder(struct pipe_context *context,
                       struct nouveau_screen *screen,
                       enum pipe_video_profile profile,
                       enum pipe_video_entrypoint entrypoint,
                       enum pipe_video_chroma_format chroma_format,
                       unsigned width, unsigned height,
                       unsigned max_references, bool expect_chunked_decode)
{
   struct nouveau_channel *chan = screen->channel;
   struct nouveau_grobj *mpeg = NULL;
   struct nouveau_decoder *dec;
   int ret;
   bool is8274 = screen->device->chipset > 0x80;

   debug_printf("Acceleration level: %s\n", entrypoint <= PIPE_VIDEO_ENTRYPOINT_BITSTREAM ? "bit":
                                            entrypoint == PIPE_VIDEO_ENTRYPOINT_IDCT ? "IDCT" : "MC");

   if (getenv("XVMC_VL"))
      goto vl;
   if (u_reduce_video_profile(profile) != PIPE_VIDEO_CODEC_MPEG12)
      goto vl;
   if (screen->device->chipset >= 0x98 && screen->device->chipset != 0xa0)
      goto vl;

   width = align(width, 64);
   height = align(height, 64);

   if (is8274)
       ret = nouveau_grobj_alloc(chan, 0xbeef8274, 0x8274, &mpeg);
   else
       ret = nouveau_grobj_alloc(chan, 0xbeef8274, 0x3174, &mpeg);
   if (ret < 0) {
      debug_printf("Creation failed: %s (%i)\n", strerror(-ret), ret);
      return NULL;
   }

   dec = CALLOC_STRUCT(nouveau_decoder);
   if (!dec) {
      nouveau_grobj_free(&mpeg);
      goto fail;
   }
   dec->mpeg = mpeg;
   dec->base.context = context;
   dec->base.profile = profile;
   dec->base.entrypoint = entrypoint;
   dec->base.chroma_format = chroma_format;
   dec->base.width = width;
   dec->base.height = height;
   dec->base.max_references = max_references;
   dec->base.destroy = nouveau_decoder_destroy;
   dec->base.begin_frame = nouveau_decoder_begin_frame;
   dec->base.end_frame = nouveau_decoder_end_frame;
   dec->base.set_decode_target = nouveau_decoder_set_decode_target;
   dec->base.set_picture_parameters = nouveau_decoder_set_picture_parameters;
   dec->base.set_reference_frames = nouveau_decoder_set_reference_frames;
   dec->base.decode_macroblock = nouveau_decoder_decode_macroblock;
   dec->base.flush = nouveau_decoder_flush;
   dec->screen = screen;

   ret = nouveau_bo_new(dec->screen->device, NOUVEAU_BO_GART, 0, 1024 * 1024, &dec->cmd_bo);
   if (ret)
      goto fail;

   ret = nouveau_bo_new(dec->screen->device, NOUVEAU_BO_GART, 0, width * height * 6, &dec->data_bo);
   if (ret)
      goto fail;

   ret = nouveau_bo_new(dec->screen->device, NOUVEAU_BO_GART|NOUVEAU_BO_MAP, 0, 4096,
                        &dec->fence_bo);
   if (ret)
      goto fail;
   nouveau_bo_map(dec->fence_bo, NOUVEAU_BO_RDWR);
   dec->fence_map = dec->fence_bo->map;
   nouveau_bo_unmap(dec->fence_bo);
   dec->fence_map[0] = 0;

   if (is8274)
      MARK_RING(chan, 25, 3);
   else
      MARK_RING(chan, 20, 2);

   BEGIN_RING(chan, mpeg, NV31_MPEG_DMA_CMD, 1);
   OUT_RING(chan, chan->vram->handle);

   BEGIN_RING(chan, mpeg, NV31_MPEG_DMA_DATA, 1);
   OUT_RING(chan, chan->vram->handle);

   BEGIN_RING(chan, mpeg, NV31_MPEG_DMA_IMAGE, 1);
   OUT_RING(chan, chan->vram->handle);

   BEGIN_RING(chan, mpeg, NV31_MPEG_PITCH, 2);
   OUT_RING(chan, width | NV31_MPEG_PITCH_UNK);
   OUT_RING(chan, (height << NV31_MPEG_SIZE_H__SHIFT) | width);

   BEGIN_RING(chan, mpeg, NV31_MPEG_FORMAT, 2);
   OUT_RING(chan, 0);
   switch (entrypoint) {
      case PIPE_VIDEO_ENTRYPOINT_BITSTREAM: OUT_RING(chan, 0x100); break;
      case PIPE_VIDEO_ENTRYPOINT_IDCT: OUT_RING(chan, 1); break;
      case PIPE_VIDEO_ENTRYPOINT_MC: OUT_RING(chan, 0); break;
      default: assert(0);
   }

   if (is8274) {
      BEGIN_RING(chan, mpeg, NV84_MPEG_DMA_QUERY, 1);
      OUT_RING(chan, chan->vram->handle);

      BEGIN_RING(chan, mpeg, NV84_MPEG_QUERY_OFFSET, 2);
      OUT_RELOCl(chan, dec->fence_bo, 0, NOUVEAU_BO_WR|NOUVEAU_BO_GART);
      OUT_RING(chan, dec->fence_seq);
   }

   ret = nouveau_vpe_init(dec);
   if (ret)
      goto fail;
   nouveau_vpe_fini(dec);
   return &dec->base;

fail:
   nouveau_decoder_destroy(&dec->base);
   return NULL;

vl:
   debug_printf("Using g3dvl renderer\n");
   return vl_create_decoder(context, profile, entrypoint,
                            chroma_format, width, height,
                            max_references, expect_chunked_decode);
}

static struct pipe_sampler_view **
nouveau_video_buffer_sampler_view_planes(struct pipe_video_buffer *buffer)
{
   struct nouveau_video_buffer *buf = (struct nouveau_video_buffer *)buffer;
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
   unsigned i;

   assert(buf);

   pipe = buf->base.context;

   for (i = 0; i < buf->num_planes; ++i ) {
      if (!buf->sampler_view_planes[i]) {
         memset(&sv_templ, 0, sizeof(sv_templ));
         u_sampler_view_default_template(&sv_templ, buf->resources[i], buf->resources[i]->format);

         if (util_format_get_nr_components(buf->resources[i]->format) == 1)
            sv_templ.swizzle_r = sv_templ.swizzle_g = sv_templ.swizzle_b = sv_templ.swizzle_a = PIPE_SWIZZLE_RED;

         buf->sampler_view_planes[i] = pipe->create_sampler_view(pipe, buf->resources[i], &sv_templ);
         if (!buf->sampler_view_planes[i])
            goto error;
      }
   }

   return buf->sampler_view_planes;

error:
   for (i = 0; i < buf->num_planes; ++i )
      pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);

   return NULL;
}

static struct pipe_sampler_view **
nouveau_video_buffer_sampler_view_components(struct pipe_video_buffer *buffer)
{
   struct nouveau_video_buffer *buf = (struct nouveau_video_buffer *)buffer;
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
   unsigned i, j, component;

   assert(buf);

   pipe = buf->base.context;

   for (component = 0, i = 0; i < buf->num_planes; ++i ) {
      unsigned nr_components = util_format_get_nr_components(buf->resources[i]->format);

      for (j = 0; j < nr_components; ++j, ++component) {
         assert(component < VL_MAX_PLANES);

         if (!buf->sampler_view_components[component]) {
            memset(&sv_templ, 0, sizeof(sv_templ));
            u_sampler_view_default_template(&sv_templ, buf->resources[i], buf->resources[i]->format);
            sv_templ.swizzle_r = sv_templ.swizzle_g = sv_templ.swizzle_b = PIPE_SWIZZLE_RED + j;
            sv_templ.swizzle_a = PIPE_SWIZZLE_ONE;
            buf->sampler_view_components[component] = pipe->create_sampler_view(pipe, buf->resources[i], &sv_templ);
            if (!buf->sampler_view_components[component])
               goto error;
         }
      }
   }

   return buf->sampler_view_components;

error:
   for (i = 0; i < 3; ++i )
      pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);

   return NULL;
}

static struct pipe_surface **
nouveau_video_buffer_surfaces(struct pipe_video_buffer *buffer)
{
   struct nouveau_video_buffer *buf = (struct nouveau_video_buffer *)buffer;
   struct pipe_surface surf_templ;
   struct pipe_context *pipe;
   unsigned i;

   assert(buf);

   pipe = buf->base.context;

   for (i = 0; i < buf->num_planes; ++i ) {
      if (!buf->surfaces[i]) {
         memset(&surf_templ, 0, sizeof(surf_templ));
         surf_templ.format = buf->resources[i]->format;
         surf_templ.usage = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
         buf->surfaces[i] = pipe->create_surface(pipe, buf->resources[i], &surf_templ);
         if (!buf->surfaces[i])
            goto error;
      }
   }

   return buf->surfaces;

error:
   for (i = 0; i < buf->num_planes; ++i )
      pipe_surface_reference(&buf->surfaces[i], NULL);

   return NULL;
}

static void
nouveau_video_buffer_destroy(struct pipe_video_buffer *buffer)
{
   struct nouveau_video_buffer *buf = (struct nouveau_video_buffer *)buffer;
   unsigned i;

   assert(buf);

   for (i = 0; i < buf->num_planes; ++i) {
      pipe_surface_reference(&buf->surfaces[i], NULL);
      pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);
      pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);
      pipe_resource_reference(&buf->resources[i], NULL);
   }
   for (;i < 3;++i)
      pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);

   FREE(buffer);
}

static struct pipe_video_buffer *
nouveau_video_buffer_create(struct pipe_context *pipe,
                            struct nouveau_screen *screen,
                            enum pipe_format buffer_format,
                            enum pipe_video_chroma_format chroma_format,
                            unsigned width, unsigned height)
{
   struct nouveau_video_buffer *buffer;
   struct pipe_resource templ;

   /* Only do a linear surface when a hardware decoder is used
    * hardware decoder is only supported on some chipsets
    * and it only supports the NV12 format
    */
   if (buffer_format != PIPE_FORMAT_NV12 || getenv("XVMC_VL") ||
       (screen->device->chipset >= 0x98 && screen->device->chipset != 0xa0))
      return vl_video_buffer_create(pipe, buffer_format, chroma_format, width, height);

   assert(chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420);
   width = align(width, 64);
   height = align(height, 64);

   buffer = CALLOC_STRUCT(nouveau_video_buffer);
   if (!buffer)
      return NULL;

   buffer->base.context = pipe;
   buffer->base.destroy = nouveau_video_buffer_destroy;
   buffer->base.get_sampler_view_planes = nouveau_video_buffer_sampler_view_planes;
   buffer->base.get_sampler_view_components = nouveau_video_buffer_sampler_view_components;
   buffer->base.get_surfaces = nouveau_video_buffer_surfaces;
   buffer->base.chroma_format = chroma_format;
   buffer->base.width = width;
   buffer->base.height = height;
   buffer->num_planes = 2;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = PIPE_FORMAT_R8_UNORM;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   templ.usage = PIPE_USAGE_STATIC;
   templ.flags = NOUVEAU_RESOURCE_FLAG_LINEAR;

   buffer->resources[0] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[0])
      goto error;
   templ.width0 /= 2;
   templ.height0 /= 2;
   templ.format = PIPE_FORMAT_R8G8_UNORM;
   buffer->resources[1] = pipe->screen->resource_create(pipe->screen, &templ);
   if (!buffer->resources[1])
      goto error;
   return &buffer->base;

error:
   nouveau_video_buffer_destroy(&buffer->base);
   return NULL;
}

static int
nouveau_screen_get_video_param(struct pipe_screen *pscreen,
                               enum pipe_video_profile profile,
                               enum pipe_video_cap param)
{
   switch (param) {
   case PIPE_VIDEO_CAP_SUPPORTED:
      return vl_profile_supported(pscreen, profile);
   case PIPE_VIDEO_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
      return vl_video_buffer_max_size(pscreen);
   default:
      debug_printf("unknown video param: %d\n", param);
      return 0;
   }
}

void
nouveau_screen_init_vdec(struct nouveau_screen *screen)
{
   screen->base.get_video_param = nouveau_screen_get_video_param;
   screen->base.is_video_format_supported = vl_video_buffer_is_format_supported;
}

static struct pipe_video_decoder *
nvfx_context_create_decoder(struct pipe_context *context,
                            enum pipe_video_profile profile,
                            enum pipe_video_entrypoint entrypoint,
                            enum pipe_video_chroma_format chroma_format,
                            unsigned width, unsigned height,
                            unsigned max_references, bool expect_chunked_decode)
{
   struct nouveau_screen *screen = &nvfx_context(context)->screen->base;
   return nouveau_create_decoder(context, screen, profile, entrypoint,
                                 chroma_format, width, height,
                                 max_references, expect_chunked_decode);
}

static struct pipe_video_buffer *
nvfx_context_video_buffer_create(struct pipe_context *pipe,
                                 enum pipe_format buffer_format,
                                 enum pipe_video_chroma_format chroma_format,
                                 unsigned width, unsigned height)
{
   struct nouveau_screen *screen = &nvfx_context(pipe)->screen->base;
   return nouveau_video_buffer_create(pipe, screen, buffer_format, chroma_format, width, height);
}

void
nvfx_context_init_vdec(struct nvfx_context *nv)
{
   nv->pipe.create_video_decoder = nvfx_context_create_decoder;
   nv->pipe.create_video_buffer = nvfx_context_video_buffer_create;
}

static struct pipe_video_decoder *
nouveau_context_create_decoder(struct pipe_context *context,
                               enum pipe_video_profile profile,
                               enum pipe_video_entrypoint entrypoint,
                               enum pipe_video_chroma_format chroma_format,
                               unsigned width, unsigned height,
                               unsigned max_references, bool expect_chunked_decode)
{
   struct nouveau_screen *screen = nouveau_context(context)->screen;
   return nouveau_create_decoder(context, screen, profile, entrypoint,
                                 chroma_format, width, height,
                                 max_references, expect_chunked_decode);
}

static struct pipe_video_buffer *
nouveau_context_video_buffer_create(struct pipe_context *pipe,
                                    enum pipe_format buffer_format,
                                    enum pipe_video_chroma_format chroma_format,
                                    unsigned width, unsigned height)
{
   struct nouveau_screen *screen = nouveau_context(pipe)->screen;
   return nouveau_video_buffer_create(pipe, screen, buffer_format, chroma_format, width, height);
}

void
nouveau_context_init_vdec(struct nouveau_context *nv)
{
   nv->pipe.create_video_decoder = nouveau_context_create_decoder;
   nv->pipe.create_video_buffer = nouveau_context_video_buffer_create;
}
