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
#include <string.h>
#include <errno.h>

#include "nouveau_drmif.h"
#include "nouveau_dma.h"

int
nouveau_channel_alloc(struct nouveau_device *dev, uint32_t fb_ctxdma,
		      uint32_t tt_ctxdma, struct nouveau_channel **chan)
{
	struct nouveau_device_priv *nvdev = nouveau_device(dev);
	struct nouveau_channel_priv *nvchan;
	int ret;

	if (!nvdev || !chan || *chan)
	    return -EINVAL;

	nvchan = calloc(1, sizeof(struct nouveau_channel_priv));
	if (!nvchan)
		return -ENOMEM;
	nvchan->base.device = dev;

	nvchan->drm.fb_ctxdma_handle = fb_ctxdma;
	nvchan->drm.tt_ctxdma_handle = tt_ctxdma;
	ret = drmCommandWriteRead(nvdev->fd, DRM_NOUVEAU_CHANNEL_ALLOC,
				  &nvchan->drm, sizeof(nvchan->drm));
	if (ret) {
		free(nvchan);
		return ret;
	}

	nvchan->base.id = nvchan->drm.channel;
	if (nouveau_grobj_ref(&nvchan->base, nvchan->drm.fb_ctxdma_handle,
			      &nvchan->base.vram) ||
	    nouveau_grobj_ref(&nvchan->base, nvchan->drm.tt_ctxdma_handle,
		    	      &nvchan->base.gart)) {
		nouveau_channel_free((void *)&nvchan);
		return -EINVAL;
	}

	ret = drmMap(nvdev->fd, nvchan->drm.ctrl, nvchan->drm.ctrl_size,
		     (void*)&nvchan->user);
	if (ret) {
		nouveau_channel_free((void *)&nvchan);
		return ret;
	}
	nvchan->put     = &nvchan->user[0x40/4];
	nvchan->get     = &nvchan->user[0x44/4];
	nvchan->ref_cnt = &nvchan->user[0x48/4];

	ret = drmMap(nvdev->fd, nvchan->drm.notifier, nvchan->drm.notifier_size,
		     (drmAddressPtr)&nvchan->notifier_block);
	if (ret) {
		nouveau_channel_free((void *)&nvchan);
		return ret;
	}

	ret = drmMap(nvdev->fd, nvchan->drm.cmdbuf, nvchan->drm.cmdbuf_size,
		     (void*)&nvchan->pushbuf);
	if (ret) {
		nouveau_channel_free((void *)&nvchan);
		return ret;
	}

	ret = nouveau_grobj_alloc(&nvchan->base, 0x00000000, 0x0030,
				  &nvchan->base.nullobj);
	if (ret) {
		nouveau_channel_free((void *)&nvchan);
		return ret;
	}

	nouveau_dma_channel_init(&nvchan->base);
	nouveau_pushbuf_init(&nvchan->base);

	*chan = &nvchan->base;
	return 0;
}

void
nouveau_channel_free(struct nouveau_channel **chan)
{
	struct nouveau_channel_priv *nvchan;
	struct nouveau_device_priv *nvdev;
	struct drm_nouveau_channel_free cf;

	if (!chan || !*chan)
		return;
	nvchan = nouveau_channel(*chan);
	*chan = NULL;
	nvdev = nouveau_device(nvchan->base.device);
	
	FIRE_RING_CH(&nvchan->base);

	nouveau_grobj_free(&nvchan->base.vram);
	nouveau_grobj_free(&nvchan->base.gart);
	nouveau_grobj_free(&nvchan->base.nullobj);

	cf.channel = nvchan->drm.channel;
	drmCommandWrite(nvdev->fd, DRM_NOUVEAU_CHANNEL_FREE, &cf, sizeof(cf));
	free(nvchan);
}


