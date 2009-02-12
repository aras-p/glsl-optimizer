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
#ifndef RADEON_BO_H
#define RADEON_BO_H

#include <stdio.h>
#include <stdint.h>
#include "radeon_track.h"

/* bo object */
#define RADEON_BO_FLAGS_MACRO_TILE  1
#define RADEON_BO_FLAGS_MICRO_TILE  2

struct radeon_bo_manager;

struct radeon_bo {
    uint32_t                    alignment;
    uint32_t                    handle;
    uint32_t                    size;
    uint32_t                    domains;
    uint32_t                    flags;
    unsigned                    cref;
#ifdef RADEON_BO_TRACK
    struct radeon_track         *track;
#endif
    void                        *ptr;
    struct radeon_bo_manager    *bom;
    uint32_t                    space_accounted;
};

/* bo functions */
struct radeon_bo_funcs {
    struct radeon_bo *(*bo_open)(struct radeon_bo_manager *bom,
                                 uint32_t handle,
                                 uint32_t size,
                                 uint32_t alignment,
                                 uint32_t domains,
                                 uint32_t flags);
    void (*bo_ref)(struct radeon_bo *bo);
    struct radeon_bo *(*bo_unref)(struct radeon_bo *bo);
    int (*bo_map)(struct radeon_bo *bo, int write);
    int (*bo_unmap)(struct radeon_bo *bo);
    int (*bo_wait)(struct radeon_bo *bo);
};

struct radeon_bo_manager {
    struct radeon_bo_funcs  *funcs;
    int                     fd;
    struct radeon_tracker   tracker;
};
    
static inline void _radeon_bo_debug(struct radeon_bo *bo,
                                    const char *op,
                                    const char *file,
                                    const char *func,
                                    int line)
{
    fprintf(stderr, "%s %p 0x%08X 0x%08X 0x%08X [%s %s %d]\n",
            op, bo, bo->handle, bo->size, bo->cref, file, func, line);
}

static inline struct radeon_bo *_radeon_bo_open(struct radeon_bo_manager *bom,
                                                uint32_t handle,
                                                uint32_t size,
                                                uint32_t alignment,
                                                uint32_t domains,
                                                uint32_t flags,
                                                const char *file,
                                                const char *func,
                                                int line)
{
    struct radeon_bo *bo;

    bo = bom->funcs->bo_open(bom, handle, size, alignment, domains, flags);
#ifdef RADEON_BO_TRACK
    if (bo) {
        bo->track = radeon_tracker_add_track(&bom->tracker, bo->handle);
        radeon_track_add_event(bo->track, file, func, "open", line);
    }
#endif
    return bo;
}

static inline void _radeon_bo_ref(struct radeon_bo *bo,
                                  const char *file,
                                  const char *func,
                                  int line)
{
    bo->cref++;
#ifdef RADEON_BO_TRACK
    radeon_track_add_event(bo->track, file, func, "ref", line); 
#endif
    bo->bom->funcs->bo_ref(bo);
}

static inline struct radeon_bo *_radeon_bo_unref(struct radeon_bo *bo,
                                                 const char *file,
                                                 const char *func,
                                                 int line)
{
    bo->cref--;
#ifdef RADEON_BO_TRACK
    radeon_track_add_event(bo->track, file, func, "unref", line);
    if (bo->cref <= 0) {
        radeon_tracker_remove_track(&bo->bom->tracker, bo->track);
        bo->track = NULL;
    }
#endif
    return bo->bom->funcs->bo_unref(bo);
}

static inline int _radeon_bo_map(struct radeon_bo *bo,
                                 int write,
                                 const char *file,
                                 const char *func,
                                 int line)
{
    return bo->bom->funcs->bo_map(bo, write);
}

static inline int _radeon_bo_unmap(struct radeon_bo *bo,
                                   const char *file,
                                   const char *func,
                                   int line)
{
    return bo->bom->funcs->bo_unmap(bo);
}

static inline int _radeon_bo_wait(struct radeon_bo *bo,
                                  const char *file,
                                  const char *func,
                                  int line)
{
    return bo->bom->funcs->bo_wait(bo);
}

#define radeon_bo_open(bom, h, s, a, d, f)\
    _radeon_bo_open(bom, h, s, a, d, f, __FILE__, __FUNCTION__, __LINE__)
#define radeon_bo_ref(bo)\
    _radeon_bo_ref(bo, __FILE__, __FUNCTION__, __LINE__)
#define radeon_bo_unref(bo)\
    _radeon_bo_unref(bo, __FILE__, __FUNCTION__, __LINE__)
#define radeon_bo_map(bo, w)\
    _radeon_bo_map(bo, w, __FILE__, __FUNCTION__, __LINE__)
#define radeon_bo_unmap(bo)\
    _radeon_bo_unmap(bo, __FILE__, __FUNCTION__, __LINE__)
#define radeon_bo_debug(bo, opcode)\
    _radeon_bo_debug(bo, opcode, __FILE__, __FUNCTION__, __LINE__)
#define radeon_bo_wait(bo) \
    _radeon_bo_wait(bo, __FILE__, __func__, __LINE__)

#endif
