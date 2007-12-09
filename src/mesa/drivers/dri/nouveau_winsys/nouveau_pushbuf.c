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
#include <assert.h>

#include "nouveau_drmif.h"
#include "nouveau_dma.h"

int
nouveau_pushbuf_init(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);

	if (!nvchan)
		return -EINVAL;

	/* Everything except first 4KiB of the push buffer is managed by us */
	if (nouveau_resource_init(&nvchan->pb_heap,
				  nvchan->drm.cmdbuf_size - 4096))
		return -EINVAL;

	/* Shrink master ring to 4KiB */
	assert(nvchan->dma.cur <= (4096/4));
	nvchan->dma.max = (4096 / 4) - 2;
	nvchan->dma.free = nvchan->dma.max - nvchan->dma.cur;

	return 0;
}

/* This would be our TTM "superioctl" */
int
nouveau_pushbuf_flush(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(nvchan->pb_tail);
	int i, ret;

	if (!nvpb)
		goto out_realloc;

	if (nvpb->base.remaining == nvpb->res->size / 4)
		return 0;

	ret = nouveau_fence_new(chan, &nvpb->fence);
	if (ret)
		return ret;

	/* Validate buffers */
	nouveau_bo_validate(chan);

	/* Apply relocations */
	if (nvchan->num_relocs) {		
		for (i = 0; i < nvchan->num_relocs; i++) {
			struct nouveau_bo_reloc *r = &nvchan->relocs[i];
			uint32_t push;

			if (r->flags & NOUVEAU_BO_LOW) {
				push = r->bo->base.offset + r->data;
			} else
			if (r->flags & NOUVEAU_BO_HIGH) {
				push = (r->bo->base.offset + r->data) >> 32;
			} else {
				push = r->data;
			}

			if (r->flags & NOUVEAU_BO_OR) {
				if (r->bo->base.flags & NOUVEAU_BO_VRAM)
					push |= r->vor;
				else
					push |= r->tor;
			}

			*r->ptr = push;
		}

		nvchan->num_relocs = 0;
	}

	/* Emit JMP to indirect pushbuf */
	if (nvchan->dma.free < 1)
		WAIT_RING_CH(chan, 1);
	nvchan->dma.free -= 1;
	OUT_RING_CH(chan, 0x20000000 | (nvpb->res->start + 4096));

	/* Add JMP back to master pushbuf from indirect pushbuf */
	(*nvpb->base.cur++) =
		0x20000000 | ((nvchan->dma.cur << 2) + nvchan->dma.base);

	/* Fence */
	nouveau_fence_emit(nvpb->fence);

	/* Kickoff */
	FIRE_RING_CH(chan);

	/* Allocate space for next push buffer */
out_realloc:
	nvpb = calloc(1, sizeof(struct nouveau_pushbuf_priv));
	if (!nvpb)
		return -ENOMEM;

	if (nouveau_resource_alloc(nvchan->pb_heap, 0x2000, NULL, &nvpb->res)) {
		struct nouveau_pushbuf_priv *e;
		int nr = 0;

		/* Update fences */
		nouveau_fence_flush(chan);

		/* Free any push buffers that have already been executed */
		e = nouveau_pushbuf(nvchan->pb_head);
		while (e && e->fence) {
			if (!e->fence || !nouveau_fence(e->fence)->signalled)
				break;
			nouveau_fence_del(&e->fence);
			nouveau_resource_free(&e->res);
			nr++;

			nvchan->pb_head = e->next;
			if (nvchan->pb_head == NULL)
				nvchan->pb_tail = NULL;
			free(e);
			e = nouveau_pushbuf(nvchan->pb_head);
		}

		/* We didn't free any buffers above.  As a last resort, busy
		 * wait on the oldest buffer becoming available.
		 */
		if (!nr) {
			e = nouveau_pushbuf(nvchan->pb_head);
			nouveau_fence_wait(&e->fence);
			nouveau_resource_free(&e->res);

			nvchan->pb_head = e->next;
			if (nvchan->pb_head == NULL)
				nvchan->pb_tail = NULL;
			free(e);
		}

		if (nouveau_resource_alloc(nvchan->pb_heap, 0x2000, nvpb,
					   &nvpb->res))
			assert(0);
	}

	nvpb->base.channel = chan;
	nvpb->base.remaining = nvpb->res->size / 4;
	nvpb->base.cur = &nvchan->pushbuf[(nvpb->res->start + 4096)/4];

	if (nvchan->pb_tail) {
		nouveau_pushbuf(nvchan->pb_tail)->next = & nvpb->base;
	} else {
		nvchan->pb_head = &nvpb->base;
	}
	nvchan->pb_tail = &nvpb->base;

	return 0;
}

