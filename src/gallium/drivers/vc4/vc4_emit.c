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

#include "vc4_context.h"

void
vc4_emit_state(struct pipe_context *pctx)
{
        struct vc4_context *vc4 = vc4_context(pctx);

        if (vc4->dirty & VC4_DIRTY_SCISSOR) {
                cl_u8(&vc4->bcl, VC4_PACKET_CLIP_WINDOW);
                cl_u16(&vc4->bcl, vc4->scissor.minx);
                cl_u16(&vc4->bcl, vc4->scissor.miny);
                cl_u16(&vc4->bcl, vc4->scissor.maxx - vc4->scissor.minx);
                cl_u16(&vc4->bcl, vc4->scissor.maxy - vc4->scissor.miny);
        }

        if (vc4->dirty & (VC4_DIRTY_RASTERIZER | VC4_DIRTY_ZSA)) {
                cl_u8(&vc4->bcl, VC4_PACKET_CONFIGURATION_BITS);
                cl_u8(&vc4->bcl,
                      vc4->rasterizer->config_bits[0] |
                      vc4->zsa->config_bits[0]);
                cl_u8(&vc4->bcl,
                      vc4->rasterizer->config_bits[1] |
                      vc4->zsa->config_bits[1]);
                cl_u8(&vc4->bcl,
                      vc4->rasterizer->config_bits[2] |
                      vc4->zsa->config_bits[2]);
        }

        if (vc4->dirty & VC4_DIRTY_RASTERIZER) {
                cl_u8(&vc4->bcl, VC4_PACKET_DEPTH_OFFSET);
                cl_u16(&vc4->bcl, vc4->rasterizer->offset_factor);
                cl_u16(&vc4->bcl, vc4->rasterizer->offset_units);

                cl_u8(&vc4->bcl, VC4_PACKET_POINT_SIZE);
                cl_f(&vc4->bcl, vc4->rasterizer->point_size);

                cl_u8(&vc4->bcl, VC4_PACKET_LINE_WIDTH);
                cl_f(&vc4->bcl, vc4->rasterizer->base.line_width);
        }

        if (vc4->dirty & VC4_DIRTY_VIEWPORT) {
                cl_u8(&vc4->bcl, VC4_PACKET_CLIPPER_XY_SCALING);
                cl_f(&vc4->bcl, vc4->viewport.scale[0] * 16.0f);
                cl_f(&vc4->bcl, vc4->viewport.scale[1] * 16.0f);

                cl_u8(&vc4->bcl, VC4_PACKET_CLIPPER_Z_SCALING);
                cl_f(&vc4->bcl, vc4->viewport.translate[2]);
                cl_f(&vc4->bcl, vc4->viewport.scale[2]);

                cl_u8(&vc4->bcl, VC4_PACKET_VIEWPORT_OFFSET);
                cl_u16(&vc4->bcl, 16 * vc4->viewport.translate[0]);
                cl_u16(&vc4->bcl, 16 * vc4->viewport.translate[1]);
        }

        if (vc4->dirty & VC4_DIRTY_FLAT_SHADE_FLAGS) {
                cl_u8(&vc4->bcl, VC4_PACKET_FLAT_SHADE_FLAGS);
                cl_u32(&vc4->bcl, vc4->rasterizer->base.flatshade ?
                       vc4->prog.fs->color_inputs : 0);
        }
}
