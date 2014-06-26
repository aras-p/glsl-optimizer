/*
 * Copyright (c) 2014 Scott Mansell
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

#include <stdio.h>

#include "vc4_context.h"
#include "vc4_resource.h"

static struct vc4_bo *
get_vbo(struct vc4_context *vc4, uint32_t width, uint32_t height)
{
        struct {
                float x, y, z, w;
        } verts[] = {
                { -1, -1, 1, 1 },
                {  1, -1, 1, 1 },
                { -1,  1, 1, 1 },
        };

        return vc4_bo_alloc_mem(vc4->screen, verts, sizeof(verts), "verts");
}
static struct vc4_bo *
get_ibo(struct vc4_context *vc4)
{
        static const uint8_t indices[] = { 0, 1, 2 };

        return vc4_bo_alloc_mem(vc4->screen, indices, sizeof(indices), "indices");
}

static void
vc4_rcl_tile_calls(struct vc4_context *vc4,
                   uint32_t xtiles, uint32_t ytiles,
                   struct vc4_bo *tile_alloc)
{
        for (int x = 0; x < xtiles; x++) {
                for (int y = 0; y < ytiles; y++) {
                        cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
                        cl_u8(&vc4->rcl, x);
                        cl_u8(&vc4->rcl, y);

                        cl_start_reloc(&vc4->rcl, 1);
                        cl_u8(&vc4->rcl, VC4_PACKET_BRANCH_TO_SUB_LIST);
                        cl_reloc(vc4, &vc4->rcl, tile_alloc,
                                 (y * xtiles + x) * 32);

                        if (x == xtiles - 1 && y == ytiles - 1) {
                                cl_u8(&vc4->rcl,
                                      VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF);
                        } else {
                                cl_u8(&vc4->rcl,
                                      VC4_PACKET_STORE_MS_TILE_BUFFER);
                        }
                }
        }
}

static void
vc4_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info)
{
        struct vc4_context *vc4 = vc4_context(pctx);
        uint32_t width = vc4->framebuffer.width;
        uint32_t height = vc4->framebuffer.height;
        uint32_t tilew = align(width, 64) / 64;
        uint32_t tileh = align(height, 64) / 64;
        struct vc4_bo *tile_alloc = vc4_bo_alloc(vc4->screen,
                                                 32 * tilew * tileh, "tilea");
        struct vc4_bo *tile_state = vc4_bo_alloc(vc4->screen,
                                                 48 * tilew * tileh, "tilestate");
        struct vc4_bo *ibo = get_ibo(vc4);

        struct vc4_bo *vbo = get_vbo(vc4, width, height);

        vc4->needs_flush = true;

        //   Tile state data is 48 bytes per tile, I think it can be thrown away
        //   as soon as binning is finished.
        cl_start_reloc(&vc4->bcl, 2);
        cl_u8(&vc4->bcl, VC4_PACKET_TILE_BINNING_MODE_CONFIG);
        cl_reloc(vc4, &vc4->bcl, tile_alloc, 0);
        cl_u32(&vc4->bcl, 0x8000); /* tile allocation memory size */
        cl_reloc(vc4, &vc4->bcl, tile_state, 0);
        cl_u8(&vc4->bcl, tilew);
        cl_u8(&vc4->bcl, tileh);
        cl_u8(&vc4->bcl, VC4_BIN_CONFIG_AUTO_INIT_TSDA);

        cl_u8(&vc4->bcl, VC4_PACKET_START_TILE_BINNING);

        cl_u8(&vc4->bcl, VC4_PACKET_PRIMITIVE_LIST_FORMAT);
        cl_u8(&vc4->bcl, 0x12); // 16 bit triangle

        vc4_emit_state(pctx);

        /* the actual draw call. */
        uint32_t nr_attributes = 1;
        cl_u8(&vc4->bcl, VC4_PACKET_GL_SHADER_STATE);
#ifndef USE_VC4_SIMULATOR
        cl_u32(&vc4->bcl, nr_attributes & 0x7); /* offset into shader_rec */
#else
        cl_u32(&vc4->bcl, simpenrose_hw_addr(vc4->shader_rec.next) |
               (nr_attributes & 0x7));
#endif

        cl_start_reloc(&vc4->bcl, 1);
        cl_u8(&vc4->bcl, VC4_PACKET_GL_INDEXED_PRIMITIVE);
        cl_u8(&vc4->bcl, 0x04); // 8bit index, trinagles
        cl_u32(&vc4->bcl, 3); // Length
        cl_reloc(vc4, &vc4->bcl, ibo, 0);
        cl_u32(&vc4->bcl, 2); // Maximum index

        cl_u8(&vc4->bcl, VC4_PACKET_FLUSH_ALL);
        cl_u8(&vc4->bcl, VC4_PACKET_NOP);
        cl_u8(&vc4->bcl, VC4_PACKET_HALT);

