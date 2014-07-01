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

#include "util/u_format.h"
#include "indices/u_primconvert.h"

#include "vc4_context.h"
#include "vc4_resource.h"

static void
vc4_rcl_tile_calls(struct vc4_context *vc4,
                   struct vc4_surface *csurf,
                   uint32_t xtiles, uint32_t ytiles)
{
        struct vc4_resource *ctex = vc4_resource(csurf->base.texture);

        for (int x = 0; x < xtiles; x++) {
                for (int y = 0; y < ytiles; y++) {
                        cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
                        cl_u8(&vc4->rcl, x);
                        cl_u8(&vc4->rcl, y);

                        cl_start_reloc(&vc4->rcl, 1);
                        cl_u8(&vc4->rcl, VC4_PACKET_LOAD_TILE_BUFFER_GENERAL);
                        cl_u8(&vc4->rcl,
                              VC4_LOADSTORE_TILE_BUFFER_COLOR |
                              VC4_LOADSTORE_TILE_BUFFER_FORMAT_RASTER);
                        cl_u8(&vc4->rcl,
                              VC4_LOADSTORE_TILE_BUFFER_RGBA8888);
                        cl_reloc(vc4, &vc4->rcl, ctex->bo, csurf->offset);

                        cl_start_reloc(&vc4->rcl, 1);
                        cl_u8(&vc4->rcl, VC4_PACKET_BRANCH_TO_SUB_LIST);
                        cl_reloc(vc4, &vc4->rcl, vc4->tile_alloc,
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

        if (info->mode >= PIPE_PRIM_QUADS) {
                util_primconvert_save_index_buffer(vc4->primconvert, &vc4->indexbuf);
                util_primconvert_save_rasterizer_state(vc4->primconvert, &vc4->rasterizer->base);
                util_primconvert_draw_vbo(vc4->primconvert, info);
                return;
        }

        uint32_t width = vc4->framebuffer.width;
        uint32_t height = vc4->framebuffer.height;
        uint32_t tilew = align(width, 64) / 64;
        uint32_t tileh = align(height, 64) / 64;

        uint32_t tile_alloc_size = 32 * tilew * tileh;
        uint32_t tile_state_size = 48 * tilew * tileh;
        if (!vc4->tile_alloc || vc4->tile_alloc->size < tile_alloc_size) {
                vc4_bo_unreference(&vc4->tile_alloc);
                vc4->tile_alloc = vc4_bo_alloc(vc4->screen, tile_alloc_size,
                                               "tile_alloc");
        }
        if (!vc4->tile_state || vc4->tile_state->size < tile_state_size) {
                vc4_bo_unreference(&vc4->tile_state);
                vc4->tile_state = vc4_bo_alloc(vc4->screen, tile_state_size,
                                               "tile_state");
        }

        vc4_update_compiled_shaders(vc4);

        vc4->needs_flush = true;

        //   Tile state data is 48 bytes per tile, I think it can be thrown away
        //   as soon as binning is finished.
        cl_start_reloc(&vc4->bcl, 2);
        cl_u8(&vc4->bcl, VC4_PACKET_TILE_BINNING_MODE_CONFIG);
        cl_reloc(vc4, &vc4->bcl, vc4->tile_alloc, 0);
        cl_u32(&vc4->bcl, 0x8000); /* tile allocation memory size */
        cl_reloc(vc4, &vc4->bcl, vc4->tile_state, 0);
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

        /* Note that the primitive type fields match with OpenGL/gallium
         * definitions, up to but not including QUADS.
         */
        if (info->indexed) {
                struct vc4_resource *rsc = vc4_resource(vc4->indexbuf.buffer);

                assert(vc4->indexbuf.index_size == 1 ||
                       vc4->indexbuf.index_size == 2);

                cl_start_reloc(&vc4->bcl, 1);
                cl_u8(&vc4->bcl, VC4_PACKET_GL_INDEXED_PRIMITIVE);
                cl_u8(&vc4->bcl,
                      info->mode |
                      (vc4->indexbuf.index_size == 2 ?
                       VC4_INDEX_BUFFER_U16:
                       VC4_INDEX_BUFFER_U8));
                cl_u32(&vc4->bcl, info->count);
                cl_reloc(vc4, &vc4->bcl, rsc->bo, vc4->indexbuf.offset);
                cl_u32(&vc4->bcl, info->max_index);
        } else {
                cl_u8(&vc4->bcl, VC4_PACKET_GL_ARRAY_PRIMITIVE);
                cl_u8(&vc4->bcl, info->mode);
                cl_u32(&vc4->bcl, info->count);
                cl_u32(&vc4->bcl, info->start);
        }

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

        struct vc4_vertex_stateobj *vtx = vc4->vtx;
        struct vc4_vertexbuf_stateobj *vertexbuf = &vc4->vertexbuf;
        for (int i = 0; i < vtx->num_elements; i++) {
                struct pipe_vertex_element *elem = &vtx->pipe[i];
                struct pipe_vertex_buffer *vb =
                        &vertexbuf->vb[elem->vertex_buffer_index];
                struct vc4_resource *rsc = vc4_resource(vb->buffer);

                cl_reloc(vc4, &vc4->shader_rec, rsc->bo,
                         vb->buffer_offset + elem->src_offset);
                cl_u8(&vc4->shader_rec,
                      util_format_get_blocksize(elem->src_format) - 1);
                cl_u8(&vc4->shader_rec, vb->stride);
                cl_u8(&vc4->shader_rec, 0); /* VS VPM offset */
                cl_u8(&vc4->shader_rec, 0); /* CS VPM offset */

                break; /* XXX: just the 1 for now. */
        }


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
        if (0) {
                cl_u8(&vc4->rcl, VC4_PACKET_TILE_COORDINATES);
                cl_u8(&vc4->rcl, 0);
                cl_u8(&vc4->rcl, 0);

                cl_u8(&vc4->rcl, VC4_PACKET_STORE_TILE_BUFFER_GENERAL);
                cl_u16(&vc4->rcl, 0); // Store nothing (just clear)
                cl_u32(&vc4->rcl, 0); // no address is needed
        }

        vc4_rcl_tile_calls(vc4, csurf, tilew, tileh);

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
