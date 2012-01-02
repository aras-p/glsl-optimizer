
#include "util/u_format.h"

#include "nv50_context.h"

#include "nv50_defs.xml.h"

struct nv50_transfer {
   struct pipe_transfer base;
   struct nv50_m2mf_rect rect[2];
   uint32_t nblocksx;
   uint32_t nblocksy;
};

void
nv50_m2mf_rect_setup(struct nv50_m2mf_rect *rect,
                     struct pipe_resource *restrict res, unsigned l,
                     unsigned x, unsigned y, unsigned z)
{
   struct nv50_miptree *mt = nv50_miptree(res);
   const unsigned w = u_minify(res->width0, l);
   const unsigned h = u_minify(res->height0, l);

   rect->bo = mt->base.bo;
   rect->domain = mt->base.domain;
   rect->base = mt->level[l].offset;
   rect->pitch = mt->level[l].pitch;
   if (util_format_is_plain(res->format)) {
      rect->width = w << mt->ms_x;
      rect->height = h << mt->ms_y;
      rect->x = x << mt->ms_x;
      rect->y = y << mt->ms_y;
   } else {
      rect->width = util_format_get_nblocksx(res->format, w);
      rect->height = util_format_get_nblocksy(res->format, h);
      rect->x = util_format_get_nblocksx(res->format, x);
      rect->y = util_format_get_nblocksy(res->format, y);
   }
   rect->tile_mode = mt->level[l].tile_mode;
   rect->cpp = util_format_get_blocksize(res->format);

   if (mt->layout_3d) {
      rect->z = z;
      rect->depth = u_minify(res->depth0, l);
   } else {
      rect->base += z * mt->layer_stride;
      rect->z = 0;
      rect->depth = 1;
   }
}

void
nv50_m2mf_transfer_rect(struct pipe_screen *pscreen,
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

   assert(dst->cpp == src->cpp);

   if (nouveau_bo_tile_layout(src->bo)) {
      BEGIN_RING(chan, RING_MF(LINEAR_IN), 6);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, src->tile_mode << 4);
      OUT_RING  (chan, src->width * cpp);
      OUT_RING  (chan, src->height);
      OUT_RING  (chan, src->depth);
      OUT_RING  (chan, src->z);
   } else {
      src_ofst += src->y * src->pitch + src->x * cpp;

      BEGIN_RING(chan, RING_MF(LINEAR_IN), 1);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_MF_(NV04_M2MF_PITCH_IN), 1);
      OUT_RING  (chan, src->pitch);
   }

   if (nouveau_bo_tile_layout(dst->bo)) {
      BEGIN_RING(chan, RING_MF(LINEAR_OUT), 6);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, dst->tile_mode << 4);
      OUT_RING  (chan, dst->width * cpp);
      OUT_RING  (chan, dst->height);
      OUT_RING  (chan, dst->depth);
      OUT_RING  (chan, dst->z);
   } else {
      dst_ofst += dst->y * dst->pitch + dst->x * cpp;

      BEGIN_RING(chan, RING_MF(LINEAR_OUT), 1);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_MF_(NV04_M2MF_PITCH_OUT), 1);
      OUT_RING  (chan, dst->pitch);
   }

   while (height) {
      int line_count = height > 2047 ? 2047 : height;

      MARK_RING (chan, 17, 4);

      BEGIN_RING(chan, RING_MF(OFFSET_IN_HIGH), 2);
      OUT_RELOCh(chan, src->bo, src_ofst, src->domain | NOUVEAU_BO_RD);
      OUT_RELOCh(chan, dst->bo, dst_ofst, dst->domain | NOUVEAU_BO_WR);

      BEGIN_RING(chan, RING_MF_(NV04_M2MF_OFFSET_IN), 2);
      OUT_RELOCl(chan, src->bo, src_ofst, src->domain | NOUVEAU_BO_RD);
      OUT_RELOCl(chan, dst->bo, dst_ofst, dst->domain | NOUVEAU_BO_WR);

      if (nouveau_bo_tile_layout(src->bo)) {
         BEGIN_RING(chan, RING_MF(TILING_POSITION_IN), 1);
         OUT_RING  (chan, (sy << 16) | (src->x * cpp));
      } else {
         src_ofst += line_count * src->pitch;
      }
      if (nouveau_bo_tile_layout(dst->bo)) {
         BEGIN_RING(chan, RING_MF(TILING_POSITION_OUT), 1);
         OUT_RING  (chan, (dy << 16) | (dst->x * cpp));
      } else {
         dst_ofst += line_count * dst->pitch;
      }

      BEGIN_RING(chan, RING_MF_(NV04_M2MF_LINE_LENGTH_IN), 4);
      OUT_RING  (chan, nblocksx * cpp);
      OUT_RING  (chan, line_count);
      OUT_RING  (chan, (1 << 8) | (1 << 0));
      OUT_RING  (chan, 0);

      height -= line_count;
      sy += line_count;
      dy += line_count;
   }
}

