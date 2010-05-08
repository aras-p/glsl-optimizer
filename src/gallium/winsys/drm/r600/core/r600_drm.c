/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 *      Joakim Sindholt <opensource@zhasha.com>
 */
#include <sys/ioctl.h>
#include "trace/tr_drm.h"
#include "util/u_inlines.h"
#include "util/u_debug.h"
#include "state_tracker/drm_api.h"
#include "radeon_priv.h"
#include "r600_screen.h"
#include "r600_texture.h"

static struct pipe_screen *r600_drm_create_screen(struct drm_api* api, int drmfd,
					struct drm_create_screen_arg *arg)
{
	struct radeon *rw = radeon_new(drmfd, 0);

	if (rw == NULL)
		return NULL;
	return radeon_create_screen(rw);
}

static boolean r600_buffer_from_texture(struct drm_api *api,
				struct pipe_screen *screen,
				struct pipe_texture *texture,
				struct pipe_buffer **buffer,
				unsigned *stride)
{
	return r600_get_texture_buffer(screen, texture, buffer, stride);
}

static struct pipe_buffer *r600_buffer_from_handle(struct drm_api *api,
						struct pipe_screen *screen,
						const char *name,
						unsigned handle)
{
	struct radeon *rw = (struct radeon*)screen->winsys;
	struct r600_pipe_buffer *rbuffer;
	struct radeon_bo *bo = NULL;

	bo = radeon_bo(rw, handle, 0, 0, NULL);
	if (bo == NULL) {
		return NULL;
	}

	rbuffer = calloc(1, sizeof(struct r600_pipe_buffer));
	if (rbuffer == NULL) {
		radeon_bo_decref(rw, bo);
		return NULL;
	}

	pipe_reference_init(&rbuffer->base.reference, 1);
	rbuffer->base.screen = screen;
	rbuffer->base.usage = PIPE_BUFFER_USAGE_PIXEL;
	rbuffer->bo = bo;
	return &rbuffer->base;
}

static struct pipe_texture *r600_texture_from_shared_handle(struct drm_api *api,
						struct pipe_screen *screen,
						struct pipe_texture *templ,
						const char *name,
						unsigned stride,
						unsigned handle)
{
	struct pipe_buffer *buffer;
	struct pipe_texture *blanket;


	buffer = r600_buffer_from_handle(api, screen, name, handle);
	if (buffer == NULL) {
		return NULL;
	}
	blanket = screen->texture_blanket(screen, templ, &stride, buffer);
	pipe_buffer_reference(&buffer, NULL);
	return blanket;
}

static boolean r600_shared_handle_from_texture(struct drm_api *api,
						struct pipe_screen *screen,
						struct pipe_texture *texture,
						unsigned *stride,
						unsigned *handle)
{
	struct radeon *rw = (struct radeon*)screen->winsys;
	struct drm_gem_flink flink;
	struct r600_pipe_buffer* rbuffer;
	struct pipe_buffer *buffer = NULL;
	int r;

	if (!r600_buffer_from_texture(api, screen, texture, &buffer, stride)) {
		return FALSE;
	}

	rbuffer = (struct r600_pipe_buffer*)buffer;
	if (!rbuffer->flink) {
		flink.handle = rbuffer->bo->handle;
		r = ioctl(rw->fd, DRM_IOCTL_GEM_FLINK, &flink);
		if (r) {
			return FALSE;
		}
		rbuffer->flink = flink.name;
	}
	*handle = rbuffer->flink;
	return TRUE;
}

static boolean r600_local_handle_from_texture(struct drm_api *api,
						struct pipe_screen *screen,
						struct pipe_texture *texture,
						unsigned *stride,
						unsigned *handle)
{
	struct pipe_buffer *buffer = NULL;

	if (!r600_buffer_from_texture(api, screen, texture, &buffer, stride)) {
		return FALSE;
	}
	*handle = ((struct r600_pipe_buffer*)buffer)->bo->handle;
	pipe_buffer_reference(&buffer, NULL);
	return TRUE;
}

static void r600_drm_api_destroy(struct drm_api *api)
{
	return;
}

struct drm_api drm_api_hooks = {
	.name = "r600",
	.driver_name = "r600",
	.create_screen = r600_drm_create_screen,
	.texture_from_shared_handle = r600_texture_from_shared_handle,
	.shared_handle_from_texture = r600_shared_handle_from_texture,
	.local_handle_from_texture = r600_local_handle_from_texture,
	.destroy = r600_drm_api_destroy,
};

struct drm_api* drm_api_create()
{
#ifdef DEBUG
	return trace_drm_create(&drm_api_hooks);
#else
	return &drm_api_hooks;
#endif
}
