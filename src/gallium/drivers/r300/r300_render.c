/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

/* r300_render: Vertex and index buffer primitive emission. Contains both
 * HW TCL fastpath rendering, and SW TCL Draw-assisted rendering. */

#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"

#include "pipe/p_inlines.h"

#include "util/u_memory.h"
#include "util/u_prim.h"

#include "r300_cs.h"
#include "r300_context.h"
#include "r300_emit.h"
#include "r300_reg.h"
#include "r300_render.h"
#include "r300_state_derived.h"

/* r300_render: Vertex and index buffer primitive emission. */
#define R300_MAX_VBO_SIZE  (1024 * 1024)

uint32_t r300_translate_primitive(unsigned prim)
{
    switch (prim) {
        case PIPE_PRIM_POINTS:
            return R300_VAP_VF_CNTL__PRIM_POINTS;
        case PIPE_PRIM_LINES:
            return R300_VAP_VF_CNTL__PRIM_LINES;
        case PIPE_PRIM_LINE_LOOP:
            return R300_VAP_VF_CNTL__PRIM_LINE_LOOP;
        case PIPE_PRIM_LINE_STRIP:
            return R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
        case PIPE_PRIM_TRIANGLES:
            return R300_VAP_VF_CNTL__PRIM_TRIANGLES;
        case PIPE_PRIM_TRIANGLE_STRIP:
            return R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
        case PIPE_PRIM_TRIANGLE_FAN:
            return R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
        case PIPE_PRIM_QUADS:
            return R300_VAP_VF_CNTL__PRIM_QUADS;
        case PIPE_PRIM_QUAD_STRIP:
            return R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
        case PIPE_PRIM_POLYGON:
            return R300_VAP_VF_CNTL__PRIM_POLYGON;
        default:
            return 0;
    }
}

static uint32_t r300_provoking_vertex_fixes(struct r300_context *r300,
                                            unsigned mode)
{
    struct r300_rs_state* rs = (struct r300_rs_state*)r300->rs_state.state;
    uint32_t color_control = rs->color_control;

    /* By default (see r300_state.c:r300_create_rs_state) color_control is
     * initialized to provoking the first vertex.
     *
     * Triangle fans must be reduced to the second vertex, not the first, in
     * Gallium flatshade-first mode, as per the GL spec.
     * (http://www.opengl.org/registry/specs/ARB/provoking_vertex.txt)
     *
     * Quads never provoke correctly in flatshade-first mode. The first
     * vertex is never considered as provoking, so only the second, third,
     * and fourth vertices can be selected, and both "third" and "last" modes
     * select the fourth vertex. This is probably due to D3D lacking quads.
     *
     * Similarly, polygons reduce to the first, not the last, vertex, when in
     * "last" mode, and all other modes start from the second vertex.
     *
     * ~ C.
     */

    if (rs->rs.flatshade_first) {
        switch (mode) {
            case PIPE_PRIM_TRIANGLE_FAN:
                color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_SECOND;
                break;
            case PIPE_PRIM_QUADS:
            case PIPE_PRIM_QUAD_STRIP:
            case PIPE_PRIM_POLYGON:
                color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
                break;
            default:
                color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_FIRST;
                break;
        }
    } else {
        color_control |= R300_GA_COLOR_CONTROL_PROVOKING_VERTEX_LAST;
    }

    return color_control;
}

