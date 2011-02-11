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

static struct r300_winsys_cs *radeon_drm_cs_create(struct r300_winsys_screen *rws)
{
    struct radeon_drm_winsys *ws = radeon_drm_winsys(rws);
    struct radeon_drm_cs *cs;

    cs = CALLOC_STRUCT(radeon_drm_cs);
    if (!cs) {
        return NULL;
    }

    cs->ws = ws;
    cs->nrelocs = 256;
    cs->relocs_bo = (struct radeon_bo**)
                     CALLOC(1, cs->nrelocs * sizeof(struct radeon_bo*));
    if (!cs->relocs_bo) {
        FREE(cs);
        return NULL;
    }

    cs->relocs = (struct drm_radeon_cs_reloc*)
                 CALLOC(1, cs->nrelocs * sizeof(struct drm_radeon_cs_reloc));
    if (!cs->relocs) {
        FREE(cs->relocs_bo);
        FREE(cs);
        return NULL;
    }

    cs->chunks[0].chunk_id = RADEON_CHUNK_ID_IB;
    cs->chunks[0].length_dw = 0;
    cs->chunks[0].chunk_data = (uint64_t)(uintptr_t)cs->base.buf;
    cs->chunks[1].chunk_id = RADEON_CHUNK_ID_RELOCS;
    cs->chunks[1].length_dw = 0;
    cs->chunks[1].chunk_data = (uint64_t)(uintptr_t)cs->relocs;
    p_atomic_inc(&ws->num_cs);
    return &cs->base;
}

#define OUT_CS(cs, value) (cs)->buf[(cs)->cdw++] = (value)

static inline void update_domains(struct drm_radeon_cs_reloc *reloc,
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

int radeon_get_reloc(struct radeon_drm_cs *cs, struct radeon_bo *bo)
{
    struct drm_radeon_cs_reloc *reloc;
    unsigned i;
    unsigned hash = bo->handle & (sizeof(cs->is_handle_added)-1);

    if (cs->is_handle_added[hash]) {
        reloc = cs->relocs_hashlist[hash];
        if (reloc->handle == bo->handle) {
            return cs->reloc_indices_hashlist[hash];
        }

        /* Hash collision, look for the BO in the list of relocs linearly. */
        for (i = cs->crelocs; i != 0;) {
            --i;
            reloc = &cs->relocs[i];
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
                cs->relocs_hashlist[hash] = reloc;
                cs->reloc_indices_hashlist[hash] = i;
                /*printf("write_reloc collision, hash: %i, handle: %i\n", hash, bo->handle);*/
                return i;
            }
        }
    }

    return -1;
}

static void radeon_add_reloc(struct radeon_drm_cs *cs,
                             struct radeon_bo *bo,
                             enum r300_buffer_domain rd,
                             enum r300_buffer_domain wd,
                             enum r300_buffer_domain *added_domains)
{
    struct drm_radeon_cs_reloc *reloc;
    unsigned i;
    unsigned hash = bo->handle & (sizeof(cs->is_handle_added)-1);

    if (cs->is_handle_added[hash]) {
        reloc = cs->relocs_hashlist[hash];
        if (reloc->handle == bo->handle) {
            update_domains(reloc, rd, wd, added_domains);
            return;
        }

        /* Hash collision, look for the BO in the list of relocs linearly. */
        for (i = cs->crelocs; i != 0;) {
            --i;
            reloc = &cs->relocs[i];
            if (reloc->handle == bo->handle) {
                update_domains(reloc, rd, wd, added_domains);

                cs->relocs_hashlist[hash] = reloc;
                cs->reloc_indices_hashlist[hash] = i;
                /*printf("write_reloc collision, hash: %i, handle: %i\n", hash, bo->handle);*/
                return;
            }
        }
    }

    /* New relocation, check if the backing array is large enough. */
    if (cs->crelocs >= cs->nrelocs) {
        uint32_t size;
        cs->nrelocs += 10;

        size = cs->nrelocs * sizeof(struct radeon_bo*);
        cs->relocs_bo = (struct radeon_bo**)realloc(cs->relocs_bo, size);

        size = cs->nrelocs * sizeof(struct drm_radeon_cs_reloc);
        cs->relocs = (struct drm_radeon_cs_reloc*)realloc(cs->relocs, size);

        cs->chunks[1].chunk_data = (uint64_t)(uintptr_t)cs->relocs;
    }

