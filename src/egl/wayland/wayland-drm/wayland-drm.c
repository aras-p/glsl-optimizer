/*
 * Copyright © 2011 Kristian Høgsberg
 * Copyright © 2011 Benjamin Franzke
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

struct wl_drm {
	struct wl_object object;
	struct wl_display *display;

	void *user_data;
	char *device_name;

	struct wayland_drm_callbacks *callbacks;
};

struct wl_drm_buffer {
	struct wl_buffer buffer;
	struct wl_drm *drm;
	
	void *driver_buffer;
};

static void
buffer_damage(struct wl_client *client, struct wl_buffer *buffer,
	      int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void
destroy_buffer(struct wl_resource *resource, struct wl_client *client)
{
	struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) resource;
	struct wl_drm *drm = buffer->drm;

	drm->callbacks->release_buffer(drm->user_data,
				       buffer->driver_buffer);
	free(buffer);
}

static void
buffer_destroy(struct wl_client *client, struct wl_buffer *buffer)
{
	wl_resource_destroy(&buffer->resource, client, 0);
}

const static struct wl_buffer_interface drm_buffer_interface = {
	buffer_damage,
	buffer_destroy
};

static void
drm_create_buffer(struct wl_client *client, struct wl_drm *drm,
		  uint32_t id, uint32_t name, int32_t width, int32_t height,
		  uint32_t stride, struct wl_visual *visual)
{
	struct wl_drm_buffer *buffer;

	buffer = calloc(1, sizeof *buffer);
	if (buffer == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	buffer->drm = drm;
	buffer->buffer.width = width;
	buffer->buffer.height = height;
	buffer->buffer.visual = visual;
	buffer->buffer.client = client;

	if (!visual || visual->object.interface != &wl_visual_interface) {
		wl_client_post_error(client, &drm->object,
				     WL_DRM_ERROR_INVALID_VISUAL,
				     "invalid visual");
		return;
	}

	buffer->driver_buffer =
		drm->callbacks->reference_buffer(drm->user_data, name,
						 width, height,
						 stride, visual);

	if (buffer->driver_buffer == NULL) {
		wl_client_post_error(client, &drm->object,
				     WL_DRM_ERROR_INVALID_NAME,
				     "invalid name");
		return;
	}

	buffer->buffer.resource.object.id = id;
	buffer->buffer.resource.object.interface = &wl_buffer_interface;
	buffer->buffer.resource.object.implementation = (void (**)(void))
		&drm_buffer_interface;

	buffer->buffer.resource.destroy = destroy_buffer;

	wl_client_add_resource(client, &buffer->buffer.resource);
}

static void
drm_authenticate(struct wl_client *client,
		 struct wl_drm *drm, uint32_t id)
{
	if (drm->callbacks->authenticate(drm->user_data, id) < 0)
		wl_client_post_error(client, &drm->object,
				     WL_DRM_ERROR_AUTHENTICATE_FAIL,
				     "authenicate failed");
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
		struct wl_object *global, uint32_t version)
{
	struct wl_drm *drm = (struct wl_drm *) global;

	wl_client_post_event(client, global, WL_DRM_DEVICE, drm->device_name);
}

struct wl_drm *
wayland_drm_init(struct wl_display *display, char *device_name,
                 struct wayland_drm_callbacks *callbacks, void *user_data)
{
	struct wl_drm *drm;

	drm = malloc(sizeof *drm);

	drm->display = display;
	drm->device_name = strdup(device_name);
	drm->callbacks = callbacks;
	drm->user_data = user_data;

	drm->object.interface = &wl_drm_interface;
	drm->object.implementation = (void (**)(void)) &drm_interface;
	wl_display_add_object(display, &drm->object);
	wl_display_add_global(display, &drm->object, post_drm_device);

	return drm;
}

void
wayland_drm_uninit(struct wl_drm *drm)
{
	free(drm->device_name);

	/* FIXME: need wl_display_del_{object,global} */

	free(drm);
}

int
wayland_buffer_is_drm(struct wl_buffer *buffer)
{
	return buffer->resource.object.implementation == 
		(void (**)(void)) &drm_buffer_interface;
}

void *
wayland_drm_buffer_get_buffer(struct wl_buffer *buffer_base)
{
	struct wl_drm_buffer *buffer = (struct wl_drm_buffer *) buffer_base;

	return buffer->driver_buffer;
}
