
#ifndef __NVC0_WINSYS_H__
#define __NVC0_WINSYS_H__

#include <stdint.h>
#include <unistd.h>
#include "pipe/p_defines.h"

#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_device.h"
#include "nouveau/nouveau_resource.h"
#include "nouveau/nouveau_reloc.h"

#ifndef NV04_PFIFO_MAX_PACKET_LEN
#define NV04_PFIFO_MAX_PACKET_LEN 2047
#endif

#define SLEEP(us) usleep(us)

extern uint64_t nouveau_bo_gpu_address(struct nouveau_bo *);

#define NVC0_SUBCH_3D 1
#define NVC0_SUBCH_2D 2
#define NVC0_SUBCH_MF 3

#define NVC0_MF_(n) NVC0_M2MF_##n

#define RING_3D(n) ((NVC0_SUBCH_3D << 13) | (NVC0_3D_##n >> 2))
#define RING_2D(n) ((NVC0_SUBCH_2D << 13) | (NVC0_2D_##n >> 2))
#define RING_MF(n) ((NVC0_SUBCH_MF << 13) | (NVC0_MF_(n) >> 2))

#define RING_3D_(m) ((NVC0_SUBCH_3D << 13) | ((m) >> 2))
#define RING_2D_(m) ((NVC0_SUBCH_2D << 13) | ((m) >> 2))
#define RING_MF_(m) ((NVC0_SUBCH_MF << 13) | ((m) >> 2))

#define RING_ANY(m) ((NVC0_SUBCH_3D << 13) | ((m) >> 2))

int nouveau_pushbuf_flush(struct nouveau_channel *, unsigned min);

static INLINE void
WAIT_RING(struct nouveau_channel *chan, unsigned size)
{
   if (chan->cur + size > chan->end)
      nouveau_pushbuf_flush(chan, size);
}

static INLINE void
OUT_RING(struct nouveau_channel *chan, uint32_t data)
{
   *(chan->cur++) = (data);
}

/* incremental methods */
static INLINE void
BEGIN_RING(struct nouveau_channel *chan, uint32_t mthd, unsigned size)
{
   WAIT_RING(chan, size + 1);
   OUT_RING (chan, (0x2 << 28) | (size << 16) | mthd);
}

/* non-incremental */
static INLINE void
BEGIN_RING_NI(struct nouveau_channel *chan, uint32_t mthd, unsigned size)
{
   WAIT_RING(chan, size + 1);
   OUT_RING (chan, (0x6 << 28) | (size << 16) | mthd);
}

/* increment-once */
static INLINE void
BEGIN_RING_1I(struct nouveau_channel *chan, uint32_t mthd, unsigned size)
{
   WAIT_RING(chan, size + 1);
   OUT_RING (chan, (0xa << 28) | (size << 16) | mthd);
}

/* inline-data */
static INLINE void
INLIN_RING(struct nouveau_channel *chan, uint32_t mthd, unsigned data)
{
   WAIT_RING(chan, 1);
   OUT_RING (chan, (0x8 << 28) | (data << 16) | mthd);
}

int
nouveau_pushbuf_marker_emit(struct nouveau_channel *chan,
                            unsigned wait_dwords, unsigned wait_relocs);
int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *, void *ptr,
                           struct nouveau_bo *, uint32_t data, uint32_t data2,
                           uint32_t flags, uint32_t vor, uint32_t tor);
int
nouveau_pushbuf_submit(struct nouveau_channel *chan, struct nouveau_bo *bo,
                       unsigned offset, unsigned length);

static INLINE int
MARK_RING(struct nouveau_channel *chan, unsigned dwords, unsigned relocs)
{
   return nouveau_pushbuf_marker_emit(chan, dwords, relocs);
}

static INLINE void
OUT_RINGf(struct nouveau_channel *chan, float data)
{
   union { uint32_t i; float f; } u;
   u.f = data;
   OUT_RING(chan, u.i);
}

static INLINE unsigned
AVAIL_RING(struct nouveau_channel *chan)
{
   return chan->end - chan->cur;
}

static INLINE void
OUT_RINGp(struct nouveau_channel *chan, const void *data, unsigned size)
{
   memcpy(chan->cur, data, size * 4);
   chan->cur += size;
}

static INLINE int
OUT_RELOC(struct nouveau_channel *chan, struct nouveau_bo *bo,
          unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
   return nouveau_pushbuf_emit_reloc(chan, chan->cur++, bo,
                                     data, 0, flags, vor, tor);
}

static INLINE int
OUT_RELOCl(struct nouveau_channel *chan, struct nouveau_bo *bo,
           unsigned delta, unsigned flags)
{
   return OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_LOW, 0, 0);
}

static INLINE int
OUT_RELOCh(struct nouveau_channel *chan, struct nouveau_bo *bo,
           unsigned delta, unsigned flags)
{
   return OUT_RELOC(chan, bo, delta, flags | NOUVEAU_BO_HIGH, 0, 0);
}

static INLINE void
FIRE_RING(struct nouveau_channel *chan)
{
   nouveau_pushbuf_flush(chan, 0);
}

#endif
