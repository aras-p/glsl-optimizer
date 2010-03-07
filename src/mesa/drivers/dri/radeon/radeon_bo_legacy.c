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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "xf86drm.h"
#include "texmem.h"
#include "main/simple_list.h"

#include "drm.h"
#include "radeon_drm.h"
#include "radeon_common.h"
#include "radeon_bocs_wrapper.h"
#include "radeon_macros.h"

#ifdef HAVE_LIBDRM_RADEON
#include "radeon_bo_int.h"
#else
#include "radeon_bo_int_drm.h"
#endif

/* no seriously texmem.c is this screwed up */
struct bo_legacy_texture_object {
    driTextureObject    base;
    struct bo_legacy *parent;
};

struct bo_legacy {
    struct radeon_bo_int    base;
    int                 map_count;
    uint32_t            pending;
    int                 is_pending;
    int                 static_bo;
    uint32_t            offset;
    struct bo_legacy_texture_object *tobj;
    int                 validated;
    int                 dirty;
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
    uint32_t                    dma_buf_count;
    unsigned                    cpendings;
    driTextureObject            texture_swapped;
    driTexHeap                  *texture_heap;
    struct radeon_screen        *screen;
    unsigned                    *free_handles;
};

static void bo_legacy_tobj_destroy(void *data, driTextureObject *t)
{
    struct bo_legacy_texture_object *tobj = (struct bo_legacy_texture_object *)t;
    
    if (tobj->parent) {
        tobj->parent->tobj = NULL;
        tobj->parent->validated = 0;
    }
}

static void inline clean_handles(struct bo_manager_legacy *bom)
{
  while (bom->cfree_handles > 0 &&
	 !bom->free_handles[bom->cfree_handles - 1])
    bom->cfree_handles--;

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
	clean_handles(bom);
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
        clean_handles(bom);
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
    unsigned char *RADEONMMIO = NULL;
    int r;

    if (   IS_R300_CLASS(boml->screen) 
        || IS_R600_CLASS(boml->screen) ) 
    {
    	gp.param = RADEON_PARAM_LAST_CLEAR;
    	gp.value = (int *)&boml->current_age;
    	r = drmCommandWriteRead(boml->base.fd, DRM_RADEON_GETPARAM,
       	                     &gp, sizeof(gp));
    	if (r) {
       	 fprintf(stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__, r);
         exit(1);
       }
    } 
    else {
        RADEONMMIO = boml->screen->mmio.map;
        boml->current_age = boml->screen->scratch[3];
        boml->current_age = INREG(RADEON_GUI_SCRATCH_REG3);
    }
}

static int legacy_is_pending(struct radeon_bo_int *boi)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)boi->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)boi;

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
	assert(bo_legacy->is_pending <= boi->cref);
        while (bo_legacy->is_pending--) {
	    boi = (struct radeon_bo_int *)radeon_bo_unref((struct radeon_bo *)boi);
	    if (!boi)
	      break;
        }
	if (boi)
	  bo_legacy->is_pending = 0;
        boml->cpendings--;
        return 0;
    }
    return 1;
}

static int legacy_wait_pending(struct radeon_bo_int *bo)
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

void legacy_track_pending(struct radeon_bo_manager *bom, int debug)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy*) bom;
    struct bo_legacy *bo_legacy;
    struct bo_legacy *next;

    legacy_get_current_age(boml);
    bo_legacy = boml->pending_bos.pnext;
    while (bo_legacy) {
        if (debug)
            fprintf(stderr,"pending %p %d %d %d\n", bo_legacy, bo_legacy->base.size,
                    boml->current_age, bo_legacy->pending);
        next = bo_legacy->pnext;
        if (legacy_is_pending(&(bo_legacy->base))) {
        }
        bo_legacy = next;
    } 
}

