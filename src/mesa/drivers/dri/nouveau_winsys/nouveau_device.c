/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "nouveau_drmif.h"

int
nouveau_device_open_existing(struct nouveau_device **userdev, int close,
			     int fd, drm_context_t ctx)
{
	struct nouveau_device_priv *nv;
	int ret;

	if (!userdev || *userdev)
	    return -EINVAL;

	nv = calloc(1, sizeof(*nv));
	if (!nv)
	    return -ENOMEM;
	nv->fd = fd;
	nv->ctx = ctx;
	nv->needs_close = close;

	drmCommandNone(nv->fd, DRM_NOUVEAU_CARD_INIT);

	if ((ret = nouveau_bo_init(&nv->base))) {
		nouveau_device_close((void *)&nv);
		return ret;
	}

	*userdev = &nv->base;
	return 0;
}

int
nouveau_device_open(struct nouveau_device **userdev, const char *busid)
{
	drm_context_t ctx;
	int fd, ret;

	if (!userdev || *userdev)
		return -EINVAL;

	fd = drmOpen("nouveau", busid);
	if (fd < 0)
		return -EINVAL;

	ret = drmCreateContext(fd, &ctx);
	if (ret) {
		drmClose(fd);
		return ret;
	}

	ret = nouveau_device_open_existing(userdev, 1, fd, ctx);
	if (ret) {
	    drmDestroyContext(fd, ctx);
	    drmClose(fd);
	    return ret;
	}

	return 0;
}

void
nouveau_device_close(struct nouveau_device **userdev)
{
	struct nouveau_device_priv *nv;

	if (userdev || !*userdev)
		return;
	nv = (struct nouveau_device_priv *)*userdev;
	*userdev = NULL;

	nouveau_bo_takedown(&nv->base);

	if (nv->needs_close) {
		drmDestroyContext(nv->fd, nv->ctx);
		drmClose(nv->fd);
	}
	free(nv);
}

int
nouveau_device_get_param(struct nouveau_device *userdev,
			 uint64_t param, uint64_t *value)
{
	struct nouveau_device_priv *nv = (struct nouveau_device_priv *)userdev;
	struct drm_nouveau_getparam g;
	int ret;

	if (!nv || !value)
		return -EINVAL;

	g.param = param;
	ret = drmCommandWriteRead(nv->fd, DRM_NOUVEAU_GETPARAM, &g, sizeof(g));
	if (ret)
		return ret;

	*value = g.value;
	return 0;
}

int
nouveau_device_set_param(struct nouveau_device *userdev,
			 uint64_t param, uint64_t value)
{
	struct nouveau_device_priv *nv = (struct nouveau_device_priv *)userdev;
	struct drm_nouveau_setparam s;
	int ret;

	if (!nv)
		return -EINVAL;

	s.param = param;
	s.value = value;
	ret = drmCommandWriteRead(nv->fd, DRM_NOUVEAU_SETPARAM, &s, sizeof(s));
	if (ret)
		return ret;

	return 0;
}