    /* Initialize the new relocation. */
    radeon_bo_ref(bo);
    p_atomic_inc(&bo->num_cs_references);
    cs->relocs_bo[cs->crelocs] = bo;
    reloc = &cs->relocs[cs->crelocs];
    reloc->handle = bo->handle;
    reloc->read_domains = rd;
    reloc->write_domain = wd;
    reloc->flags = 0;

    cs->is_handle_added[hash] = TRUE;
    cs->relocs_hashlist[hash] = reloc;
    cs->reloc_indices_hashlist[hash] = cs->crelocs;

    cs->chunks[1].length_dw += RELOC_DWORDS;
    cs->crelocs++;

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

    radeon_add_reloc(cs, bo, rd, wd, &added_domains);

    if (!added_domains)
        return;

    if (added_domains & R300_DOMAIN_GTT)
        cs->used_gart += bo->size;
    if (added_domains & R300_DOMAIN_VRAM)
        cs->used_vram += bo->size;
}

static boolean radeon_drm_cs_validate(struct r300_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    return cs->used_gart < cs->ws->gart_size * 0.8 &&
           cs->used_vram < cs->ws->vram_size * 0.8;
}

static void radeon_drm_cs_write_reloc(struct r300_winsys_cs *rcs,
                                      struct r300_winsys_cs_handle *buf)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    struct radeon_bo *bo = (struct radeon_bo*)buf;

    unsigned index = radeon_get_reloc(cs, bo);

    if (index == -1) {
        fprintf(stderr, "r300: Cannot get a relocation in %s.\n", __func__);
        return;
    }

    OUT_CS(&cs->base, 0xc0001000);
    OUT_CS(&cs->base, index * RELOC_DWORDS);
}

static void radeon_drm_cs_emit(struct r300_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    uint64_t chunk_array[2];
    unsigned i;
    int r;

    if (cs->base.cdw) {
        /* Prepare the arguments. */
        cs->chunks[0].length_dw = cs->base.cdw;

        chunk_array[0] = (uint64_t)(uintptr_t)&cs->chunks[0];
        chunk_array[1] = (uint64_t)(uintptr_t)&cs->chunks[1];

        cs->cs.num_chunks = 2;
        cs->cs.chunks = (uint64_t)(uintptr_t)chunk_array;

        /* Emit. */
        r = drmCommandWriteRead(cs->ws->fd, DRM_RADEON_CS,
                                &cs->cs, sizeof(struct drm_radeon_cs));
        if (r) {
            if (debug_get_bool_option("RADEON_DUMP_CS", FALSE)) {
                fprintf(stderr, "radeon: The kernel rejected CS, dumping...\n");
                fprintf(stderr, "VENDORID:DEVICEID 0x%04X:0x%04X\n", 0x1002,
                        cs->ws->pci_id);
                for (i = 0; i < cs->base.cdw; i++) {
                    fprintf(stderr, "0x%08X\n", cs->base.buf[i]);
                }
            } else {
                fprintf(stderr, "radeon: The kernel rejected CS, "
                                "see dmesg for more information.\n");
            }
        }
    }

    /* Unreference buffers, cleanup. */
    for (i = 0; i < cs->crelocs; i++) {
        radeon_bo_unref(cs->relocs_bo[i]);
        p_atomic_dec(&cs->relocs_bo[i]->num_cs_references);
        cs->relocs_bo[i] = NULL;
    }

    cs->base.cdw = 0;
    cs->crelocs = 0;
    cs->chunks[0].length_dw = 0;
    cs->chunks[1].length_dw = 0;
    cs->used_gart = 0;
    cs->used_vram = 0;
    memset(cs->is_handle_added, 0, sizeof(cs->is_handle_added));
}

static void radeon_drm_cs_destroy(struct r300_winsys_cs *rcs)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);
    p_atomic_dec(&cs->ws->num_cs);
    FREE(cs->relocs_bo);
    FREE(cs->relocs);
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
    ws->base.cs_flush = radeon_drm_cs_emit;
    ws->base.cs_set_flush = radeon_drm_cs_set_flush;
    ws->base.cs_is_buffer_referenced = radeon_bo_is_referenced;
}