static int legacy_wait_any_pending(struct bo_manager_legacy *boml)
{
    struct bo_legacy *bo_legacy;

    legacy_get_current_age(boml);
    bo_legacy = boml->pending_bos.pnext;
    if (!bo_legacy)
      return -1;
    legacy_wait_pending(&bo_legacy->base);
    return 0;
}

static void legacy_kick_all_buffers(struct bo_manager_legacy *boml)
{
    struct bo_legacy *legacy;

    legacy = boml->bos.next;
    while (legacy != &boml->bos) {
	if (legacy->tobj) {
	    if (legacy->validated) {
		driDestroyTextureObject(&legacy->tobj->base);
		legacy->tobj = 0;
		legacy->validated = 0;
	    }
	}
	legacy = legacy->next;
    }
}

static struct bo_legacy *bo_allocate(struct bo_manager_legacy *boml,
                                     uint32_t size,
                                     uint32_t alignment,
                                     uint32_t domains,
                                     uint32_t flags)
{
    struct bo_legacy *bo_legacy;
    static int pgsize;

    if (pgsize == 0)
        pgsize = getpagesize() - 1;

    size = (size + pgsize) & ~pgsize;

    bo_legacy = (struct bo_legacy*)calloc(1, sizeof(struct bo_legacy));
    if (bo_legacy == NULL) {
        return NULL;
    }
    bo_legacy->base.bom = (struct radeon_bo_manager*)boml;
    bo_legacy->base.handle = 0;
    bo_legacy->base.size = size;
    bo_legacy->base.alignment = alignment;
    bo_legacy->base.domains = domains;
    bo_legacy->base.flags = flags;
    bo_legacy->base.ptr = NULL;
    bo_legacy->map_count = 0;
    bo_legacy->next = NULL;
    bo_legacy->prev = NULL;
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

static int bo_dma_alloc(struct radeon_bo_int *bo)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    drm_radeon_mem_alloc_t alloc;
    unsigned size;
    int base_offset;
    int r;

    /* align size on 4Kb */
    size = (((4 * 1024) - 1) + bo_legacy->base.size) & ~((4 * 1024) - 1);
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
        return r;
    }
    bo_legacy->ptr = boml->screen->gartTextures.map + base_offset;
    bo_legacy->offset = boml->screen->gart_texture_offset + base_offset;
    bo->size = size;
    boml->dma_alloc_size += size;
    boml->dma_buf_count++;
    return 0;
}

static int bo_dma_free(struct radeon_bo_int *bo)
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
    boml->dma_buf_count--;
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
        if (bo_legacy->base.domains & RADEON_GEM_DOMAIN_GTT) {
            /* dma buffers */
            bo_dma_free(&bo_legacy->base);
        } else {
  	    driDestroyTextureObject(&bo_legacy->tobj->base);
	    bo_legacy->tobj = NULL;
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
                                 uint32_t domains,
                                 uint32_t flags)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bom;
    struct bo_legacy *bo_legacy;
    int r;

    if (handle) {
        bo_legacy = boml->bos.next;
        while (bo_legacy) {
            if (bo_legacy->base.handle == handle) {
                radeon_bo_ref((struct radeon_bo *)&(bo_legacy->base));
                return (struct radeon_bo*)bo_legacy;
            }
            bo_legacy = bo_legacy->next;
        }
        return NULL;
    }
    bo_legacy = bo_allocate(boml, size, alignment, domains, flags);
    bo_legacy->static_bo = 0;
    r = legacy_new_handle(boml, &bo_legacy->base.handle);
    if (r) {
        bo_free(bo_legacy);
        return NULL;
    }
    if (bo_legacy->base.domains & RADEON_GEM_DOMAIN_GTT) 
    {
retry:
        legacy_track_pending(&boml->base, 0);
        /* dma buffers */

        r = bo_dma_alloc(&(bo_legacy->base));
        if (r) 
        {
	         if (legacy_wait_any_pending(boml) == -1) 
             {
                  bo_free(bo_legacy);
	              return NULL;
             }
	         goto retry;
	         return NULL;
        }
    } 
    else 
    {
        bo_legacy->ptr = malloc(bo_legacy->base.size);
        if (bo_legacy->ptr == NULL) {
            bo_free(bo_legacy);
            return NULL;
        }
    }
    radeon_bo_ref((struct radeon_bo *)&(bo_legacy->base));

    return (struct radeon_bo*)bo_legacy;
}