void
nv50_sifc_linear_u8(struct nouveau_context *nv,
                    struct nouveau_bo *dst, unsigned offset, unsigned domain,
                    unsigned size, const void *data)
{
   struct nouveau_channel *chan = nv->screen->channel;
   uint32_t *src = (uint32_t *)data;
   unsigned count = (size + 3) / 4;
   unsigned xcoord = offset & 0xff;

   offset &= ~0xff;

   MARK_RING (chan, 23, 4);
   BEGIN_RING(chan, RING_2D(DST_FORMAT), 2);
   OUT_RING  (chan, NV50_SURFACE_FORMAT_R8_UNORM);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_2D(DST_PITCH), 5);
   OUT_RING  (chan, 262144);
   OUT_RING  (chan, 65536);
   OUT_RING  (chan, 1);
   OUT_RELOCh(chan, dst, offset, domain | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, dst, offset, domain | NOUVEAU_BO_WR);
   BEGIN_RING(chan, RING_2D(SIFC_BITMAP_ENABLE), 2);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, NV50_SURFACE_FORMAT_R8_UNORM);
   BEGIN_RING(chan, RING_2D(SIFC_WIDTH), 10);
   OUT_RING  (chan, size);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, xcoord);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 0);

   while (count) {
      unsigned nr = AVAIL_RING(chan);

      if (nr < 9) {
         FIRE_RING(chan);
         nouveau_bo_validate(chan, dst, NOUVEAU_BO_WR);
         continue;
      }
      nr = MIN2(count, nr - 1);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN);

      BEGIN_RING_NI(chan, RING_2D(SIFC_DATA), nr);
      OUT_RINGp (chan, src, nr);

      src += nr;
      count -= nr;
   }
}

void
nv50_m2mf_copy_linear(struct nouveau_context *nv,
                      struct nouveau_bo *dst, unsigned dstoff, unsigned dstdom,
                      struct nouveau_bo *src, unsigned srcoff, unsigned srcdom,
                      unsigned size)
{
   struct nouveau_channel *chan = nv->screen->channel;

   BEGIN_RING(chan, RING_MF(LINEAR_IN), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_MF(LINEAR_OUT), 1);
   OUT_RING  (chan, 1);

   while (size) {
      unsigned bytes = MIN2(size, 1 << 17);

      MARK_RING (chan, 11, 4);
      BEGIN_RING(chan, RING_MF(OFFSET_IN_HIGH), 2);
      OUT_RELOCh(chan, src, srcoff, srcdom | NOUVEAU_BO_RD);
      OUT_RELOCh(chan, dst, dstoff, dstdom | NOUVEAU_BO_WR);
      BEGIN_RING(chan, RING_MF_(NV04_M2MF_OFFSET_IN), 2);
      OUT_RELOCl(chan, src, srcoff, srcdom | NOUVEAU_BO_RD);
      OUT_RELOCl(chan, dst, dstoff, dstdom | NOUVEAU_BO_WR);
      BEGIN_RING(chan, RING_MF_(NV04_M2MF_LINE_LENGTH_IN), 4);
      OUT_RING  (chan, bytes);
      OUT_RING  (chan, 1);
      OUT_RING  (chan, (1 << 8) | (1 << 0));
      OUT_RING  (chan, 0);

      srcoff += bytes;
      dstoff += bytes;
      size -= bytes;
   }
}

