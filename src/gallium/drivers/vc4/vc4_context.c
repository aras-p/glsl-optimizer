/*
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <xf86drm.h>
#include <err.h>
#include <stdio.h>

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_blitter.h"
#include "indices/u_primconvert.h"
#include "pipe/p_screen.h"

#include "vc4_screen.h"
#include "vc4_context.h"
#include "vc4_resource.h"

static void
dump_fbo(struct vc4_context *vc4, struct vc4_bo *fbo)
{
#ifndef USE_VC4_SIMULATOR
        uint32_t *map = vc4_bo_map(fbo);
        uint32_t width = vc4->framebuffer.width;
        uint32_t height = vc4->framebuffer.height;
        uint32_t chunk_w = width / 79;
        uint32_t chunk_h = height / 40;
        uint32_t found_colors[10];
        uint32_t num_found_colors = 0;

        for (int by = 0; by < height; by += chunk_h) {
                for (int bx = 0; bx < width; bx += chunk_w) {
                        bool on = false, black = false;

                        for (int y = by; y < MIN2(height, by + chunk_h); y++) {
                                for (int x = bx; x < MIN2(width, bx + chunk_w); x++) {
                                        uint32_t pix = map[y * width + x];
                                        on |= pix != 0;
                                        black |= pix == 0xff000000;

                                        int i;
                                        for (i = 0; i < num_found_colors; i++) {
                                                if (pix == found_colors[i])
                                                        break;
                                        }
                                        if (i == num_found_colors &&
                                            num_found_colors < Elements(found_colors))
                                                found_colors[num_found_colors++] = pix;
                                }
                        }
                        if (black)
                                fprintf(stderr, "O");
                        else if (on)
                                fprintf(stderr, "X");
                        else
                                fprintf(stderr, ".");
                }
                fprintf(stderr, "\n");
        }

        for (int i = 0; i < num_found_colors; i++) {
                fprintf(stderr, "color %d: 0x%08x\n", i, found_colors[i]);
        }
#endif
}

void
vc4_flush(struct pipe_context *pctx)
{
        struct vc4_context *vc4 = vc4_context(pctx);

        if (!vc4->needs_flush)
                return;

        struct vc4_surface *csurf = vc4_surface(vc4->framebuffer.cbufs[0]);
        struct vc4_resource *ctex = vc4_resource(csurf->base.texture);
        struct drm_vc4_submit_cl submit;
        memset(&submit, 0, sizeof(submit));

        submit.bo_handles = vc4->bo_handles.base;
        submit.bo_handle_count = (vc4->bo_handles.next -
                                  vc4->bo_handles.base) / 4;
        submit.bin_cl = vc4->bcl.base;
        submit.bin_cl_size = vc4->bcl.next - vc4->bcl.base;
        submit.render_cl = vc4->rcl.base;
        submit.render_cl_size = vc4->rcl.next - vc4->rcl.base;
        submit.shader_rec = vc4->shader_rec.base;
        submit.shader_rec_size = vc4->shader_rec.next - vc4->shader_rec.base;
        submit.shader_rec_count = vc4->shader_rec_count;
        submit.uniforms = vc4->uniforms.base;
        submit.uniforms_size = vc4->uniforms.next - vc4->uniforms.base;

        if (!(vc4_debug & VC4_DEBUG_NORAST)) {
                int ret;

#ifndef USE_VC4_SIMULATOR
                ret = drmIoctl(vc4->fd, DRM_IOCTL_VC4_SUBMIT_CL, &submit);
#else
                ret = vc4_simulator_flush(vc4, &submit, csurf);
#endif
                if (ret)
                        errx(1, "VC4 submit failed\n");
        }

        vc4_reset_cl(&vc4->bcl);
        vc4_reset_cl(&vc4->rcl);
        vc4_reset_cl(&vc4->shader_rec);
        vc4_reset_cl(&vc4->uniforms);
        vc4_reset_cl(&vc4->bo_handles);
#ifdef USE_VC4_SIMULATOR
        vc4_reset_cl(&vc4->bo_pointers);
#endif
        vc4->shader_rec_count = 0;

        vc4->needs_flush = false;
        vc4->dirty = ~0;

        dump_fbo(vc4, ctex->bo);
}

static void
vc4_pipe_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
               unsigned flags)
{
        vc4_flush(pctx);
}

static void
vc4_context_destroy(struct pipe_context *pctx)
{
        struct vc4_context *vc4 = vc4_context(pctx);

        if (vc4->blitter)
                util_blitter_destroy(vc4->blitter);

        if (vc4->primconvert)
                util_primconvert_destroy(vc4->primconvert);

        util_slab_destroy(&vc4->transfer_pool);

        free(vc4);
}

struct pipe_context *
vc4_context_create(struct pipe_screen *pscreen, void *priv)
{
        struct vc4_screen *screen = vc4_screen(pscreen);
        struct vc4_context *vc4;

        /* Prevent dumping of the shaders built during context setup. */
        uint32_t saved_shaderdb_flag = vc4_debug & VC4_DEBUG_SHADERDB;
        vc4_debug &= ~VC4_DEBUG_SHADERDB;

        vc4 = CALLOC_STRUCT(vc4_context);
        if (vc4 == NULL)
                return NULL;
        struct pipe_context *pctx = &vc4->base;

        vc4->screen = screen;

        pctx->screen = pscreen;
        pctx->priv = priv;
        pctx->destroy = vc4_context_destroy;
        pctx->flush = vc4_pipe_flush;

        vc4_draw_init(pctx);
        vc4_state_init(pctx);
        vc4_program_init(pctx);
        vc4_resource_context_init(pctx);

        vc4_init_cl(vc4, &vc4->bcl);
        vc4_init_cl(vc4, &vc4->rcl);
        vc4_init_cl(vc4, &vc4->shader_rec);
        vc4_init_cl(vc4, &vc4->bo_handles);

        vc4->dirty = ~0;
        vc4->fd = screen->fd;

        util_slab_create(&vc4->transfer_pool, sizeof(struct pipe_transfer),
                         16, UTIL_SLAB_SINGLETHREADED);
        vc4->blitter = util_blitter_create(pctx);
        if (!vc4->blitter)
                goto fail;

        vc4->primconvert = util_primconvert_create(pctx,
                                                   !((1 << PIPE_PRIM_QUADS) - 1));
        if (!vc4->primconvert)
                goto fail;

        vc4_debug |= saved_shaderdb_flag;

        return &vc4->base;

fail:
        pctx->destroy(pctx);
        return NULL;
}
