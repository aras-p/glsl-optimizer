/* 
 * Copyright © 2008 Nicolai Haehnle
 * Copyright © 2008 Dave Airlie
 * Copyright © 2008 Jérôme Glisse
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Aapo Tahkola <aet@rasterburn.org>
 *      Nicolai Haehnle <prefect_@gmx.net>
 *      Dave Airlie
 *      Jérôme Glisse <glisse@freedesktop.org>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "xf86drm.h"
#include "drm.h"
#include "radeon_drm.h"
#include "radeon_bo.h"
#include "radeon_bo_legacy.h"
#include "radeon_ioctl.h"
#include "texmem.h"

struct bo_legacy {
    struct radeon_bo    base;
    driTextureObject    tobj_base;
    int                 map_count;
    uint32_t            pending;
    int                 is_pending;
    int                 validated;
    int                 static_bo;
    int                 got_dri_texture_obj;
    int                 dirty;
    uint32_t            offset;
    driTextureObject    dri_texture_obj;
    void                *ptr;
    struct bo_legacy    *next, *prev;
    struct bo_legacy    *pnext, *pprev;
};

struct bo_manager_legacy {
    struct radeon_bo_manager    base;
    unsigned                    nhandle;
    unsigned                    nfree_handles;
    unsigned                    cfree_handles;
    uint32_t                    current_age;
    struct bo_legacy            bos;
    struct bo_legacy            pending_bos;
    uint32_t                    fb_location;
    uint32_t                    texture_offset;
    unsigned                    dma_alloc_size;
    unsigned                    cpendings;
    driTextureObject            texture_swapped;
    driTexHeap                  *texture_heap;
    struct radeon_screen        *screen;
    unsigned                    *free_handles;
};

static void bo_legacy_tobj_destroy(void *data, driTextureObject *t)
{
    struct bo_legacy *bo_legacy;

    bo_legacy = (struct bo_legacy*)((char*)t)-sizeof(struct radeon_bo);
    bo_legacy->got_dri_texture_obj = 0;
    bo_legacy->validated = 0;
}

static int legacy_new_handle(struct bo_manager_legacy *bom, uint32_t *handle)
{
    uint32_t tmp;

    *handle = 0;
    if (bom->nhandle == 0xFFFFFFFF) {
        return -EINVAL;
    }
    if (bom->cfree_handles > 0) {
        tmp = bom->free_handles[--bom->cfree_handles];
        while (!bom->free_handles[bom->cfree_handles - 1]) {
            bom->cfree_handles--;
            if (bom->cfree_handles <= 0) {
                bom->cfree_handles = 0;
            }
        }
    } else {
        bom->cfree_handles = 0;
        tmp = bom->nhandle++;
    }
    assert(tmp);
    *handle = tmp;
    return 0;
}

static int legacy_free_handle(struct bo_manager_legacy *bom, uint32_t handle)
{
    uint32_t *handles;

    if (!handle) {
        return 0;
    }
    if (handle == (bom->nhandle - 1)) {
        int i;

        bom->nhandle--;
        for (i = bom->cfree_handles - 1; i >= 0; i--) {
            if (bom->free_handles[i] == (bom->nhandle - 1)) {
                bom->nhandle--;
                bom->free_handles[i] = 0;
            }
        }
        while (!bom->free_handles[bom->cfree_handles - 1]) {
            bom->cfree_handles--;
            if (bom->cfree_handles <= 0) {
                bom->cfree_handles = 0;
            }
        }
        return 0;
    }
    if (bom->cfree_handles < bom->nfree_handles) {
        bom->free_handles[bom->cfree_handles++] = handle;
        return 0;
    }
    bom->nfree_handles += 0x100;
    handles = (uint32_t*)realloc(bom->free_handles, bom->nfree_handles * 4);
    if (handles == NULL) {
        bom->nfree_handles -= 0x100;
        return -ENOMEM;
    }
    bom->free_handles = handles;
    bom->free_handles[bom->cfree_handles++] = handle;
    return 0;
}

static void legacy_get_current_age(struct bo_manager_legacy *boml)
{
    drm_radeon_getparam_t gp;
    int r;

    gp.param = RADEON_PARAM_LAST_CLEAR;
    gp.value = (int *)&boml->current_age;
    r = drmCommandWriteRead(boml->base.fd, DRM_RADEON_GETPARAM,
                            &gp, sizeof(gp));
    if (r) {
        fprintf(stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__, r);
        exit(1);
    }
}

static int legacy_is_pending(struct radeon_bo *bo)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    if (bo_legacy->is_pending <= 0) {
        bo_legacy->is_pending = 0;
        return 0;
    }
    if (boml->current_age >= bo_legacy->pending) {
        if (boml->pending_bos.pprev == bo_legacy) {
            boml->pending_bos.pprev = bo_legacy->pprev;
        }
        bo_legacy->pprev->pnext = bo_legacy->pnext;
        if (bo_legacy->pnext) {
            bo_legacy->pnext->pprev = bo_legacy->pprev;
        }
        while (bo_legacy->is_pending--) {
            radeon_bo_unref(bo);
        }
        bo_legacy->is_pending = 0;
        boml->cpendings--;
        return 0;
    }
    return 1;
}

static int legacy_wait_pending(struct radeon_bo *bo)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    if (!bo_legacy->is_pending) {
        return 0;
    }
    /* FIXME: lockup and userspace busy looping that's all the folks */
    legacy_get_current_age(boml);
    while (legacy_is_pending(bo)) {
        usleep(10);
        legacy_get_current_age(boml);
    }
    return 0;
}

