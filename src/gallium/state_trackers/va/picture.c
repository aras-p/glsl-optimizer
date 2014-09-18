/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
 * Copyright 2014 Advanced Micro Devices, Inc.
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
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_video_codec.h"

#include "util/u_handle_table.h"
#include "util/u_video.h"

#include "vl/vl_vlc.h"

#include "va_private.h"

VAStatus
vlVaBeginPicture(VADriverContextP ctx, VAContextID context_id, VASurfaceID render_target)
{
   vlVaDriver *drv;
   vlVaContext *context;
   vlVaSurface *surf;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   drv = VL_VA_DRIVER(ctx);
   if (!drv)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   context = handle_table_get(drv->htab, context_id);
   if (!context)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   surf = handle_table_get(drv->htab, render_target);
   if (!surf || !surf->buffer)
      return VA_STATUS_ERROR_INVALID_SURFACE;

   context->target = surf->buffer;
   context->decoder->begin_frame(context->decoder, context->target, NULL);

   return VA_STATUS_SUCCESS;
}

static void
getReferenceFrame(vlVaDriver *drv, VASurfaceID surface_id,
                  struct pipe_video_buffer **ref_frame)
{
   vlVaSurface *surf = handle_table_get(drv->htab, surface_id);
   if (surf)
      *ref_frame = surf->buffer;
   else
      *ref_frame = NULL;
}

