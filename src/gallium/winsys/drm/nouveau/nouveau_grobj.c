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

int
nouveau_grobj_alloc(struct nouveau_channel *chan, uint32_t handle,
		    int class, struct nouveau_grobj **grobj)
{
	struct nouveau_device_priv *nvdev = nouveau_device(chan->device);
	struct nouveau_grobj_priv *nvgrobj;
	struct drm_nouveau_grobj_alloc g;
	int ret;

	if (!nvdev || !grobj || *grobj)
		return -EINVAL;

	nvgrobj = calloc(1, sizeof(*nvgrobj));
	if (!nvgrobj)
		return -ENOMEM;
	nvgrobj->base.channel = chan;
	nvgrobj->base.handle  = handle;
	nvgrobj->base.grclass = class;

	g.channel = chan->id;
	g.handle  = handle;
	g.class   = class;
	ret = drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GROBJ_ALLOC,
			      &g, sizeof(g));
	if (ret) {
		nouveau_grobj_free((void *)&nvgrobj);
		return ret;
	}

	*grobj = &nvgrobj->base;
	return 0;
}

int
nouveau_grobj_ref(struct nouveau_channel *chan, uint32_t handle,
		  struct nouveau_grobj **grobj)
{
	struct nouveau_grobj_priv *nvgrobj;

	if (!chan || !grobj || *grobj)
		return -EINVAL;

	nvgrobj = calloc(1, sizeof(struct nouveau_grobj_priv));
	if (!nvgrobj)
		return -ENOMEM;
	nvgrobj->base.channel = chan;
	nvgrobj->base.handle = handle;
	nvgrobj->base.grclass = 0;

	*grobj = &nvgrobj->base;
	return 0;
}

void
nouveau_grobj_free(struct nouveau_grobj **grobj)
{
	struct nouveau_device_priv *nvdev;
	struct nouveau_channel_priv *chan;
	struct nouveau_grobj_priv *nvgrobj;

	if (!grobj || !*grobj)
		return;
	nvgrobj = nouveau_grobj(*grobj);
	*grobj = NULL;


	chan = nouveau_channel(nvgrobj->base.channel);
	nvdev = nouveau_device(chan->base.device);

	if (nvgrobj->base.grclass) {
		struct drm_nouveau_gpuobj_free f;

		f.channel = chan->drm.channel;
		f.handle  = nvgrobj->base.handle;
		drmCommandWrite(nvdev->fd, DRM_NOUVEAU_GPUOBJ_FREE,
				&f, sizeof(f));	
	}
	free(nvgrobj);
}

