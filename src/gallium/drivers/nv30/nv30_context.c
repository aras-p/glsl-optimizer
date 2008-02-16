#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "nv30_context.h"

static const char *
nv30_get_name(struct pipe_context *pipe)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", nv30->chipset);
	return buffer;
}

static const char *
nv30_get_vendor(struct pipe_context *pipe)
{
	return "nouveau";
}

static int
nv30_get_param(struct pipe_context *pipe, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 16;
	case PIPE_CAP_NPOT_TEXTURES:
		return 0;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 1;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_S3TC:
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 1;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 2;
	case PIPE_CAP_OCCLUSION_QUERY:
		return 1;
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		return 1;
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
		return 13;
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		return 10;
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 13;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static float
nv30_get_paramf(struct pipe_context *pipe, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_LINE_WIDTH:
	case PIPE_CAP_MAX_LINE_WIDTH_AA:
		return 10.0;
	case PIPE_CAP_MAX_POINT_WIDTH:
	case PIPE_CAP_MAX_POINT_WIDTH_AA:
		return 64.0;
	case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
		return 16.0;
	case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
		return 4.0;
	case PIPE_CAP_BITMAP_TEXCOORD_BIAS:
		return 0.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static void
nv30_flush(struct pipe_context *pipe, unsigned flags)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nouveau_winsys *nvws = nv30->nvws;
	
	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(rankine, 0x1fd8, 1);
		OUT_RING  (2);
		BEGIN_RING(rankine, 0x1fd8, 1);
		OUT_RING  (1);
	}

	if (flags & PIPE_FLUSH_WAIT) {
		nvws->notifier_reset(nv30->sync, 0);
		BEGIN_RING(rankine, 0x104, 1);
		OUT_RING  (0);
		BEGIN_RING(rankine, 0x100, 1);
		OUT_RING  (0);
	}

	FIRE_RING();

	if (flags & PIPE_FLUSH_WAIT)
		nvws->notifier_wait(nv30->sync, 0, 0, 2000);
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

	/* Attempt to setup a known state.. Probably missing a heap of
	 * stuff here..
	 */
	BEGIN_RING(rankine, NV34TCL_STENCIL_FRONT_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(rankine, NV34TCL_STENCIL_BACK_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(rankine, NV34TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(rankine, NV34TCL_DEPTH_WRITE_ENABLE, 2);
	OUT_RING  (0); /* wr disable */
	OUT_RING  (0); /* test disable */
	BEGIN_RING(rankine, NV34TCL_COLOR_MASK, 1);
	OUT_RING  (0x01010101); /* TR,TR,TR,TR */
	BEGIN_RING(rankine, NV34TCL_CULL_FACE_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(rankine, NV34TCL_BLEND_FUNC_ENABLE, 5);
	OUT_RING  (0);				/* Blend enable */
	OUT_RING  (0);				/* Blend src */
	OUT_RING  (0);				/* Blend dst */
	OUT_RING  (0x00000000);			/* Blend colour */
	OUT_RING  (0x8006);			/* FUNC_ADD */
	BEGIN_RING(rankine, NV34TCL_COLOR_LOGIC_OP_ENABLE, 2);
	OUT_RING  (0);
	OUT_RING  (0x1503 /*GL_COPY*/);
	BEGIN_RING(rankine, NV34TCL_DITHER_ENABLE, 1);
	OUT_RING  (1);
	BEGIN_RING(rankine, NV34TCL_SHADE_MODEL, 1);
	OUT_RING  (0x1d01 /*GL_SMOOTH*/);
	BEGIN_RING(rankine, NV34TCL_POLYGON_OFFSET_FACTOR,2);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	BEGIN_RING(rankine, NV34TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING  (0x1b02 /*GL_FILL*/);
	OUT_RING  (0x1b02 /*GL_FILL*/);
	/* - Disable texture units
	 * - Set fragprog to MOVR result.color, fragment.color */
	for (i=0;i<16;i++) {
		BEGIN_RING(rankine,
				NV34TCL_TX_ENABLE(i), 1);
		OUT_RING  (0);
	}
	/* Polygon stipple */
	BEGIN_RING(rankine,
			NV34TCL_POLYGON_STIPPLE_PATTERN(0), 0x20);
	for (i=0;i<0x20;i++)
		OUT_RING  (0xFFFFFFFF);

	int w=4096;
	int h=4096;
	int pitch=4096*4;
	BEGIN_RING(rankine, NV34TCL_RT_HORIZ, 5);
	OUT_RING  (w<<16);
	OUT_RING  (h<<16);
	OUT_RING  (0x148); /* format */
	OUT_RING  (pitch << 16 | pitch);
	OUT_RING  (0x0);
        BEGIN_RING(rankine, 0x0a00, 2);
        OUT_RING  ((w<<16) | 0);
        OUT_RING  ((h<<16) | 0);
	BEGIN_RING(rankine, NV34TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	OUT_RING  ((w-1)<<16);
	OUT_RING  ((h-1)<<16);
	BEGIN_RING(rankine, NV34TCL_SCISSOR_HORIZ, 2);
	OUT_RING  (w<<16);
	OUT_RING  (h<<16);
	BEGIN_RING(rankine, NV34TCL_VIEWPORT_HORIZ, 2);
	OUT_RING  (w<<16);
	OUT_RING  (h<<16);

	BEGIN_RING(rankine, NV34TCL_VIEWPORT_TRANSLATE_X, 8);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	OUT_RINGf (0.0);

	BEGIN_RING(rankine, NV34TCL_MODELVIEW_MATRIX(0), 16);
	OUT_RINGf (1.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);

	BEGIN_RING(rankine, NV34TCL_PROJECTION_MATRIX(0), 16);
	OUT_RINGf (1.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);

	BEGIN_RING(rankine, NV34TCL_SCISSOR_HORIZ, 2);
	OUT_RING  (4096<<16);
	OUT_RING  (4096<<16);

	BEGIN_RING(rankine, NV34TCL_MULTISAMPLE_CONTROL, 1);
	OUT_RING  (0xffff0000);

	FIRE_RING ();
	return TRUE;
}

#define NV30TCL_CHIPSET_3X_MASK 0x00000003
#define NV34TCL_CHIPSET_3X_MASK 0x00000010
#define NV35TCL_CHIPSET_3X_MASK 0x000001e0

struct pipe_context *
nv30_create(struct pipe_winsys *pipe_winsys, struct nouveau_winsys *nvws,
	    unsigned chipset)
{
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
	if (nvws->res_init(&nv30->vertprog.exec_heap, 0, 512) ||
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

	nv30->pipe.destroy = nv30_destroy;
	nv30->pipe.get_name = nv30_get_name;
	nv30->pipe.get_vendor = nv30_get_vendor;
	nv30->pipe.get_param = nv30_get_param;
	nv30->pipe.get_paramf = nv30_get_paramf;

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
	
