/*
 * Copyright (C) 2009-2010 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_class.h"
#include "nv04_driver.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

static const struct dri_extension nv20_extensions[] = {
	{ "GL_EXT_texture_rectangle",	NULL },
	{ NULL,				NULL }
};

static void
nv20_hwctx_init(GLcontext *ctx)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	int i;

	BEGIN_RING(chan, kelvin, NV20TCL_DMA_NOTIFY, 1);
	OUT_RING  (chan, hw->ntfy->handle);
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_TEXTURE0, 2);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->gart->handle);
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_COLOR, 2);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_VTXBUF0, 2);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);

	BEGIN_RING(chan, kelvin, NV20TCL_DMA_QUERY, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, NV20TCL_RT_HORIZ, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING  (chan, 0xfff << 16 | 0x0);
	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING  (chan, 0xfff << 16 | 0x0);

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

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_RING(chan, kelvin, NV20TCL_TX_RCOMP, 1);
		OUT_RING  (chan, NV20TCL_TX_RCOMP_LEQUAL | 0xdb0);
	} else {
		BEGIN_RING(chan, kelvin, 0x1e68, 1);
		OUT_RING  (chan, 0x4b800000); /* 16777216.000000 */
		BEGIN_RING(chan, kelvin, NV20TCL_TX_RCOMP, 1);
		OUT_RING  (chan, NV20TCL_TX_RCOMP_LEQUAL);
	}

	BEGIN_RING(chan, kelvin, 0x290, 1);
	OUT_RING  (chan, 0x10 << 16 | 1);
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

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_RING(chan, kelvin, 0x1d88, 1);
		OUT_RING  (chan, 3);

		BEGIN_RING(chan, kelvin, NV25TCL_DMA_IN_MEMORY9, 1);
		OUT_RING  (chan, chan->vram->handle);
		BEGIN_RING(chan, kelvin, NV25TCL_DMA_IN_MEMORY8, 1);
		OUT_RING  (chan, chan->vram->handle);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_DMA_FENCE, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, 0x1e98, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, NV20TCL_NOTIFY, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, 0x120, 3);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 2);

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_RING(chan, kelvin, 0x022c, 2);
		OUT_RING  (chan, 0x280);
		OUT_RING  (chan, 0x07d28000);

		BEGIN_RING(chan, kelvin, 0x1da4, 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_RT_HORIZ, 2);
	OUT_RING  (chan, 0 << 16 | 0);
	OUT_RING  (chan, 0 << 16 | 0);

	BEGIN_RING(chan, kelvin, NV20TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_ALPHA_FUNC_FUNC, 2);
	OUT_RING  (chan, NV20TCL_ALPHA_FUNC_FUNC_ALWAYS);
	OUT_RING  (chan, 0);

	for (i = 0; i < NV20TCL_TX_ENABLE__SIZE; i++) {
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
	OUT_RING  (chan, 0);
	OUT_RING  (chan, NV20TCL_BLEND_EQUATION_FUNC_ADD);
	BEGIN_RING(chan, kelvin, NV20TCL_STENCIL_MASK, 7);
	OUT_RING  (chan, 0xff);
	OUT_RING  (chan, NV20TCL_STENCIL_FUNC_FUNC_ALWAYS);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0xff);
	OUT_RING  (chan, NV20TCL_STENCIL_OP_FAIL_KEEP);
	OUT_RING  (chan, NV20TCL_STENCIL_OP_ZFAIL_KEEP);
	OUT_RING  (chan, NV20TCL_STENCIL_OP_ZPASS_KEEP);

	BEGIN_RING(chan, kelvin, NV20TCL_COLOR_LOGIC_OP_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, NV20TCL_COLOR_LOGIC_OP_OP_COPY);
	BEGIN_RING(chan, kelvin, 0x17cc, 1);
	OUT_RING  (chan, 0);
	if (context_chipset(ctx) >= 0x25) {
		BEGIN_RING(chan, kelvin, 0x1d84, 1);
		OUT_RING  (chan, 1);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_LIGHTING_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_LIGHT_MODEL, 1);
	OUT_RING  (chan, NV20TCL_LIGHT_MODEL_VIEWER_NONLOCAL);
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
	for (i = 0; i < NV20TCL_POLYGON_STIPPLE_PATTERN__SIZE; i++) {
		OUT_RING(chan, 0xffffffff);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_FUNC, 1);
	OUT_RING  (chan, NV20TCL_DEPTH_FUNC_LESS);
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_OFFSET_FACTOR, 2);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_UNK17D8, 1);
	OUT_RING  (chan, 1);
	if (context_chipset(ctx) < 0x25) {
		BEGIN_RING(chan, kelvin, 0x1d84, 1);
		OUT_RING  (chan, 3);
	}
	BEGIN_RING(chan, kelvin, NV20TCL_POINT_SIZE, 1);
	if (context_chipset(ctx) >= 0x25)
		OUT_RINGf (chan, 1.0);
	else
		OUT_RING  (chan, 8);

	if (context_chipset(ctx) >= 0x25) {
		BEGIN_RING(chan, kelvin, NV20TCL_POINT_PARAMETERS_ENABLE, 1);
		OUT_RING  (chan, 0);
		BEGIN_RING(chan, kelvin, 0x0a1c, 1);
		OUT_RING  (chan, 0x800);
	} else {
		BEGIN_RING(chan, kelvin, NV20TCL_POINT_PARAMETERS_ENABLE, 2);
		OUT_RING  (chan, 0);
		OUT_RING  (chan, 0);
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

	BEGIN_RING(chan, kelvin, NV20TCL_TX_GEN_S(0),
		   4 * NV20TCL_TX_GEN_S__SIZE);
	for (i=0; i < 4 * NV20TCL_TX_GEN_S__SIZE; i++)
		OUT_RING(chan, 0);

	BEGIN_RING(chan, kelvin, NV20TCL_FOG_EQUATION_CONSTANT, 3);
	OUT_RINGf (chan, 1.5);
	OUT_RINGf (chan, -0.090168);
	OUT_RINGf (chan, 0.0);
	BEGIN_RING(chan, kelvin, NV20TCL_FOG_MODE, 2);
	OUT_RING  (chan, NV20TCL_FOG_MODE_EXP_SIGNED);
	OUT_RING  (chan, NV20TCL_FOG_COORD_FOG);
	BEGIN_RING(chan, kelvin, NV20TCL_FOG_ENABLE, 2);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, kelvin, NV20TCL_ENGINE, 1);
	OUT_RING  (chan, NV20TCL_ENGINE_FIXED);

	for (i = 0; i < NV20TCL_TX_MATRIX_ENABLE__SIZE; i++) {
		BEGIN_RING(chan, kelvin, NV20TCL_TX_MATRIX_ENABLE(i), 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_VTX_ATTR_4F_X(1), 4 * 15);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 1.0);
	for (i = 0; i < 12; i++) {
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

	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_RANGE_NEAR, 2);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 16777216.0);

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_TRANSLATE_X, 4);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 16777215.0);

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_SCALE_X, 4);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 0.0);
	OUT_RINGf (chan, 16777215.0 * 0.5);
	OUT_RINGf (chan, 65535.0);

	FIRE_RING(chan);
}

