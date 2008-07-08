/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "utils.h"

#include "state_tracker/st_public.h"
#include "i915simple/i915_screen.h"

#include "intel_context.h"
#include "intel_device.h"
#include "intel_batchbuffer.h"
#include "intel_egl.h"


extern const struct dri_extension card_extensions[];


int
intel_create_device(struct egl_drm_device *device)
{
	struct intel_device *intel_device;

	/* Allocate the private area */
	intel_device = CALLOC_STRUCT(intel_device);
	if (!intel_device)
		return FALSE;

	device->priv = (void *)intel_device;
	intel_device->device = device;

	intel_device->deviceID = device->deviceID;

	intel_be_init_device(&intel_device->base, device->drmFD, intel_device->deviceID);

	intel_device->pipe = i915_create_screen(&intel_device->base.base, intel_device->deviceID);

	/* hack */
	driInitExtensions(NULL, card_extensions, GL_FALSE);

	return TRUE;
}

int
intel_destroy_device(struct egl_drm_device *device)
{
	struct intel_device *intel_device = (struct intel_device *)device->priv;
	
	intel_be_destroy_device(&intel_device->base);

	free(intel_device);
	device->priv = NULL;

	return TRUE;
}

int
intel_create_drawable(struct egl_drm_drawable *drawable,
                      const __GLcontextModes * visual)
{
	enum pipe_format colorFormat, depthFormat, stencilFormat;
	struct intel_framebuffer *intelfb = CALLOC_STRUCT(intel_framebuffer);

	if (!intelfb)
		return GL_FALSE;

	intelfb->device = drawable->device->priv;

	if (visual->redBits == 5)
		colorFormat = PIPE_FORMAT_R5G6B5_UNORM;
	else
		colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

	if (visual->depthBits == 16)
		depthFormat = PIPE_FORMAT_Z16_UNORM;
	else if (visual->depthBits == 24)
		depthFormat = PIPE_FORMAT_S8Z24_UNORM;
	else
		depthFormat = PIPE_FORMAT_NONE;

	if (visual->stencilBits == 8)
		stencilFormat = PIPE_FORMAT_S8Z24_UNORM;
	else
		stencilFormat = PIPE_FORMAT_NONE;

	intelfb->stfb = st_create_framebuffer(visual,
			colorFormat,
			depthFormat,
			stencilFormat,
			drawable->w,
			drawable->h,
			(void*) intelfb);

	if (!intelfb->stfb) {
		free(intelfb);
		return GL_FALSE;
	}

	drawable->priv = (void *) intelfb;
	return GL_TRUE;
}

int
intel_destroy_drawable(struct egl_drm_drawable *drawable)
{
	struct intel_framebuffer *intelfb = (struct intel_framebuffer *)drawable->priv;
	drawable->priv = NULL;

	assert(intelfb->stfb);
	st_unreference_framebuffer(&intelfb->stfb);
	free(intelfb);
	return TRUE;
}
