/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *                Joakim Sindholt <opensource@zhasha.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r300_surface.h"

static void r300_surface_setup(struct pipe_context* pipe,
                               struct pipe_surface* dest,
                               unsigned x, unsigned y,
                               unsigned w, unsigned h)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_capabilities* caps = r300_screen(pipe->screen)->caps;
    struct r300_texture* tex = (struct r300_texture*)dest->texture;
    unsigned pixpitch = tex->stride / tex->tex.block.size;
    CS_LOCALS(r300);

    /* Make sure our target BO is okay. */
    r300->winsys->add_buffer(r300->winsys, tex->buffer,
            0, RADEON_GEM_DOMAIN_VRAM);
    if (r300->winsys->validate(r300->winsys)) {
        r300->context.flush(&r300->context, 0, NULL);
    }

    r300_emit_blend_state(r300, &blend_clear_state);
    r300_emit_blend_color_state(r300, &blend_color_clear_state);
    r300_emit_dsa_state(r300, &dsa_clear_state);
    r300_emit_rs_state(r300, &rs_clear_state);

    BEGIN_CS(15);

    /* Pixel scissors. */
    OUT_CS_REG_SEQ(R300_SC_SCISSORS_TL, 2);
    if (caps->is_r500) {
        OUT_CS((x << R300_SCISSORS_X_SHIFT) | (y << R300_SCISSORS_Y_SHIFT));
        OUT_CS((w << R300_SCISSORS_X_SHIFT) | (h << R300_SCISSORS_Y_SHIFT));
    } else {
        /* Non-R500 chipsets have an offset of 1440 in their scissors. */
        OUT_CS(((x + 1440) << R300_SCISSORS_X_SHIFT) |
                ((y + 1440) << R300_SCISSORS_Y_SHIFT));
        OUT_CS(((w + 1440) << R300_SCISSORS_X_SHIFT) |
                ((h + 1440) << R300_SCISSORS_Y_SHIFT));
    }

    /* Flush colorbuffer and blend caches. */
    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT,
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D |
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FINISH_SIGNAL);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);

    /* Setup colorbuffer. */
    OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0, 1);
    OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
    OUT_CS_REG(R300_RB3D_COLORPITCH0, pixpitch |
        r300_translate_colorformat(tex->tex.format));
    OUT_CS_REG(RB3D_COLOR_CHANNEL_MASK, 0xf);

    END_CS;
}

/* Provides pipe_context's "surface_fill". Commonly used for clearing
 * buffers. */
