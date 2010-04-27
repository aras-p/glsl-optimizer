#include "draw/draw_context.h"
#include "pipe/p_defines.h"

#include "nv30_context.h"
#include "nv30_screen.h"

static void
nv30_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;

	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(chan, rankine, 0x1fd8, 1);
		OUT_RING  (chan, 2);
		BEGIN_RING(chan, rankine, 0x1fd8, 1);
		OUT_RING  (chan, 1);
	}

	FIRE_RING(chan);
	if (fence)
		*fence = NULL;
}

static void
nv30_destroy(struct pipe_context *pipe)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	unsigned i;

	for (i = 0; i < NV30_STATE_MAX; i++) {
		if (nv30->state.hw[i])
			so_ref(NULL, &nv30->state.hw[i]);
	}

	if (nv30->draw)
		draw_destroy(nv30->draw);
	FREE(nv30);
}

struct pipe_context *
nv30_create(struct pipe_screen *pscreen, void *priv)
{
	struct nv30_screen *screen = nv30_screen(pscreen);
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv30_context *nv30;
	struct nouveau_winsys *nvws = screen->nvws;

	nv30 = CALLOC(1, sizeof(struct nv30_context));
	if (!nv30)
		return NULL;
	nv30->screen = screen;

	nv30->nvws = nvws;

	nv30->pipe.winsys = ws;
	nv30->pipe.screen = pscreen;
	nv30->pipe.priv = priv;
	nv30->pipe.destroy = nv30_destroy;
	nv30->pipe.draw_arrays = nv30_draw_arrays;
	nv30->pipe.draw_elements = nv30_draw_elements;
	nv30->pipe.clear = nv30_clear;
	nv30->pipe.flush = nv30_flush;

	nv30->pipe.is_texture_referenced = nouveau_is_texture_referenced;
	nv30->pipe.is_buffer_referenced = nouveau_is_buffer_referenced;

	screen->base.channel->user_private = nv30;
	screen->base.channel->flush_notify = nv30_state_flush_notify;

	nv30_init_query_functions(nv30);
	nv30_init_surface_functions(nv30);
	nv30_init_state_functions(nv30);

	/* Create, configure, and install fallback swtnl path */
	nv30->draw = draw_create(&nv30->pipe);
	draw_wide_point_threshold(nv30->draw, 9999999.0);
	draw_wide_line_threshold(nv30->draw, 9999999.0);
	draw_enable_line_stipple(nv30->draw, FALSE);
	draw_enable_point_sprites(nv30->draw, FALSE);
	draw_set_rasterize_stage(nv30->draw, nv30_draw_render_stage(nv30));

	return &nv30->pipe;
}
