/*
 * Copyright 2009 Nicolai Haehnle <nhaehnle@gmail.com>
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

#include "r300_context.h"

#include "util/u_debug.h"

#include <stdio.h>

static const struct debug_named_value debug_options[] = {
    { "fp", DBG_FP, "Fragment program handling (for debugging)" },
    { "vp", DBG_VP, "Vertex program handling (for debugging)" },
    { "draw", DBG_DRAW, "Draw and emit (for debugging)" },
    { "tex", DBG_TEX, "Textures (for debugging)" },
    { "texalloc", DBG_TEXALLOC, "Texture allocation (for debugging)" },
    { "fall", DBG_FALL, "Fallbacks (for debugging)" },
    { "rs", DBG_RS, "Rasterizer (for debugging)" },
    { "fb", DBG_FB, "Framebuffer (for debugging)" },
    { "anisohq", DBG_ANISOHQ, "High quality anisotropic filtering (for benchmarking)" },
    { "notiling", DBG_NO_TILING, "Disable tiling (for benchmarking)" },
    { "noimmd", DBG_NO_IMMD, "Disable immediate mode (for benchmarking)" },
    { "stats", DBG_STATS, "Gather statistics (for lulz)" },

    /* must be last */
    DEBUG_NAMED_VALUE_END
};

void r300_init_debug(struct r300_screen * screen)
{
    screen->debug = debug_get_flags_option("RADEON_DEBUG", debug_options, 0);
}

void r500_dump_rs_block(struct r300_rs_block *rs)
{
    unsigned count, ip, it_count, ic_count, i, j;
    unsigned tex_ptr;
    unsigned col_ptr, col_fmt;

    count = rs->inst_count & 0xf;
    count++;

    it_count = rs->count & 0x7f;
    ic_count = (rs->count >> 7) & 0xf;

    fprintf(stderr, "RS Block: %d texcoords (linear), %d colors (perspective)\n",
        it_count, ic_count);
    fprintf(stderr, "%d instructions\n", count);

    for (i = 0; i < count; i++) {
        if (rs->inst[i] & 0x10) {
            ip = rs->inst[i] & 0xf;
            fprintf(stderr, "texture: ip %d to psf %d\n",
                ip, (rs->inst[i] >> 5) & 0x7f);

            tex_ptr = rs->ip[ip] & 0xffffff;
            fprintf(stderr, "       : ");

            j = 3;
            do {
                if ((tex_ptr & 0x3f) == 63) {
                    fprintf(stderr, "1.0");
                } else if ((tex_ptr & 0x3f) == 62) {
                    fprintf(stderr, "0.0");
                } else {
                    fprintf(stderr, "[%d]", tex_ptr & 0x3f);
                }
            } while (j-- && fprintf(stderr, "/"));
            fprintf(stderr, "\n");
        }

        if (rs->inst[i] & 0x10000) {
            ip = (rs->inst[i] >> 12) & 0xf;
            fprintf(stderr, "color: ip %d to psf %d\n",
                ip, (rs->inst[i] >> 18) & 0x7f);

            col_ptr = (rs->ip[ip] >> 24) & 0x7;
            col_fmt = (rs->ip[ip] >> 27) & 0xf;
            fprintf(stderr, "     : offset %d ", col_ptr);

            switch (col_fmt) {
                case 0:
                    fprintf(stderr, "(R/G/B/A)");
                    break;
                case 1:
                    fprintf(stderr, "(R/G/B/0)");
                    break;
                case 2:
                    fprintf(stderr, "(R/G/B/1)");
                    break;
                case 4:
                    fprintf(stderr, "(0/0/0/A)");
                    break;
                case 5:
                    fprintf(stderr, "(0/0/0/0)");
                    break;
                case 6:
                    fprintf(stderr, "(0/0/0/1)");
                    break;
                case 8:
                    fprintf(stderr, "(1/1/1/A)");
                    break;
                case 9:
                    fprintf(stderr, "(1/1/1/0)");
                    break;
                case 10:
                    fprintf(stderr, "(1/1/1/1)");
                    break;
            }
            fprintf(stderr, "\n");
        }
    }
}
