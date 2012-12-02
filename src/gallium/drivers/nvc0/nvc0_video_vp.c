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
#include <sys/mman.h>

struct mpeg12_picparm_vp {
	uint16_t width; // 00 in mb units
	uint16_t height; // 02 in mb units

	uint32_t unk04; // 04 stride for Y?
	uint32_t unk08; // 08 stride for CbCr?

	uint32_t ofs[6]; // 1c..20 ofs
	uint32_t bucket_size; // 24
	uint32_t inter_ring_data_size; // 28
	uint16_t unk2c; // 2c
	uint16_t alternate_scan; // 2e
	uint16_t unk30; // 30 not seen set yet
	uint16_t picture_structure; // 32
	uint16_t pad2[3];
	uint16_t unk3a; // 3a set on I frame?

	uint32_t f_code[4]; // 3c
	uint32_t picture_coding_type; // 4c
	uint32_t intra_dc_precision; // 50
	uint32_t q_scale_type; // 54
	uint32_t top_field_first; // 58
	uint32_t full_pel_forward_vector; // 5c
	uint32_t full_pel_backward_vector; // 60
	uint8_t intra_quantizer_matrix[0x40]; // 64
	uint8_t non_intra_quantizer_matrix[0x40]; // a4
};

struct mpeg4_picparm_vp {
	uint32_t width; // 00 in normal units
	uint32_t height; // 04 in normal units
	uint32_t unk08; // stride 1
	uint32_t unk0c; // stride 2
	uint32_t ofs[6]; // 10..24 ofs
	uint32_t bucket_size; // 28
	uint32_t pad1; // 2c, pad
	uint32_t pad2; // 30
	uint32_t inter_ring_data_size; // 34

	uint32_t trd[2]; // 38, 3c
	uint32_t trb[2]; // 40, 44
	uint32_t u48; // XXX codec selection? Should test with different values of VdpDecoderProfile
	uint16_t f_code_fw; // 4c
	uint16_t f_code_bw; // 4e
	uint8_t interlaced; // 50

	uint8_t quant_type; // bool, written to 528
	uint8_t quarter_sample; // bool, written to 548
	uint8_t short_video_header; // bool, negated written to 528 shifted by 1
	uint8_t u54; // bool, written to 0x740
	uint8_t vop_coding_type; // 55
	uint8_t rounding_control; // 56
	uint8_t alternate_vertical_scan_flag; // 57 bool
	uint8_t top_field_first; // bool, written to vuc

	uint8_t pad4[3]; // 59, 5a, 5b, contains garbage on blob
	uint32_t pad5[0x10]; // 5c...9c non-inclusive, but WHY?

	uint32_t intra[0x10]; // 9c
	uint32_t non_intra[0x10]; // bc
	// udc..uff pad?
};

// Full version, with data pumped from BSP
struct vc1_picparm_vp {
	uint32_t bucket_size; // 00
	uint32_t pad; // 04

	uint32_t inter_ring_data_size; // 08
	uint32_t unk0c; // stride 1
	uint32_t unk10; // stride 2
	uint32_t ofs[6]; // 14..28 ofs

	uint16_t width; // 2c
	uint16_t height; // 2e

	uint8_t profile; // 30 0 = simple, 1 = main, 2 = advanced
	uint8_t loopfilter; // 31 written into vuc
	uint8_t fastuvmc; // 32, written into vuc
	uint8_t dquant; // 33

	uint8_t overlap; // 34
	uint8_t quantizer; // 35
	uint8_t u36; // 36, bool
	uint8_t pad2; // 37, to align to 0x38
};

struct h264_picparm_vp { // 700..a00
	uint16_t width, height;
	uint32_t stride1, stride2; // 04 08
	uint32_t ofs[6]; // 0c..24 in-image offset

	uint32_t u24; // nfi ac8 ?
	uint32_t bucket_size; // 28 bucket size
	uint32_t inter_ring_data_size; // 2c