// Shader Record

        struct vc4_bo *fs_ubo, *vs_ubo, *cs_ubo;
        uint32_t fs_ubo_offset, vs_ubo_offset, cs_ubo_offset;
        vc4_get_uniform_bo(vc4, vc4->prog.fs,
                           &vc4->constbuf[PIPE_SHADER_FRAGMENT],
                           0, &fs_ubo, &fs_ubo_offset);
        vc4_get_uniform_bo(vc4, vc4->prog.vs,
                           &vc4->constbuf[PIPE_SHADER_VERTEX],
                           0, &vs_ubo, &vs_ubo_offset);
        vc4_get_uniform_bo(vc4, vc4->prog.vs,
                           &vc4->constbuf[PIPE_SHADER_VERTEX],
                           1, &cs_ubo, &cs_ubo_offset);

        cl_start_shader_reloc(&vc4->shader_rec, 7);
        cl_u16(&vc4->shader_rec, VC4_SHADER_FLAG_ENABLE_CLIPPING);
        cl_u8(&vc4->shader_rec, 0); /* fs num uniforms (unused) */
        cl_u8(&vc4->shader_rec, 0); /* fs num varyings */
        cl_reloc(vc4, &vc4->shader_rec, vc4->prog.fs->bo, 0);
        cl_reloc(vc4, &vc4->shader_rec, fs_ubo, fs_ubo_offset);

        cl_u16(&vc4->shader_rec, 0); /* vs num uniforms */
        cl_u8(&vc4->shader_rec, 1); /* vs attribute array bitfield */
        cl_u8(&vc4->shader_rec, 16); /* vs total attribute size */
        cl_reloc(vc4, &vc4->shader_rec, vc4->prog.vs->bo, 0);
        cl_reloc(vc4, &vc4->shader_rec, vs_ubo, vs_ubo_offset);

        cl_u16(&vc4->shader_rec, 0); /* cs num uniforms */
        cl_u8(&vc4->shader_rec, 1); /* cs attribute array bitfield */
        cl_u8(&vc4->shader_rec, 16); /* vs total attribute size */
        cl_reloc(vc4, &vc4->shader_rec, vc4->prog.vs->bo,
                vc4->prog.vs->coord_shader_offset);
        cl_reloc(vc4, &vc4->shader_rec, cs_ubo, cs_ubo_offset);

        cl_reloc(vc4, &vc4->shader_rec, vbo, 0);
        cl_u8(&vc4->shader_rec, 15); /* bytes - 1 in the attribute*/
        cl_u8(&vc4->shader_rec, 16); /* attribute stride */
        cl_u8(&vc4->shader_rec, 0); /* VS VPM offset */
        cl_u8(&vc4->shader_rec, 0); /* CS VPM offset */

        vc4->shader_rec_count++;

        cl_u8(&vc4->rcl, VC4_PACKET_CLEAR_COLORS);
        cl_u32(&vc4->rcl, 0xff000000); // Opaque Black
        cl_u32(&vc4->rcl, 0xff000000); // 32 bit clear colours need to be repeated twice
        cl_u32(&vc4->rcl, 0);
        cl_u8(&vc4->rcl, 0);

        struct vc4_surface *csurf = vc4_surface(vc4->framebuffer.cbufs[0]);
        struct vc4_resource *ctex = vc4_resource(csurf->base.texture);

        cl_start_reloc(&vc4->rcl, 1);
        cl_u8(&vc4->rcl, VC4_PACKET_TILE_RENDERING_MODE_CONFIG);
        cl_reloc(vc4, &vc4->rcl, ctex->bo, csurf->offset);
        cl_u16(&vc4->rcl, width);
        cl_u16(&vc4->rcl, height);
        cl_u8(&vc4->rcl, (VC4_RENDER_CONFIG_MEMORY_FORMAT_LINEAR |
                          VC4_RENDER_CONFIG_FORMAT_RGBA8888));
        cl_u8(&vc4->rcl, 0);

        // Do a store of the first tile to force the tile buffer to be cleared
        /* XXX: I think these two packets may be unnecessary. */
        cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
        cl_u8(&vc4->rcl, 0);
        cl_u8(&vc4->rcl, 0);

        cl_u8(&vc4->rcl, VC4_PACKET_STORE_TILE_BUFFER_GENERAL);
        cl_u16(&vc4->rcl, 0); // Store nothing (just clear)
        cl_u32(&vc4->rcl, 0); // no address is needed

        vc4_rcl_tile_calls(vc4, tilew, tileh, tile_alloc);

        vc4_flush(pctx);
}

static void
vc4_clear(struct pipe_context *pctx, unsigned buffers,
          const union pipe_color_union *color, double depth, unsigned stencil)
{
        struct vc4_context *vc4 = vc4_context(pctx);

        vc4->needs_flush = true;
}

static void
vc4_clear_render_target(struct pipe_context *pctx, struct pipe_surface *ps,
                        const union pipe_color_union *color,
                        unsigned x, unsigned y, unsigned w, unsigned h)
{
        fprintf(stderr, "unimpl: clear RT\n");
}

static void
vc4_clear_depth_stencil(struct pipe_context *pctx, struct pipe_surface *ps,
                        unsigned buffers, double depth, unsigned stencil,
                        unsigned x, unsigned y, unsigned w, unsigned h)
{
        fprintf(stderr, "unimpl: clear DS\n");
}

void
vc4_draw_init(struct pipe_context *pctx)
{
        pctx->draw_vbo = vc4_draw_vbo;
        pctx->clear = vc4_clear;
        pctx->clear_render_target = vc4_clear_render_target;
        pctx->clear_depth_stencil = vc4_clear_depth_stencil;
}
