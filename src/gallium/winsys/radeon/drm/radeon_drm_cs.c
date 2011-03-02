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
    It's optimized specifically for r300g, but r600g could use it as well.
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

#define RELOC_DWORDS (sizeof(struct drm_radeon_cs_reloc) / sizeof(uint32_t))

static boolean radeon_init_cs_context(struct radeon_cs_context *csc, int fd)
{
    csc->fd = fd;
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

    csc->chunk_array[0] = (uint64_t)(uintptr_t)&csc->chunks[0];
    csc->chunk_array[1] = (uint64_t)(uintptr_t)&csc->chunks[1];

    csc->cs.num_chunks = 2;
    csc->cs.chunks = (uint64_t)(uintptr_t)csc->chunk_array;
    return TRUE;
}

static void radeon_cs_context_cleanup(struct radeon_cs_context *csc)
{
    unsigned i;

    for (i = 0; i < csc->crelocs; i++) {
        p_atomic_dec(&csc->relocs_bo[i]->num_cs_references);
        radeon_bo_unref(csc->relocs_bo[i]);
        csc->relocs_bo[i] = NULL;
    }

    csc->crelocs = 0;
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

static struct r300_winsys_cs *radeon_drm_cs_create(struct r300_winsys_screen *rws)
{
    struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
    struct radeon_drm_cs *cs;

    cs = CALLOC_STRUCT(radeon_drm_cs);
    if (!cs) {
        return NULL;
    }

    cs->ws = ws;

    if (!radeon_init_cs_context(&cs->csc1, cs->ws->fd)) {
        FREE(cs);
        return NULL;
    }
    if (!radeon_init_cs_context(&cs->csc2, cs->ws->fd)) {
        radeon_destroy_cs_context(&cs->csc1);
        FREE(cs);
        return NULL;
    }

    /* Set the first command buffer as current. */
    cs->csc = &cs->csc1;
    cs->cst = &cs->csc2;
    cs->base.buf = cs->csc->buf;

    p_atomic_inc(&ws->num_cs);
    return &cs->base;
}

#define OUT_CS(cs, value) (cs)->buf[(cs)->cdw++] = (value)

static INLINE void update_domains(struct drm_radeon_cs_reloc *reloc,
                                  enum r300_buffer_domain rd,
                                  enum r300_buffer_domain wd,
                                  enum r300_buffer_domain *added_domains)
{
    *added_domains = (rd | wd) & ~(reloc->read_domains | reloc->write_domain);

    if (reloc->read_domains & wd) {
        reloc->read_domains = rd;
        reloc->write_domain = wd;
    } else if (rd & reloc->write_domain) {
        reloc->read_domains = rd;
        reloc->write_domain |= wd;
    } else {
        reloc->read_domains |= rd;
        reloc->write_domain |= wd;
    }
}

int radeon_get_reloc(struct radeon_cs_context *csc, struct radeon_bo *bo)
{
    struct drm_radeon_cs_reloc *reloc;
    unsigned i;
    unsigned hash = bo->handle & (sizeof(csc->is_handle_added)-1);

    if (csc->is_handle_added[hash]) {
        reloc = csc->relocs_hashlist[hash];
        if (reloc->handle == bo->handle) {
            return csc->reloc_indices_hashlist[hash];
        }

        /* Hash collision, look for the BO in the list of relocs linearly. */
        for (i = csc->crelocs; i != 0;) {
            --i;
            reloc = &csc->relocs[i];
            if (reloc->handle == bo->handle) {
                /* Put this reloc in the hash list.
                 * This will prevent additional hash collisions if there are
                 * several subsequent get_reloc calls of the same buffer.
                 *
                 * Example: Assuming buffers A,B,C collide in the hash list,
                 * the following sequence of relocs:
                 *         AAAAAAAAAAABBBBBBBBBBBBBBCCCCCCCC
                 * will collide here: ^ and here:   ^,
                 * meaning that we should get very few collisions in the end. */
                csc->relocs_hashlist[hash] = reloc;
                csc->reloc_indices_hashlist[hash] = i;
                /*printf("write_reloc collision, hash: %i, handle: %i\n", hash, bo->handle);*/
                return i;
            }
        }
    }

    return -1;
}

static void radeon_add_reloc(struct radeon_cs_context *csc,
                             struct radeon_bo *bo,
                             enum r300_buffer_domain rd,
                             enum r300_buffer_domain wd,
                             enum r300_buffer_domain *added_domains)
{
    struct drm_radeon_cs_reloc *reloc;
    unsigned i;
    unsigned hash = bo->handle & (sizeof(csc->is_handle_added)-1);

    if (csc->is_handle_added[hash]) {
        reloc = csc->relocs_hashlist[hash];
        if (reloc->handle == bo->handle) {
            update_domains(reloc, rd, wd, added_domains);
            return;
        }

        /* Hash collision, look for the BO in the list of relocs linearly. */
        for (i = csc->crelocs; i != 0;) {
            --i;
            reloc = &csc->relocs[i];
            if (reloc->handle == bo->handle) {
                update_domains(reloc, rd, wd, added_domains);

                csc->relocs_hashlist[hash] = reloc;
                csc->reloc_indices_hashlist[hash] = i;
                /*printf("write_reloc collision, hash: %i, handle: %i\n", hash, bo->handle);*/
                return;
            }
        }
    }

    /* New relocation, check if the backing array is large enough. */
    if (csc->crelocs >= csc->nrelocs) {
        uint32_t size;
        csc->nrelocs += 10;

        size = csc->nrelocs * sizeof(struct radeon_bo*);
        csc->relocs_bo = (struct radeon_bo**)realloc(csc->relocs_bo, size);

        size = csc->nrelocs * sizeof(struct drm_radeon_cs_reloc);
        csc->relocs = (struct drm_radeon_cs_reloc*)realloc(csc->relocs, size);

        csc->chunks[1].chunk_data = (uint64_t)(uintptr_t)csc->relocs;
    }

    /* Initialize the new relocation. */
    radeon_bo_ref(bo);
    p_atomic_inc(&bo->num_cs_references);
    csc->relocs_bo[csc->crelocs] = bo;
    reloc = &csc->relocs[csc->crelocs];
    reloc->handle = bo->handle;
    reloc->read_domains = rd;
    reloc->write_domain = wd;
    reloc->flags = 0;

    csc->is_handle_added[hash] = TRUE;
    csc->relocs_hashlist[hash] = reloc;
    csc->reloc_indices_hashlist[hash] = csc->crelocs;

    csc->chunks[1].length_dw += RELOC_DWORDS;
    csc->crelocs++;

    *added_domains = rd | wd;
}

static void radeon_drm_cs_add_reloc(struct r300_winsys_cs *rcs,
                                    struct r300_winsys_cs_handle *buf,
                                    enum r300_buffer_domain rd,
                                    enum r300_buffer_domain wd)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)buf;
    enum r300_buffer_domain added_domains;

    radeon_add_reloc(cs->csc, bo, rd, wd, &added_domains);

    if (!added_domains)
        return;

    if (added_domains & R300_DOMAIN_GTT)
        cs->csc->used_gart += bo->size;
    if (added_domains & R300_DOMAIN_VRAM)
        cs->csc->used_vram += bo->size;
}

