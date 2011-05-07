/*
 * Copyright © 2009 Corbin Simpson
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
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 */
#ifndef RADEON_DRM_WINSYS_H
#define RADEON_DRM_WINSYS_H

#include "radeon_winsys.h"

#include "os/os_thread.h"

struct radeon_drm_winsys {
    struct radeon_winsys base;

    int fd; /* DRM file descriptor */
    int num_cs; /* The number of command streams created. */

    struct pb_manager *kman;
    struct pb_manager *cman;

    uint32_t pci_id;        /* PCI ID */
    uint32_t gb_pipes;      /* GB pipe count */
    uint32_t z_pipes;       /* Z pipe count (rv530 only) */
    uint32_t gart_size;     /* GART size. */
    uint32_t vram_size;     /* VRAM size. */
    uint32_t num_cpus;      /* Number of CPUs. */

    unsigned drm_major;
    unsigned drm_minor;
    unsigned drm_patchlevel;

    struct radeon_drm_cs *hyperz_owner;
    pipe_mutex hyperz_owner_mutex;
    struct radeon_drm_cs *cmask_owner;
    pipe_mutex cmask_owner_mutex;
};

static INLINE struct radeon_drm_winsys *
radeon_drm_winsys(struct radeon_winsys *base)
{
    return (struct radeon_drm_winsys*)base;
}

#endif
