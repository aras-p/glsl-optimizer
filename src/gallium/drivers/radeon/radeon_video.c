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

#include <unistd.h>

#include "util/u_memory.h"
#include "util/u_video.h"

#include "vl/vl_defines.h"
#include "vl/vl_video_buffer.h"

#include "../../winsys/radeon/drm/radeon_winsys.h"
#include "r600_pipe_common.h"
#include "radeon_video.h"
#include "radeon_vce.h"

/* generate an stream handle */
unsigned rvid_alloc_stream_handle()
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

/* create a buffer in the winsys */
bool rvid_create_buffer(struct radeon_winsys *ws, struct rvid_buffer *buffer,
			unsigned size, enum radeon_bo_domain domain)
{
	buffer->domain = domain;

	buffer->buf = ws->buffer_create(ws, size, 4096, false, domain);
	if (!buffer->buf)
		return false;

	buffer->cs_handle = ws->buffer_get_cs_handle(buffer->buf);
	if (!buffer->cs_handle)
		return false;

	return true;
}

/* destroy a buffer */
void rvid_destroy_buffer(struct rvid_buffer *buffer)
{
	pb_reference(&buffer->buf, NULL);
	buffer->cs_handle = NULL;
}

/* reallocate a buffer, preserving its content */
bool rvid_resize_buffer(struct radeon_winsys *ws, struct radeon_winsys_cs *cs,
			struct rvid_buffer *new_buf, unsigned new_size)
{
	unsigned bytes = MIN2(new_buf->buf->size, new_size);
	struct rvid_buffer old_buf = *new_buf;
	void *src = NULL, *dst = NULL;

	if (!rvid_create_buffer(ws, new_buf, new_size, new_buf->domain))
		goto error;

	src = ws->buffer_map(old_buf.cs_handle, cs, PIPE_TRANSFER_READ);
	if (!src)
		goto error;

	dst = ws->buffer_map(new_buf->cs_handle, cs, PIPE_TRANSFER_WRITE);
	if (!dst)
		goto error;

	memcpy(dst, src, bytes);
	if (new_size > bytes) {
		new_size -= bytes;
		dst += bytes;
		memset(dst, 0, new_size);
	}
	ws->buffer_unmap(new_buf->cs_handle);
	ws->buffer_unmap(old_buf.cs_handle);
	rvid_destroy_buffer(&old_buf);
	return true;

error:
	if (src)
		ws->buffer_unmap(old_buf.cs_handle);
	rvid_destroy_buffer(new_buf);
	*new_buf = old_buf;
	return false;
}

/* clear the buffer with zeros */
void rvid_clear_buffer(struct radeon_winsys *ws, struct radeon_winsys_cs *cs, struct rvid_buffer* buffer)
{
        void *ptr = ws->buffer_map(buffer->cs_handle, cs, PIPE_TRANSFER_WRITE);
        if (!ptr)
                return;

        memset(ptr, 0, buffer->buf->size);
        ws->buffer_unmap(buffer->cs_handle);
}

/**
 * join surfaces into the same buffer with identical tiling params
 * sumup their sizes and replace the backend buffers with a single bo
 */
void rvid_join_surfaces(struct radeon_winsys* ws, unsigned bind,
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

int rvid_get_video_param(struct pipe_screen *screen,
			 enum pipe_video_profile profile,
			 enum pipe_video_entrypoint entrypoint,
			 enum pipe_video_cap param)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen *)screen;

	if (entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
		switch (param) {
		case PIPE_VIDEO_CAP_SUPPORTED:
			return u_reduce_video_profile(profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC &&
				rvce_is_fw_version_supported(rscreen);
	        case PIPE_VIDEO_CAP_NPOT_TEXTURES:
        	        return 1;
	        case PIPE_VIDEO_CAP_MAX_WIDTH:
        	        return 2048;
	        case PIPE_VIDEO_CAP_MAX_HEIGHT:
        	        return 1152;
	        case PIPE_VIDEO_CAP_PREFERED_FORMAT:
        	        return PIPE_FORMAT_NV12;
	        case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
        	        return false;
	        case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
        	        return false;
	        case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
        	        return true;
	        default:
        	        return 0;
		}
	}

	/* UVD 2.x limits */
	if (rscreen->family < CHIP_PALM) {
		enum pipe_video_format codec = u_reduce_video_profile(profile);
		switch (param) {
		case PIPE_VIDEO_CAP_SUPPORTED:
			/* no support for MPEG4 */
			return codec != PIPE_VIDEO_FORMAT_MPEG4 &&
			       /* FIXME: VC-1 simple/main profile is broken */
			       profile != PIPE_VIDEO_PROFILE_VC1_SIMPLE &&
			       profile != PIPE_VIDEO_PROFILE_VC1_MAIN;
		case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
		case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
			/* and MPEG2 only with shaders */
			return codec != PIPE_VIDEO_FORMAT_MPEG12;
		default:
			break;
		}
	}

	switch (param) {
	case PIPE_VIDEO_CAP_SUPPORTED:
		switch (u_reduce_video_profile(profile)) {
		case PIPE_VIDEO_FORMAT_MPEG12:
		case PIPE_VIDEO_FORMAT_MPEG4:
		case PIPE_VIDEO_FORMAT_MPEG4_AVC:
			return entrypoint != PIPE_VIDEO_ENTRYPOINT_ENCODE;
		case PIPE_VIDEO_FORMAT_VC1:
			/* FIXME: VC-1 simple/main profile is broken */
			return profile == PIPE_VIDEO_PROFILE_VC1_ADVANCED &&
			       entrypoint != PIPE_VIDEO_ENTRYPOINT_ENCODE;
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
	case PIPE_VIDEO_CAP_MAX_LEVEL:
		switch (profile) {
		case PIPE_VIDEO_PROFILE_MPEG1:
			return 0;
		case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
		case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
			return 3;
		case PIPE_VIDEO_PROFILE_MPEG4_SIMPLE:
			return 3;
		case PIPE_VIDEO_PROFILE_MPEG4_ADVANCED_SIMPLE:
			return 5;
		case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
			return 1;
		case PIPE_VIDEO_PROFILE_VC1_MAIN:
			return 2;
		case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
			return 4;
		case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
		case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
		case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
			return 41;
		default:
			return 0;
		}
	default:
		return 0;
	}
}

boolean rvid_is_format_supported(struct pipe_screen *screen,
				 enum pipe_format format,
				 enum pipe_video_profile profile,
				 enum pipe_video_entrypoint entrypoint)
{
	/* we can only handle this one with UVD */
	if (profile != PIPE_VIDEO_PROFILE_UNKNOWN)
		return format == PIPE_FORMAT_NV12;

	return vl_video_buffer_is_format_supported(screen, format, profile, entrypoint);
}
