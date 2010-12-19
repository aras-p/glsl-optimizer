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

#include "main/state.h"
#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_fbo.h"
#include "nouveau_util.h"
#include "nv_object.xml.h"
#include "nv10_3d.xml.h"
#include "nv04_driver.h"
#include "nv10_driver.h"

static const struct dri_extension nv10_extensions[] = {
	{ "GL_ARB_texture_env_crossbar", NULL },
	{ "GL_EXT_texture_rectangle",	NULL },
	{ "GL_ARB_texture_env_combine", NULL },
	{ "GL_ARB_texture_env_dot3",    NULL },
	{ NULL,				NULL }
};

static GLboolean
use_fast_zclear(struct gl_context *ctx, GLbitfield buffers)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	if (buffers & BUFFER_BIT_STENCIL) {
		/*
		 * The stencil test is bypassed when fast Z clears are
		 * enabled.
		 */
		nctx->hierz.clear_blocked = GL_TRUE;
		context_dirty(ctx, ZCLEAR);
		return GL_FALSE;
	}

	return !nctx->hierz.clear_blocked &&
		fb->_Xmax == fb->Width && fb->_Xmin == 0 &&
		fb->_Ymax == fb->Height && fb->_Ymin == 0;
}

GLboolean
nv10_use_viewport_zclear(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;

	return context_chipset(ctx) < 0x17 &&
		!nctx->hierz.clear_blocked && fb->_DepthBuffer &&
		(_mesa_get_format_bits(fb->_DepthBuffer->Format,
				       GL_DEPTH_BITS) >= 24);
}

float
nv10_transform_depth(struct gl_context *ctx, float z)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	if (nv10_use_viewport_zclear(ctx))
		return 2097152.0 * (z + (nctx->hierz.clear_seq & 7));
	else
		return ctx->DrawBuffer->_DepthMaxF * z;
}

static void
nv10_zclear(struct gl_context *ctx, GLbitfield *buffers)
{
	/*
	 * Pre-nv17 cards don't have native support for fast Z clears,
	 * but in some cases we can still "clear" the Z buffer without
	 * actually blitting to it if we're willing to sacrifice a few
	 * bits of depth precision.
	 *
	 * Each time a clear is requested we modify the viewport
	 * transform in such a way that the old contents of the depth
	 * buffer are clamped to the requested clear value when
	 * they're read by the GPU.
	 */
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(fb);
	struct nouveau_surface *s = &to_nouveau_renderbuffer(
		fb->_DepthBuffer->Wrapped)->surface;

	if (nv10_use_viewport_zclear(ctx)) {
		int x, y, w, h;
		float z = ctx->Depth.Clear;
		uint32_t value = pack_zs_f(s->format, z, 0);

		get_scissors(fb, &x, &y, &w, &h);
		*buffers &= ~BUFFER_BIT_DEPTH;

		if (use_fast_zclear(ctx, *buffers)) {
			if (nfb->hierz.clear_value != value) {
				/* Don't fast clear if we're changing
				 * the depth value. */
				nfb->hierz.clear_value = value;

			} else if (z == 0.0) {
				nctx->hierz.clear_seq++;
				context_dirty(ctx, ZCLEAR);

				if ((nctx->hierz.clear_seq & 7) != 0 &&
				    nctx->hierz.clear_seq != 1)
					/* We didn't wrap around -- no need to
					 * clear the depth buffer for real. */
					return;

			} else if (z == 1.0) {
				nctx->hierz.clear_seq--;
				context_dirty(ctx, ZCLEAR);

				if ((nctx->hierz.clear_seq & 7) != 7)
					/* No wrap around */
					return;
			}
		}

		value = pack_zs_f(s->format,
				  (z + (nctx->hierz.clear_seq & 7)) / 8, 0);
		context_drv(ctx)->surface_fill(ctx, s, ~0, value, x, y, w, h);
	}
}

static void
nv17_zclear(struct gl_context *ctx, GLbitfield *buffers)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *celsius = context_eng3d(ctx);
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(
		ctx->DrawBuffer);
	struct nouveau_surface *s = &to_nouveau_renderbuffer(
		nfb->base._DepthBuffer->Wrapped)->surface;

	/* Clear the hierarchical depth buffer */
	BEGIN_RING(chan, celsius, NV17_3D_HIERZ_FILL_VALUE, 1);
	OUT_RING(chan, pack_zs_f(s->format, ctx->Depth.Clear, 0));
	BEGIN_RING(chan, celsius, NV17_3D_HIERZ_BUFFER_CLEAR, 1);
	OUT_RING(chan, 1);

	/* Mark the depth buffer as cleared */
	if (use_fast_zclear(ctx, *buffers)) {
		if (nctx->hierz.clear_seq)
			*buffers &= ~BUFFER_BIT_DEPTH;

		nfb->hierz.clear_value =
			pack_zs_f(s->format, ctx->Depth.Clear, 0);
		nctx->hierz.clear_seq++;

		context_dirty(ctx, ZCLEAR);
	}
}

