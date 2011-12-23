/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_debug.h"
#include "util/u_video.h"

#include "vdpau_private.h"

/**
 * Create a VdpDecoder.
 */
VdpStatus
vlVdpDecoderCreate(VdpDevice device,
                   VdpDecoderProfile profile,
                   uint32_t width, uint32_t height,
                   uint32_t max_references,
                   VdpDecoder *decoder)
{
   enum pipe_video_profile p_profile;
   struct pipe_context *pipe;
   struct pipe_screen *screen;
   vlVdpDevice *dev;
   vlVdpDecoder *vldecoder;
   VdpStatus ret;
   bool supported;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating decoder\n");

   if (!decoder)
      return VDP_STATUS_INVALID_POINTER;
   *decoder = 0;

   if (!(width && height))
      return VDP_STATUS_INVALID_VALUE;

   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)
      return VDP_STATUS_INVALID_DECODER_PROFILE;

   dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   pipe = dev->context->pipe;
   screen = dev->vscreen->pscreen;
   supported = screen->get_video_param
   (
      screen,
      p_profile,
      PIPE_VIDEO_CAP_SUPPORTED
   );
   if (!supported)
      return VDP_STATUS_INVALID_DECODER_PROFILE;

   vldecoder = CALLOC(1,sizeof(vlVdpDecoder));
   if (!vldecoder)
      return VDP_STATUS_RESOURCES;

   vldecoder->device = dev;

   vldecoder->decoder = pipe->create_video_decoder
   (
      pipe, p_profile,
      PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
      PIPE_VIDEO_CHROMA_FORMAT_420,
      width, height, max_references,
      false
   );

   if (!vldecoder->decoder) {
      ret = VDP_STATUS_ERROR;
      goto error_decoder;
   }

   *decoder = vlAddDataHTAB(vldecoder);
   if (*decoder == 0) {
      ret = VDP_STATUS_ERROR;
      goto error_handle;
   }

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoder created succesfully\n");

   return VDP_STATUS_OK;

error_handle:

   vldecoder->decoder->destroy(vldecoder->decoder);

error_decoder:
   FREE(vldecoder);
   return ret;
}

/**
 * Destroy a VdpDecoder.
 */
VdpStatus
vlVdpDecoderDestroy(VdpDecoder decoder)
{
   vlVdpDecoder *vldecoder;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying decoder\n");

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   vldecoder->decoder->destroy(vldecoder->decoder);

   FREE(vldecoder);

   return VDP_STATUS_OK;
}

/**
 * Retrieve the parameters used to create a VdpBitmapSurface.
 */
VdpStatus
vlVdpDecoderGetParameters(VdpDecoder decoder,
                          VdpDecoderProfile *profile,
                          uint32_t *width,
                          uint32_t *height)
{
   vlVdpDecoder *vldecoder;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoder get parameters called\n");

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   *profile = PipeToProfile(vldecoder->decoder->profile);
   *width = vldecoder->decoder->width;
   *height = vldecoder->decoder->height;

   return VDP_STATUS_OK;
}

/**
 * Decode a mpeg 1/2 video.
 */
static VdpStatus
vlVdpDecoderRenderMpeg12(struct pipe_video_decoder *decoder,
                         VdpPictureInfoMPEG1Or2 *picture_info)
{
   struct pipe_mpeg12_picture_desc picture;
   struct pipe_mpeg12_quant_matrix quant;
   struct pipe_video_buffer *ref_frames[2];
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding MPEG12\n");

   i = 0;

   /* if surfaces equals VDP_STATUS_INVALID_HANDLE, they are not used */
   if (picture_info->forward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[i] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->forward_reference))->video_buffer;
      if (!ref_frames[i])
         return VDP_STATUS_INVALID_HANDLE;
      ++i;
   }

   if (picture_info->backward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[i] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->backward_reference))->video_buffer;
      if (!ref_frames[i])
         return VDP_STATUS_INVALID_HANDLE;
      ++i;
   }

   decoder->set_reference_frames(decoder, ref_frames, i);

   memset(&picture, 0, sizeof(picture));
   picture.base.profile = decoder->profile;
   picture.picture_coding_type = picture_info->picture_coding_type;
   picture.picture_structure = picture_info->picture_structure;
   picture.frame_pred_frame_dct = picture_info->frame_pred_frame_dct;
   picture.q_scale_type = picture_info->q_scale_type;
   picture.alternate_scan = picture_info->alternate_scan;
   picture.intra_vlc_format = picture_info->intra_vlc_format;
   picture.concealment_motion_vectors = picture_info->concealment_motion_vectors;
   picture.intra_dc_precision = picture_info->intra_dc_precision;
   picture.f_code[0][0] = picture_info->f_code[0][0] - 1;
   picture.f_code[0][1] = picture_info->f_code[0][1] - 1;
   picture.f_code[1][0] = picture_info->f_code[1][0] - 1;
   picture.f_code[1][1] = picture_info->f_code[1][1] - 1;
   picture.num_slices = picture_info->slice_count;
   picture.top_field_first = picture_info->top_field_first;
   picture.full_pel_forward_vector = picture_info->full_pel_forward_vector;
   picture.full_pel_backward_vector = picture_info->full_pel_backward_vector;

   decoder->set_picture_parameters(decoder, &picture.base);

   memset(&quant, 0, sizeof(quant));
   quant.base.codec = PIPE_VIDEO_CODEC_MPEG12;
   quant.intra_matrix = picture_info->intra_quantizer_matrix;
   quant.non_intra_matrix = picture_info->non_intra_quantizer_matrix;

   decoder->set_quant_matrix(decoder, &quant.base);
   return VDP_STATUS_OK;
}

