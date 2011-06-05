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

enum pipe_mpeg12_dct_intra
{
   PIPE_MPEG12_DCT_DELTA = 0,
   PIPE_MPEG12_DCT_INTRA = 1
};

enum pipe_mpeg12_dct_type
{
   PIPE_MPEG12_DCT_TYPE_FRAME = 0,
   PIPE_MPEG12_DCT_TYPE_FIELD = 1
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

/* bitfields because this is used as a vertex buffer element */
struct pipe_motionvector
{
   struct {
      signed x:16, y:16;
      enum pipe_video_field_select field_select:16;
      enum pipe_video_mv_weight weight:16;
   } top, bottom;
};

/* bitfields because this is used as a vertex buffer element */
struct pipe_ycbcr_block
{
   unsigned x:8, y:8;
   enum pipe_mpeg12_dct_intra intra:8;
   enum pipe_mpeg12_dct_type coding:8;
};

struct pipe_picture_desc
{
   enum pipe_video_profile profile;
};

struct pipe_mpeg12_picture_desc
{
   struct pipe_picture_desc base;

   unsigned picture_coding_type;
   unsigned picture_structure;
   unsigned frame_pred_frame_dct;
   unsigned q_scale_type;
   unsigned alternate_scan;
   unsigned intra_vlc_format;
   unsigned concealment_motion_vectors;
   unsigned f_code[2][2];
};

#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_STATE_H */
