#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "nv30_context.h"
#include "nv30_screen.h"

static void
nv30_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	
	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(rankine, 0x1fd8, 1);
		OUT_RING  (2);
		BEGIN_RING(rankine, 0x1fd8, 1);
		OUT_RING  (1);
	}

	FIRE_RING(fence);
}

static void
nv30_destroy(struct pipe_context *pipe)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nouveau_winsys *nvws = nv30->nvws;

	if (nv30->draw)
		draw_destroy(nv30->draw);

	nvws->res_free(&nv30->vertprog.exec_heap);
	nvws->res_free(&nv30->vertprog.data_heap);

	nvws->res_free(&nv30->query_heap);
	nvws->notifier_free(&nv30->query);

	nvws->notifier_free(&nv30->sync);

	nvws->grobj_free(&nv30->rankine);

	free(nv30);
}

static boolean
nv30_init_hwctx(struct nv30_context *nv30, int rankine_class)
{
	struct nouveau_winsys *nvws = nv30->nvws;
	int ret;
	int i;

	ret = nvws->grobj_alloc(nvws, rankine_class, &nv30->rankine);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	BEGIN_RING(rankine, NV34TCL_DMA_NOTIFY, 1);
	OUT_RING  (nv30->sync->handle);
	BEGIN_RING(rankine, NV34TCL_DMA_TEXTURE0, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->gart->handle);
	BEGIN_RING(rankine, NV34TCL_DMA_COLOR1, 1);
	OUT_RING  (nvws->channel->vram->handle);
	BEGIN_RING(rankine, NV34TCL_DMA_COLOR0, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->vram->handle);
	BEGIN_RING(rankine, NV34TCL_DMA_VTXBUF0, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->gart->handle);
/*	BEGIN_RING(rankine, NV34TCL_DMA_FENCE, 2);
	OUT_RING  (0);
	OUT_RING  (nv30->query->handle);*/
	BEGIN_RING(rankine, NV34TCL_DMA_IN_MEMORY7, 1);
	OUT_RING  (nvws->channel->vram->handle);
	BEGIN_RING(rankine, NV34TCL_DMA_IN_MEMORY8, 1);
	OUT_RING  (nvws->channel->vram->handle);

	for (i=1; i<8; i++) {
		BEGIN_RING(rankine, NV34TCL_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING  (0);
		BEGIN_RING(rankine, NV34TCL_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING  (0);
	}

	BEGIN_RING(rankine, 0x220, 1);
	OUT_RING  (1);

	BEGIN_RING(rankine, 0x03b0, 1);
	OUT_RING  (0x00100000);
	BEGIN_RING(rankine, 0x1454, 1);
	OUT_RING  (0);
	BEGIN_RING(rankine, 0x1d80, 1);
	OUT_RING  (3);
	BEGIN_RING(rankine, 0x1450, 1);
	OUT_RING  (0x00030004);
	
	/* NEW */
	BEGIN_RING(rankine, 0x1e98, 1);
	OUT_RING  (0);
	BEGIN_RING(rankine, 0x17e0, 3);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0x3f800000);
	BEGIN_RING(rankine, 0x1f80, 16);
	OUT_RING  (0); OUT_RING  (0); OUT_RING  (0); OUT_RING  (0); 
	OUT_RING  (0); OUT_RING  (0); OUT_RING  (0); OUT_RING  (0); 
	OUT_RING  (0x0000ffff);
	OUT_RING  (0); OUT_RING  (0); OUT_RING  (0); OUT_RING  (0); 
	OUT_RING  (0); OUT_RING  (0); OUT_RING  (0); 

	BEGIN_RING(rankine, 0x120, 3);
	OUT_RING  (0);
	OUT_RING  (1);
	OUT_RING  (2);

	BEGIN_RING(rankine, 0x1d88, 1);
	OUT_RING  (0x00001200);

	BEGIN_RING(rankine, NV34TCL_RC_ENABLE, 1);
	OUT_RING  (0);

	BEGIN_RING(rankine, NV34TCL_DEPTH_RANGE_NEAR, 2);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);

	BEGIN_RING(rankine, NV34TCL_MULTISAMPLE_CONTROL, 1);
	OUT_RING  (0xffff0000);

	/* enables use of vp rather than fixed-function somehow */
	BEGIN_RING(rankine, 0x1e94, 1);
	OUT_RING  (0x13);

	FIRE_RING (NULL);
	return TRUE;
}

