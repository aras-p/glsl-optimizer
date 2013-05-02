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

#include "radeonsi_pipe.h"
#include "radeon/radeon_uvd.h"
#include "sid.h"

/**
 * creates an video buffer with an UVD compatible memory layout
 */
struct pipe_video_buffer *radeonsi_video_buffer_create(struct pipe_context *pipe,
						       const struct pipe_video_buffer *tmpl)
{
	struct r600_context *ctx = (struct r600_context *)pipe;
	struct r600_resource_texture *resources[VL_NUM_COMPONENTS] = {};
	struct radeon_surface *surfaces[VL_NUM_COMPONENTS] = {};
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
	/* TODO: Setting the transfer flag is only a workaround till we get tiling working */
	templ.flags = R600_RESOURCE_FLAG_TRANSFER;
	resources[0] = (struct r600_resource_texture *)
		pipe->screen->resource_create(pipe->screen, &templ);
	if (!resources[0])
		goto error;

	if (resource_formats[1] != PIPE_FORMAT_NONE) {
		vl_video_buffer_template(&templ, &template, resource_formats[1], 1, array_size, PIPE_USAGE_STATIC, 1);
		templ.flags = R600_RESOURCE_FLAG_TRANSFER;
		resources[1] = (struct r600_resource_texture *)
			pipe->screen->resource_create(pipe->screen, &templ);
		if (!resources[1])
			goto error;
	}

	if (resource_formats[2] != PIPE_FORMAT_NONE) {
		vl_video_buffer_template(&templ, &template, resource_formats[2], 1, array_size, PIPE_USAGE_STATIC, 2);
		templ.flags = R600_RESOURCE_FLAG_TRANSFER;
		resources[2] = (struct r600_resource_texture *)
			pipe->screen->resource_create(pipe->screen, &templ);
		if (!resources[2])
			goto error;
	}

	for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
		if (!resources[i])
			continue;

		surfaces[i] = & resources[i]->surface;
		pbs[i] = &resources[i]->resource.buf;
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

/* set the decoding target buffer offsets */
static struct radeon_winsys_cs_handle* radeonsi_uvd_set_dtb(struct ruvd_msg *msg, struct vl_video_buffer *buf)
{
	struct r600_resource_texture *luma = (struct r600_resource_texture *)buf->resources[0];
	struct r600_resource_texture *chroma = (struct r600_resource_texture *)buf->resources[1];

	msg->body.decode.dt_field_mode = buf->base.interlaced;

	ruvd_set_dt_surfaces(msg, &luma->surface, &chroma->surface);

	return luma->resource.cs_buf;
}

/**
 * creates an UVD compatible decoder
 */
struct pipe_video_decoder *radeonsi_uvd_create_decoder(struct pipe_context *context,
						       enum pipe_video_profile profile,
						       enum pipe_video_entrypoint entrypoint,
						       enum pipe_video_chroma_format chroma_format,
						       unsigned width, unsigned height,
						       unsigned max_references, bool expect_chunked_decode)
{
	struct r600_context *ctx = (struct r600_context *)context;

	return ruvd_create_decoder(context, profile, entrypoint, chroma_format,
				   width, height, max_references, expect_chunked_decode,
				   ctx->ws, radeonsi_uvd_set_dtb);
}
