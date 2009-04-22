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

#include "r300_context.h"

static boolean r300_draw_range_elements(struct pipe_context* pipe,
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

    if (r300->dirty_state) {
        r300_emit_dirty_state(r300);
    }

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        void* buf = pipe_buffer_map(pipe->screen,
                                    r300->vertex_buffers[i].buffer,
                                    PIPE_BUFFER_USAGE_CPU_READ);
        draw_set_mapped_vertex_buffer(r300->draw, i, buf);
    }

    if (indexBuffer) {
        void* indices = pipe_buffer_map(pipe->screen, indexBuffer,
                                        PIPE_BUFFER_USAGE_CPU_READ);
        draw_set_mapped_element_buffer_range(r300->draw, indexSize,
                                             minIndex, maxIndex, indices);
    } else {
        draw_set_mapped_element_buffer(r300->draw, 0, NULL);
    }

    draw_set_mapped_constant_buffer(r300->draw,
            r300->shader_constants[PIPE_SHADER_VERTEX].constants,
            r300->shader_constants[PIPE_SHADER_VERTEX].user_count *
                (sizeof(float) * 4));

    draw_arrays(r300->draw, mode, start, count);

    for (i = 0; i < r300->vertex_buffer_count; i++) {
        pipe_buffer_unmap(pipe->screen, r300->vertex_buffers[i].buffer);
        draw_set_mapped_vertex_buffer(r300->draw, i, NULL);
    }

    if (indexBuffer) {
        pipe_buffer_unmap(pipe->screen, indexBuffer);
        draw_set_mapped_element_buffer_range(r300->draw, 0, start,
                                             start + count - 1, NULL);
    }

    return TRUE;
}

static boolean r300_draw_elements(struct pipe_context* pipe,
                                  struct pipe_buffer* indexBuffer,
                                  unsigned indexSize, unsigned mode,
                                  unsigned start, unsigned count)
{
    return r300_draw_range_elements(pipe, indexBuffer, indexSize, 0, ~0,
                                    mode, start, count);
}

static boolean r300_draw_arrays(struct pipe_context* pipe, unsigned mode,
                                unsigned start, unsigned count)
{
    return r300_draw_elements(pipe, NULL, 0, mode, start, count);
}

static void r300_destroy_context(struct pipe_context* context) {
    struct r300_context* r300 = r300_context(context);

    draw_destroy(r300->draw);

    FREE(r300->blend_color_state);
    FREE(r300->rs_block);
    FREE(r300->scissor_state);
    FREE(r300->viewport_state);
    FREE(r300);
}

static unsigned int
r300_is_texture_referenced( struct pipe_context *pipe,
			    struct pipe_texture *texture,
			    unsigned face, unsigned level)
{
   /**
    * FIXME: Optimize.
    */

   return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
}

static unsigned int
r300_is_buffer_referenced( struct pipe_context *pipe,
			   struct pipe_buffer *buf)
{
   /**
    * FIXME: Optimize.
    */

   return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
}

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct r300_winsys* r300_winsys)
{
    struct r300_context* r300 = CALLOC_STRUCT(r300_context);

    if (!r300)
        return NULL;

    /* XXX this could be refactored now? */
    r300->winsys = r300_winsys;

    r300->context.winsys = (struct pipe_winsys*)r300_winsys;
    r300->context.screen = r300_screen(screen);

    r300->context.destroy = r300_destroy_context;

    r300->context.clear = r300_clear;

    r300->context.draw_arrays = r300_draw_arrays;
    r300->context.draw_elements = r300_draw_elements;
    r300->context.draw_range_elements = r300_draw_range_elements;

    r300->context.is_texture_referenced = r300_is_texture_referenced;
    r300->context.is_buffer_referenced = r300_is_buffer_referenced;

    r300->draw = draw_create();
    draw_set_rasterize_stage(r300->draw, r300_draw_stage(r300));

    r300->blend_color_state = CALLOC_STRUCT(r300_blend_color_state);
    r300->rs_block = CALLOC_STRUCT(r300_rs_block);
    r300->scissor_state = CALLOC_STRUCT(r300_scissor_state);
    r300->viewport_state = CALLOC_STRUCT(r300_viewport_state);

    r300_init_flush_functions(r300);

    r300_init_query_functions(r300);

    r300_init_surface_functions(r300);

    r300_init_state_functions(r300);

    r300_emit_invariant_state(r300);
    r300->dirty_state = R300_NEW_KITCHEN_SINK;
    r300->dirty_hw++;

    return &r300->context;
}
