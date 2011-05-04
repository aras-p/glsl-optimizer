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

#include <pipe/p_video_context.h>

#include <util/u_memory.h>
#include <util/u_math.h>
#include <util/u_debug.h>

#include "vdpau_private.h"

VdpStatus
vlVdpDecoderCreate(VdpDevice device,
                   VdpDecoderProfile profile,
                   uint32_t width, uint32_t height,
                   uint32_t max_references,
                   VdpDecoder *decoder)
{
   enum pipe_video_profile p_profile;
   struct pipe_video_context *vpipe;
   vlVdpDevice *dev;
   vlVdpDecoder *vldecoder;
   VdpStatus ret;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Creating decoder\n");

   if (!decoder)
      return VDP_STATUS_INVALID_POINTER;

   if (!(width && height))
      return VDP_STATUS_INVALID_VALUE;

   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)
      return VDP_STATUS_INVALID_DECODER_PROFILE;

   dev = vlGetDataHTAB(device);
   if (!dev)
      return VDP_STATUS_INVALID_HANDLE;

   vpipe = dev->context->vpipe;

   vldecoder = CALLOC(1,sizeof(vlVdpDecoder));
   if (!vldecoder)
      return VDP_STATUS_RESOURCES;

   vldecoder->device = dev;

   // TODO: Define max_references. Used mainly for H264
   vldecoder->decoder = vpipe->create_decoder
   (
      vpipe, p_profile,
      PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
      PIPE_VIDEO_CHROMA_FORMAT_420,
      width, height
   );
   if (!vldecoder->decoder) {
      ret = VDP_STATUS_ERROR;
      goto error_decoder;
   }

   vldecoder->buffer = vldecoder->decoder->create_buffer(vldecoder->decoder);
   if (!vldecoder->buffer) {
      ret = VDP_STATUS_ERROR;
      goto error_buffer;
   }

   *decoder = vlAddDataHTAB(vldecoder);
   if (*decoder == 0) {
      ret = VDP_STATUS_ERROR;
      goto error_handle;
   }

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoder created succesfully\n");

   return VDP_STATUS_OK;

error_handle:
   vldecoder->buffer->destroy(vldecoder->buffer);

error_buffer:
   vldecoder->decoder->destroy(vldecoder->decoder);

error_decoder:
   FREE(vldecoder);
   return ret;
}

VdpStatus
vlVdpDecoderDestroy(VdpDecoder decoder)
{
   vlVdpDecoder *vldecoder;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Destroying decoder\n");

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   vldecoder->buffer->destroy(vldecoder->buffer);
   vldecoder->decoder->destroy(vldecoder->decoder);

   FREE(vldecoder);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpDecoderGetParameters(VdpDecoder decoder,
                          VdpDecoderProfile *profile,
                          uint32_t *width,
                          uint32_t *height)
{
   return VDP_STATUS_OK;
}

static VdpStatus
vlVdpDecoderRenderMpeg2(struct pipe_video_decoder *decoder,
                        struct pipe_video_decode_buffer *buffer,
                        struct pipe_video_buffer *target,
                        VdpPictureInfoMPEG1Or2 *picture_info,
                        uint32_t bitstream_buffer_count,
                        VdpBitstreamBuffer const *bitstream_buffers)
{
   struct pipe_mpeg12_picture_desc picture;
   struct pipe_video_buffer *ref_frames[2];
   unsigned num_ycbcr_blocks[3] = { 0, 0, 0 };
   unsigned i;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding MPEG2\n");

   /* if surfaces equals VDP_STATUS_INVALID_HANDLE, they are not used */
   if (picture_info->forward_reference ==  VDP_INVALID_HANDLE)
      ref_frames[0] = NULL;
   else {
      ref_frames[0] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->forward_reference))->video_buffer;
      if (!ref_frames[0])
         return VDP_STATUS_INVALID_HANDLE;
   }

   if (picture_info->backward_reference ==  VDP_INVALID_HANDLE)
      ref_frames[1] = NULL;
   else {
      ref_frames[1] = ((vlVdpSurface *)vlGetDataHTAB(picture_info->backward_reference))->video_buffer;
      if (!ref_frames[1])
         return VDP_STATUS_INVALID_HANDLE;
   }

   memset(&picture, 0, sizeof(picture));
   picture.picture_coding_type = picture_info->picture_coding_type;
   picture.picture_structure = picture_info->picture_structure;
   picture.frame_pred_frame_dct = picture_info->frame_pred_frame_dct;
   picture.q_scale_type = picture_info->q_scale_type;
   picture.alternate_scan = picture_info->alternate_scan;
   picture.intra_dc_precision = picture_info->intra_dc_precision;
   picture.intra_vlc_format = picture_info->intra_vlc_format;
   picture.concealment_motion_vectors = picture_info->concealment_motion_vectors;
   picture.f_code[0][0] = picture_info->f_code[0][0] - 1;
   picture.f_code[0][1] = picture_info->f_code[0][1] - 1;
   picture.f_code[1][0] = picture_info->f_code[1][0] - 1;
   picture.f_code[1][1] = picture_info->f_code[1][1] - 1;

   picture.intra_quantizer_matrix = picture_info->intra_quantizer_matrix;
   picture.non_intra_quantizer_matrix = picture_info->non_intra_quantizer_matrix;

   buffer->map(buffer);

   for (i = 0; i < bitstream_buffer_count; ++i)
      buffer->decode_bitstream(buffer, bitstream_buffers[i].bitstream_bytes,
                               bitstream_buffers[i].bitstream, &picture, num_ycbcr_blocks);

   buffer->unmap(buffer);

   decoder->flush_buffer(buffer, num_ycbcr_blocks, ref_frames, target);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpDecoderRender(VdpDecoder decoder,
                   VdpVideoSurface target,
                   VdpPictureInfo const *picture_info,
                   uint32_t bitstream_buffer_count,
                   VdpBitstreamBuffer const *bitstream_buffers)
{
   vlVdpDecoder *vldecoder;
   vlVdpSurface *vlsurf;

   VDPAU_MSG(VDPAU_TRACE, "[VDPAU] Decoding\n");

   if (!(picture_info && bitstream_buffers))
      return VDP_STATUS_INVALID_POINTER;

   vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
   if (!vldecoder)
      return VDP_STATUS_INVALID_HANDLE;

   vlsurf = (vlVdpSurface *)vlGetDataHTAB(target);
   if (!vlsurf)
      return VDP_STATUS_INVALID_HANDLE;

   if (vlsurf->device != vldecoder->device)
      return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

   if (vlsurf->video_buffer->chroma_format != vldecoder->decoder->chroma_format)
      // TODO: Recreate decoder with correct chroma
      return VDP_STATUS_INVALID_CHROMA_TYPE;

   // TODO: Right now only mpeg2 is supported.
   switch (vldecoder->decoder->profile)   {
   case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
   case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
      return vlVdpDecoderRenderMpeg2(vldecoder->decoder, vldecoder->buffer, vlsurf->video_buffer,
                                     (VdpPictureInfoMPEG1Or2 *)picture_info,
                                     bitstream_buffer_count,bitstream_buffers);
      break;

   default:
      return VDP_STATUS_INVALID_DECODER_PROFILE;
   }
}
