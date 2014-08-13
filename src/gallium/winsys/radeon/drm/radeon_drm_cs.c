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
#include "os/os_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xf86drm.h>


#define RELOC_DWORDS (sizeof(struct drm_radeon_cs_reloc) / sizeof(uint32_t))

static struct pipe_fence_handle *
radeon_cs_create_fence(struct radeon_winsys_cs *rcs);
static void radeon_fence_reference(struct pipe_fence_handle **dst,
                                   struct pipe_fence_handle *src);

static boolean radeon_init_cs_context(struct radeon_cs_context *csc,
                                      struct radeon_drm_winsys *ws)
{
    int i;

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

    for (i = 0; i < Elements(csc->reloc_indices_hashlist); i++) {
        csc->reloc_indices_hashlist[i] = -1;
    }
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

    for (i = 0; i < Elements(csc->reloc_indices_hashlist); i++) {
        csc->reloc_indices_hashlist[i] = -1;
    }
}

static void radeon_destroy_cs_context(struct radeon_cs_context *csc)
{
    radeon_cs_context_cleanup(csc);
    FREE(csc->relocs_bo);
    FREE(csc->relocs);
}


static struct radeon_winsys_cs *
radeon_drm_cs_create(struct radeon_winsys *rws,
                     enum ring_type ring_type,
                     void (*flush)(void *ctx, unsigned flags,
                                   struct pipe_fence_handle **fence),
                     void *flush_ctx,
                     struct radeon_winsys_cs_handle *trace_buf)
{
    struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
    struct radeon_drm_cs *cs;

    cs = CALLOC_STRUCT(radeon_drm_cs);
    if (!cs) {
        return NULL;
    }
    pipe_semaphore_init(&cs->flush_completed, 1);

    cs->ws = ws;
    cs->flush_cs = flush;
    cs->flush_data = flush_ctx;
    cs->trace_buf = (struct radeon_bo*)trace_buf;

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

static INLINE void update_reloc(struct drm_radeon_cs_reloc *reloc,
                                enum radeon_bo_domain rd,
                                enum radeon_bo_domain wd,
                                unsigned priority,
                                enum radeon_bo_domain *added_domains)
{
    *added_domains = (rd | wd) & ~(reloc->read_domains | reloc->write_domain);

    reloc->read_domains |= rd;
    reloc->write_domain |= wd;
    reloc->flags = MAX2(reloc->flags, priority);
}

int radeon_get_reloc(struct radeon_cs_context *csc, struct radeon_bo *bo)
{
    unsigned hash = bo->handle & (Elements(csc->reloc_indices_hashlist)-1);
    int i = csc->reloc_indices_hashlist[hash];

    /* not found or found */
    if (i == -1 || csc->relocs_bo[i] == bo)
        return i;

    /* Hash collision, look for the BO in the list of relocs linearly. */
    for (i = csc->crelocs - 1; i >= 0; i--) {
        if (csc->relocs_bo[i] == bo) {
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
            return i;
        }
    }
    return -1;
}

static unsigned radeon_add_reloc(struct radeon_drm_cs *cs,
                                 struct radeon_bo *bo,
                                 enum radeon_bo_usage usage,
                                 enum radeon_bo_domain domains,
                                 unsigned priority,
                                 enum radeon_bo_domain *added_domains)
{
    struct radeon_cs_context *csc = cs->csc;
    struct drm_radeon_cs_reloc *reloc;
    unsigned hash = bo->handle & (Elements(csc->reloc_indices_hashlist)-1);
    enum radeon_bo_domain rd = usage & RADEON_USAGE_READ ? domains : 0;
    enum radeon_bo_domain wd = usage & RADEON_USAGE_WRITE ? domains : 0;
    int i = -1;

    priority = MIN2(priority, 15);
    *added_domains = 0;

    i = radeon_get_reloc(csc, bo);