static void bo_ref(struct radeon_bo_int *bo)
{
}

static struct radeon_bo *bo_unref(struct radeon_bo_int *boi)
{
    struct bo_legacy *bo_legacy = (struct bo_legacy*)boi;

    if (boi->cref <= 0) {
        bo_legacy->prev->next = bo_legacy->next;
        if (bo_legacy->next) {
            bo_legacy->next->prev = bo_legacy->prev;
        }
        if (!bo_legacy->is_pending) {
            bo_free(bo_legacy);
        }
        return NULL;
    }
    return (struct radeon_bo *)boi;
}

static int bo_map(struct radeon_bo_int *bo, int write)
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
    if (!(bo->domains & RADEON_GEM_DOMAIN_GTT)) {
        int p;
        volatile int *buf = (int*)boml->screen->driScreen->pFB;
        p = *buf;
    }

    return 0;
}

static int bo_unmap(struct radeon_bo_int *bo)
{
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    if (--bo_legacy->map_count > 0) 
    {
        return 0;
    }
    
    bo->ptr = NULL;

    return 0;
}

static int bo_is_busy(struct radeon_bo_int *bo, uint32_t *domain)
{
    *domain = 0;
    if (bo->domains & RADEON_GEM_DOMAIN_GTT)
        *domain = RADEON_GEM_DOMAIN_GTT;
    else
        *domain = RADEON_GEM_DOMAIN_CPU;
    if (legacy_is_pending(bo))
        return -EBUSY;
    else
        return 0;
}

static int bo_is_static(struct radeon_bo_int *bo)
{
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    return bo_legacy->static_bo;
}

static struct radeon_bo_funcs bo_legacy_funcs = {
    bo_open,
    bo_ref,
    bo_unref,
    bo_map,
    bo_unmap,
    NULL,
    bo_is_static,
    NULL,
    NULL,
    bo_is_busy
};

static int bo_vram_validate(struct radeon_bo_int *bo,
                            uint32_t *soffset,
                            uint32_t *eoffset)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bo->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    int r;
    int retry_count = 0, pending_retry = 0;
    
    if (!bo_legacy->tobj) {
	bo_legacy->tobj = CALLOC(sizeof(struct bo_legacy_texture_object));
	bo_legacy->tobj->parent = bo_legacy;
	make_empty_list(&bo_legacy->tobj->base);
	bo_legacy->tobj->base.totalSize = bo->size;
    retry:
        r = driAllocateTexture(&boml->texture_heap, 1,
                               &bo_legacy->tobj->base);
        if (r) {
		pending_retry = 0;
		while(boml->cpendings && pending_retry++ < 10000) {
			legacy_track_pending(&boml->base, 0);
			retry_count++;
			if (retry_count > 2) {
				free(bo_legacy->tobj);
				bo_legacy->tobj = NULL;
				fprintf(stderr, "Ouch! vram_validate failed %d\n", r);
				return -1;
			}
			goto retry;
		}
	}
        bo_legacy->offset = boml->texture_offset +
                            bo_legacy->tobj->base.memBlock->ofs;
        bo_legacy->dirty = 1;
    }

    assert(bo_legacy->tobj->base.memBlock);

    if (bo_legacy->tobj)
	driUpdateTextureLRU(&bo_legacy->tobj->base);

    if (bo_legacy->dirty || bo_legacy->tobj->base.dirty_images[0]) {
	    if (IS_R600_CLASS(boml->screen)) {
		    drm_radeon_texture_t tex;
		    drm_radeon_tex_image_t tmp;
		    int ret;

		    tex.offset = bo_legacy->offset;
		    tex.image = &tmp;
		    assert(!(tex.offset & 1023));

		    tmp.x = 0;
		    tmp.y = 0;
		    tmp.width = bo->size;
		    tmp.height = 1;
		    tmp.data = bo_legacy->ptr;
		    tex.format = RADEON_TXFORMAT_ARGB8888;
		    tex.width = tmp.width;
		    tex.height = tmp.height;
		    tex.pitch = bo->size;
		    do {
			    ret = drmCommandWriteRead(bo->bom->fd,
						      DRM_RADEON_TEXTURE,
						      &tex,
						      sizeof(drm_radeon_texture_t));
			    if (ret) {
				    if (RADEON_DEBUG & RADEON_IOCTL)
					    fprintf(stderr, "DRM_RADEON_TEXTURE:  again!\n");
				    usleep(1);
			    }
		    } while (ret == -EAGAIN);
	    } else {
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
				    if (RADEON_DEBUG & RADEON_IOCTL)
					    fprintf(stderr, "DRM_RADEON_TEXTURE:  again!\n");
				    usleep(1);
			    }
		    } while (ret == -EAGAIN);
	    }
	    bo_legacy->dirty = 0;
	    bo_legacy->tobj->base.dirty_images[0] = 0;
    }
    return 0;
}

