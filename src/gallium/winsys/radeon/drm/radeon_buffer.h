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
#ifndef RADEON_BUFFER_H
#define RADEON_BUFFER_H

#include <stdio.h>

#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_bufmgr.h"

#include "radeon_bo.h"
#include "radeon_cs.h"

#include "radeon_winsys.h"

#define RADEON_PB_USAGE_VERTEX      (1 << 28)
#define RADEON_PB_USAGE_DOMAIN_GTT  (1 << 29)
#define RADEON_PB_USAGE_DOMAIN_VRAM (1 << 30)

static INLINE struct pb_buffer *
radeon_pb_buffer(struct r300_winsys_buffer *buffer)
{
    return (struct pb_buffer *)buffer;
}

static INLINE struct r300_winsys_buffer *
radeon_libdrm_winsys_buffer(struct pb_buffer *buffer)
{
    return (struct r300_winsys_buffer *)buffer;
}

struct pb_manager *
radeon_drm_bufmgr_create(struct radeon_libdrm_winsys *rws);

void radeon_drm_bufmgr_add_buffer(struct r300_winsys_cs *cs,
                                  struct r300_winsys_buffer *buf,
                                  enum r300_buffer_domain rd,
                                  enum r300_buffer_domain wd);

void radeon_drm_bufmgr_write_reloc(struct r300_winsys_cs *cs,
                                   struct r300_winsys_buffer *buf,
				   enum r300_buffer_domain rd,
                                   enum r300_buffer_domain wd);

struct pb_buffer *radeon_drm_bufmgr_create_buffer_from_handle(struct pb_manager *_mgr,
							      uint32_t handle);

void radeon_drm_bufmgr_get_tiling(struct r300_winsys_screen *ws,
                                  struct r300_winsys_buffer *buf,
                                  enum r300_buffer_tiling *microtiled,
                                  enum r300_buffer_tiling *macrotiled);

void radeon_drm_bufmgr_set_tiling(struct r300_winsys_screen *ws,
                                  struct r300_winsys_buffer *buf,
                                  enum r300_buffer_tiling microtiled,
                                  enum r300_buffer_tiling macrotiled,
                                  uint32_t pitch);

void radeon_drm_bufmgr_flush_maps(struct pb_manager *_mgr);

boolean radeon_drm_bufmgr_get_handle(struct pb_buffer *_buf,
				     struct winsys_handle *whandle);

boolean radeon_drm_bufmgr_is_buffer_referenced(struct r300_winsys_cs *cs,
                                               struct r300_winsys_buffer *buf,
                                               enum r300_reference_domain domain);

void radeon_drm_bufmgr_wait(struct r300_winsys_screen *ws,
                            struct r300_winsys_buffer *buf);

void *radeon_drm_buffer_map(struct r300_winsys_screen *ws,
                            struct r300_winsys_buffer *buf,
                            struct r300_winsys_cs *cs,
                            enum pipe_transfer_usage usage);

void radeon_drm_buffer_unmap(struct r300_winsys_screen *ws,
                             struct r300_winsys_buffer *buf);

#endif
