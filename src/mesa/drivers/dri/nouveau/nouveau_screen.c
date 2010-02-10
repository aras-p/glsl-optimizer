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
#include "nouveau_drmif.h"
#include "nv04_driver.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

#include "main/framebuffer.h"
#include "main/renderbuffer.h"

static const __DRIextension *nouveau_screen_extensions[];

static void
nouveau_destroy_screen(__DRIscreen *dri_screen);

static void
nouveau_channel_flush_notify(struct nouveau_channel *chan)
{
	struct nouveau_screen *screen = chan->user_private;
	struct nouveau_context *nctx = screen->context;

	if (nctx && nctx->fallback < SWRAST)
		nouveau_state_emit(&nctx->base);
}

static const __DRIconfig **
nouveau_get_configs(void)
{
	__DRIconfig **configs = NULL;
	int i;

	const uint8_t depth_bits[]   = { 0, 16, 24, 24 };
	const uint8_t stencil_bits[] = { 0,  0,  0,  8 };
	const uint8_t msaa_samples[] = { 0 };

	const struct {
		GLenum format;
		GLenum type;
	} fb_formats[] = {
		{ GL_RGB , GL_UNSIGNED_SHORT_5_6_5     },
		{ GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV },
		{ GL_BGR , GL_UNSIGNED_INT_8_8_8_8_REV },
	};

	const GLenum back_buffer_modes[] = {
		GLX_NONE, GLX_SWAP_UNDEFINED_OML
	};

	for (i = 0; i < Elements(fb_formats); i++) {
		__DRIconfig **config;

		config = driCreateConfigs(fb_formats[i].format,
					  fb_formats[i].type,
					  depth_bits, stencil_bits,
					  Elements(depth_bits),
					  back_buffer_modes,
					  Elements(back_buffer_modes),
					  msaa_samples,
					  Elements(msaa_samples),
					  GL_TRUE);
		assert(config);

		configs = configs ? driConcatConfigs(configs, config)
			: config;
	}

	return (const __DRIconfig **)configs;
}

static const __DRIconfig **
nouveau_init_screen2(__DRIscreen *dri_screen)
{
	const __DRIconfig **configs;
	struct nouveau_screen *screen;
	int ret;

	/* Allocate the screen. */
	screen = CALLOC_STRUCT(nouveau_screen);
	if (!screen)
		return NULL;

	dri_screen->private = screen;
	dri_screen->extensions = nouveau_screen_extensions;
	screen->dri_screen = dri_screen;

	/* Open the DRM device. */
	ret = nouveau_device_open_existing(&screen->device, 0, dri_screen->fd,
					   0);
	if (ret) {
		nouveau_error("Error opening the DRM device.\n");
		goto fail;
	}

	ret = nouveau_channel_alloc(screen->device, 0xbeef0201, 0xbeef0202,
				    &screen->chan);
	if (ret) {
		nouveau_error("Error initializing the FIFO.\n");
		goto fail;
	}
	screen->chan->flush_notify = nouveau_channel_flush_notify;
	screen->chan->user_private = screen;

	/* Do the card specific initialization */
	switch (screen->device->chipset & 0xf0) {
	case 0x00:
		ret = nv04_screen_init(screen);
		break;
	case 0x10:
		ret = nv10_screen_init(screen);
		break;
	case 0x20:
		ret = nv20_screen_init(screen);
		break;
	default:
		assert(0);
	}
	if (!ret) {
		nouveau_error("Error initializing the hardware.\n");
		goto fail;
	}

	configs = nouveau_get_configs();
	if (!configs) {
		nouveau_error("Error creating the framebuffer configs.\n");
		goto fail;
	}

	return configs;
fail:
	nouveau_destroy_screen(dri_screen);
	return NULL;

}

static void
nouveau_destroy_screen(__DRIscreen *dri_screen)
{
	struct nouveau_screen *screen = dri_screen->private;

	if (!screen)
		return;

	screen->driver->screen_destroy(screen);

	if (screen->chan) {
		screen->chan->flush_notify = NULL;
		nouveau_channel_free(&screen->chan);
	}

	if (screen->device)
		nouveau_device_close(&screen->device);

	FREE(screen);
	dri_screen->private = NULL;
}

static GLboolean
nouveau_create_buffer(__DRIscreen *dri_screen,
		      __DRIdrawable *drawable,
		      const __GLcontextModes *visual,
		      GLboolean is_pixmap)
{
	struct gl_renderbuffer  *rb;
	struct gl_framebuffer *fb;
	GLenum color_format;

	if (is_pixmap)
		return GL_FALSE; /* not implemented */

	if (visual->redBits == 5)
		color_format = GL_RGB5;
	else if (visual->alphaBits == 0)
		color_format = GL_RGB8;
	else
		color_format = GL_RGBA8;

	fb = nouveau_framebuffer_dri_new(visual);
	if (!fb)
		return GL_FALSE;

	/* Front buffer. */
	rb = nouveau_renderbuffer_dri_new(color_format, drawable);
	_mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, rb);

	/* Back buffer */
	if (visual->doubleBufferMode) {
		rb = nouveau_renderbuffer_dri_new(color_format, drawable);
		_mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, rb);
	}

	/* Depth/stencil buffer. */
	if (visual->depthBits == 24 && visual->stencilBits == 8) {
		rb = nouveau_renderbuffer_dri_new(GL_DEPTH24_STENCIL8_EXT, drawable);
		_mesa_add_renderbuffer(fb, BUFFER_DEPTH, rb);
		_mesa_add_renderbuffer(fb, BUFFER_STENCIL, rb);

	} else if (visual->depthBits == 24) {
		rb = nouveau_renderbuffer_dri_new(GL_DEPTH_COMPONENT24, drawable);
		_mesa_add_renderbuffer(fb, BUFFER_DEPTH, rb);

	} else if (visual->depthBits == 16) {
		rb = nouveau_renderbuffer_dri_new(GL_DEPTH_COMPONENT16, drawable);
		_mesa_add_renderbuffer(fb, BUFFER_DEPTH, rb);
	}

	/* Software renderbuffers. */
	_mesa_add_soft_renderbuffers(fb, GL_FALSE, GL_FALSE, GL_FALSE,
				     visual->accumRedBits > 0,
				     GL_FALSE, GL_FALSE);

	drawable->driverPrivate = fb;

	return GL_TRUE;
}

static void
nouveau_destroy_buffer(__DRIdrawable *drawable)
{
	_mesa_reference_framebuffer(
		(struct gl_framebuffer **)&drawable->driverPrivate, NULL);
}

static const __DRIextension *nouveau_screen_extensions[] = {
    NULL
};

const struct __DriverAPIRec driDriverAPI = {
	.InitScreen2     = nouveau_init_screen2,
	.DestroyScreen   = nouveau_destroy_screen,
	.CreateBuffer    = nouveau_create_buffer,
	.DestroyBuffer   = nouveau_destroy_buffer,
	.CreateContext   = nouveau_context_create,
	.DestroyContext  = nouveau_context_destroy,
	.MakeCurrent     = nouveau_context_make_current,
	.UnbindContext   = nouveau_context_unbind,
};

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
	&driCoreExtension.base,
	&driDRI2Extension.base,
	NULL
};
