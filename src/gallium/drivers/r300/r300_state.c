#include "r300_context.h"

static void* r300_create_vs_state(struct pipe_context* pipe,
                                  struct pipe_shader_state* state)
{
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    return draw_create_vertex_shader(context->draw, state);
}

static void r300_bind_vs_state(struct pipe_context* pipe, void* state) {
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    draw_bind_vertex_shader(context->draw, (struct draw_vertex_shader*)state);
}

static void r300_delete_vs_state(struct pipe_context* pipe, void* state)
{
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    draw_delete_vertex_shader(context->draw, (struct draw_vertex_shader*)state);
}