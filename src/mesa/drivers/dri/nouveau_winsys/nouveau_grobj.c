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
nouveau_grobj_alloc(struct nouveau_channel *userchan, uint32_t handle,
		    int class, struct nouveau_grobj **usergrobj)
{
	struct nouveau_device_priv *nv = nouveau_device(userchan->device);
	struct nouveau_grobj_priv *gr;
	struct drm_nouveau_grobj_alloc g;
	int ret;

	if (!nv || !usergrobj || *usergrobj)
		return -EINVAL;

	gr = calloc(1, sizeof(*gr));
	if (!gr)
		return -ENOMEM;
	gr->base.channel = userchan;
	gr->base.handle  = handle;
	gr->base.grclass = class;

	g.channel = userchan->id;
	g.handle  = handle;
	g.class   = class;
	ret = drmCommandWrite(nv->fd, DRM_NOUVEAU_GROBJ_ALLOC, &g, sizeof(g));
	if (ret) {
		nouveau_grobj_free((void *)&gr);
		return ret;
	}

	*usergrobj = &gr->base;
	return 0;
}

int
nouveau_grobj_ref(struct nouveau_channel *userchan, uint32_t handle,
		  struct nouveau_grobj **usergr)
{
	struct nouveau_grobj_priv *gr;

	if (!userchan || !usergr || *usergr)
		return -EINVAL;

	gr = calloc(1, sizeof(*gr));
	if (!gr)
		return -ENOMEM;
	gr->base.channel = userchan;
	gr->base.handle = handle;
	gr->base.grclass = 0;

	*usergr = &gr->base;
	return 0;
}

void
nouveau_grobj_free(struct nouveau_grobj **usergrobj)
{
	struct nouveau_grobj_priv *gr;

	if (!usergrobj)
		return;
	gr = nouveau_grobj(*usergrobj);
	*usergrobj = NULL;

	if (gr) {
		struct nouveau_channel_priv *chan;
		struct nouveau_device_priv *nv;
		struct drm_nouveau_gpuobj_free f;

		chan = nouveau_channel(gr->base.channel);
		nv   = nouveau_device(chan->base.device);

		if (gr->base.grclass) {
			f.channel = chan->drm.channel;
			f.handle  = gr->base.handle;
			drmCommandWrite(nv->fd, DRM_NOUVEAU_GPUOBJ_FREE,
					&f, sizeof(f));	
		}
		free(gr);
	}
}