static void legacy_track_pending(struct bo_manager_legacy *boml)
{
    struct bo_legacy *bo_legacy;
    struct bo_legacy *next;

    legacy_get_current_age(boml);
    bo_legacy = boml->pending_bos.pnext;
    while (bo_legacy) {
        next = bo_legacy->pnext;
        if (legacy_is_pending(&(bo_legacy->base))) {
        }
        bo_legacy = next;
    } 
}

static struct bo_legacy *bo_allocate(struct bo_manager_legacy *boml,
                                     uint32_t size,
                                     uint32_t alignment,
                                     uint32_t flags)
{
    struct bo_legacy *bo_legacy;

    bo_legacy = (struct bo_legacy*)calloc(1, sizeof(struct bo_legacy));
    if (bo_legacy == NULL) {
        return NULL;
    }
    bo_legacy->base.bom = (struct radeon_bo_manager*)boml;
    bo_legacy->base.handle = 0;
    bo_legacy->base.size = size;
    bo_legacy->base.alignment = alignment;
    bo_legacy->base.flags = flags;
    bo_legacy->base.ptr = NULL;
    bo_legacy->map_count = 0;
    bo_legacy->next = NULL;
    bo_legacy->prev = NULL;
    bo_legacy->got_dri_texture_obj = 0;
    bo_legacy->pnext = NULL;
    bo_legacy->pprev = NULL;
    bo_legacy->next = boml->bos.next;
    bo_legacy->prev = &boml->bos;
    boml->bos.next = bo_legacy;
    if (bo_legacy->next) {
        bo_legacy->next->prev = bo_legacy;
    }
    return bo_legacy;
}

static int bo_dma_alloc(struct radeon_bo *bo)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    drm_radeon_mem_alloc_t alloc;
    unsigned size;
    int base_offset;
    int r;

    /* align size on 4Kb */
    size = (((4 * 1024) - 1) + bo->size) & ~((4 * 1024) - 1);
    alloc.region = RADEON_MEM_REGION_GART;
    alloc.alignment = bo_legacy->base.alignment;
    alloc.size = size;
    alloc.region_offset = &base_offset;
    r = drmCommandWriteRead(bo->bom->fd,
                            DRM_RADEON_ALLOC,
                            &alloc,
                            sizeof(alloc));
    if (r) {
        /* ptr is set to NULL if dma allocation failed */
        bo_legacy->ptr = NULL;
        exit(0);
        return r;
    }
    bo_legacy->ptr = boml->screen->gartTextures.map + base_offset;
    bo_legacy->offset = boml->screen->gart_texture_offset + base_offset;
    bo->size = size;
    boml->dma_alloc_size += size;
    return 0;
}

static int bo_dma_free(struct radeon_bo *bo)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    drm_radeon_mem_free_t memfree;
    int r;

    if (bo_legacy->ptr == NULL) {
        /* ptr is set to NULL if dma allocation failed */
        return 0;
    }
    legacy_get_current_age(boml);
    memfree.region = RADEON_MEM_REGION_GART;
    memfree.region_offset  = bo_legacy->offset;
    memfree.region_offset -= boml->screen->gart_texture_offset;
    r = drmCommandWrite(boml->base.fd,
                        DRM_RADEON_FREE,
                        &memfree,
                        sizeof(memfree));
    if (r) {
        fprintf(stderr, "Failed to free bo[%p] at %08x\n",
                &bo_legacy->base, memfree.region_offset);
        fprintf(stderr, "ret = %s\n", strerror(-r));
        return r;
    }
    boml->dma_alloc_size -= bo_legacy->base.size;
    return 0;
}

