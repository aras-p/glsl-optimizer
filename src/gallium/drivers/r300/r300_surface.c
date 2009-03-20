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

/* Provides pipe_context's "surface_fill". Commonly used for clearing
 * buffers. */
static void r300_surface_fill(struct pipe_context* pipe,
                              struct pipe_surface* dest,
                              unsigned x, unsigned y,
                              unsigned w, unsigned h,
                              unsigned color)
{
    struct r300_context* r300 = r300_context(pipe);
    CS_LOCALS(r300);
    struct r300_capabilities* caps = r300_screen(pipe->screen)->caps;
    struct r300_texture* tex = (struct r300_texture*)dest->texture;
    int i;
    float r, g, b, a, depth;
    unsigned pixpitch = tex->stride / tex->tex.block.size;

    r = (float)((color >> 16) & 0xff) / 255.0f;
    g = (float)((color >>  8) & 0xff) / 255.0f;
    b = (float)((color >>  0) & 0xff) / 255.0f;
    debug_printf("r300: Filling surface %p at (%d,%d),"
        " dimensions %dx%d (pixel pitch %d), color 0x%x\n",
        dest, x, y, w, h, pixpitch, color);

    /* Fallback? */
    if (tex->tex.format != PIPE_FORMAT_A8R8G8B8_UNORM) {
        debug_printf("r300: Falling back on surface clear...");
        util_surface_fill(pipe, dest, x, y, w, h, color);
        return;
    }

    r300_emit_blend_state(r300, &blend_clear_state);
    r300_emit_blend_color_state(r300, &blend_color_clear_state);
    r300_emit_dsa_state(r300, &dsa_clear_state);
    r300_emit_rs_state(r300, &rs_clear_state);

    /* Fragment shader setup */
    if (caps->is_r500) {
        r500_emit_fragment_shader(r300, &r500_passthrough_fragment_shader);
        r300_emit_rs_block_state(r300, &r500_rs_block_clear_state);
    } else {
        r300_emit_fragment_shader(r300, &r300_passthrough_fragment_shader);
        r300_emit_rs_block_state(r300, &r300_rs_block_clear_state);
    }

    BEGIN_CS(36);

    /* Viewport setup */
    OUT_CS_REG_SEQ(R300_SE_VPORT_XSCALE, 6);
    OUT_CS_32F(1.0);
    OUT_CS_32F((float)x);
    OUT_CS_32F(1.0);
    OUT_CS_32F((float)y);
    OUT_CS_32F(1.0);
    OUT_CS_32F(0.0);

    /* Pixel scissors */
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

    /* The size of the point we're about to draw, in sixths of pixels */
    OUT_CS_REG(R300_GA_POINT_SIZE,
        ((h * 6) & R300_POINTSIZE_Y_MASK) |
        ((w * 6) << R300_POINTSIZE_X_SHIFT));

    /* Flush colorbuffer and blend caches. */
    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT,
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D |
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FINISH_SIGNAL);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);

    OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0, 1);
    OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
    OUT_CS_REG(R300_RB3D_COLORPITCH0, pixpitch |
        r300_translate_colorformat(tex->tex.format));
    OUT_CS_REG(RB3D_COLOR_CHANNEL_MASK, 0x0000000F);
    /* XXX Packet3 */
    OUT_CS(CP_PACKET3(R200_3D_DRAW_IMMD_2, 8));
    OUT_CS(R300_PRIM_TYPE_POINT | R300_PRIM_WALK_RING |
    (1 << R300_PRIM_NUM_VERTICES_SHIFT));
    OUT_CS_32F(w / 2.0);
    OUT_CS_32F(h / 2.0);
    /* XXX this should be the depth value to clear to */
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(r);
    OUT_CS_32F(g);
    OUT_CS_32F(b);
    OUT_CS_32F(1.0);

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
    CS_LOCALS(r300);
    struct r300_texture* srctex = (struct r300_texture*)src->texture;
    struct r300_texture* desttex = (struct r300_texture*)dest->texture;

    unsigned pixpitch = srctex->stride / srctex->tex.block.size;
    debug_printf("r300: Copying surface %p at (%d,%d) to %p at (%d, %d),"
        " dimensions %dx%d (pixel pitch %d)\n",
        src, srcx, srcy, dest, destx, desty, w, h, pixpitch);

    if (TRUE) {
        debug_printf("r300: Falling back on surface_copy\n");
        return util_surface_copy(pipe, FALSE, dest, destx, desty, src,
                srcx, srcy, w, h);
    }
#if 0
    BEGIN_CS();
    OUT_CS_REG(RADEON_DEFAULT_SC_BOTTOM_RIGHT,(RADEON_DEFAULT_SC_RIGHT_MAX |
                RADEON_DEFAULT_SC_BOTTOM_MAX));
    OUT_ACCEL_REG(RADEON_DP_GUI_MASTER_CNTL, (RADEON_GMC_DST_PITCH_OFFSET_CNTL |
					  RADEON_GMC_SRC_PITCH_OFFSET_CNTL |
					  RADEON_GMC_BRUSH_NONE |
					  (datatype << 8) |
					  RADEON_GMC_SRC_DATATYPE_COLOR |
					  RADEON_ROP[rop].rop |
					  RADEON_DP_SRC_SOURCE_MEMORY |
					  RADEON_GMC_CLR_CMP_CNTL_DIS));
    OUT_CS_REG(RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
    OUT_CS_REG(RADEON_DP_BRUSH_BKGD_CLR, 0x0);
    OUT_CS_REG(RADEON_DP_SRC_FRGD_CLR, 0xffffffff);
    OUT_CS_REG(RADEON_DP_SRC_BKGD_CLR, 0x0);
    OUT_ACCEL_REG(RADEON_DP_WRITE_MASK, planemask);
    OUT_ACCEL_REG(RADEON_DP_CNTL, ((info->accel_state->xdir >= 0 ? RADEON_DST_X_LEFT_TO_RIGHT : 0) |
			       (info->accel_state->ydir >= 0 ? RADEON_DST_Y_TOP_TO_BOTTOM : 0));
);

    OUT_CS_REG_SEQ(RADEON_DST_PITCH_OFFSET, 1);
    OUT_CS_RELOC(desttex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);

    OUT_CS_REG_SEQ(RADEON_SRC_PITCH_OFFSET, 1);
    OUT_CS_RELOC(srctex->buffer, 0,
            RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0, 0);

    OUT_CS_REG(RADEON_SRC_Y_X, (srcy << 16) | srcx);
    OUT_CS_REG(RADEON_DST_Y_X, (desty << 16) | destx);
    OUT_CS_REG(RADEON_DST_HEIGHT_WIDTH, (h << 16) | w);
    OUT_CS_REG(RADEON_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL);
    OUT_CS_REG(RADEON_WAIT_UNTIL,
                  RADEON_WAIT_2D_IDLECLEAN | RADEON_WAIT_DMA_GUI_IDLE);
    END_CS;
#endif
}

void r300_init_surface_functions(struct r300_context* r300)
{
    r300->context.surface_fill = r300_surface_fill;
    r300->context.surface_copy = r300_surface_copy;
}
