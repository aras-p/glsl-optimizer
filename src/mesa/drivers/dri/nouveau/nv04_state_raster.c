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
#include "nouveau_context.h"
#include "nouveau_util.h"
#include "nouveau_class.h"
#include "nv04_driver.h"

static unsigned
get_comparison_op(unsigned op)
{
	switch (op) {
	case GL_NEVER:
		return 0x1;
	case GL_LESS:
		return 0x2;
	case GL_EQUAL:
		return 0x3;
	case GL_LEQUAL:
		return 0x4;
	case GL_GREATER:
		return 0x5;
	case GL_NOTEQUAL:
		return 0x6;
	case GL_GEQUAL:
		return 0x7;
	case GL_ALWAYS:
		return 0x8;
	default:
		assert(0);
	}
}

static unsigned
get_stencil_op(unsigned op)
{
	switch (op) {
	case GL_KEEP:
		return 0x1;
	case GL_ZERO:
		return 0x2;
	case GL_REPLACE:
		return 0x3;
	case GL_INCR:
		return 0x4;
	case GL_DECR:
		return 0x5;
	case GL_INVERT:
		return 0x6;
	case GL_INCR_WRAP:
		return 0x7;
	case GL_DECR_WRAP:
		return 0x8;
	default:
		assert(0);
	}
}

static unsigned
get_texenv_mode(unsigned mode)
{
	switch (mode) {
	case GL_REPLACE:
		return 0x1;
	case GL_DECAL:
		return 0x3;
	case GL_MODULATE:
		return 0x4;
	default:
		assert(0);
	}
}

static unsigned
get_blend_func(unsigned func)
{
	switch (func) {
	case GL_ZERO:
		return 0x1;
	case GL_ONE:
		return 0x2;
	case GL_SRC_COLOR:
		return 0x3;
	case GL_ONE_MINUS_SRC_COLOR:
		return 0x4;
	case GL_SRC_ALPHA:
		return 0x5;
	case GL_ONE_MINUS_SRC_ALPHA:
		return 0x6;
	case GL_DST_ALPHA:
		return 0x7;
	case GL_ONE_MINUS_DST_ALPHA:
		return 0x8;
	case GL_DST_COLOR:
		return 0x9;
	case GL_ONE_MINUS_DST_COLOR:
		return 0xa;
	case GL_SRC_ALPHA_SATURATE:
		return 0xb;
	default:
		assert(0);
	}
}

void
nv04_defer_control(GLcontext *ctx, int emit)
{
	context_dirty(ctx, CONTROL);
}

void
nv04_emit_control(GLcontext *ctx, int emit)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *fahrenheit = nv04_context_engine(ctx);

	if (nv04_mtex_engine(fahrenheit)) {
		int cull_mode = ctx->Polygon.CullFaceMode;
		int front_face = ctx->Polygon.FrontFace;
		uint32_t ctrl0 = 1 << 30 |
			NV04_MULTITEX_TRIANGLE_CONTROL0_ORIGIN;
		uint32_t ctrl1 = 0, ctrl2 = 0;

		/* Color mask. */
		if (ctx->Color.ColorMask[0][RCOMP])
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_RED_WRITE;
		if (ctx->Color.ColorMask[0][GCOMP])
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_GREEN_WRITE;
		if (ctx->Color.ColorMask[0][BCOMP])
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_BLUE_WRITE;
		if (ctx->Color.ColorMask[0][ACOMP])
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_ALPHA_WRITE;

		/* Dithering. */
		if (ctx->Color.DitherFlag)
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_DITHER_ENABLE;

		/* Cull mode. */
		if (!ctx->Polygon.CullFlag)
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_CULL_MODE_NONE;
		else if (cull_mode == GL_FRONT_AND_BACK)
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_CULL_MODE_BOTH;
		else
			ctrl0 |= (cull_mode == GL_FRONT) ^ (front_face == GL_CCW) ?
				NV04_MULTITEX_TRIANGLE_CONTROL0_CULL_MODE_CW :
				NV04_MULTITEX_TRIANGLE_CONTROL0_CULL_MODE_CCW;

		/* Depth test. */
		if (ctx->Depth.Test)
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_Z_ENABLE;

		if (ctx->Depth.Mask)
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_Z_WRITE;

		ctrl0 |= get_comparison_op(ctx->Depth.Func) << 16;

		/* Alpha test. */
		if (ctx->Color.AlphaEnabled)
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_ALPHA_ENABLE;

		ctrl0 |= get_comparison_op(ctx->Color.AlphaFunc) << 8 |
			FLOAT_TO_UBYTE(ctx->Color.AlphaRef);

		/* Stencil test. */
		if (ctx->Stencil.WriteMask[0])
			ctrl0 |= NV04_MULTITEX_TRIANGLE_CONTROL0_STENCIL_WRITE;

		if (ctx->Stencil.Enabled)
			ctrl1 |= NV04_MULTITEX_TRIANGLE_CONTROL1_STENCIL_ENABLE;

		ctrl1 |= get_comparison_op(ctx->Stencil.Function[0]) << 4 |
			ctx->Stencil.Ref[0] << 8 |
			ctx->Stencil.ValueMask[0] << 16 |
			ctx->Stencil.WriteMask[0] << 24;

		ctrl2 |= get_stencil_op(ctx->Stencil.ZPassFunc[0]) << 8 |
			get_stencil_op(ctx->Stencil.ZFailFunc[0]) << 4 |
			get_stencil_op(ctx->Stencil.FailFunc[0]);

		BEGIN_RING(chan, fahrenheit, NV04_MULTITEX_TRIANGLE_CONTROL0, 3);
		OUT_RING(chan, ctrl0);
		OUT_RING(chan, ctrl1);
		OUT_RING(chan, ctrl2);

	} else {
		int cull_mode = ctx->Polygon.CullFaceMode;
		int front_face = ctx->Polygon.FrontFace;
		uint32_t ctrl = 1 << 30 |
			NV04_TEXTURED_TRIANGLE_CONTROL_ORIGIN;

		/* Dithering. */
		if (ctx->Color.DitherFlag)
			ctrl |= NV04_TEXTURED_TRIANGLE_CONTROL_DITHER_ENABLE;

		/* Cull mode. */
		if (!ctx->Polygon.CullFlag)
			ctrl |= NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_NONE;
		else if (cull_mode == GL_FRONT_AND_BACK)
			ctrl |= NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_BOTH;
		else
			ctrl |= (cull_mode == GL_FRONT) ^ (front_face == GL_CCW) ?
				NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_CW :
				NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_CCW;

		/* Depth test. */
		if (ctx->Depth.Test)
			ctrl |= NV04_TEXTURED_TRIANGLE_CONTROL_Z_ENABLE;
		if (ctx->Depth.Mask)
			ctrl |= NV04_TEXTURED_TRIANGLE_CONTROL_Z_WRITE;

		ctrl |= get_comparison_op(ctx->Depth.Func) << 16;

		/* Alpha test. */
		if (ctx->Color.AlphaEnabled)
			ctrl |= NV04_TEXTURED_TRIANGLE_CONTROL_ALPHA_ENABLE;

		ctrl |= get_comparison_op(ctx->Color.AlphaFunc) << 8 |
			FLOAT_TO_UBYTE(ctx->Color.AlphaRef);

		BEGIN_RING(chan, fahrenheit, NV04_TEXTURED_TRIANGLE_CONTROL, 1);
		OUT_RING(chan, ctrl);
	}
}