static void r300_emit_draw_immediate(struct r300_context *r300,
                                     unsigned mode,
                                     unsigned start,
                                     unsigned count)
{
    struct pipe_buffer* vbo = r300->vertex_buffer[0].buffer;
    unsigned vertex_size = r300->vertex_buffer[0].stride / sizeof(float);
    unsigned i;
    uint32_t* map;
    CS_LOCALS(r300);

    map = (uint32_t*)pipe_buffer_map_range(r300->context.screen, vbo,
            start * vertex_size, count * vertex_size,
            PIPE_BUFFER_USAGE_CPU_READ);

    BEGIN_CS(10 + count * vertex_size);
    OUT_CS_REG(R300_GA_COLOR_CONTROL,
            r300_provoking_vertex_fixes(r300, mode));
    OUT_CS_REG(R300_VAP_VTX_SIZE, vertex_size);
    OUT_CS_REG(R300_VAP_VF_MIN_VTX_INDX, 0);
    OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, count - 1);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_IMMD_2, count * vertex_size);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_EMBEDDED | (count << 16) |
            r300_translate_primitive(mode));
    //debug_printf("r300: Immd %d verts, %d attrs\n", count, vertex_size);
    for (i = 0; i < count * vertex_size; i++) {
        if (i % vertex_size == 0) {
            //debug_printf("r300: -- vert --\n");
        }
        //debug_printf("r300: 0x%08x\n", *map);
        OUT_CS(*map);
        map++;
    }
    END_CS;

    pipe_buffer_unmap(r300->context.screen, vbo);
}

static void r300_emit_draw_arrays(struct r300_context *r300,
                                  unsigned mode,
                                  unsigned count)
{
    CS_LOCALS(r300);

    BEGIN_CS(8);
    OUT_CS_REG(R300_GA_COLOR_CONTROL,
            r300_provoking_vertex_fixes(r300, mode));
    OUT_CS_REG(R300_VAP_VF_MIN_VTX_INDX, 0);
    OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, count - 1);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (count << 16) |
           r300_translate_primitive(mode));
    END_CS;
}

static void r300_emit_draw_elements(struct r300_context *r300,
                                    struct pipe_buffer* indexBuffer,
                                    unsigned indexSize,
                                    unsigned minIndex,
                                    unsigned maxIndex,
                                    unsigned mode,
                                    unsigned start,
                                    unsigned count)
{
    uint32_t count_dwords;
    uint32_t offset_dwords = indexSize * start / sizeof(uint32_t);
    CS_LOCALS(r300);

    /* XXX most of these are stupid */
    assert(indexSize == 4 || indexSize == 2);
    assert((start * indexSize)  % 4 == 0);
    assert(offset_dwords == 0);

    BEGIN_CS(14);
    OUT_CS_REG(R300_GA_COLOR_CONTROL,
            r300_provoking_vertex_fixes(r300, mode));
    OUT_CS_REG(R300_VAP_VF_MIN_VTX_INDX, minIndex);
    OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, maxIndex);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, 0);
    if (indexSize == 4) {
        count_dwords = count + start;
        OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
               R300_VAP_VF_CNTL__INDEX_SIZE_32bit |
               r300_translate_primitive(mode));
    } else {
        count_dwords = (count + start + 1) / 2;
        OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
               r300_translate_primitive(mode));
    }

    /* INDX_BUFFER is a truly special packet3.
     * Unlike most other packet3, where the offset is after the count,
     * the order is reversed, so the relocation ends up carrying the
     * size of the indexbuf instead of the offset.
     *
     * XXX Fix offset
     */
    OUT_CS_PKT3(R300_PACKET3_INDX_BUFFER, 2);
    OUT_CS(R300_INDX_BUFFER_ONE_REG_WR | (R300_VAP_PORT_IDX0 >> 2) |
           (0 << R300_INDX_BUFFER_SKIP_SHIFT));
    OUT_CS(offset_dwords);
    OUT_CS_RELOC(indexBuffer, count_dwords,
        RADEON_GEM_DOMAIN_GTT, 0, 0);

    END_CS;
}


static boolean r300_setup_vertex_buffers(struct r300_context *r300)
{
    struct pipe_vertex_buffer *vbuf = r300->vertex_buffer;
    struct pipe_vertex_element *velem = r300->vertex_element;

validate:
    for (int i = 0; i < r300->vertex_element_count; i++) {
        if (!r300->winsys->add_buffer(r300->winsys,
                vbuf[velem[i].vertex_buffer_index].buffer,
            RADEON_GEM_DOMAIN_GTT, 0)) {
            r300->context.flush(&r300->context, 0, NULL);
            goto validate;
        }
    }

    if (!r300->winsys->validate(r300->winsys)) {
        r300->context.flush(&r300->context, 0, NULL);
        return r300->winsys->validate(r300->winsys);
    }

    return TRUE;
}

