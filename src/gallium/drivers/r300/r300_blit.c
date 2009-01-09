/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r300_blit.h"

/* Does a "paint" into the specified rectangle.
 * Returns 1 on success, 0 on error. */
int r300_fill_blit(struct r300_context* r300,
                   unsigned cpp,
                   short dst_pitch,
                   struct pipe_buffer* dst_buffer,
                   unsigned dst_offset,
                   short x, short y,
                   short w, short h,
                   unsigned color)
{
    uint32_t dest_type;

    /* Check for fallbacks. */
    /* XXX we can do YUV surfaces, too, but only in 3D mode. Hmm... */
    switch(cpp) {
        case 2:
        case 6:
            dest_type = ATI_DATATYPE_CI8;
            break;
        case 4:
            dest_type = ATI_DATATYPE_RGB565;
            break;
        case 8:
            dest_type = ATI_DATATYPE_ARGB8888;
            break;
        default:
            /* Whatever this is, we can't fill it. (Yet.) */
            return 0;
    }

    /* XXX odds are *incredibly* good that we were in 3D just a bit ago,
     * so flush here first. */

    /* Set up the 2D engine. */
    OUT_CS_REG(RADEON_DEFAULT_SC_BOTTOM_RIGHT,
                  RADEON_DEFAULT_SC_RIGHT_MAX | RADEON_DEFAULT_SC_BOTTOM_MAX);
    /* XXX I have no idea what these flags mean, is this awesome? (y/n) */
    OUT_CS_REG(RADEON_DP_GUI_MASTER_CNTL,
                  RADEON_GMC_DST_PITCH_OFFSET_CNTL |
                  RADEON_GMC_BRUSH_SOLID_COLOR |
                  (dest_type << 8) |
                  RADEON_GMC_SRC_DATATYPE_COLOR |
                  /* XXX is this the right rop? */
                  RADEON_ROP3_ONE |
                  RADEON_GMC_CLR_CMP_CNTL_DIS);
    /* XXX pack this? */
    OUT_CS_REG(RADEON_DP_BRUSH_FRGD_CLR, color);
    OUT_CS_REG(RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);
    OUT_CS_REG(RADEON_DP_SRC_FRGD_CLR, 0xffffffff);
    OUT_CS_REG(RADEON_DP_SRC_BKGD_CLR, 0x00000000);
    /* XXX what should this be? */
    OUT_CS_REG(RADEON_DP_WRITE_MASK, 0x00000000);
    OUT_CS_REG(RADEON_DP_CNTL,
                  RADEON_DST_X_LEFT_TO_RIGHT | RADEON_DST_Y_TOP_TO_BOTTOM);
    OUT_CS_REG(RADEON_DST_PITCH_OFFSET, 0x0);
    /* XXX fix this shit -> OUT_RELOC(dst, 0, RADEON_GEM_DOMAIN_VRAM) */

    /* Do the actual paint. */
    OUT_CS_REG(RADEON_DST_Y_X, (y << 16) | x);
    OUT_CS_REG(RADEON_DST_HEIGHT_WIDTH, (h << 16) | w);

    /* Let the 2D engine settle. */
    OUT_CS_REG(RADEON_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL);
    OUT_CS_REG(RADEON_WAIT_UNTIL,
               RADEON_WAIT_2D_IDLECLEAN | RADEON_WAIT_DMA_GUI_IDLE);
    return 1;
}
