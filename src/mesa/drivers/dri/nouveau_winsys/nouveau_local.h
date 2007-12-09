#ifndef __NOUVEAU_LOCAL_H__
#define __NOUVEAU_LOCAL_H__

#include <stdio.h>

/* Debug output */
#define NOUVEAU_MSG(fmt, args...) do {                                         \
	fprintf(stdout, "nouveau: "fmt, ##args);                               \
	fflush(stdout);                                                        \
} while(0)

#define NOUVEAU_ERR(fmt, args...) do {                                         \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);           \
	fflush(stderr);                                                        \
} while(0)

#define NOUVEAU_TIME_MSEC() 0

/* User FIFO control */
//#define NOUVEAU_DMA_TRACE
//#define NOUVEAU_DMA_DEBUG
//#define NOUVEAU_DMA_DUMP_POSTRELOC_PUSHBUF
#define NOUVEAU_DMA_SUBCHAN_LRU
#define NOUVEAU_DMA_BARRIER 
#define NOUVEAU_DMA_TIMEOUT 2000

/* Push buffer access macros */
#define BEGIN_RING(obj,mthd,size) do {                                         \
	nv->pushbuf = nouveau_pipe_dma_beginp(nv->obj, (mthd), (size));        \
} while(0)

#define OUT_RING(data) do {                                                    \
	(*nv->pushbuf++) = (data);                                             \
} while(0)

#define OUT_RINGp(src,size) do {                                               \
	memcpy(nv->pushbuf, (src), (size)<<2);                                 \
	nv->pushbuf += (size);                                                 \
} while(0)

#define OUT_RINGf(data) do {                                                   \
	union { float v; uint32_t u; } c;                                      \
	c.v = (data);                                                          \
	OUT_RING(c.u);                                                         \
} while(0)

#define FIRE_RING() do {                                                       \
	nouveau_pipe_dma_kickoff(nv->channel);                                 \
} while(0)

#define OUT_RELOC(bo,data,flags,vor,tor) do {                                  \
	nouveau_pushbuf_emit_reloc(nv->channel, nv->pushbuf, (void*)(bo),      \
				   (data), (flags), (vor), (tor));             \
	OUT_RING(0);                                                           \
} while(0)

/* Raw data + flags depending on FB/TT buffer */
#define OUT_RELOCd(bo,data,flags,vor,tor) do {                                 \
	OUT_RELOC((bo), (data), (flags) | NOUVEAU_BO_OR, (vor), (tor));        \
} while(0)

/* FB/TT object handle */
#define OUT_RELOCo(bo,flags) do {                                              \
	OUT_RELOC((bo), 0, (flags) | NOUVEAU_BO_OR,                            \
		  nv->channel->vram->handle, nv->channel->gart->handle);       \
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
