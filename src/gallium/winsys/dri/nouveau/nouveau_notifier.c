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

#include <stdlib.h>
#include <errno.h>

#include "nouveau_drmif.h"
#include "nouveau_local.h"

#define NOTIFIER(__v)                                                          \
	struct nouveau_notifier_priv *nvnotify = nouveau_notifier(notifier);   \
	volatile uint32_t *__v = (void*)nvnotify->map + (id * 32)

int
nouveau_notifier_alloc(struct nouveau_channel *chan, uint32_t handle,
		       int count, struct nouveau_notifier **notifier)
{
	struct nouveau_notifier_priv *nvnotify;
	int ret;

	if (!chan || !notifier || *notifier)
		return -EINVAL;

	nvnotify = calloc(1, sizeof(struct nouveau_notifier_priv));
	if (!nvnotify)
		return -ENOMEM;
	nvnotify->base.channel = chan;
	nvnotify->base.handle  = handle;

	nvnotify->drm.channel = chan->id;
	nvnotify->drm.handle  = handle;
	nvnotify->drm.count   = count;
	if ((ret = drmCommandWriteRead(nouveau_device(chan->device)->fd,
				       DRM_NOUVEAU_NOTIFIEROBJ_ALLOC,
				       &nvnotify->drm,
				       sizeof(nvnotify->drm)))) {
		nouveau_notifier_free((void *)&nvnotify);
		return ret;
	}

	nvnotify->map = (void *)nouveau_channel(chan)->notifier_block +
				nvnotify->drm.offset;
	*notifier = &nvnotify->base;
	return 0;
}

void
nouveau_notifier_free(struct nouveau_notifier **notifier)
{

	struct nouveau_notifier_priv *nvnotify;
	struct nouveau_channel_priv *nvchan;
	struct nouveau_device_priv *nvdev;
	struct drm_nouveau_gpuobj_free f;

	if (!notifier || !*notifier)
		return;
	nvnotify = nouveau_notifier(*notifier);
	*notifier = NULL;

	nvchan = nouveau_channel(nvnotify->base.channel);
	nvdev   = nouveau_device(nvchan->base.device);

	f.channel = nvchan->drm.channel;
	f.handle  = nvnotify->base.handle;
	drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GPUOBJ_FREE, &f, sizeof(f));		
	free(nvnotify);
}

void
nouveau_notifier_reset(struct nouveau_notifier *notifier, int id)
{
	NOTIFIER(n);

	n[NV_NOTIFY_TIME_0      /4] = 0x00000000;
	n[NV_NOTIFY_TIME_1      /4] = 0x00000000;
	n[NV_NOTIFY_RETURN_VALUE/4] = 0x00000000;
	n[NV_NOTIFY_STATE       /4] = (NV_NOTIFY_STATE_STATUS_IN_PROCESS <<
				       NV_NOTIFY_STATE_STATUS_SHIFT);
}

uint32_t
nouveau_notifier_status(struct nouveau_notifier *notifier, int id)
{
	NOTIFIER(n);

	return n[NV_NOTIFY_STATE/4] >> NV_NOTIFY_STATE_STATUS_SHIFT;
}

uint32_t
nouveau_notifier_return_val(struct nouveau_notifier *notifier, int id)
{
	NOTIFIER(n);

	return n[NV_NOTIFY_RETURN_VALUE/4];
}

int
nouveau_notifier_wait_status(struct nouveau_notifier *notifier, int id,
			     int status, int timeout)
{
	NOTIFIER(n);
	uint32_t time = 0, t_start = NOUVEAU_TIME_MSEC();

	while (time <= timeout) {
		uint32_t v;

		v = n[NV_NOTIFY_STATE/4] >> NV_NOTIFY_STATE_STATUS_SHIFT;
		if (v == status)
			return 0;

		if (timeout)
			time = NOUVEAU_TIME_MSEC() - t_start;
	}

	return -EBUSY;
}

