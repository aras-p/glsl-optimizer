/*
 * Copyright © 2008 Jérôme Glisse
 * Copyright © 2010 Marek Olšák <maraeo@gmail.com>
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
 *      Marek Olšák <maraeo@gmail.com>
 *
 * Based on work from libdrm_radeon by:
 *      Aapo Tahkola <aet@rasterburn.org>
 *      Nicolai Haehnle <prefect_@gmx.net>
 *      Jérôme Glisse <glisse@freedesktop.org>
 */

/*
    This file replaces libdrm's radeon_cs_gem with our own implemention.
    It's optimized specifically for Radeon DRM.
    Reloc writes and space checking are faster and simpler than their
    counterparts in libdrm (the time complexity of all the functions
    is O(1) in nearly all scenarios, thanks to hashing).

    It works like this:

    cs_add_reloc(cs, buf, read_domain, write_domain) adds a new relocation and
    also adds the size of 'buf' to the used_gart and used_vram winsys variables
    based on the domains, which are simply or'd for the accounting purposes.
    The adding is skipped if the reloc is already present in the list, but it
    accounts any newly-referenced domains.

    cs_validate is then called, which just checks:
        used_vram/gart < vram/gart_size * 0.8
    The 0.8 number allows for some memory fragmentation. If the validation
    fails, the pipe driver flushes CS and tries do the validation again,
    i.e. it validates only that one operation. If it fails again, it drops
    the operation on the floor and prints some nasty message to stderr.
    (done in the pipe driver)

    cs_write_reloc(cs, buf) just writes a reloc that has been added using
    cs_add_reloc. The read_domain and write_domain parameters have been removed,
    because we already specify them in cs_add_reloc.
*/

#include "radeon_drm_cs.h"

#include "util/u_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xf86drm.h>

/*
 * this are copy from radeon_drm, once an updated libdrm is released
 * we should bump configure.ac requirement for it and remove the following
 * field
 */
#ifndef RADEON_CHUNK_ID_FLAGS
#define RADEON_CHUNK_ID_FLAGS       0x03

/* The first dword of RADEON_CHUNK_ID_FLAGS is a uint32 of these flags: */
#define RADEON_CS_KEEP_TILING_FLAGS 0x01
#endif

#ifndef RADEON_CS_USE_VM
#define RADEON_CS_USE_VM            0x02
/* The second dword of RADEON_CHUNK_ID_FLAGS is a uint32 that sets the ring type */
#define RADEON_CS_RING_GFX          0
#define RADEON_CS_RING_COMPUTE      1
#endif

#ifndef RADEON_CS_RING_DMA
#define RADEON_CS_RING_DMA          2
#endif

#ifndef RADEON_CS_END_OF_FRAME
#define RADEON_CS_END_OF_FRAME      0x04
#endif


#define RELOC_DWORDS (sizeof(struct drm_radeon_cs_reloc) / sizeof(uint32_t))

static boolean radeon_init_cs_context(struct radeon_cs_context *csc,
                                      struct radeon_drm_winsys *ws)
{
    csc->fd = ws->fd;
    csc->nrelocs = 512;
    csc->relocs_bo = (struct radeon_bo**)
                     CALLOC(1, csc->nrelocs * sizeof(struct radeon_bo*));
    if (!csc->relocs_bo) {
        return FALSE;
    }

    csc->relocs = (struct drm_radeon_cs_reloc*)
                  CALLOC(1, csc->nrelocs * sizeof(struct drm_radeon_cs_reloc));
    if (!csc->relocs) {
        FREE(csc->relocs_bo);
        return FALSE;
    }

    csc->chunks[0].chunk_id = RADEON_CHUNK_ID_IB;
    csc->chunks[0].length_dw = 0;
    csc->chunks[0].chunk_data = (uint64_t)(uintptr_t)csc->buf;
    csc->chunks[1].chunk_id = RADEON_CHUNK_ID_RELOCS;
    csc->chunks[1].length_dw = 0;
    csc->chunks[1].chunk_data = (uint64_t)(uintptr_t)csc->relocs;
    csc->chunks[2].chunk_id = RADEON_CHUNK_ID_FLAGS;
    csc->chunks[2].length_dw = 2;
    csc->chunks[2].chunk_data = (uint64_t)(uintptr_t)&csc->flags;

