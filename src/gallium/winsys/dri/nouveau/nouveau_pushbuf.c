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

#define PB_BUFMGR_DWORDS   (4096 / 2)
#define PB_MIN_USER_DWORDS  2048

static int
nouveau_pushbuf_space(struct nouveau_channel *chan, unsigned min)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;

	assert((min + 1) <= nvchan->dma->max);

	/* Wait for enough space in push buffer */
	min = min < PB_MIN_USER_DWORDS ? PB_MIN_USER_DWORDS : min;
	min += 1; /* a bit extra for the NOP */
	if (nvchan->dma->free < min)
		WAIT_RING_CH(chan, min);

	/* Insert NOP, may turn into a jump later */
	RING_SPACE_CH(chan, 1);
	nvpb->nop_jump = nvchan->dma->cur;
	OUT_RING_CH(chan, 0);

	/* Any remaining space is available to the user */
	nvpb->start = nvchan->dma->cur;
	nvpb->size = nvchan->dma->free;
	nvpb->base.channel = chan;
	nvpb->base.remaining = nvpb->size;
	nvpb->base.cur = &nvchan->pushbuf[nvpb->start];

	/* Create a new fence object for this "frame" */
	nouveau_fence_ref(NULL, &nvpb->fence);
	nouveau_fence_new(chan, &nvpb->fence);

	return 0;
}

int
nouveau_pushbuf_init(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_dma_priv *m = &nvchan->dma_master;
	struct nouveau_dma_priv *b = &nvchan->dma_bufmgr;
	int i;

	if (!nvchan)
		return -EINVAL;

	/* Reassign last bit of push buffer for a "separate" bufmgr
	 * ring buffer
	 */
	m->max -= PB_BUFMGR_DWORDS;
	m->free -= PB_BUFMGR_DWORDS;

	b->base = m->base + ((m->max + 2) << 2);
	b->max = PB_BUFMGR_DWORDS - 2;
	b->cur = b->put = 0;
	b->free = b->max - b->cur;

	/* Some NOPs just to be safe
	 *XXX: RING_SKIPS
	 */
	nvchan->dma = b;
	RING_SPACE_CH(chan, 8);
	for (i = 0; i < 8; i++)
		OUT_RING_CH(chan, 0);
	nvchan->dma = m;

	nouveau_pushbuf_space(chan, 0);
	chan->pushbuf = &nvchan->pb.base;

	nvchan->pb.buffers = calloc(NOUVEAU_PUSHBUF_MAX_BUFFERS,
				    sizeof(struct nouveau_pushbuf_bo));
	nvchan->pb.relocs = calloc(NOUVEAU_PUSHBUF_MAX_RELOCS,
				   sizeof(struct nouveau_pushbuf_reloc));
	return 0;
}

static uint32_t
nouveau_pushbuf_calc_reloc(struct nouveau_bo *bo,
			   struct nouveau_pushbuf_reloc *r)
{
	uint32_t push;

	if (r->flags & NOUVEAU_BO_LOW) {
		push = bo->offset + r->data;
	} else
	if (r->flags & NOUVEAU_BO_HIGH) {
		push = (bo->offset + r->data) >> 32;
	} else {
		push = r->data;
	}

	if (r->flags & NOUVEAU_BO_OR) {
		if (bo->flags & NOUVEAU_BO_VRAM)
			push |= r->vor;
		else
			push |= r->tor;
	}

	return push;
}

