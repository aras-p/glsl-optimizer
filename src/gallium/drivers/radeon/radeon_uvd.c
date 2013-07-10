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
 *	Christian König <christian.koenig@amd.com>
 *
 */

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#include "pipe/p_video_decoder.h"

#include "util/u_memory.h"
#include "util/u_video.h"

#include "vl/vl_defines.h"
#include "vl/vl_mpeg12_decoder.h"

#include "../../winsys/radeon/drm/radeon_winsys.h"
#include "radeon_uvd.h"

#define RUVD_ERR(fmt, args...) \
	fprintf(stderr, "EE %s:%d %s UVD - "fmt, __FILE__, __LINE__, __func__, ##args)

#define NUM_BUFFERS 4

#define NUM_MPEG2_REFS 6
#define NUM_H264_REFS 17

/* UVD buffer representation */
struct ruvd_buffer
{
	struct pb_buffer*		buf;
	struct radeon_winsys_cs_handle*	cs_handle;
};

/* UVD decoder representation */
struct ruvd_decoder {
	struct pipe_video_decoder	base;

	ruvd_set_dtb			set_dtb;

	unsigned			stream_handle;
	unsigned			frame_number;

	struct radeon_winsys*		ws;
	struct radeon_winsys_cs*	cs;

	unsigned			cur_buffer;

	struct ruvd_buffer		msg_fb_buffers[NUM_BUFFERS];
	struct ruvd_buffer		bs_buffers[NUM_BUFFERS];
	void*				bs_ptr;
	unsigned			bs_size;

	struct ruvd_buffer		dpb;
};

/* generate an UVD stream handle */
static unsigned alloc_stream_handle()
{
	static unsigned counter = 0;
	unsigned stream_handle = 0;
	unsigned pid = getpid();
	int i;

	for (i = 0; i < 32; ++i)
		stream_handle |= ((pid >> i) & 1) << (31 - i);

	stream_handle ^= ++counter;
	return stream_handle;
}

/* flush IB to the hardware */
static void flush(struct ruvd_decoder *dec)
{
	uint32_t *pm4 =	dec->cs->buf;

	// align IB
	while(dec->cs->cdw % 16)
		pm4[dec->cs->cdw++] = RUVD_PKT2();

	dec->ws->cs_flush(dec->cs, 0, 0);
}

/* add a new set register command to the IB */
static void set_reg(struct ruvd_decoder *dec, unsigned reg, uint32_t val)
{
	uint32_t *pm4 =	dec->cs->buf;
	pm4[dec->cs->cdw++] = RUVD_PKT0(reg >> 2, 0);
	pm4[dec->cs->cdw++] = val;
}

/* send a command to the VCPU through the GPCOM registers */
static void send_cmd(struct ruvd_decoder *dec, unsigned cmd,
		     struct radeon_winsys_cs_handle* cs_buf, uint32_t off,
		     enum radeon_bo_usage usage, enum radeon_bo_domain domain)
{
	int reloc_idx;

	reloc_idx = dec->ws->cs_add_reloc(dec->cs, cs_buf, usage, domain);
	set_reg(dec, RUVD_GPCOM_VCPU_DATA0, off);
	set_reg(dec, RUVD_GPCOM_VCPU_DATA1, reloc_idx * 4);
	set_reg(dec, RUVD_GPCOM_VCPU_CMD, cmd << 1);
}

/* send a message command to the VCPU */
static void send_msg(struct ruvd_decoder *dec, struct ruvd_msg *msg)
{
	struct ruvd_buffer* buf;
	void *ptr;

	/* grap a message buffer */
	buf = &dec->msg_fb_buffers[dec->cur_buffer];

	/* copy the message into it */
	ptr = dec->ws->buffer_map(buf->cs_handle, dec->cs, PIPE_TRANSFER_WRITE);
	if (!ptr)
		return;

	memcpy(ptr, msg, sizeof(*msg));
	memset(ptr + sizeof(*msg), 0, buf->buf->size - sizeof(*msg));
	dec->ws->buffer_unmap(buf->cs_handle);

	/* and send it to the hardware */
	send_cmd(dec, RUVD_CMD_MSG_BUFFER, buf->cs_handle, 0,
		 RADEON_USAGE_READ, RADEON_DOMAIN_VRAM);
}

/* create a buffer in the winsys */
static bool create_buffer(struct ruvd_decoder *dec,
			  struct ruvd_buffer *buffer,
			  unsigned size)
{
	buffer->buf = dec->ws->buffer_create(dec->ws, size, 4096, false,
					     RADEON_DOMAIN_GTT | RADEON_DOMAIN_VRAM);
	if (!buffer->buf)
		return false;

	buffer->cs_handle = dec->ws->buffer_get_cs_handle(buffer->buf);
	if (!buffer->cs_handle)
		return false;

	return true;
}

/* destroy a buffer */
static void destroy_buffer(struct ruvd_buffer *buffer)
{
	pb_reference(&buffer->buf, NULL);
	buffer->cs_handle = NULL;
}

/* reallocate a buffer, preserving its content */
static bool resize_buffer(struct ruvd_decoder *dec,
			  struct ruvd_buffer *new_buf,
			  unsigned new_size)
{
	unsigned bytes = MIN2(new_buf->buf->size, new_size);
	struct ruvd_buffer old_buf = *new_buf;
	void *src = NULL, *dst = NULL;

	if (!create_buffer(dec, new_buf, new_size))
		goto error;

	src = dec->ws->buffer_map(old_buf.cs_handle, dec->cs, PIPE_TRANSFER_READ);
	if (!src)
		goto error;

	dst = dec->ws->buffer_map(new_buf->cs_handle, dec->cs, PIPE_TRANSFER_WRITE);
	if (!dst)
		goto error;

	memcpy(dst, src, bytes);
	if (new_size > bytes) {
		new_size -= bytes;
		dst += bytes;
		memset(dst, 0, new_size);
	}
	dec->ws->buffer_unmap(new_buf->cs_handle);
	dec->ws->buffer_unmap(old_buf.cs_handle);
	destroy_buffer(&old_buf);
	return true;

error:
	if (src) dec->ws->buffer_unmap(old_buf.cs_handle);
	destroy_buffer(new_buf);
	*new_buf = old_buf;
	return false;
}

/* clear the buffer with zeros */
static void clear_buffer(struct ruvd_decoder *dec,
			 struct ruvd_buffer* buffer)
{
	//TODO: let the GPU do the job
	void *ptr = dec->ws->buffer_map(buffer->cs_handle, dec->cs,
					PIPE_TRANSFER_WRITE);
	if (!ptr)
		return;

	memset(ptr, 0, buffer->buf->size);
	dec->ws->buffer_unmap(buffer->cs_handle);
}

/* cycle to the next set of buffers */
static void next_buffer(struct ruvd_decoder *dec)
{
	++dec->cur_buffer;
	dec->cur_buffer %= NUM_BUFFERS;
}

/* convert the profile into something UVD understands */
static uint32_t profile2stream_type(enum pipe_video_profile profile)
{
	switch (u_reduce_video_profile(profile)) {
	case PIPE_VIDEO_CODEC_MPEG4_AVC:
		return RUVD_CODEC_H264;

	case PIPE_VIDEO_CODEC_VC1:
		return RUVD_CODEC_VC1;

	case PIPE_VIDEO_CODEC_MPEG12:
		return RUVD_CODEC_MPEG2;

	case PIPE_VIDEO_CODEC_MPEG4:
		return RUVD_CODEC_MPEG4;

	default:
		assert(0);
		return 0;
	}
}

/* calculate size of reference picture buffer */
static unsigned calc_dpb_size(enum pipe_video_profile profile,
			      unsigned width, unsigned height,
			      unsigned max_references)
{
	unsigned width_in_mb, height_in_mb, image_size, dpb_size;

	// always align them to MB size for dpb calculation
	width = align(width, VL_MACROBLOCK_WIDTH);
	height = align(height, VL_MACROBLOCK_HEIGHT);

	// always one more for currently decoded picture
	max_references += 1;

	// aligned size of a single frame
	image_size = width * height;
	image_size += image_size / 2;
	image_size = align(image_size, 1024);

	// picture width & height in 16 pixel units
	width_in_mb = width / VL_MACROBLOCK_WIDTH;
	height_in_mb = align(height / VL_MACROBLOCK_HEIGHT, 2);

	switch (u_reduce_video_profile(profile)) {
	case PIPE_VIDEO_CODEC_MPEG4_AVC:
		// the firmware seems to allways assume a minimum of ref frames
		max_references = MAX2(NUM_H264_REFS, max_references);

		// reference picture buffer
		dpb_size = image_size * max_references;

		// macroblock context buffer
		dpb_size += width_in_mb * height_in_mb * max_references * 192;

		// IT surface buffer
		dpb_size += width_in_mb * height_in_mb * 32;
		break;

	case PIPE_VIDEO_CODEC_VC1:
		// reference picture buffer
		dpb_size = image_size * max_references;

		// CONTEXT_BUFFER
		dpb_size += width_in_mb * height_in_mb * 128;

		// IT surface buffer
		dpb_size += width_in_mb * 64;

		// DB surface buffer
		dpb_size += width_in_mb * 128;

		// BP
		dpb_size += align(MAX2(width_in_mb, height_in_mb) * 7 * 16, 64);
		break;

	case PIPE_VIDEO_CODEC_MPEG12:
		// reference picture buffer, must be big enough for all frames
		dpb_size = image_size * NUM_MPEG2_REFS;
		break;

	case PIPE_VIDEO_CODEC_MPEG4:
		// reference picture buffer
		dpb_size = image_size * max_references;

		// CM
		dpb_size += width_in_mb * height_in_mb * 64;

		// IT surface buffer
		dpb_size += align(width_in_mb * height_in_mb * 32, 64);
		break;

	default:
		// something is missing here
		assert(0);

		// at least use a sane default value
		dpb_size = 32 * 1024 * 1024;
		break;
	}
	return dpb_size;
}

/* get h264 specific message bits */
static struct ruvd_h264 get_h264_msg(struct ruvd_decoder *dec, struct pipe_h264_picture_desc *pic)
{
	struct ruvd_h264 result;

	memset(&result, 0, sizeof(result));
	switch (pic->base.profile) {
	case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
		result.profile = RUVD_H264_PROFILE_BASELINE;
		break;

	case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
		result.profile = RUVD_H264_PROFILE_MAIN;
		break;

	case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
		result.profile = RUVD_H264_PROFILE_HIGH;
		break;

	default:
		assert(0);
		break;
	}
	if (((dec->base.width * dec->base.height) >> 8) <= 1620)
		result.level = 30;
	else
		result.level = 41;

	result.sps_info_flags = 0;
	result.sps_info_flags |= pic->direct_8x8_inference_flag << 0;
	result.sps_info_flags |= pic->mb_adaptive_frame_field_flag << 1;
	result.sps_info_flags |= pic->frame_mbs_only_flag << 2;
	result.sps_info_flags |= pic->delta_pic_order_always_zero_flag << 3;

	result.pps_info_flags = 0;
	result.pps_info_flags |= pic->transform_8x8_mode_flag << 0;
	result.pps_info_flags |= pic->redundant_pic_cnt_present_flag << 1;
	result.pps_info_flags |= pic->constrained_intra_pred_flag << 2;
	result.pps_info_flags |= pic->deblocking_filter_control_present_flag << 3;
	result.pps_info_flags |= pic->weighted_bipred_idc << 4;
	result.pps_info_flags |= pic->weighted_pred_flag << 6;
	result.pps_info_flags |= pic->pic_order_present_flag << 7;
	result.pps_info_flags |= pic->entropy_coding_mode_flag << 8;

	result.chroma_format = 0x1;
	result.bit_depth_luma_minus8 = 0;
	result.bit_depth_chroma_minus8 = 0;

	result.log2_max_frame_num_minus4 = pic->log2_max_frame_num_minus4;
	result.pic_order_cnt_type = pic->pic_order_cnt_type;
	result.log2_max_pic_order_cnt_lsb_minus4 = pic->log2_max_pic_order_cnt_lsb_minus4;
	result.num_ref_frames = pic->num_ref_frames;
	result.pic_init_qp_minus26 = pic->pic_init_qp_minus26;
	result.chroma_qp_index_offset = pic->chroma_qp_index_offset;
	result.second_chroma_qp_index_offset = pic->second_chroma_qp_index_offset;

	result.num_slice_groups_minus1 = 0;
	result.slice_group_map_type = 0;

	result.num_ref_idx_l0_active_minus1 = pic->num_ref_idx_l0_active_minus1;
	result.num_ref_idx_l1_active_minus1 = pic->num_ref_idx_l1_active_minus1;

	result.slice_group_change_rate_minus1 = 0;

	memcpy(result.scaling_list_4x4, pic->scaling_lists_4x4, 6*64);
	memcpy(result.scaling_list_8x8, pic->scaling_lists_8x8, 2*64);

	result.frame_num = pic->frame_num;
	memcpy(result.frame_num_list, pic->frame_num_list, 4*16);
	result.curr_field_order_cnt_list[0] = pic->field_order_cnt[0];
	result.curr_field_order_cnt_list[1] = pic->field_order_cnt[1];
	memcpy(result.field_order_cnt_list, pic->field_order_cnt_list, 4*16*2);

	result.decoded_pic_idx = pic->frame_num;

	return result;
}

/* get vc1 specific message bits */
static struct ruvd_vc1 get_vc1_msg(struct pipe_vc1_picture_desc *pic)
{
	struct ruvd_vc1 result;

	memset(&result, 0, sizeof(result));
	switch(pic->base.profile) {
	case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
		result.profile = RUVD_VC1_PROFILE_SIMPLE;
		break;

	case PIPE_VIDEO_PROFILE_VC1_MAIN:
		result.profile = RUVD_VC1_PROFILE_MAIN;
		break;
		
	case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
		result.profile = RUVD_VC1_PROFILE_ADVANCED;
		break;
	default:
		assert(0);
	}

	if (pic->base.profile == PIPE_VIDEO_PROFILE_VC1_ADVANCED) {
		result.level = 0;

		result.sps_info_flags |= pic->postprocflag << 7;
		result.sps_info_flags |= pic->pulldown << 6;
		result.sps_info_flags |= pic->interlace << 5;
		result.sps_info_flags |= pic->tfcntrflag << 4;
		result.sps_info_flags |= pic->psf << 1;

		result.pps_info_flags |= pic->panscan_flag << 7;
		result.pps_info_flags |= pic->refdist_flag << 6;
		result.pps_info_flags |= pic->extended_dmv << 8;
		result.pps_info_flags |= pic->range_mapy_flag << 31;
		result.pps_info_flags |= pic->range_mapy << 28;
		result.pps_info_flags |= pic->range_mapuv_flag << 27;
		result.pps_info_flags |= pic->range_mapuv << 24;

	} else {
		result.level = 0;
		result.pps_info_flags |= pic->multires << 21;
		result.pps_info_flags |= pic->syncmarker << 20;
		result.pps_info_flags |= pic->rangered << 19;
		result.pps_info_flags |= pic->maxbframes << 16;
	}

	result.sps_info_flags |= pic->finterpflag << 3;
	//(((unsigned int)(pPicParams->advance.reserved1))        << SPS_INFO_VC1_RESERVED_SHIFT)

	result.pps_info_flags |= pic->loopfilter << 5;
	result.pps_info_flags |= pic->fastuvmc << 4;
	result.pps_info_flags |= pic->extended_mv << 3;
	result.pps_info_flags |= pic->dquant << 1;
	result.pps_info_flags |= pic->vstransform << 0;
	result.pps_info_flags |= pic->overlap << 11;
	result.pps_info_flags |= pic->quantizer << 9;


#if 0
uint32_t 	slice_count
uint8_t 	picture_type
uint8_t 	frame_coding_mode
uint8_t 	deblockEnable
uint8_t 	pquant
#endif

        result.chroma_format  = 1;
	return result;
}

/* extract the frame number from a referenced video buffer */
static uint32_t get_ref_pic_idx(struct ruvd_decoder *dec, struct pipe_video_buffer *ref)
{
	uint32_t min = dec->frame_number - NUM_MPEG2_REFS;
	uint32_t max = dec->frame_number - 1;
	uintptr_t frame;

	/* seems to be the most sane fallback */
	if (!ref)
		return max;

	/* get the frame number from the associated data */
	frame = (uintptr_t)vl_video_buffer_get_associated_data(ref, &dec->base);

	/* limit the frame number to a valid range */
	return MAX2(MIN2(frame, max), min);
}

/* get mpeg2 specific msg bits */
static struct ruvd_mpeg2 get_mpeg2_msg(struct ruvd_decoder *dec,
				       struct pipe_mpeg12_picture_desc *pic)
{
	const int *zscan = pic->alternate_scan ? vl_zscan_alternate : vl_zscan_normal;
	struct ruvd_mpeg2 result;
	unsigned i;

	memset(&result, 0, sizeof(result));
	result.decoded_pic_idx = dec->frame_number;
	for (i = 0; i < 2; ++i)
		result.ref_pic_idx[i] = get_ref_pic_idx(dec, pic->ref[i]);

	result.load_intra_quantiser_matrix = 1;
	result.load_nonintra_quantiser_matrix = 1;

	for (i = 0; i < 64; ++i) {
		result.intra_quantiser_matrix[i] = pic->intra_matrix[zscan[i]];
		result.nonintra_quantiser_matrix[i] = pic->non_intra_matrix[zscan[i]];
	}

	result.profile_and_level_indication = 0;
	result.chroma_format = 0x1;

	result.picture_coding_type = pic->picture_coding_type;
	result.f_code[0][0] = pic->f_code[0][0] + 1;
	result.f_code[0][1] = pic->f_code[0][1] + 1;
	result.f_code[1][0] = pic->f_code[1][0] + 1;
	result.f_code[1][1] = pic->f_code[1][1] + 1;
	result.intra_dc_precision = pic->intra_dc_precision;
	result.pic_structure = pic->picture_structure;
	result.top_field_first = pic->top_field_first;
	result.frame_pred_frame_dct = pic->frame_pred_frame_dct;
	result.concealment_motion_vectors = pic->concealment_motion_vectors;
	result.q_scale_type = pic->q_scale_type;
	result.intra_vlc_format = pic->intra_vlc_format;
	result.alternate_scan = pic->alternate_scan;

	return result;
}

/* get mpeg4 specific msg bits */
static struct ruvd_mpeg4 get_mpeg4_msg(struct ruvd_decoder *dec,
				       struct pipe_mpeg4_picture_desc *pic)
{
	struct ruvd_mpeg4 result;
	unsigned i;

	memset(&result, 0, sizeof(result));
	result.decoded_pic_idx = dec->frame_number;
	for (i = 0; i < 2; ++i)
		result.ref_pic_idx[i] = get_ref_pic_idx(dec, pic->ref[i]);

	result.variant_type = 0;
	result.profile_and_level_indication = 0xF0; // ASP Level0

	result.video_object_layer_verid = 0x5; // advanced simple
	result.video_object_layer_shape = 0x0; // rectangular

	result.video_object_layer_width = dec->base.width;
	result.video_object_layer_height = dec->base.height;

	result.vop_time_increment_resolution = pic->vop_time_increment_resolution;

	result.flags |= pic->short_video_header << 0;
	//result.flags |= obmc_disable << 1;
	result.flags |= pic->interlaced << 2;
        result.flags |= 1 << 3; // load_intra_quant_mat
	result.flags |= 1 << 4; // load_nonintra_quant_mat
	result.flags |= pic->quarter_sample << 5;
	result.flags |= 1 << 6; // complexity_estimation_disable
	result.flags |= pic->resync_marker_disable << 7;
	//result.flags |= data_partitioned << 8;
	//result.flags |= reversible_vlc << 9;
	result.flags |= 0 << 10; // newpred_enable
	result.flags |= 0 << 11; // reduced_resolution_vop_enable
	//result.flags |= scalability << 12;
	//result.flags |= is_object_layer_identifier << 13;
	//result.flags |= fixed_vop_rate << 14;
	//result.flags |= newpred_segment_type << 15;

	result.quant_type = pic->quant_type;

	for (i = 0; i < 64; ++i) {
		result.intra_quant_mat[i] = pic->intra_matrix[vl_zscan_normal[i]];
		result.nonintra_quant_mat[i] = pic->non_intra_matrix[vl_zscan_normal[i]];
	}

	/*
	int32_t 	trd [2]
	int32_t 	trb [2]
	uint8_t 	vop_coding_type
	uint8_t 	vop_fcode_forward
	uint8_t 	vop_fcode_backward
	uint8_t 	rounding_control
	uint8_t 	alternate_vertical_scan_flag
	uint8_t 	top_field_first
	*/

	return result;
}

/**
 * destroy this video decoder
 */
static void ruvd_destroy(struct pipe_video_decoder *decoder)
{
	struct ruvd_decoder *dec = (struct ruvd_decoder*)decoder;
	struct ruvd_msg msg;
	unsigned i;

	assert(decoder);

	memset(&msg, 0, sizeof(msg));
	msg.size = sizeof(msg);
	msg.msg_type = RUVD_MSG_DESTROY;
	msg.stream_handle = dec->stream_handle;
	send_msg(dec, &msg);

	flush(dec);

	dec->ws->cs_destroy(dec->cs);

	for (i = 0; i < NUM_BUFFERS; ++i) {
		destroy_buffer(&dec->msg_fb_buffers[i]);
		destroy_buffer(&dec->bs_buffers[i]);
	}

	destroy_buffer(&dec->dpb);

	FREE(dec);
}

/* free associated data in the video buffer callback */
static void ruvd_destroy_associated_data(void *data)
{
	/* NOOP, since we only use an intptr */
}

/**
 * start decoding of a new frame
 */
static void ruvd_begin_frame(struct pipe_video_decoder *decoder,
			     struct pipe_video_buffer *target,
			     struct pipe_picture_desc *picture)
{
	struct ruvd_decoder *dec = (struct ruvd_decoder*)decoder;
	uintptr_t frame;

	assert(decoder);

	frame = ++dec->frame_number;
	vl_video_buffer_set_associated_data(target, decoder, (void *)frame,
					    &ruvd_destroy_associated_data);

	dec->bs_size = 0;
	dec->bs_ptr = dec->ws->buffer_map(
		dec->bs_buffers[dec->cur_buffer].cs_handle,
		dec->cs, PIPE_TRANSFER_WRITE);
}

/**
 * decode a macroblock
 */
static void ruvd_decode_macroblock(struct pipe_video_decoder *decoder,
				   struct pipe_video_buffer *target,
				   struct pipe_picture_desc *picture,
				   const struct pipe_macroblock *macroblocks,
				   unsigned num_macroblocks)
{
	/* not supported (yet) */
	assert(0);
}

/**
 * decode a bitstream
 */
static void ruvd_decode_bitstream(struct pipe_video_decoder *decoder,
				  struct pipe_video_buffer *target,
				  struct pipe_picture_desc *picture,
				  unsigned num_buffers,
				  const void * const *buffers,
				  const unsigned *sizes)
{
	struct ruvd_decoder *dec = (struct ruvd_decoder*)decoder;
	unsigned i;

	assert(decoder);

	if (!dec->bs_ptr)
		return;

	for (i = 0; i < num_buffers; ++i) {
		struct ruvd_buffer *buf = &dec->bs_buffers[dec->cur_buffer];
		unsigned new_size = dec->bs_size + sizes[i];

		if (new_size > buf->buf->size) {
			dec->ws->buffer_unmap(buf->cs_handle);
			if (!resize_buffer(dec, buf, new_size)) {
				RUVD_ERR("Can't resize bitstream buffer!");
				return;
			}

			dec->bs_ptr = dec->ws->buffer_map(buf->cs_handle, dec->cs,
							  PIPE_TRANSFER_WRITE);
			if (!dec->bs_ptr)
				return;

			dec->bs_ptr += dec->bs_size;
		}

		memcpy(dec->bs_ptr, buffers[i], sizes[i]);
		dec->bs_size += sizes[i];
		dec->bs_ptr += sizes[i];
	}
}

/**
 * end decoding of the current frame
 */
static void ruvd_end_frame(struct pipe_video_decoder *decoder,
			   struct pipe_video_buffer *target,
			   struct pipe_picture_desc *picture)
{
	struct ruvd_decoder *dec = (struct ruvd_decoder*)decoder;
	struct radeon_winsys_cs_handle *dt;
	struct ruvd_buffer *msg_fb_buf, *bs_buf;
	struct ruvd_msg msg;
	unsigned bs_size;

	assert(decoder);

	if (!dec->bs_ptr)
		return;

	msg_fb_buf = &dec->msg_fb_buffers[dec->cur_buffer];
	bs_buf = &dec->bs_buffers[dec->cur_buffer];

	bs_size = align(dec->bs_size, 128);
	memset(dec->bs_ptr, 0, bs_size - dec->bs_size);
	dec->ws->buffer_unmap(bs_buf->cs_handle);

	memset(&msg, 0, sizeof(msg));
	msg.size = sizeof(msg);
	msg.msg_type = RUVD_MSG_DECODE;
	msg.stream_handle = dec->stream_handle;
	msg.status_report_feedback_number = dec->frame_number;

	msg.body.decode.stream_type = profile2stream_type(dec->base.profile);
	msg.body.decode.decode_flags = 0x1;
	msg.body.decode.width_in_samples = dec->base.width;
	msg.body.decode.height_in_samples = dec->base.height;

	msg.body.decode.dpb_size = dec->dpb.buf->size;
	msg.body.decode.bsd_size = bs_size;

	dt = dec->set_dtb(&msg, (struct vl_video_buffer *)target);

	switch (u_reduce_video_profile(picture->profile)) {
	case PIPE_VIDEO_CODEC_MPEG4_AVC:
		msg.body.decode.codec.h264 = get_h264_msg(dec, (struct pipe_h264_picture_desc*)picture);
		break;

	case PIPE_VIDEO_CODEC_VC1:
		msg.body.decode.codec.vc1 = get_vc1_msg((struct pipe_vc1_picture_desc*)picture);
		break;

	case PIPE_VIDEO_CODEC_MPEG12:
		msg.body.decode.codec.mpeg2 = get_mpeg2_msg(dec, (struct pipe_mpeg12_picture_desc*)picture);
		break;

	case PIPE_VIDEO_CODEC_MPEG4:
		msg.body.decode.codec.mpeg4 = get_mpeg4_msg(dec, (struct pipe_mpeg4_picture_desc*)picture);
		break;

	default:
		assert(0);
		return;
	}

	msg.body.decode.db_surf_tile_config = msg.body.decode.dt_surf_tile_config;
	msg.body.decode.extension_support = 0x1;

	send_msg(dec, &msg);
	send_cmd(dec, RUVD_CMD_DPB_BUFFER, dec->dpb.cs_handle, 0,
		 RADEON_USAGE_READWRITE, RADEON_DOMAIN_VRAM);
	send_cmd(dec, RUVD_CMD_BITSTREAM_BUFFER, bs_buf->cs_handle,
		 0, RADEON_USAGE_READ, RADEON_DOMAIN_GTT);
	send_cmd(dec, RUVD_CMD_DECODING_TARGET_BUFFER, dt, 0,
		 RADEON_USAGE_WRITE, RADEON_DOMAIN_VRAM);
	send_cmd(dec, RUVD_CMD_FEEDBACK_BUFFER, msg_fb_buf->cs_handle,
		 0x1000, RADEON_USAGE_WRITE, RADEON_DOMAIN_VRAM);
	set_reg(dec, RUVD_ENGINE_CNTL, 1);

	flush(dec);
	next_buffer(dec);
}

/**
 * flush any outstanding command buffers to the hardware
 */
static void ruvd_flush(struct pipe_video_decoder *decoder)
{
}

/**
 * create and UVD decoder
 */
struct pipe_video_decoder *ruvd_create_decoder(struct pipe_context *context,
					       enum pipe_video_profile profile,
					       enum pipe_video_entrypoint entrypoint,
					       enum pipe_video_chroma_format chroma_format,
					       unsigned width, unsigned height,
					       unsigned max_references, bool expect_chunked_decode,
					       struct radeon_winsys* ws,
					       ruvd_set_dtb set_dtb)
{
	unsigned dpb_size = calc_dpb_size(profile, width, height, max_references);
	struct radeon_info info;
	struct ruvd_decoder *dec;
	struct ruvd_msg msg;
	int i;

	ws->query_info(ws, &info);

	switch(u_reduce_video_profile(profile)) {
	case PIPE_VIDEO_CODEC_MPEG12:
		if (entrypoint > PIPE_VIDEO_ENTRYPOINT_BITSTREAM || info.family < CHIP_PALM)
			return vl_create_mpeg12_decoder(context, profile, entrypoint,
							chroma_format, width,
							height, max_references, expect_chunked_decode);

		/* fall through */
	case PIPE_VIDEO_CODEC_MPEG4:
	case PIPE_VIDEO_CODEC_MPEG4_AVC:
		width = align(width, VL_MACROBLOCK_WIDTH);
		height = align(height, VL_MACROBLOCK_HEIGHT);
		break;

	default:
		break;
	}


	dec = CALLOC_STRUCT(ruvd_decoder);

	if (!dec)
		return NULL;

	dec->base.context = context;
	dec->base.profile = profile;
	dec->base.entrypoint = entrypoint;
	dec->base.chroma_format = chroma_format;
	dec->base.width = width;
	dec->base.height = height;

	dec->base.destroy = ruvd_destroy;
	dec->base.begin_frame = ruvd_begin_frame;
	dec->base.decode_macroblock = ruvd_decode_macroblock;
	dec->base.decode_bitstream = ruvd_decode_bitstream;
	dec->base.end_frame = ruvd_end_frame;
	dec->base.flush = ruvd_flush;

	dec->set_dtb = set_dtb;
	dec->stream_handle = alloc_stream_handle();
	dec->ws = ws;
	dec->cs = ws->cs_create(ws, RING_UVD, NULL);
	if (!dec->cs) {
		RUVD_ERR("Can't get command submission context.\n");
		goto error;
	}

	for (i = 0; i < NUM_BUFFERS; ++i) {
		unsigned msg_fb_size = align(sizeof(struct ruvd_msg), 0x1000) + 0x1000;
		if (!create_buffer(dec, &dec->msg_fb_buffers[i], msg_fb_size)) {
			RUVD_ERR("Can't allocated message buffers.\n");
			goto error;
		}

		if (!create_buffer(dec, &dec->bs_buffers[i], 4096)) {
			RUVD_ERR("Can't allocated bitstream buffers.\n");
			goto error;
		}

		clear_buffer(dec, &dec->msg_fb_buffers[i]);
		clear_buffer(dec, &dec->bs_buffers[i]);
	}

	if (!create_buffer(dec, &dec->dpb, dpb_size)) {
		RUVD_ERR("Can't allocated dpb.\n");
		goto error;
	}

	clear_buffer(dec, &dec->dpb);

	memset(&msg, 0, sizeof(msg));
	msg.size = sizeof(msg);
	msg.msg_type = RUVD_MSG_CREATE;
	msg.stream_handle = dec->stream_handle;
	msg.body.create.stream_type = profile2stream_type(dec->base.profile);
	msg.body.create.width_in_samples = dec->base.width;
	msg.body.create.height_in_samples = dec->base.height;
	msg.body.create.dpb_size = dec->dpb.buf->size;
	send_msg(dec, &msg);
	flush(dec);
	next_buffer(dec);

	return &dec->base;

error:
	if (dec->cs) dec->ws->cs_destroy(dec->cs);

	for (i = 0; i < NUM_BUFFERS; ++i) {
		destroy_buffer(&dec->msg_fb_buffers[i]);
		destroy_buffer(&dec->bs_buffers[i]);
	}

	destroy_buffer(&dec->dpb);

	FREE(dec);

	return NULL;
}

/**
 * join surfaces into the same buffer with identical tiling params
 * sumup their sizes and replace the backend buffers with a single bo
 */
void ruvd_join_surfaces(struct radeon_winsys* ws, unsigned bind,
			struct pb_buffer** buffers[VL_NUM_COMPONENTS],
			struct radeon_surface *surfaces[VL_NUM_COMPONENTS])
{
	unsigned best_tiling, best_wh, off;
	unsigned size, alignment;
	struct pb_buffer *pb;
	unsigned i, j;

	for (i = 0, best_tiling = 0, best_wh = ~0; i < VL_NUM_COMPONENTS; ++i) {
		unsigned wh;

		if (!surfaces[i])
			continue;

		/* choose the smallest bank w/h for now */
		wh = surfaces[i]->bankw * surfaces[i]->bankh;
		if (wh < best_wh) {
			best_wh = wh;
			best_tiling = i;
		}
	}

	for (i = 0, off = 0; i < VL_NUM_COMPONENTS; ++i) {
		if (!surfaces[i])
			continue;

		/* copy the tiling parameters */
		surfaces[i]->bankw = surfaces[best_tiling]->bankw;
		surfaces[i]->bankh = surfaces[best_tiling]->bankh;
		surfaces[i]->mtilea = surfaces[best_tiling]->mtilea;
		surfaces[i]->tile_split = surfaces[best_tiling]->tile_split;

		/* adjust the texture layer offsets */
		off = align(off, surfaces[i]->bo_alignment);
		for (j = 0; j < Elements(surfaces[i]->level); ++j)
			surfaces[i]->level[j].offset += off;
		off += surfaces[i]->bo_size;
	}

	for (i = 0, size = 0, alignment = 0; i < VL_NUM_COMPONENTS; ++i) {
		if (!buffers[i] || !*buffers[i])
			continue;

		size = align(size, (*buffers[i])->alignment);
		size += (*buffers[i])->size;
		alignment = MAX2(alignment, (*buffers[i])->alignment * 1);
	}

	if (!size)
		return;

	/* TODO: 2D tiling workaround */
	alignment *= 2;

	pb = ws->buffer_create(ws, size, alignment, bind, RADEON_DOMAIN_VRAM);
	if (!pb)
		return;

	for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
		if (!buffers[i] || !*buffers[i])
			continue;

		pb_reference(buffers[i], pb);
	}

	pb_reference(&pb, NULL);
}