static void bo_free(struct bo_legacy *bo_legacy)
{
    struct bo_manager_legacy *boml;

    if (bo_legacy == NULL) {
        return;
    }
    boml = (struct bo_manager_legacy *)bo_legacy->base.bom;
    bo_legacy->prev->next = bo_legacy->next;
    if (bo_legacy->next) {
        bo_legacy->next->prev = bo_legacy->prev;
    }
    if (!bo_legacy->static_bo) {
        legacy_free_handle(boml, bo_legacy->base.handle);
        if (bo_legacy->base.flags & RADEON_GEM_DOMAIN_GTT) {
            /* dma buffers */
            bo_dma_free(&bo_legacy->base);
        } else {
            /* free backing store */
            free(bo_legacy->ptr);
        }
    }
    memset(bo_legacy, 0 , sizeof(struct bo_legacy));
    free(bo_legacy);
}

static struct radeon_bo *bo_open(struct radeon_bo_manager *bom,
                                 uint32_t handle,
                                 uint32_t size,
                                 uint32_t alignment,
                                 uint32_t flags)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bom;
    struct bo_legacy *bo_legacy;
    int r;

    if (handle) {
        bo_legacy = boml->bos.next;
        while (bo_legacy) {
            if (bo_legacy->base.handle == handle) {
                radeon_bo_ref(&(bo_legacy->base));
                return (struct radeon_bo*)bo_legacy;
            }
            bo_legacy = bo_legacy->next;
        }
        return NULL;
    }

    bo_legacy = bo_allocate(boml, size, alignment, flags);
    bo_legacy->static_bo = 0;
    r = legacy_new_handle(boml, &bo_legacy->base.handle);
    if (r) {
        bo_free(bo_legacy);
        return NULL;
    }
    if (bo_legacy->base.flags & RADEON_GEM_DOMAIN_GTT) {
        legacy_track_pending(boml);
        /* dma buffers */
        r = bo_dma_alloc(&(bo_legacy->base));
        if (r) {
            fprintf(stderr, "Ran out of GART memory (for %d)!\n", size);
            fprintf(stderr, "Please consider adjusting GARTSize option.\n");
            bo_free(bo_legacy);
            exit(-1);
            return NULL;
        }
    } else {
        bo_legacy->ptr = malloc(bo_legacy->base.size);
        if (bo_legacy->ptr == NULL) {
            bo_free(bo_legacy);
            return NULL;
        }
    }
    radeon_bo_ref(&(bo_legacy->base));
    return (struct radeon_bo*)bo_legacy;
}

static void bo_ref(struct radeon_bo *bo)
{
}

static void bo_unref(struct radeon_bo *bo)
{
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    if (bo->cref <= 0) {
        bo_legacy->prev->next = bo_legacy->next;
        if (bo_legacy->next) {
            bo_legacy->next->prev = bo_legacy->prev;
        }
        if (!bo_legacy->is_pending) {
            bo_free(bo_legacy);
        }
    }
}

static int bo_map(struct radeon_bo *bo, int write)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    
    legacy_wait_pending(bo);
    bo_legacy->validated = 0;
    bo_legacy->dirty = 1;
    bo_legacy->map_count++;
    bo->ptr = bo_legacy->ptr;
    /* Read the first pixel in the frame buffer.  This should
     * be a noop, right?  In fact without this conform fails as reading
     * from the framebuffer sometimes produces old results -- the
     * on-card read cache gets mixed up and doesn't notice that the
     * framebuffer has been updated.
     *
     * Note that we should probably be reading some otherwise unused
     * region of VRAM, otherwise we might get incorrect results when
     * reading pixels from the top left of the screen.
     *
     * I found this problem on an R420 with glean's texCube test.
     * Note that the R200 span code also *writes* the first pixel in the
     * framebuffer, but I've found this to be unnecessary.
     *  -- Nicolai Hähnle, June 2008
     */
    {
        int p;
        volatile int *buf = (int*)boml->screen->driScreen->pFB;
        p = *buf;
    }

    return 0;
}