	unsigned f0 : 1; // 0 0x01: into 640 shifted by 3, 540 shifted by 5, half size something?
	unsigned f1 : 1; // 1 0x02: into vuc ofs 56
	unsigned weighted_pred_flag : 1; // 2 0x04
	unsigned f3 : 1; // 3 0x08: into vuc ofs 68
	unsigned is_reference : 1; // 4
	unsigned interlace : 1; // 5 field_pic_flag
	unsigned bottom_field_flag : 1; // 6
	unsigned f7 : 1; // 7 0x80: nfi yet

	signed log2_max_frame_num_minus4 : 4; // 31 0..3
	unsigned u31_45 : 2; // 31 4..5
	unsigned pic_order_cnt_type : 2; // 31 6..7
	signed pic_init_qp_minus26 : 6; // 32 0..5
	signed chroma_qp_index_offset : 5; // 32 6..10
	signed second_chroma_qp_index_offset : 5; // 32 11..15

	unsigned weighted_bipred_idc : 2; // 34 0..1
	unsigned fifo_dec_index : 7; // 34 2..8
	unsigned tmp_idx : 5; // 34 9..13
	unsigned frame_number : 16; // 34 14..29
	unsigned u34_3030 : 1; // 34 30..30 pp.u34[30:30]
	unsigned u34_3131 : 1; // 34 31..31 pad?

	uint32_t field_order_cnt[2]; // 38, 3c

	struct { // 40
		// 0x00223102
		// nfi (needs: top_is_reference, bottom_is_reference, is_long_term, maybe some other state that was saved..
		unsigned fifo_idx : 7; // 00 0..6
		unsigned tmp_idx : 5; // 00 7..11
		unsigned unk12 : 1; // 00 12 not seen yet, but set, maybe top_is_reference
		unsigned unk13 : 1; // 00 13 not seen yet, but set, maybe bottom_is_reference?
		unsigned unk14 : 1; // 00 14 skipped?
		unsigned notseenyet : 1; // 00 15 pad?
		unsigned unk16 : 1; // 00 16
		unsigned unk17 : 4; // 00 17..20
		unsigned unk21 : 4; // 00 21..24
		unsigned pad : 7; // 00 d25..31

		uint32_t field_order_cnt[2]; // 04,08
		uint32_t frame_idx; // 0c
	} refs[0x10];

	uint8_t m4x4[6][16]; // 140
	uint8_t m8x8[2][64]; // 1a0
	uint32_t u220; // 220 number of extra reorder_list to append?
	uint8_t u224[0x20]; // 224..244 reorder_list append ?
	uint8_t nfi244[0xb0]; // add some pad to make sure nulls are read
};

static void
nvc0_decoder_handle_references(struct nvc0_decoder *dec, struct nvc0_video_buffer *refs[16], unsigned seq, struct nvc0_video_buffer *target)
{
   unsigned h264 = u_reduce_video_profile(dec->base.profile) == PIPE_VIDEO_CODEC_MPEG4_AVC;
   unsigned i, idx, empty_spot = dec->base.max_references + 1;
   for (i = 0; i < dec->base.max_references; ++i) {
      if (!refs[i])
         continue;

      idx = refs[i]->valid_ref;
      //debug_printf("ref[%i] %p in slot %i\n", i, refs[i], idx);
      assert(target != refs[i] ||
             (h264 && empty_spot &&
              (!dec->refs[idx].decoded_bottom || !dec->refs[idx].decoded_top)));
      if (target == refs[i])
         empty_spot = 0;
      assert(!h264 ||
             dec->refs[idx].last_used == seq - 1);

      if (dec->refs[idx].vidbuf != refs[i]) {
         debug_printf("%p is not a real ref\n", refs[i]);
         // FIXME: Maybe do m2mf copy here if a application really depends on it?
         continue;
      }

      assert(dec->refs[idx].vidbuf == refs[i]);
      dec->refs[idx].last_used = seq;
   }
   if (!empty_spot)
      return;

   /* Try to find a real empty spot first, there should be one..
    */
   for (i = 0; i < dec->base.max_references + 1; ++i) {
      if (dec->refs[i].last_used < seq) {
         if (!dec->refs[i].vidbuf) {
            empty_spot = i;
            break;
         }
         if (empty_spot < dec->base.max_references+1 &&
             dec->refs[empty_spot].last_used < dec->refs[i].last_used)
            continue;
         empty_spot = i;
      }
   }
   assert(empty_spot < dec->base.max_references+1);
   dec->refs[empty_spot].last_used = seq;
//   debug_printf("Kicked %p to add %p to slot %i\n", dec->refs[empty_spot].vidbuf, target, i);
   dec->refs[empty_spot].vidbuf = target;
   dec->refs[empty_spot].decoded_bottom = dec->refs[empty_spot].decoded_top = 0;
   target->valid_ref = empty_spot;
}