static void r300_surface_fill(struct pipe_context* pipe,
                              struct pipe_surface* dest,
                              unsigned x, unsigned y,
                              unsigned w, unsigned h,
                              unsigned color)
{
    int i;
    float r, g, b, a, depth;
    struct r300_context* r300 = r300_context(pipe);
    struct r300_capabilities* caps = r300_screen(pipe->screen)->caps;
    struct r300_texture* tex = (struct r300_texture*)dest->texture;
    unsigned pixpitch = tex->stride / tex->tex.block.size;
    CS_LOCALS(r300);

    a = (float)((color >> 24) & 0xff) / 255.0f;
    r = (float)((color >> 16) & 0xff) / 255.0f;
    g = (float)((color >>  8) & 0xff) / 255.0f;
    b = (float)((color >>  0) & 0xff) / 255.0f;
    debug_printf("r300: Filling surface %p at (%d,%d),"
        " dimensions %dx%d (pixel pitch %d), color 0x%x\n",
        dest, x, y, w, h, pixpitch, color);

    /* Fallback? */
    if (FALSE) {
        debug_printf("r300: Falling back on surface clear...");
        util_surface_fill(pipe, dest, x, y, w, h, color);
        return;
    }

    r300_surface_setup(r300, dest, x, y, w, h);

    /* Vertex shader setup */
    if (caps->has_tcl) {
        r300_emit_vertex_shader(r300, &r300_passthrough_vertex_shader);
    } else {
        BEGIN_CS(4);
        OUT_CS_REG(R300_VAP_CNTL_STATUS, R300_VAP_TCL_BYPASS);
        OUT_CS_REG(R300_VAP_CNTL, R300_PVS_NUM_SLOTS(5) |
                R300_PVS_NUM_CNTLRS(5) |
                R300_PVS_NUM_FPUS(caps->num_vert_fpus) |
                R300_PVS_VF_MAX_VTX_NUM(12));
        END_CS;
    }

    /* Fragment shader setup */
    if (caps->is_r500) {
        r500_emit_fragment_shader(r300, &r500_passthrough_fragment_shader);
        r300_emit_rs_block_state(r300, &r500_rs_block_clear_state);
    } else {
        r300_emit_fragment_shader(r300, &r300_passthrough_fragment_shader);
        r300_emit_rs_block_state(r300, &r300_rs_block_clear_state);
    }

    BEGIN_CS(31);

    /* VAP stream control, mapping from input memory to PVS/RS memory */
    if (caps->has_tcl) {
        OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0,
            (R300_DATA_TYPE_FLOAT_4 << R300_DATA_TYPE_0_SHIFT) |
            ((R300_LAST_VEC | (1 << R300_DST_VEC_LOC_SHIFT) |
                R300_DATA_TYPE_FLOAT_4) << R300_DATA_TYPE_1_SHIFT));
    } else {
        OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0,
            (R300_DATA_TYPE_FLOAT_4 << R300_DATA_TYPE_0_SHIFT) |
            ((R300_LAST_VEC | (2 << R300_DST_VEC_LOC_SHIFT) |
                R300_DATA_TYPE_FLOAT_4) << R300_DATA_TYPE_1_SHIFT));
    }
    OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_EXT_0,
            (R300_VAP_SWIZZLE_XYZW << R300_SWIZZLE0_SHIFT) |
            (R300_VAP_SWIZZLE_XYZW << R300_SWIZZLE1_SHIFT));

    /* VAP format controls */
    OUT_CS_REG(R300_VAP_OUTPUT_VTX_FMT_0,
            R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT |
            R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT);
    OUT_CS_REG(R300_VAP_OUTPUT_VTX_FMT_1, 0x0);

    /* Disable textures */
    OUT_CS_REG(R300_TX_ENABLE, 0x0);

    /* Viewport setup */
    OUT_CS_REG_SEQ(R300_SE_VPORT_XSCALE, 6);
    OUT_CS_32F(1.0);
    OUT_CS_32F((float)x);
    OUT_CS_32F(1.0);
    OUT_CS_32F((float)y);
    OUT_CS_32F(1.0);
    OUT_CS_32F(0.0);

    /* The size of the point we're about to draw, in sixths of pixels */
    OUT_CS_REG(R300_GA_POINT_SIZE,
        ((h * 6) & R300_POINTSIZE_Y_MASK) |
        ((w * 6) << R300_POINTSIZE_X_SHIFT));

    /* Packet3 with our point vertex */
    OUT_CS_PKT3(R200_3D_DRAW_IMMD_2, 8);
    OUT_CS(R300_PRIM_TYPE_POINT | R300_PRIM_WALK_RING |
            (1 << R300_PRIM_NUM_VERTICES_SHIFT));
    /* Position */
    OUT_CS_32F(w / 2.0);
    OUT_CS_32F(h / 2.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    /* Color */
    OUT_CS_32F(r);
    OUT_CS_32F(g);
    OUT_CS_32F(b);
    OUT_CS_32F(a);

    /* XXX figure out why this is 0xA and not 0x2 */
    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT, 0xA);
    /* XXX OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE); */

    END_CS;

    r300->dirty_hw++;
}

