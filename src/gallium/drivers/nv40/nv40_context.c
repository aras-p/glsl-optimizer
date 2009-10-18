#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/internal/p_winsys_screen.h"

#include "nv40_context.h"
#include "nv40_screen.h"

static void
nv40_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(curie, 0x1fd8, 1);
		OUT_RING  (2);
		BEGIN_RING(curie, 0x1fd8, 1);
		OUT_RING  (1);
	}

	FIRE_RING(fence);
}

static void
nv40_destroy(struct pipe_context *pipe)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	if (nv40->draw)
		draw_destroy(nv40->draw);
	FREE(nv40);
}

struct pipe_context *
nv40_create(struct pipe_screen *pscreen, unsigned pctx_id)
{
	struct nv40_screen *screen = nv40_screen(pscreen);
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv40_context *nv40;
	struct nouveau_winsys *nvws = screen->nvws;

	nv40 = CALLOC(1, sizeof(struct nv40_context));
	if (!nv40)
		return NULL;
	nv40->screen = screen;
	nv40->pctx_id = pctx_id;

	nv40->nvws = nvws;

	nv40->pipe.winsys = ws;
	nv40->pipe.screen = pscreen;
	nv40->pipe.destroy = nv40_destroy;
	nv40->pipe.draw_arrays = nv40_draw_arrays;
	nv40->pipe.draw_elements = nv40_draw_elements;
	nv40->pipe.clear = nv40_clear;
	nv40->pipe.flush = nv40_flush;

	nv40->pipe.is_texture_referenced = nouveau_is_texture_referenced;
	nv40->pipe.is_buffer_referenced = nouveau_is_buffer_referenced;

	nv40_init_query_functions(nv40);
	nv40_init_surface_functions(nv40);
	nv40_init_state_functions(nv40);

	/* Create, configure, and install fallback swtnl path */
	nv40->draw = draw_create();
	draw_wide_point_threshold(nv40->draw, 9999999.0);
	draw_wide_line_threshold(nv40->draw, 9999999.0);
	draw_enable_line_stipple(nv40->draw, FALSE);
	draw_enable_point_sprites(nv40->draw, FALSE);
	draw_set_rasterize_stage(nv40->draw, nv40_draw_render_stage(nv40));

	return &nv40->pipe;
}
