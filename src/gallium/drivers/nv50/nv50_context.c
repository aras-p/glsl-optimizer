#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_screen.h"

static void
nv50_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	
	FIRE_RING(fence);
}

static void
nv50_destroy(struct pipe_context *pipe)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;

	draw_destroy(nv50->draw);
	free(nv50);
}

struct pipe_context *
nv50_create(struct pipe_screen *pscreen, unsigned pctx_id)
{
	struct pipe_winsys *pipe_winsys = pscreen->winsys;
	struct nv50_screen *screen = nv50_screen(pscreen);
	struct nv50_context *nv50;

	nv50 = CALLOC_STRUCT(nv50_context);
	if (!nv50)
		return NULL;
	nv50->screen = screen;
	nv50->pctx_id = pctx_id;

	nv50->pipe.winsys = pipe_winsys;
	nv50->pipe.screen = pscreen;

	nv50->pipe.destroy = nv50_destroy;

	nv50->pipe.draw_arrays = nv50_draw_arrays;
	nv50->pipe.draw_elements = nv50_draw_elements;
	nv50->pipe.clear = nv50_clear;

	nv50->pipe.flush = nv50_flush;

	nv50_init_miptree_functions(nv50);
	nv50_init_surface_functions(nv50);
	nv50_init_state_functions(nv50);
	nv50_init_query_functions(nv50);

	nv50->draw = draw_create();
	assert(nv50->draw);
	draw_set_rasterize_stage(nv50->draw, nv50_draw_render_stage(nv50));

	return &nv50->pipe;
}

		