    if (i >= 0) {
        reloc = &csc->relocs[i];
        update_reloc(reloc, rd, wd, priority, added_domains);

        /* For async DMA, every add_reloc call must add a buffer to the list
         * no matter how many duplicates there are. This is due to the fact
         * the DMA CS checker doesn't use NOP packets for offset patching,
         * but always uses the i-th buffer from the list to patch the i-th
         * offset. If there are N offsets in a DMA CS, there must also be N
         * buffers in the relocation list.
         *
         * This doesn't have to be done if virtual memory is enabled,
         * because there is no offset patching with virtual memory.
         */
        if (cs->base.ring_type != RING_DMA || cs->ws->info.r600_virtual_address) {
            return i;
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
    reloc->flags = priority;

    csc->reloc_indices_hashlist[hash] = csc->crelocs;

    csc->chunks[1].length_dw += RELOC_DWORDS;

    *added_domains = rd | wd;
    return csc->crelocs++;
}

static unsigned radeon_drm_cs_add_reloc(struct radeon_winsys_cs *rcs,
                                        struct radeon_winsys_cs_handle *buf,
                                        enum radeon_bo_usage usage,
                                        enum radeon_bo_domain domains,
                                        enum radeon_bo_priority priority)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)buf;
    enum radeon_bo_domain added_domains;
    unsigned index = radeon_add_reloc(cs, bo, usage, domains, priority, &added_domains);

    if (added_domains & RADEON_DOMAIN_GTT)
        cs->csc->used_gart += bo->base.size;
    if (added_domains & RADEON_DOMAIN_VRAM)
        cs->csc->used_vram += bo->base.size;

    return index;
}

static int radeon_drm_cs_get_reloc(struct radeon_winsys_cs *rcs,
                                   struct radeon_winsys_cs_handle *buf)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    return radeon_get_reloc(cs->csc, (struct radeon_bo*)buf);
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
            cs->flush_cs(cs->flush_data, RADEON_FLUSH_ASYNC, NULL);
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

static boolean radeon_drm_cs_memory_below_limit(struct radeon_winsys_cs *rcs, uint64_t vram, uint64_t gtt)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    boolean status =
        (cs->csc->used_gart + gtt) < cs->ws->info.gart_size * 0.7 &&
        (cs->csc->used_vram + vram) < cs->ws->info.vram_size * 0.7;

    return status;
}

void radeon_drm_cs_emit_ioctl_oneshot(struct radeon_drm_cs *cs, struct radeon_cs_context *csc)
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

    if (cs->trace_buf) {
        radeon_dump_cs_on_lockup(cs, csc);
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
    if (cs->ws->thread) {
        pipe_semaphore_wait(&cs->flush_completed);
        pipe_semaphore_signal(&cs->flush_completed);
    }
}

DEBUG_GET_ONCE_BOOL_OPTION(noop, "RADEON_NOOP", FALSE)

static void radeon_drm_cs_flush(struct radeon_winsys_cs *rcs,
                                unsigned flags,
                                struct pipe_fence_handle **fence,
                                uint32_t cs_trace_id)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_cs_context *tmp;

    switch (cs->base.ring_type) {
    case RING_DMA:
        /* pad DMA ring to 8 DWs */
        if (cs->ws->info.chip_class <= SI) {
            while (rcs->cdw & 7)
                OUT_CS(&cs->base, 0xf0000000); /* NOP packet */
        } else {
            while (rcs->cdw & 7)
                OUT_CS(&cs->base, 0x00000000); /* NOP packet */
        }
        break;
    case RING_GFX:
        /* pad DMA ring to 8 DWs to meet CP fetch alignment requirements
         * r6xx, requires at least 4 dw alignment to avoid a hw bug.
         * hawaii with old firmware needs type2 nop packet.
         * accel_working2 with value 3 indicates the new firmware.
         */
        if (cs->ws->info.chip_class <= SI ||
            (cs->ws->info.family == CHIP_HAWAII &&
             cs->ws->accel_working2 < 3)) {
            while (rcs->cdw & 7)
                OUT_CS(&cs->base, 0x80000000); /* type2 nop packet */
        } else {
            while (rcs->cdw & 7)
                OUT_CS(&cs->base, 0xffff1000); /* type3 nop packet */
        }
        break;
    case RING_UVD:
        while (rcs->cdw & 15)
            OUT_CS(&cs->base, 0x80000000); /* type2 nop packet */
        break;
    default:
        break;
    }

    if (rcs->cdw > RADEON_MAX_CMDBUF_DWORDS) {
       fprintf(stderr, "radeon: command stream overflowed\n");
    }

    if (fence) {
        radeon_fence_reference(fence, NULL);
        *fence = radeon_cs_create_fence(rcs);
    }

    radeon_drm_cs_sync_flush(rcs);

    /* Swap command streams. */
    tmp = cs->csc;
    cs->csc = cs->cst;
    cs->cst = tmp;

    cs->cst->cs_trace_id = cs_trace_id;

    /* If the CS is not empty or overflowed, emit it in a separate thread. */
    if (cs->base.cdw && cs->base.cdw <= RADEON_MAX_CMDBUF_DWORDS && !debug_get_option_noop()) {
        unsigned i, crelocs;

        crelocs = cs->cst->crelocs;

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

        case RING_UVD:
            cs->cst->flags[0] = 0;
            cs->cst->flags[1] = RADEON_CS_RING_UVD;
            cs->cst->cs.num_chunks = 3;
            break;

        case RING_VCE:
            cs->cst->flags[0] = 0;
            cs->cst->flags[1] = RADEON_CS_RING_VCE;
            cs->cst->cs.num_chunks = 3;
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

        if (cs->ws->thread) {
            pipe_semaphore_wait(&cs->flush_completed);
            radeon_drm_ws_queue_cs(cs->ws, cs);
            if (!(flags & RADEON_FLUSH_ASYNC))
                radeon_drm_cs_sync_flush(rcs);
        } else {
            radeon_drm_cs_emit_ioctl_oneshot(cs, cs->cst);
        }
    } else {
        radeon_cs_context_cleanup(cs->cst);
    }

    /* Prepare a new CS. */
    cs->base.buf = cs->csc->buf;
    cs->base.cdw = 0;

    cs->ws->num_cs_flushes++;
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

