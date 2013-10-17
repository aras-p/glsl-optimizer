/**************************************************************************
 *
 * Copyright 2011 Advanced Micro Devices, Inc.
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

#ifndef RADEON_UVD_H
#define RADEON_UVD_H

#include "../../winsys/radeon/drm/radeon_winsys.h"
#include "vl/vl_video_buffer.h"

/* UVD uses PM4 packet type 0 and 2 */
#define RUVD_PKT_TYPE_S(x)		(((x) & 0x3) << 30)
#define RUVD_PKT_TYPE_G(x)		(((x) >> 30) & 0x3)
#define RUVD_PKT_TYPE_C			0x3FFFFFFF
#define RUVD_PKT_COUNT_S(x)		(((x) & 0x3FFF) << 16)
#define RUVD_PKT_COUNT_G(x)		(((x) >> 16) & 0x3FFF)
#define RUVD_PKT_COUNT_C		0xC000FFFF
#define RUVD_PKT0_BASE_INDEX_S(x)	(((x) & 0xFFFF) << 0)
#define RUVD_PKT0_BASE_INDEX_G(x)	(((x) >> 0) & 0xFFFF)
#define RUVD_PKT0_BASE_INDEX_C		0xFFFF0000
#define RUVD_PKT0(index, count)		(RUVD_PKT_TYPE_S(0) | RUVD_PKT0_BASE_INDEX_S(index) | RUVD_PKT_COUNT_S(count))
#define RUVD_PKT2()			(RUVD_PKT_TYPE_S(2))

/* registers involved with UVD */
#define RUVD_GPCOM_VCPU_CMD		0xEF0C
#define RUVD_GPCOM_VCPU_DATA0		0xEF10
#define RUVD_GPCOM_VCPU_DATA1		0xEF14
#define RUVD_ENGINE_CNTL		0xEF18

/* UVD commands to VCPU */
#define RUVD_CMD_MSG_BUFFER		0x00000000
#define RUVD_CMD_DPB_BUFFER		0x00000001
#define RUVD_CMD_DECODING_TARGET_BUFFER	0x00000002
#define RUVD_CMD_FEEDBACK_BUFFER	0x00000003
#define RUVD_CMD_BITSTREAM_BUFFER	0x00000100

/* UVD message types */
#define RUVD_MSG_CREATE		0
#define RUVD_MSG_DECODE		1
#define RUVD_MSG_DESTROY	2

/* UVD stream types */
#define RUVD_CODEC_H264		0x00000000
#define RUVD_CODEC_VC1		0x00000001
#define RUVD_CODEC_MPEG2	0x00000003
#define RUVD_CODEC_MPEG4	0x00000004

/* UVD decode target buffer tiling mode */
#define RUVD_TILE_LINEAR	0x00000000
#define RUVD_TILE_8X4		0x00000001
#define RUVD_TILE_8X8		0x00000002
#define RUVD_TILE_32AS8		0x00000003

/* UVD decode target buffer array mode */
#define RUVD_ARRAY_MODE_LINEAR				0x00000000
#define RUVD_ARRAY_MODE_MACRO_LINEAR_MICRO_TILED	0x00000001
#define RUVD_ARRAY_MODE_1D_THIN				0x00000002
#define RUVD_ARRAY_MODE_2D_THIN				0x00000004
#define RUVD_ARRAY_MODE_MACRO_TILED_MICRO_LINEAR	0x00000004
#define RUVD_ARRAY_MODE_MACRO_TILED_MICRO_TILED		0x00000005

/* UVD tile config */
#define RUVD_BANK_WIDTH(x)		((x) << 0)
#define RUVD_BANK_HEIGHT(x)		((x) << 3)
#define RUVD_MACRO_TILE_ASPECT_RATIO(x)	((x) << 6)
#define RUVD_NUM_BANKS(x)		((x) << 9)

/* H.264 profile definitions */
#define RUVD_H264_PROFILE_BASELINE	0x00000000
#define RUVD_H264_PROFILE_MAIN		0x00000001
#define RUVD_H264_PROFILE_HIGH		0x00000002
#define RUVD_H264_PROFILE_STEREO_HIGH	0x00000003
#define RUVD_H264_PROFILE_MVC		0x00000004

