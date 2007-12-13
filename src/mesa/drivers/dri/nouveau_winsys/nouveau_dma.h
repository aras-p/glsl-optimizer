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

#ifdef NOUVEAU_DMA_DEBUG
	if (nvchan->dma.push_free == 0) {
		NOUVEAU_ERR("No space left in packet. Error at %s\n",faulty);
		return;
	}
	nvchan->dma.push_free--;
#endif
#ifdef NOUVEAU_DMA_TRACE
	{
		uint32_t offset = (nvchan->dma.cur << 2) + nvchan->dma.base;
		NOUVEAU_MSG("\tOUT_RING %d/0x%08x -> 0x%08x\n",
			    nvchan->drm.channel, offset, data);
	}
#endif
	nvchan->pushbuf[nvchan->dma.cur++] = data;
}

static inline void
nouveau_dma_outp(struct nouveau_channel *chan, uint32_t *ptr, int size)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	(void)chan;

#ifdef NOUVEAU_DMA_DEBUG
	if (nvchan->dma.push_free < size) {
		NOUVEAU_ERR("Packet too small.  Free=%d, Need=%d\n",
			    nvchan->dma.push_free, size);
		return;
	}
#endif
#ifdef NOUVEAU_DMA_TRACE
	while (size--) {
		nouveau_dma_out(chan, *ptr);
		ptr++;
	}
#else
	memcpy(&nvchan->pushbuf[nvchan->dma.cur], ptr, size << 2);
#ifdef NOUVEAU_DMA_DEBUG
	nvchan->dma.push_free -= size;
#endif
	nvchan->dma.cur += size;
#endif
}

static inline void
nouveau_dma_begin(struct nouveau_channel *chan, struct nouveau_grobj *grobj,
		  int method, int size, const char* file, int line)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	int push_size = size + 1;

#ifdef NOUVEAU_DMA_SUBCHAN_LRU
	if (grobj->bound == NOUVEAU_GROBJ_UNBOUND)
		nouveau_dma_subc_bind(grobj);
	nvchan->subchannel[grobj->subc].seq = nvchan->subc_sequence++;
#endif

#ifdef NOUVEAU_DMA_TRACE
	NOUVEAU_MSG("BEGIN_RING %d/%08x/%d/0x%04x/%d\n", nvchan->drm.channel,
		    grobj->handle, grobj->subc, method, size);
#endif

#ifdef NOUVEAU_DMA_DEBUG
	if (nvchan->dma.push_free) {
		NOUVEAU_ERR("Previous packet incomplete: %d left. Error at %s\n",
			    nvchan->dma.push_free,faulty);
		return;
	}
	sprintf(faulty,"%s:%d",file,line);
#endif

	if (nvchan->dma.free < push_size) {
		if (nouveau_dma_wait(chan, push_size) &&
		    chan->hang_notify) {
			chan->hang_notify(chan);
		}
	}
	nvchan->dma.free -= push_size;
#ifdef NOUVEAU_DMA_DEBUG
	nvchan->dma.push_free = push_size;
#endif

	nouveau_dma_out(chan, (size << 18) | (grobj->subc << 13) | method);
}

static inline void
nouveau_dma_bind(struct nouveau_channel *chan, struct nouveau_grobj *grobj,
		 int subc)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);

	if (nvchan->subchannel[subc].grobj == grobj)
		return;

	if (nvchan->subchannel[subc].grobj)
		nvchan->subchannel[subc].grobj->bound = NOUVEAU_GROBJ_UNBOUND;
	nvchan->subchannel[subc].grobj = grobj;
	grobj->subc  = subc;
	grobj->bound = NOUVEAU_GROBJ_EXPLICIT_BIND;

	nouveau_dma_begin(chan, grobj, 0x0000, 1, __FUNCTION__, __LINE__);
	nouveau_dma_out  (chan, grobj->handle);
}

#define BIND_RING_CH(ch,gr,sc)       nouveau_dma_bind((ch), (gr), (sc))
#define BEGIN_RING_CH(ch,gr,m,sz)    nouveau_dma_begin((ch), (gr), (m), (sz), __FUNCTION__, __LINE__ )
#define OUT_RING_CH(ch, data)        nouveau_dma_out((ch), (data))
#define OUT_RINGp_CH(ch,ptr,dwords)  nouveau_dma_outp((ch), (void*)(ptr),      \
						      (dwords))
#define FIRE_RING_CH(ch)             nouveau_dma_kickoff((ch))
#define WAIT_RING_CH(ch,sz)          nouveau_dma_wait((ch), (sz))
		
#endif
