
#include "util/u_format.h"

#include "nvc0_context.h"

#include "nv50/nv50_defs.xml.h"

struct nvc0_transfer {
   struct pipe_transfer base;
   struct nv50_m2mf_rect rect[2];
   uint32_t nblocksx;
   uint16_t nblocksy;
   uint16_t nlayers;
};

void
nvc0_m2mf_transfer_rect(struct pipe_screen *pscreen,
                        const struct nv50_m2mf_rect *dst,
                        const struct nv50_m2mf_rect *src,
                        uint32_t nblocksx, uint32_t nblocksy)
{
   struct nouveau_channel *chan = nouveau_screen(pscreen)->channel;
   const int cpp = dst->cpp;
   uint32_t src_ofst = src->base;
   uint32_t dst_ofst = dst->base;
   uint32_t height = nblocksy;
   uint32_t sy = src->y;
   uint32_t dy = dst->y;
   uint32_t exec = (1 << 20);

   assert(dst->cpp == src->cpp);

   if (nouveau_bo_tile_layout(src->bo)) {
      BEGIN_RING(chan, RING_MF(TILING_MODE_IN), 5);
      OUT_RING  (chan, src->tile_mode);
      OUT_RING  (chan, src->width * cpp);
      OUT_RING  (chan, src->height);
      OUT_RING  (chan, src->depth);
      OUT_RING  (chan, src->z);
   } else {
      src_ofst += src->y * src->pitch + src->x * cpp;

      BEGIN_RING(chan, RING_MF(PITCH_IN), 1);
      OUT_RING  (chan, src->width * cpp);

      exec |= NVC0_M2MF_EXEC_LINEAR_IN;
   }

   if (nouveau_bo_tile_layout(dst->bo)) {
      BEGIN_RING(chan, RING_MF(TILING_MODE_OUT), 5);
      OUT_RING  (chan, dst->tile_mode);
      OUT_RING  (chan, dst->width * cpp);
      OUT_RING  (chan, dst->height);
      OUT_RING  (chan, dst->depth);
      OUT_RING  (chan, dst->z);
   } else {
      dst_ofst += dst->y * dst->pitch + dst->x * cpp;

      BEGIN_RING(chan, RING_MF(PITCH_OUT), 1);
      OUT_RING  (chan, dst->width * cpp);

      exec |= NVC0_M2MF_EXEC_LINEAR_OUT;
   }

   while (height) {
      int line_count = height > 2047 ? 2047 : height;

      MARK_RING (chan, 17, 4);

      BEGIN_RING(chan, RING_MF(OFFSET_IN_HIGH), 2);
      OUT_RELOCh(chan, src->bo, src_ofst, src->domain | NOUVEAU_BO_RD);
      OUT_RELOCl(chan, src->bo, src_ofst, src->domain | NOUVEAU_BO_RD);

      BEGIN_RING(chan, RING_MF(OFFSET_OUT_HIGH), 2);
      OUT_RELOCh(chan, dst->bo, dst_ofst, dst->domain | NOUVEAU_BO_WR);
      OUT_RELOCl(chan, dst->bo, dst_ofst, dst->domain | NOUVEAU_BO_WR);

      if (!(exec & NVC0_M2MF_EXEC_LINEAR_IN)) {
         BEGIN_RING(chan, RING_MF(TILING_POSITION_IN_X), 2);
         OUT_RING  (chan, src->x * cpp);
         OUT_RING  (chan, sy);
      } else {
         src_ofst += line_count * src->pitch;
      }
      if (!(exec & NVC0_M2MF_EXEC_LINEAR_OUT)) {
         BEGIN_RING(chan, RING_MF(TILING_POSITION_OUT_X), 2);
         OUT_RING  (chan, dst->x * cpp);
         OUT_RING  (chan, dy);
      } else {
         dst_ofst += line_count * dst->pitch;
      }

      BEGIN_RING(chan, RING_MF(LINE_LENGTH_IN), 2);
      OUT_RING  (chan, nblocksx * cpp);
      OUT_RING  (chan, line_count);
      BEGIN_RING(chan, RING_MF(EXEC), 1);
      OUT_RING  (chan, exec);

      height -= line_count;
      sy += line_count;
      dy += line_count;
   }
}

void
nvc0_m2mf_push_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned offset, unsigned domain,
                      unsigned size, const void *data)
{
   struct nouveau_channel *chan = nv->screen->channel;
   uint32_t *src = (uint32_t *)data;
   unsigned count = (size + 3) / 4;

   while (count) {
      unsigned nr;

      MARK_RING (chan, 16, 2);

      nr = AVAIL_RING(chan) - 9;
      nr = MIN2(count, nr);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN);

      BEGIN_RING(chan, RING_MF(OFFSET_OUT_HIGH), 2);
      OUT_RELOCh(chan, dst, offset, domain | NOUVEAU_BO_WR);
      OUT_RELOCl(chan, dst, offset, domain | NOUVEAU_BO_WR);
      BEGIN_RING(chan, RING_MF(LINE_LENGTH_IN), 2);
      OUT_RING  (chan, nr * 4);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_MF(EXEC), 1);
      OUT_RING  (chan, 0x100111);

      /* must not be interrupted (trap on QUERY fence, 0x50 works however) */
      BEGIN_RING_NI(chan, RING_MF(DATA), nr);
      OUT_RINGp (chan, src, nr);

      count -= nr;
      src += nr;
      offset += nr * 4;
   }
}

