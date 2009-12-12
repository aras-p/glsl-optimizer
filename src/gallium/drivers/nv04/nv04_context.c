#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/internal/p_winsys_screen.h"

#include "nv04_context.h"
#include "nv04_screen.h"

static void
nv04_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	draw_flush(nv04->draw);

	FIRE_RING(fence);
}

static void
nv04_destroy(struct pipe_context *pipe)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	if (nv04->draw)
		draw_destroy(nv04->draw);

	FREE(nv04);
}

static void
nv04_set_edgeflags(struct pipe_context *pipe, const unsigned *bitfield)
{
}

static boolean
nv04_init_hwctx(struct nv04_context *nv04)
{
	// requires a valid handle
//	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_NOTIFY, 1);
//	OUT_RING(0);
	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_NOP, 1);
	OUT_RING(0);

	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_CONTROL, 1);
	OUT_RING(0x40182800);
//	OUT_RING(1<<20/*no cull*/);
	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_BLEND, 1);
//	OUT_RING(0x24|(1<<6)|(1<<8));
	OUT_RING(0x120001a4);
	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_FORMAT, 1);
	OUT_RING(0x332213a1);
	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_FILTER, 1);
	OUT_RING(0x11001010);
	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_COLORKEY, 1);
	OUT_RING(0x0);
//	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_OFFSET, 1);
//	OUT_RING(SCREEN_OFFSET);
	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_FOGCOLOR, 1);
	OUT_RING(0xff000000);



	FIRE_RING (NULL);
	return TRUE;
}

struct pipe_context *
nv04_create(struct pipe_screen *pscreen, unsigned pctx_id)
{
	struct nv04_screen *screen = nv04_screen(pscreen);
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv04_context *nv04;
	struct nouveau_winsys *nvws = screen->nvws;

	nv04 = CALLOC(1, sizeof(struct nv04_context));
	if (!nv04)
		return NULL;
	nv04->screen = screen;
	nv04->pctx_id = pctx_id;

	nv04->nvws = nvws;

	nv04->pipe.winsys = ws;
	nv04->pipe.screen = pscreen;
	nv04->pipe.destroy = nv04_destroy;
	nv04->pipe.set_edgeflags = nv04_set_edgeflags;
	nv04->pipe.draw_arrays = nv04_draw_arrays;
	nv04->pipe.draw_elements = nv04_draw_elements;
	nv04->pipe.clear = nv04_clear;
	nv04->pipe.flush = nv04_flush;

	nv04->pipe.is_texture_referenced = nouveau_is_texture_referenced;
	nv04->pipe.is_buffer_referenced = nouveau_is_buffer_referenced;

	nv04_init_surface_functions(nv04);
	nv04_init_state_functions(nv04);

	nv04->draw = draw_create();
	assert(nv04->draw);
	draw_wide_point_threshold(nv04->draw, 0.0);
	draw_wide_line_threshold(nv04->draw, 0.0);
	draw_enable_line_stipple(nv04->draw, FALSE);
	draw_enable_point_sprites(nv04->draw, FALSE);
	draw_set_rasterize_stage(nv04->draw, nv04_draw_vbuf_stage(nv04));

	nv04_init_hwctx(nv04);

	return &nv04->pipe;
}

