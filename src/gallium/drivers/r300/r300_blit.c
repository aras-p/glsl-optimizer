/*
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
#include "r300_context.h"

static void r300_blitter_save_states(struct r300_context* r300)
{
    util_blitter_save_blend(r300->blitter, r300->blend_state.state);
    util_blitter_save_depth_stencil_alpha(r300->blitter, r300->dsa_state.state);
    util_blitter_save_rasterizer(r300->blitter, r300->rs_state.state);
    util_blitter_save_fragment_shader(r300->blitter, r300->fs);
    util_blitter_save_vertex_shader(r300->blitter, r300->vs);
}

/* Clear currently bound buffers. */
void r300_clear(struct pipe_context* pipe,
                unsigned buffers,
                const float* rgba,
                double depth,
                unsigned stencil)
{
    /* XXX Implement fastfill.
     *
     * If fastfill is enabled, a few facts should be considered:
     *
     * 1) Zbuffer must be micro-tiled and whole microtiles must be
     *    written.
     *
     * 2) ZB_DEPTHCLEARVALUE is used to clear a zbuffer and Z Mask must be
     *    equal to 0.
     *
     * 3) RB3D_COLOR_CLEAR_VALUE is used to clear a colorbuffer and
     *    RB3D_COLOR_CHANNEL_MASK must be equal to 0.
     *
     * 4) ZB_CB_CLEAR can be used to make the ZB units help in clearing
     *    the colorbuffer. The color clear value is supplied through both
     *    RB3D_COLOR_CLEAR_VALUE and ZB_DEPTHCLEARVALUE, and the colorbuffer
     *    must be set in ZB_DEPTHOFFSET and ZB_DEPTHPITCH in addition to
     *    RB3D_COLOROFFSET and RB3D_COLORPITCH. It's obvious that the zbuffer
     *    will not be cleared and multiple render targets cannot be cleared
     *    this way either.
     *
     * 5) For 16-bit integer buffering, compression causes a hung with one or
     *    two samples and should not be used.
     *
     * 6) Fastfill must not be used if reading of compressed Z data is disabled
     *    and writing of compressed Z data is enabled (RD/WR_COMP_ENABLE),
     *    i.e. it cannot be used to compress the zbuffer.
     *    (what the hell does that mean and how does it fit in clearing
     *    the buffers?)
     *
     * - Marek
     */

    struct r300_context* r300 = r300_context(pipe);

    r300_blitter_save_states(r300);

    util_blitter_clear(r300->blitter,
                       r300->framebuffer_state.width,
                       r300->framebuffer_state.height,
                       r300->framebuffer_state.nr_cbufs,
                       buffers, rgba, depth, stencil);
}

/* Copy a block of pixels from one surface to another. */
void r300_surface_copy(struct pipe_context* pipe,
                       struct pipe_surface* dst,
                       unsigned dstx, unsigned dsty,
                       struct pipe_surface* src,
                       unsigned srcx, unsigned srcy,
                       unsigned width, unsigned height)
{
    struct r300_context* r300 = r300_context(pipe);

    /* Yeah we have to save all those states to ensure this blitter operation
     * is really transparent. The states will be restored by the blitter once
     * copying is done. */
    r300_blitter_save_states(r300);
    util_blitter_save_framebuffer(r300->blitter, &r300->framebuffer_state);

    util_blitter_save_fragment_sampler_states(
        r300->blitter, r300->sampler_count, (void**)r300->sampler_states);

    util_blitter_save_fragment_sampler_textures(
        r300->blitter, r300->texture_count,
        (struct pipe_texture**)r300->textures);

    /* Do a copy */
    util_blitter_copy(r300->blitter,
                      dst, dstx, dsty, src, srcx, srcy, width, height, TRUE);
}

/* Fill a region of a surface with a constant value. */
void r300_surface_fill(struct pipe_context* pipe,
                       struct pipe_surface* dst,
                       unsigned dstx, unsigned dsty,
                       unsigned width, unsigned height,
                       unsigned value)
{
    struct r300_context* r300 = r300_context(pipe);

    r300_blitter_save_states(r300);
    util_blitter_save_framebuffer(r300->blitter, &r300->framebuffer_state);

    util_blitter_fill(r300->blitter,
                      dst, dstx, dsty, width, height, value);
}
