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

#ifndef PIPE_VIDEO_STATE_H
#define PIPE_VIDEO_STATE_H

#include <pipe/p_defines.h>
#include <pipe/p_format.h>
#include <pipe/p_state.h>
#include <pipe/p_screen.h>
#include <util/u_inlines.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pipe_video_rect
{
   unsigned x, y, w, h;
};

enum pipe_mpeg12_picture_type
{
   PIPE_MPEG12_PICTURE_TYPE_FIELD_TOP,
   PIPE_MPEG12_PICTURE_TYPE_FIELD_BOTTOM,
   PIPE_MPEG12_PICTURE_TYPE_FRAME
};

enum pipe_mpeg12_dct_type
{
   PIPE_MPEG12_DCT_TYPE_FIELD,
   PIPE_MPEG12_DCT_TYPE_FRAME
};

enum pipe_video_field_select
{
   PIPE_VIDEO_FRAME = 0,
   PIPE_VIDEO_TOP_FIELD = 1,
   PIPE_VIDEO_BOTTOM_FIELD = 3,

   /* TODO
   PIPE_VIDEO_DUALPRIME
   PIPE_VIDEO_16x8
   */
};

enum pipe_video_mv_weight
{
   PIPE_VIDEO_MV_WEIGHT_MIN = 0,
   PIPE_VIDEO_MV_WEIGHT_HALF = 128,
   PIPE_VIDEO_MV_WEIGHT_MAX = 256
};

struct pipe_macroblock
{
   enum pipe_video_codec codec;
};

/* bitfields because this is used as a vertex buffer element */
struct pipe_motionvector
{
   struct {
      signed x:16, y:16;
      enum pipe_video_field_select field_select:16;
      enum pipe_video_mv_weight weight:16;
   } top, bottom;
};

struct pipe_mpeg12_macroblock
{
   struct pipe_macroblock base;

   unsigned mbx;
   unsigned mby;
   bool dct_intra;
   enum pipe_mpeg12_dct_type dct_type;
   unsigned cbp;
   short *blocks;
};

#if 0
struct pipe_picture_desc
{
   enum pipe_video_format format;
};

struct pipe_mpeg12_picture_desc
{
   struct pipe_picture_desc base;

   /* TODO: Use bitfields where possible? */
   struct pipe_surface *forward_reference;
   struct pipe_surface *backward_reference;
   unsigned picture_coding_type;
   unsigned fcode;
   unsigned intra_dc_precision;
   unsigned picture_structure;
   unsigned top_field_first;
   unsigned frame_pred_frame_dct;
   unsigned concealment_motion_vectors;
   unsigned q_scale_type;
   unsigned intra_vlc_format;
   unsigned alternate_scan;
   unsigned full_pel_forward_vector;
   unsigned full_pel_backward_vector;
   struct pipe_buffer *intra_quantizer_matrix;
   struct pipe_buffer *non_intra_quantizer_matrix;
   struct pipe_buffer *chroma_intra_quantizer_matrix;
   struct pipe_buffer *chroma_non_intra_quantizer_matrix;
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_STATE_H */