static boolean radeon_drm_cs_validate(struct r300_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    return cs->csc->used_gart < cs->ws->gart_size * 0.8 &&
           cs->csc->used_vram < cs->ws->vram_size * 0.8;
}

static void radeon_drm_cs_write_reloc(struct r300_winsys_cs *rcs,
                                      struct r300_winsys_cs_handle *buf)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)buf;

    unsigned index = radeon_get_reloc(cs->csc, bo);

    if (index == -1) {
        fprintf(stderr, "r300: Cannot get a relocation in %s.\n", __func__);
        return;
    }

    OUT_CS(&cs->base, 0xc0001000);
    OUT_CS(&cs->base, index * RELOC_DWORDS);
}

static PIPE_THREAD_ROUTINE(radeon_drm_cs_emit_ioctl, param)
{
    struct radeon_cs_context *csc = (struct radeon_cs_context*)param;
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
    return NULL;
}

void radeon_drm_cs_sync_flush(struct r300_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    /* Wait for any pending ioctl to complete. */
    if (cs->thread) {
        pipe_thread_wait(cs->thread);
        cs->thread = 0;
    }
}

DEBUG_GET_ONCE_BOOL_OPTION(thread, "RADEON_THREAD", TRUE)

void radeon_drm_cs_flush(struct r300_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_cs_context *tmp;

    radeon_drm_cs_sync_flush(rcs);

    /* If the CS is not empty, emit it in a newly-spawned thread. */
    if (cs->base.cdw) {
        unsigned i, crelocs = cs->csc->crelocs;

        cs->csc->chunks[0].length_dw = cs->base.cdw;

        for (i = 0; i < crelocs; i++)
            p_atomic_inc(&cs->csc->relocs_bo[i]->num_active_ioctls);

        if (cs->ws->num_cpus > 1 && debug_get_option_thread()) {
            cs->thread = pipe_thread_create(radeon_drm_cs_emit_ioctl, cs->csc);
            assert(cs->thread);
        } else {
            radeon_drm_cs_emit_ioctl(cs->csc);
        }
    }

    /* Flip command streams. */
    tmp = cs->csc;
    cs->csc = cs->cst;
    cs->cst = tmp;

    /* Prepare a new CS. */
    radeon_cs_context_cleanup(cs->csc);

    cs->base.buf = cs->csc->buf;
    cs->base.cdw = 0;
}

static void radeon_drm_cs_destroy(struct r300_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    radeon_drm_cs_sync_flush(rcs);
    radeon_cs_context_cleanup(&cs->csc1);
    radeon_cs_context_cleanup(&cs->csc2);
    p_atomic_dec(&cs->ws->num_cs);
    radeon_destroy_cs_context(&cs->csc1);
    radeon_destroy_cs_context(&cs->csc2);
    FREE(cs);
}

static void radeon_drm_cs_set_flush(struct r300_winsys_cs *rcs,
                                    void (*flush)(void *), void *user)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    cs->flush_cs = flush;
    cs->flush_data = user;
}

static boolean radeon_bo_is_referenced(struct r300_winsys_cs *rcs,
                                       struct r300_winsys_cs_handle *_buf)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)_buf;

    return radeon_bo_is_referenced_by_cs(cs, bo);
}

void radeon_drm_cs_init_functions(struct radeon_drm_winsys *ws)
{
    ws->base.cs_create = radeon_drm_cs_create;
    ws->base.cs_destroy = radeon_drm_cs_destroy;
    ws->base.cs_add_reloc = radeon_drm_cs_add_reloc;
    ws->base.cs_validate = radeon_drm_cs_validate;
    ws->base.cs_write_reloc = radeon_drm_cs_write_reloc;
    ws->base.cs_flush = radeon_drm_cs_flush;
    ws->base.cs_sync_flush = radeon_drm_cs_sync_flush;
    ws->base.cs_set_flush = radeon_drm_cs_set_flush;
    ws->base.cs_is_buffer_referenced = radeon_bo_is_referenced;
}
