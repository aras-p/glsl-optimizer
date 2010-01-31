#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/internal/p_winsys_screen.h"

#include "nv20_context.h"
#include "nv20_screen.h"

static void
nv20_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv20_context *nv20 = nv20_context(pipe);
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;

	draw_flush(nv20->draw);

	FIRE_RING(chan);
	if (fence)
		*fence = NULL;
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
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;
	int i;
	float projectionmatrix[16];
	const boolean is_nv25tcl = (kelvin->grclass == NV25TCL);

	BEGIN_RING(chan, kelvin, NV20TCL_DMA_NOTIFY, 1);
	OUT_RING  (chan, screen->sync->handle);
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_TEXTURE0, 2);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->gart->handle); /* TEXTURE1 */
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_COLOR, 2);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle); /* ZETA */

	BEGIN_RING(chan, kelvin, NV20TCL_DMA_QUERY, 1);
	OUT_RING  (chan, 0); /* renouveau: beef0351, unique */

	BEGIN_RING(chan, kelvin, NV20TCL_RT_HORIZ, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING  (chan, (0xfff << 16) | 0x0);
	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING  (chan, (0xfff << 16) | 0x0);

	for (i = 1; i < NV20TCL_VIEWPORT_CLIP_HORIZ__SIZE; i++) {
		BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING  (chan, 0);
		BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_MODE, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, 0x17e0, 3);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 1.0);

	if (is_nv25tcl) {
		BEGIN_RING(chan, kelvin, NV20TCL_TX_RCOMP, 1);
		OUT_RING  (chan, NV20TCL_TX_RCOMP_LEQUAL | 0xdb0);
	} else {
		BEGIN_RING(chan, kelvin, 0x1e68, 1);
		OUT_RING  (chan, 0x4b800000); /* 16777216.000000 */
		BEGIN_RING(chan, kelvin, NV20TCL_TX_RCOMP, 1);
		OUT_RING  (chan, NV20TCL_TX_RCOMP_LEQUAL);
	}

	BEGIN_RING(chan, kelvin, 0x290, 1);
	OUT_RING  (chan, (0x10 << 16) | 1);
	BEGIN_RING(chan, kelvin, 0x9fc, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, 0x1d80, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, kelvin, 0x9f8, 1);
	OUT_RING  (chan, 4);
	BEGIN_RING(chan, kelvin, 0x17ec, 3);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 1.0);
	OUT_RINGf (chan, 0.0);

	if (is_nv25tcl) {
		BEGIN_RING(chan, kelvin, 0x1d88, 1);
		OUT_RING  (chan, 3);

		BEGIN_RING(chan, kelvin, NV25TCL_DMA_IN_MEMORY9, 1);
		OUT_RING  (chan, chan->vram->handle);
		BEGIN_RING(chan, kelvin, NV25TCL_DMA_IN_MEMORY8, 1);
		OUT_RING  (chan, chan->vram->handle);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_FENCE, 1);
	OUT_RING  (chan, 0);	/* renouveau: beef1e10 */

	BEGIN_RING(chan, kelvin, 0x1e98, 1);
	OUT_RING  (chan, 0);
#if 0
	if (is_nv25tcl) {
		BEGIN_RING(chan, NvSub3D, NV25TCL_DMA_IN_MEMORY4, 2);
		OUT_RING  (chan, NvDmaTT);	/* renouveau: beef0202 */
		OUT_RING  (chan, NvDmaFB);	/* renouveau: beef0201 */

		BEGIN_RING(chan, NvSub3D, NV20TCL_DMA_TEXTURE1, 1);
		OUT_RING  (chan, NvDmaTT);	/* renouveau: beef0202 */
	}
#endif
	BEGIN_RING(chan, kelvin, NV20TCL_NOTIFY, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, 0x120, 3);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 2);

/* error: ILLEGAL_MTHD, PROTECTION_FAULT
	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_TRANSLATE_X, 4);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 512.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
*/

	if (is_nv25tcl) {
		BEGIN_RING(chan, kelvin, 0x022c, 2);
		OUT_RING  (chan, 0x280);
		OUT_RING  (chan, 0x07d28000);
	}

/* * illegal method, protection fault
	BEGIN_RING(chan, NvSub3D, 0x1c2c, 1);
	OUT_RING  (chan, 0); */

	if (is_nv25tcl) {
		BEGIN_RING(chan, kelvin, 0x1da4, 1);
		OUT_RING  (chan, 0);
	}