/* calculate top/bottom offset */
static unsigned texture_offset(struct radeon_surface *surface, unsigned layer)
{
	return surface->level[0].offset +
		layer * surface->level[0].slice_size;
}

/* hw encode the aspect of macro tiles */
static unsigned macro_tile_aspect(unsigned macro_tile_aspect)
{
	switch (macro_tile_aspect) {
	default:
	case 1: macro_tile_aspect = 0;  break;
	case 2: macro_tile_aspect = 1;  break;
	case 4: macro_tile_aspect = 2;  break;
	case 8: macro_tile_aspect = 3;  break;
	}
	return macro_tile_aspect;
}

/* hw encode the bank width and height */
static unsigned bank_wh(unsigned bankwh)
{
	switch (bankwh) {
	default:
	case 1: bankwh = 0;     break;
	case 2: bankwh = 1;     break;
	case 4: bankwh = 2;     break;
	case 8: bankwh = 3;     break;
	}
	return bankwh;
}

/**
 * fill decoding target field from the luma and chroma surfaces
 */
void ruvd_set_dt_surfaces(struct ruvd_msg *msg, struct radeon_surface *luma,
			  struct radeon_surface *chroma)
{
	msg->body.decode.dt_pitch = luma->level[0].pitch_bytes;
	switch (luma->level[0].mode) {
	case RADEON_SURF_MODE_LINEAR_ALIGNED:
		msg->body.decode.dt_tiling_mode = RUVD_TILE_LINEAR;
		msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_LINEAR;
		break;
	case RADEON_SURF_MODE_1D:
		msg->body.decode.dt_tiling_mode = RUVD_TILE_8X8;
		msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_1D_THIN;
		break;
	case RADEON_SURF_MODE_2D:
		msg->body.decode.dt_tiling_mode = RUVD_TILE_8X8;
		msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_2D_THIN;
		break;
	default:
		assert(0);
		break;
	}

