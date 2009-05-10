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

static const char *radeon_get_name(struct pipe_winsys *ws)
{
    return "Radeon/GEM+KMS";
}

static struct pipe_buffer *radeon_buffer_create(struct pipe_winsys *ws,
                                                unsigned alignment,
                                                unsigned usage,
                                                unsigned size)
{
    struct radeon_winsys *radeon_ws = (struct radeon_winsys *)ws;
    struct radeon_pipe_buffer *radeon_buffer;
    uint32_t domain;

    radeon_buffer = CALLOC_STRUCT(radeon_pipe_buffer);
    if (radeon_buffer == NULL) {
        return NULL;
    }

    pipe_reference_init(&radeon_buffer->base.reference, 1);
    radeon_buffer->base.alignment = alignment;
    radeon_buffer->base.usage = usage;
    radeon_buffer->base.size = size;

    domain = 0;

    if (usage & PIPE_BUFFER_USAGE_PIXEL) {
        domain |= RADEON_GEM_DOMAIN_VRAM;
    }
    if (usage & PIPE_BUFFER_USAGE_VERTEX) {
        domain |= RADEON_GEM_DOMAIN_GTT;
    }
    if (usage & PIPE_BUFFER_USAGE_INDEX) {
        domain |= RADEON_GEM_DOMAIN_GTT;
    }

    radeon_buffer->bo = radeon_bo_open(radeon_ws->priv->bom, 0, size,
            alignment, domain, 0);
    if (radeon_buffer->bo == NULL) {
        FREE(radeon_buffer);
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

static void radeon_buffer_del(struct pipe_buffer *buffer)
{
    struct radeon_pipe_buffer *radeon_buffer =
        (struct radeon_pipe_buffer*)buffer;

    radeon_bo_unref(radeon_buffer->bo);
    free(radeon_buffer);
}

static void *radeon_buffer_map(struct pipe_winsys *ws,
                               struct pipe_buffer *buffer,
                               unsigned flags)
{
    struct radeon_pipe_buffer *radeon_buffer =
        (struct radeon_pipe_buffer*)buffer;
    int write = 0;

    if (flags & PIPE_BUFFER_USAGE_DONTBLOCK) {
       /* XXX Remove this when radeon_bo_map supports DONTBLOCK */
       return NULL;
    }
    if (flags & PIPE_BUFFER_USAGE_CPU_WRITE) {
        write = 1;
    }

    if (radeon_bo_map(radeon_buffer->bo, write))
        return NULL;
    return radeon_buffer->bo->ptr;
}

static void radeon_buffer_unmap(struct pipe_winsys *ws,
                                struct pipe_buffer *buffer)
{
    struct radeon_pipe_buffer *radeon_buffer =
        (struct radeon_pipe_buffer*)buffer;

    radeon_bo_unmap(radeon_buffer->bo);
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

static void radeon_flush_frontbuffer(struct pipe_winsys *pipe_winsys,
                                     struct pipe_surface *pipe_surface,
                                     void *context_private)
{
    /* XXX TODO: call dri2CopyRegion */
}

struct radeon_winsys* radeon_pipe_winsys(int fd)
{
    struct radeon_winsys* radeon_ws;
    struct radeon_bo_manager* bom;

    radeon_ws = CALLOC_STRUCT(radeon_winsys);
    if (radeon_ws == NULL) {
        return NULL;
    }

    radeon_ws->priv = CALLOC_STRUCT(radeon_winsys_priv);
    if (radeon_ws->priv == NULL) {
        FREE(radeon_ws);
        return NULL;
    }

    radeon_ws->priv->bom = radeon_bo_manager_gem_ctor(fd);

    radeon_ws->base.flush_frontbuffer = radeon_flush_frontbuffer;

    radeon_ws->base.buffer_create = radeon_buffer_create;
    radeon_ws->base.buffer_destroy = radeon_buffer_del;
    radeon_ws->base.user_buffer_create = radeon_buffer_user_create;
    radeon_ws->base.buffer_map = radeon_buffer_map;
    radeon_ws->base.buffer_unmap = radeon_buffer_unmap;

    radeon_ws->base.fence_reference = radeon_fence_reference;
    radeon_ws->base.fence_signalled = radeon_fence_signalled;
    radeon_ws->base.fence_finish = radeon_fence_finish;

    radeon_ws->base.get_name = radeon_get_name;

    return radeon_ws;
}
#if 0
static struct pipe_buffer *radeon_buffer_from_handle(struct radeon_screen *radeon_screen,
                                                  uint32_t handle)
{
    struct radeon_pipe_buffer *radeon_buffer;
    struct radeon_bo *bo = NULL;

    bo = radeon_bo_open(radeon_screen->bom, handle, 0, 0, 0, 0);
    if (bo == NULL) {
        return NULL;
    }
    radeon_buffer = calloc(1, sizeof(struct radeon_pipe_buffer));
    if (radeon_buffer == NULL) {
        radeon_bo_unref(bo);
        return NULL;
    }
    pipe_reference_init(&radeon_buffer->base.reference, 1);
    radeon_buffer->base.usage = PIPE_BUFFER_USAGE_PIXEL;
    radeon_buffer->bo = bo;
    return &radeon_buffer->base;
}

struct pipe_surface *radeon_surface_from_handle(struct radeon_context *radeon_context,
                                             uint32_t handle,
                                             enum pipe_format format,
                                             int w, int h, int pitch)
{
    struct pipe_screen *pipe_screen = radeon_context->pipe_screen;
    struct pipe_winsys *pipe_winsys = radeon_context->pipe_winsys;
    struct pipe_texture tmpl;
    struct pipe_surface *ps;
    struct pipe_texture *pt;
    struct pipe_buffer *pb;

    pb = radeon_buffer_from_handle(radeon_context->radeon_screen, handle);
    if (pb == NULL) {
        return NULL;
    }
    memset(&tmpl, 0, sizeof(tmpl));
    tmpl.tex_usage = PIPE_TEXTURE_USAGE_DISPLAY_TARGET;
    tmpl.target = PIPE_TEXTURE_2D;
    tmpl.width[0] = w;
    tmpl.height[0] = h;
    tmpl.depth[0] = 1;
    tmpl.format = format;
    pf_get_block(tmpl.format, &tmpl.block);
    tmpl.nblocksx[0] = pf_get_nblocksx(&tmpl.block, w);
    tmpl.nblocksy[0] = pf_get_nblocksy(&tmpl.block, h);

    pt = pipe_screen->texture_blanket(pipe_screen, &tmpl, &pitch, pb);
    if (pt == NULL) {
        pipe_buffer_reference(&pb, NULL);
    }
    ps = pipe_screen->get_tex_surface(pipe_screen, pt, 0, 0, 0,
                                      PIPE_BUFFER_USAGE_GPU_WRITE);
    return ps;
}
#endif
