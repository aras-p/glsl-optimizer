
#ifndef __NV50_WINSYS_H__
#define __NV50_WINSYS_H__

#include <stdint.h>
#include <unistd.h>

#include "pipe/p_defines.h"

#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_grobj.h"
#include "nouveau/nouveau_device.h"
#include "nouveau/nouveau_resource.h"
#include "nouveau/nouveau_pushbuf.h"
#include "nouveau/nouveau_reloc.h"
#include "nouveau/nouveau_notifier.h"

#include "nouveau/nouveau_buffer.h"

#ifndef NV04_PFIFO_MAX_PACKET_LEN
#define NV04_PFIFO_MAX_PACKET_LEN 2047
#endif

#define NV50_SUBCH_3D 5
#define NV50_SUBCH_2D 6
#define NV50_SUBCH_MF 7

#define NV50_MF_(n) NV50_M2MF_##n

#define RING_3D(n) ((NV50_SUBCH_3D << 13) | NV50_3D_##n)
#define RING_2D(n) ((NV50_SUBCH_2D << 13) | NV50_2D_##n)
#define RING_MF(n) ((NV50_SUBCH_MF << 13) | NV50_MF_(n))

#define RING_3D_(m) ((NV50_SUBCH_3D << 13) | (m))
#define RING_2D_(m) ((NV50_SUBCH_2D << 13) | (m))
#define RING_MF_(m) ((NV50_SUBCH_MF << 13) | (m))

#define RING_GR(gr, m) (((gr)->subc << 13) | (m))

int nouveau_pushbuf_flush(struct nouveau_channel *, unsigned min);

static inline uint32_t
nouveau_bo_tile_layout(struct nouveau_bo *bo)
{
   return bo->tile_flags & NOUVEAU_BO_TILE_LAYOUT_MASK;
}

static INLINE void
nouveau_bo_validate(struct nouveau_channel *chan,
                    struct nouveau_bo *bo, unsigned flags)
{
   nouveau_reloc_emit(chan, NULL, 0, NULL, bo, 0, 0, flags, 0, 0);
}

/* incremental methods */
static INLINE void
BEGIN_RING(struct nouveau_channel *chan, uint32_t mthd, unsigned size)
{
   WAIT_RING(chan, size + 1);
   OUT_RING (chan, (size << 18) | mthd);
}

/* non-incremental */
static INLINE void
BEGIN_RING_NI(struct nouveau_channel *chan, uint32_t mthd, unsigned size)
{
   WAIT_RING(chan, size + 1);
   OUT_RING (chan, (0x2 << 29) | (size << 18) | mthd);
}

static INLINE int
OUT_RESRCh(struct nouveau_channel *chan, struct nv04_resource *res,
           unsigned delta, unsigned flags)
{
   return OUT_RELOCh(chan, res->bo, res->offset + delta, res->domain | flags);
}

static INLINE int
OUT_RESRCl(struct nouveau_channel *chan, struct nv04_resource *res,
           unsigned delta, unsigned flags)
{
   if (flags & NOUVEAU_BO_WR)
      res->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
   return OUT_RELOCl(chan, res->bo, res->offset + delta, res->domain | flags);
}

static INLINE void
BIND_RING(struct nouveau_channel *chan, struct nouveau_grobj *gr, unsigned s)
{
   struct nouveau_subchannel *subc = &gr->channel->subc[s];

   assert(s < 8);
   if (subc->gr) {
      assert(subc->gr->bound != NOUVEAU_GROBJ_BOUND_EXPLICIT);
      subc->gr->bound = NOUVEAU_GROBJ_UNBOUND;
   }
   subc->gr = gr;
   subc->gr->subc = s;
   subc->gr->bound = NOUVEAU_GROBJ_BOUND_EXPLICIT;

   BEGIN_RING(chan, RING_GR(gr, 0x0000), 1);
   OUT_RING  (chan, gr->handle);
}

#endif
