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

#ifndef __NOUVEAU_DMA_H__
#define __NOUVEAU_DMA_H__

#include <string.h>
#include "nouveau_drmif.h"
#include "nouveau_local.h"

#define RING_SKIPS 8

extern int  nouveau_dma_wait(struct nouveau_channel *chan, int size);
extern void nouveau_dma_subc_bind(struct nouveau_grobj *);
extern void nouveau_dma_channel_init(struct nouveau_channel *);
extern void nouveau_dma_kickoff(struct nouveau_channel *);

#ifdef NOUVEAU_DMA_DEBUG
static char faulty[1024];
#endif

static inline void
nouveau_dma_out(struct nouveau_channel *chan, uint32_t data)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_dma_priv *dma = nvchan->dma;

#ifdef NOUVEAU_DMA_DEBUG
	if (dma->push_free == 0) {
		NOUVEAU_ERR("No space left in packet at %s\n", faulty);
		return;
	}
	dma->push_free--;
#endif
#ifdef NOUVEAU_DMA_TRACE
	{
		uint32_t offset = (dma->cur << 2) + dma->base;
		NOUVEAU_MSG("\tOUT_RING %d/0x%08x -> 0x%08x\n",
			    nvchan->drm.channel, offset, data);
	}
#endif
	nvchan->pushbuf[dma->cur + (dma->base - nvchan->drm.put_base)/4] = data;
	dma->cur++;
}

static inline void
nouveau_dma_outp(struct nouveau_channel *chan, uint32_t *ptr, int size)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_dma_priv *dma = nvchan->dma;
	(void)dma;

#ifdef NOUVEAU_DMA_DEBUG
	if (dma->push_free < size) {
		NOUVEAU_ERR("Packet too small.  Free=%d, Need=%d\n",
			    dma->push_free, size);
		return;
	}
#endif
#ifdef NOUVEAU_DMA_TRACE
	while (size--) {
		nouveau_dma_out(chan, *ptr);
		ptr++;
	}
#else
	memcpy(&nvchan->pushbuf[dma->cur], ptr, size << 2);
#ifdef NOUVEAU_DMA_DEBUG
	dma->push_free -= size;
#endif
	dma->cur += size;
#endif
}

static inline void
nouveau_dma_space(struct nouveau_channel *chan, int size)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_dma_priv *dma = nvchan->dma;

	if (dma->free < size) {
		if (nouveau_dma_wait(chan, size) && chan->hang_notify)
			chan->hang_notify(chan);
	}
	dma->free -= size;
#ifdef NOUVEAU_DMA_DEBUG
	dma->push_free = size;
#endif
}

static inline void
nouveau_dma_begin(struct nouveau_channel *chan, struct nouveau_grobj *grobj,
		  int method, int size, const char* file, int line)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_dma_priv *dma = nvchan->dma;
	(void)dma;

#ifdef NOUVEAU_DMA_TRACE
	NOUVEAU_MSG("BEGIN_RING %d/%08x/%d/0x%04x/%d\n", nvchan->drm.channel,
		    grobj->handle, grobj->subc, method, size);
#endif

#ifdef NOUVEAU_DMA_DEBUG
	if (dma->push_free) {
		NOUVEAU_ERR("Previous packet incomplete: %d left at %s\n",
			    dma->push_free, faulty);
		return;
	}
	sprintf(faulty,"%s:%d",file,line);
#endif

	nouveau_dma_space(chan, (size + 1));
	nouveau_dma_out(chan, (size << 18) | (grobj->subc << 13) | method);
}

#define RING_SPACE_CH(ch,sz)         nouveau_dma_space((ch), (sz))
#define BEGIN_RING_CH(ch,gr,m,sz)    nouveau_dma_begin((ch), (gr), (m), (sz), __FUNCTION__, __LINE__ )
#define OUT_RING_CH(ch, data)        nouveau_dma_out((ch), (data))
#define OUT_RINGp_CH(ch,ptr,dwords)  nouveau_dma_outp((ch), (void*)(ptr),      \
						      (dwords))
#define FIRE_RING_CH(ch)             nouveau_dma_kickoff((ch))
#define WAIT_RING_CH(ch,sz)          nouveau_dma_wait((ch), (sz))
		
#endif