static int bo_unmap(struct radeon_bo *bo)
{
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    if (--bo_legacy->map_count > 0) {
        return 0;
    }
    bo->ptr = NULL;
    return 0;
}

static struct radeon_bo_funcs bo_legacy_funcs = {
    bo_open,
    bo_ref,
    bo_unref,
    bo_map,
    bo_unmap
};

static int bo_vram_validate(struct radeon_bo *bo,
                            uint32_t *soffset,
                            uint32_t *eoffset)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    int r;
    
    if (!bo_legacy->got_dri_texture_obj) {
        make_empty_list(&bo_legacy->dri_texture_obj);
        bo_legacy->dri_texture_obj.totalSize = bo->size;
        r = driAllocateTexture(&boml->texture_heap, 1,
                               &bo_legacy->dri_texture_obj);
        if (r) {
            uint8_t *segfault=NULL;
            fprintf(stderr, "Ouch! vram_validate failed %d\n", r);
            *segfault=1;
            return -1;
        }
        bo_legacy->offset = boml->texture_offset +
                            bo_legacy->dri_texture_obj.memBlock->ofs;
        bo_legacy->got_dri_texture_obj = 1;
        bo_legacy->dirty = 1;
    }
    if (bo_legacy->dirty) {
        /* Copy to VRAM using a blit.
         * All memory is 4K aligned. We're using 1024 pixels wide blits.
         */
        drm_radeon_texture_t tex;
        drm_radeon_tex_image_t tmp;
        int ret;

        tex.offset = bo_legacy->offset;
        tex.image = &tmp;
        assert(!(tex.offset & 1023));

        tmp.x = 0;
        tmp.y = 0;
        if (bo->size < 4096) {
            tmp.width = (bo->size + 3) / 4;
            tmp.height = 1;
        } else {
            tmp.width = 1024;
            tmp.height = (bo->size + 4095) / 4096;
        }
        tmp.data = bo_legacy->ptr;
        tex.format = RADEON_TXFORMAT_ARGB8888;
        tex.width = tmp.width;
        tex.height = tmp.height;
        tex.pitch = MAX2(tmp.width / 16, 1);
        do {
            ret = drmCommandWriteRead(bo->bom->fd,
                                      DRM_RADEON_TEXTURE,
                                      &tex,
                                      sizeof(drm_radeon_texture_t));
            if (ret) {
                if (RADEON_DEBUG & DEBUG_IOCTL)
                    fprintf(stderr, "DRM_RADEON_TEXTURE:  again!\n");
                usleep(1);
            }
        } while (ret == -EAGAIN);
        bo_legacy->dirty = 0;
    }
    return 0;
}

int radeon_bo_legacy_validate(struct radeon_bo *bo,
                              uint32_t *soffset,
                              uint32_t *eoffset)
{
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    int r;

    if (bo_legacy->map_count) {
        fprintf(stderr, "bo(%p, %d) is mapped (%d) can't valide it.\n",
                bo, bo->size, bo_legacy->map_count);
        return -EINVAL;
    }
    if (bo_legacy->static_bo || bo_legacy->validated) {
        *soffset = bo_legacy->offset;
        *eoffset = bo_legacy->offset + bo->size;
        return 0;
    }
    if (!(bo->flags & RADEON_GEM_DOMAIN_GTT)) {
        r = bo_vram_validate(bo, soffset, eoffset);
        if (r) {
            return r;
        }
    }
    *soffset = bo_legacy->offset;
    *eoffset = bo_legacy->offset + bo->size;
    bo_legacy->validated = 1;
    return 0;
}

void radeon_bo_legacy_pending(struct radeon_bo *bo, uint32_t pending)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    bo_legacy->pending = pending;
    bo_legacy->is_pending += 1;
    /* add to pending list */
    radeon_bo_ref(bo);
    if (bo_legacy->is_pending > 1) {
        return;    
    }
    bo_legacy->pprev = boml->pending_bos.pprev;
    bo_legacy->pnext = NULL;
    bo_legacy->pprev->pnext = bo_legacy;
    boml->pending_bos.pprev = bo_legacy;
    boml->cpendings++;
}

void radeon_bo_manager_legacy_shutdown(struct radeon_bo_manager *bom)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bom;
    struct bo_legacy *bo_legacy;

    if (bom == NULL) {
        return;
    }
    bo_legacy = boml->bos.next;
    while (bo_legacy) {
        struct bo_legacy *next;

        next = bo_legacy->next;
        bo_free(bo_legacy);
        bo_legacy = next;
    }
    free(boml->free_handles);
    free(boml);
}