    csc->chunk_array[0] = (uint64_t)(uintptr_t)&csc->chunks[0];
    csc->chunk_array[1] = (uint64_t)(uintptr_t)&csc->chunks[1];
    csc->chunk_array[2] = (uint64_t)(uintptr_t)&csc->chunks[2];

    csc->cs.chunks = (uint64_t)(uintptr_t)csc->chunk_array;
    return TRUE;
}

static void radeon_cs_context_cleanup(struct radeon_cs_context *csc)
{
    unsigned i;

    for (i = 0; i < csc->crelocs; i++) {
        p_atomic_dec(&csc->relocs_bo[i]->num_cs_references);
        radeon_bo_reference(&csc->relocs_bo[i], NULL);
    }

    csc->crelocs = 0;
    csc->validated_crelocs = 0;
    csc->chunks[0].length_dw = 0;
    csc->chunks[1].length_dw = 0;
    csc->used_gart = 0;
    csc->used_vram = 0;
    memset(csc->is_handle_added, 0, sizeof(csc->is_handle_added));
}

static void radeon_destroy_cs_context(struct radeon_cs_context *csc)
{
    radeon_cs_context_cleanup(csc);
    FREE(csc->relocs_bo);
    FREE(csc->relocs);
}


static struct radeon_winsys_cs *radeon_drm_cs_create(struct radeon_winsys *rws, enum ring_type ring_type)
{
    struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
    struct radeon_drm_cs *cs;

    cs = CALLOC_STRUCT(radeon_drm_cs);
    if (!cs) {
        return NULL;
    }
    pipe_semaphore_init(&cs->flush_completed, 0);

    cs->ws = ws;

    if (!radeon_init_cs_context(&cs->csc1, cs->ws)) {
        FREE(cs);
        return NULL;
    }
    if (!radeon_init_cs_context(&cs->csc2, cs->ws)) {
        radeon_destroy_cs_context(&cs->csc1);
        FREE(cs);
        return NULL;
    }

    /* Set the first command buffer as current. */
    cs->csc = &cs->csc1;
    cs->cst = &cs->csc2;
    cs->base.buf = cs->csc->buf;
    cs->base.ring_type = ring_type;

    p_atomic_inc(&ws->num_cs);
    return &cs->base;
}

#define OUT_CS(cs, value) (cs)->buf[(cs)->cdw++] = (value)

static INLINE void update_reloc_domains(struct drm_radeon_cs_reloc *reloc,
                                        enum radeon_bo_domain rd,
                                        enum radeon_bo_domain wd,
                                        enum radeon_bo_domain *added_domains)
{
    *added_domains = (rd | wd) & ~(reloc->read_domains | reloc->write_domain);

    reloc->read_domains |= rd;
    reloc->write_domain |= wd;
}

int radeon_get_reloc(struct radeon_cs_context *csc, struct radeon_bo *bo)
{
    struct drm_radeon_cs_reloc *reloc;
    unsigned i;
    unsigned hash = bo->handle & (sizeof(csc->is_handle_added)-1);

    if (csc->is_handle_added[hash]) {
        i = csc->reloc_indices_hashlist[hash];
        reloc = &csc->relocs[i];
        if (reloc->handle == bo->handle) {
            return i;
        }

        /* Hash collision, look for the BO in the list of relocs linearly. */
        for (i = csc->crelocs; i != 0;) {
            --i;
            reloc = &csc->relocs[i];
            if (reloc->handle == bo->handle) {
                /* Put this reloc in the hash list.
                 * This will prevent additional hash collisions if there are
                 * several consecutive get_reloc calls for the same buffer.
                 *
                 * Example: Assuming buffers A,B,C collide in the hash list,
                 * the following sequence of relocs:
                 *         AAAAAAAAAAABBBBBBBBBBBBBBCCCCCCCC
                 * will collide here: ^ and here:   ^,
                 * meaning that we should get very few collisions in the end. */
                csc->reloc_indices_hashlist[hash] = i;
                /*printf("write_reloc collision, hash: %i, handle: %i\n", hash, bo->handle);*/
                return i;
            }
        }
    }

    return -1;
}

