#include "draw/draw_context.h"
#include "pipe/p_defines.h"

#include "nvfx_context.h"
#include "nvfx_screen.h"
#include "nvfx_resource.h"

static void
nvfx_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;

	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(chan, eng3d, 0x1fd8, 1);
		OUT_RING  (chan, 2);
		BEGIN_RING(chan, eng3d, 0x1fd8, 1);
		OUT_RING  (chan, 1);
	}

	FIRE_RING(chan);
	if (fence)
		*fence = NULL;
}

static void
nvfx_destroy(struct pipe_context *pipe)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	if (nvfx->draw)
		draw_destroy(nvfx->draw);
	FREE(nvfx);
}

struct pipe_context *
nvfx_create(struct pipe_screen *pscreen, void *priv)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);
	struct pipe_winsys *ws = pscreen->winsys;
	struct nvfx_context *nvfx;
	struct nouveau_winsys *nvws = screen->nvws;

	nvfx = CALLOC(1, sizeof(struct nvfx_context));
	if (!nvfx)
		return NULL;
	nvfx->screen = screen;

	nvfx->nvws = nvws;

	nvfx->pipe.winsys = ws;
	nvfx->pipe.screen = pscreen;
	nvfx->pipe.priv = priv;
	nvfx->pipe.destroy = nvfx_destroy;
	nvfx->pipe.draw_arrays = nvfx_draw_arrays;
	nvfx->pipe.draw_elements = nvfx_draw_elements;
	nvfx->pipe.clear = nvfx_clear;
	nvfx->pipe.flush = nvfx_flush;

	screen->base.channel->user_private = nvfx;

	nvfx->is_nv4x = screen->is_nv4x;

	nvfx_init_query_functions(nvfx);
	nvfx_init_surface_functions(nvfx);
	nvfx_init_state_functions(nvfx);
	nvfx_init_resource_functions(&nvfx->pipe);

	/* Create, configure, and install fallback swtnl path */
	nvfx->draw = draw_create();
	draw_wide_point_threshold(nvfx->draw, 9999999.0);
	draw_wide_line_threshold(nvfx->draw, 9999999.0);
	draw_enable_line_stipple(nvfx->draw, FALSE);
	draw_enable_point_sprites(nvfx->draw, FALSE);
	draw_set_rasterize_stage(nvfx->draw, nvfx_draw_render_stage(nvfx));

	/* set these to that we init them on first validation */
	nvfx->state.scissor_enabled = ~0;
	nvfx->state.stipple_enabled = ~0;
	return &nvfx->pipe;
}
