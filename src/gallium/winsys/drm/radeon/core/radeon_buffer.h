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

#include "pipebuffer/pb_buffer.h"

#include "radeon_bo.h"
#include "radeon_cs.h"

#include "radeon_winsys.h"

struct radeon_pipe_buffer {
    struct pipe_buffer  base;
    /* Pointer to GPU-backed BO. */
    struct radeon_bo    *bo;
    /* Pointer to fallback PB buffer. */
    struct pb_buffer    *pb;
    boolean flinked;
    uint32_t flink;
};

#define RADEON_MAX_BOS 24

struct radeon_winsys_priv {
    /* DRM FD */
    int fd;

    /* Radeon BO manager. */
    struct radeon_bo_manager* bom;

    /* Radeon CS manager. */
    struct radeon_cs_manager* csm;

    /* Current CS. */
    struct radeon_cs* cs;

    /* Flush CB */
    void (*flush_cb)(void *);
    void *flush_data;
};

struct radeon_winsys* radeon_pipe_winsys(int fb);
#if 0
struct pipe_surface *radeon_surface_from_handle(struct radeon_context *radeon_context,
                                             uint32_t handle,
                                             enum pipe_format format,
                                             int w, int h, int pitch);
#endif
#endif