static void
nvc0_decoder_kick_ref(struct nvc0_decoder *dec, struct nvc0_video_buffer *target)
{
   dec->refs[target->valid_ref].vidbuf = NULL;
   dec->refs[target->valid_ref].last_used = 0;
//   debug_printf("Unreffed %p\n", target);
}

static uint32_t
nvc0_decoder_fill_picparm_mpeg12_vp(struct nvc0_decoder *dec,
                                    struct pipe_mpeg12_picture_desc *desc,
                                    struct nvc0_video_buffer *refs[16],
                                    unsigned *is_ref,
                                    char *map)
{
   struct mpeg12_picparm_vp pic_vp_stub = {}, *pic_vp = &pic_vp_stub;
   uint32_t i, ret = 0x01010, ring; // !async_shutdown << 16 | watchdog << 12 | irq_record << 4 | unk;
   assert(!(dec->base.width & 0xf));
   *is_ref = desc->picture_coding_type <= 2;

   if (dec->base.profile == PIPE_VIDEO_PROFILE_MPEG1)
      pic_vp->picture_structure = 3;
   else
      pic_vp->picture_structure = desc->picture_structure;

   assert(desc->picture_structure != 4);
   if (desc->picture_structure == 4) // Untested, but should work
      ret |= 0x100;
   pic_vp->width = mb(dec->base.width);
   pic_vp->height = mb(dec->base.height);
   pic_vp->unk08 = pic_vp->unk04 = (dec->base.width+0xf)&~0xf; // Stride

   nvc0_decoder_ycbcr_offsets(dec, &pic_vp->ofs[1], &pic_vp->ofs[3], &pic_vp->ofs[4]);
   pic_vp->ofs[5] = pic_vp->ofs[3];
   pic_vp->ofs[0] = pic_vp->ofs[2] = 0;
   nvc0_decoder_inter_sizes(dec, 1, &ring, &pic_vp->bucket_size, &pic_vp->inter_ring_data_size);

   pic_vp->alternate_scan = desc->alternate_scan;
   pic_vp->pad2[0] = pic_vp->pad2[1] = pic_vp->pad2[2] = 0;
   pic_vp->unk30 = desc->picture_structure < 3 && (desc->picture_structure == 2 - desc->top_field_first);
   pic_vp->unk3a = (desc->picture_coding_type == 1);
   for (i = 0; i < 4; ++i)
      pic_vp->f_code[i] = desc->f_code[i/2][i%2] + 1; // FU
   pic_vp->picture_coding_type = desc->picture_coding_type;
   pic_vp->intra_dc_precision = desc->intra_dc_precision;
   pic_vp->q_scale_type = desc->q_scale_type;
   pic_vp->top_field_first = desc->top_field_first;
   pic_vp->full_pel_forward_vector = desc->full_pel_forward_vector;
   pic_vp->full_pel_backward_vector = desc->full_pel_backward_vector;
   memcpy(pic_vp->intra_quantizer_matrix, desc->intra_matrix, 0x40);
   memcpy(pic_vp->non_intra_quantizer_matrix, desc->non_intra_matrix, 0x40);
   memcpy(map, pic_vp, sizeof(*pic_vp));
   refs[0] = (struct nvc0_video_buffer *)desc->ref[0];
   refs[!!refs[0]] = (struct nvc0_video_buffer *)desc->ref[1];
   return ret | (dec->base.profile != PIPE_VIDEO_PROFILE_MPEG1);
}