static void r300_shorten_ubyte_elts(struct r300_context* r300,
                                    struct pipe_buffer** elts,
                                    unsigned count)
{
    struct pipe_screen* screen = r300->context.screen;
    struct pipe_buffer* new_elts;
    unsigned char *in_map;
    unsigned short *out_map;
    unsigned i;

    new_elts = screen->buffer_create(screen, 32,
                                     PIPE_BUFFER_USAGE_INDEX |
                                     PIPE_BUFFER_USAGE_CPU_WRITE |
                                     PIPE_BUFFER_USAGE_GPU_READ,
                                     2 * count);

    in_map = pipe_buffer_map(screen, *elts, PIPE_BUFFER_USAGE_CPU_READ);
    out_map = pipe_buffer_map(screen, new_elts, PIPE_BUFFER_USAGE_CPU_WRITE);

    for (i = 0; i < count; i++) {
        *out_map = (unsigned short)*in_map;
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(screen, *elts);
    pipe_buffer_unmap(screen, new_elts);

    *elts = new_elts;
}

/* This is the fast-path drawing & emission for HW TCL. */
void r300_draw_range_elements(struct pipe_context* pipe,
                              struct pipe_buffer* indexBuffer,
                              unsigned indexSize,
                              unsigned minIndex,
                              unsigned maxIndex,
                              unsigned mode,
                              unsigned start,
                              unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    struct pipe_buffer* orgIndexBuffer = indexBuffer;

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    if (count > 65535) {
       /* XXX: use aux/indices functions to split this into smaller
        * primitives.
        */
        return;
    }

    r300_update_derived_state(r300);

    if (!r300_setup_vertex_buffers(r300)) {
        return;
    }

    if (indexSize == 1) {
        r300_shorten_ubyte_elts(r300, &indexBuffer, count);
        indexSize = 2;
    }

    r300_emit_buffer_validate(r300->winsys);

    if (!r300->winsys->add_buffer(r300->winsys, indexBuffer,
                                  RADEON_GEM_DOMAIN_GTT, 0)) {
        goto cleanup;
    }

    if (!r300->winsys->validate(r300->winsys)) {
        goto cleanup;
    }

    r300_emit_dirty_state(r300);

    r300_emit_aos(r300, 0);

    r300_emit_draw_elements(r300, indexBuffer, indexSize, minIndex, maxIndex,
                            mode, start, count);

cleanup:
    if (indexBuffer != orgIndexBuffer) {
        pipe->screen->buffer_destroy(indexBuffer);
    }
}

/* Simple helpers for context setup. Should probably be moved to util. */
void r300_draw_elements(struct pipe_context* pipe,
                        struct pipe_buffer* indexBuffer,
                        unsigned indexSize, unsigned mode,
                        unsigned start, unsigned count)
{
   pipe->draw_range_elements(pipe, indexBuffer, indexSize, 0, ~0,
                             mode, start, count);
}

void r300_draw_arrays(struct pipe_context* pipe, unsigned mode,
                      unsigned start, unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    if (count > 65535) {
        /* XXX: driver needs to handle this -- use the functions in
         * aux/indices to split this into several smaller primitives.
         */
        return;
    }

    r300_update_derived_state(r300);

    r300_emit_buffer_validate(r300);

    if (!r300_setup_vertex_buffers(r300)) {
        return;
    }

    r300_emit_dirty_state(r300);

    if (FALSE && count <= 4 && r300->vertex_buffer_count == 1) {
        r300_emit_draw_immediate(r300, mode, start, count);
    } else {
        r300_emit_aos(r300, start);
        r300_emit_draw_arrays(r300, mode, count);
    }
}

/****************************************************************************
 * The rest of this file is for SW TCL rendering only. Please be polite and *
 * keep these functions separated so that they are easier to locate. ~C.    *
 ***************************************************************************/

/* SW TCL arrays, using Draw. */
void r300_swtcl_draw_arrays(struct pipe_context* pipe,
                               unsigned mode,
                               unsigned start,
                               unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    int i;

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        void* buf = pipe_buffer_map(pipe->screen,
                                    r300->vertex_buffer[i].buffer,
                                    PIPE_BUFFER_USAGE_CPU_READ);
        draw_set_mapped_vertex_buffer(r300->draw, i, buf);
    }

    draw_set_mapped_element_buffer(r300->draw, 0, NULL);

    draw_set_mapped_constant_buffer(r300->draw,
				    PIPE_SHADER_VERTEX,
				    r300->shader_constants[PIPE_SHADER_VERTEX].constants,
				    r300->shader_constants[PIPE_SHADER_VERTEX].count *
                (sizeof(float) * 4));

    draw_arrays(r300->draw, mode, start, count);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        pipe_buffer_unmap(pipe->screen, r300->vertex_buffer[i].buffer);
        draw_set_mapped_vertex_buffer(r300->draw, i, NULL);
    }
}