/* This would be our TTM "superioctl" */
int
nouveau_pushbuf_flush(struct nouveau_channel *chan, unsigned min)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_pushbuf_priv *nvpb = &nvchan->pb;
	int ret, i;

	if (nvpb->base.remaining == nvpb->size)
		return 0;

	nouveau_fence_flush(chan);

	nvpb->size -= nvpb->base.remaining;
	nvchan->dma->cur += nvpb->size;
	nvchan->dma->free -= nvpb->size;
	assert(nvchan->dma->cur <= nvchan->dma->max);

	nvchan->dma = &nvchan->dma_bufmgr;
	nvchan->pushbuf[nvpb->nop_jump] = 0x20000000 |
		(nvchan->dma->base + (nvchan->dma->cur << 2));

	/* Validate buffers + apply relocations */
	nvchan->user_charge = 0;
	for (i = 0; i < nvpb->nr_relocs; i++) {
		struct nouveau_pushbuf_reloc *r = &nvpb->relocs[i];
		struct nouveau_pushbuf_bo *pbbo = r->pbbo;
		struct nouveau_bo *bo = pbbo->bo;

		/* Validated, mem matches presumed, no relocation necessary */
		if (pbbo->handled & 2) {
			if (!(pbbo->handled & 1))
				assert(0);
			continue;
		}

		/* Not yet validated, do it now */
		if (!(pbbo->handled & 1)) {
			ret = nouveau_bo_validate(chan, bo, pbbo->flags);
			if (ret) {
				assert(0);
				return ret;
			}
			pbbo->handled |= 1;

			if (bo->offset == nouveau_bo(bo)->offset &&
			    bo->flags == nouveau_bo(bo)->flags) {
				pbbo->handled |= 2;
				continue;
			}
			bo->offset = nouveau_bo(bo)->offset;
			bo->flags = nouveau_bo(bo)->flags;
		}

		/* Apply the relocation */
		*r->ptr = nouveau_pushbuf_calc_reloc(bo, r);
	}
	nvpb->nr_relocs = 0;

	/* Dereference all buffers on validate list */
	for (i = 0; i < nvpb->nr_buffers; i++) {
		struct nouveau_pushbuf_bo *pbbo = &nvpb->buffers[i];

		nouveau_bo(pbbo->bo)->pending = NULL;
		nouveau_bo_del(&pbbo->bo);
	}
	nvpb->nr_buffers = 0;

	/* Switch back to user's ring */
	RING_SPACE_CH(chan, 1);
	OUT_RING_CH(chan, 0x20000000 | ((nvpb->start << 2) +
					nvchan->dma_master.base));
	nvchan->dma = &nvchan->dma_master;

	/* Fence + kickoff */
	nouveau_fence_emit(nvpb->fence);
	FIRE_RING_CH(chan);

	/* Allocate space for next push buffer */
	ret = nouveau_pushbuf_space(chan, min);
	assert(!ret);

	return 0;
}

static struct nouveau_pushbuf_bo *
nouveau_pushbuf_emit_buffer(struct nouveau_channel *chan, struct nouveau_bo *bo)
{
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(chan->pushbuf);
	struct nouveau_bo_priv *nvbo = nouveau_bo(bo);
	struct nouveau_pushbuf_bo *pbbo;

	if (nvbo->pending)
		return nvbo->pending;

	if (nvpb->nr_buffers >= NOUVEAU_PUSHBUF_MAX_BUFFERS)
		return NULL;
	pbbo = nvpb->buffers + nvpb->nr_buffers++;
	nvbo->pending = pbbo;

	nouveau_bo_ref(bo->device, bo->handle, &pbbo->bo);
	pbbo->channel = chan;
	pbbo->flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART;
	pbbo->handled = 0;
	return pbbo;
}

int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *chan, void *ptr,
			   struct nouveau_bo *bo, uint32_t data, uint32_t flags,
			   uint32_t vor, uint32_t tor)
{
	struct nouveau_pushbuf_priv *nvpb = nouveau_pushbuf(chan->pushbuf);
	struct nouveau_pushbuf_bo *pbbo;
	struct nouveau_pushbuf_reloc *r;

	if (nvpb->nr_relocs >= NOUVEAU_PUSHBUF_MAX_RELOCS)
		return -ENOMEM;

	pbbo = nouveau_pushbuf_emit_buffer(chan, bo);
	if (!pbbo)
		return -ENOMEM;
	pbbo->flags |= (flags & NOUVEAU_BO_RDWR);
	pbbo->flags &= (flags | NOUVEAU_BO_RDWR);

	r = nvpb->relocs + nvpb->nr_relocs++;
	r->pbbo = pbbo;
	r->ptr = ptr;
	r->flags = flags;
	r->data = data;
	r->vor = vor;
	r->tor = tor;

	if (flags & NOUVEAU_BO_DUMMY)
		*(uint32_t *)ptr = 0;
	else
		*(uint32_t *)ptr = nouveau_pushbuf_calc_reloc(bo, r);
	return 0;
}

