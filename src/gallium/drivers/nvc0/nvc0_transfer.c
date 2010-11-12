
#include "util/u_format.h"

#include "nvc0_context.h"
#include "nvc0_transfer.h"

#include "nv50_defs.xml.h"

struct nvc0_transfer {
   struct pipe_transfer base;
   struct nvc0_m2mf_rect rect[2];
   uint32_t nblocksx;
   uint32_t nblocksy;
};

static void
nvc0_m2mf_transfer_rect(struct pipe_screen *pscreen,
                        const struct nvc0_m2mf_rect *dst,
                        const struct nvc0_m2mf_rect *src,
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

   if (src->bo->tile_flags) {      
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

   if (dst->bo->tile_flags) {
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
nvc0_m2mf_push_linear(struct nvc0_context *nvc0,
                      struct nouveau_bo *dst, unsigned domain, int offset,
                      unsigned size, void *data)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   uint32_t *src = (uint32_t *)data;
   unsigned count = (size + 3) / 4;

   BEGIN_RING(chan, RING_MF(OFFSET_OUT_HIGH), 2);
   OUT_RELOCh(chan, dst, offset, domain | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, dst, offset, domain | NOUVEAU_BO_WR);
   BEGIN_RING(chan, RING_MF(LINE_LENGTH_IN), 2);
   OUT_RING  (chan, size);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_MF(EXEC), 1);
   OUT_RING  (chan, 0x100111);

   while (count) {
      unsigned nr = AVAIL_RING(chan);

      if (nr < 9) {
         FIRE_RING(chan);
         continue;
      }
      nr = MIN2(count, nr - 1);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN);
   
      BEGIN_RING_NI(chan, RING_MF(DATA), nr);
      OUT_RINGp (chan, src, nr);

      src += nr;
      count -= nr;
   }
}

static void
nvc0_sifc_push_rect(struct pipe_screen *pscreen,
                    const struct nvc0_m2mf_rect *dst, unsigned dst_format,
                    unsigned src_format, unsigned src_pitch, void *src,
                    unsigned nblocksx, unsigned nblocksy)
{
   struct nouveau_channel *chan;

   if (dst->bo->tile_flags) {
      BEGIN_RING(chan, RING_2D(DST_FORMAT), 5);
      OUT_RING  (chan, dst_format);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, dst->tile_mode);
      OUT_RING  (chan, 1);
      OUT_RING  (chan, 0);
   } else {
      BEGIN_RING(chan, RING_2D(DST_FORMAT), 2);
      OUT_RING  (chan, NV50_SURFACE_FORMAT_A8R8G8B8_UNORM);
      OUT_RING  (chan, 1);
      BEGIN_RING(chan, RING_2D(DST_PITCH), 1);
      OUT_RING  (chan, dst->pitch);
   }

   BEGIN_RING(chan, RING_2D(DST_WIDTH), 4);
   OUT_RING  (chan, dst->width);
   OUT_RING  (chan, dst->height);
   OUT_RELOCh(chan, dst->bo, dst->base, dst->domain | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, dst->bo, dst->base, dst->domain | NOUVEAU_BO_WR);

   BEGIN_RING(chan, RING_2D(SIFC_BITMAP_ENABLE), 2);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, src_format);
   BEGIN_RING(chan, RING_2D(SIFC_WIDTH), 10);
   OUT_RING  (chan, nblocksx);
   OUT_RING  (chan, nblocksy);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, dst->x);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, dst->y);

   while (nblocksy) {

      src = (uint8_t *)src + src_pitch;
   }
}

struct pipe_transfer *
nvc0_miptree_transfer_new(struct pipe_context *pctx,
                          struct pipe_resource *res,
                          struct pipe_subresource sr,
                          unsigned usage,
                          const struct pipe_box *box)
{
   struct nvc0_context *nvc0 = nvc0_context(pctx);
   struct pipe_screen *pscreen = pctx->screen;
   struct nouveau_device *dev = nvc0->screen->base.device;
   struct nvc0_miptree *mt = nvc0_miptree(res);
   struct nvc0_miptree_level *lvl = &mt->level[sr.level];
   struct nvc0_transfer *tx;
   uint32_t image;
   uint32_t w, h, z;
   int ret;

   if (res->target == PIPE_TEXTURE_CUBE)
      image = sr.face;
   else
      image = 0;

   tx = CALLOC_STRUCT(nvc0_transfer);
   if (!tx)
      return NULL;

   pipe_resource_reference(&tx->base.resource, res);

   tx->base.sr = sr;
   tx->base.usage = usage;
   tx->base.box = *box;

   tx->nblocksx = util_format_get_nblocksx(res->format, box->width);
   tx->nblocksy = util_format_get_nblocksy(res->format, box->height);

   tx->base.stride = tx->nblocksx * util_format_get_blocksize(res->format);

   w = u_minify(res->width0, sr.level);
   h = u_minify(res->height0, sr.level);

   tx->rect[0].cpp = tx->rect[1].cpp = util_format_get_blocksize(res->format);

   tx->rect[0].bo = mt->base.bo;
   tx->rect[0].base = lvl->image_offset[image];
   tx->rect[0].tile_mode = lvl->tile_mode;
   tx->rect[0].x = util_format_get_nblocksx(res->format, box->x);
   tx->rect[0].y = util_format_get_nblocksx(res->format, box->y);
   tx->rect[0].z = box->z;
   tx->rect[0].width = util_format_get_nblocksx(res->format, w);
   tx->rect[0].height = util_format_get_nblocksx(res->format, h);
   tx->rect[0].depth = res->depth0;
   tx->rect[0].pitch = lvl->pitch;
   tx->rect[0].domain = NOUVEAU_BO_VRAM;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
                        tx->nblocksy * tx->base.stride, &tx->rect[1].bo);
   if (ret) {
      FREE(tx);
      return NULL;
   }

   tx->rect[1].width = tx->nblocksx;
   tx->rect[1].height = tx->nblocksy;
   tx->rect[1].depth = box->depth;
   tx->rect[1].pitch = tx->base.stride;
   tx->rect[1].domain = NOUVEAU_BO_GART;

   if (usage & PIPE_TRANSFER_READ) {
      for (z = 0; z < box->depth; ++z) {
         nvc0_m2mf_transfer_rect(pscreen, &tx->rect[1], &tx->rect[0],
                                 tx->nblocksx, tx->nblocksy);
         tx->rect[0].z++;
      }
   }
   tx->rect[0].z = box->z;

   return &tx->base;
}

void
nvc0_miptree_transfer_del(struct pipe_context *pctx,
                          struct pipe_transfer *transfer)
{
   struct pipe_screen *pscreen = pctx->screen;
   struct nvc0_transfer *tx = (struct nvc0_transfer *)transfer;
   unsigned z;

   if (tx->base.usage & PIPE_TRANSFER_WRITE) {
      for (z = 0; z < tx->base.box.depth; ++z) {
         nvc0_m2mf_transfer_rect(pscreen, &tx->rect[0], &tx->rect[1],
                                 tx->nblocksx, tx->nblocksy);
         tx->rect[0].z++;
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

