/*
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

#ifndef RADEON_DRM_CS_H
#define RADEON_DRM_CS_H

#include "radeon_drm_bo.h"
#include <radeon_drm.h>

struct radeon_drm_cs {
    struct r300_winsys_cs base;

    /* The winsys. */
    struct radeon_drm_winsys *ws;

    /* Flush CS. */
    void (*flush_cs)(void *);
    void *flush_data;

    /* Relocs. */
    unsigned                    crelocs;
    unsigned                    nrelocs;
    struct drm_radeon_cs_reloc  *relocs;
    struct radeon_bo            **relocs_bo;
    struct drm_radeon_cs        cs;
    struct drm_radeon_cs_chunk  chunks[2];

    unsigned used_vram;
    unsigned used_gart;

    /* 0 = BO not added, 1 = BO added */
    char                        is_handle_added[256];
    struct drm_radeon_cs_reloc  *relocs_hashlist[256];
    unsigned                    reloc_indices_hashlist[256];
};

int radeon_get_reloc(struct radeon_drm_cs *cs, struct radeon_bo *bo);

static INLINE struct radeon_drm_cs *
radeon_drm_cs(struct r300_winsys_cs *base)
{
    return (struct radeon_drm_cs*)base;
}

static INLINE boolean radeon_bo_is_referenced_by_cs(struct radeon_drm_cs *cs,
                                                    struct radeon_bo *bo)
{
    return bo->num_cs_references == bo->rws->num_cs ||
           (bo->num_cs_references && radeon_get_reloc(cs, bo) != -1);
}

static INLINE boolean radeon_bo_is_referenced_by_any_cs(struct radeon_bo *bo)
{
    return bo->num_cs_references;
}

void radeon_drm_cs_init_functions(struct radeon_drm_winsys *ws);

#endif
