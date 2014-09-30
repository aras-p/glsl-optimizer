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

#include "pipe/p_defines.h"
#include "util/ralloc.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_blitter.h"
#include "indices/u_primconvert.h"
#include "pipe/p_screen.h"

#include "vc4_screen.h"
#include "vc4_context.h"
#include "vc4_resource.h"

static void
vc4_setup_rcl(struct vc4_context *vc4)
{
        struct vc4_surface *csurf = vc4_surface(vc4->framebuffer.cbufs[0]);
        struct vc4_resource *ctex = csurf ? vc4_resource(csurf->base.texture) : NULL;
        struct vc4_surface *zsurf = vc4_surface(vc4->framebuffer.zsbuf);
        struct vc4_resource *ztex = zsurf ? vc4_resource(zsurf->base.texture) : NULL;

        if (!csurf)
                vc4->resolve &= ~PIPE_CLEAR_COLOR0;
        if (!zsurf)
                vc4->resolve &= ~(PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL);
        uint32_t resolve_uncleared = vc4->resolve & ~vc4->cleared;
        uint32_t width = vc4->framebuffer.width;
        uint32_t height = vc4->framebuffer.height;
        uint32_t xtiles = align(width, 64) / 64;
        uint32_t ytiles = align(height, 64) / 64;

#if 0
        fprintf(stderr, "RCL: resolve 0x%x clear 0x%x resolve uncleared 0x%x\n",
                vc4->resolve,
                vc4->cleared,
                resolve_uncleared);
#endif

        cl_u8(&vc4->rcl, VC4_PACKET_CLEAR_COLORS);
        cl_u32(&vc4->rcl, vc4->clear_color[0]);
        cl_u32(&vc4->rcl, vc4->clear_color[1]);
        cl_u32(&vc4->rcl, vc4->clear_depth);
        cl_u8(&vc4->rcl, vc4->clear_stencil);

        /* The rendering mode config determines the pointer that's used for
         * VC4_PACKET_STORE_MS_TILE_BUFFER address computations.  The kernel
         * could handle a no-relocation rendering mode config and deny those
         * packets, but instead we just tell the kernel we're doing our color
         * rendering to the Z buffer, and just don't emit any of those
         * packets.
         */
        struct vc4_surface *render_surf = csurf ? csurf : zsurf;
        struct vc4_resource *render_tex = vc4_resource(render_surf->base.texture);

        cl_start_reloc(&vc4->rcl, 1);
        cl_u8(&vc4->rcl, VC4_PACKET_TILE_RENDERING_MODE_CONFIG);
        cl_reloc(vc4, &vc4->rcl, render_tex->bo, render_surf->offset);
        cl_u16(&vc4->rcl, width);
        cl_u16(&vc4->rcl, height);
        cl_u16(&vc4->rcl, ((render_surf->tiling <<
                            VC4_RENDER_CONFIG_MEMORY_FORMAT_SHIFT) |
                           (vc4_rt_format_is_565(render_surf->base.format) ?
                            VC4_RENDER_CONFIG_FORMAT_BGR565 :
                            VC4_RENDER_CONFIG_FORMAT_RGBA8888) |
                           VC4_RENDER_CONFIG_EARLY_Z_COVERAGE_DISABLE));

        /* The tile buffer normally gets cleared when the previous tile is
         * stored.  If the clear values changed between frames, then the tile
         * buffer has stale clear values in it, so we have to do a store in
         * None mode (no writes) so that we trigger the tile buffer clear.
         *
         * Excess clearing is only a performance cost, since per-tile contents
         * will be loaded/stored in the loop below.
         */
        if (vc4->cleared & (PIPE_CLEAR_COLOR0 |
                            PIPE_CLEAR_DEPTH |
                            PIPE_CLEAR_STENCIL)) {
                cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
                cl_u8(&vc4->rcl, 0);
                cl_u8(&vc4->rcl, 0);

                cl_u8(&vc4->rcl, VC4_PACKET_STORE_TILE_BUFFER_GENERAL);
                cl_u16(&vc4->rcl, VC4_LOADSTORE_TILE_BUFFER_NONE);
                cl_u32(&vc4->rcl, 0); /* no address, since we're in None mode */
        }

        for (int y = 0; y < ytiles; y++) {
                for (int x = 0; x < xtiles; x++) {
                        bool end_of_frame = (x == xtiles - 1 &&
                                             y == ytiles - 1);
                        bool coords_emitted = false;

                        /* Note that the load doesn't actually occur until the
                         * tile coords packet is processed.
                         */
                        if (resolve_uncleared & PIPE_CLEAR_COLOR) {
                                cl_start_reloc(&vc4->rcl, 1);
                                cl_u8(&vc4->rcl, VC4_PACKET_LOAD_TILE_BUFFER_GENERAL);
                                cl_u8(&vc4->rcl,
                                      VC4_LOADSTORE_TILE_BUFFER_COLOR |
                                      (csurf->tiling <<
                                       VC4_LOADSTORE_TILE_BUFFER_FORMAT_SHIFT));
                                cl_u8(&vc4->rcl,
                                      vc4_rt_format_is_565(csurf->base.format) ?
                                      VC4_LOADSTORE_TILE_BUFFER_BGR565 :
                                      VC4_LOADSTORE_TILE_BUFFER_RGBA8888);
                                cl_reloc(vc4, &vc4->rcl, ctex->bo,
                                         csurf->offset);

                                cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
                                cl_u8(&vc4->rcl, x);
                                cl_u8(&vc4->rcl, y);
                                coords_emitted = true;
                        }

                        if (resolve_uncleared & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
                                cl_start_reloc(&vc4->rcl, 1);
                                cl_u8(&vc4->rcl, VC4_PACKET_LOAD_TILE_BUFFER_GENERAL);
                                cl_u8(&vc4->rcl,
                                      VC4_LOADSTORE_TILE_BUFFER_ZS |
                                      (zsurf->tiling <<
                                       VC4_LOADSTORE_TILE_BUFFER_FORMAT_SHIFT));
                                cl_u8(&vc4->rcl, 0);
                                cl_reloc(vc4, &vc4->rcl, ztex->bo,
                                         zsurf->offset);

                                cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
                                cl_u8(&vc4->rcl, x);
                                cl_u8(&vc4->rcl, y);
                                coords_emitted = true;
                        }

                        /* Clipping depends on tile coordinates having been
                         * emitted, so make sure it's happened even if
                         * everything was cleared to start.
                         */
                        if (!coords_emitted) {
                                cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
                                cl_u8(&vc4->rcl, x);
                                cl_u8(&vc4->rcl, y);
                        }

                        cl_start_reloc(&vc4->rcl, 1);
                        cl_u8(&vc4->rcl, VC4_PACKET_BRANCH_TO_SUB_LIST);
                        cl_reloc(vc4, &vc4->rcl, vc4->tile_alloc,
                                 (y * xtiles + x) * 32);

                        if (vc4->resolve & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
                                cl_start_reloc(&vc4->rcl, 1);
                                cl_u8(&vc4->rcl, VC4_PACKET_STORE_TILE_BUFFER_GENERAL);
                                cl_u8(&vc4->rcl,
                                      VC4_LOADSTORE_TILE_BUFFER_ZS |
                                      (zsurf->tiling <<
                                       VC4_LOADSTORE_TILE_BUFFER_FORMAT_SHIFT));
                                cl_u8(&vc4->rcl,
                                      VC4_STORE_TILE_BUFFER_DISABLE_COLOR_CLEAR);
                                cl_reloc(vc4, &vc4->rcl, ztex->bo,
                                         zsurf->offset |
                                         ((end_of_frame &&
                                           !(vc4->resolve & PIPE_CLEAR_COLOR0)) ?
                                          VC4_LOADSTORE_TILE_BUFFER_EOF : 0));
                        }

                        if (vc4->resolve & PIPE_CLEAR_COLOR0) {
                                if (end_of_frame) {
                                        cl_u8(&vc4->rcl,
                                              VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF);
                                } else {
                                        cl_u8(&vc4->rcl,
                                              VC4_PACKET_STORE_MS_TILE_BUFFER);
                                }
                        }

                        /* One of the bits needs to have been set that would
                         * have triggered an EOFq
                         */
                        assert(vc4->resolve & (PIPE_CLEAR_COLOR0 |
                                               PIPE_CLEAR_DEPTH |
                                               PIPE_CLEAR_STENCIL));
                }
        }
}

