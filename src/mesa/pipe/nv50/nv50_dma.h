#ifndef __NV50_DMA_H__
#define __NV50_DMA_H__

#include "pipe/nouveau/nouveau_winsys.h"

#define BEGIN_RING(obj,mthd,size) do {                                         \
	nv50->pushbuf = nv50->nvws->begin_ring(nv50->obj, (mthd), (size));     \
} while(0)

#define BEGIN_RING_NI(obj,mthd,size) do {                                      \
	BEGIN_RING(obj, (mthd) | 0x40000000, (size));                          \
} while(0)

#define OUT_RING(data) do {                                                    \
	(*nv50->pushbuf++) = (data);                                           \
} while(0)

#define OUT_RINGp(src,size) do {                                               \
	memcpy(nv50->pushbuf, (src), (size) * 4);                              \
	nv50->pushbuf += (size);                                               \
} while(0)

#define OUT_RINGf(data) do {                                                   \
	union { float v; uint32_t u; } c;                                      \
	c.v = (data);                                                          \
	OUT_RING(c.u);                                                         \
} while(0)

#define FIRE_RING() do {                                                       \
	nv50->nvws->fire_ring(nv50->nvws->channel);                            \
} while(0)

#define OUT_RELOC(bo,data,flags,vor,tor) do {                                  \
	nv50->nvws->out_reloc(nv50->nvws->channel, nv50->pushbuf,              \
			      (struct nouveau_bo *)(bo),                       \
			      (data), (flags), (vor), (tor));                  \
	OUT_RING(0);                                                           \
} while(0)

/* Raw data + flags depending on FB/TT buffer */
#define OUT_RELOCd(bo,data,flags,vor,tor) do {                                 \
	OUT_RELOC((bo), (data), (flags) | NOUVEAU_BO_OR, (vor), (tor));        \
} while(0)

/* FB/TT object handle */
#define OUT_RELOCo(bo,flags) do {                                              \
	OUT_RELOC((bo), 0, (flags) | NOUVEAU_BO_OR,                            \
		  nv50->nvws->channel->vram->handle,                           \
		  nv50->nvws->channel->gart->handle);                          \
} while(0)

/* Low 32-bits of offset */
#define OUT_RELOCl(bo,delta,flags) do {                                        \
	OUT_RELOC((bo), (delta), (flags) | NOUVEAU_BO_LOW, 0, 0);              \
} while(0)

/* High 32-bits of offset */
#define OUT_RELOCh(bo,delta,flags) do {                                        \
	OUT_RELOC((bo), (delta), (flags) | NOUVEAU_BO_HIGH, 0, 0);             \
} while(0)

#endif