static uint32_t
nvc0_decoder_fill_picparm_mpeg4_vp(struct nvc0_decoder *dec,
                                   struct pipe_mpeg4_picture_desc *desc,
                                   struct nvc0_video_buffer *refs[16],
                                   unsigned *is_ref,
                                   char *map)
{
   struct mpeg4_picparm_vp pic_vp_stub = {}, *pic_vp = &pic_vp_stub;
   uint32_t ring, ret = 0x01014; // !async_shutdown << 16 | watchdog << 12 | irq_record << 4 | unk;
   assert(!(dec->base.width & 0xf));
   *is_ref = desc->vop_coding_type <= 1;

   pic_vp->width = dec->base.width;
   pic_vp->height = mb(dec->base.height)<<4;
   pic_vp->unk0c = pic_vp->unk08 = mb(dec->base.width)<<4; // Stride

   nvc0_decoder_ycbcr_offsets(dec, &pic_vp->ofs[1], &pic_vp->ofs[3], &pic_vp->ofs[4]);
   pic_vp->ofs[5] = pic_vp->ofs[3];
   pic_vp->ofs[0] = pic_vp->ofs[2] = 0;
   pic_vp->pad1 = pic_vp->pad2 = 0;
   nvc0_decoder_inter_sizes(dec, 1, &ring, &pic_vp->bucket_size, &pic_vp->inter_ring_data_size);

   pic_vp->trd[0] = desc->trd[0];
   pic_vp->trd[1] = desc->trd[1];
   pic_vp->trb[0] = desc->trb[0];
   pic_vp->trb[1] = desc->trb[1];
   pic_vp->u48 = 0; // Codec?
   pic_vp->pad1 = pic_vp->pad2 = 0;
   pic_vp->f_code_fw = desc->vop_fcode_forward;
   pic_vp->f_code_bw = desc->vop_fcode_backward;
   pic_vp->interlaced = desc->interlaced;
   pic_vp->quant_type = desc->quant_type;
   pic_vp->quarter_sample = desc->quarter_sample;
   pic_vp->short_video_header = desc->short_video_header;
   pic_vp->u54 = 0;
   pic_vp->vop_coding_type = desc->vop_coding_type;
   pic_vp->rounding_control = desc->rounding_control;
   pic_vp->alternate_vertical_scan_flag = desc->alternate_vertical_scan_flag;
   pic_vp->top_field_first = desc->top_field_first;

   memcpy(pic_vp->intra, desc->intra_matrix, 0x40);
   memcpy(pic_vp->non_intra, desc->non_intra_matrix, 0x40);
   memcpy(map, pic_vp, sizeof(*pic_vp));
   refs[0] = (struct nvc0_video_buffer *)desc->ref[0];
   refs[!!refs[0]] = (struct nvc0_video_buffer *)desc->ref[1];
   return ret;
}