static void r300_surface_copy(struct pipe_context* pipe,
                              struct pipe_surface* dest,
                              unsigned destx, unsigned desty,
                              struct pipe_surface* src,
                              unsigned srcx, unsigned srcy,
                              unsigned w, unsigned h)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_capabilities* caps = r300_screen(pipe->screen)->caps;
    struct r300_texture* srctex = (struct r300_texture*)src->texture;
    struct r300_texture* desttex = (struct r300_texture*)dest->texture;
    unsigned pixpitch = srctex->stride / srctex->tex.block.size;
    CS_LOCALS(r300);

    debug_printf("r300: Copying surface %p at (%d,%d) to %p at (%d, %d),"
        " dimensions %dx%d (pixel pitch %d)\n",
        src, srcx, srcy, dest, destx, desty, w, h, pixpitch);

    if ((srctex == desttex) &&
            ((destx < srcx + w) || (srcx < destx + w)) &&
            ((desty < srcy + h) || (srcy < desty + h))) {
        debug_printf("r300: Falling back on surface_copy\n");
        util_surface_copy(pipe, FALSE, dest, destx, desty, src,
                srcx, srcy, w, h);
    }

    r300_emit_sampler(r300, &r300_sampler_copy_state, 0);
    r300_emit_texture(r300, srctex, 0);
    r300_flush_textures(r300);

    /* Vertex shader setup */
    if (caps->has_tcl) {
        r300_emit_vertex_shader(r300, &r300_texture_vertex_shader);
    } else {
        BEGIN_CS(4);
        OUT_CS_REG(R300_VAP_CNTL_STATUS, R300_VAP_TCL_BYPASS);
        OUT_CS_REG(R300_VAP_CNTL, R300_PVS_NUM_SLOTS(5) |
                R300_PVS_NUM_CNTLRS(5) |
                R300_PVS_NUM_FPUS(caps->num_vert_fpus) |
                R300_PVS_VF_MAX_VTX_NUM(12));
        END_CS;
    }

    /* Fragment shader setup */
    if (caps->is_r500) {
        r500_emit_fragment_shader(r300, &r500_texture_fragment_shader);
        r300_emit_rs_block_state(r300, &r500_rs_block_copy_state);
    } else {
        r300_emit_fragment_shader(r300, &r300_texture_fragment_shader);
        r300_emit_rs_block_state(r300, &r300_rs_block_copy_state);
    }

    /* VAP stream control, mapping from input memory to PVS/RS memory */
    if (caps->has_tcl) {
        OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0,
            (R300_DATA_TYPE_FLOAT_2 << R300_DATA_TYPE_0_SHIFT) |
            ((R300_LAST_VEC | (1 << R300_DST_VEC_LOC_SHIFT) |
                R300_DATA_TYPE_FLOAT_2) << R300_DATA_TYPE_1_SHIFT));
    } else {
        OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0,
            (R300_DATA_TYPE_FLOAT_2 << R300_DATA_TYPE_0_SHIFT) |
            ((R300_LAST_VEC | (6 << R300_DST_VEC_LOC_SHIFT) |
                R300_DATA_TYPE_FLOAT_2) << R300_DATA_TYPE_1_SHIFT));
    }
    OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_EXT_0,
            (R300_VAP_SWIZZLE_XYZW << R300_SWIZZLE0_SHIFT) |
            (R300_VAP_SWIZZLE_XYZW << R300_SWIZZLE1_SHIFT));

    /* VAP format controls */
    OUT_CS_REG(R300_VAP_OUTPUT_VTX_FMT_0,
            R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT);
    /* Two components of texture 0 */
    OUT_CS_REG(R300_VAP_OUTPUT_VTX_FMT_1, 0x2);

    /* Packet3 with our texcoords */
    OUT_CS_PKT3(R200_3D_DRAW_IMMD_2, 8);
    OUT_CS(R300_PRIM_TYPE_QUADS | R300_PRIM_WALK_RING |
            (4 << R300_PRIM_NUM_VERTICES_SHIFT));
    /* (x    , y    ) */
    OUT_CS_32F((float)destx);
    OUT_CS_32F((float)desty);
    OUT_CS_32F((float)srcx);
    OUT_CS_32F((float)srcy);
    /* (x    , y + h) */
    OUT_CS_32F((float)destx);
    OUT_CS_32F((float)(desty + h));
    OUT_CS_32F((float)srcx);
    OUT_CS_32F((float)(srcy + h));
    /* (x + w, y + h) */
    OUT_CS_32F((float)(destx + w));
    OUT_CS_32F((float)(desty + h));
    OUT_CS_32F((float)(srcx + w));
    OUT_CS_32F((float)(srcy + h));
    /* (x + w, y    ) */
    OUT_CS_32F((float)(destx + w));
    OUT_CS_32F((float)desty);
    OUT_CS_32F((float)(srcx + w));
    OUT_CS_32F((float)srcy);

    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT, 0xA);

    r300->dirty_hw++;
}

void r300_init_surface_functions(struct r300_context* r300)
{
    r300->context.surface_fill = r300_surface_fill;
    r300->context.surface_copy = r300_surface_copy;
}
