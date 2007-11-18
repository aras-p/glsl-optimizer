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
nouveau_dma_out(struct nouveau_channel *userchan, uint32_t data)
{
	struct nouveau_channel_priv *chan = nouveau_channel(userchan);

#ifdef NOUVEAU_DMA_DEBUG
	if (chan->dma.push_free == 0) {
		NOUVEAU_ERR("No space left in packet. Error at %s\n",faulty);
		return;
	}
	chan->dma.push_free--;
#endif
#ifdef NOUVEAU_DMA_TRACE
	{
		uint32_t offset = (chan->dma.cur << 2) + chan->dma.base;
		NOUVEAU_MSG("\tOUT_RING %d/0x%08x -> 0x%08x\n",
			    chan->drm.channel, offset, data);
	}
#endif
	chan->pushbuf[chan->dma.cur++] = data;
}

static inline void
nouveau_dma_outp(struct nouveau_channel *userchan, uint32_t *ptr, int size)
{
	struct nouveau_channel_priv *chan = nouveau_channel(userchan);
	(void)chan;

#ifdef NOUVEAU_DMA_DEBUG
	if (chan->dma.push_free < size) {
		NOUVEAU_ERR("Packet too small.  Free=%d, Need=%d\n",
			    chan->dma.push_free, size);
		return;
	}
#endif
#ifdef NOUVEAU_DMA_TRACE
	while (size--) {
		nouveau_dma_out(userchan, *ptr);
		ptr++;
	}
#else
	memcpy(&chan->pushbuf[chan->dma.cur], ptr, size << 2);
#ifdef NOUVEAU_DMA_DEBUG
	chan->dma.push_free -= size;
#endif
	chan->dma.cur += size;
#endif
}

static inline void
nouveau_dma_begin(struct nouveau_channel *userchan, struct nouveau_grobj *grobj,
		  int method, int size, const char* file, int line)
{
	struct nouveau_channel_priv *chan = nouveau_channel(userchan);
	int push_size = size + 1;

#ifdef NOUVEAU_DMA_SUBCHAN_LRU
	if (grobj->bound == NOUVEAU_GROBJ_UNBOUND)
		nouveau_dma_subc_bind(grobj);
	chan->subchannel[grobj->subc].seq = chan->subc_sequence++;
#endif

#ifdef NOUVEAU_DMA_TRACE
	NOUVEAU_MSG("BEGIN_RING %d/%08x/%d/0x%04x/%d\n", chan->drm.channel,
		    grobj->handle, grobj->subc, method, size);
#endif

#ifdef NOUVEAU_DMA_DEBUG
	if (chan->dma.push_free) {
		NOUVEAU_ERR("Previous packet incomplete: %d left. Error at %s\n",
			    chan->dma.push_free,faulty);
		return;
	}
	sprintf(faulty,"%s:%d",file,line);
#endif

	if (chan->dma.free < push_size) {
		if (nouveau_dma_wait(userchan, push_size) &&
		    userchan->hang_notify) {
			userchan->hang_notify(userchan);
		}
	}
	chan->dma.free -= push_size;
#ifdef NOUVEAU_DMA_DEBUG
	chan->dma.push_free = push_size;
#endif

	nouveau_dma_out(userchan, (size << 18) | (grobj->subc << 13) | method);
}

static inline void
nouveau_dma_bind(struct nouveau_channel *userchan, struct nouveau_grobj *grobj,
		 int subc)
{
	struct nouveau_channel_priv *chan = nouveau_channel(userchan);

	if (chan->subchannel[subc].grobj == grobj)
		return;

	if (chan->subchannel[subc].grobj)
		chan->subchannel[subc].grobj->bound = NOUVEAU_GROBJ_UNBOUND;
	chan->subchannel[subc].grobj = grobj;
	grobj->subc  = subc;
	grobj->bound = NOUVEAU_GROBJ_EXPLICIT_BIND;

	nouveau_dma_begin(userchan, grobj, 0x0000, 1, __FUNCTION__, __LINE__);
	nouveau_dma_out  (userchan, grobj->handle);
}

#define BIND_RING_CH(ch,gr,sc)       nouveau_dma_bind((ch), (gr), (sc))
#define BEGIN_RING_CH(ch,gr,m,sz)    nouveau_dma_begin((ch), (gr), (m), (sz), __FUNCTION__, __LINE__ )
#define OUT_RING_CH(ch, data)        nouveau_dma_out((ch), (data))
#define OUT_RINGp_CH(ch,ptr,dwords)  nouveau_dma_outp((ch), (void*)(ptr),      \
						      (dwords))
#define FIRE_RING_CH(ch)             nouveau_dma_kickoff((ch))
#define WAIT_RING_CH(ch,sz)          nouveau_dma_wait((ch), (sz))
		
#endif