/* * crashes with illegal method, protection fault
	BEGIN_RING(chan, NvSub3D, 0x1c18, 1);
	OUT_RING  (chan, 0x200); */

	BEGIN_RING(chan, kelvin, NV20TCL_RT_HORIZ, 2);
	OUT_RING  (chan, (0 << 16) | 0);
	OUT_RING  (chan, (0 << 16) | 0);

	/* *** Set state *** */

	BEGIN_RING(chan, kelvin, NV20TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_ALPHA_FUNC_FUNC, 2);
	OUT_RING  (chan, NV20TCL_ALPHA_FUNC_FUNC_ALWAYS);
	OUT_RING  (chan, 0);			/* NV20TCL_ALPHA_FUNC_REF */

	for (i = 0; i < NV20TCL_TX_ENABLE__SIZE; ++i) {
		BEGIN_RING(chan, kelvin, NV20TCL_TX_ENABLE(i), 1);
		OUT_RING  (chan, 0);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_TX_SHADER_OP, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_TX_SHADER_CULL_MODE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_IN_ALPHA(0), 4);
	OUT_RING  (chan, 0x30d410d0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_OUT_RGB(0), 4);
	OUT_RING  (chan, 0x00000c00);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_ENABLE, 1);
	OUT_RING  (chan, 0x00011101);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_FINAL0, 2);
	OUT_RING  (chan, 0x130e0300);
	OUT_RING  (chan, 0x0c091c80);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_OUT_ALPHA(0), 4);
	OUT_RING  (chan, 0x00000c00);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_IN_RGB(0), 4);
	OUT_RING  (chan, 0x20c400c0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_COLOR0, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_CONSTANT_COLOR0(0), 4);
	OUT_RING  (chan, 0x035125a0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0x40002000);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_MULTISAMPLE_CONTROL, 1);
	OUT_RING  (chan, 0xffff0000);

	BEGIN_RING(chan, kelvin, NV20TCL_BLEND_FUNC_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_DITHER_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_STENCIL_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_BLEND_FUNC_SRC, 4);
	OUT_RING  (chan, NV20TCL_BLEND_FUNC_SRC_ONE);
	OUT_RING  (chan, NV20TCL_BLEND_FUNC_DST_ZERO);
	OUT_RING  (chan, 0);			/* NV20TCL_BLEND_COLOR */
	OUT_RING  (chan, NV20TCL_BLEND_EQUATION_FUNC_ADD);
	BEGIN_RING(chan, kelvin, NV20TCL_STENCIL_MASK, 7);
	OUT_RING  (chan, 0xff);
	OUT_RING  (chan, NV20TCL_STENCIL_FUNC_FUNC_ALWAYS);
	OUT_RING  (chan, 0);			/* NV20TCL_STENCIL_FUNC_REF */
	OUT_RING  (chan, 0xff);		/* NV20TCL_STENCIL_FUNC_MASK */
	OUT_RING  (chan, NV20TCL_STENCIL_OP_FAIL_KEEP);
	OUT_RING  (chan, NV20TCL_STENCIL_OP_ZFAIL_KEEP);
	OUT_RING  (chan, NV20TCL_STENCIL_OP_ZPASS_KEEP);

	BEGIN_RING(chan, kelvin, NV20TCL_COLOR_LOGIC_OP_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, NV20TCL_COLOR_LOGIC_OP_OP_COPY);
	BEGIN_RING(chan, kelvin, 0x17cc, 1);
	OUT_RING  (chan, 0);
	if (is_nv25tcl) {
		BEGIN_RING(chan, kelvin, 0x1d84, 1);
		OUT_RING  (chan, 1);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_LIGHTING_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_MODEL, 1);
	OUT_RING  (chan, 0x00020000);
	BEGIN_RING(chan, kelvin, NV20TCL_SEPARATE_SPECULAR_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_MODEL_TWO_SIDE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_ENABLED_LIGHTS, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_NORMALIZE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_STIPPLE_PATTERN(0),
					NV20TCL_POLYGON_STIPPLE_PATTERN__SIZE);
	for (i = 0; i < NV20TCL_POLYGON_STIPPLE_PATTERN__SIZE; ++i) {
		OUT_RING(chan, 0xffffffff);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);		/* NV20TCL.POLYGON_OFFSET_LINE_ENABLE */
	OUT_RING  (chan, 0);		/* NV20TCL.POLYGON_OFFSET_FILL_ENABLE */
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_FUNC, 1);
	OUT_RING  (chan, NV20TCL_DEPTH_FUNC_LESS);
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_OFFSET_FACTOR, 2);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);	/* NV20TCL.POLYGON_OFFSET_UNITS */
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_UNK17D8, 1);
	OUT_RING  (chan, 1);
	if (!is_nv25tcl) {
		BEGIN_RING(chan, kelvin, 0x1d84, 1);
		OUT_RING  (chan, 3);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_POINT_SIZE, 1);
	if (!is_nv25tcl) {
		OUT_RING  (chan, 8);
	} else {
		OUT_RINGf (chan, 1.0);
	}
	if (!is_nv25tcl) {
		BEGIN_RING(chan, kelvin, NV20TCL_POINT_PARAMETERS_ENABLE, 2);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, 0);		/* NV20TCL.POINT_SMOOTH_ENABLE */
	} else {
		BEGIN_RING(chan, kelvin, NV20TCL_POINT_PARAMETERS_ENABLE, 1);
		OUT_RING  (chan, 0);
		BEGIN_RING(chan, kelvin, 0x0a1c, 1);
		OUT_RING  (chan, 0x800);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_LINE_WIDTH, 1);
	OUT_RING  (chan, 8);
	BEGIN_RING(chan, kelvin, NV20TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING  (chan, NV20TCL_POLYGON_MODE_FRONT_FILL);
	OUT_RING  (chan, NV20TCL_POLYGON_MODE_BACK_FILL);
	BEGIN_RING(chan, kelvin, NV20TCL_CULL_FACE, 2);
	OUT_RING  (chan, NV20TCL_CULL_FACE_BACK);
	OUT_RING  (chan, NV20TCL_FRONT_FACE_CCW);
	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_SMOOTH_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_CULL_FACE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_SHADE_MODEL, 1);
	OUT_RING  (chan, NV20TCL_SHADE_MODEL_SMOOTH);
	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_STIPPLE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_TX_GEN_S(0), 4 * NV20TCL_TX_GEN_S__SIZE);
	for (i=0; i < 4 * NV20TCL_TX_GEN_S__SIZE; ++i) {
		OUT_RING(chan, 0);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_FOG_EQUATION_CONSTANT, 3);
	OUT_RINGf (chan, 1.5);
	OUT_RINGf (chan, -0.090168);		/* NV20TCL.FOG_EQUATION_LINEAR */
	OUT_RINGf (chan, 0.0);		/* NV20TCL.FOG_EQUATION_QUADRATIC */
	BEGIN_RING(chan, kelvin, NV20TCL_FOG_MODE, 2);
	OUT_RING  (chan, NV20TCL_FOG_MODE_EXP_SIGNED);
	OUT_RING  (chan, NV20TCL_FOG_COORD_FOG);
	BEGIN_RING(chan, kelvin, NV20TCL_FOG_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);			/* NV20TCL.FOG_COLOR */
	BEGIN_RING(chan, kelvin, NV20TCL_ENGINE, 1);
	OUT_RING  (chan, NV20TCL_ENGINE_FIXED);

	for (i = 0; i < NV20TCL_TX_MATRIX_ENABLE__SIZE; ++i) {
		BEGIN_RING(chan, kelvin, NV20TCL_TX_MATRIX_ENABLE(i), 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_VTX_ATTR_4F_X(1), 4 * 15);
	OUT_RINGf(chan, 1.0); OUT_RINGf(chan, 0.0); OUT_RINGf(chan, 0.0); OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 0.0); OUT_RINGf(chan, 0.0); OUT_RINGf(chan, 1.0); OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 1.0); OUT_RINGf(chan, 1.0); OUT_RINGf(chan, 1.0); OUT_RINGf(chan, 1.0);
	for (i = 4; i < 16; ++i) {
		OUT_RINGf(chan, 0.0);
		OUT_RINGf(chan, 0.0);
		OUT_RINGf(chan, 0.0);
		OUT_RINGf(chan, 1.0);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_EDGEFLAG_ENABLE, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, kelvin, NV20TCL_COLOR_MASK, 1);
	OUT_RING (chan, 0x00010101);
	BEGIN_RING(chan, kelvin, NV20TCL_CLEAR_VALUE, 1);
	OUT_RING (chan, 0);

	memset(projectionmatrix, 0, sizeof(projectionmatrix));
	projectionmatrix[0*4+0] = 1.0;
	projectionmatrix[1*4+1] = 1.0;
	projectionmatrix[2*4+2] = 16777215.0;
	projectionmatrix[3*4+3] = 1.0;
	BEGIN_RING(chan, kelvin, NV20TCL_PROJECTION_MATRIX(0), 16);
	for (i = 0; i < 16; i++) {
		OUT_RINGf  (chan, projectionmatrix[i]);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_RANGE_NEAR, 2);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 16777216.0); /* [0, 1] scaled approx to [0, 2^24] */

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_TRANSLATE_X, 4);
	OUT_RINGf (chan, 0.0); /* x-offset, w/2 + 1.031250 */
	OUT_RINGf (chan, 0.0); /* y-offset, h/2 + 0.030762 */
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 16777215.0);

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_SCALE_X, 4);
	OUT_RINGf (chan, 0.0); /* no effect?, w/2 */
	OUT_RINGf (chan, 0.0); /* no effect?, h/2 */
	OUT_RINGf (chan, 16777215.0 * 0.5);
	OUT_RINGf (chan, 65535.0);

	FIRE_RING (chan);
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
	nv20->pipe.draw_arrays = nv20_draw_arrays;
	nv20->pipe.draw_elements = nv20_draw_elements;
	nv20->pipe.clear = nv20_clear;
	nv20->pipe.flush = nv20_flush;

	nv20->pipe.is_texture_referenced = nouveau_is_texture_referenced;
	nv20->pipe.is_buffer_referenced = nouveau_is_buffer_referenced;

	nv20_init_surface_functions(nv20);
	nv20_init_state_functions(nv20);

	nv20->draw = draw_create();
	assert(nv20->draw);
	draw_set_rasterize_stage(nv20->draw, nv20_draw_vbuf_stage(nv20));

	nv20_init_hwctx(nv20);

	return &nv20->pipe;
}