static void
handlePictureParameterBuffer(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
{
   VAPictureParameterBufferMPEG2 *mpeg2;
   VAPictureParameterBufferH264 *h264;
   VAPictureParameterBufferVC1 * vc1;

   switch (u_reduce_video_profile(context->decoder->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12:
      assert(buf->size >= sizeof(VAPictureParameterBufferMPEG2) && buf->num_elements == 1);
      mpeg2 = buf->data;
      /*horizontal_size;*/
      /*vertical_size;*/
      getReferenceFrame(drv, mpeg2->forward_reference_picture, &context->desc.mpeg12.ref[0]);
      getReferenceFrame(drv, mpeg2->backward_reference_picture, &context->desc.mpeg12.ref[1]);
      context->desc.mpeg12.picture_coding_type = mpeg2->picture_coding_type;
      context->desc.mpeg12.f_code[0][0] = ((mpeg2->f_code >> 12) & 0xf) - 1;
      context->desc.mpeg12.f_code[0][1] = ((mpeg2->f_code >> 8) & 0xf) - 1;
      context->desc.mpeg12.f_code[1][0] = ((mpeg2->f_code >> 4) & 0xf) - 1;
      context->desc.mpeg12.f_code[1][1] = (mpeg2->f_code & 0xf) - 1;
      context->desc.mpeg12.intra_dc_precision =
         mpeg2->picture_coding_extension.bits.intra_dc_precision;
      context->desc.mpeg12.picture_structure =
         mpeg2->picture_coding_extension.bits.picture_structure;
      context->desc.mpeg12.top_field_first =
         mpeg2->picture_coding_extension.bits.top_field_first;
      context->desc.mpeg12.frame_pred_frame_dct =
         mpeg2->picture_coding_extension.bits.frame_pred_frame_dct;
      context->desc.mpeg12.concealment_motion_vectors =
         mpeg2->picture_coding_extension.bits.concealment_motion_vectors;
      context->desc.mpeg12.q_scale_type =
         mpeg2->picture_coding_extension.bits.q_scale_type;
      context->desc.mpeg12.intra_vlc_format =
         mpeg2->picture_coding_extension.bits.intra_vlc_format;
      context->desc.mpeg12.alternate_scan =
         mpeg2->picture_coding_extension.bits.alternate_scan;
      /*repeat_first_field*/
      /*progressive_frame*/
      /*is_first_field*/
      break;

   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      assert(buf->size >= sizeof(VAPictureParameterBufferH264) && buf->num_elements == 1);
      h264 = buf->data;
      /*CurrPic*/
      context->desc.h264.field_order_cnt[0] = h264->CurrPic.TopFieldOrderCnt;
      context->desc.h264.field_order_cnt[1] = h264->CurrPic.BottomFieldOrderCnt;
      /*ReferenceFrames[16]*/
      /*picture_width_in_mbs_minus1*/
      /*picture_height_in_mbs_minus1*/
      /*bit_depth_luma_minus8*/
      /*bit_depth_chroma_minus8*/
      context->desc.h264.num_ref_frames = h264->num_ref_frames;
      /*chroma_format_idc*/
      /*residual_colour_transform_flag*/
      /*gaps_in_frame_num_value_allowed_flag*/
      context->desc.h264.pps->sps->frame_mbs_only_flag =
         h264->seq_fields.bits.frame_mbs_only_flag;
      context->desc.h264.pps->sps->mb_adaptive_frame_field_flag =
         h264->seq_fields.bits.mb_adaptive_frame_field_flag;
      context->desc.h264.pps->sps->direct_8x8_inference_flag =
         h264->seq_fields.bits.direct_8x8_inference_flag;
      /*MinLumaBiPredSize8x8*/
      context->desc.h264.pps->sps->log2_max_frame_num_minus4 =
         h264->seq_fields.bits.log2_max_frame_num_minus4;
      context->desc.h264.pps->sps->pic_order_cnt_type =
         h264->seq_fields.bits.pic_order_cnt_type;
      context->desc.h264.pps->sps->log2_max_pic_order_cnt_lsb_minus4 =
         h264->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4;
      context->desc.h264.pps->sps->delta_pic_order_always_zero_flag =
         h264->seq_fields.bits.delta_pic_order_always_zero_flag;
      /*num_slice_groups_minus1*/
      /*slice_group_map_type*/
      /*slice_group_change_rate_minus1*/
      context->desc.h264.pps->pic_init_qp_minus26 =
         h264->pic_init_qp_minus26;
      /*pic_init_qs_minus26*/
      context->desc.h264.pps->chroma_qp_index_offset =
         h264->chroma_qp_index_offset;
      context->desc.h264.pps->second_chroma_qp_index_offset =
         h264->second_chroma_qp_index_offset;
      context->desc.h264.pps->entropy_coding_mode_flag =
         h264->pic_fields.bits.entropy_coding_mode_flag;
      context->desc.h264.pps->weighted_pred_flag =
         h264->pic_fields.bits.weighted_pred_flag;
      context->desc.h264.pps->weighted_bipred_idc =
         h264->pic_fields.bits.weighted_bipred_idc;
      context->desc.h264.pps->transform_8x8_mode_flag =
         h264->pic_fields.bits.transform_8x8_mode_flag;
      context->desc.h264.field_pic_flag =
         h264->pic_fields.bits.field_pic_flag;
      context->desc.h264.pps->constrained_intra_pred_flag =
         h264->pic_fields.bits.constrained_intra_pred_flag;
      context->desc.h264.pps->bottom_field_pic_order_in_frame_present_flag =
         h264->pic_fields.bits.pic_order_present_flag;
      context->desc.h264.pps->deblocking_filter_control_present_flag =
         h264->pic_fields.bits.deblocking_filter_control_present_flag;
      context->desc.h264.pps->redundant_pic_cnt_present_flag =
         h264->pic_fields.bits.redundant_pic_cnt_present_flag;
      /*reference_pic_flag*/
      context->desc.h264.frame_num = h264->frame_num;
      break;

   case PIPE_VIDEO_FORMAT_VC1:
      assert(buf->size >= sizeof(VAPictureParameterBufferVC1) && buf->num_elements == 1);
      vc1 = buf->data;
      getReferenceFrame(drv, vc1->forward_reference_picture, &context->desc.vc1.ref[0]);
      getReferenceFrame(drv, vc1->backward_reference_picture, &context->desc.vc1.ref[1]);
      context->desc.vc1.picture_type = vc1->picture_fields.bits.picture_type;
      context->desc.vc1.frame_coding_mode = vc1->picture_fields.bits.frame_coding_mode;
      context->desc.vc1.postprocflag = vc1->post_processing != 0;
      context->desc.vc1.pulldown = vc1->sequence_fields.bits.pulldown;
      context->desc.vc1.interlace = vc1->sequence_fields.bits.interlace;
      context->desc.vc1.tfcntrflag = vc1->sequence_fields.bits.tfcntrflag;
      context->desc.vc1.finterpflag = vc1->sequence_fields.bits.finterpflag;
      context->desc.vc1.psf = vc1->sequence_fields.bits.psf;
      context->desc.vc1.dquant = vc1->pic_quantizer_fields.bits.dquant;
      context->desc.vc1.panscan_flag = vc1->entrypoint_fields.bits.panscan_flag;
      context->desc.vc1.refdist_flag =
         vc1->reference_fields.bits.reference_distance_flag;
      context->desc.vc1.quantizer = vc1->pic_quantizer_fields.bits.quantizer;
      context->desc.vc1.extended_mv = vc1->mv_fields.bits.extended_mv_flag;
      context->desc.vc1.extended_dmv = vc1->mv_fields.bits.extended_dmv_flag;
      context->desc.vc1.overlap = vc1->sequence_fields.bits.overlap;
      context->desc.vc1.vstransform =
         vc1->transform_fields.bits.variable_sized_transform_flag;
      context->desc.vc1.loopfilter = vc1->entrypoint_fields.bits.loopfilter;
      context->desc.vc1.fastuvmc = vc1->fast_uvmc_flag;
      context->desc.vc1.range_mapy_flag = vc1->range_mapping_fields.bits.luma_flag;
      context->desc.vc1.range_mapy = vc1->range_mapping_fields.bits.luma;
      context->desc.vc1.range_mapuv_flag = vc1->range_mapping_fields.bits.chroma_flag;
      context->desc.vc1.range_mapuv = vc1->range_mapping_fields.bits.chroma;
      context->desc.vc1.multires = vc1->sequence_fields.bits.multires;
      context->desc.vc1.syncmarker = vc1->sequence_fields.bits.syncmarker;
      context->desc.vc1.rangered = vc1->sequence_fields.bits.rangered;
      context->desc.vc1.maxbframes = vc1->sequence_fields.bits.max_b_frames;
      context->desc.vc1.deblockEnable = vc1->post_processing != 0;
      context->desc.vc1.pquant = vc1->pic_quantizer_fields.bits.pic_quantizer_scale;
      break;

   default:
      break;
   }
}

static void
handleIQMatrixBuffer(vlVaContext *context, vlVaBuffer *buf)
{
   VAIQMatrixBufferMPEG2 *mpeg2;
   VAIQMatrixBufferH264 *h264;

   switch (u_reduce_video_profile(context->decoder->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12:
      assert(buf->size >= sizeof(VAIQMatrixBufferMPEG2) && buf->num_elements == 1);
      mpeg2 = buf->data;
      if (mpeg2->load_intra_quantiser_matrix)
         context->desc.mpeg12.intra_matrix = mpeg2->intra_quantiser_matrix;
      else
         context->desc.mpeg12.intra_matrix = NULL;

      if (mpeg2->load_non_intra_quantiser_matrix)
         context->desc.mpeg12.non_intra_matrix = mpeg2->non_intra_quantiser_matrix;
      else
         context->desc.mpeg12.non_intra_matrix = NULL;
      break;

   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      assert(buf->size >= sizeof(VAIQMatrixBufferH264) && buf->num_elements == 1);
      h264 = buf->data;
      memcpy(&context->desc.h264.pps->ScalingList4x4, h264->ScalingList4x4, 6 * 16);
      memcpy(&context->desc.h264.pps->ScalingList8x8, h264->ScalingList8x8, 2 * 64);
      break;

   default:
      break;
   }
}

static void
handleSliceParameterBuffer(vlVaContext *context, vlVaBuffer *buf)
{
   VASliceParameterBufferH264 *h264;

   switch (u_reduce_video_profile(context->decoder->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      assert(buf->size >= sizeof(VASliceParameterBufferH264) && buf->num_elements == 1);
      h264 = buf->data;
      context->desc.h264.num_ref_idx_l0_active_minus1 =
         h264->num_ref_idx_l0_active_minus1;
      context->desc.h264.num_ref_idx_l1_active_minus1 =
         h264->num_ref_idx_l1_active_minus1;
      break;

   default:
      break;
   }
}

static void
handleVASliceDataBufferType(vlVaContext *context, vlVaBuffer *buf)
{
   unsigned num_buffers = 0;
   void * const *buffers[2];
   unsigned sizes[2];
   enum pipe_video_format format;

   format = u_reduce_video_profile(context->decoder->profile);
   if (format == PIPE_VIDEO_FORMAT_MPEG4_AVC ||
       format == PIPE_VIDEO_FORMAT_VC1) {
      struct vl_vlc vlc = {0};
      bool found = false;
      int peek_bits, i;

      /* search the first 64 bytes for a startcode */
      vl_vlc_init(&vlc, 1, (const void * const*)&buf->data, &buf->size);
      peek_bits = (format == PIPE_VIDEO_FORMAT_MPEG4_AVC) ? 24 : 32;
      for (i = 0; i < 64 && vl_vlc_bits_left(&vlc) >= peek_bits; ++i) {
         uint32_t value = vl_vlc_peekbits(&vlc, peek_bits);
         if ((format == PIPE_VIDEO_FORMAT_MPEG4_AVC && value == 0x000001) ||
            (format == PIPE_VIDEO_FORMAT_VC1 && (value == 0x0000010d ||
            value == 0x0000010c || value == 0x0000010b))) {
            found = true;
            break;
         }
         vl_vlc_eatbits(&vlc, 8);
         vl_vlc_fillbits(&vlc);
      }
      /* none found, ok add one manually */
      if (!found) {
         static const uint8_t start_code_h264[] = { 0x00, 0x00, 0x01 };
         static const uint8_t start_code_vc1[] = { 0x00, 0x00, 0x01, 0x0d };

         if (format == PIPE_VIDEO_FORMAT_MPEG4_AVC) {
            buffers[num_buffers] = (void *const)&start_code_h264;
            sizes[num_buffers] = sizeof(start_code_h264);
         }
         else {
            buffers[num_buffers] = (void *const)&start_code_vc1;
            sizes[num_buffers] = sizeof(start_code_vc1);
         }
         ++num_buffers;
      }
   }
   buffers[num_buffers] = buf->data;
   sizes[num_buffers] = buf->size;
   ++num_buffers;
   context->decoder->decode_bitstream(context->decoder, context->target, NULL,
      num_buffers, (const void * const*)buffers, sizes);
}

VAStatus
vlVaRenderPicture(VADriverContextP ctx, VAContextID context_id, VABufferID *buffers, int num_buffers)
{
   vlVaDriver *drv;
   vlVaContext *context;

   unsigned i;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   drv = VL_VA_DRIVER(ctx);
   if (!drv)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   context = handle_table_get(drv->htab, context_id);
   if (!context)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   for (i = 0; i < num_buffers; ++i) {
      vlVaBuffer *buf = handle_table_get(drv->htab, buffers[i]);
      if (!buf)
         return VA_STATUS_ERROR_INVALID_BUFFER;

      switch (buf->type) {
      case VAPictureParameterBufferType:
         handlePictureParameterBuffer(drv, context, buf);
         break;

      case VAIQMatrixBufferType:
         handleIQMatrixBuffer(context, buf);
         break;

      case VASliceParameterBufferType:
         handleSliceParameterBuffer(context, buf);
         break;

      case VASliceDataBufferType:
         handleVASliceDataBufferType(context, buf);
         break;

      default:
         break;
      }
   }

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaEndPicture(VADriverContextP ctx, VAContextID context_id)
{
   vlVaDriver *drv;
   vlVaContext *context;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   drv = VL_VA_DRIVER(ctx);
   if (!drv)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   context = handle_table_get(drv->htab, context_id);
   if (!context)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   context->decoder->end_frame(context->decoder, context->target, &context->desc.base);

   return VA_STATUS_SUCCESS;
}
