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

#ifndef RADEON_VCE_H
#define RADEON_VCE_H

#include "util/u_double_list.h"

#define RVCE_RELOC(buf, usage, domain) (enc->ws->cs_add_reloc(enc->cs, (buf), (usage), domain, RADEON_PRIO_MIN))

#define RVCE_CS(value) (enc->cs->buf[enc->cs->cdw++] = (value))
#define RVCE_BEGIN(cmd) { uint32_t *begin = &enc->cs->buf[enc->cs->cdw++]; RVCE_CS(cmd)
#define RVCE_READ(buf, domain) RVCE_CS(RVCE_RELOC(buf, RADEON_USAGE_READ, domain) * 4)
#define RVCE_WRITE(buf, domain) RVCE_CS(RVCE_RELOC(buf, RADEON_USAGE_WRITE, domain) * 4)
#define RVCE_READWRITE(buf, domain) RVCE_CS(RVCE_RELOC(buf, RADEON_USAGE_READWRITE, domain) * 4)
#define RVCE_END() *begin = (&enc->cs->buf[enc->cs->cdw] - begin) * 4; }

#define RVCE_NUM_CPB_FRAMES 3

struct r600_common_screen;

/* driver dependent callback */
typedef void (*rvce_get_buffer)(struct pipe_resource *resource,
				struct radeon_winsys_cs_handle **handle,
				struct radeon_surface **surface);

/* Coded picture buffer slot */
struct rvce_cpb_slot {
	struct list_head		list;

	unsigned			index;
	enum pipe_h264_enc_picture_type	picture_type;
	unsigned			frame_num;
	unsigned			pic_order_cnt;
};

/* VCE encoder representation */
struct rvce_encoder {
	struct pipe_video_codec		base;

	/* version specific packets */
	void (*session)(struct rvce_encoder *enc);
	void (*create)(struct rvce_encoder *enc);
	void (*feedback)(struct rvce_encoder *enc);
	void (*rate_control)(struct rvce_encoder *enc);
	void (*config_extension)(struct rvce_encoder *enc);
	void (*pic_control)(struct rvce_encoder *enc);
	void (*motion_estimation)(struct rvce_encoder *enc);
	void (*rdo)(struct rvce_encoder *enc);
	void (*encode)(struct rvce_encoder *enc);
	void (*destroy)(struct rvce_encoder *enc);

	unsigned			stream_handle;

	struct radeon_winsys*		ws;
	struct radeon_winsys_cs*	cs;

	rvce_get_buffer			get_buffer;

	struct radeon_winsys_cs_handle*	handle;
	struct radeon_surface*		luma;
	struct radeon_surface*		chroma;

	struct radeon_winsys_cs_handle*	bs_handle;
	unsigned			bs_size;

	struct rvce_cpb_slot		*cpb_array;
	struct list_head		cpb_slots;

	struct rvid_buffer		*fb;
	struct rvid_buffer		cpb;
	struct pipe_h264_enc_picture_desc pic;
};

struct pipe_video_codec *rvce_create_encoder(struct pipe_context *context,
					     const struct pipe_video_codec *templat,
					     struct radeon_winsys* ws,
					     rvce_get_buffer get_buffer);

bool rvce_is_fw_version_supported(struct r600_common_screen *rscreen);

/* init vce fw 40.2.2 specific callbacks */
void radeon_vce_40_2_2_init(struct rvce_encoder *enc);

#endif
