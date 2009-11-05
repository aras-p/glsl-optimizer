#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/internal/p_winsys_screen.h"

#include "nv10_context.h"
#include "nv10_screen.h"

static void
nv10_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	draw_flush(nv10->draw);

	FIRE_RING(fence);
}

static void
nv10_destroy(struct pipe_context *pipe)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	if (nv10->draw)
		draw_destroy(nv10->draw);

	FREE(nv10);
}

static void nv10_init_hwctx(struct nv10_context *nv10)
{
	struct nv10_screen *screen = nv10->screen;
	struct nouveau_channel *chan = screen->base.channel;
	int i;
	float projectionmatrix[16];

	BEGIN_RING(celsius, NV10TCL_DMA_NOTIFY, 1);
	OUT_RING  (screen->sync->handle);
	BEGIN_RING(celsius, NV10TCL_DMA_IN_MEMORY0, 2);
	OUT_RING  (chan->vram->handle);
	OUT_RING  (chan->gart->handle);
	BEGIN_RING(celsius, NV10TCL_DMA_IN_MEMORY2, 2);
	OUT_RING  (chan->vram->handle);
	OUT_RING  (chan->vram->handle);

	BEGIN_RING(celsius, NV10TCL_NOP, 1);
	OUT_RING  (0);

	BEGIN_RING(celsius, NV10TCL_RT_HORIZ, 2);
	OUT_RING  (0);
	OUT_RING  (0);

	BEGIN_RING(celsius, NV10TCL_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING  ((0x7ff<<16)|0x800);
	BEGIN_RING(celsius, NV10TCL_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING  ((0x7ff<<16)|0x800);

	for (i=1;i<8;i++) {
		BEGIN_RING(celsius, NV10TCL_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING  (0);
		BEGIN_RING(celsius, NV10TCL_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING  (0);
	}

	BEGIN_RING(celsius, 0x290, 1);
	OUT_RING  ((0x10<<16)|1);
	BEGIN_RING(celsius, 0x3f4, 1);
	OUT_RING  (0);

	BEGIN_RING(celsius, NV10TCL_NOP, 1);
	OUT_RING  (0);

	if (nv10->screen->celsius->grclass != NV10TCL) {
		/* For nv11, nv17 */
		BEGIN_RING(celsius, 0x120, 3);
		OUT_RING  (0);
		OUT_RING  (1);
		OUT_RING  (2);

		BEGIN_RING(celsius, NV10TCL_NOP, 1);
		OUT_RING  (0);
	}

	BEGIN_RING(celsius, NV10TCL_NOP, 1);
	OUT_RING  (0);

	/* Set state */
	BEGIN_RING(celsius, NV10TCL_FOG_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_ALPHA_FUNC_FUNC, 2);
	OUT_RING  (0x207);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_TX_ENABLE(0), 2);
	OUT_RING  (0);
	OUT_RING  (0);

	BEGIN_RING(celsius, NV10TCL_RC_IN_ALPHA(0), 12);
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

	BEGIN_RING(celsius, NV10TCL_BLEND_FUNC_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_DITHER_ENABLE, 2);
	OUT_RING  (1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_VERTEX_WEIGHT_ENABLE, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_BLEND_FUNC_SRC, 4);
	OUT_RING  (1);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0x8006);
	BEGIN_RING(celsius, NV10TCL_STENCIL_MASK, 8);
	OUT_RING  (0xff);
	OUT_RING  (0x207);
	OUT_RING  (0);
	OUT_RING  (0xff);
	OUT_RING  (0x1e00);
	OUT_RING  (0x1e00);
	OUT_RING  (0x1e00);
	OUT_RING  (0x1d01);
	BEGIN_RING(celsius, NV10TCL_NORMALIZE_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_FOG_ENABLE, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_LIGHT_MODEL, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_COLOR_CONTROL, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_ENABLED_LIGHTS, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_DEPTH_FUNC, 1);
	OUT_RING  (0x201);
	BEGIN_RING(celsius, NV10TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_POLYGON_OFFSET_FACTOR, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_POINT_SIZE, 1);
	OUT_RING  (8);
	BEGIN_RING(celsius, NV10TCL_POINT_PARAMETERS_ENABLE, 2);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_LINE_WIDTH, 1);
	OUT_RING  (8);
	BEGIN_RING(celsius, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING  (0x1b02);
	OUT_RING  (0x1b02);
	BEGIN_RING(celsius, NV10TCL_CULL_FACE, 2);
	OUT_RING  (0x405);
	OUT_RING  (0x901);
	BEGIN_RING(celsius, NV10TCL_POLYGON_SMOOTH_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_CULL_FACE_ENABLE, 1);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_TX_GEN_S(0), 8);
	for (i=0;i<8;i++) {
		OUT_RING  (0);
	}
	BEGIN_RING(celsius, NV10TCL_FOG_EQUATION_CONSTANT, 3);
	OUT_RING  (0x3fc00000);	/* -1.50 */
	OUT_RING  (0xbdb8aa0a);	/* -0.09 */
	OUT_RING  (0);		/*  0.00 */

	BEGIN_RING(celsius, NV10TCL_NOP, 1);
	OUT_RING  (0);

	BEGIN_RING(celsius, NV10TCL_FOG_MODE, 2);
	OUT_RING  (0x802);
	OUT_RING  (2);
	/* for some reason VIEW_MATRIX_ENABLE need to be 6 instead of 4 when
	 * using texturing, except when using the texture matrix
	 */
	BEGIN_RING(celsius, NV10TCL_VIEW_MATRIX_ENABLE, 1);
	OUT_RING  (6);
	BEGIN_RING(celsius, NV10TCL_COLOR_MASK, 1);
	OUT_RING  (0x01010101);

	/* Set vertex component */
	BEGIN_RING(celsius, NV10TCL_VERTEX_COL_4F_R, 4);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	OUT_RINGf (1.0);
	BEGIN_RING(celsius, NV10TCL_VERTEX_COL2_3F_R, 3);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RING  (0);
	BEGIN_RING(celsius, NV10TCL_VERTEX_NOR_3F_X, 3);
	OUT_RING  (0);
	OUT_RING  (0);
	OUT_RINGf (1.0);
	BEGIN_RING(celsius, NV10TCL_VERTEX_TX0_4F_S, 4);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	BEGIN_RING(celsius, NV10TCL_VERTEX_TX1_4F_S, 4);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (0.0);
	OUT_RINGf (1.0);
	BEGIN_RING(celsius, NV10TCL_VERTEX_FOG_1F, 1);
	OUT_RINGf (0.0);
	BEGIN_RING(celsius, NV10TCL_EDGEFLAG_ENABLE, 1);
	OUT_RING  (1);

	memset(projectionmatrix, 0, sizeof(projectionmatrix));
	BEGIN_RING(celsius, NV10TCL_PROJECTION_MATRIX(0), 16);
	projectionmatrix[0*4+0] = 1.0;
	projectionmatrix[1*4+1] = 1.0;
	projectionmatrix[2*4+2] = 1.0;
	projectionmatrix[3*4+3] = 1.0;
	for (i=0;i<16;i++) {
		OUT_RINGf  (projectionmatrix[i]);
	}

	BEGIN_RING(celsius, NV10TCL_DEPTH_RANGE_NEAR, 2);
	OUT_RING  (0.0);
	OUT_RINGf  (16777216.0);

	BEGIN_RING(celsius, NV10TCL_VIEWPORT_TRANSLATE_X, 4);
	OUT_RINGf  (-2048.0);
	OUT_RINGf  (-2048.0);
	OUT_RINGf  (16777215.0 * 0.5);
	OUT_RING  (0);

	FIRE_RING (NULL);
}

static void
nv10_set_edgeflags(struct pipe_context *pipe, const unsigned *bitfield)
{
}

struct pipe_context *
nv10_create(struct pipe_screen *pscreen, unsigned pctx_id)
{
	struct nv10_screen *screen = nv10_screen(pscreen);
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv10_context *nv10;
	struct nouveau_winsys *nvws = screen->nvws;

	nv10 = CALLOC(1, sizeof(struct nv10_context));
	if (!nv10)
		return NULL;
	nv10->screen = screen;
	nv10->pctx_id = pctx_id;

	nv10->nvws = nvws;

	nv10->pipe.winsys = ws;
	nv10->pipe.screen = pscreen;
	nv10->pipe.destroy = nv10_destroy;
	nv10->pipe.set_edgeflags = nv10_set_edgeflags;
	nv10->pipe.draw_arrays = nv10_draw_arrays;
	nv10->pipe.draw_elements = nv10_draw_elements;
	nv10->pipe.clear = nv10_clear;
	nv10->pipe.flush = nv10_flush;

	nv10->pipe.is_texture_referenced = nouveau_is_texture_referenced;
	nv10->pipe.is_buffer_referenced = nouveau_is_buffer_referenced;

	nv10_init_surface_functions(nv10);
	nv10_init_state_functions(nv10);

	nv10->draw = draw_create();
	assert(nv10->draw);
	draw_set_rasterize_stage(nv10->draw, nv10_draw_vbuf_stage(nv10));

	nv10_init_hwctx(nv10);

	return &nv10->pipe;
}

