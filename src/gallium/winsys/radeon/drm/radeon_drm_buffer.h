/*
 * Copyright © 2008 Jérôme Glisse
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Jérôme Glisse <glisse@freedesktop.org>
 */
#ifndef RADEON_DRM_BUFFER_H
#define RADEON_DRM_BUFFER_H

#include "radeon_winsys.h"

#define RADEON_PB_USAGE_VERTEX      (1 << 28)
#define RADEON_PB_USAGE_DOMAIN_GTT  (1 << 29)
#define RADEON_PB_USAGE_DOMAIN_VRAM (1 << 30)

static INLINE struct pb_buffer *
radeon_pb_buffer(struct r300_winsys_buffer *buffer)
{
    return (struct pb_buffer *)buffer;
}

struct pb_manager *radeon_drm_bufmgr_create(struct radeon_drm_winsys *rws);
struct pb_buffer *radeon_drm_bufmgr_create_buffer_from_handle(struct pb_manager *_mgr,
							      uint32_t handle);
void radeon_drm_bufmgr_flush_maps(struct pb_manager *_mgr);
boolean radeon_drm_bufmgr_get_handle(struct pb_buffer *_buf,
				     struct winsys_handle *whandle);
void radeon_drm_bufmgr_init_functions(struct radeon_drm_winsys *ws);

#endif