/* SW TCL elements, using Draw. */
void r300_swtcl_draw_range_elements(struct pipe_context* pipe,
                                       struct pipe_buffer* indexBuffer,
                                       unsigned indexSize,
                                       unsigned minIndex,
                                       unsigned maxIndex,
                                       unsigned mode,
                                       unsigned start,
                                       unsigned count)
{
    struct r300_context* r300 = r300_context(pipe);
    int i;
    void* indices;

    if (!u_trim_pipe_prim(mode, &count)) {
        return;
    }

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        void* buf = pipe_buffer_map(pipe->screen,
                                    r300->vertex_buffer[i].buffer,
                                    PIPE_BUFFER_USAGE_CPU_READ);
        draw_set_mapped_vertex_buffer(r300->draw, i, buf);
    }

    indices = pipe_buffer_map(pipe->screen, indexBuffer,
                              PIPE_BUFFER_USAGE_CPU_READ);
    draw_set_mapped_element_buffer_range(r300->draw, indexSize,
                                         minIndex, maxIndex, indices);

    draw_set_mapped_constant_buffer(r300->draw,
				    PIPE_SHADER_VERTEX,
            r300->shader_constants[PIPE_SHADER_VERTEX].constants,
            r300->shader_constants[PIPE_SHADER_VERTEX].count *
                (sizeof(float) * 4));

    draw_arrays(r300->draw, mode, start, count);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        pipe_buffer_unmap(pipe->screen, r300->vertex_buffer[i].buffer);
        draw_set_mapped_vertex_buffer(r300->draw, i, NULL);
    }

    pipe_buffer_unmap(pipe->screen, indexBuffer);
    draw_set_mapped_element_buffer_range(r300->draw, 0, start,
                                         start + count - 1, NULL);
}

/* Object for rendering using Draw. */
struct r300_render {
    /* Parent class */
    struct vbuf_render base;

    /* Pipe context */
    struct r300_context* r300;

    /* Vertex information */
    size_t vertex_size;
    unsigned prim;
    unsigned hwprim;

    /* VBO */
    struct pipe_buffer* vbo;
    size_t vbo_size;
    size_t vbo_offset;
    size_t vbo_max_used;
    void * vbo_ptr;
};

static INLINE struct r300_render*
r300_render(struct vbuf_render* render)
{
    return (struct r300_render*)render;
}

static const struct vertex_info*
r300_render_get_vertex_info(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;

    r300_update_derived_state(r300);

    return &r300->vertex_info->vinfo;
}

static boolean r300_render_allocate_vertices(struct vbuf_render* render,
                                                   ushort vertex_size,
                                                   ushort count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;
    struct pipe_screen* screen = r300->context.screen;
    size_t size = (size_t)vertex_size * (size_t)count;

    if (size + r300render->vbo_offset > r300render->vbo_size)
    {
        pipe_buffer_reference(&r300->vbo, NULL);
        r300render->vbo = pipe_buffer_create(screen,
                                             64,
                                             PIPE_BUFFER_USAGE_VERTEX,
                                             R300_MAX_VBO_SIZE);
        r300render->vbo_offset = 0;
        r300render->vbo_size = R300_MAX_VBO_SIZE;
    }

    r300render->vertex_size = vertex_size;
    r300->vbo = r300render->vbo;
    r300->vbo_offset = r300render->vbo_offset;

    return (r300render->vbo) ? TRUE : FALSE;
}

