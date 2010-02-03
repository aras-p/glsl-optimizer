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

/* u_reduce_video_profile() needs these */
#include <pipe/p_compiler.h>

#include <pipe/p_defines.h>
#include <pipe/p_format.h>
#include <pipe/p_state.h>
#include <pipe/p_screen.h>
#include <util/u_inlines.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pipe_video_surface
{
   struct pipe_reference reference;
   struct pipe_screen *screen;
   enum pipe_video_chroma_format chroma_format;
   /*enum pipe_video_surface_format surface_format;*/
   unsigned width;
   unsigned height;
};

static INLINE void
pipe_video_surface_reference(struct pipe_video_surface **ptr, struct pipe_video_surface *surf)
{
   struct pipe_video_surface *old_surf = *ptr;

   if (pipe_reference(&(*ptr)->reference, &surf->reference))
      old_surf->screen->video_surface_destroy(old_surf);
   *ptr = surf;
}

struct pipe_video_rect
{
   unsigned x, y, w, h;
};

static INLINE enum pipe_video_codec
u_reduce_video_profile(enum pipe_video_profile profile)
{
   switch (profile)
   {
      case PIPE_VIDEO_PROFILE_MPEG1:
      case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
      case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
         return PIPE_VIDEO_CODEC_MPEG12;

      case PIPE_VIDEO_PROFILE_MPEG4_SIMPLE:
      case PIPE_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE:
         return PIPE_VIDEO_CODEC_MPEG4;

      case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
      case PIPE_VIDEO_PROFILE_VC1_MAIN:
      case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
         return PIPE_VIDEO_CODEC_VC1;

      case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
         return PIPE_VIDEO_CODEC_MPEG4_AVC;

      default:
         assert(0);
         return PIPE_VIDEO_CODEC_UNKNOWN;
   }
}

enum pipe_mpeg12_picture_type
{
   PIPE_MPEG12_PICTURE_TYPE_FIELD_TOP,
   PIPE_MPEG12_PICTURE_TYPE_FIELD_BOTTOM,
   PIPE_MPEG12_PICTURE_TYPE_FRAME
};

enum pipe_mpeg12_macroblock_type
{
   PIPE_MPEG12_MACROBLOCK_TYPE_INTRA,
   PIPE_MPEG12_MACROBLOCK_TYPE_FWD,
   PIPE_MPEG12_MACROBLOCK_TYPE_BKWD,
   PIPE_MPEG12_MACROBLOCK_TYPE_BI,
	
   PIPE_MPEG12_MACROBLOCK_NUM_TYPES
};

enum pipe_mpeg12_motion_type
{
   PIPE_MPEG12_MOTION_TYPE_FIELD,
   PIPE_MPEG12_MOTION_TYPE_FRAME,
   PIPE_MPEG12_MOTION_TYPE_DUALPRIME,
   PIPE_MPEG12_MOTION_TYPE_16x8
};

enum pipe_mpeg12_dct_type
{
   PIPE_MPEG12_DCT_TYPE_FIELD,
   PIPE_MPEG12_DCT_TYPE_FRAME
};

struct pipe_macroblock
{
   enum pipe_video_codec codec;
};

struct pipe_mpeg12_macroblock
{
   struct pipe_macroblock base;

   unsigned mbx;
   unsigned mby;
   enum pipe_mpeg12_macroblock_type mb_type;
   enum pipe_mpeg12_motion_type mo_type;
   enum pipe_mpeg12_dct_type dct_type;
   signed pmv[2][2][2];
   unsigned cbp;
   void *blocks;
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