void
nv04_defer_blend(GLcontext *ctx, int emit)
{
	context_dirty(ctx, BLEND);
}

void
nv04_emit_blend(GLcontext *ctx, int emit)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *fahrenheit = nv04_context_engine(ctx);

	if (nv04_mtex_engine(fahrenheit)) {
		uint32_t blend = 0x2 << 4 |
			NV04_MULTITEX_TRIANGLE_BLEND_TEXTURE_PERSPECTIVE_ENABLE;

		/* Alpha blending. */
		blend |= get_blend_func(ctx->Color.BlendDstRGB) << 28 |
			get_blend_func(ctx->Color.BlendSrcRGB) << 24;

		if (ctx->Color.BlendEnabled)
			blend |= NV04_MULTITEX_TRIANGLE_BLEND_BLEND_ENABLE;

		/* Shade model. */
		if (ctx->Light.ShadeModel == GL_SMOOTH)
			blend |= NV04_MULTITEX_TRIANGLE_BLEND_SHADE_MODE_GOURAUD;
		else
			blend |= NV04_MULTITEX_TRIANGLE_BLEND_SHADE_MODE_FLAT;

		/* Fog. */
		if (ctx->Fog.Enabled)
			blend |= NV04_MULTITEX_TRIANGLE_BLEND_FOG_ENABLE;

		BEGIN_RING(chan, fahrenheit, NV04_MULTITEX_TRIANGLE_BLEND, 1);
		OUT_RING(chan, blend);

		BEGIN_RING(chan, fahrenheit, NV04_MULTITEX_TRIANGLE_FOGCOLOR, 1);
		OUT_RING(chan, pack_rgba_f(MESA_FORMAT_ARGB8888,
					   ctx->Fog.Color));

	} else {
		uint32_t blend = 0x2 << 4 |
			NV04_TEXTURED_TRIANGLE_BLEND_TEXTURE_PERSPECTIVE_ENABLE;

		/* Alpha blending. */
		blend |= get_blend_func(ctx->Color.BlendDstRGB) << 28 |
			get_blend_func(ctx->Color.BlendSrcRGB) << 24;

		if (ctx->Color.BlendEnabled)
			blend |= NV04_TEXTURED_TRIANGLE_BLEND_BLEND_ENABLE;

		/* Shade model. */
		if (ctx->Light.ShadeModel == GL_SMOOTH)
			blend |= NV04_TEXTURED_TRIANGLE_BLEND_SHADE_MODE_GOURAUD;
		else
			blend |= NV04_TEXTURED_TRIANGLE_BLEND_SHADE_MODE_FLAT;

		/* Texture environment. */
		if (ctx->Texture._EnabledUnits)
			blend |= get_texenv_mode(ctx->Texture.Unit[0].EnvMode);
		else
			blend |= get_texenv_mode(GL_MODULATE);

		/* Fog. */
		if (ctx->Fog.Enabled)
			blend |= NV04_TEXTURED_TRIANGLE_BLEND_FOG_ENABLE;

		BEGIN_RING(chan, fahrenheit, NV04_TEXTURED_TRIANGLE_BLEND, 1);
		OUT_RING(chan, blend);

		BEGIN_RING(chan, fahrenheit, NV04_TEXTURED_TRIANGLE_FOGCOLOR, 1);
		OUT_RING(chan, pack_rgba_f(MESA_FORMAT_ARGB8888,
					   ctx->Fog.Color));
	}
}
