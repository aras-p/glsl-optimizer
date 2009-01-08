#include "r300_context.h"

static void r300_destroy_context(struct pipe_context* pipe) {
    struct r300_context* context = r300_context(pipe);

    draw_destroy(context->draw);

    FREE(context);
}

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct pipe_winsys* winsys,
                                         struct amd_winsys* amd_winsys)
{
    struct r300_context* context = CALLOC_STRUCT(r300_context);

    if (!context)
        return NULL;

    context->winsys = amd_winsys;
    context->context.winsys = winsys;
    context->context.screen = screen;

    context->context.destroy = r300_destroy_context;

    return &context->context;
}