	msg->body.decode.dt_luma_top_offset = texture_offset(luma, 0);
	msg->body.decode.dt_chroma_top_offset = texture_offset(chroma, 0);
	if (msg->body.decode.dt_field_mode) {
		msg->body.decode.dt_luma_bottom_offset = texture_offset(luma, 1);
		msg->body.decode.dt_chroma_bottom_offset = texture_offset(chroma, 1);
	} else {
		msg->body.decode.dt_luma_bottom_offset = msg->body.decode.dt_luma_top_offset;
		msg->body.decode.dt_chroma_bottom_offset = msg->body.decode.dt_chroma_top_offset;
	}

	assert(luma->bankw == chroma->bankw);
	assert(luma->bankh == chroma->bankh);
	assert(luma->mtilea == chroma->mtilea);

	msg->body.decode.dt_surf_tile_config |= RUVD_BANK_WIDTH(bank_wh(luma->bankw));
	msg->body.decode.dt_surf_tile_config |= RUVD_BANK_HEIGHT(bank_wh(luma->bankh));
	msg->body.decode.dt_surf_tile_config |= RUVD_MACRO_TILE_ASPECT_RATIO(macro_tile_aspect(luma->mtilea));
}

int ruvd_get_video_param(struct pipe_screen *screen,
			 enum pipe_video_profile profile,
			 enum pipe_video_cap param)
{
	switch (param) {
	case PIPE_VIDEO_CAP_SUPPORTED:
		switch (u_reduce_video_profile(profile)) {
		case PIPE_VIDEO_CODEC_MPEG12:
		case PIPE_VIDEO_CODEC_MPEG4:
		case PIPE_VIDEO_CODEC_MPEG4_AVC:
		case PIPE_VIDEO_CODEC_VC1:
			return true;
		default:
			return false;
		}
	case PIPE_VIDEO_CAP_NPOT_TEXTURES:
		return 1;
	case PIPE_VIDEO_CAP_MAX_WIDTH:
		return 2048;
	case PIPE_VIDEO_CAP_MAX_HEIGHT:
		return 1152;
	case PIPE_VIDEO_CAP_PREFERED_FORMAT:
		return PIPE_FORMAT_NV12;
	case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
		return true;
	case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
		return true;
	case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
		return true;
	default:
		return 0;
	}
}

boolean ruvd_is_format_supported(struct pipe_screen *screen,
				 enum pipe_format format,
				 enum pipe_video_profile profile)
{
	/* we can only handle this one anyway */
	return format == PIPE_FORMAT_NV12;
}