/* FENCES */

static struct pipe_fence_handle *
radeon_cs_create_fence(struct radeon_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct pb_buffer *fence;

    /* Create a fence, which is a dummy BO. */
    fence = cs->ws->base.buffer_create(&cs->ws->base, 1, 1, TRUE,
                                       RADEON_DOMAIN_GTT, 0);
    /* Add the fence as a dummy relocation. */
    cs->ws->base.cs_add_reloc(rcs, cs->ws->base.buffer_get_cs_handle(fence),
                              RADEON_USAGE_READWRITE, RADEON_DOMAIN_GTT,
                              RADEON_PRIO_MIN);
    return (struct pipe_fence_handle*)fence;
}

static bool radeon_fence_wait(struct radeon_winsys *ws,
                              struct pipe_fence_handle *fence,
                              uint64_t timeout)
{
    struct pb_buffer *rfence = (struct pb_buffer*)fence;

    if (timeout == 0)
        return !ws->buffer_is_busy(rfence, RADEON_USAGE_READWRITE);

    if (timeout != PIPE_TIMEOUT_INFINITE) {
        int64_t start_time = os_time_get();

        /* Convert to microseconds. */
        timeout /= 1000;

        /* Wait in a loop. */
        while (ws->buffer_is_busy(rfence, RADEON_USAGE_READWRITE)) {
            if (os_time_get() - start_time >= timeout) {
                return FALSE;
            }
            os_time_sleep(10);
        }
        return TRUE;
    }

    ws->buffer_wait(rfence, RADEON_USAGE_READWRITE);
    return TRUE;
}

static void radeon_fence_reference(struct pipe_fence_handle **dst,
                                   struct pipe_fence_handle *src)
{
    pb_reference((struct pb_buffer**)dst, (struct pb_buffer*)src);
}

void radeon_drm_cs_init_functions(struct radeon_drm_winsys *ws)
{
    ws->base.cs_create = radeon_drm_cs_create;
    ws->base.cs_destroy = radeon_drm_cs_destroy;
    ws->base.cs_add_reloc = radeon_drm_cs_add_reloc;
    ws->base.cs_get_reloc = radeon_drm_cs_get_reloc;
    ws->base.cs_validate = radeon_drm_cs_validate;
    ws->base.cs_memory_below_limit = radeon_drm_cs_memory_below_limit;
    ws->base.cs_flush = radeon_drm_cs_flush;
    ws->base.cs_is_buffer_referenced = radeon_bo_is_referenced;
    ws->base.cs_sync_flush = radeon_drm_cs_sync_flush;
    ws->base.fence_wait = radeon_fence_wait;
    ws->base.fence_reference = radeon_fence_reference;
}
