#include "r300_context.h"

static void r300_destroy_context(struct pipe_context* context) {
    struct r300_context* r300 = r300_context(context);

    draw_destroy(r300->draw);

    FREE(context);
}

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct pipe_winsys* winsys,
                                         struct amd_winsys* amd_winsys)
{
    struct r300_context* r300 = CALLOC_STRUCT(r300_context);

    if (!r300)
        return NULL;

    r300->winsys = amd_winsys;
    r300->context.winsys = winsys;
    r300->context.screen = screen;

    r300->context.destroy = r300_destroy_context;

    return &r300->context;
}