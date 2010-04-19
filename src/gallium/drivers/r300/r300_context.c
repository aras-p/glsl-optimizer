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

#include "draw/draw_context.h"

#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "r300_blit.h"
#include "r300_context.h"
#include "r300_emit.h"
#include "r300_flush.h"
#include "r300_query.h"
#include "r300_render.h"
#include "r300_screen.h"
#include "r300_state_invariant.h"
#include "r300_texture.h"

#include "radeon_winsys.h"

static void r300_destroy_context(struct pipe_context* context)
{
    struct r300_context* r300 = r300_context(context);
    struct r300_query* query, * temp;

    util_blitter_destroy(r300->blitter);
    draw_destroy(r300->draw);

    /* Free the OQ BO. */
    context->screen->buffer_destroy(r300->oqbo);

    /* If there are any queries pending or not destroyed, remove them now. */
    foreach_s(query, temp, &r300->query_list) {
        remove_from_list(query);
        FREE(query);
    }

    FREE(r300->blend_color_state.state);
    FREE(r300->clip_state.state);
    FREE(r300->fb_state.state);
    FREE(r300->rs_block_state.state);
    FREE(r300->scissor_state.state);
    FREE(r300->textures_state.state);
    FREE(r300->vertex_stream_state.state);
    FREE(r300->vap_output_state.state);
    FREE(r300->viewport_state.state);
    FREE(r300->ztop_state.state);
    FREE(r300);
}

static unsigned int
r300_is_texture_referenced(struct pipe_context *pipe,
                           struct pipe_texture *texture,
                           unsigned face, unsigned level)
{
    struct pipe_buffer* buf = 0;

    r300_get_texture_buffer(pipe->screen, texture, &buf, NULL);

    return pipe->is_buffer_referenced(pipe, buf);
}

static unsigned int
r300_is_buffer_referenced(struct pipe_context *pipe,
                          struct pipe_buffer *buf)
{
    /* This only checks to see whether actual hardware buffers are
     * referenced. Since we use managed BOs and transfers, it's actually not
     * possible for pipe_buffers to ever reference the actual hardware, so
     * buffers are never referenced. */
    return 0;
}

static void r300_flush_cb(void *data)
{
    struct r300_context* const cs_context_copy = data;

    cs_context_copy->context.flush(&cs_context_copy->context, 0, NULL);
}

#define R300_INIT_ATOM(atomname, atomsize) \
    r300->atomname.name = #atomname; \
    r300->atomname.state = NULL; \
    r300->atomname.size = atomsize; \
    r300->atomname.emit = r300_emit_##atomname; \
    r300->atomname.dirty = FALSE; \
    insert_at_tail(&r300->atom_list, &r300->atomname);