static uint32_t
nvc0_decoder_fill_picparm_h264_vp(struct nvc0_decoder *dec,
                                  const struct pipe_h264_picture_desc *d,
                                  struct nvc0_video_buffer *refs[16],
                                  unsigned *is_ref,
                                  char *map)
{
   struct h264_picparm_vp stub_h = {}, *h = &stub_h;
   unsigned ring, i, j = 0;
   assert(offsetof(struct h264_picparm_vp, u224) == 0x224);
   *is_ref = d->is_reference;
   assert(!d->frame_num || dec->last_frame_num + 1 == d->frame_num || dec->last_frame_num == d->frame_num);
   dec->last_frame_num = d->frame_num;

   h->width = mb(dec->base.width);
   h->height = mb(dec->base.height);
   h->stride1 = h->stride2 = mb(dec->base.width)*16;
   nvc0_decoder_ycbcr_offsets(dec, &h->ofs[1], &h->ofs[3], &h->ofs[4]);
   h->ofs[5] = h->ofs[3];
   h->ofs[0] = h->ofs[2] = 0;
   h->u24 = dec->tmp_stride >> 8;
   assert(h->u24);
   nvc0_decoder_inter_sizes(dec, 1, &ring, &h->bucket_size, &h->inter_ring_data_size);

   h->u220 = 0;
   h->f0 = d->mb_adaptive_frame_field_flag;
   h->f1 = d->direct_8x8_inference_flag;
   h->weighted_pred_flag = d->weighted_pred_flag;
   h->f3 = d->constrained_intra_pred_flag;
   h->is_reference = d->is_reference;
   h->interlace = d->field_pic_flag;
   h->bottom_field_flag = d->bottom_field_flag;
   h->f7 = 0; // TODO: figure out when set..
   h->log2_max_frame_num_minus4 = d->log2_max_frame_num_minus4;
   h->u31_45 = 1;

   h->pic_order_cnt_type = d->pic_order_cnt_type;
   h->pic_init_qp_minus26 = d->pic_init_qp_minus26;
   h->chroma_qp_index_offset = d->chroma_qp_index_offset;
   h->second_chroma_qp_index_offset = d->second_chroma_qp_index_offset;
   h->weighted_bipred_idc = d->weighted_bipred_idc;
   h->tmp_idx = 0; // set in h264_vp_refs below
   h->fifo_dec_index = 0; // always set to 0 to be fifo compatible with other codecs
   h->frame_number = d->frame_num;
   h->u34_3030 = h->u34_3131 = 0;
   h->field_order_cnt[0] = d->field_order_cnt[0];
   h->field_order_cnt[1] = d->field_order_cnt[1];
   memset(h->refs, 0, sizeof(h->refs));
   memcpy(h->m4x4, d->scaling_lists_4x4, sizeof(h->m4x4) + sizeof(h->m8x8));
   h->u220 = 0;
   for (i = 0; i < d->num_ref_frames; ++i) {
      if (!d->ref[i])
         break;
      refs[j] = (struct nvc0_video_buffer *)d->ref[i];
      h->refs[j].fifo_idx = j + 1;
      h->refs[j].tmp_idx = refs[j]->valid_ref;
      h->refs[j].field_order_cnt[0] = d->field_order_cnt_list[i][0];
      h->refs[j].field_order_cnt[1] = d->field_order_cnt_list[i][1];
      h->refs[j].frame_idx = d->frame_num_list[i];
      if (!dec->refs[refs[j]->valid_ref].field_pic_flag) {
         h->refs[j].unk12 = d->top_is_reference[i];
         h->refs[j].unk13 = d->bottom_is_reference[i];
      }
      h->refs[j].unk14 = 0;
      h->refs[j].notseenyet = 0;
      h->refs[j].unk16 = dec->refs[refs[j]->valid_ref].field_pic_flag;
      h->refs[j].unk17 = dec->refs[refs[j]->valid_ref].decoded_top &&
                         d->top_is_reference[i];
      h->refs[j].unk21 = dec->refs[refs[j]->valid_ref].decoded_bottom &&
                         d->bottom_is_reference[i];
      h->refs[j].pad = 0;
      assert(!d->is_long_term[i]);
      j++;
   }
   for (; i < 16; ++i)
      assert(!d->ref[i]);
   assert(d->num_ref_frames <= dec->base.max_references);

   for (; i < d->num_ref_frames; ++i)
      h->refs[j].unk16 = d->field_pic_flag;
   *(struct h264_picparm_vp *)map = *h;

   return 0x1113;
}

static void
nvc0_decoder_fill_picparm_h264_vp_refs(struct nvc0_decoder *dec,
                                       struct pipe_h264_picture_desc *d,
                                       struct nvc0_video_buffer *refs[16],
                                       struct nvc0_video_buffer *target,
                                       char *map)
{
   struct h264_picparm_vp *h = (struct h264_picparm_vp *)map;
   assert(dec->refs[target->valid_ref].vidbuf == target);
//    debug_printf("Target: %p\n", target);

   h->tmp_idx = target->valid_ref;
   dec->refs[target->valid_ref].field_pic_flag = d->field_pic_flag;
   if (!d->field_pic_flag || d->bottom_field_flag)
      dec->refs[target->valid_ref].decoded_bottom = 1;
   if (!d->field_pic_flag || !d->bottom_field_flag)
      dec->refs[target->valid_ref].decoded_top = 1;
}

