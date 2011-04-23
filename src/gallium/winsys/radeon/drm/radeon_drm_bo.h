/*
 * Copyright © 2008 Jérôme Glisse
 * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
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
 *      Marek Olšák <maraeo@gmail.com>
 */
#ifndef RADEON_DRM_BO_H
#define RADEON_DRM_BO_H

#include "radeon_drm_winsys.h"
#include "pipebuffer/pb_bufmgr.h"
#include "os/os_thread.h"

#define RADEON_PB_USAGE_DOMAIN_GTT  (1 << 29)
#define RADEON_PB_USAGE_DOMAIN_VRAM (1 << 30)

struct radeon_bomgr;

struct radeon_bo {
    struct pb_buffer base;
    struct radeon_bomgr *mgr;
    struct radeon_drm_winsys *rws;

    void *ptr;
    pipe_mutex map_mutex;

    uint32_t size;
    uint32_t handle;
    uint32_t name;

    /* how many command streams is this bo referenced in? */
    int num_cs_references;

    /* how many command streams, which are being emitted in a separate
     * thread, is this bo referenced in? */
    int num_active_ioctls;

    /* Whether the buffer has been relocated for write and is busy since then.
     * This field is updated in:
     * - radeon_drm_cs_flush (to TRUE if it's relocated for write)
     * - radeon_bo_is_busy (to FALSE if it's not busy)
     * - radeon_bo_wait (to FALSE) */
    boolean busy_for_write;

    boolean flinked;
    uint32_t flink;
};

struct pb_manager *radeon_bomgr_create(struct radeon_drm_winsys *rws);
void radeon_bomgr_init_functions(struct radeon_drm_winsys *ws);

static INLINE
void radeon_bo_reference(struct radeon_bo **dst, struct radeon_bo *src)
{
    pb_reference((struct pb_buffer**)dst, (struct pb_buffer*)src);
}

static INLINE struct pb_buffer *
pb_buffer(struct pb_buffer *buffer)
{
    return (struct pb_buffer *)buffer;
}

#endif
