#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"

#include "nv20_context.h"
#include "nv20_screen.h"

static void
nv20_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv20_context *nv20 = nv20_context(pipe);

	draw_flush(nv20->draw);

	FIRE_RING(fence);
}

static void
nv20_destroy(struct pipe_context *pipe)
{
	struct nv20_context *nv20 = nv20_context(pipe);

	if (nv20->draw)
		draw_destroy(nv20->draw);

	FREE(nv20);
}

static void nv20_init_hwctx(struct nv20_context *nv20)
{
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_winsys *nvws = screen->nvws;
	int i;
	float projectionmatrix[16];

	BEGIN_RING(kelvin, NV10TCL_DMA_NOTIFY, 1);
	OUT_RING  (screen->sync->handle);
	BEGIN_RING(kelvin, NV10TCL_DMA_IN_MEMORY0, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->gart->handle);
	BEGIN_RING(kelvin, NV10TCL_DMA_IN_MEMORY2, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->vram->handle);

	BEGIN_RING(kelvin, NV10TCL_NOP, 1);
	OUT_RING  (0);

	BEGIN_RING(kelvin, NV10TCL_RT_HORIZ, 2);
	OUT_RING  (0);
	OUT_RING  (0);

	BEGIN_RING(kelvin, NV10TCL_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING  ((0x7ff<<16)|0x800);
	BEGIN_RING(kelvin, NV10TCL_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING  ((0x7ff<<16)|0x800);

	for (i=1;i<8;i++) {
		BEGIN_RING(kelvin, NV10TCL_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING  (0);
		BEGIN_RING(kelvin, NV10TCL_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING  (0);
	}

	BEGIN_RING(kelvin, 0x290, 1);
	OUT_RING  ((0x10<<16)|1);
	BEGIN_RING(kelvin, 0x3f4, 1);
	OUT_RING  (0);

	BEGIN_RING(kelvin, NV10TCL_NOP, 1);
	OUT_RING  (0);

	if (nv20->screen->kelvin->grclass != NV10TCL) {
		/* For nv11, nv17 */
		BEGIN_RING(kelvin, 0x120, 3);
		OUT_RING  (0);
		OUT_RING  (1);
		OUT_RING  (2);

		BEGIN_RING(kelvin, NV10TCL_NOP, 1);
		OUT_RING  (0);
	}

	BEGIN_RING(kelvin, NV10TCL_NOP, 1);
	OUT_RING  (0);

	/* Set state */
	BEGIN_RING(kelvin, NV10TCL_FOG_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_ALPHA_FUNC_FUNC, 2);
	OUT_RING  (0x207);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_TX_ENABLE(0), 2);
	OUT_RING  (0);
	OUT_RING  (0);

	BEGIN_RING(kelvin, NV10TCL_RC_IN_ALPHA(0), 12);
	OUT_RING  (0x30141010);
	OUT_RING  (0);
	OUT_RING  (0x20040000);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0x00000c00);
	OUT_RING  (0);
	OUT_RING  (0x00000c00);
	OUT_RING  (0x18000000);
	OUT_RING  (0x300e0300);
	OUT_RING  (0x0c091c80);

	BEGIN_RING(kelvin, NV10TCL_BLEND_FUNC_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_DITHER_ENABLE, 2);
	OUT_RING  (1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_VERTEX_WEIGHT_ENABLE, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_BLEND_FUNC_SRC, 4);
	OUT_RING  (1);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0x8006);
	BEGIN_RING(kelvin, NV10TCL_STENCIL_MASK, 8);
	OUT_RING  (0xff);
	OUT_RING  (0x207);
	OUT_RING  (0);
	OUT_RING  (0xff);
	OUT_RING  (0x1e00);
	OUT_RING  (0x1e00);
	OUT_RING  (0x1e00);
	OUT_RING  (0x1d01);
	BEGIN_RING(kelvin, NV10TCL_NORMALIZE_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_FOG_ENABLE, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_LIGHT_MODEL, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_COLOR_CONTROL, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_ENABLED_LIGHTS, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_DEPTH_FUNC, 1);
	OUT_RING  (0x201);
	BEGIN_RING(kelvin, NV10TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_POLYGON_OFFSET_FACTOR, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_POINT_SIZE, 1);
	OUT_RING  (8);
	BEGIN_RING(kelvin, NV10TCL_POINT_PARAMETERS_ENABLE, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_LINE_WIDTH, 1);
	OUT_RING  (8);
	BEGIN_RING(kelvin, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING  (0x1b02);
	OUT_RING  (0x1b02);
	BEGIN_RING(kelvin, NV10TCL_CULL_FACE, 2);
	OUT_RING  (0x405);
	OUT_RING  (0x901);
	BEGIN_RING(kelvin, NV10TCL_POLYGON_SMOOTH_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_CULL_FACE_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_CLIP_PLANE_ENABLE(0), 8);
	for (i=0;i<8;i++) {
		OUT_RING  (0);
	}
	BEGIN_RING(kelvin, NV10TCL_FOG_EQUATION_CONSTANT, 3);
	OUT_RING  (0x3fc00000);	/* -1.50 */
	OUT_RING  (0xbdb8aa0a);	/* -0.09 */
	OUT_RING  (0);		/*  0.00 */

	BEGIN_RING(kelvin, NV10TCL_NOP, 1);
	OUT_RING  (0);

	BEGIN_RING(kelvin, NV10TCL_FOG_MODE, 2);
	OUT_RING  (0x802);
	OUT_RING  (2);
	/* for some reason VIEW_MATRIX_ENABLE need to be 6 instead of 4 when
	 * using texturing, except when using the texture matrix
	 */
	BEGIN_RING(kelvin, NV10TCL_VIEW_MATRIX_ENABLE, 1);
	OUT_RING  (6);
	BEGIN_RING(kelvin, NV10TCL_COLOR_MASK, 1);
	OUT_RING  (0x01010101);

	/* Set vertex component */
	BEGIN_RING(kelvin, NV10TCL_VERTEX_COL_4F_R, 4);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	BEGIN_RING(kelvin, NV10TCL_VERTEX_COL2_3F_R, 3);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(kelvin, NV10TCL_VERTEX_NOR_3F_X, 3);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RINGf (1.0);
	BEGIN_RING(kelvin, NV10TCL_VERTEX_TX0_4F_S, 4);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	BEGIN_RING(kelvin, NV10TCL_VERTEX_TX1_4F_S, 4);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	BEGIN_RING(kelvin, NV10TCL_VERTEX_FOG_1F, 1);
	OUT_RINGf (0.0);
	BEGIN_RING(kelvin, NV10TCL_EDGEFLAG_ENABLE, 1);
	OUT_RING  (1);

	memset(projectionmatrix, 0, sizeof(projectionmatrix));
	BEGIN_RING(kelvin, NV10TCL_PROJECTION_MATRIX(0), 16);
	projectionmatrix[0*4+0] = 1.0;
	projectionmatrix[1*4+1] = 1.0;
	projectionmatrix[2*4+2] = 1.0;
	projectionmatrix[3*4+3] = 1.0;
	for (i=0;i<16;i++) {
		OUT_RINGf  (projectionmatrix[i]);
	}

	BEGIN_RING(kelvin, NV10TCL_DEPTH_RANGE_NEAR, 2);
	OUT_RING  (0.0);
	OUT_RINGf  (16777216.0);

	BEGIN_RING(kelvin, NV10TCL_VIEWPORT_SCALE_X, 4);
	OUT_RINGf  (-2048.0);
	OUT_RINGf  (-2048.0);
	OUT_RINGf  (16777215.0 * 0.5);
	OUT_RING  (0);

	FIRE_RING (NULL);
}

static void
nv20_set_edgeflags(struct pipe_context *pipe, const unsigned *bitfield)
{
}

struct pipe_context *
nv20_create(struct pipe_screen *pscreen, unsigned pctx_id)
{
	struct nv20_screen *screen = nv20_screen(pscreen);
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv20_context *nv20;
	struct nouveau_winsys *nvws = screen->nvws;

	nv20 = CALLOC(1, sizeof(struct nv20_context));
	if (!nv20)
		return NULL;
	nv20->screen = screen;
	nv20->pctx_id = pctx_id;

	nv20->nvws = nvws;

	nv20->pipe.winsys = ws;
	nv20->pipe.screen = pscreen;
	nv20->pipe.destroy = nv20_destroy;
	nv20->pipe.set_edgeflags = nv20_set_edgeflags;
	nv20->pipe.draw_arrays = nv20_draw_arrays;
	nv20->pipe.draw_elements = nv20_draw_elements;
	nv20->pipe.clear = nv20_clear;
	nv20->pipe.flush = nv20_flush;

	nv20_init_surface_functions(nv20);
	nv20_init_state_functions(nv20);

	nv20->draw = draw_create();
	assert(nv20->draw);
	draw_set_rasterize_stage(nv20->draw, nv20_draw_vbuf_stage(nv20));

	nv20_init_hwctx(nv20);

	return &nv20->pipe;
}

