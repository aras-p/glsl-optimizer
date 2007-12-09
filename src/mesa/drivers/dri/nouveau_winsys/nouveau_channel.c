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
nouveau_channel_alloc(struct nouveau_device *userdev, uint32_t fb_ctxdma,
		      uint32_t tt_ctxdma, struct nouveau_channel **userchan)
{
	struct nouveau_device_priv *nv = nouveau_device(userdev);
	struct nouveau_channel_priv *chan;
	int ret;

	if (!nv || !userchan || *userchan)
	    return -EINVAL;

	chan = calloc(1, sizeof(*chan));
	if (!chan)
		return -ENOMEM;
	chan->base.device = userdev;

	chan->drm.fb_ctxdma_handle = fb_ctxdma;
	chan->drm.tt_ctxdma_handle = tt_ctxdma;
	ret = drmCommandWriteRead(nv->fd, DRM_NOUVEAU_CHANNEL_ALLOC,
				  &chan->drm, sizeof(chan->drm));
	if (ret) {
		free(chan);
		return ret;
	}

	chan->base.id = chan->drm.channel;
	if (nouveau_grobj_ref(&chan->base, chan->drm.fb_ctxdma_handle,
			      &chan->base.vram) ||
	    nouveau_grobj_ref(&chan->base, chan->drm.tt_ctxdma_handle,
		    	      &chan->base.gart)) {
		nouveau_channel_free((void *)&chan);
		return -EINVAL;
	}

	ret = drmMap(nv->fd, chan->drm.ctrl, chan->drm.ctrl_size,
		     (void*)&chan->user);
	if (ret) {
		nouveau_channel_free((void *)&chan);
		return ret;
	}
	chan->put     = &chan->user[0x40/4];
	chan->get     = &chan->user[0x44/4];
	chan->ref_cnt = &chan->user[0x48/4];

	ret = drmMap(nv->fd, chan->drm.notifier, chan->drm.notifier_size,
		     (drmAddressPtr)&chan->notifier_block);
	if (ret) {
		nouveau_channel_free((void *)&chan);
		return ret;
	}

	ret = drmMap(nv->fd, chan->drm.cmdbuf, chan->drm.cmdbuf_size,
		     (void*)&chan->pushbuf);
	if (ret) {
		nouveau_channel_free((void *)&chan);
		return ret;
	}

	nouveau_dma_channel_init(&chan->base);
	nouveau_pushbuf_init(&chan->base);

	*userchan = &chan->base;
	return 0;
}

void
nouveau_channel_free(struct nouveau_channel **userchan)
{
	struct nouveau_channel_priv *chan;
	
	if (!userchan)
		return;
	chan = nouveau_channel(*userchan);

	if (chan) {
		struct nouveau_device_priv *nv;
		struct drm_nouveau_channel_free cf;

		nv = nouveau_device((*userchan)->device);
		*userchan = NULL;

		FIRE_RING_CH(&chan->base);

		nouveau_grobj_free(&chan->base.vram);
		nouveau_grobj_free(&chan->base.gart);

		cf.channel = chan->drm.channel;
		drmCommandWrite(nv->fd, DRM_NOUVEAU_CHANNEL_FREE,
				&cf, sizeof(cf));
		free(chan);
	}
}


