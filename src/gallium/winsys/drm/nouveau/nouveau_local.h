#ifndef __NOUVEAU_LOCAL_H__
#define __NOUVEAU_LOCAL_H__

#include "pipe/p_compiler.h"
#include "nouveau_winsys_pipe.h"
#include <stdio.h>

struct pipe_buffer;

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
#define NOUVEAU_DMA_BARRIER 
#define NOUVEAU_DMA_TIMEOUT 2000

/* Push buffer access macros */
static INLINE void
OUT_RING(struct nouveau_channel *chan, unsigned data)
{
	*(chan->pushbuf->cur++) = (data);
}

static INLINE void
OUT_RINGp(struct nouveau_channel *chan, uint32_t *data, unsigned size)
{
	memcpy(chan->pushbuf->cur, data, size * 4);
	chan->pushbuf->cur += size;
}

static INLINE void
OUT_RINGf(struct nouveau_channel *chan, float f)
{
	union { uint32_t i; float f; } c;
	c.f = f;
	OUT_RING(chan, c.i);
}

static INLINE void
BEGIN_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr,
	   unsigned mthd, unsigned size)
{
	if (chan->pushbuf->remaining < (size + 1))
		nouveau_pushbuf_flush(chan, (size + 1));
	OUT_RING(chan, (gr->subc << 13) | (size << 18) | mthd);
	chan->pushbuf->remaining -= (size + 1);
}

static INLINE void
FIRE_RING(struct nouveau_channel *chan)
{
	nouveau_pushbuf_flush(chan, 0);
}

static INLINE void
BIND_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr, unsigned subc)
{
	gr->subc = subc;
	BEGIN_RING(chan, gr, 0x0000, 1);
	OUT_RING  (chan, gr->handle);
}

static INLINE void
OUT_RELOC(struct nouveau_channel *chan, struct nouveau_bo *bo,
	  unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	nouveau_pushbuf_emit_reloc(chan, chan->pushbuf->cur++, bo,
				   data, flags, vor, tor);
}

/* Raw data + flags depending on FB/TT buffer */
static INLINE void
OUT_RELOCd(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	OUT_RELOC(chan, bo, data, flags | NOUVEAU_BO_OR, vor, tor);
}

/* FB/TT object handle */
static INLINE void
OUT_RELOCo(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned flags)
{
	OUT_RELOC(chan, bo, 0, flags | NOUVEAU_BO_OR,
		  chan->vram->handle, chan->gart->handle);
}

/* Low 32-bits of offset */
static INLINE void
OUT_RELOCl(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned delta, unsigned flags)
{
	OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_LOW, 0, 0);
}

/* High 32-bits of offset */
static INLINE void
OUT_RELOCh(struct nouveau_channel *chan, struct nouveau_bo *bo,
	   unsigned delta, unsigned flags)
{
	OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_HIGH, 0, 0);
}

#endif