/* 
 *  radeon_bo_legacy_validate -
 *  returns:
 *  0 - all good
 *  -EINVAL - mapped buffer can't be validated
 *  -EAGAIN - restart validation we've kicked all the buffers out
 */
int radeon_bo_legacy_validate(struct radeon_bo *bo,
                              uint32_t *soffset,
                              uint32_t *eoffset)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)boi->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;
    int r;
    int retries = 0;

    if (bo_legacy->map_count) {
        fprintf(stderr, "bo(%p, %d) is mapped (%d) can't valide it.\n",
                bo, boi->size, bo_legacy->map_count);
        return -EINVAL;
    }
    if(boi->size == 0) {
        fprintf(stderr, "bo(%p) has size 0.\n", bo);
        return -EINVAL;
    }
    if (bo_legacy->static_bo || bo_legacy->validated) {
        *soffset = bo_legacy->offset;
        *eoffset = bo_legacy->offset + boi->size;

        return 0;
    }
    if (!(boi->domains & RADEON_GEM_DOMAIN_GTT)) {

        r = bo_vram_validate(boi, soffset, eoffset);
        if (r) {
	    legacy_track_pending(&boml->base, 0);
	    legacy_kick_all_buffers(boml);
	    retries++;
	    if (retries == 2) {
		fprintf(stderr,"legacy bo: failed to get relocations into aperture\n");
		assert(0);
		exit(-1);
	    }
	    return -EAGAIN;
        }
    }
    *soffset = bo_legacy->offset;
    *eoffset = bo_legacy->offset + boi->size;
    bo_legacy->validated = 1;

    return 0;
}

void radeon_bo_legacy_pending(struct radeon_bo *bo, uint32_t pending)
{
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)boi->bom;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    bo_legacy->pending = pending;
    bo_legacy->is_pending++;
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

void radeon_bo_manager_legacy_dtor(struct radeon_bo_manager *bom)
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
    driDestroyTextureHeap(boml->texture_heap);
    free(boml->free_handles);
    free(boml);
}

static struct bo_legacy *radeon_legacy_bo_alloc_static(struct bo_manager_legacy *bom,
						       int size,
						       uint32_t offset)
{
    struct bo_legacy *bo;

    bo = bo_allocate(bom, size, 0, RADEON_GEM_DOMAIN_VRAM, 0);

    if (bo == NULL)
	return NULL;
    bo->static_bo = 1;
    bo->offset = offset + bom->fb_location;
    bo->base.handle = bo->offset;
    bo->ptr = bom->screen->driScreen->pFB + offset;
    if (bo->base.handle > bom->nhandle) {
        bom->nhandle = bo->base.handle + 1;
    }
    radeon_bo_ref((struct radeon_bo *)&(bo->base));
    return bo;
}