/* VC-1 profile definitions */
#define RUVD_VC1_PROFILE_SIMPLE		0x00000000
#define RUVD_VC1_PROFILE_MAIN		0x00000001
#define RUVD_VC1_PROFILE_ADVANCED	0x00000002

struct ruvd_mvc_element {
	uint16_t	viewOrderIndex;
	uint16_t	viewId;
	uint16_t	numOfAnchorRefsInL0;
	uint16_t	viewIdOfAnchorRefsInL0[15];
	uint16_t	numOfAnchorRefsInL1;
	uint16_t	viewIdOfAnchorRefsInL1[15];
	uint16_t	numOfNonAnchorRefsInL0;
	uint16_t	viewIdOfNonAnchorRefsInL0[15];
	uint16_t	numOfNonAnchorRefsInL1;
	uint16_t	viewIdOfNonAnchorRefsInL1[15];
};

struct ruvd_h264 {
	uint32_t	profile;
	uint32_t	level;

	uint32_t	sps_info_flags;
	uint32_t	pps_info_flags;
	uint8_t		chroma_format;
	uint8_t		bit_depth_luma_minus8;
	uint8_t		bit_depth_chroma_minus8;
	uint8_t		log2_max_frame_num_minus4;

	uint8_t		pic_order_cnt_type;
	uint8_t		log2_max_pic_order_cnt_lsb_minus4;
	uint8_t		num_ref_frames;
	uint8_t		reserved_8bit;

	int8_t		pic_init_qp_minus26;
	int8_t		pic_init_qs_minus26;
	int8_t		chroma_qp_index_offset;
	int8_t		second_chroma_qp_index_offset;

	uint8_t		num_slice_groups_minus1;
	uint8_t		slice_group_map_type;
	uint8_t		num_ref_idx_l0_active_minus1;
	uint8_t		num_ref_idx_l1_active_minus1;

	uint16_t	slice_group_change_rate_minus1;
	uint16_t	reserved_16bit_1;

	uint8_t		scaling_list_4x4[6][16];
	uint8_t		scaling_list_8x8[2][64];

	uint32_t	frame_num;
	uint32_t	frame_num_list[16];
	int32_t		curr_field_order_cnt_list[2];
	int32_t		field_order_cnt_list[16][2];

	uint32_t	decoded_pic_idx;

	uint32_t	curr_pic_ref_frame_num;

	uint8_t		ref_frame_list[16];

	uint32_t	reserved[122];

	struct {
		uint32_t			numViews;
		uint32_t			viewId0;
		struct ruvd_mvc_element	mvcElements[1];
	} mvc;
};

struct ruvd_vc1 {
	uint32_t	profile;
	uint32_t	level;
	uint32_t	sps_info_flags;
	uint32_t	pps_info_flags;
	uint32_t	pic_structure;
	uint32_t	chroma_format;
};

struct ruvd_mpeg2 {
	uint32_t	decoded_pic_idx;
	uint32_t	ref_pic_idx[2];

	uint8_t		load_intra_quantiser_matrix;
	uint8_t		load_nonintra_quantiser_matrix;
	uint8_t		reserved_quantiser_alignement[2];
	uint8_t		intra_quantiser_matrix[64];
	uint8_t		nonintra_quantiser_matrix[64];

	uint8_t		profile_and_level_indication;
	uint8_t		chroma_format;

	uint8_t		picture_coding_type;

	uint8_t		reserved_1;

	uint8_t		f_code[2][2];
	uint8_t		intra_dc_precision;
	uint8_t		pic_structure;
	uint8_t		top_field_first;
	uint8_t		frame_pred_frame_dct;
	uint8_t		concealment_motion_vectors;
	uint8_t		q_scale_type;
	uint8_t		intra_vlc_format;
	uint8_t		alternate_scan;
};

struct ruvd_mpeg4
{
	uint32_t	decoded_pic_idx;
	uint32_t	ref_pic_idx[2];

	uint32_t	variant_type;
	uint8_t		profile_and_level_indication;

	uint8_t		video_object_layer_verid;
	uint8_t		video_object_layer_shape;

