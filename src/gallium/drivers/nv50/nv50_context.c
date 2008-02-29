#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_screen.h"

static void
nv50_flush(struct pipe_context *pipe, unsigned flags)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->nvws;
	
	if (flags & PIPE_FLUSH_WAIT) {
		nvws->notifier_reset(nv50->sync, 0);
		BEGIN_RING(tesla, 0x104, 1);
		OUT_RING  (0);
		BEGIN_RING(tesla, 0x100, 1);
		OUT_RING  (0);
	}

	FIRE_RING();

	if (flags & PIPE_FLUSH_WAIT)
		nvws->notifier_wait(nv50->sync, 0, 0, 2000);
}

static void
nv50_destroy(struct pipe_context *pipe)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;

	draw_destroy(nv50->draw);
	free(nv50);
}

static boolean
nv50_init_hwctx(struct nv50_context *nv50, int tesla_class)
{
	struct nouveau_winsys *nvws = nv50->nvws;
	int ret;

	if ((ret = nvws->grobj_alloc(nvws, tesla_class, &nv50->tesla))) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	BEGIN_RING(tesla, NV50TCL_DMA_NOTIFY, 1);
	OUT_RING  (nv50->sync->handle);

	FIRE_RING ();
	return TRUE;
}

#define GRCLASS5097_CHIPSETS 0x00000000
#define GRCLASS8297_CHIPSETS 0x00000010
struct pipe_context *
nv50_create(struct pipe_screen *pscreen, struct nouveau_winsys *nvws)
{
	struct pipe_winsys *pipe_winsys = pscreen->winsys;
	unsigned chipset = nv50_screen(pscreen)->chipset;
	struct nv50_context *nv50;
	int tesla_class, ret;

	if ((chipset & 0xf0) != 0x50 && (chipset & 0xf0) != 0x80) {
		NOUVEAU_ERR("Not a G8x chipset\n");
		return NULL;
	}

	if (GRCLASS5097_CHIPSETS & (1 << (chipset & 0x0f))) {
		tesla_class = 0x5097;
	} else
	if (GRCLASS8297_CHIPSETS & (1 << (chipset & 0x0f))) {
		tesla_class = 0x8297;
	} else {
		NOUVEAU_ERR("Unknown G8x chipset: NV%02x\n", chipset);
		return NULL;
	}

	nv50 = CALLOC_STRUCT(nv50_context);
	if (!nv50)
		return NULL;
	nv50->chipset = chipset;
	nv50->nvws = nvws;

	if ((ret = nvws->notifier_alloc(nvws, 1, &nv50->sync))) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		free(nv50);
		return NULL;
	}

	if (!nv50_init_hwctx(nv50, tesla_class)) {
		free(nv50);
		return NULL;
	}

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

		
