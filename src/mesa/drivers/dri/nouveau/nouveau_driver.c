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
#include "nouveau_fbo.h"
#include "nouveau_util.h"

#include "drivers/common/meta.h"

static const GLubyte *
nouveau_get_string(GLcontext *ctx, GLenum name)
{
	static char buffer[128];
	char hardware_name[32];

	switch (name) {
		case GL_VENDOR:
			return (GLubyte *)"Nouveau";

		case GL_RENDERER:
			sprintf(hardware_name, "nv%02X", context_chipset(ctx));
			driGetRendererString(buffer, hardware_name, DRIVER_DATE, 0);

			return (GLubyte *)buffer;
		default:
			return NULL;
	}
}

static void
nouveau_flush(GLcontext *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);
	struct nouveau_channel *chan = context_chan(ctx);

	FIRE_RING(chan);

	if (ctx->DrawBuffer->Name == 0 &&
	    ctx->DrawBuffer->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT) {
		__DRIscreen *screen = nctx->screen->dri_screen;
		__DRIdri2LoaderExtension *dri2 = screen->dri2.loader;
		__DRIdrawable *drawable = nctx->dri_context->driDrawablePriv;

		dri2->flushFrontBuffer(drawable, drawable->loaderPrivate);
	}
}

static void
nouveau_finish(GLcontext *ctx)
{
	nouveau_flush(ctx);
}

void
nouveau_clear(GLcontext *ctx, GLbitfield buffers)
{
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	int x, y, w, h;
	int i, buf;

	nouveau_validate_framebuffer(ctx);
	get_scissors(fb, &x, &y, &w, &h);

	for (i = 0; i < BUFFER_COUNT; i++) {
		struct nouveau_surface *s;
		unsigned mask, value;

		buf = buffers & (1 << i);
		if (!buf)
			continue;

		s = &to_nouveau_renderbuffer(
			fb->Attachment[i].Renderbuffer->Wrapped)->surface;

		if (buf & BUFFER_BITS_COLOR) {
			mask = pack_rgba_i(s->format, ctx->Color.ColorMask[0]);
			value = pack_rgba_f(s->format, ctx->Color.ClearColor);

			if (mask)
				context_drv(ctx)->surface_fill(
					ctx, s, mask, value, x, y, w, h);

			buffers &= ~buf;

		} else if (buf & (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL)) {
			mask = pack_zs_i(s->format,
					 (buffers & BUFFER_BIT_DEPTH &&
					  ctx->Depth.Mask) ? ~0 : 0,
					 (buffers & BUFFER_BIT_STENCIL ?
					  ctx->Stencil.WriteMask[0] : 0));
			value = pack_zs_f(s->format,
					  ctx->Depth.Clear,
					  ctx->Stencil.Clear);

			if (mask)
				context_drv(ctx)->surface_fill(
					ctx, s, mask, value, x, y, w, h);

			buffers &= ~(BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
		}
	}

	if (buffers)
		_mesa_meta_Clear(ctx, buffers);
}

void
nouveau_driver_functions_init(struct dd_function_table *functions)
{
	functions->GetString = nouveau_get_string;
	functions->Flush = nouveau_flush;
	functions->Finish = nouveau_finish;
	functions->Clear = nouveau_clear;
}