static void
nv10_clear(struct gl_context *ctx, GLbitfield buffers)
{
	nouveau_validate_framebuffer(ctx);

	if ((buffers & BUFFER_BIT_DEPTH) && ctx->Depth.Mask) {
		if (context_chipset(ctx) >= 0x17)
			nv17_zclear(ctx, &buffers);
		else
			nv10_zclear(ctx, &buffers);

		/* Emit the zclear state if it's dirty */
		_mesa_update_state(ctx);
	}

	nouveau_clear(ctx, buffers);
}

static void
nv10_hwctx_init(struct gl_context *ctx)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *celsius = context_eng3d(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	int i;

	BEGIN_RING(chan, celsius, NV10_3D_DMA_NOTIFY, 1);
	OUT_RING(chan, hw->ntfy->handle);

	BEGIN_RING(chan, celsius, NV10_3D_DMA_TEXTURE0, 3);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);
	OUT_RING(chan, chan->gart->handle);
	BEGIN_RING(chan, celsius, NV10_3D_DMA_COLOR, 2);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->vram->handle);

	BEGIN_RING(chan, celsius, NV04_GRAPH_NOP, 1);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10_3D_RT_HORIZ, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10_3D_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING(chan, 0x7ff << 16 | 0x800);
	BEGIN_RING(chan, celsius, NV10_3D_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING(chan, 0x7ff << 16 | 0x800);

	for (i = 1; i < 8; i++) {
		BEGIN_RING(chan, celsius, NV10_3D_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING(chan, 0);
		BEGIN_RING(chan, celsius, NV10_3D_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING(chan, 0);
	}

	BEGIN_RING(chan, celsius, 0x290, 1);
	OUT_RING(chan, 0x10 << 16 | 1);
	BEGIN_RING(chan, celsius, 0x3f4, 1);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius,  NV04_GRAPH_NOP, 1);
	OUT_RING(chan, 0);

	if (context_chipset(ctx) >= 0x17) {
		BEGIN_RING(chan, celsius, NV17_3D_UNK01AC, 2);
		OUT_RING(chan, chan->vram->handle);
		OUT_RING(chan, chan->vram->handle);

		BEGIN_RING(chan, celsius, 0xd84, 1);
		OUT_RING(chan, 0x3);

		BEGIN_RING(chan, celsius, NV17_3D_COLOR_MASK_ENABLE, 1);
		OUT_RING(chan, 1);
	}

	if (context_chipset(ctx) >= 0x11) {
		BEGIN_RING(chan, celsius, 0x120, 3);
		OUT_RING(chan, 0);
		OUT_RING(chan, 1);
		OUT_RING(chan, 2);

		BEGIN_RING(chan, celsius, NV04_GRAPH_NOP, 1);
		OUT_RING(chan, 0);
	}

	BEGIN_RING(chan, celsius,  NV04_GRAPH_NOP, 1);
	OUT_RING(chan, 0);

	/* Set state */
	BEGIN_RING(chan, celsius, NV10_3D_FOG_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_ALPHA_FUNC_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_ALPHA_FUNC_FUNC, 2);
	OUT_RING(chan, 0x207);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_TEX_ENABLE(0), 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10_3D_BLEND_FUNC_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_DITHER_ENABLE, 2);
	OUT_RING(chan, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_LINE_SMOOTH_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_VERTEX_WEIGHT_ENABLE, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_BLEND_FUNC_SRC, 4);
	OUT_RING(chan, 1);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0x8006);
	BEGIN_RING(chan, celsius, NV10_3D_STENCIL_MASK, 8);
	OUT_RING(chan, 0xff);
	OUT_RING(chan, 0x207);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0xff);
	OUT_RING(chan, 0x1e00);
	OUT_RING(chan, 0x1e00);
	OUT_RING(chan, 0x1e00);
	OUT_RING(chan, 0x1d01);
	BEGIN_RING(chan, celsius, NV10_3D_NORMALIZE_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_FOG_ENABLE, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_LIGHT_MODEL, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_SEPARATE_SPECULAR_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_ENABLED_LIGHTS, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_POLYGON_OFFSET_POINT_ENABLE, 3);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_DEPTH_FUNC, 1);
	OUT_RING(chan, 0x201);
	BEGIN_RING(chan, celsius, NV10_3D_DEPTH_WRITE_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_DEPTH_TEST_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_POLYGON_OFFSET_FACTOR, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_POINT_SIZE, 1);
	OUT_RING(chan, 8);
	BEGIN_RING(chan, celsius, NV10_3D_POINT_PARAMETERS_ENABLE, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_LINE_WIDTH, 1);
	OUT_RING(chan, 8);
	BEGIN_RING(chan, celsius, NV10_3D_LINE_SMOOTH_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_POLYGON_MODE_FRONT, 2);
	OUT_RING(chan, 0x1b02);
	OUT_RING(chan, 0x1b02);
	BEGIN_RING(chan, celsius, NV10_3D_CULL_FACE, 2);
	OUT_RING(chan, 0x405);
	OUT_RING(chan, 0x901);
	BEGIN_RING(chan, celsius, NV10_3D_POLYGON_SMOOTH_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_CULL_FACE_ENABLE, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_TEX_GEN_MODE(0, 0), 8);
	for (i = 0; i < 8; i++)
		OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10_3D_TEX_MATRIX_ENABLE(0), 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_FOG_COEFF(0), 3);
	OUT_RING(chan, 0x3fc00000);	/* -1.50 */
	OUT_RING(chan, 0xbdb8aa0a);	/* -0.09 */
	OUT_RING(chan, 0);		/*  0.00 */

	BEGIN_RING(chan, celsius,  NV04_GRAPH_NOP, 1);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, celsius, NV10_3D_FOG_MODE, 2);
	OUT_RING(chan, 0x802);
	OUT_RING(chan, 2);
	/* for some reason VIEW_MATRIX_ENABLE need to be 6 instead of 4 when
	 * using texturing, except when using the texture matrix
	 */
	BEGIN_RING(chan, celsius, NV10_3D_VIEW_MATRIX_ENABLE, 1);
	OUT_RING(chan, 6);
	BEGIN_RING(chan, celsius, NV10_3D_COLOR_MASK, 1);
	OUT_RING(chan, 0x01010101);

	/* Set vertex component */
	BEGIN_RING(chan, celsius, NV10_3D_VERTEX_COL_4F_R, 4);
	OUT_RINGf(chan, 1.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10_3D_VERTEX_COL2_3F_R, 3);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, celsius, NV10_3D_VERTEX_NOR_3F_X, 3);
	OUT_RING(chan, 0);
	OUT_RING(chan, 0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10_3D_VERTEX_TX0_4F_S, 4);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10_3D_VERTEX_TX1_4F_S, 4);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 1.0);
	BEGIN_RING(chan, celsius, NV10_3D_VERTEX_FOG_1F, 1);
	OUT_RINGf(chan, 0.0);
	BEGIN_RING(chan, celsius, NV10_3D_EDGEFLAG_ENABLE, 1);
	OUT_RING(chan, 1);

	BEGIN_RING(chan, celsius, NV10_3D_DEPTH_RANGE_NEAR, 2);
	OUT_RINGf(chan, 0.0);
	OUT_RINGf(chan, 16777216.0);

	FIRE_RING(chan);
}