/**
 * Decode a mpeg 4 video.
 */
static VdpStatus
vlVdpDecoderRenderMpeg4(struct pipe_video_decoder *decoder,
                         VdpPictureInfoMPEG4Part2 *picture_info)
{
   struct pipe_mpeg4_picture_desc picture;
   struct pipe_mpeg4_quant_matrix quant;
   struct pipe_video_buffer *ref_frames[2] = {};
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding MPEG4\n");

   /* if surfaces equals VDP_STATUS_INVALID_HANDLE, they are not used */
   if (picture_info->forward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[0] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->forward_reference))->video_buffer;
      if (!ref_frames[0])
         return VDP_STATUS_INVALID_HANDLE;
   }

   if (picture_info->backward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[1] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->backward_reference))->video_buffer;
      if (!ref_frames[1])
         return VDP_STATUS_INVALID_HANDLE;
   }
   decoder->set_reference_frames(decoder, ref_frames, 2);

   memset(&picture, 0, sizeof(picture));
   picture.base.profile = decoder->profile;
   for (i = 0; i < 2; ++i) {
      picture.trd[i] = picture_info->trd[i];
      picture.trb[i] = picture_info->trb[i];
   }
   picture.vop_time_increment_resolution = picture_info->vop_time_increment_resolution;
   picture.vop_coding_type = picture_info->vop_coding_type;
   picture.vop_fcode_forward = picture_info->vop_fcode_forward;
   picture.vop_fcode_backward = picture_info->vop_fcode_backward;
   picture.resync_marker_disable = picture_info->resync_marker_disable;
   picture.interlaced = picture_info->interlaced;
   picture.quant_type = picture_info->quant_type;
   picture.quarter_sample = picture_info->quarter_sample;
   picture.short_video_header = picture_info->short_video_header;
   picture.rounding_control = picture_info->rounding_control;
   picture.alternate_vertical_scan_flag = picture_info->alternate_vertical_scan_flag;
   picture.top_field_first = picture_info->top_field_first;
   decoder->set_picture_parameters(decoder, &picture.base);

   memset(&quant, 0, sizeof(quant));
   quant.base.codec = PIPE_VIDEO_CODEC_MPEG4;
   quant.intra_matrix = picture_info->intra_quantizer_matrix;
   quant.non_intra_matrix = picture_info->non_intra_quantizer_matrix;
   decoder->set_quant_matrix(decoder, &quant.base);
   return VDP_STATUS_OK;
}

