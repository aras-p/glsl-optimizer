/* 
 * Copyright © 2008 Jérôme Glisse
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
 */
#include <stdio.h>
#include "dri_util.h"
#include "state_tracker/st_public.h"
#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "amd_buffer.h"
#include "amd_screen.h"
#include "amd_context.h"
#include "radeon_bo.h"
#include "radeon_drm.h"

static const char *amd_get_name(struct pipe_winsys *ws)
{
    return "AMD/DRI2";
}

static struct pipe_buffer *amd_buffer_create(struct pipe_winsys *ws,
                                             unsigned alignment,
                                             unsigned usage,
                                             unsigned size)
{
    struct amd_pipe_winsys *amd_ws = (struct amd_pipe_winsys *)ws;
    struct amd_pipe_buffer *amd_buffer;
    uint32_t domain;

    amd_buffer = calloc(1, sizeof(*amd_buffer));
    if (amd_buffer == NULL) {
        return NULL;
    }
    amd_buffer->base.refcount = 1;
    amd_buffer->base.alignment = alignment;
    amd_buffer->base.usage = usage;
    amd_buffer->base.size = size;

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
    amd_buffer->bo = radeon_bo_open(amd_ws->amd_screen->bom, 0,
                                    size, alignment, domain, 0);
    if (amd_buffer->bo == NULL) {
        free(amd_buffer);
    }
    return &amd_buffer->base;
}

static struct pipe_buffer *amd_buffer_user_create(struct pipe_winsys *ws,
                                                  void *ptr,
                                                  unsigned bytes)
{
    struct amd_pipe_buffer *amd_buffer;

    amd_buffer = (struct amd_pipe_buffer*)amd_buffer_create(ws, 0, 0, bytes);
    if (amd_buffer == NULL) {
        return NULL;
    }
    radeon_bo_map(amd_buffer->bo, 1);
    memcpy(amd_buffer->bo->ptr, ptr, bytes);
    radeon_bo_unmap(amd_buffer->bo);
    return &amd_buffer->base;
}

static void amd_buffer_del(struct pipe_winsys *ws, struct pipe_buffer *buffer)
{
    struct amd_pipe_buffer *amd_buffer = (struct amd_pipe_buffer*)buffer;

    radeon_bo_unref(amd_buffer->bo);
    free(amd_buffer);
}

static void *amd_buffer_map(struct pipe_winsys *ws,
                            struct pipe_buffer *buffer,
                            unsigned flags)
{
    struct amd_pipe_buffer *amd_buffer = (struct amd_pipe_buffer*)buffer;
    int write = 0;

    if (flags & PIPE_BUFFER_USAGE_CPU_WRITE) {
        write = 1;
    }
    if (radeon_bo_map(amd_buffer->bo, write))
        return NULL;
    return amd_buffer->bo->ptr;
}

static void amd_buffer_unmap(struct pipe_winsys *ws, struct pipe_buffer *buffer)
{
    struct amd_pipe_buffer *amd_buffer = (struct amd_pipe_buffer*)buffer;

    radeon_bo_unmap(amd_buffer->bo);
}

static void amd_fence_reference(struct pipe_winsys *ws,
                                struct pipe_fence_handle **ptr,
                                struct pipe_fence_handle *pfence)
{
}

static int amd_fence_signalled(struct pipe_winsys *ws,
                               struct pipe_fence_handle *pfence,
                               unsigned flag)
{
    return 1;
}

static int amd_fence_finish(struct pipe_winsys *ws,
                            struct pipe_fence_handle *pfence,
                            unsigned flag)
{
    return 0;
}

static void amd_flush_frontbuffer(struct pipe_winsys *pipe_winsys,
                                  struct pipe_surface *pipe_surface,
                                  void *context_private)
{
    /* TODO: call dri2CopyRegion */
}

struct pipe_winsys *amd_pipe_winsys(struct amd_screen *amd_screen)
{
    struct amd_pipe_winsys *amd_ws;

    amd_ws = calloc(1, sizeof(struct amd_pipe_winsys));
    if (amd_ws == NULL) {
        return NULL;
    }
    amd_ws->amd_screen = amd_screen;

    amd_ws->winsys.flush_frontbuffer = amd_flush_frontbuffer;

    amd_ws->winsys.buffer_create = amd_buffer_create;
    amd_ws->winsys.buffer_destroy = amd_buffer_del;
    amd_ws->winsys.user_buffer_create = amd_buffer_user_create;
    amd_ws->winsys.buffer_map = amd_buffer_map;
    amd_ws->winsys.buffer_unmap = amd_buffer_unmap;

    amd_ws->winsys.fence_reference = amd_fence_reference;
    amd_ws->winsys.fence_signalled = amd_fence_signalled;
    amd_ws->winsys.fence_finish = amd_fence_finish;

    amd_ws->winsys.get_name = amd_get_name;

    return &amd_ws->winsys;
}

static struct pipe_buffer *amd_buffer_from_handle(struct amd_screen *amd_screen,
                                                  uint32_t handle)
{
    struct amd_pipe_buffer *amd_buffer;
    struct radeon_bo *bo = NULL;

    bo = radeon_bo_open(amd_screen->bom, handle, 0, 0, 0, 0);
    if (bo == NULL) {
        return NULL;
    }
    amd_buffer = calloc(1, sizeof(struct amd_pipe_buffer));
    if (amd_buffer == NULL) {
        radeon_bo_unref(bo);
        return NULL;
    }
    amd_buffer->base.refcount = 1;
    amd_buffer->base.usage = PIPE_BUFFER_USAGE_PIXEL;
    amd_buffer->bo = bo;
    return &amd_buffer->base;
}

struct pipe_surface *amd_surface_from_handle(struct amd_context *amd_context,
                                             uint32_t handle,
                                             enum pipe_format format,
                                             int w, int h, int pitch)
{
    struct pipe_screen *pipe_screen = amd_context->pipe_screen;
    struct pipe_winsys *pipe_winsys = amd_context->pipe_winsys;
    struct pipe_texture tmpl;
    struct pipe_surface *ps;
    struct pipe_texture *pt;
    struct pipe_buffer *pb;

    pb = amd_buffer_from_handle(amd_context->amd_screen, handle);
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
        winsys_buffer_reference(pipe_winsys, &pb, NULL);
    }
    ps = pipe_screen->get_tex_surface(pipe_screen, pt, 0, 0, 0,
                                      PIPE_BUFFER_USAGE_GPU_WRITE);
    return ps;
}
