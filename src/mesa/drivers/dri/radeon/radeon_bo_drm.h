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
//#include "radeon_track.h"

#ifndef RADEON_DEBUG_BO
#define RADEON_DEBUG_BO 0
#endif

/* bo object */
#define RADEON_BO_FLAGS_MACRO_TILE  1
#define RADEON_BO_FLAGS_MICRO_TILE  2

#define RADEON_TILING_MACRO 0x1
#define RADEON_TILING_MICRO 0x2
#define RADEON_TILING_SWAP 0x4
#define RADEON_TILING_SURFACE 0x8 /* this object requires a surface
				   * when mapped - i.e. front buffer */

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
#ifdef RADEON_DEBUG_BO
    struct radeon_bo *(*bo_open)(struct radeon_bo_manager *bom,
                                 uint32_t handle,
                                 uint32_t size,
                                 uint32_t alignment,
                                 uint32_t domains,
                                 uint32_t flags,
                                 char * szBufUsage);
#else
    struct radeon_bo *(*bo_open)(struct radeon_bo_manager *bom,
                                 uint32_t handle,
                                 uint32_t size,
                                 uint32_t alignment,
                                 uint32_t domains,
                                 uint32_t flags);
#endif /* RADEON_DEBUG_BO */
    void (*bo_ref)(struct radeon_bo *bo);
    struct radeon_bo *(*bo_unref)(struct radeon_bo *bo);
    int (*bo_map)(struct radeon_bo *bo, int write);
    int (*bo_unmap)(struct radeon_bo *bo);
    int (*bo_wait)(struct radeon_bo *bo);
    int (*bo_is_static)(struct radeon_bo *bo);
    int (*bo_set_tiling)(struct radeon_bo *bo, uint32_t tiling_flags,
			  uint32_t pitch);
    int (*bo_get_tiling)(struct radeon_bo *bo, uint32_t *tiling_flags,
			  uint32_t *pitch);
};

struct radeon_bo_manager {
    struct radeon_bo_funcs  *funcs;
    int                     fd;

#ifdef RADEON_BO_TRACK
    struct radeon_tracker   tracker;
#endif
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
#ifdef RADEON_DEBUG_BO
                                                char * szBufUsage,
#endif /* RADEON_DEBUG_BO */
                                                const char *file,
                                                const char *func,
                                                int line)
{
    struct radeon_bo *bo;

#ifdef RADEON_DEBUG_BO
    bo = bom->funcs->bo_open(bom, handle, size, alignment, domains, flags, szBufUsage);
#else
    bo = bom->funcs->bo_open(bom, handle, size, alignment, domains, flags);
#endif /* RADEON_DEBUG_BO */

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

static inline int radeon_bo_set_tiling(struct radeon_bo *bo,
				       uint32_t tiling_flags, uint32_t pitch)
{
    return bo->bom->funcs->bo_set_tiling(bo, tiling_flags, pitch);
}

static inline int radeon_bo_get_tiling(struct radeon_bo *bo,
				       uint32_t *tiling_flags, uint32_t *pitch)
{
    return bo->bom->funcs->bo_get_tiling(bo, tiling_flags, pitch);
}

static inline int radeon_bo_is_static(struct radeon_bo *bo)
{
	if (bo->bom->funcs->bo_is_static)
		return bo->bom->funcs->bo_is_static(bo);
	return 0;
}

#ifdef RADEON_DEBUG_BO
#define radeon_bo_open(bom, h, s, a, d, f, u)\
    _radeon_bo_open(bom, h, s, a, d, f, u, __FILE__, __FUNCTION__, __LINE__)
#else
#define radeon_bo_open(bom, h, s, a, d, f)\
    _radeon_bo_open(bom, h, s, a, d, f, __FILE__, __FUNCTION__, __LINE__)
#endif /* RADEON_DEBUG_BO */
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
