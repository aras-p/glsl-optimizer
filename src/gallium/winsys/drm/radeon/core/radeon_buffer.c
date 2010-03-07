/*
 * Copyright © 2008 Jérôme Glisse
 *             2009 Corbin Simpson
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
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 */

#include "radeon_buffer.h"
#include "radeon_drm.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "radeon_bo_gem.h"
#include <X11/Xutil.h>

struct radeon_vl_context
{
    Display *display;
    int screen;
    Drawable drawable;
};

static const char *radeon_get_name(struct pipe_winsys *ws)
{
    return "Radeon/GEM+KMS";
}

static uint32_t radeon_domain_from_usage(unsigned usage)
{
    uint32_t domain = 0;

    if (usage & PIPE_BUFFER_USAGE_GPU_WRITE) {
        domain |= RADEON_GEM_DOMAIN_VRAM;
    }
    if (usage & PIPE_BUFFER_USAGE_PIXEL) {
        domain |= RADEON_GEM_DOMAIN_VRAM;
    }
    if (usage & PIPE_BUFFER_USAGE_VERTEX) {
        domain |= RADEON_GEM_DOMAIN_GTT;
    }
    if (usage & PIPE_BUFFER_USAGE_INDEX) {
        domain |= RADEON_GEM_DOMAIN_GTT;
    }

    return domain;
}

static struct pipe_buffer *radeon_buffer_create(struct pipe_winsys *ws,
                                                unsigned alignment,
                                                unsigned usage,
                                                unsigned size)
{
    struct radeon_winsys *radeon_ws = (struct radeon_winsys *)ws;
    struct radeon_pipe_buffer *radeon_buffer;
    struct pb_desc desc;
    uint32_t domain;

    radeon_buffer = CALLOC_STRUCT(radeon_pipe_buffer);
    if (radeon_buffer == NULL) {
        return NULL;
    }

    pipe_reference_init(&radeon_buffer->base.reference, 1);
    radeon_buffer->base.alignment = alignment;
    radeon_buffer->base.usage = usage;
    radeon_buffer->base.size = size;

    if (usage & PIPE_BUFFER_USAGE_CONSTANT && is_r3xx(radeon_ws->pci_id)) {
        /* Don't bother allocating a BO, as it'll never get to the card. */
        desc.alignment = alignment;
        desc.usage = usage;
        radeon_buffer->pb = pb_malloc_buffer_create(size, &desc);
        return &radeon_buffer->base;
    }

    domain = radeon_domain_from_usage(usage);

    radeon_buffer->bo = radeon_bo_open(radeon_ws->priv->bom, 0, size,
            alignment, domain, 0);
    if (radeon_buffer->bo == NULL) {
        FREE(radeon_buffer);
        return NULL;
    }
    return &radeon_buffer->base;
}

static struct pipe_buffer *radeon_buffer_user_create(struct pipe_winsys *ws,
                                                     void *ptr,
                                                     unsigned bytes)
{
    struct radeon_pipe_buffer *radeon_buffer;

    radeon_buffer =
        (struct radeon_pipe_buffer*)radeon_buffer_create(ws, 0, 0, bytes);
    if (radeon_buffer == NULL) {
        return NULL;
    }
    radeon_bo_map(radeon_buffer->bo, 1);
    memcpy(radeon_buffer->bo->ptr, ptr, bytes);
    radeon_bo_unmap(radeon_buffer->bo);
    return &radeon_buffer->base;
}

static struct pipe_buffer *radeon_surface_buffer_create(struct pipe_winsys *ws,
                                                        unsigned width,
                                                        unsigned height,
                                                        enum pipe_format format,
                                                        unsigned usage,
                                                        unsigned tex_usage,
                                                        unsigned *stride)
{
    /* Radeons enjoy things in multiples of 32. */
    /* XXX this can be 32 when POT */
    const unsigned alignment = 64;
    unsigned nblocksy, size;

    nblocksy = util_format_get_nblocksy(format, height);
    *stride = align(util_format_get_stride(format, width), alignment);
    size = *stride * nblocksy;

    return radeon_buffer_create(ws, 64, usage, size);
}

static void radeon_buffer_del(struct pipe_buffer *buffer)
{
    struct radeon_pipe_buffer *radeon_buffer =
        (struct radeon_pipe_buffer*)buffer;

    if (radeon_buffer->pb) {
        pipe_reference_init(&radeon_buffer->pb->base.reference, 0);
        pb_destroy(radeon_buffer->pb);
    }

    if (radeon_buffer->bo) {
        radeon_bo_unref(radeon_buffer->bo);
    }

    FREE(radeon_buffer);
}