static uint32_t
nvc0_decoder_fill_picparm_vc1_vp(struct nvc0_decoder *dec,
                                 struct pipe_vc1_picture_desc *d,
                                 struct nvc0_video_buffer *refs[16],
                                 unsigned *is_ref,
                                 char *map)
{
   struct vc1_picparm_vp *vc = (struct vc1_picparm_vp *)map;
   unsigned ring;
   assert(dec->base.profile != PIPE_VIDEO_PROFILE_VC1_SIMPLE);
   *is_ref = d->picture_type <= 1;

   nvc0_decoder_ycbcr_offsets(dec, &vc->ofs[1], &vc->ofs[3], &vc->ofs[4]);
   vc->ofs[5] = vc->ofs[3];
   vc->ofs[0] = vc->ofs[2] = 0;
   vc->width = dec->base.width;
   vc->height = mb(dec->base.height)<<4;
   vc->unk0c = vc->unk10 = mb(dec->base.width)<<4; // Stride
   vc->pad = vc->pad2 = 0;
   nvc0_decoder_inter_sizes(dec, 1, &ring, &vc->bucket_size, &vc->inter_ring_data_size);
   vc->profile = dec->base.profile - PIPE_VIDEO_PROFILE_VC1_SIMPLE;
   vc->loopfilter = d->loopfilter;
   vc->fastuvmc = d->fastuvmc;
   vc->dquant = d->dquant;
   vc->overlap = d->overlap;
   vc->quantizer = d->quantizer;
   vc->u36 = 0; // ? No idea what this one is..
   refs[0] = (struct nvc0_video_buffer *)d->ref[0];
   refs[!!refs[0]] = (struct nvc0_video_buffer *)d->ref[1];
   return 0x12;
}

#if NVC0_DEBUG_FENCE
static void dump_comm_vp(struct nvc0_decoder *dec, struct comm *comm, u32 comm_seq,
                         struct nouveau_bo *inter_bo, unsigned slice_size)
{
	unsigned i, idx = comm->pvp_cur_index & 0xf;
	debug_printf("Status: %08x, stage: %08x\n", comm->status_vp[idx], comm->pvp_stage);
#if 0
	debug_printf("Acked byte ofs: %x, bsp byte ofs: %x\n", comm->acked_byte_ofs, comm->byte_ofs);
	debug_printf("Irq/parse indexes: %i %i\n", comm->irq_index, comm->parse_endpos_index);

	for (i = 0; i != comm->irq_index; ++i)
		debug_printf("irq[%i] = { @ %08x -> %04x }\n", i, comm->irq_pos[i], comm->irq_470[i]);
	for (i = 0; i != comm->parse_endpos_index; ++i)
		debug_printf("parse_endpos[%i] = { @ %08x}\n", i, comm->parse_endpos[i]);
#endif
	debug_printf("mb_y = %u\n", comm->mb_y[idx]);
	if (comm->status_vp[idx] == 1)
		return;

	if ((comm->pvp_stage & 0xff) != 0xff) {
		unsigned *map;
		assert(nouveau_bo_map(inter_bo, NOUVEAU_BO_RD|NOUVEAU_BO_NOBLOCK, dec->client) >= 0);
		map = inter_bo->map;
		for (i = 0; i < comm->byte_ofs + slice_size; i += 0x10) {
			debug_printf("%05x: %08x %08x %08x %08x\n", i, map[i/4], map[i/4+1], map[i/4+2], map[i/4+3]);
		}
		munmap(inter_bo->map, inter_bo->size);
		inter_bo->map = NULL;
	}
	assert((comm->pvp_stage & 0xff) == 0xff);
}
#endif

void nvc0_decoder_vp_caps(struct nvc0_decoder *dec, union pipe_desc desc,
                          struct nvc0_video_buffer *target, unsigned comm_seq,
                          unsigned *caps, unsigned *is_ref,
                          struct nvc0_video_buffer *refs[16])
{
   struct nouveau_bo *bsp_bo = dec->bsp_bo[comm_seq % NVC0_VIDEO_QDEPTH];
   enum pipe_video_codec codec = u_reduce_video_profile(dec->base.profile);
   char *vp = bsp_bo->map + VP_OFFSET;

