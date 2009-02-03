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

#include "draw/draw_pipe.h"
#include "util/u_memory.h"

#include "r300_context.h"
#include "r300_reg.h"

/* r300_swtcl_emit: Primitive vertex emission using an immediate
 * vertex buffer and no HW TCL. */

struct swtcl_stage {
    /* Parent class */
    struct draw_stage draw;

    struct r300_context* r300;
};

static INLINE struct swtcl_stage* swtcl_stage(struct draw_stage* draw) {
    return (struct swtcl_stage*)draw;
}

static INLINE void r300_emit_prim(struct draw_stage* draw,
                                  struct prim_header* prim,
                                  unsigned hwprim,
                                  unsigned count)
{
    struct r300_context* r300 = swtcl_stage(draw)->r300;
}

/* Just as an aside...
 *
 * Radeons can do many more primitives:
 * - Line strip
 * - Triangle fan
 * - Triangle strip
 * - Line loop
 * - Quads
 * - Quad strip
 * - Polygons
 *
 * The following were just the only ones in Draw. */

static void r300_emit_point(struct draw_stage* draw, struct prim_header* prim)
{
    r300_emit_prim(draw, prim, R300_VAP_VF_CNTL__PRIM_POINTS, 1);
}

static void r300_emit_line(struct draw_stage* draw, struct prim_header* prim)
{
    r300_emit_prim(draw, prim, R300_VAP_VF_CNTL__PRIM_LINES, 2);
}

static void r300_emit_tri(struct draw_stage* draw, struct prim_header* prim)
{
    r300_emit_prim(draw, prim, R300_VAP_VF_CNTL__PRIM_TRIANGLES, 3);
}

static void r300_swtcl_flush(struct draw_stage* draw, unsigned flags)
{
}

static void r300_reset_stipple(struct draw_stage* draw)
{
    /* XXX */
}

static void r300_swtcl_destroy(struct draw_stage* draw)
{
    FREE(draw);
}

struct draw_stage* r300_draw_swtcl_stage(struct r300_context* r300)
{
    struct swtcl_stage* swtcl = CALLOC_STRUCT(swtcl_stage);

    swtcl->r300 = r300;
    swtcl->draw.point = r300_emit_point;
    swtcl->draw.line = r300_emit_line;
    swtcl->draw.tri = r300_emit_tri;
    swtcl->draw.flush = r300_swtcl_flush;
    swtcl->draw.reset_stipple_counter = r300_reset_stipple;
    swtcl->draw.destroy = r300_swtcl_destroy;

    return &swtcl->draw;
}