struct radeon_bo_manager *radeon_bo_manager_legacy_ctor(struct radeon_screen *scrn)
{
    struct bo_manager_legacy *bom;
    struct bo_legacy *bo;
    unsigned size;

    bom = (struct bo_manager_legacy*)
          calloc(1, sizeof(struct bo_manager_legacy));
    if (bom == NULL) {
        return NULL;
    }

    make_empty_list(&bom->texture_swapped);

    bom->texture_heap = driCreateTextureHeap(0,
                                             bom,
                                             scrn->texSize[0],
                                             12,
                                             RADEON_NR_TEX_REGIONS,
                                             (drmTextureRegionPtr)scrn->sarea->tex_list[0],
                                             &scrn->sarea->tex_age[0],
                                             &bom->texture_swapped,
                                             sizeof(struct bo_legacy_texture_object),
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
        radeon_bo_manager_legacy_dtor((struct radeon_bo_manager*)bom);
        return NULL;
    }

    /* biggest framebuffer size */
    size = 4096*4096*4; 

    /* allocate front */
    bo = radeon_legacy_bo_alloc_static(bom, size, bom->screen->frontOffset);

    if (!bo) {
        radeon_bo_manager_legacy_dtor((struct radeon_bo_manager*)bom);
        return NULL;
    }
    if (scrn->sarea->tiling_enabled) {
        bo->base.flags = RADEON_BO_FLAGS_MACRO_TILE;
    }

    /* allocate back */
    bo = radeon_legacy_bo_alloc_static(bom, size, bom->screen->backOffset);

    if (!bo) {
        radeon_bo_manager_legacy_dtor((struct radeon_bo_manager*)bom);
        return NULL;
    }
    if (scrn->sarea->tiling_enabled) {
        bo->base.flags = RADEON_BO_FLAGS_MACRO_TILE;
    }

    /* allocate depth */
    bo = radeon_legacy_bo_alloc_static(bom, size, bom->screen->depthOffset);

    if (!bo) {
        radeon_bo_manager_legacy_dtor((struct radeon_bo_manager*)bom);
        return NULL;
    }
    bo->base.flags = 0;
    if (scrn->sarea->tiling_enabled) {
        bo->base.flags |= RADEON_BO_FLAGS_MACRO_TILE;
        bo->base.flags |= RADEON_BO_FLAGS_MICRO_TILE;
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
    struct radeon_bo_int *boi = (struct radeon_bo_int *)bo;
    struct bo_legacy *bo_legacy = (struct bo_legacy*)bo;

    if (bo_legacy->static_bo || (boi->domains & RADEON_GEM_DOMAIN_GTT)) {
        return 0;
    }
    return boi->size;
}

/*
 * Fake up a bo for things like texture image_override.
 * bo->offset already includes fb_location
 */
struct radeon_bo *radeon_legacy_bo_alloc_fake(struct radeon_bo_manager *bom,
					      int size,
	                                      uint32_t offset)
{
    struct bo_manager_legacy *boml = (struct bo_manager_legacy *)bom;
    struct bo_legacy *bo;

    bo = bo_allocate(boml, size, 0, RADEON_GEM_DOMAIN_VRAM, 0);

    if (bo == NULL)
	return NULL;
    bo->static_bo = 1;
    bo->offset = offset;
    bo->base.handle = bo->offset;
    bo->ptr = boml->screen->driScreen->pFB + (offset - boml->fb_location);
    if (bo->base.handle > boml->nhandle) {
        boml->nhandle = bo->base.handle + 1;
    }
    radeon_bo_ref((struct radeon_bo *)&(bo->base));
    return (struct radeon_bo *)&(bo->base);
}