   switch (codec){
   case PIPE_VIDEO_CODEC_MPEG12:
      *caps = nvc0_decoder_fill_picparm_mpeg12_vp(dec, desc.mpeg12, refs, is_ref, vp);
      nvc0_decoder_handle_references(dec, refs, dec->fence_seq, target);
      return;
   case PIPE_VIDEO_CODEC_MPEG4:
      *caps = nvc0_decoder_fill_picparm_mpeg4_vp(dec, desc.mpeg4, refs, is_ref, vp);
      nvc0_decoder_handle_references(dec, refs, dec->fence_seq, target);
      return;
   case PIPE_VIDEO_CODEC_VC1: {
      *caps = nvc0_decoder_fill_picparm_vc1_vp(dec, desc.vc1, refs, is_ref, vp);
      nvc0_decoder_handle_references(dec, refs, dec->fence_seq, target);
      return;
   }
   case PIPE_VIDEO_CODEC_MPEG4_AVC: {
      *caps = nvc0_decoder_fill_picparm_h264_vp(dec, desc.h264, refs, is_ref, vp);
      nvc0_decoder_handle_references(dec, refs, dec->fence_seq, target);
      nvc0_decoder_fill_picparm_h264_vp_refs(dec, desc.h264, refs, target, vp);
      return;
   }
   default: assert(0); return;
   }
}

void
nvc0_decoder_vp(struct nvc0_decoder *dec, union pipe_desc desc,
                struct nvc0_video_buffer *target, unsigned comm_seq,
                unsigned caps, unsigned is_ref,
                struct nvc0_video_buffer *refs[16])
{
   struct nouveau_pushbuf *push = dec->pushbuf[1];
   uint32_t bsp_addr, comm_addr, inter_addr, ucode_addr, pic_addr[17], last_addr, null_addr;
   uint32_t slice_size, bucket_size, ring_size, i;
   enum pipe_video_codec codec = u_reduce_video_profile(dec->base.profile);
   struct nouveau_bo *bsp_bo = dec->bsp_bo[comm_seq % NVC0_VIDEO_QDEPTH];
   struct nouveau_bo *inter_bo = dec->inter_bo[comm_seq & 1];
   u32 fence_extra = 0, codec_extra = 0;
   struct nouveau_pushbuf_refn bo_refs[] = {
      { inter_bo, NOUVEAU_BO_WR | NOUVEAU_BO_VRAM },
      { dec->ref_bo, NOUVEAU_BO_WR | NOUVEAU_BO_VRAM },
      { bsp_bo, NOUVEAU_BO_RD | NOUVEAU_BO_VRAM },
#ifdef NVC0_DEBUG_FENCE
      { dec->fence_bo, NOUVEAU_BO_WR | NOUVEAU_BO_GART },
#endif
      { dec->fw_bo, NOUVEAU_BO_RD | NOUVEAU_BO_VRAM },
   };
   int num_refs = sizeof(bo_refs)/sizeof(*bo_refs) - !dec->fw_bo;

#if NVC0_DEBUG_FENCE
   fence_extra = 4;
#endif

   if (codec == PIPE_VIDEO_CODEC_MPEG4_AVC) {
      nvc0_decoder_inter_sizes(dec, desc.h264->slice_count, &slice_size, &bucket_size, &ring_size);
      codec_extra += 2;
   } else
      nvc0_decoder_inter_sizes(dec, 1, &slice_size, &bucket_size, &ring_size);

   if (dec->base.max_references > 2)
      codec_extra += 1 + (dec->base.max_references - 2);

   pic_addr[16] = nvc0_video_addr(dec, target) >> 8;
   last_addr = null_addr = nvc0_video_addr(dec, NULL) >> 8;

   for (i = 0; i < dec->base.max_references; ++i) {
      if (!refs[i])
         pic_addr[i] = last_addr;
      else if (dec->refs[refs[i]->valid_ref].vidbuf == refs[i])
         last_addr = pic_addr[i] = nvc0_video_addr(dec, refs[i]) >> 8;
      else
         pic_addr[i] = null_addr;
   }
   if (!is_ref)
      nvc0_decoder_kick_ref(dec, target);

