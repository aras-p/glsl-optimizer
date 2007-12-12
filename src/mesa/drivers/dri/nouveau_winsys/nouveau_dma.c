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

#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "nouveau_drmif.h"
#include "nouveau_dma.h"
#include "nouveau_local.h"

static __inline__ uint32_t
READ_GET(struct nouveau_channel_priv *nvchan)
{
	return ((*nvchan->get - nvchan->dma.base) >> 2);
}

static __inline__ void
WRITE_PUT(struct nouveau_channel_priv *nvchan, uint32_t val)
{
	uint32_t put = ((val << 2) + nvchan->dma.base);
	volatile int dum;

	NOUVEAU_DMA_BARRIER;
	dum = READ_GET(nvchan);

	*nvchan->put = put;
	nvchan->dma.put = val;
#ifdef NOUVEAU_DMA_TRACE
	NOUVEAU_MSG("WRITE_PUT %d/0x%08x\n", nvchan->drm.channel, put);
#endif

	NOUVEAU_DMA_BARRIER;
}

void
nouveau_dma_channel_init(struct nouveau_channel *userchan)
{
	struct nouveau_channel_priv *chan = nouveau_channel(userchan);
	int i;

	chan->dma.base = chan->drm.put_base;
	chan->dma.cur  = chan->dma.put = RING_SKIPS;
	chan->dma.max  = (chan->drm.cmdbuf_size >> 2) - 2;
	chan->dma.free = chan->dma.max - chan->dma.cur;

	for (i = 0; i < RING_SKIPS; i++)
		chan->pushbuf[i] = 0x00000000;
}

#define CHECK_TIMEOUT() do {                                                   \
	if ((NOUVEAU_TIME_MSEC() - t_start) > NOUVEAU_DMA_TIMEOUT)             \
		return - EBUSY;                                                \
} while(0)

#define IN_MASTER_RING(chan, ptr) ((ptr) <= (chan)->dma.max)

int
nouveau_dma_wait(struct nouveau_channel *userchan, int size)
{
	struct nouveau_channel_priv *chan = nouveau_channel(userchan);
	uint32_t get, t_start;

	FIRE_RING_CH(userchan);

	t_start = NOUVEAU_TIME_MSEC();
	while (chan->dma.free < size) {
		CHECK_TIMEOUT();

		get = READ_GET(chan);
		if (!IN_MASTER_RING(chan, get))
			continue;

		if (chan->dma.put >= get) {
			chan->dma.free = chan->dma.max - chan->dma.cur;

			if (chan->dma.free < size) {
#ifdef NOUVEAU_DMA_DEBUG
				chan->dma.push_free = 1;
#endif
				OUT_RING_CH(userchan,
					    0x20000000 | chan->dma.base);
				if (get <= RING_SKIPS) {
					/*corner case - will be idle*/
					if (chan->dma.put <= RING_SKIPS)
						WRITE_PUT(chan, RING_SKIPS + 1);

					do {
						CHECK_TIMEOUT();
						get = READ_GET(chan);
						if (!IN_MASTER_RING(chan, get))
							continue;
					} while (get <= RING_SKIPS);
				}

				WRITE_PUT(chan, RING_SKIPS);
				chan->dma.cur  = chan->dma.put = RING_SKIPS;
				chan->dma.free = get - (RING_SKIPS + 1);
			}
		} else {
			chan->dma.free = get - chan->dma.cur - 1;
		}
	}

	return 0;
}

#ifdef NOUVEAU_DMA_SUBCHAN_LRU
void
nouveau_dma_subc_bind(struct nouveau_grobj *grobj)
{
	struct nouveau_channel_priv *chan = nouveau_channel(grobj->channel);
	int subc = -1, i;
	
	for (i = 0; i < 8; i++) {
		if (chan->subchannel[i].grobj &&
		    chan->subchannel[i].grobj->bound == 
		    NOUVEAU_GROBJ_EXPLICIT_BIND)
			continue;
		if (chan->subchannel[i].seq < chan->subchannel[subc].seq)
			subc = i;
	}
	assert(subc >= 0);

	if (chan->subchannel[subc].grobj)
		chan->subchannel[subc].grobj->bound = 0;
	chan->subchannel[subc].grobj = grobj;
	grobj->subc  = subc;
	grobj->bound = NOUVEAU_GROBJ_BOUND;

	BEGIN_RING_CH(grobj->channel, grobj, 0, 1);
	nouveau_dma_out  (grobj->channel, grobj->handle);
}
#endif

#ifdef NOUVEAU_DMA_DUMP_POSTRELOC_PUSHBUF
static void
nouveau_dma_parse_pushbuf(struct nouveau_channel *chan, int get, int put)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	unsigned mthd_count = 0;
	
	while (get != put) {
		uint32_t gpuget = (get << 2) + nvchan->dma.base;
		uint32_t data;

		if (get < 0 || get >= nvchan->drm.cmdbuf_size) {
			NOUVEAU_ERR("DMA_PT 0x%08x\n", gpuget);
			assert(0);
		}
		data = nvchan->pushbuf[get++];

		if (mthd_count) {
			NOUVEAU_MSG("0x%08x 0x%08x\n", gpuget, data);
			mthd_count--;
			continue;
		}

		switch (data & 0x60000000) {
		case 0x00000000:
			mthd_count = (data >> 18) & 0x7ff;
			NOUVEAU_MSG("0x%08x 0x%08x MTHD "
				    "Sc %d Mthd 0x%04x Size %d\n",
				    gpuget, data, (data>>13) & 7, data & 0x1ffc,
				    mthd_count);
			break;
		case 0x20000000:
			get = (data & 0x1ffffffc) >> 2;
			NOUVEAU_MSG("0x%08x 0x%08x JUMP 0x%08x\n",
				    gpuget, data, data & 0x1ffffffc);
			continue;
		case 0x40000000:
			mthd_count = (data >> 18) & 0x7ff;
			NOUVEAU_MSG("0x%08x 0x%08x NINC "
				    "Sc %d Mthd 0x%04x Size %d\n",
				    gpuget, data, (data>>13) & 7, data & 0x1ffc,
				    mthd_count);
			break;
		case 0x60000000:
			/* DMA_OPCODE_CALL apparently, doesn't seem to work on
			 * my NV40 at least..
			 */
			/* fall-through */
		default:
			NOUVEAU_MSG("DMA_PUSHER 0x%08x 0x%08x\n",
				    gpuget, data);
			assert(0);
		}
	}
}
#endif

void
nouveau_dma_kickoff(struct nouveau_channel *userchan)
{
	struct nouveau_channel_priv *chan = nouveau_channel(userchan);

	if (chan->dma.cur == chan->dma.put)
		return;

#ifdef NOUVEAU_DMA_DEBUG
	if (chan->dma.push_free) {
		NOUVEAU_ERR("Packet incomplete: %d left\n", chan->dma.push_free);
		return;
	}
#endif

#ifdef NOUVEAU_DMA_DUMP_POSTRELOC_PUSHBUF
	nouveau_dma_parse_pushbuf(userchan, chan->dma.put, chan->dma.cur);
#endif

	WRITE_PUT(chan, chan->dma.cur);
}
