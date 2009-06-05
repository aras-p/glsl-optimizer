#ifndef __NOUVEAU_PUSH_H__
#define __NOUVEAU_PUSH_H__

#include "nouveau/nouveau_winsys.h"

#ifndef NOUVEAU_PUSH_CONTEXT
#error undefined push context
#endif

#define OUT_RING(data) do {                                                    \
	NOUVEAU_PUSH_CONTEXT(pc);                                              \
	(*pc->base.channel->pushbuf->cur++) = (data);                          \
} while(0)

#define OUT_RINGp(src,size) do {                                               \
	NOUVEAU_PUSH_CONTEXT(pc);                                              \
	memcpy(pc->base.channel->pushbuf->cur, (src), (size) * 4);             \
	pc->base.channel->pushbuf->cur += (size);                              \
} while(0)

#define OUT_RINGf(data) do {                                                   \
	union { float v; uint32_t u; } c;                                      \
	c.v = (data);                                                          \
	OUT_RING(c.u);                                                         \
} while(0)

#define BEGIN_RING(obj,mthd,size) do {                                         \
	NOUVEAU_PUSH_CONTEXT(pc);                                              \
	struct nouveau_channel *chan = pc->base.channel;                       \
	if (chan->pushbuf->remaining < ((size) + 1))                           \
		nouveau_pushbuf_flush(chan, ((size) + 1));                     \
	OUT_RING((pc->obj->subc << 13) | ((size) << 18) | (mthd));             \
	chan->pushbuf->remaining -= ((size) + 1);                              \
} while(0)

#define BEGIN_RING_NI(obj,mthd,size) do {                                      \
	BEGIN_RING(obj, (mthd) | 0x40000000, (size));                          \
} while(0)

static inline void
DO_FIRE_RING(struct nouveau_channel *chan, struct pipe_fence_handle **fence)
{
	nouveau_pushbuf_flush(chan, 0);
	if (fence)
		*fence = NULL;
}

#define FIRE_RING(fence) do {                                                  \
	NOUVEAU_PUSH_CONTEXT(pc);                                              \
	DO_FIRE_RING(pc->base.channel, fence);                                 \
} while(0)

#define OUT_RELOC(bo,data,flags,vor,tor) do {                                  \
	NOUVEAU_PUSH_CONTEXT(pc);                                              \
	struct nouveau_channel *chan = pc->base.channel;                       \
	nouveau_pushbuf_emit_reloc(chan, chan->pushbuf->cur++, nouveau_bo(bo), \
				   (data), 0, (flags), (vor), (tor));          \
} while(0)

/* Raw data + flags depending on FB/TT buffer */
#define OUT_RELOCd(bo,data,flags,vor,tor) do {                                 \
	OUT_RELOC((bo), (data), (flags) | NOUVEAU_BO_OR, (vor), (tor));        \
} while(0)

/* FB/TT object handle */
#define OUT_RELOCo(bo,flags) do {                                              \
	OUT_RELOC((bo), 0, (flags) | NOUVEAU_BO_OR,                            \
		  pc->base.channel->vram->handle,                              \
		  pc->base.channel->gart->handle);                             \
} while(0)

/* Low 32-bits of offset */
#define OUT_RELOCl(bo,delta,flags) do {                                        \
	OUT_RELOC((bo), (delta), (flags) | NOUVEAU_BO_LOW, 0, 0);              \
} while(0)

/* High 32-bits of offset */
#define OUT_RELOCh(bo,delta,flags) do {                                        \
	OUT_RELOC((bo), (delta), (flags) | NOUVEAU_BO_HIGH, 0, 0);             \
} while(0)

/* A reloc which'll recombine into a NV_DMA_METHOD packet header */
#define OUT_RELOCm(bo, flags, obj, mthd, size) do {                            \
	NOUVEAU_PUSH_CONTEXT(pc);                                              \
	struct nouveau_channel *chan = pc->base.channel;                       \
	if (chan->pushbuf->remaining < ((size) + 1))                           \
		nouveau_pushbuf_flush(chan, ((size) + 1));                     \
	OUT_RELOCd((bo), (pc->obj->subc << 13) | ((size) << 18) | (mthd),      \
		   (flags), 0, 0);                                             \
	chan->pushbuf->remaining -= ((size) + 1);                              \
} while(0)

#endif