#define NV30TCL_CHIPSET_3X_MASK 0x00000003
#define NV34TCL_CHIPSET_3X_MASK 0x00000010
#define NV35TCL_CHIPSET_3X_MASK 0x000001e0

struct pipe_context *
nv30_create(struct pipe_screen *screen, unsigned pctx_id)
{
	struct pipe_winsys *pipe_winsys = screen->winsys;
	struct nouveau_winsys *nvws = nv30_screen(screen)->nvws;
	unsigned chipset = nv30_screen(screen)->chipset;
	struct nv30_context *nv30;
	int rankine_class = 0, ret;

	if ((chipset & 0xf0) != 0x30) {
		NOUVEAU_ERR("Not a NV3X chipset\n");
		return NULL;
	}

	if (NV30TCL_CHIPSET_3X_MASK & (1 << (chipset & 0x0f))) {
		rankine_class = 0x0397;
	} else if (NV34TCL_CHIPSET_3X_MASK & (1 << (chipset & 0x0f))) {
		rankine_class = 0x0697;
	} else if (NV35TCL_CHIPSET_3X_MASK & (1 << (chipset & 0x0f))) {
		rankine_class = 0x0497;
	} else {
		NOUVEAU_ERR("Unknown NV3X chipset: NV%02x\n", chipset);
		return NULL;
	}

	nv30 = CALLOC_STRUCT(nv30_context);
	if (!nv30)
		return NULL;
	nv30->chipset = chipset;
	nv30->nvws = nvws;

	/* Notifier for sync purposes */
	ret = nvws->notifier_alloc(nvws, 1, &nv30->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv30_destroy(&nv30->pipe);
		return NULL;
	}

	/* Query objects */
	ret = nvws->notifier_alloc(nvws, 32, &nv30->query);
	if (ret) {
		NOUVEAU_ERR("Error initialising query objects: %d\n", ret);
		nv30_destroy(&nv30->pipe);
		return NULL;
	}

	ret = nvws->res_init(&nv30->query_heap, 0, 32);
	if (ret) {
		NOUVEAU_ERR("Error initialising query object heap: %d\n", ret);
		nv30_destroy(&nv30->pipe);
		return NULL;
	}

	/* Vtxprog resources */
	if (nvws->res_init(&nv30->vertprog.exec_heap, 0, 256) ||
	    nvws->res_init(&nv30->vertprog.data_heap, 0, 256)) {
		nv30_destroy(&nv30->pipe);
		return NULL;
	}

	/* Static rankine initialisation */
	if (!nv30_init_hwctx(nv30, rankine_class)) {
		nv30_destroy(&nv30->pipe);
		return NULL;
	}

	/* Pipe context setup */
	nv30->pipe.winsys = pipe_winsys;
	nv30->pipe.screen = screen;

	nv30->pipe.destroy = nv30_destroy;

	nv30->pipe.draw_arrays = nv30_draw_arrays;
	nv30->pipe.draw_elements = nv30_draw_elements;
	nv30->pipe.clear = nv30_clear;

	nv30->pipe.flush = nv30_flush;

	nv30_init_query_functions(nv30);
	nv30_init_surface_functions(nv30);
	nv30_init_state_functions(nv30);
	nv30_init_miptree_functions(nv30);

	nv30->draw = draw_create();
	assert(nv30->draw);
	draw_set_rasterize_stage(nv30->draw, nv30_draw_render_stage(nv30));

	return &nv30->pipe;
}
	
