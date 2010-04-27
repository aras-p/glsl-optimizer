#include "draw/draw_context.h"
#include "pipe/p_defines.h"

#include "nv40_context.h"
#include "nv40_screen.h"

static void
nv40_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;

	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(chan, curie, 0x1fd8, 1);
		OUT_RING  (chan, 2);
		BEGIN_RING(chan, curie, 0x1fd8, 1);
		OUT_RING  (chan, 1);
	}

	FIRE_RING(chan);
	if (fence)
		*fence = NULL;
}

static void
nv40_destroy(struct pipe_context *pipe)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	unsigned i;

	for (i = 0; i < NV40_STATE_MAX; i++) {
		if (nv40->state.hw[i])
			so_ref(NULL, &nv40->state.hw[i]);
	}

	if (nv40->draw)
		draw_destroy(nv40->draw);
	FREE(nv40);
}

struct pipe_context *
nv40_create(struct pipe_screen *pscreen, void *priv)
{
	struct nv40_screen *screen = nv40_screen(pscreen);
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv40_context *nv40;
	struct nouveau_winsys *nvws = screen->nvws;

	nv40 = CALLOC(1, sizeof(struct nv40_context));
	if (!nv40)
		return NULL;
	nv40->screen = screen;

	nv40->nvws = nvws;

	nv40->pipe.winsys = ws;
	nv40->pipe.priv = priv;
	nv40->pipe.screen = pscreen;
	nv40->pipe.destroy = nv40_destroy;
	nv40->pipe.draw_arrays = nv40_draw_arrays;
	nv40->pipe.draw_elements = nv40_draw_elements;
	nv40->pipe.clear = nv40_clear;
	nv40->pipe.flush = nv40_flush;

	nv40->pipe.is_texture_referenced = nouveau_is_texture_referenced;
	nv40->pipe.is_buffer_referenced = nouveau_is_buffer_referenced;

	screen->base.channel->user_private = nv40;
	screen->base.channel->flush_notify = nv40_state_flush_notify;

	nv40_init_query_functions(nv40);
	nv40_init_surface_functions(nv40);
	nv40_init_state_functions(nv40);

	/* Create, configure, and install fallback swtnl path */
	nv40->draw = draw_create(&nv40->pipe);
	draw_wide_point_threshold(nv40->draw, 9999999.0);
	draw_wide_line_threshold(nv40->draw, 9999999.0);
	draw_enable_line_stipple(nv40->draw, FALSE);
	draw_enable_point_sprites(nv40->draw, FALSE);
	draw_set_rasterize_stage(nv40->draw, nv40_draw_render_stage(nv40));

	return &nv40->pipe;
}
