/*
 * Copyright 2011 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jérôme Glisse
 */
#ifndef RADEON_CTX_H
#define RADEON_CTX_H

#define _FILE_OFFSET_BITS 64
#include <sys/mman.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "xf86drm.h"
#include "radeon_drm.h"

struct ctx {
    int                         fd;
};

struct bo {
    uint32_t                    handle;
    uint32_t                    alignment;
    uint64_t                    size;
    uint64_t                    va;
    void                        *ptr;
};

static void ctx_init(struct ctx *ctx)
{
    ctx->fd = drmOpen("radeon", NULL);
    if (ctx->fd < 0) {
        fprintf(stderr, "failed to open radeon drm device file\n");
        exit(-1);
    }
}

static void bo_wait(struct ctx *ctx, struct bo *bo)
{
    struct drm_radeon_gem_wait_idle args;
    void *ptr;
    int r;

    /* Zero out args to make valgrind happy */
    memset(&args, 0, sizeof(args));
    args.handle = bo->handle;
    do {
        r = drmCommandWrite(ctx->fd, DRM_RADEON_GEM_WAIT_IDLE, &args, sizeof(args));
    } while (r == -EBUSY);
}


static void ctx_cs(struct ctx *ctx, uint32_t *cs, uint32_t cs_flags[2], unsigned ndw,
                   struct bo **bo, uint32_t *bo_relocs, unsigned nbo)
{
    struct drm_radeon_cs args;
    struct drm_radeon_cs_chunk chunks[3];
    uint64_t chunk_array[3];
    unsigned i;
    int r;

    /* update handle */
    for (i = 0; i < nbo; i++) {
        bo_relocs[i*4+0] = bo[i]->handle;
    }

    args.num_chunks = 2;
    if (cs_flags[0] || cs_flags[1]) {
        /* enable RADEON_CHUNK_ID_FLAGS */
        args.num_chunks = 3;
    }
    args.chunks = (uint64_t)(uintptr_t)chunk_array;
    chunks[0].chunk_id = RADEON_CHUNK_ID_IB;
    chunks[0].length_dw = ndw;
    chunks[0].chunk_data = (uintptr_t)cs;
    chunks[1].chunk_id = RADEON_CHUNK_ID_RELOCS;
    chunks[1].length_dw = nbo * 4;
    chunks[1].chunk_data = (uintptr_t)bo_relocs;
    chunks[2].chunk_id = RADEON_CHUNK_ID_FLAGS;
    chunks[2].length_dw = 2;
    chunks[2].chunk_data = (uintptr_t)cs_flags;
    chunk_array[0] = (uintptr_t)&chunks[0];
    chunk_array[1] = (uintptr_t)&chunks[1];
    chunk_array[2] = (uintptr_t)&chunks[2];

    fprintf(stderr, "emiting cs %ddw with %d bo\n", ndw, nbo);
    r = drmCommandWriteRead(ctx->fd, DRM_RADEON_CS, &args, sizeof(args));
    if (r) {
        fprintf(stderr, "cs submission failed with %d\n", r);
        return;
    }
}

static void bo_map(struct ctx *ctx, struct bo *bo)
{
    struct drm_radeon_gem_mmap args;
    void *ptr;
    int r;

    /* Zero out args to make valgrind happy */
    memset(&args, 0, sizeof(args));
    args.handle = bo->handle;
    args.offset = 0;
    args.size = (uint64_t)bo->size;
    r = drmCommandWriteRead(ctx->fd, DRM_RADEON_GEM_MMAP, &args, sizeof(args));
    if (r) {
        fprintf(stderr, "error mapping %p 0x%08X (error = %d)\n", bo, bo->handle, r);
        exit(-1);
    }
    ptr = mmap(0, args.size, PROT_READ|PROT_WRITE, MAP_SHARED, ctx->fd, args.addr_ptr);
    if (ptr == MAP_FAILED) {
        fprintf(stderr, "%s failed to map bo\n", __func__);
        exit(-1);
    }
    bo->ptr = ptr;
}

static void bo_va(struct ctx *ctx, struct bo *bo)
{
    struct drm_radeon_gem_va args;
    int r;

    args.handle = bo->handle;
    args.vm_id = 0;
    args.operation = RADEON_VA_MAP;
    args.flags = RADEON_VM_PAGE_READABLE | RADEON_VM_PAGE_WRITEABLE | RADEON_VM_PAGE_SNOOPED;
    args.offset = bo->va;
    r = drmCommandWriteRead(ctx->fd, DRM_RADEON_GEM_VA, &args, sizeof(args));
    if (r && args.operation == RADEON_VA_RESULT_ERROR) {
        fprintf(stderr, "radeon: Failed to allocate virtual address for buffer:\n");
        fprintf(stderr, "radeon:    size      : %d bytes\n", bo->size);
        fprintf(stderr, "radeon:    alignment : %d bytes\n", bo->alignment);
        fprintf(stderr, "radeon:    va        : 0x%016llx\n", (unsigned long long)bo->va);
        exit(-1);
    }
}

static struct bo *bo_new(struct ctx *ctx, unsigned ndw, uint32_t *data, uint64_t va, uint32_t alignment)
{
    struct drm_radeon_gem_create args;
    struct bo *bo;
    int r;

    bo = calloc(1, sizeof(*bo));
    if (bo == NULL) {
        fprintf(stderr, "failed to malloc bo struct\n");
        exit(-1);
    }
    bo->size = ndw * 4ULL;
    bo->va = va;
    bo->alignment = alignment;

    args.size = bo->size;
    args.alignment = bo->alignment;
    args.initial_domain = RADEON_GEM_DOMAIN_GTT;
    args.flags = 0;
    args.handle = 0;

    r = drmCommandWriteRead(ctx->fd, DRM_RADEON_GEM_CREATE, &args, sizeof(args));
    bo->handle = args.handle;
    if (r) {
        fprintf(stderr, "Failed to allocate :\n");
        fprintf(stderr, "   size      : %d bytes\n", bo->size);
        fprintf(stderr, "   alignment : %d bytes\n", bo->alignment);
        free(bo);
        exit(-1);
    }

    if (data) {
        bo_map(ctx, bo);
        memcpy(bo->ptr, data, bo->size);
    }

    if (va) {
        bo_va(ctx, bo);
    }

    return bo;
}


#endif
