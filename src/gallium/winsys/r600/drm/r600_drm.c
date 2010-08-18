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
#include "util/u_inlines.h"
#include "util/u_debug.h"
#include "radeon_priv.h"
#include "r600_screen.h"
#include "r600_resource.h"
#include "r600_drm_public.h"
#include "state_tracker/drm_driver.h"

struct radeon *r600_drm_winsys_create(int drmfd)
{
	return radeon_new(drmfd, 0);
}

boolean r600_buffer_get_handle(struct radeon *rw,
			       struct pipe_resource *buf,
			       struct winsys_handle *whandle)
{
	struct drm_gem_flink flink;
	struct r600_resource* rbuffer = (struct r600_resource*)buf;

	if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) {
		if (!rbuffer->flink) {
			flink.handle = rbuffer->bo->handle;

			if (ioctl(rw->fd, DRM_IOCTL_GEM_FLINK, &flink)) {
				return FALSE;
			}

			rbuffer->flink = flink.name;
		}
		whandle->handle = rbuffer->flink;
	} else if (whandle->type == DRM_API_HANDLE_TYPE_KMS) {
		whandle->handle = rbuffer->bo->handle;
	}
	return TRUE;
}
