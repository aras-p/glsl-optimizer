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

#include "tgsi/tgsi_scan.h"

#include "util/u_hash_table.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "r300_clear.h"
#include "r300_context.h"
#include "r300_flush.h"
#include "r300_query.h"
#include "r300_render.h"
#include "r300_screen.h"
#include "r300_state_derived.h"
#include "r300_state_invariant.h"
#include "r300_winsys.h"

static enum pipe_error r300_clear_hash_table(void* key, void* value,
                                             void* data)
{
    FREE(key);
    FREE(value);
    return PIPE_OK;
}

static void r300_destroy_context(struct pipe_context* context)
{
    struct r300_context* r300 = r300_context(context);
    struct r300_query* query, * temp;

    util_hash_table_foreach(r300->shader_hash_table, r300_clear_hash_table,
        NULL);
    util_hash_table_destroy(r300->shader_hash_table);

    draw_destroy(r300->draw);

    /* Free the OQ BO. */
    context->screen->buffer_destroy(r300->oqbo);

    /* If there are any queries pending or not destroyed, remove them now. */
    foreach_s(query, temp, &r300->query_list) {
        remove_from_list(query);
        FREE(query);
    }

    FREE(r300->blend_color_state);
    FREE(r300->rs_block);
    FREE(r300->scissor_state);
    FREE(r300->viewport_state);
    FREE(r300);
}

static unsigned int
r300_is_texture_referenced(struct pipe_context *pipe,
                           struct pipe_texture *texture,
                           unsigned face, unsigned level)
{
    struct pipe_buffer* buf = 0;

    r300_get_texture_buffer(texture, &buf, NULL);

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

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct r300_winsys* r300_winsys)
{
    struct r300_context* r300 = CALLOC_STRUCT(r300_context);
    struct r300_screen* r300screen = r300_screen(screen);

    if (!r300)
        return NULL;

    r300->winsys = r300_winsys;

    r300->context.winsys = (struct pipe_winsys*)r300_winsys;
    r300->context.screen = screen;

    r300_init_debug(r300);

    r300->context.destroy = r300_destroy_context;

    r300->context.clear = r300_clear;

    if (r300screen->caps->has_tcl)
    {
        r300->context.draw_arrays = r300_draw_arrays;
        r300->context.draw_elements = r300_draw_elements;
        r300->context.draw_range_elements = r300_draw_range_elements;
    }
    else
    {
        assert(0);
    }

    r300->context.is_texture_referenced = r300_is_texture_referenced;
    r300->context.is_buffer_referenced = r300_is_buffer_referenced;

    r300->shader_hash_table = util_hash_table_create(r300_shader_key_hash,
        r300_shader_key_compare);

    r300->blend_color_state = CALLOC_STRUCT(r300_blend_color_state);
    r300->rs_block = CALLOC_STRUCT(r300_rs_block);
    r300->scissor_state = CALLOC_STRUCT(r300_scissor_state);
    r300->viewport_state = CALLOC_STRUCT(r300_viewport_state);

    /* Create a Draw. This is used for vert collation and SW TCL. */
    r300->draw = draw_create();
    /* Enable our renderer. */
    draw_set_rasterize_stage(r300->draw, r300_draw_stage(r300));
    /* Disable Draw's clipping if TCL is present. */
    draw_set_driver_clipping(r300->draw, r300_screen(screen)->caps->has_tcl);
    /* Force Draw to never do viewport transform, since (again) we can do
     * transform in hardware, always. */
    draw_set_viewport_state(r300->draw, &r300_viewport_identity);

    /* Open up the OQ BO. */
    r300->oqbo = screen->buffer_create(screen, 4096,
            PIPE_BUFFER_USAGE_VERTEX, 4096);
    make_empty_list(&r300->query_list);

    r300_init_flush_functions(r300);

    r300_init_query_functions(r300);

    /* r300_init_surface_functions(r300); */

    r300_init_state_functions(r300);

    r300_emit_invariant_state(r300);

    r300->winsys->set_flush_cb(r300->winsys, r300_flush_cb, r300);
    r300->dirty_state = R300_NEW_KITCHEN_SINK;
    r300->dirty_hw++;
    return &r300->context;
}