static unsigned radeon_add_reloc(struct radeon_drm_cs *cs,
                                 struct radeon_bo *bo,
                                 enum radeon_bo_usage usage,
                                 enum radeon_bo_domain domains,
                                 enum radeon_bo_domain *added_domains)
{
    struct radeon_cs_context *csc = cs->csc;
    struct drm_radeon_cs_reloc *reloc;
    unsigned hash = bo->handle & (sizeof(csc->is_handle_added)-1);
    enum radeon_bo_domain rd = usage & RADEON_USAGE_READ ? domains : 0;
    enum radeon_bo_domain wd = usage & RADEON_USAGE_WRITE ? domains : 0;
    bool update_hash = TRUE;
    int i;

    *added_domains = 0;
    if (csc->is_handle_added[hash]) {
        i = csc->reloc_indices_hashlist[hash];
        reloc = &csc->relocs[i];
        if (reloc->handle != bo->handle) {
            /* Hash collision, look for the BO in the list of relocs linearly. */
            for (i = csc->crelocs - 1; i >= 0; i--) {
                reloc = &csc->relocs[i];
                if (reloc->handle == bo->handle) {
                    /*printf("write_reloc collision, hash: %i, handle: %i\n", hash, bo->handle);*/
                    break;
                }
            }
        }

        if (i >= 0) {
            /* On DMA ring we need to emit as many relocation as there is use of the bo
             * thus each time this function is call we should grow add again the bo to
             * the relocation buffer
             *
             * Do not update the hash table if it's dma ring, so that first hash always point
             * to first bo relocation which will the one used by the kernel. Following relocation
             * will be ignore by the kernel memory placement (but still use by the kernel to
             * update the cmd stream with proper buffer offset).
             */
            update_hash = FALSE;
            update_reloc_domains(reloc, rd, wd, added_domains);
            if (cs->base.ring_type != RING_DMA) {
                csc->reloc_indices_hashlist[hash] = i;
                return i;
            }
        }
    }

    /* New relocation, check if the backing array is large enough. */
    if (csc->crelocs >= csc->nrelocs) {
        uint32_t size;
        csc->nrelocs += 10;

        size = csc->nrelocs * sizeof(struct radeon_bo*);
        csc->relocs_bo = realloc(csc->relocs_bo, size);

        size = csc->nrelocs * sizeof(struct drm_radeon_cs_reloc);
        csc->relocs = realloc(csc->relocs, size);

        csc->chunks[1].chunk_data = (uint64_t)(uintptr_t)csc->relocs;
    }

    /* Initialize the new relocation. */
    csc->relocs_bo[csc->crelocs] = NULL;
    radeon_bo_reference(&csc->relocs_bo[csc->crelocs], bo);
    p_atomic_inc(&bo->num_cs_references);
    reloc = &csc->relocs[csc->crelocs];
    reloc->handle = bo->handle;
    reloc->read_domains = rd;
    reloc->write_domain = wd;
    reloc->flags = 0;

    csc->is_handle_added[hash] = TRUE;
    if (update_hash) {
        csc->reloc_indices_hashlist[hash] = csc->crelocs;
    }

    csc->chunks[1].length_dw += RELOC_DWORDS;

    *added_domains = rd | wd;
    return csc->crelocs++;
}

static unsigned radeon_drm_cs_add_reloc(struct radeon_winsys_cs *rcs,
                                        struct radeon_winsys_cs_handle *buf,
                                        enum radeon_bo_usage usage,
                                        enum radeon_bo_domain domains)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)buf;
    enum radeon_bo_domain added_domains;
    unsigned index = radeon_add_reloc(cs, bo, usage, domains, &added_domains);

    if (added_domains & RADEON_DOMAIN_GTT)
        cs->csc->used_gart += bo->base.size;
    if (added_domains & RADEON_DOMAIN_VRAM)
        cs->csc->used_vram += bo->base.size;

    return index;
}