static void
nv20_context_destroy(GLcontext *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nv04_surface_takedown(ctx);
	nv20_render_destroy(ctx);

	nouveau_grobj_free(&nctx->hw.eng3d);

	nouveau_context_deinit(ctx);
	FREE(ctx);
}

static GLcontext *
nv20_context_create(struct nouveau_screen *screen, const GLvisual *visual,
		    GLcontext *share_ctx)
{
	struct nouveau_context *nctx;
	GLcontext *ctx;
	unsigned kelvin_class;
	int ret;

	nctx = CALLOC_STRUCT(nouveau_context);
	if (!nctx)
		return NULL;

	ctx = &nctx->base;

	if (!nouveau_context_init(ctx, screen, visual, share_ctx))
		goto fail;

	driInitExtensions(ctx, nv20_extensions, GL_FALSE);

	/* GL constants. */
	ctx->Const.MaxTextureCoordUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 8;
	ctx->Const.MaxTextureLodBias = 15;

	/* 2D engine. */
	ret = nv04_surface_init(ctx);
	if (!ret)
		goto fail;

	/* 3D engine. */
	if (context_chipset(ctx) >= 0x25)
		kelvin_class = NV25TCL;
	else
		kelvin_class = NV20TCL;

	ret = nouveau_grobj_alloc(context_chan(ctx), 0xbeef0001, kelvin_class,
				  &nctx->hw.eng3d);
	if (ret)
		goto fail;

	nv20_hwctx_init(ctx);
	nv20_render_init(ctx);

	return ctx;

fail:
	nv20_context_destroy(ctx);
	return NULL;
}

const struct nouveau_driver nv20_driver = {
	.context_create = nv20_context_create,
	.context_destroy = nv20_context_destroy,
	.surface_copy = nv04_surface_copy,
	.surface_fill = nv04_surface_fill,
	.emit = (nouveau_state_func[]) {
		nv10_emit_alpha_func,
		nv10_emit_blend_color,
		nv10_emit_blend_equation,
		nv10_emit_blend_func,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv20_emit_clip_plane,
		nv10_emit_color_mask,
		nv20_emit_color_material,
		nv10_emit_cull_face,
		nv10_emit_front_face,
		nv10_emit_depth,
		nv10_emit_dither,
		nv20_emit_frag,
		nv20_emit_framebuffer,
		nv20_emit_fog,
		nv10_emit_light_enable,
		nv20_emit_light_model,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv20_emit_light_source,
		nv10_emit_line_stipple,
		nv10_emit_line_mode,
		nv20_emit_logic_opcode,
		nv20_emit_material_ambient,
		nv20_emit_material_ambient,
		nv20_emit_material_diffuse,
		nv20_emit_material_diffuse,
		nv20_emit_material_specular,
		nv20_emit_material_specular,
		nv20_emit_material_shininess,
		nv20_emit_material_shininess,
		nv20_emit_modelview,
		nv20_emit_point_mode,
		nv10_emit_point_parameter,
		nv10_emit_polygon_mode,
		nv10_emit_polygon_offset,
		nv10_emit_polygon_stipple,
		nv20_emit_projection,
		nv10_emit_render_mode,
		nv10_emit_scissor,
		nv10_emit_shade_model,
		nv10_emit_stencil_func,
		nv10_emit_stencil_mask,
		nv10_emit_stencil_op,
		nv20_emit_tex_env,
		nv20_emit_tex_env,
		nv20_emit_tex_env,
		nv20_emit_tex_env,
		nv10_emit_tex_gen,
		nv10_emit_tex_gen,
		nv10_emit_tex_gen,
		nv10_emit_tex_gen,
		nv20_emit_tex_obj,
		nv20_emit_tex_obj,
		nv20_emit_tex_obj,
		nv20_emit_tex_obj,
		nv20_emit_viewport,
		nv20_emit_tex_shader
	},
	.num_emit = NUM_NV20_STATE,
};