void
nvc0_m2mf_copy_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned dstoff, unsigned dstdom,
                      struct nouveau_bo *src, unsigned srcoff, unsigned srcdom,
                      unsigned size)
{
   struct nouveau_channel *chan = nv->screen->channel;

   while (size) {
      unsigned bytes = MIN2(size, 1 << 17);

      MARK_RING (chan, 11, 4);

      BEGIN_RING(chan, RING_MF(OFFSET_OUT_HIGH), 2);
      OUT_RELOCh(chan, dst, dstoff, dstdom | NOUVEAU_BO_WR);
      OUT_RELOCl(chan, dst, dstoff, dstdom | NOUVEAU_BO_WR);
      BEGIN_RING(chan, RING_MF(OFFSET_IN_HIGH), 2);
      OUT_RELOCh(chan, src, srcoff, srcdom | NOUVEAU_BO_RD);
      OUT_RELOCl(chan, src, srcoff, srcdom | NOUVEAU_BO_RD);
      BEGIN_RING(chan, RING_MF(LINE_LENGTH_IN), 2);
      OUT_RING  (chan, bytes);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_MF(EXEC), 1);
      OUT_RING  (chan, (1 << NVC0_M2MF_EXEC_INC__SHIFT) |
                 NVC0_M2MF_EXEC_LINEAR_IN | NVC0_M2MF_EXEC_LINEAR_OUT);

      srcoff += bytes;
      dstoff += bytes;
      size -= bytes;
   }
}

#if 0
static void
nvc0_m2mf_push_rect(struct pipe_screen *pscreen,
                    const struct nv50_m2mf_rect *dst,
                    const void *data,
                    unsigned nblocksx, unsigned nblocksy)
{
   struct nouveau_channel *chan;
   const uint8_t *src = (const uint8_t *)data;
   const int cpp = dst->cpp;
   const int line_len = nblocksx * cpp;
   int dy = dst->y;

   assert(nouveau_bo_tile_layout(dst->bo));

   BEGIN_RING(chan, RING_MF(TILING_MODE_OUT), 5);
   OUT_RING  (chan, dst->tile_mode);
   OUT_RING  (chan, dst->width * cpp);
   OUT_RING  (chan, dst->height);
   OUT_RING  (chan, dst->depth);
   OUT_RING  (chan, dst->z);

   while (nblocksy) {
      int line_count, words;
      int size = MIN2(AVAIL_RING(chan), NV04_PFIFO_MAX_PACKET_LEN);

      if (size < (12 + words)) {
         FIRE_RING(chan);
         continue;
      }
      line_count = (size * 4) / line_len;
      words = (line_count * line_len + 3) / 4;

      BEGIN_RING(chan, RING_MF(OFFSET_OUT_HIGH), 2);
      OUT_RELOCh(chan, dst->bo, dst->base, dst->domain | NOUVEAU_BO_WR);
      OUT_RELOCl(chan, dst->bo, dst->base, dst->domain | NOUVEAU_BO_WR);

      BEGIN_RING(chan, RING_MF(TILING_POSITION_OUT_X), 2);
      OUT_RING  (chan, dst->x * cpp);
      OUT_RING  (chan, dy);
      BEGIN_RING(chan, RING_MF(LINE_LENGTH_IN), 2);
      OUT_RING  (chan, line_len);
      OUT_RING  (chan, line_count);
      BEGIN_RING(chan, RING_MF(EXEC), 1);
      OUT_RING  (chan, (1 << NVC0_M2MF_EXEC_INC__SHIFT) |
                 NVC0_M2MF_EXEC_PUSH | NVC0_M2MF_EXEC_LINEAR_IN);

      BEGIN_RING_NI(chan, RING_MF(DATA), words);
      OUT_RINGp (chan, src, words);

      dy += line_count;
      src += line_len * line_count;
      nblocksy -= line_count;
   }
}
#endif

struct pipe_transfer *
nvc0_miptree_transfer_new(struct pipe_context *pctx,
                          struct pipe_resource *res,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box)
{
   struct nvc0_context *nvc0 = nvc0_context(pctx);
   struct pipe_screen *pscreen = pctx->screen;
   struct nouveau_device *dev = nvc0->screen->base.device;
   struct nv50_miptree *mt = nv50_miptree(res);
   struct nvc0_transfer *tx;
   uint32_t size;
   int ret;

   if (usage & (PIPE_TRANSFER_MAP_DIRECTLY | PIPE_TRANSFER_MAP_PERMANENTLY))
      return NULL;

   tx = CALLOC_STRUCT(nvc0_transfer);
   if (!tx)
      return NULL;

   pipe_resource_reference(&tx->base.resource, res);

   tx->base.level = level;
   tx->base.usage = usage;
   tx->base.box = *box;