static boolean radeon_drm_cs_validate(struct radeon_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    boolean status =
        cs->csc->used_gart < cs->ws->info.gart_size * 0.8 &&
        cs->csc->used_vram < cs->ws->info.vram_size * 0.8;

    if (status) {
        cs->csc->validated_crelocs = cs->csc->crelocs;
    } else {
        /* Remove lately-added relocations. The validation failed with them
         * and the CS is about to be flushed because of that. Keep only
         * the already-validated relocations. */
        unsigned i;

        for (i = cs->csc->validated_crelocs; i < cs->csc->crelocs; i++) {
            p_atomic_dec(&cs->csc->relocs_bo[i]->num_cs_references);
            radeon_bo_reference(&cs->csc->relocs_bo[i], NULL);
        }
        cs->csc->crelocs = cs->csc->validated_crelocs;

        /* Flush if there are any relocs. Clean up otherwise. */
        if (cs->csc->crelocs) {
            cs->flush_cs(cs->flush_data, RADEON_FLUSH_ASYNC);
        } else {
            radeon_cs_context_cleanup(cs->csc);

            assert(cs->base.cdw == 0);
            if (cs->base.cdw != 0) {
                fprintf(stderr, "radeon: Unexpected error in %s.\n", __func__);
            }
        }
    }
    return status;
}

static void radeon_drm_cs_write_reloc(struct radeon_winsys_cs *rcs,
                                      struct radeon_winsys_cs_handle *buf)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)buf;
    unsigned index = radeon_get_reloc(cs->csc, bo);

    if (index == -1) {
        fprintf(stderr, "radeon: Cannot get a relocation in %s.\n", __func__);
        return;
    }

    OUT_CS(&cs->base, 0xc0001000);
    OUT_CS(&cs->base, index * RELOC_DWORDS);
}

void radeon_drm_cs_emit_ioctl_oneshot(struct radeon_cs_context *csc)
{
    unsigned i;

    if (drmCommandWriteRead(csc->fd, DRM_RADEON_CS,
                            &csc->cs, sizeof(struct drm_radeon_cs))) {
        if (debug_get_bool_option("RADEON_DUMP_CS", FALSE)) {
            unsigned i;

            fprintf(stderr, "radeon: The kernel rejected CS, dumping...\n");
            for (i = 0; i < csc->chunks[0].length_dw; i++) {
                fprintf(stderr, "0x%08X\n", csc->buf[i]);
            }
        } else {
            fprintf(stderr, "radeon: The kernel rejected CS, "
                    "see dmesg for more information.\n");
        }
    }

    for (i = 0; i < csc->crelocs; i++)
        p_atomic_dec(&csc->relocs_bo[i]->num_active_ioctls);

    radeon_cs_context_cleanup(csc);
}

/*
 * Make sure previous submission of this cs are completed
 */
void radeon_drm_cs_sync_flush(struct radeon_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    /* Wait for any pending ioctl to complete. */
    if (cs->ws->thread && cs->flush_started) {
        pipe_semaphore_wait(&cs->flush_completed);
        cs->flush_started = 0;
    }
}

DEBUG_GET_ONCE_BOOL_OPTION(noop, "RADEON_NOOP", FALSE)