static void *radeon_buffer_map(struct pipe_winsys *ws,
                               struct pipe_buffer *buffer,
                               unsigned flags)
{
    struct radeon_winsys_priv *priv = ((struct radeon_winsys *)ws)->priv;
    struct radeon_pipe_buffer *radeon_buffer =
        (struct radeon_pipe_buffer*)buffer;
    int write = 0;

    if (radeon_buffer->pb) {
        return pb_map(radeon_buffer->pb, flags);
    }

    if (flags & PIPE_BUFFER_USAGE_DONTBLOCK) {
        uint32_t domain;

        if (radeon_bo_is_busy(radeon_buffer->bo, &domain))
            return NULL;
    }

    if (radeon_bo_is_referenced_by_cs(radeon_buffer->bo, priv->cs)) {
        priv->flush_cb(priv->flush_data);
    }

    if (flags & PIPE_BUFFER_USAGE_CPU_WRITE) {
        write = 1;
    }

    if (radeon_bo_map(radeon_buffer->bo, write)) {
        return NULL;
    }

    return radeon_buffer->bo->ptr;
}

static void radeon_buffer_unmap(struct pipe_winsys *ws,
                                struct pipe_buffer *buffer)
{
    struct radeon_pipe_buffer *radeon_buffer =
        (struct radeon_pipe_buffer*)buffer;

    if (radeon_buffer->pb) {
        pb_unmap(radeon_buffer->pb);
    } else {
        radeon_bo_unmap(radeon_buffer->bo);
    }
}

static void radeon_buffer_set_tiling(struct radeon_winsys *ws,
                                     struct pipe_buffer *buffer,
                                     uint32_t pitch,
                                     boolean microtiled,
                                     boolean macrotiled)
{
    struct radeon_winsys_priv *priv = ((struct radeon_winsys *)ws)->priv;
    struct radeon_pipe_buffer *radeon_buffer =
        (struct radeon_pipe_buffer*)buffer;
    uint32_t flags = 0, old_flags, old_pitch;

    if (microtiled) {
        flags |= RADEON_BO_FLAGS_MICRO_TILE;
    }
    if (macrotiled) {
        flags |= RADEON_BO_FLAGS_MACRO_TILE;
    }

    radeon_bo_get_tiling(radeon_buffer->bo, &old_flags, &old_pitch);

    if (flags != old_flags || pitch != old_pitch) {
        /* Tiling determines how DRM treats the buffer data.
         * We must flush CS when changing it if the buffer is referenced. */
        if (radeon_bo_is_referenced_by_cs(radeon_buffer->bo, priv->cs)) {
            priv->flush_cb(priv->flush_data);
        }

        radeon_bo_set_tiling(radeon_buffer->bo, flags, pitch);
    }
}

static void radeon_fence_reference(struct pipe_winsys *ws,
                                   struct pipe_fence_handle **ptr,
                                   struct pipe_fence_handle *pfence)
{
}

static int radeon_fence_signalled(struct pipe_winsys *ws,
                                  struct pipe_fence_handle *pfence,
                                  unsigned flag)
{
    return 1;
}

static int radeon_fence_finish(struct pipe_winsys *ws,
                               struct pipe_fence_handle *pfence,
                               unsigned flag)
{
    return 0;
}

struct radeon_winsys* radeon_pipe_winsys(int fd)
{
    struct radeon_winsys* radeon_ws;

    radeon_ws = CALLOC_STRUCT(radeon_winsys);
    if (radeon_ws == NULL) {
        return NULL;
    }

    radeon_ws->priv = CALLOC_STRUCT(radeon_winsys_priv);
    if (radeon_ws->priv == NULL) {
        FREE(radeon_ws);
        return NULL;
    }

    radeon_ws->priv->fd = fd;
    radeon_ws->priv->bom = radeon_bo_manager_gem_ctor(fd);

    radeon_ws->base.flush_frontbuffer = NULL; /* overriden by co-state tracker */

    radeon_ws->base.buffer_create = radeon_buffer_create;
    radeon_ws->base.user_buffer_create = radeon_buffer_user_create;
    radeon_ws->base.surface_buffer_create = radeon_surface_buffer_create;
    radeon_ws->base.buffer_map = radeon_buffer_map;
    radeon_ws->base.buffer_unmap = radeon_buffer_unmap;
    radeon_ws->base.buffer_destroy = radeon_buffer_del;

    radeon_ws->base.fence_reference = radeon_fence_reference;
    radeon_ws->base.fence_signalled = radeon_fence_signalled;
    radeon_ws->base.fence_finish = radeon_fence_finish;

    radeon_ws->base.get_name = radeon_get_name;

    radeon_ws->buffer_set_tiling = radeon_buffer_set_tiling;

    return radeon_ws;
}
