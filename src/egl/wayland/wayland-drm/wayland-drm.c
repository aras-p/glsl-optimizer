/*
 * Copyright © 2011 Kristian Høgsberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 *    Benjamin Franzke <benjaminfranzke@googlemail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <wayland-server.h>
#include "wayland-drm.h"
#include "wayland-drm-server-protocol.h"

#include "egldisplay.h"
#include "egldriver.h"
#include "eglimage.h"
#include "egltypedefs.h"

struct wl_drm {
	struct wl_object object;
	struct wl_display *display;

	_EGLDisplay *edisp;

	char *device_name;
	authenticate_t authenticate;
};

static void
drm_buffer_damage(struct wl_buffer *buffer_base,
		  struct wl_surface *surface,
		  int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void
destroy_buffer(struct wl_resource *resource, struct wl_client *client)
{
	struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) resource;
	_EGLDriver *drv = buffer->drm->edisp->Driver;

	drv->API.DestroyImageKHR(drv, buffer->drm->edisp, buffer->image);
	free(buffer);
}

static void
buffer_destroy(struct wl_client *client, struct wl_buffer *buffer)
{
	wl_resource_destroy(&buffer->resource, client);
}

const static struct wl_buffer_interface buffer_interface = {
	buffer_destroy
};

static void
drm_create_buffer(struct wl_client *client, struct wl_drm *drm,
		  uint32_t id, uint32_t name, int32_t width, int32_t height,
		  uint32_t stride, struct wl_visual *visual)
{
	struct wl_drm_buffer *buffer;
	EGLint attribs[] = {
		EGL_WIDTH,		0,
		EGL_HEIGHT,		0,
		EGL_DRM_BUFFER_STRIDE_MESA,	0,
		EGL_DRM_BUFFER_FORMAT_MESA,	EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
		EGL_NONE
	};
	_EGLDriver *drv = drm->edisp->Driver;

	buffer = malloc(sizeof *buffer);
	if (buffer == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	buffer->drm = drm;
	buffer->buffer.compositor = NULL;
	buffer->buffer.width = width;
	buffer->buffer.height = height;
	buffer->buffer.visual = visual;
	buffer->buffer.attach = NULL;
	buffer->buffer.damage = drm_buffer_damage;

	if (visual->object.interface != &wl_visual_interface) {
		/* FIXME: Define a real exception event instead of
		 * abusing this one */
		wl_client_post_event(client,
				     (struct wl_object *) drm->display,
				     WL_DISPLAY_INVALID_OBJECT, 0);
		fprintf(stderr, "invalid visual in create_buffer\n");
		return;
	}

	attribs[1] = width;
	attribs[3] = height;
	attribs[5] = stride / 4;
	buffer->image = drv->API.CreateImageKHR(drv, drm->edisp,
						EGL_NO_CONTEXT,
						EGL_DRM_BUFFER_MESA,
						(EGLClientBuffer) (intptr_t) name,
						attribs);

	if (buffer->image == NULL) {
		/* FIXME: Define a real exception event instead of
		 * abusing this one */
		wl_client_post_event(client,
				     (struct wl_object *) drm->display,
				     WL_DISPLAY_INVALID_OBJECT, 0);
		fprintf(stderr, "failed to create image for name %d\n", name);
		return;
	}

	buffer->buffer.resource.object.id = id;
	buffer->buffer.resource.object.interface = &wl_buffer_interface;
	buffer->buffer.resource.object.implementation = (void (**)(void))
		&buffer_interface;

	buffer->buffer.resource.destroy = destroy_buffer;

	wl_client_add_resource(client, &buffer->buffer.resource);
}

static void
drm_authenticate(struct wl_client *client,
		 struct wl_drm *drm, uint32_t id)
{
	if (drm->authenticate(drm->edisp, id) < 0)
		wl_client_post_event(client,
				     (struct wl_object *) drm->display,
				     WL_DISPLAY_INVALID_OBJECT, 0);
	else
		wl_client_post_event(client, &drm->object,
				     WL_DRM_AUTHENTICATED);
}

const static struct wl_drm_interface drm_interface = {
	drm_authenticate,
	drm_create_buffer
};

static void
post_drm_device(struct wl_client *client,
		struct wl_object *global,
		uint32_t version)
{
	struct wl_drm *drm = (struct wl_drm *) global;

	wl_client_post_event(client, global, WL_DRM_DEVICE, drm->device_name);
}

struct wl_drm *
wayland_drm_init(struct wl_display *display, _EGLDisplay *disp,
		 authenticate_t authenticate, char *device_name)
{
	struct wl_drm *drm;

	drm = malloc(sizeof *drm);

	drm->display = display;
	drm->edisp = disp;
	drm->authenticate = authenticate;
	drm->device_name = strdup(device_name);

	drm->object.interface = &wl_drm_interface;
	drm->object.implementation = (void (**)(void)) &drm_interface;
	wl_display_add_object(display, &drm->object);
	wl_display_add_global(display, &drm->object, post_drm_device);

	return drm;
}

void
wayland_drm_destroy(struct wl_drm *drm)
{
	free(drm->device_name);

	/* FIXME: need wl_display_del_{object,global} */

	free(drm);
}