struct radeon_bo_manager *radeon_bo_manager_legacy(struct radeon_screen *scrn)
{
    struct bo_manager_legacy *bom;
    struct bo_legacy *bo;
    unsigned size;

    bom = (struct bo_manager_legacy*)
          calloc(1, sizeof(struct bo_manager_legacy));
    if (bom == NULL) {
        return NULL;
    }

    bom->texture_heap = driCreateTextureHeap(0,
                                             bom,
                                             scrn->texSize[0],
                                             12,
                                             RADEON_NR_TEX_REGIONS,
                                             (drmTextureRegionPtr)scrn->sarea->tex_list[0],
                                             &scrn->sarea->tex_age[0],
                                             &bom->texture_swapped,
                                             sizeof(struct bo_legacy),
                                             &bo_legacy_tobj_destroy);
    bom->texture_offset = scrn->texOffset[0];

    bom->base.funcs = &bo_legacy_funcs;
    bom->base.fd = scrn->driScreen->fd;
    bom->bos.next = NULL;
    bom->bos.prev = NULL;
    bom->pending_bos.pprev = &bom->pending_bos;
    bom->pending_bos.pnext = NULL;
    bom->screen = scrn;
    bom->fb_location = scrn->fbLocation;
    bom->nhandle = 1;
    bom->cfree_handles = 0;
    bom->nfree_handles = 0x400;
    bom->free_handles = (uint32_t*)malloc(bom->nfree_handles * 4);
    if (bom->free_handles == NULL) {
        radeon_bo_manager_legacy_shutdown((struct radeon_bo_manager*)bom);
        return NULL;
    }

    /* biggest framebuffer size */
    size = 4096*4096*4; 
    /* allocate front */
    bo = bo_allocate(bom, size, 0, 0);
    if (bo == NULL) {
        radeon_bo_manager_legacy_shutdown((struct radeon_bo_manager*)bom);
        return NULL;
    }
    if (scrn->sarea->tiling_enabled) {
        bo->base.flags = RADEON_BO_FLAGS_MACRO_TILE;
    }
    bo->static_bo = 1;
    bo->offset = bom->screen->frontOffset + bom->fb_location;
    bo->base.handle = bo->offset;
    bo->ptr = scrn->driScreen->pFB + bom->screen->frontOffset;
    if (bo->base.handle > bom->nhandle) {
        bom->nhandle = bo->base.handle + 1;
    }
    /* allocate back */
    bo = bo_allocate(bom, size, 0, 0);
    if (bo == NULL) {
        radeon_bo_manager_legacy_shutdown((struct radeon_bo_manager*)bom);
        return NULL;
    }
    if (scrn->sarea->tiling_enabled) {
        bo->base.flags = RADEON_BO_FLAGS_MACRO_TILE;
    }
    bo->static_bo = 1;
    bo->offset = bom->screen->backOffset + bom->fb_location;
    bo->base.handle = bo->offset;
    bo->ptr = scrn->driScreen->pFB + bom->screen->backOffset;
    if (bo->base.handle > bom->nhandle) {
        bom->nhandle = bo->base.handle + 1;
    }
    /* allocate depth */
    bo = bo_allocate(bom, size, 0, 0);
    if (bo == NULL) {
        radeon_bo_manager_legacy_shutdown((struct radeon_bo_manager*)bom);
        return NULL;
    }
    bo->base.flags = 0;
    if (scrn->sarea->tiling_enabled) {
        bo->base.flags = RADEON_BO_FLAGS_MACRO_TILE;
    }
    bo->static_bo = 1;
    bo->offset = bom->screen->depthOffset + bom->fb_location;
    bo->base.handle = bo->offset;
    bo->ptr = scrn->driScreen->pFB + bom->screen->depthOffset;
    if (bo->base.handle > bom->nhandle) {
        bom->nhandle = bo->base.handle + 1;
    }
    return (struct radeon_bo_manager*)bom;
}

void radeon_bo_legacy_texture_age(struct radeon_bo_manager *bom)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bom;
    DRI_AGE_TEXTURES(boml->texture_heap);
}

unsigned radeon_bo_legacy_relocs_size(struct radeon_bo *bo)
{
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    if (bo_legacy->static_bo || (bo->flags & RADEON_GEM_DOMAIN_GTT)) {
        return 0;
    }
    return bo->size;
}