static void
nv10_context_destroy(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nv04_surface_takedown(ctx);
	nv10_swtnl_destroy(ctx);
	nv10_vbo_destroy(ctx);

	nouveau_grobj_free(&nctx->hw.eng3d);

	nouveau_context_deinit(ctx);
	FREE(ctx);
}

static struct gl_context *
nv10_context_create(struct nouveau_screen *screen, const struct gl_config *visual,
		    struct gl_context *share_ctx)
{
	struct nouveau_context *nctx;
	struct gl_context *ctx;
	unsigned celsius_class;
	int ret;

	nctx = CALLOC_STRUCT(nouveau_context);
	if (!nctx)
		return NULL;

	ctx = &nctx->base;

	if (!nouveau_context_init(ctx, screen, visual, share_ctx))
		goto fail;

	driInitExtensions(ctx, nv10_extensions, GL_FALSE);

	/* GL constants. */
	ctx->Const.MaxTextureLevels = 12;
	ctx->Const.MaxTextureCoordUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 2;
	ctx->Const.MaxTextureLodBias = 15;
	ctx->Driver.Clear = nv10_clear;

	/* 2D engine. */
	ret = nv04_surface_init(ctx);
	if (!ret)
		goto fail;

	/* 3D engine. */
	if (context_chipset(ctx) >= 0x17)
		celsius_class = NV17_3D;
	else if (context_chipset(ctx) >= 0x11)
		celsius_class = NV11_3D;
	else
		celsius_class = NV10_3D;

	ret = nouveau_grobj_alloc(context_chan(ctx), 0xbeef0001, celsius_class,
				  &nctx->hw.eng3d);
	if (ret)
		goto fail;

	nv10_hwctx_init(ctx);
	nv10_vbo_init(ctx);
	nv10_swtnl_init(ctx);

	return ctx;

fail:
	nv10_context_destroy(ctx);
	return NULL;
}

const struct nouveau_driver nv10_driver = {
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
		nv10_emit_tex_mat,
		nv10_emit_tex_mat,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_tex_obj,
		nv10_emit_tex_obj,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv10_emit_viewport,
		nv10_emit_zclear
	},
	.num_emit = NUM_NV10_STATE,
};
