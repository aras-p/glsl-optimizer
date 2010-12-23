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

static INLINE int radeon_bo_is_referenced_by_cs(struct radeon_drm_cs *cs,
                                                struct radeon_bo *bo)
{
    return radeon_get_reloc(cs, bo) != -1;
}

static INLINE int radeon_bo_is_referenced_by_any_cs(struct radeon_bo *bo)
{
    return bo->cref > 1;
}

void radeon_drm_cs_init_functions(struct radeon_drm_winsys *ws);

#endif