	uint8_t		reserved_1;

	uint16_t	video_object_layer_width;
	uint16_t	video_object_layer_height;

	uint16_t	vop_time_increment_resolution;

	uint16_t	reserved_2;

	uint32_t	flags;

	uint8_t		quant_type;

	uint8_t		reserved_3[3];

	uint8_t		intra_quant_mat[64];
	uint8_t		nonintra_quant_mat[64];

	struct {
		uint8_t		sprite_enable;

		uint8_t		reserved_4[3];

		uint16_t	sprite_width;
		uint16_t	sprite_height;
		int16_t		sprite_left_coordinate;
		int16_t		sprite_top_coordinate;

		uint8_t		no_of_sprite_warping_points;
		uint8_t		sprite_warping_accuracy;
		uint8_t		sprite_brightness_change;
		uint8_t		low_latency_sprite_enable;
	} sprite_config;

	struct {
		uint32_t	flags;
		uint8_t		vol_mode;
		uint8_t		reserved_5[3];
	} divx_311_config;
};

/* message between driver and hardware */
struct ruvd_msg {

	uint32_t	size;
	uint32_t	msg_type;
	uint32_t	stream_handle;
	uint32_t	status_report_feedback_number;

	union {
		struct {
			uint32_t	stream_type;
			uint32_t	session_flags;
			uint32_t	asic_id;
			uint32_t	width_in_samples;
			uint32_t	height_in_samples;
			uint32_t	dpb_buffer;
			uint32_t	dpb_size;
			uint32_t	dpb_model;
			uint32_t	version_info;
		} create;

		struct {
			uint32_t	stream_type;
			uint32_t	decode_flags;
			uint32_t	width_in_samples;
			uint32_t	height_in_samples;

			uint32_t	dpb_buffer;
			uint32_t	dpb_size;
			uint32_t	dpb_model;
			uint32_t	dpb_reserved;

			uint32_t	db_offset_alignment;
			uint32_t	db_pitch;
			uint32_t	db_tiling_mode;
			uint32_t	db_array_mode;
			uint32_t	db_field_mode;
			uint32_t	db_surf_tile_config;
			uint32_t	db_aligned_height;
			uint32_t	db_reserved;

			uint32_t	use_addr_macro;

			uint32_t	bsd_buffer;
			uint32_t	bsd_size;

			uint32_t	pic_param_buffer;
			uint32_t	pic_param_size;
			uint32_t	mb_cntl_buffer;
			uint32_t	mb_cntl_size;

			uint32_t	dt_buffer;
			uint32_t	dt_pitch;
			uint32_t	dt_tiling_mode;
			uint32_t	dt_array_mode;
			uint32_t	dt_field_mode;
			uint32_t	dt_luma_top_offset;
			uint32_t	dt_luma_bottom_offset;
			uint32_t	dt_chroma_top_offset;
			uint32_t	dt_chroma_bottom_offset;
			uint32_t	dt_surf_tile_config;
			uint32_t	dt_reserved[3];

			uint32_t	reserved[16];

			union {
				struct ruvd_h264	h264;
				struct ruvd_vc1		vc1;
				struct ruvd_mpeg2	mpeg2;
				struct ruvd_mpeg4	mpeg4;

				uint32_t info[768];
			} codec;

			uint8_t		extension_support;
			uint8_t		reserved_8bit_1;
			uint8_t		reserved_8bit_2;
			uint8_t		reserved_8bit_3;
			uint32_t	extension_reserved[64];
		} decode;
	} body;
};

/* driver dependent callback */
typedef struct radeon_winsys_cs_handle* (*ruvd_set_dtb)
(struct ruvd_msg* msg, struct vl_video_buffer *vb);

/* create an UVD decode */
struct pipe_video_codec *ruvd_create_decoder(struct pipe_context *context,
					     const struct pipe_video_codec *templat,
					     ruvd_set_dtb set_dtb);

/* fill decoding target field from the luma and chroma surfaces */
void ruvd_set_dt_surfaces(struct ruvd_msg *msg, struct radeon_surface *luma,
			  struct radeon_surface *chroma);
#endif
