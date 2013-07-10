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

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "pipe/p_video_decoder.h"

#include "util/u_memory.h"
#include "util/u_video.h"

#include "vl/vl_defines.h"
#include "vl/vl_mpeg12_decoder.h"

#include "r600_pipe.h"
#include "radeon/radeon_uvd.h"
#include "r600d.h"

/**
 * creates an video buffer with an UVD compatible memory layout
 */
struct pipe_video_buffer *r600_video_buffer_create(struct pipe_context *pipe,
						   const struct pipe_video_buffer *tmpl)
{
	struct r600_context *ctx = (struct r600_context *)pipe;
	struct r600_texture *resources[VL_NUM_COMPONENTS] = {};
	struct radeon_surface* surfaces[VL_NUM_COMPONENTS] = {};
	struct pb_buffer **pbs[VL_NUM_COMPONENTS] = {};
	const enum pipe_format *resource_formats;
	struct pipe_video_buffer template;
	struct pipe_resource templ;
	unsigned i, array_size;

	assert(pipe);

	/* first create the needed resources as "normal" textures */
	resource_formats = vl_video_buffer_formats(pipe->screen, tmpl->buffer_format);
	if (!resource_formats)
		return NULL;

	array_size = tmpl->interlaced ? 2 : 1;
	template = *tmpl;
	template.width = align(tmpl->width, VL_MACROBLOCK_WIDTH);
	template.height = align(tmpl->height / array_size, VL_MACROBLOCK_HEIGHT);

	vl_video_buffer_template(&templ, &template, resource_formats[0], 1, array_size, PIPE_USAGE_STATIC, 0);
	if (ctx->chip_class < EVERGREEN || tmpl->interlaced)
		templ.flags = R600_RESOURCE_FLAG_TRANSFER;
	resources[0] = (struct r600_texture *)
		pipe->screen->resource_create(pipe->screen, &templ);
	if (!resources[0])
		goto error;

	if (resource_formats[1] != PIPE_FORMAT_NONE) {
		vl_video_buffer_template(&templ, &template, resource_formats[1], 1, array_size, PIPE_USAGE_STATIC, 1);
		if (ctx->chip_class < EVERGREEN || tmpl->interlaced)
			templ.flags = R600_RESOURCE_FLAG_TRANSFER;
		resources[1] = (struct r600_texture *)
			pipe->screen->resource_create(pipe->screen, &templ);
		if (!resources[1])
			goto error;
	}

	if (resource_formats[2] != PIPE_FORMAT_NONE) {
		vl_video_buffer_template(&templ, &template, resource_formats[2], 1, array_size, PIPE_USAGE_STATIC, 2);
		if (ctx->chip_class < EVERGREEN || tmpl->interlaced)
			templ.flags = R600_RESOURCE_FLAG_TRANSFER;
		resources[2] = (struct r600_texture *)
			pipe->screen->resource_create(pipe->screen, &templ);
		if (!resources[2])
			goto error;
	}

	for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
		if (!resources[i])
			continue;

		pbs[i] = &resources[i]->resource.buf;
		surfaces[i] = &resources[i]->surface;
	}

	ruvd_join_surfaces(ctx->ws, templ.bind, pbs, surfaces);

	for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
		if (!resources[i])
			continue;

		/* recreate the CS handle */
		resources[i]->resource.cs_buf = ctx->ws->buffer_get_cs_handle(
			resources[i]->resource.buf);
	}

	template.height *= array_size;
	return vl_video_buffer_create_ex2(pipe, &template, (struct pipe_resource **)resources);

error:
	for (i = 0; i < VL_NUM_COMPONENTS; ++i)
		pipe_resource_reference((struct pipe_resource **)&resources[i], NULL);

	return NULL;
}

/* hw encode the number of memory banks */
static uint32_t eg_num_banks(uint32_t nbanks)
{
	switch (nbanks) {
	case 2:
		return 0;
	case 4:
		return 1;
	case 8:
	default:
		return 2;
	case 16:
		return 3;
	}
}

/* set the decoding target buffer offsets */
static struct radeon_winsys_cs_handle* r600_uvd_set_dtb(struct ruvd_msg *msg, struct vl_video_buffer *buf)
{
	struct r600_screen *rscreen = (struct r600_screen*)buf->base.context->screen;
	struct r600_texture *luma = (struct r600_texture *)buf->resources[0];
	struct r600_texture *chroma = (struct r600_texture *)buf->resources[1];

	msg->body.decode.dt_field_mode = buf->base.interlaced;
	msg->body.decode.dt_surf_tile_config |= RUVD_NUM_BANKS(eg_num_banks(rscreen->tiling_info.num_banks));

	ruvd_set_dt_surfaces(msg, &luma->surface, &chroma->surface);

	return luma->resource.cs_buf;
}

/* create decoder */
struct pipe_video_decoder *r600_uvd_create_decoder(struct pipe_context *context,
						   enum pipe_video_profile profile,
						   enum pipe_video_entrypoint entrypoint,
						   enum pipe_video_chroma_format chroma_format,
						   unsigned width, unsigned height,
						   unsigned max_references, bool expect_chunked_decode)
{
	struct r600_context *ctx = (struct r600_context *)context;

	return ruvd_create_decoder(context, profile, entrypoint, chroma_format,
			 	   width, height, max_references, expect_chunked_decode,
				   ctx->ws, r600_uvd_set_dtb);
}

int r600_uvd_get_video_param(struct pipe_screen *screen,
			     enum pipe_video_profile profile,
			     enum pipe_video_cap param)
{
	struct r600_screen *rscreen = (struct r600_screen *)screen;

	/* UVD 2.x limits */
	if (rscreen->family < CHIP_PALM) {
		enum pipe_video_codec codec = u_reduce_video_profile(profile);
		switch (param) {
		case PIPE_VIDEO_CAP_SUPPORTED:
			/* no support for MPEG4 */
	    		return codec != PIPE_VIDEO_CODEC_MPEG4;
        	case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
        	case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
			/* and MPEG2 only with shaders */
	    		return codec != PIPE_VIDEO_CODEC_MPEG12;
		default:
			break;
		}
	}

	return ruvd_get_video_param(screen, profile, param);
}