static VdpStatus
vlVdpDecoderRenderVC1(struct pipe_video_decoder *decoder,
                      VdpPictureInfoVC1 *picture_info)
{
   struct pipe_vc1_picture_desc picture;
   struct pipe_video_buffer *ref_frames[2] = {};

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding VC-1\n");

   /* if surfaces equals VDP_STATUS_INVALID_HANDLE, they are not used */
   if (picture_info->forward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[0] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->forward_reference))->video_buffer;
      if (!ref_frames[0])
         return VDP_STATUS_INVALID_HANDLE;
   }

   if (picture_info->backward_reference !=  VDP_INVALID_HANDLE) {
      ref_frames[1] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->backward_reference))->video_buffer;
      if (!ref_frames[1])
         return VDP_STATUS_INVALID_HANDLE;
   }
   decoder->set_reference_frames(decoder, ref_frames, 2);

   memset(&picture, 0, sizeof(picture));
   picture.base.profile = decoder->profile;
   picture.slice_count = picture_info->slice_count;
   picture.picture_type = picture_info->picture_type;
   picture.frame_coding_mode = picture_info->frame_coding_mode;
   picture.postprocflag = picture_info->postprocflag;
   picture.pulldown = picture_info->pulldown;
   picture.interlace = picture_info->interlace;
   picture.tfcntrflag = picture_info->tfcntrflag;
   picture.finterpflag = picture_info->finterpflag;
   picture.psf = picture_info->psf;
   picture.dquant = picture_info->dquant;
   picture.panscan_flag = picture_info->panscan_flag;
   picture.refdist_flag = picture_info->refdist_flag;
   picture.quantizer = picture_info->quantizer;
   picture.extended_mv = picture_info->extended_mv;
   picture.extended_dmv = picture_info->extended_dmv;
   picture.overlap = picture_info->overlap;
   picture.vstransform = picture_info->vstransform;
   picture.loopfilter = picture_info->loopfilter;
   picture.fastuvmc = picture_info->fastuvmc;
   picture.range_mapy_flag = picture_info->range_mapy_flag;
   picture.range_mapy = picture_info->range_mapy;
   picture.range_mapuv_flag = picture_info->range_mapuv_flag;
   picture.range_mapuv = picture_info->range_mapuv;
   picture.multires = picture_info->multires;
   picture.syncmarker = picture_info->syncmarker;
   picture.rangered = picture_info->rangered;
   picture.maxbframes = picture_info->maxbframes;
   picture.deblockEnable = picture_info->deblockEnable;
   picture.pquant = picture_info->pquant;
   decoder->set_picture_parameters(decoder, &picture.base);
   return VDP_STATUS_OK;
}

/**
 * Decode a compressed field/frame and render the result into a VdpVideoSurface.
 */
VdpStatus
vlVdpDecoderRender(VdpDecoder decoder,
                   VdpVideoSurface target,
                   VdpPictureInfo const *picture_info,
                   uint32_t bitstream_buffer_count,
                   VdpBitstreamBuffer const *bitstream_buffers)
{
   const void * buffers[bitstream_buffer_count];
   unsigned sizes[bitstream_buffer_count];
   vlVdpDecoder *vldecoder;
   vlVdpSurface *vlsurf;
   VdpStatus ret;
   struct pipe_video_decoder *dec;
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding\n");

   if (!(picture_info && bitstream_buffers))
      return VDP_STATUS_INVALID_POINTER;

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;
   dec = vldecoder->decoder;

   vlsurf = (vlVdpSurface *)vlGetDataHTAB(target);
   if (!vlsurf)
      return VDP_STATUS_INVALID_HANDLE;

   if (vlsurf->device != vldecoder->device)
      return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

   if (vlsurf->video_buffer->chroma_format != dec->chroma_format)
      // TODO: Recreate decoder with correct chroma
      return VDP_STATUS_INVALID_CHROMA_TYPE;

   dec->set_decode_target(dec, vlsurf->video_buffer);

   switch (u_reduce_video_profile(dec->profile)) {
   case PIPE_VIDEO_CODEC_MPEG12:
      ret = vlVdpDecoderRenderMpeg12(dec, (VdpPictureInfoMPEG1Or2 *)picture_info);
      break;
   case PIPE_VIDEO_CODEC_MPEG4:
      ret = vlVdpDecoderRenderMpeg4(dec, (VdpPictureInfoMPEG4Part2 *)picture_info);
      break;
   case PIPE_VIDEO_CODEC_VC1:
      ret = vlVdpDecoderRenderVC1(dec, (VdpPictureInfoVC1 *)picture_info);
      break;
   default:
      return VDP_STATUS_INVALID_DECODER_PROFILE;
   }
   if (ret != VDP_STATUS_OK)
      return ret;

   dec->begin_frame(dec);
   for (i = 0; i < bitstream_buffer_count; ++i) {
      buffers[i] = bitstream_buffers[i].bitstream;
      sizes[i] = bitstream_buffers[i].bitstream_bytes;
   }
   dec->decode_bitstream(dec, bitstream_buffer_count, buffers, sizes);
   dec->end_frame(dec);
   return ret;
}
