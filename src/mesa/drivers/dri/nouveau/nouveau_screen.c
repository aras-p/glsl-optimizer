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

#include <xf86drm.h>
#include <nouveau_drm.h>
#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_fbo.h"
#include "nouveau_texture.h"
#include "nv04_driver.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

#include "main/framebuffer.h"
#include "main/fbobject.h"
#include "main/renderbuffer.h"
#include "swrast/s_renderbuffer.h"

static const __DRIextension *nouveau_screen_extensions[];

static void
nouveau_destroy_screen(__DRIscreen *dri_screen);

static const __DRIconfig **
nouveau_get_configs(void)
{
	__DRIconfig **configs = NULL;
	int i;

	const uint8_t depth_bits[]   = { 0, 16, 24, 24 };
	const uint8_t stencil_bits[] = { 0,  0,  0,  8 };
	const uint8_t msaa_samples[] = { 0 };

	static const mesa_format formats[3] = {
		MESA_FORMAT_B5G6R5_UNORM,
		MESA_FORMAT_B8G8R8A8_UNORM,
		MESA_FORMAT_B8G8R8X8_UNORM,
	};

	const GLenum back_buffer_modes[] = {
		GLX_NONE, GLX_SWAP_UNDEFINED_OML
	};

	for (i = 0; i < Elements(formats); i++) {
		__DRIconfig **config;

		config = driCreateConfigs(formats[i],
					  depth_bits, stencil_bits,
					  Elements(depth_bits),
					  back_buffer_modes,
					  Elements(back_buffer_modes),
					  msaa_samples,
					  Elements(msaa_samples),
					  GL_TRUE);
		assert(config);

		configs = driConcatConfigs(configs, config);
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

	dri_screen->driverPrivate = screen;

	/* Open the DRM device. */
	ret = nouveau_device_wrap(dri_screen->fd, 0, &screen->device);
	if (ret) {
		nouveau_error("Error opening the DRM device.\n");
		goto fail;
	}

	/* Choose the card specific function pointers. */
	switch (screen->device->chipset & 0xf0) {
	case 0x00:
		screen->driver = &nv04_driver;
		dri_screen->max_gl_compat_version = 12;
		break;
	case 0x10:
		screen->driver = &nv10_driver;
		dri_screen->max_gl_compat_version = 12;
		dri_screen->max_gl_es1_version = 10;
		break;
	case 0x20:
		screen->driver = &nv20_driver;
		dri_screen->max_gl_compat_version = 13;
		dri_screen->max_gl_es1_version = 10;
		break;
	default:
		nouveau_error("Unknown chipset: %02X\n",
			      screen->device->chipset);
		goto fail;
	}

	dri_screen->extensions = nouveau_screen_extensions;
	screen->dri_screen = dri_screen;

	configs = nouveau_get_configs();
	if (!configs)
		goto fail;

	return configs;
fail:
	nouveau_destroy_screen(dri_screen);
	return NULL;

}

static int
nouveau_query_renderer_integer(__DRIscreen *psp, int param,
			       unsigned int *value)
{
	const struct nouveau_screen *const screen =
		(struct nouveau_screen *) psp->driverPrivate;

	switch (param) {
	case __DRI2_RENDERER_VENDOR_ID:
		value[0] = 0x10de;
		return 0;
	case __DRI2_RENDERER_DEVICE_ID: {
		uint64_t device_id;

		if (nouveau_getparam(screen->device,
				     NOUVEAU_GETPARAM_PCI_DEVICE,
				     &device_id)) {
			nouveau_error("Error retrieving the device PCIID.\n");
			device_id = -1;
		}
		value[0] = (unsigned int) device_id;
		return 0;
	}
	case __DRI2_RENDERER_ACCELERATED:
		value[0] = 1;
		return 0;
	case __DRI2_RENDERER_VIDEO_MEMORY:
		/* XXX: return vram_size or vram_limit ? */
		value[0] = screen->device->vram_size >> 20;
		return 0;
	case __DRI2_RENDERER_UNIFIED_MEMORY_ARCHITECTURE:
		value[0] = 0;
		return 0;
	default:
		return driQueryRendererIntegerCommon(psp, param, value);
	}
}

static int
nouveau_query_renderer_string(__DRIscreen *psp, int param, const char **value)
{
	const struct nouveau_screen *const screen =
		(struct nouveau_screen *) psp->driverPrivate;

	switch (param) {
	case __DRI2_RENDERER_VENDOR_ID:
		value[0] = nouveau_vendor_string;
		return 0;
	case __DRI2_RENDERER_DEVICE_ID:
		value[0] = nouveau_get_renderer_string(screen->device->chipset);
		return 0;
	default:
		return -1;
   }
}

static const __DRI2rendererQueryExtension nouveau_renderer_query_extension = {
	.base = { __DRI2_RENDERER_QUERY, 1 },

	.queryInteger        = nouveau_query_renderer_integer,
	.queryString         = nouveau_query_renderer_string
};

static void
nouveau_destroy_screen(__DRIscreen *dri_screen)
{
	struct nouveau_screen *screen = dri_screen->driverPrivate;

	if (!screen)
		return;

	nouveau_device_del(&screen->device);

	free(screen);
	dri_screen->driverPrivate = NULL;
}

static GLboolean
nouveau_create_buffer(__DRIscreen *dri_screen,
		      __DRIdrawable *drawable,
		      const struct gl_config *visual,
		      GLboolean is_pixmap)
{
	struct gl_renderbuffer *rb;
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
	_swrast_add_soft_renderbuffers(fb, GL_FALSE, GL_FALSE, GL_FALSE,
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

static void
nouveau_drawable_flush(__DRIdrawable *draw)
{
}

static const struct __DRI2flushExtensionRec nouveau_flush_extension = {
   .base = { __DRI2_FLUSH, 3 },

   .flush               = nouveau_drawable_flush,
   .invalidate          = dri2InvalidateDrawable,
};

static const struct __DRItexBufferExtensionRec nouveau_texbuffer_extension = {
   .base = { __DRI_TEX_BUFFER, 3 },

   .setTexBuffer        = NULL,
   .setTexBuffer2       = nouveau_set_texbuffer,
   .releaseTexBuffer    = NULL,
};

static const __DRIextension *nouveau_screen_extensions[] = {
    &nouveau_flush_extension.base,
    &nouveau_texbuffer_extension.base,
    &nouveau_renderer_query_extension.base,
    &dri2ConfigQueryExtension.base,
    NULL
};

const struct __DriverAPIRec nouveau_driver_api = {
	.InitScreen      = nouveau_init_screen2,
	.DestroyScreen   = nouveau_destroy_screen,
	.CreateBuffer    = nouveau_create_buffer,
	.DestroyBuffer   = nouveau_destroy_buffer,
	.CreateContext   = nouveau_context_create,
	.DestroyContext  = nouveau_context_destroy,
	.MakeCurrent     = nouveau_context_make_current,
	.UnbindContext   = nouveau_context_unbind,
};

static const struct __DRIDriverVtableExtensionRec nouveau_vtable = {
   .base = { __DRI_DRIVER_VTABLE, 1 },
   .vtable = &nouveau_driver_api,
};

/* This is the table of extensions that the loader will dlsym() for. */
static const __DRIextension *nouveau_driver_extensions[] = {
	&driCoreExtension.base,
	&driDRI2Extension.base,
	&nouveau_vtable.base,
	NULL
};

PUBLIC const __DRIextension **__driDriverGetExtensions_nouveau_vieux(void)
{
   globalDriverAPI = &nouveau_driver_api;

   return nouveau_driver_extensions;
}