struct pipe_transfer *
nv50_miptree_transfer_new(struct pipe_context *pctx,
                          struct pipe_resource *res,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box)
{
   struct nv50_context *nv50 = nv50_context(pctx);
   struct pipe_screen *pscreen = pctx->screen;
   struct nouveau_device *dev = nv50->screen->base.device;
   const struct nv50_miptree *mt = nv50_miptree(res);
   struct nv50_transfer *tx;
   uint32_t size;
   int ret;

   if (usage & (PIPE_TRANSFER_MAP_DIRECTLY | PIPE_TRANSFER_MAP_PERMANENTLY))
      return NULL;

   tx = CALLOC_STRUCT(nv50_transfer);
   if (!tx)
      return NULL;

   pipe_resource_reference(&tx->base.resource, res);

   tx->base.level = level;
   tx->base.usage = usage;
   tx->base.box = *box;

   if (util_format_is_plain(res->format)) {
      tx->nblocksx = box->width << mt->ms_x;
      tx->nblocksy = box->height << mt->ms_x;
   } else {
      tx->nblocksx = util_format_get_nblocksx(res->format, box->width);
      tx->nblocksy = util_format_get_nblocksy(res->format, box->height);
   }

   tx->base.stride = tx->nblocksx * util_format_get_blocksize(res->format);
   tx->base.layer_stride = tx->nblocksy * tx->base.stride;

   nv50_m2mf_rect_setup(&tx->rect[0], res, level, box->x, box->y, box->z);

   size = tx->base.layer_stride;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
                        size * tx->base.box.depth, &tx->rect[1].bo);
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
      for (i = 0; i < box->depth; ++i) {
         nv50_m2mf_transfer_rect(pscreen, &tx->rect[1], &tx->rect[0],
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
nv50_miptree_transfer_del(struct pipe_context *pctx,
                          struct pipe_transfer *transfer)
{
   struct pipe_screen *pscreen = pctx->screen;
   struct nv50_transfer *tx = (struct nv50_transfer *)transfer;
   struct nv50_miptree *mt = nv50_miptree(tx->base.resource);
   unsigned i;

   if (tx->base.usage & PIPE_TRANSFER_WRITE) {
      for (i = 0; i < tx->base.box.depth; ++i) {
         nv50_m2mf_transfer_rect(pscreen, &tx->rect[0], &tx->rect[1],
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
nv50_miptree_transfer_map(struct pipe_context *pctx,
                          struct pipe_transfer *transfer)
{
   struct nv50_transfer *tx = (struct nv50_transfer *)transfer;
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
nv50_miptree_transfer_unmap(struct pipe_context *pctx,
                            struct pipe_transfer *transfer)
{
   struct nv50_transfer *tx = (struct nv50_transfer *)transfer;

   nouveau_bo_unmap(tx->rect[1].bo);
}

void
nv50_cb_push(struct nouveau_context *nv,
             struct nouveau_bo *bo, unsigned domain,
             unsigned base, unsigned size,
             unsigned offset, unsigned words, const uint32_t *data)
{
   struct nouveau_channel *chan = nv->screen->channel;

   assert(!(offset & 3));
   size = align(size, 0x100);

   while (words) {
      unsigned nr;

      MARK_RING(chan, 24, 2);
      nr = AVAIL_RING(chan);
      nr = MIN2(nr - 7, words);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN - 1);

      BEGIN_RING(chan, RING_3D(CB_DEF_ADDRESS_HIGH), 3);
      OUT_RELOCh(chan, bo, base, domain | NOUVEAU_BO_WR);
      OUT_RELOCl(chan, bo, base, domain | NOUVEAU_BO_WR);
      OUT_RING  (chan, (NV50_CB_TMP << 16) | (size & 0xffff));
      BEGIN_RING(chan, RING_3D(CB_ADDR), 1);
      OUT_RING  (chan, (offset << 6) | NV50_CB_TMP);
      BEGIN_RING_NI(chan, RING_3D(CB_DATA(0)), nr);
      OUT_RINGp (chan, data, nr);

      words -= nr;
      data += nr;
      offset += nr * 4;
   }
}
