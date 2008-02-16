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

static inline uint32_t
READ_GET(struct nouveau_channel_priv *nvchan)
{
	return *nvchan->get;
}

static inline void
WRITE_PUT(struct nouveau_channel_priv *nvchan, uint32_t val)
{
	uint32_t put = ((val << 2) + nvchan->dma->base);
	volatile int dum;

	NOUVEAU_DMA_BARRIER;
	dum = READ_GET(nvchan);

	*nvchan->put = put;
	nvchan->dma->put = val;
#ifdef NOUVEAU_DMA_TRACE
	NOUVEAU_MSG("WRITE_PUT %d/0x%08x\n", nvchan->drm.channel, put);
#endif

	NOUVEAU_DMA_BARRIER;
}

static inline int
LOCAL_GET(struct nouveau_dma_priv *dma, uint32_t *val)
{
	uint32_t get = *val;

	if (get >= dma->base && get <= (dma->base + (dma->max << 2))) {
		*val = (get - dma->base) >> 2;
		return 1;
	}

	return 0;
}

void
nouveau_dma_channel_init(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	int i;

	nvchan->dma = &nvchan->dma_master;
	nvchan->dma->base = nvchan->drm.put_base;
	nvchan->dma->cur  = nvchan->dma->put = 0;
	nvchan->dma->max  = (nvchan->drm.cmdbuf_size >> 2) - 2;
	nvchan->dma->free = nvchan->dma->max - nvchan->dma->cur;

	RING_SPACE_CH(chan, RING_SKIPS);
	for (i = 0; i < RING_SKIPS; i++)
		OUT_RING_CH(chan, 0);
}

#define CHECK_TIMEOUT() do {                                                   \
	if ((NOUVEAU_TIME_MSEC() - t_start) > NOUVEAU_DMA_TIMEOUT)             \
		return - EBUSY;                                                \
} while(0)

int
nouveau_dma_wait(struct nouveau_channel *chan, int size)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_dma_priv *dma = nvchan->dma;
	uint32_t get, t_start;

	FIRE_RING_CH(chan);

	t_start = NOUVEAU_TIME_MSEC();
	while (dma->free < size) {
		CHECK_TIMEOUT();

		get = READ_GET(nvchan);
		if (!LOCAL_GET(dma, &get))
			continue;

		if (dma->put >= get) {
			dma->free = dma->max - dma->cur;

			if (dma->free < size) {
#ifdef NOUVEAU_DMA_DEBUG
				dma->push_free = 1;
#endif
				OUT_RING_CH(chan, 0x20000000 | dma->base);
				if (get <= RING_SKIPS) {
					/*corner case - will be idle*/
					if (dma->put <= RING_SKIPS)
						WRITE_PUT(nvchan,
							  RING_SKIPS + 1);

					do {
						CHECK_TIMEOUT();
						get = READ_GET(nvchan);
						if (!LOCAL_GET(dma, &get))
							get = 0;
					} while (get <= RING_SKIPS);
				}

				WRITE_PUT(nvchan, RING_SKIPS);
				dma->cur  = dma->put = RING_SKIPS;
				dma->free = get - (RING_SKIPS + 1);
			}
		} else {
			dma->free = get - dma->cur - 1;
		}
	}

	return 0;
}

#ifdef NOUVEAU_DMA_DUMP_POSTRELOC_PUSHBUF
static void
nouveau_dma_parse_pushbuf(struct nouveau_channel *chan, int get, int put)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	unsigned mthd_count = 0;
	
	while (get != put) {
		uint32_t gpuget = (get << 2) + nvchan->drm.put_base;
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
nouveau_dma_kickoff(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	struct nouveau_dma_priv *dma = nvchan->dma;

	if (dma->cur == dma->put)
		return;

#ifdef NOUVEAU_DMA_DEBUG
	if (dma->push_free) {
		NOUVEAU_ERR("Packet incomplete: %d left\n", dma->push_free);
		return;
	}
#endif

#ifdef NOUVEAU_DMA_DUMP_POSTRELOC_PUSHBUF
	nouveau_dma_parse_pushbuf(chan, dma->put, dma->cur);
#endif

	WRITE_PUT(nvchan, dma->cur);
}
