/*
 * Copyright (C) 2009 Francisco Jerez.
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
#include "nouveau_screen.h"
#include "nouveau_class.h"
#include "nv04_driver.h"
#include "nv10_driver.h"

static const struct nouveau_driver nv10_driver;

static void
nv10_hwctx_init(struct nouveau_screen *screen)
{
	struct nouveau_channel *chan = screen->chan;
	struct nouveau_grobj *celsius = screen->eng3d;
	const unsigned chipset = screen->device->chipset;
	int i;

	BEGIN_RING(chan, celsius, NV10TCL_DMA_NOTIFY, 1);
	OUT_RING(chan, screen->ntfy->handle);

	BEGIN_RING(chan, celsius, NV10TCL_DMA_IN_MEMORY0, 3);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);
	OUT_RING(chan, chan->gart->handle);
	BEGIN_RING(chan, celsius, NV10TCL_DMA_IN_MEMORY2, 2);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->vram->handle);

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_RT_HORIZ, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING(chan, 0x7ff << 16 | 0x800);
	BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING(chan, 0x7ff << 16 | 0x800);

	for (i = 1; i < 8; i++) {
		BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING(chan, 0);
		BEGIN_RING(chan, celsius, NV10TCL_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING(chan, 0);
	}

	BEGIN_RING(chan, celsius, 0x290, 1);
	OUT_RING(chan, 0x10 << 16 | 1);
	BEGIN_RING(chan, celsius, 0x3f4, 1);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING(chan, 0);

	if (chipset >= 0x17) {
		BEGIN_RING(chan, celsius, NV17TCL_DMA_IN_MEMORY4, 2);
		OUT_RING(chan, chan->vram->handle);
		OUT_RING(chan, chan->vram->handle);

		BEGIN_RING(chan, celsius, 0xd84, 1);
		OUT_RING(chan, 0x3);

		BEGIN_RING(chan, celsius, NV17TCL_COLOR_MASK_ENABLE, 1);
		OUT_RING(chan, 1);
	}

	if (chipset >= 0x11) {
		BEGIN_RING(chan, celsius, 0x120, 3);
		OUT_RING(chan, 0);
		OUT_RING(chan, 1);
		OUT_RING(chan, 2);

		BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
		OUT_RING(chan, 0);
	}

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING(chan, 0);

	/* Set state */
	BEGIN_RING(chan, celsius, NV10TCL_FOG_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_ALPHA_FUNC_FUNC, 2);
	OUT_RING(chan, 0x207);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_TX_ENABLE(0), 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_BLEND_FUNC_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_DITHER_ENABLE, 2);
	OUT_RING(chan, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_WEIGHT_ENABLE, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_BLEND_FUNC_SRC, 4);
	OUT_RING(chan, 1);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0x8006);
	BEGIN_RING(chan, celsius, NV10TCL_STENCIL_MASK, 8);
	OUT_RING(chan, 0xff);
	OUT_RING(chan, 0x207);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0xff);
	OUT_RING(chan, 0x1e00);
	OUT_RING(chan, 0x1e00);
	OUT_RING(chan, 0x1e00);
	OUT_RING(chan, 0x1d01);
	BEGIN_RING(chan, celsius, NV10TCL_NORMALIZE_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_FOG_ENABLE, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_LIGHT_MODEL, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_SEPARATE_SPECULAR_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_ENABLED_LIGHTS, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_FUNC, 1);
	OUT_RING(chan, 0x201);
	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_OFFSET_FACTOR, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POINT_SIZE, 1);
	OUT_RING(chan, 8);
	BEGIN_RING(chan, celsius, NV10TCL_POINT_PARAMETERS_ENABLE, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_LINE_WIDTH, 1);
	OUT_RING(chan, 8);
	BEGIN_RING(chan, celsius, NV10TCL_LINE_SMOOTH_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING(chan, 0x1b02);
	OUT_RING(chan, 0x1b02);
	BEGIN_RING(chan, celsius, NV10TCL_CULL_FACE, 2);
	OUT_RING(chan, 0x405);
	OUT_RING(chan, 0x901);
	BEGIN_RING(chan, celsius, NV10TCL_POLYGON_SMOOTH_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_CULL_FACE_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_TX_GEN_S(0), 8);
	for (i = 0; i < 8; i++)
		OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_TX_MATRIX_ENABLE(0), 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_FOG_EQUATION_CONSTANT, 3);
	OUT_RING(chan, 0x3fc00000);	/* -1.50 */
	OUT_RING(chan, 0xbdb8aa0a);	/* -0.09 */
	OUT_RING(chan, 0);		/*  0.00 */

	BEGIN_RING(chan, celsius, NV10TCL_NOP, 1);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10TCL_FOG_MODE, 2);
	OUT_RING(chan, 0x802);
	OUT_RING(chan, 2);
	/* for some reason VIEW_MATRIX_ENABLE need to be 6 instead of 4 when
	 * using texturing, except when using the texture matrix
	 */
	BEGIN_RING(chan, celsius, NV10TCL_VIEW_MATRIX_ENABLE, 1);
	OUT_RING(chan, 6);
	BEGIN_RING(chan, celsius, NV10TCL_COLOR_MASK, 1);
	OUT_RING(chan, 0x01010101);

	/* Set vertex component */
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_COL_4F_R, 4);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_COL2_3F_R, 3);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_NOR_3F_X, 3);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_TX0_4F_S, 4);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_TX1_4F_S, 4);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_FOG_1F, 1);
	OUT_RINGf(chan, 0.0);
	BEGIN_RING(chan, celsius, NV10TCL_EDGEFLAG_ENABLE, 1);
	OUT_RING(chan, 1);

	BEGIN_RING(chan, celsius, NV10TCL_DEPTH_RANGE_NEAR, 2);
	OUT_RING(chan, 0.0);
	OUT_RINGf(chan, 16777216.0);

	FIRE_RING(chan);
}

