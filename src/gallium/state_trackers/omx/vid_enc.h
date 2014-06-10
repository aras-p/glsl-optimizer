/**************************************************************************
 *
 * Copyright 2013 Advanced Micro Devices, Inc.
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

/*
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 *
 */

#ifndef OMX_VID_ENC_H
#define OMX_VID_ENC_H

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>

#include <bellagio/st_static_component_loader.h>
#include <bellagio/omx_base_filter.h>

#include "util/u_double_list.h"

#include "vl/vl_defines.h"
#include "vl/vl_compositor.h"

#define OMX_VID_ENC_BASE_NAME "OMX.mesa.video_encoder"
#define OMX_VID_ENC_AVC_NAME "OMX.mesa.video_encoder.avc"
#define OMX_VID_ENC_AVC_ROLE "video_encoder.avc"

#define OMX_VID_ENC_BITRATE_MIN 64000
#define OMX_VID_ENC_BITRATE_MEDIAN 2000000
#define OMX_VID_ENC_BITRATE_MAX 240000000
#define OMX_VID_ENC_CONTROL_FRAME_RATE_DEN_DEFAULT 1001
#define OMX_VID_ENC_QUANT_I_FRAMES_DEFAULT 0x1c
#define OMX_VID_ENC_QUANT_P_FRAMES_DEFAULT 0x1c
#define OMX_VID_ENC_QUANT_B_FRAMES_DEFAULT 0x1c
#define OMX_VID_ENC_SCALING_WIDTH_DEFAULT 0xffffffff
#define OMX_VID_ENC_SCALING_HEIGHT_DEFAULT 0xffffffff
#define OMX_VID_ENC_IDR_PERIOD_DEFAULT 1000
#define OMX_VID_ENC_P_PERIOD_DEFAULT 3

#define OMX_VID_ENC_NUM_SCALING_BUFFERS 4

DERIVEDCLASS(vid_enc_PrivateType, omx_base_filter_PrivateType)
#define vid_enc_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
	struct vl_screen *screen; \
	struct pipe_context *s_pipe; \
	struct pipe_context *t_pipe; \
	struct pipe_video_codec *codec; \
	struct list_head free_tasks; \
	struct list_head used_tasks; \
	struct list_head b_frames; \
	OMX_U32 frame_rate; \
	OMX_U32 frame_num; \
	OMX_U32 pic_order_cnt; \
	OMX_U32 ref_idx_l0, ref_idx_l1; \
	OMX_BOOL restricted_b_frames; \
	OMX_VIDEO_PARAM_BITRATETYPE bitrate; \
	OMX_VIDEO_PARAM_QUANTIZATIONTYPE quant; \
	OMX_VIDEO_PARAM_PROFILELEVELTYPE profile_level; \
	OMX_CONFIG_INTRAREFRESHVOPTYPE force_pic_type; \
	struct vl_compositor compositor; \
	struct vl_compositor_state cstate; \
	struct pipe_video_buffer *scale_buffer[OMX_VID_ENC_NUM_SCALING_BUFFERS]; \
	OMX_CONFIG_SCALEFACTORTYPE scale; \
	OMX_U32 current_scale_buffer;
ENDCLASS(vid_enc_PrivateType)

OMX_ERRORTYPE vid_enc_LoaderComponent(stLoaderComponentType *comp);

#endif