static void r300_setup_atoms(struct r300_context* r300)
{
    boolean is_r500 = r300_screen(r300->context.screen)->caps->is_r500;
    boolean has_tcl = r300_screen(r300->context.screen)->caps->has_tcl;

    /* Create the actual atom list.
     *
     * Each atom is examined and emitted in the order it appears here, which
     * can affect performance and conformance if not handled with care.
     *
     * Some atoms never change size, others change every emit - those have
     * the size of 0 here. */
    make_empty_list(&r300->atom_list);
    R300_INIT_ATOM(invariant_state, 71);
    R300_INIT_ATOM(ztop_state, 2);
    R300_INIT_ATOM(blend_state, 8);
    R300_INIT_ATOM(blend_color_state, is_r500 ? 3 : 2);
    R300_INIT_ATOM(clip_state, has_tcl ? 5 + (6 * 4) : 2);
    R300_INIT_ATOM(dsa_state, is_r500 ? 8 : 6);
    R300_INIT_ATOM(fb_state, 0);
    R300_INIT_ATOM(rs_state, 0);
    R300_INIT_ATOM(scissor_state, 3);
    R300_INIT_ATOM(viewport_state, 9);
    R300_INIT_ATOM(rs_block_state, 0);
    R300_INIT_ATOM(vertex_stream_state, 0);
    R300_INIT_ATOM(vap_output_state, 6);
    R300_INIT_ATOM(pvs_flush, 2);
    R300_INIT_ATOM(vs_state, 0);
    R300_INIT_ATOM(texture_cache_inval, 2);
    R300_INIT_ATOM(textures_state, 0);

    /* Some non-CSO atoms need explicit space to store the state locally. */
    r300->blend_color_state.state = CALLOC_STRUCT(r300_blend_color_state);
    r300->clip_state.state = CALLOC_STRUCT(pipe_clip_state);
    r300->fb_state.state = CALLOC_STRUCT(pipe_framebuffer_state);
    r300->rs_block_state.state = CALLOC_STRUCT(r300_rs_block);
    r300->scissor_state.state = CALLOC_STRUCT(pipe_scissor_state);
    r300->textures_state.state = CALLOC_STRUCT(r300_textures_state);
    r300->vertex_stream_state.state = CALLOC_STRUCT(r300_vertex_stream_state);
    r300->vap_output_state.state = CALLOC_STRUCT(r300_vap_output_state);
    r300->viewport_state.state = CALLOC_STRUCT(r300_viewport_state);
    r300->ztop_state.state = CALLOC_STRUCT(r300_ztop_state);
}

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         void *priv)
{
    struct r300_context* r300 = CALLOC_STRUCT(r300_context);
    struct r300_screen* r300screen = r300_screen(screen);
    struct radeon_winsys* radeon_winsys = r300screen->radeon_winsys;

    if (!r300)
        return NULL;

    r300->winsys = radeon_winsys;

    r300->context.winsys = (struct pipe_winsys*)radeon_winsys;
    r300->context.screen = screen;
    r300->context.priv = priv;

    r300->context.destroy = r300_destroy_context;

    r300->context.clear = r300_clear;
    r300->context.surface_copy = r300_surface_copy;
    r300->context.surface_fill = r300_surface_fill;

    if (r300screen->caps->has_tcl) {
        r300->context.draw_arrays = r300_draw_arrays;
        r300->context.draw_elements = r300_draw_elements;
        r300->context.draw_range_elements = r300_draw_range_elements;
    } else {
        r300->context.draw_arrays = r300_swtcl_draw_arrays;
        r300->context.draw_elements = r300_draw_elements;
        r300->context.draw_range_elements = r300_swtcl_draw_range_elements;

        /* Create a Draw. This is used for SW TCL. */
        r300->draw = draw_create(&r300->context);
        /* Enable our renderer. */
        draw_set_rasterize_stage(r300->draw, r300_draw_stage(r300));
        /* Enable Draw's clipping. */
        draw_set_driver_clipping(r300->draw, FALSE);
        /* Force Draw to never do viewport transform, since we can do
         * transform in hardware, always. */
        draw_set_viewport_state(r300->draw, &r300_viewport_identity);
    }

    r300->context.is_texture_referenced = r300_is_texture_referenced;
    r300->context.is_buffer_referenced = r300_is_buffer_referenced;

    r300_setup_atoms(r300);

    /* Open up the OQ BO. */
    r300->oqbo = screen->buffer_create(screen, 4096,
            PIPE_BUFFER_USAGE_VERTEX, 4096);
    make_empty_list(&r300->query_list);

    r300_init_flush_functions(r300);

    r300_init_query_functions(r300);

    /* r300_init_surface_functions(r300); */

    r300_init_state_functions(r300);

    r300->invariant_state.dirty = TRUE;

    r300->winsys->set_flush_cb(r300->winsys, r300_flush_cb, r300);
    r300->dirty_state = R300_NEW_KITCHEN_SINK;
    r300->dirty_hw++;

    r300->blitter = util_blitter_create(&r300->context);

    return &r300->context;
}