void
vc4_flush(struct pipe_context *pctx)
{
        struct vc4_context *vc4 = vc4_context(pctx);

        if (!vc4->needs_flush)
                return;

        cl_u8(&vc4->bcl, VC4_PACKET_FLUSH_ALL);
        cl_u8(&vc4->bcl, VC4_PACKET_NOP);
        cl_u8(&vc4->bcl, VC4_PACKET_HALT);

        vc4_setup_rcl(vc4);

        if (vc4_debug & VC4_DEBUG_CL) {
                fprintf(stderr, "BCL:\n");
                vc4_dump_cl(&vc4->bcl, false);
                fprintf(stderr, "RCL:\n");
                vc4_dump_cl(&vc4->rcl, true);
        }

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
                ret = vc4_simulator_flush(vc4, &submit);
#endif
                if (ret) {
                        fprintf(stderr, "VC4 submit failed\n");
                        abort();
                }
        }

        vc4_reset_cl(&vc4->bcl);
        vc4_reset_cl(&vc4->rcl);
        vc4_reset_cl(&vc4->shader_rec);
        vc4_reset_cl(&vc4->uniforms);
        vc4_reset_cl(&vc4->bo_handles);
        struct vc4_bo **referenced_bos = vc4->bo_pointers.base;
        for (int i = 0; i < submit.bo_handle_count; i++)
                vc4_bo_unreference(&referenced_bos[i]);
        vc4_reset_cl(&vc4->bo_pointers);
        vc4->shader_rec_count = 0;

        vc4->needs_flush = false;
        vc4->draw_call_queued = false;
        vc4->dirty = ~0;
        vc4->resolve = 0;
        vc4->cleared = 0;
}