   PUSH_SPACE(push, 8 + 3 * (codec != PIPE_VIDEO_CODEC_MPEG12) +
              6 + codec_extra + fence_extra + 2);

   nouveau_pushbuf_refn(push, bo_refs, num_refs);

   bsp_addr = bsp_bo->offset >> 8;
#if NVC0_DEBUG_FENCE
   comm_addr = (dec->fence_bo->offset + COMM_OFFSET)>>8;
#else
   comm_addr = bsp_addr + (COMM_OFFSET>>8);
#endif
   inter_addr = inter_bo->offset >> 8;
   if (dec->fw_bo)
      ucode_addr = dec->fw_bo->offset >> 8;
   else
      ucode_addr = 0;

   BEGIN_NVC0(push, SUBC_VP(0x700), 7);
   PUSH_DATA (push, caps); // 700
   PUSH_DATA (push, comm_seq); // 704
   PUSH_DATA (push, 0); // 708 fuc targets, ignored for nvc0
   PUSH_DATA (push, dec->fw_sizes); // 70c
   PUSH_DATA (push, bsp_addr+(VP_OFFSET>>8)); // 710 picparm_addr
   PUSH_DATA (push, inter_addr); // 714 inter_parm
   PUSH_DATA (push, inter_addr + slice_size + bucket_size); // 718 inter_data_ofs

   if (bucket_size) {
      uint64_t tmpimg_addr = dec->ref_bo->offset + dec->ref_stride * (dec->base.max_references+2);

      BEGIN_NVC0(push, SUBC_VP(0x71c), 2);
      PUSH_DATA (push, tmpimg_addr >> 8); // 71c
      PUSH_DATA (push, inter_addr + slice_size); // 720 bucket_ofs
   }

   BEGIN_NVC0(push, SUBC_VP(0x724), 5);
   PUSH_DATA (push, comm_addr); // 724
   PUSH_DATA (push, ucode_addr); // 728
   PUSH_DATA (push, pic_addr[16]); // 734
   PUSH_DATA (push, pic_addr[0]); // 72c
   PUSH_DATA (push, pic_addr[1]); // 730

   if (dec->base.max_references > 2) {
      int i;

      BEGIN_NVC0(push, SUBC_VP(0x400), dec->base.max_references - 2);
      for (i = 2; i < dec->base.max_references; ++i) {
         assert(0x400 + (i - 2) * 4 < 0x438);
         PUSH_DATA (push, pic_addr[i]);
      }
   }

   if (codec == PIPE_VIDEO_CODEC_MPEG4_AVC) {
      BEGIN_NVC0(push, SUBC_VP(0x438), 1);
      PUSH_DATA (push, desc.h264->slice_count);
   }

   //debug_printf("Decoding %08lx with %08lx and %08lx\n", pic_addr[16], pic_addr[0], pic_addr[1]);

#if NVC0_DEBUG_FENCE
   BEGIN_NVC0(push, SUBC_VP(0x240), 3);
   PUSH_DATAh(push, (dec->fence_bo->offset + 0x10));
   PUSH_DATA (push, (dec->fence_bo->offset + 0x10));
   PUSH_DATA (push, dec->fence_seq);

   BEGIN_NVC0(push, SUBC_VP(0x300), 1);
   PUSH_DATA (push, 1);
   PUSH_KICK(push);

   {
      unsigned spin = 0;
      do {
         usleep(100);
         if ((spin++ & 0xff) == 0xff) {
            debug_printf("vp%u: %u\n", dec->fence_seq, dec->fence_map[4]);
            dump_comm_vp(dec, dec->comm, comm_seq, inter_bo, slice_size << 8);
         }
      } while (dec->fence_seq > dec->fence_map[4]);
   }
   dump_comm_vp(dec, dec->comm, comm_seq, inter_bo, slice_size << 8);
#else
   BEGIN_NVC0(push, SUBC_VP(0x300), 1);
   PUSH_DATA (push, 0);
   PUSH_KICK (push);
#endif
}