static void radeon_drm_cs_flush(struct radeon_winsys_cs *rcs, unsigned flags)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_cs_context *tmp;

    if (rcs->cdw > RADEON_MAX_CMDBUF_DWORDS) {
       fprintf(stderr, "radeon: command stream overflowed\n");
    }

    radeon_drm_cs_sync_flush(rcs);

    /* Flip command streams. */
    tmp = cs->csc;
    cs->csc = cs->cst;
    cs->cst = tmp;

    /* If the CS is not empty or overflowed, emit it in a separate thread. */
    if (cs->base.cdw && cs->base.cdw <= RADEON_MAX_CMDBUF_DWORDS && !debug_get_option_noop()) {
        unsigned i, crelocs = cs->cst->crelocs;

        cs->cst->chunks[0].length_dw = cs->base.cdw;

        for (i = 0; i < crelocs; i++) {
            /* Update the number of active asynchronous CS ioctls for the buffer. */
            p_atomic_inc(&cs->cst->relocs_bo[i]->num_active_ioctls);
        }

        switch (cs->base.ring_type) {
        case RING_DMA:
            cs->cst->flags[0] = 0;
            cs->cst->flags[1] = RADEON_CS_RING_DMA;
            cs->cst->cs.num_chunks = 3;
            if (cs->ws->info.r600_virtual_address) {
                cs->cst->flags[0] |= RADEON_CS_USE_VM;
            }
            break;
        default:
        case RING_GFX:
            cs->cst->flags[0] = 0;
            cs->cst->flags[1] = RADEON_CS_RING_GFX;
            cs->cst->cs.num_chunks = 2;
            if (flags & RADEON_FLUSH_KEEP_TILING_FLAGS) {
                cs->cst->flags[0] |= RADEON_CS_KEEP_TILING_FLAGS;
                cs->cst->cs.num_chunks = 3;
            }
            if (cs->ws->info.r600_virtual_address) {
                cs->cst->flags[0] |= RADEON_CS_USE_VM;
                cs->cst->cs.num_chunks = 3;
            }
            if (flags & RADEON_FLUSH_END_OF_FRAME) {
                cs->cst->flags[0] |= RADEON_CS_END_OF_FRAME;
                cs->cst->cs.num_chunks = 3;
            }
            if (flags & RADEON_FLUSH_COMPUTE) {
                cs->cst->flags[1] = RADEON_CS_RING_COMPUTE;
                cs->cst->cs.num_chunks = 3;
            }
            break;
        }

        if (cs->ws->thread && (flags & RADEON_FLUSH_ASYNC)) {
            cs->flush_started = 1;
            radeon_drm_ws_queue_cs(cs->ws, cs);
        } else {
            pipe_mutex_lock(cs->ws->cs_stack_lock);
            if (cs->ws->thread) {
                while (p_atomic_read(&cs->ws->ncs)) {
                    pipe_condvar_wait(cs->ws->cs_queue_empty, cs->ws->cs_stack_lock);
                }
            }
            pipe_mutex_unlock(cs->ws->cs_stack_lock);
            radeon_drm_cs_emit_ioctl_oneshot(cs->cst);
        }
    } else {
        radeon_cs_context_cleanup(cs->cst);
    }

    /* Prepare a new CS. */
    cs->base.buf = cs->csc->buf;
    cs->base.cdw = 0;
}

static void radeon_drm_cs_destroy(struct radeon_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    radeon_drm_cs_sync_flush(rcs);
    pipe_semaphore_destroy(&cs->flush_completed);
    radeon_cs_context_cleanup(&cs->csc1);
    radeon_cs_context_cleanup(&cs->csc2);
    p_atomic_dec(&cs->ws->num_cs);
    radeon_destroy_cs_context(&cs->csc1);
    radeon_destroy_cs_context(&cs->csc2);
    FREE(cs);
}

static void radeon_drm_cs_set_flush(struct radeon_winsys_cs *rcs,
                                    void (*flush)(void *ctx, unsigned flags),
                                    void *user)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    cs->flush_cs = flush;
    cs->flush_data = user;
}

static boolean radeon_bo_is_referenced(struct radeon_winsys_cs *rcs,
                                       struct radeon_winsys_cs_handle *_buf,
                                       enum radeon_bo_usage usage)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)_buf;
    int index;

    if (!bo->num_cs_references)
        return FALSE;

    index = radeon_get_reloc(cs->csc, bo);
    if (index == -1)
        return FALSE;

    if ((usage & RADEON_USAGE_WRITE) && cs->csc->relocs[index].write_domain)
        return TRUE;
    if ((usage & RADEON_USAGE_READ) && cs->csc->relocs[index].read_domains)
        return TRUE;

    return FALSE;
}

void radeon_drm_cs_init_functions(struct radeon_drm_winsys *ws)
{
    ws->base.cs_create = radeon_drm_cs_create;
    ws->base.cs_destroy = radeon_drm_cs_destroy;
    ws->base.cs_add_reloc = radeon_drm_cs_add_reloc;
    ws->base.cs_validate = radeon_drm_cs_validate;
    ws->base.cs_write_reloc = radeon_drm_cs_write_reloc;
    ws->base.cs_flush = radeon_drm_cs_flush;
    ws->base.cs_set_flush_callback = radeon_drm_cs_set_flush;
    ws->base.cs_is_buffer_referenced = radeon_bo_is_referenced;
    ws->base.cs_sync_flush = radeon_drm_cs_sync_flush;
}
