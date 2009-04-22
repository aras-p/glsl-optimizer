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

#include "draw/draw_pipe.h"
#include "draw/draw_vbuf.h"
#include "util/u_memory.h"

#include "r300_cs.h"
#include "r300_context.h"
#include "r300_reg.h"
#include "r300_state_derived.h"

/* r300_render: Vertex and index buffer primitive emission. */

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
    void* vbo_map;
    size_t vbo_alloc_size;
    size_t vbo_max_used;
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

    return &r300->vertex_info.vinfo;
}

static boolean r300_render_allocate_vertices(struct vbuf_render* render,
                                                   ushort vertex_size,
                                                   ushort count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;
    struct pipe_screen* screen = r300->context.screen;
    size_t size = (size_t)vertex_size * (size_t)count;

    if (r300render->vbo) {
        pipe_buffer_reference(&r300render->vbo, NULL);
    }

    r300render->vbo_size = MAX2(size, r300render->vbo_alloc_size);
    r300render->vbo_offset = 0;
    r300render->vbo = pipe_buffer_create(screen,
                                         64,
                                         PIPE_BUFFER_USAGE_VERTEX,
                                         r300render->vbo_size);

    r300render->vertex_size = vertex_size;

    if (r300render->vbo) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static void* r300_render_map_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);
    struct pipe_screen* screen = r300render->r300->context.screen;

    r300render->vbo_map = pipe_buffer_map(screen, r300render->vbo,
                                          PIPE_BUFFER_USAGE_CPU_WRITE);

    return (unsigned char*)r300render->vbo_map + r300render->vbo_offset;
}

static void r300_render_unmap_vertices(struct vbuf_render* render,
                                             ushort min,
                                             ushort max)
{
    struct r300_render* r300render = r300_render(render);
    struct pipe_screen* screen = r300render->r300->context.screen;

    r300render->vbo_max_used = MAX2(r300render->vbo_max_used,
             r300render->vertex_size * (max + 1));

    pipe_buffer_unmap(screen, r300render->vbo);
}

static void r300_render_release_vertices(struct vbuf_render* render)
{
    struct r300_render* r300render = r300_render(render);

    pipe_buffer_reference(&r300render->vbo, NULL);
}

static boolean r300_render_set_primitive(struct vbuf_render* render,
                                               unsigned prim)
{
    struct r300_render* r300render = r300_render(render);
    r300render->prim = prim;

    switch (prim) {
        case PIPE_PRIM_POINTS:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_POINTS;
            break;
        case PIPE_PRIM_LINES:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_LINES;
            break;
        case PIPE_PRIM_LINE_LOOP:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_LINE_LOOP;
            break;
        case PIPE_PRIM_LINE_STRIP:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
            break;
        case PIPE_PRIM_TRIANGLES:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_TRIANGLES;
            break;
        case PIPE_PRIM_TRIANGLE_STRIP:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
            break;
        case PIPE_PRIM_TRIANGLE_FAN:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
            break;
        case PIPE_PRIM_QUADS:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_QUADS;
            break;
        case PIPE_PRIM_QUAD_STRIP:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
            break;
        case PIPE_PRIM_POLYGON:
            r300render->hwprim = R300_VAP_VF_CNTL__PRIM_POLYGON;
            break;
        default:
            return FALSE;
            break;
    }

    return TRUE;
}

static void prepare_render(struct r300_render* render, unsigned count)
{
    struct r300_context* r300 = render->r300;

    CS_LOCALS(r300);

    /* Make sure that all possible state is emitted. */
    r300_emit_dirty_state(r300);

    debug_printf("r300: Preparing vertex buffer %p for render, "
            "vertex size %d, vertex count %d\n", render->vbo,
            r300->vertex_info.vinfo.size, count);
    /* Set the pointer to our vertex buffer. The emitted values are this:
     * PACKET3 [3D_LOAD_VBPNTR]
     * COUNT   [1]
     * FORMAT  [size | stride << 8]
     * OFFSET  [0]
     * VBPNTR  [relocated BO]
     */
    BEGIN_CS(7);
    OUT_CS_PKT3(R300_PACKET3_3D_LOAD_VBPNTR, 3);
    OUT_CS(1);
    OUT_CS(r300->vertex_info.vinfo.size |
            (r300->vertex_info.vinfo.size << 8));
    OUT_CS(render->vbo_offset);
    OUT_CS_RELOC(render->vbo, 0, RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_CS;
}

static void r300_render_draw_arrays(struct vbuf_render* render,
                                          unsigned start,
                                          unsigned count)
{
    struct r300_render* r300render = r300_render(render);
    struct r300_context* r300 = r300render->r300;

    CS_LOCALS(r300);

    r300render->vbo_offset = start;

    prepare_render(r300render, count);

    debug_printf("r300: Doing vbuf render, count %d\n", count);

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
    struct pipe_screen* screen = r300->context.screen;
    struct pipe_buffer* index_buffer;
    void* index_map;
    int i;
    uint32_t index;

    CS_LOCALS(r300);

    prepare_render(r300render, count);

    /* Send our indices into an index buffer. */
    index_buffer = pipe_buffer_create(screen, 64, PIPE_BUFFER_USAGE_VERTEX,
                                      count * 2);
    if (!index_buffer) {
        return;
    }

    index_map = pipe_buffer_map(screen, index_buffer,
                                PIPE_BUFFER_USAGE_CPU_WRITE);
    memcpy(index_map, indices, count);
    pipe_buffer_unmap(screen, index_buffer);

    debug_printf("r300: Doing indexbuf render, count %d\n", count);
/*
    BEGIN_CS(8);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, 0);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
           r300render->hwprim);
    OUT_CS_PKT3(R300_PACKET3_INDX_BUFFER, 2);
    OUT_CS(R300_INDX_BUFFER_ONE_REG_WR | (R300_VAP_PORT_IDX0 >> 2));
    OUT_CS_INDEX_RELOC(index_buffer, 0, count, RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_CS; */

    BEGIN_CS(2 + count);
    OUT_CS_PKT3(R300_PACKET3_3D_DRAW_INDX_2, count);
    OUT_CS(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (count << 16) |
           r300render->hwprim | R300_VAP_VF_CNTL__INDEX_SIZE_32bit);
    for (i = 0; i < count; i++) {
        index = indices[i];
        OUT_CS(index);
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