GLboolean
nv10_screen_init(struct nouveau_screen *screen)
{
	unsigned chipset = screen->device->chipset;
	unsigned celsius_class;
	int ret;

	screen->driver = &nv10_driver;

	/* 2D engine. */
	ret = nv04_surface_init(screen);
	if (!ret)
		return GL_FALSE;

	/* 3D engine. */
	if (chipset >= 0x17)
		celsius_class = NV17TCL;
	else if (chipset >= 0x11)
		celsius_class = NV11TCL;
	else
		celsius_class = NV10TCL;

	ret = nouveau_grobj_alloc(screen->chan, 0xbeef0001, celsius_class,
				  &screen->eng3d);
	if (ret)
		return GL_FALSE;

	nv10_hwctx_init(screen);

	return GL_TRUE;
}

static void
nv10_screen_destroy(struct nouveau_screen *screen)
{
	if (screen->eng3d)
		nouveau_grobj_free(&screen->eng3d);

	nv04_surface_takedown(screen);
}

static const struct nouveau_driver nv10_driver = {
	.screen_destroy = nv10_screen_destroy,
	.context_create = nv10_context_create,
	.context_destroy = nv10_context_destroy,
	.surface_copy = nv04_surface_copy,
	.surface_fill = nv04_surface_fill,
	.emit = (nouveau_state_func[]) {
		nv10_emit_alpha_func,
		nv10_emit_blend_color,
		nv10_emit_blend_equation,
		nv10_emit_blend_func,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_clip_plane,
		nv10_emit_color_mask,
		nv10_emit_color_material,
		nv10_emit_cull_face,
		nv10_emit_front_face,
		nv10_emit_depth,
		nv10_emit_dither,
		nv10_emit_frag,
		nv10_emit_framebuffer,
		nv10_emit_fog,
		nv10_emit_index_mask,
		nv10_emit_light_enable,
		nv10_emit_light_model,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_light_source,
		nv10_emit_line_stipple,
		nv10_emit_line_mode,
		nv10_emit_logic_opcode,
		nv10_emit_material_ambient,
		nouveau_emit_nothing,
		nv10_emit_material_diffuse,
		nouveau_emit_nothing,
		nv10_emit_material_specular,
		nouveau_emit_nothing,
		nv10_emit_material_shininess,
		nouveau_emit_nothing,
		nv10_emit_modelview,
		nv10_emit_point_mode,
		nv10_emit_point_parameter,
		nv10_emit_polygon_mode,
		nv10_emit_polygon_offset,
		nv10_emit_polygon_stipple,
		nv10_emit_projection,
		nv10_emit_render_mode,
		nv10_emit_scissor,
		nv10_emit_shade_model,
		nv10_emit_stencil_func,
		nv10_emit_stencil_mask,
		nv10_emit_stencil_op,
		nv10_emit_tex_env,
		nv10_emit_tex_env,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_tex_gen,
		nv10_emit_tex_gen,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_tex_obj,
		nv10_emit_tex_obj,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_viewport
	},
	.num_emit = NUM_NOUVEAU_STATE,
};