static void* r300_render_map_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);
    struct pipe_screen* screen = r300render->r300->context.screen;

    r300render->vbo_ptr = pipe_buffer_map(screen, r300render->vbo,
                                          PIPE_BUFFER_USAGE_CPU_WRITE);

    return ((uint8_t*)r300render->vbo_ptr + r300render->vbo_offset);
}

static void r300_render_unmap_vertices(struct vbuf_render* render,
                                             ushort min,
                                             ushort max)
{
    struct r300_render* r300render = r300_render(render);
    struct pipe_screen* screen = r300render->r300->context.screen;
    CS_LOCALS(r300render->r300);
    BEGIN_CS(2);
    OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, max);
    END_CS;

    r300render->vbo_max_used = MAX2(r300render->vbo_max_used,
                                    r300render->vertex_size * (max + 1));
    pipe_buffer_unmap(screen, r300render->vbo);
}

static void r300_render_release_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);

    r300render->vbo_offset += r300render->vbo_max_used;
    r300render->vbo_max_used = 0;
}

static boolean r300_render_set_primitive(struct vbuf_render* render,
                                               unsigned prim)
{
    struct r300_render* r300render = r300_render(render);

    r300render->prim = prim;
    r300render->hwprim = r300_translate_primitive(prim);

    return TRUE;
}

static void r300_render_draw_arrays(struct vbuf_render* render,
                                          unsigned start,
                                          unsigned count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;

    CS_LOCALS(r300);

    r300_emit_dirty_state(r300);

    DBG(r300, DBG_DRAW, "r300: Doing vbuf render, count %d\n", count);

    BEGIN_CS(2);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_VBUF_2, 0);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_LIST | (count << 16) |
           r300render->hwprim);
    END_CS;
}

static void r300_render_draw(struct vbuf_render* render,
                                   const ushort* indices,
                                   uint count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;
    int i;

    CS_LOCALS(r300);

    r300_emit_dirty_state(r300);

    BEGIN_CS(2 + (count+1)/2);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, (count+1)/2);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
           r300render->hwprim);
    for (i = 0; i < count-1; i += 2) {
        OUT_CS(indices[i+1] << 16 | indices[i]);
    }
    if (count % 2) {
        OUT_CS(indices[count-1]);
    }
    END_CS;
}

static void r300_render_destroy(struct vbuf_render* render)
{
    FREE(render);
}

static struct vbuf_render* r300_render_create(struct r300_context* r300)
{
    struct r300_render* r300render = CALLOC_STRUCT(r300_render);

    r300render->r300 = r300;

    /* XXX find real numbers plz */
    r300render->base.max_vertex_buffer_bytes = 128 * 1024;
    r300render->base.max_indices = 16 * 1024;

    r300render->base.get_vertex_info = r300_render_get_vertex_info;
    r300render->base.allocate_vertices = r300_render_allocate_vertices;
    r300render->base.map_vertices = r300_render_map_vertices;
    r300render->base.unmap_vertices = r300_render_unmap_vertices;
    r300render->base.set_primitive = r300_render_set_primitive;
    r300render->base.draw = r300_render_draw;
    r300render->base.draw_arrays = r300_render_draw_arrays;
    r300render->base.release_vertices = r300_render_release_vertices;
    r300render->base.destroy = r300_render_destroy;

    r300render->vbo = NULL;
    r300render->vbo_size = 0;
    r300render->vbo_offset = 0;

    return &r300render->base;
}

struct draw_stage* r300_draw_stage(struct r300_context* r300)
{
    struct vbuf_render* render;
    struct draw_stage* stage;

    render = r300_render_create(r300);

    if (!render) {
        return NULL;
    }

    stage = draw_vbuf_stage(r300->draw, render);

    if (!stage) {
        render->destroy(render);
        return NULL;
    }

    draw_set_render(r300->draw, render);

    return stage;
}
