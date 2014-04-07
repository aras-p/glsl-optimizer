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

#include <stdio.h>

#include "pipe/p_video_codec.h"

#include "util/u_video.h"
#include "util/u_memory.h"

#include "vl/vl_video_buffer.h"

#include "../../winsys/radeon/drm/radeon_winsys.h"
#include "r600_pipe_common.h"
#include "radeon_video.h"
#include "radeon_vce.h"

/**
 * flush commands to the hardware
 */
static void flush(struct rvce_encoder *enc)
{
	enc->ws->cs_flush(enc->cs, RADEON_FLUSH_ASYNC, 0);
}

#if 0
static void dump_feedback(struct rvce_encoder *enc, struct rvid_buffer *fb)
{
	uint32_t *ptr = enc->ws->buffer_map(fb->cs_handle, enc->cs, PIPE_TRANSFER_READ_WRITE);
	unsigned i = 0;
	fprintf(stderr, "\n");
	fprintf(stderr, "encStatus:\t\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "encHasBitstream:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "encHasAudioBitstream:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "encBitstreamOffset:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "encBitstreamSize:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "encAudioBitstreamOffset:\t%08x\n", ptr[i++]);
	fprintf(stderr, "encAudioBitstreamSize:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "encExtrabytes:\t\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "encAudioExtrabytes:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "videoTimeStamp:\t\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "audioTimeStamp:\t\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "videoOutputType:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "attributeFlags:\t\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "seiPrivatePackageOffset:\t%08x\n", ptr[i++]);
	fprintf(stderr, "seiPrivatePackageSize:\t\t%08x\n", ptr[i++]);
	fprintf(stderr, "\n");
	enc->ws->buffer_unmap(fb->cs_handle);
}
#endif

/**
 * destroy this video encoder
 */
static void rvce_destroy(struct pipe_video_codec *encoder)
{
	struct rvce_encoder *enc = (struct rvce_encoder*)encoder;
	if (enc->stream_handle) {
		struct rvid_buffer fb;
		rvid_create_buffer(enc->ws, &fb, 512, RADEON_DOMAIN_GTT);
		enc->fb = &fb;
		enc->session(enc);
		enc->feedback(enc);
		enc->destroy(enc);
		flush(enc);
		rvid_destroy_buffer(&fb);
	}
	rvid_destroy_buffer(&enc->cpb);
	enc->ws->cs_destroy(enc->cs);
	FREE(enc);
}

static void rvce_begin_frame(struct pipe_video_codec *encoder,
			     struct pipe_video_buffer *source,
			     struct pipe_picture_desc *picture)
{
	struct rvce_encoder *enc = (struct rvce_encoder*)encoder;
	struct vl_video_buffer *vid_buf = (struct vl_video_buffer *)source;
	struct pipe_h264_enc_picture_desc *pic = (struct pipe_h264_enc_picture_desc *)picture;

	bool need_rate_control =
		enc->pic.rate_ctrl.rate_ctrl_method != pic->rate_ctrl.rate_ctrl_method ||
		enc->pic.quant_i_frames != pic->quant_i_frames ||
		enc->pic.quant_p_frames != pic->quant_p_frames ||
		enc->pic.quant_b_frames != pic->quant_b_frames;

	enc->pic = *pic;

	enc->get_buffer(vid_buf->resources[0], &enc->handle, &enc->luma);
	enc->get_buffer(vid_buf->resources[1], NULL, &enc->chroma);
	
	if (!enc->stream_handle) {
		struct rvid_buffer fb;
		enc->stream_handle = rvid_alloc_stream_handle();
		rvid_create_buffer(enc->ws, &fb, 512, RADEON_DOMAIN_GTT);
		enc->fb = &fb;
		enc->session(enc);
		enc->create(enc);
		enc->rate_control(enc);
		need_rate_control = false;
		enc->config_extension(enc);
		enc->motion_estimation(enc);
		enc->rdo(enc);
		enc->pic_control(enc);
		enc->feedback(enc);
		flush(enc);
		//dump_feedback(enc, &fb);
		rvid_destroy_buffer(&fb);
	}

	enc->session(enc);

	if (need_rate_control)
		enc->rate_control(enc);
}

static void rvce_encode_bitstream(struct pipe_video_codec *encoder,
				  struct pipe_video_buffer *source,
				  struct pipe_resource *destination,
				  void **fb)
{
	struct rvce_encoder *enc = (struct rvce_encoder*)encoder;
	enc->get_buffer(destination, &enc->bs_handle, NULL);
	enc->bs_size = destination->width0;

	*fb = enc->fb = CALLOC_STRUCT(rvid_buffer);
	if (!rvid_create_buffer(enc->ws, enc->fb, 512, RADEON_DOMAIN_GTT)) {
		RVID_ERR("Can't create feedback buffer.\n");
		return;
	}
	enc->encode(enc);
	enc->feedback(enc);
}

static void rvce_end_frame(struct pipe_video_codec *encoder,
			   struct pipe_video_buffer *source,
			   struct pipe_picture_desc *picture)
{
	struct rvce_encoder *enc = (struct rvce_encoder*)encoder;
	flush(enc);
}

static void rvce_get_feedback(struct pipe_video_codec *encoder,
			      void *feedback, unsigned *size)
{
	struct rvce_encoder *enc = (struct rvce_encoder*)encoder;
	struct rvid_buffer *fb = feedback;

	if (size) {
		uint32_t *ptr = enc->ws->buffer_map(fb->cs_handle, enc->cs, PIPE_TRANSFER_READ_WRITE);

		if (ptr[1]) {
			*size = ptr[4] - ptr[9];
		} else {
			*size = 0;
		}

		enc->ws->buffer_unmap(fb->cs_handle);
	}
	//dump_feedback(enc, fb);
	rvid_destroy_buffer(fb);
	FREE(fb);
}

/**
 * flush any outstanding command buffers to the hardware
 */
static void rvce_flush(struct pipe_video_codec *encoder)
{
}

static void rvce_cs_flush(void *ctx, unsigned flags)
{
	// just ignored
}

struct pipe_video_codec *rvce_create_encoder(struct pipe_context *context,
					     const struct pipe_video_codec *templ,
					     struct radeon_winsys* ws,
					     rvce_get_buffer get_buffer)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen *)context->screen;
	struct rvce_encoder *enc;
	struct pipe_video_buffer *tmp_buf, templat = {};
	struct radeon_surface *tmp_surf;
	unsigned pitch, vpitch;

	if (!rscreen->info.vce_fw_version) {
		RVID_ERR("Kernel doesn't supports VCE!\n");
		return NULL;

	} else if (!rvce_is_fw_version_supported(rscreen)) {
		RVID_ERR("Unsupported VCE fw version loaded!\n");
		return NULL;
	}

	enc = CALLOC_STRUCT(rvce_encoder);
	if (!enc)
		return NULL;

	enc->base = *templ;
	enc->base.context = context;

	enc->base.destroy = rvce_destroy;
	enc->base.begin_frame = rvce_begin_frame;
	enc->base.encode_bitstream = rvce_encode_bitstream;
	enc->base.end_frame = rvce_end_frame;
	enc->base.flush = rvce_flush;
	enc->base.get_feedback = rvce_get_feedback;
	enc->get_buffer = get_buffer;

	enc->ws = ws;
	enc->cs = ws->cs_create(ws, RING_VCE, NULL);
	if (!enc->cs) {
		RVID_ERR("Can't get command submission context.\n");
		goto error;
	}

	enc->ws->cs_set_flush_callback(enc->cs, rvce_cs_flush, enc);
	templat.buffer_format = PIPE_FORMAT_NV12;
	templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
	templat.width = enc->base.width;
	templat.height = enc->base.height;
	templat.interlaced = false;
	if (!(tmp_buf = context->create_video_buffer(context, &templat))) {
		RVID_ERR("Can't create video buffer.\n");
		goto error;
	}

	get_buffer(((struct vl_video_buffer *)tmp_buf)->resources[0], NULL, &tmp_surf);
	pitch = align(tmp_surf->level[0].pitch_bytes, 128);
	vpitch = align(tmp_surf->npix_y, 16);
	tmp_buf->destroy(tmp_buf);
	if (!rvid_create_buffer(enc->ws, &enc->cpb,
			pitch * vpitch * 1.5 * RVCE_NUM_CPB_FRAMES,
			RADEON_DOMAIN_VRAM)) {
		RVID_ERR("Can't create CPB buffer.\n");
		goto error;
	}

	radeon_vce_40_2_2_init(enc);

	return &enc->base;

error:
	if (enc->cs)
		enc->ws->cs_destroy(enc->cs);

	rvid_destroy_buffer(&enc->cpb);

	FREE(enc);
	return NULL;
}

/**
 * check if kernel has the right fw version loaded
 */
bool rvce_is_fw_version_supported(struct r600_common_screen *rscreen)
{
	return rscreen->info.vce_fw_version == ((40 << 24) | (2 << 16) | (2 << 8));
}