static void
vc4_pipe_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
               unsigned flags)
{
        vc4_flush(pctx);
}

/**
 * Flushes the current command lists if they reference the given BO.
 *
 * This helps avoid flushing the command buffers when unnecessary.
 */
void
vc4_flush_for_bo(struct pipe_context *pctx, struct vc4_bo *bo)
{
        struct vc4_context *vc4 = vc4_context(pctx);

        if (!vc4->needs_flush)
                return;

        /* Walk all the referenced BOs in the drawing command list to see if
         * they match.
         */
        struct vc4_bo **referenced_bos = vc4->bo_pointers.base;
        for (int i = 0; i < (vc4->bo_handles.next -
                             vc4->bo_handles.base) / 4; i++) {
                if (referenced_bos[i] == bo) {
                        vc4_flush(pctx);
                        return;
                }
        }

        /* Also check for the Z/color buffers, since the references to those
         * are only added immediately before submit.
         */
        struct vc4_surface *csurf = vc4_surface(vc4->framebuffer.cbufs[0]);
        if (csurf) {
                struct vc4_resource *ctex = vc4_resource(csurf->base.texture);
                if (ctex->bo == bo) {
                        vc4_flush(pctx);
                        return;
                }
        }

        struct vc4_surface *zsurf = vc4_surface(vc4->framebuffer.zsbuf);
        if (zsurf) {
                struct vc4_resource *ztex =
                        vc4_resource(zsurf->base.texture);
                if (ztex->bo == bo) {
                        vc4_flush(pctx);
                        return;
                }
        }
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

        ralloc_free(vc4);
}

struct pipe_context *
vc4_context_create(struct pipe_screen *pscreen, void *priv)
{
        struct vc4_screen *screen = vc4_screen(pscreen);
        struct vc4_context *vc4;

        /* Prevent dumping of the shaders built during context setup. */
        uint32_t saved_shaderdb_flag = vc4_debug & VC4_DEBUG_SHADERDB;
        vc4_debug &= ~VC4_DEBUG_SHADERDB;

        vc4 = rzalloc(NULL, struct vc4_context);
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
        vc4_query_init(pctx);
        vc4_resource_context_init(pctx);

        vc4_init_cl(vc4, &vc4->bcl);
        vc4_init_cl(vc4, &vc4->rcl);
        vc4_init_cl(vc4, &vc4->shader_rec);
        vc4_init_cl(vc4, &vc4->bo_handles);

        vc4->dirty = ~0;
        vc4->fd = screen->fd;

        util_slab_create(&vc4->transfer_pool, sizeof(struct vc4_transfer),
                         16, UTIL_SLAB_SINGLETHREADED);
        vc4->blitter = util_blitter_create(pctx);
        if (!vc4->blitter)
                goto fail;

        vc4->primconvert = util_primconvert_create(pctx,
                                                   (1 << PIPE_PRIM_QUADS) - 1);
        if (!vc4->primconvert)
                goto fail;

        vc4_debug |= saved_shaderdb_flag;

        return &vc4->base;

fail:
        pctx->destroy(pctx);
        return NULL;
}