   if (util_format_is_plain(res->format)) {
      tx->nblocksx = box->width << mt->ms_x;
      tx->nblocksy = box->height << mt->ms_y;
   } else {
      tx->nblocksx = util_format_get_nblocksx(res->format, box->width);
      tx->nblocksy = util_format_get_nblocksy(res->format, box->height);
   }
   tx->nlayers = box->depth;

   tx->base.stride = tx->nblocksx * util_format_get_blocksize(res->format);
   tx->base.layer_stride = tx->nblocksy * tx->base.stride;

   nv50_m2mf_rect_setup(&tx->rect[0], res, level, box->x, box->y, box->z);

   size = tx->base.layer_stride;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
                        size * tx->nlayers, &tx->rect[1].bo);
   if (ret) {
      FREE(tx);
      return NULL;
   }

   tx->rect[1].cpp = tx->rect[0].cpp;
   tx->rect[1].width = tx->nblocksx;
   tx->rect[1].height = tx->nblocksy;
   tx->rect[1].depth = 1;
   tx->rect[1].pitch = tx->base.stride;
   tx->rect[1].domain = NOUVEAU_BO_GART;

   if (usage & PIPE_TRANSFER_READ) {
      unsigned base = tx->rect[0].base;
      unsigned z = tx->rect[0].z;
      unsigned i;
      for (i = 0; i < tx->nlayers; ++i) {
         nvc0_m2mf_transfer_rect(pscreen, &tx->rect[1], &tx->rect[0],
                                 tx->nblocksx, tx->nblocksy);
         if (mt->layout_3d)
            tx->rect[0].z++;
         else
            tx->rect[0].base += mt->layer_stride;
         tx->rect[1].base += size;
      }
      tx->rect[0].z = z;
      tx->rect[0].base = base;
      tx->rect[1].base = 0;
   }

   return &tx->base;
}

void
nvc0_miptree_transfer_del(struct pipe_context *pctx,
                          struct pipe_transfer *transfer)
{
   struct pipe_screen *pscreen = pctx->screen;
   struct nvc0_transfer *tx = (struct nvc0_transfer *)transfer;
   struct nv50_miptree *mt = nv50_miptree(tx->base.resource);
   unsigned i;

   if (tx->base.usage & PIPE_TRANSFER_WRITE) {
      for (i = 0; i < tx->nlayers; ++i) {
         nvc0_m2mf_transfer_rect(pscreen, &tx->rect[0], &tx->rect[1],
                                 tx->nblocksx, tx->nblocksy);
         if (mt->layout_3d)
            tx->rect[0].z++;
         else
            tx->rect[0].base += mt->layer_stride;
         tx->rect[1].base += tx->nblocksy * tx->base.stride;
      }
   }

   nouveau_bo_ref(NULL, &tx->rect[1].bo);
   pipe_resource_reference(&transfer->resource, NULL);

   FREE(tx);
}

void *
nvc0_miptree_transfer_map(struct pipe_context *pctx,
                          struct pipe_transfer *transfer)
{
   struct nvc0_transfer *tx = (struct nvc0_transfer *)transfer;
   int ret;
   unsigned flags = 0;

   if (tx->rect[1].bo->map)
      return tx->rect[1].bo->map;

   if (transfer->usage & PIPE_TRANSFER_READ)
      flags = NOUVEAU_BO_RD;
   if (transfer->usage & PIPE_TRANSFER_WRITE)
      flags |= NOUVEAU_BO_WR;

   ret = nouveau_bo_map(tx->rect[1].bo, flags);
   if (ret)
      return NULL;
   return tx->rect[1].bo->map;
}

void
nvc0_miptree_transfer_unmap(struct pipe_context *pctx,
                            struct pipe_transfer *transfer)
{
   struct nvc0_transfer *tx = (struct nvc0_transfer *)transfer;

   nouveau_bo_unmap(tx->rect[1].bo);
}

void
nvc0_cb_push(struct nouveau_context *nv,
             struct nouveau_bo *bo, unsigned domain,
             unsigned base, unsigned size,
             unsigned offset, unsigned words, const uint32_t *data)
{
   struct nouveau_channel *chan = nv->screen->channel;

   assert(!(offset & 3));
   size = align(size, 0x100);

   MARK_RING (chan, 16, 2);
   BEGIN_RING(chan, RING_3D(CB_SIZE), 3);
   OUT_RING  (chan, size);
   OUT_RELOCh(chan, bo, base, domain | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, bo, base, domain | NOUVEAU_BO_WR);

   while (words) {
      unsigned nr = AVAIL_RING(chan);
      nr = MIN2(nr, words);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN - 1);

      BEGIN_RING_1I(chan, RING_3D(CB_POS), nr + 1);
      OUT_RING  (chan, offset);
      OUT_RINGp (chan, data, nr);

      words -= nr;
      data += nr;
      offset += nr * 4;

      if (words) {
         MARK_RING(chan, 6, 1);
         nouveau_bo_validate(chan, bo, domain | NOUVEAU_BO_WR);
      }
   }
}
