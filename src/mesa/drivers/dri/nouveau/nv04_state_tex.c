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
#include "nouveau_texture.h"
#include "nouveau_util.h"
#include "nouveau_gldefs.h"
#include "nouveau_class.h"
#include "nv04_driver.h"

static uint32_t
get_tex_format(struct gl_texture_image *ti)
{
	switch (ti->TexFormat) {
	case MESA_FORMAT_A8:
	case MESA_FORMAT_L8:
	case MESA_FORMAT_I8:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_Y8;
	case MESA_FORMAT_ARGB1555:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A1R5G5B5;
	case MESA_FORMAT_ARGB4444:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A4R4G4B4;
	case MESA_FORMAT_RGB565:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_R5G6B5;
	case MESA_FORMAT_ARGB8888:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A8R8G8B8;
	case MESA_FORMAT_XRGB8888:
		return NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_X8R8G8B8;
	default:
		assert(0);
	}
}

static inline unsigned
get_wrap_mode(unsigned wrap)
{
	switch (wrap) {
	case GL_REPEAT:
		return 0x1;
	case GL_MIRRORED_REPEAT:
		return 0x2;
	case GL_CLAMP:
	case GL_CLAMP_TO_EDGE:
		return 0x3;
	case GL_CLAMP_TO_BORDER:
		return 0x4;
	default:
		assert(0);
	}
}

void
nv04_emit_tex_obj(GLcontext *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_OBJ0;
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *fahrenheit = nv04_context_engine(ctx);
	struct nouveau_bo_context *bctx = context_bctx_i(ctx, TEXTURE, i);
	const int bo_flags = NOUVEAU_BO_RD | NOUVEAU_BO_GART | NOUVEAU_BO_VRAM;
	struct nouveau_surface *s;
	uint32_t format = 0xa0, filter = 0x1010;

	if (i && !nv04_mtex_engine(fahrenheit))
		return;

	if (ctx->Texture.Unit[i]._ReallyEnabled) {
		struct gl_texture_object *t = ctx->Texture.Unit[i]._Current;
		struct gl_texture_image *ti = t->Image[0][t->BaseLevel];
		int lod_max = 1, lod_bias = 0;

		if (!nouveau_texture_validate(ctx, t))
			return;

		s = &to_nouveau_texture(t)->surfaces[t->BaseLevel];

		if (t->MinFilter != GL_NEAREST &&
		    t->MinFilter != GL_LINEAR) {
			lod_max = CLAMP(MIN2(t->MaxLod, t->_MaxLambda),
					0, 15) + 1;

			lod_bias = CLAMP(ctx->Texture.Unit[i].LodBias +
					 t->LodBias, 0, 15);
		}

		format |= get_wrap_mode(t->WrapT) << 28 |
			get_wrap_mode(t->WrapS) << 24 |
			ti->HeightLog2 << 20 |
			ti->WidthLog2 << 16 |
			lod_max << 12 |
			get_tex_format(ti);

		filter |= log2i(t->MaxAnisotropy) << 31 |
			nvgl_filter_mode(t->MagFilter) << 28 |
			log2i(t->MaxAnisotropy) << 27 |
			nvgl_filter_mode(t->MinFilter) << 24 |
			lod_bias << 16;

	} else {
		s = &to_nv04_context(ctx)->dummy_texture;

		format |= NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_REPEAT |
			NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSV_REPEAT |
			1 << 12 |
			NV04_TEXTURED_TRIANGLE_FORMAT_COLOR_A8R8G8B8;

		filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_NEAREST |
			NV04_TEXTURED_TRIANGLE_FILTER_MAGNIFY_NEAREST;
	}

	if (nv04_mtex_engine(fahrenheit)) {
		nouveau_bo_markl(bctx, fahrenheit,
				 NV04_MULTITEX_TRIANGLE_OFFSET(i),
				 s->bo, 0, bo_flags);

		nouveau_bo_mark(bctx, fahrenheit,
				NV04_MULTITEX_TRIANGLE_FORMAT(i),
				s->bo, format, 0,
				NV04_MULTITEX_TRIANGLE_FORMAT_DMA_A,
				NV04_MULTITEX_TRIANGLE_FORMAT_DMA_B,
				bo_flags | NOUVEAU_BO_OR);

		BEGIN_RING(chan, fahrenheit, NV04_MULTITEX_TRIANGLE_FILTER(i), 1);
		OUT_RING(chan, filter);

	} else {
		nouveau_bo_markl(bctx, fahrenheit,
				 NV04_TEXTURED_TRIANGLE_OFFSET,
				 s->bo, 0, bo_flags);

		nouveau_bo_mark(bctx, fahrenheit,
				NV04_TEXTURED_TRIANGLE_FORMAT,
				s->bo, format, 0,
				NV04_TEXTURED_TRIANGLE_FORMAT_DMA_A,
				NV04_TEXTURED_TRIANGLE_FORMAT_DMA_B,
				bo_flags | NOUVEAU_BO_OR);

		BEGIN_RING(chan, fahrenheit, NV04_TEXTURED_TRIANGLE_COLORKEY, 1);
		OUT_RING(chan, 0);

		BEGIN_RING(chan, fahrenheit, NV04_TEXTURED_TRIANGLE_FILTER, 1);
		OUT_RING(chan, filter);
	}
}
