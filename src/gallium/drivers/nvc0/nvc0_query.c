/*
 * Copyright 2011 Nouveau Project
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
 *
 * Authors: Christoph Bumiller
 */

#include "nvc0_context.h"
#include "nouveau/nv_object.xml.h"

struct nvc0_query {
   uint32_t *data;
   uint16_t type;
   uint16_t index;
   uint32_t sequence;
   struct nouveau_bo *bo;
   uint32_t base;
   uint32_t offset; /* base + i * rotate */
   boolean ready;
   boolean active;
   boolean is64bit;
   uint8_t rotate;
   int nesting; /* only used for occlusion queries */
   struct nouveau_mm_allocation *mm;
};

#define NVC0_QUERY_ALLOC_SPACE 256

static INLINE struct nvc0_query *
nvc0_query(struct pipe_query *pipe)
{
   return (struct nvc0_query *)pipe;
}

static boolean
nvc0_query_allocate(struct nvc0_context *nvc0, struct nvc0_query *q, int size)
{
   struct nvc0_screen *screen = nvc0->screen;
   int ret;

   if (q->bo) {
      nouveau_bo_ref(NULL, &q->bo);
      if (q->mm) {
         if (q->ready)
            nouveau_mm_free(q->mm);
         else
            nouveau_fence_work(screen->base.fence.current,
                               nouveau_mm_free_work, q->mm);
      }
   }
   if (size) {
      q->mm = nouveau_mm_allocate(screen->base.mm_GART, size, &q->bo, &q->base);
      if (!q->bo)
         return FALSE;
      q->offset = q->base;

      ret = nouveau_bo_map_range(q->bo, q->base, size, NOUVEAU_BO_RD |
                                 NOUVEAU_BO_NOSYNC);
      if (ret) {
         nvc0_query_allocate(nvc0, q, 0);
         return FALSE;
      }
      q->data = q->bo->map;
      nouveau_bo_unmap(q->bo);
   }
   return TRUE;
}

static void
nvc0_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
   nvc0_query_allocate(nvc0_context(pipe), nvc0_query(pq), 0);
   FREE(nvc0_query(pq));
}

static struct pipe_query *
nvc0_query_create(struct pipe_context *pipe, unsigned type)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_query *q;
   unsigned space = NVC0_QUERY_ALLOC_SPACE;

   q = CALLOC_STRUCT(nvc0_query);
   if (!q)
      return NULL;

   switch (type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      q->rotate = 32;
      space = NVC0_QUERY_ALLOC_SPACE;
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      q->is64bit = TRUE;
      space = 512;
      break;
   case PIPE_QUERY_SO_STATISTICS:
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      q->is64bit = TRUE;
      space = 64;
      break;
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_GPU_FINISHED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      space = 32;
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      space = 16;
      break;
   default:
      FREE(q);
      return NULL;
   }
   if (!nvc0_query_allocate(nvc0, q, space)) {
      FREE(q);
      return NULL;
   }

   q->type = type;

   if (q->rotate) {
      /* we advance before query_begin ! */
      q->offset -= q->rotate;
      q->data -= q->rotate / sizeof(*q->data);
   } else
   if (!q->is64bit)
      q->data[0] = 0; /* initialize sequence */

   return (struct pipe_query *)q;
}

static void
nvc0_query_get(struct nouveau_channel *chan, struct nvc0_query *q,
               unsigned offset, uint32_t get)
{
   offset += q->offset;

   MARK_RING (chan, 5, 2);
   BEGIN_RING(chan, RING_3D(QUERY_ADDRESS_HIGH), 4);
   OUT_RELOCh(chan, q->bo, offset, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   OUT_RELOCl(chan, q->bo, offset, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   OUT_RING  (chan, q->sequence);
   OUT_RING  (chan, get);
}

static void
nvc0_query_rotate(struct nvc0_context *nvc0, struct nvc0_query *q)
{
   q->offset += q->rotate;
   q->data += q->rotate / sizeof(*q->data);
   if (q->offset - q->base == NVC0_QUERY_ALLOC_SPACE)
      nvc0_query_allocate(nvc0, q, NVC0_QUERY_ALLOC_SPACE);
}

static void
nvc0_query_begin(struct pipe_context *pipe, struct pipe_query *pq)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_query *q = nvc0_query(pq);

   /* For occlusion queries we have to change the storage, because a previous
    * query might set the initial render conition to FALSE even *after* we re-
    * initialized it to TRUE.
    */
   if (q->rotate) {
      nvc0_query_rotate(nvc0, q);

      /* XXX: can we do this with the GPU, and sync with respect to a previous
       *  query ?
       */
      q->data[0] = q->sequence; /* initialize sequence */
      q->data[1] = 1; /* initial render condition = TRUE */
      q->data[4] = q->sequence + 1; /* for comparison COND_MODE */
      q->data[5] = 0;
   }
   q->sequence++;

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      q->nesting = nvc0->screen->num_occlusion_queries_active++;
      if (q->nesting) {
         nvc0_query_get(chan, q, 0x10, 0x0100f002);
      } else {
         BEGIN_RING(chan, RING_3D(COUNTER_RESET), 1);
         OUT_RING  (chan, NVC0_3D_COUNTER_RESET_SAMPLECNT);
         IMMED_RING(chan, RING_3D(SAMPLECNT_ENABLE), 1);
      }
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_query_get(chan, q, 0x10, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_query_get(chan, q, 0x10, 0x05805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nvc0_query_get(chan, q, 0x20, 0x05805002 | (q->index << 5));
      nvc0_query_get(chan, q, 0x30, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      nvc0_query_get(chan, q, 0x10, 0x03005002 | (q->index << 5));
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nvc0_query_get(chan, q, 0x10, 0x00005002);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_query_get(chan, q, 0xc0 + 0x00, 0x00801002); /* VFETCH, VERTICES */
      nvc0_query_get(chan, q, 0xc0 + 0x10, 0x01801002); /* VFETCH, PRIMS */
      nvc0_query_get(chan, q, 0xc0 + 0x20, 0x02802002); /* VP, LAUNCHES */
      nvc0_query_get(chan, q, 0xc0 + 0x30, 0x03806002); /* GP, LAUNCHES */
      nvc0_query_get(chan, q, 0xc0 + 0x40, 0x04806002); /* GP, PRIMS_OUT */
      nvc0_query_get(chan, q, 0xc0 + 0x50, 0x07804002); /* RAST, PRIMS_IN */
      nvc0_query_get(chan, q, 0xc0 + 0x60, 0x08804002); /* RAST, PRIMS_OUT */
      nvc0_query_get(chan, q, 0xc0 + 0x70, 0x0980a002); /* ROP, PIXELS */
      nvc0_query_get(chan, q, 0xc0 + 0x80, 0x0d808002); /* TCP, LAUNCHES */
      nvc0_query_get(chan, q, 0xc0 + 0x90, 0x0e809002); /* TEP, LAUNCHES */
      break;
   default:
      break;
   }
   q->ready = FALSE;
   q->active = TRUE;
}

static void
nvc0_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_query *q = nvc0_query(pq);

   if (!q->active) {
      /* some queries don't require 'begin' to be called (e.g. GPU_FINISHED) */
      if (q->rotate)
         nvc0_query_rotate(nvc0, q);
      q->sequence++;
   }
   q->ready = FALSE;
   q->active = FALSE;

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      nvc0_query_get(chan, q, 0, 0x0100f002);
      if (--nvc0->screen->num_occlusion_queries_active == 0)
         IMMED_RING(chan, RING_3D(SAMPLECNT_ENABLE), 0);
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_query_get(chan, q, 0, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_query_get(chan, q, 0, 0x05805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nvc0_query_get(chan, q, 0x00, 0x05805002 | (q->index << 5));
      nvc0_query_get(chan, q, 0x10, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      /* TODO: How do we sum over all streams for render condition ? */
      /* PRIMS_DROPPED doesn't write sequence, use a ZERO query to sync on */
      nvc0_query_get(chan, q, 0x00, 0x03005002 | (q->index << 5));
      nvc0_query_get(chan, q, 0x20, 0x00005002);
      break;
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nvc0_query_get(chan, q, 0, 0x00005002);
      break;
   case PIPE_QUERY_GPU_FINISHED:
      nvc0_query_get(chan, q, 0, 0x1000f010);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_query_get(chan, q, 0x00, 0x00801002); /* VFETCH, VERTICES */
      nvc0_query_get(chan, q, 0x10, 0x01801002); /* VFETCH, PRIMS */
      nvc0_query_get(chan, q, 0x20, 0x02802002); /* VP, LAUNCHES */
      nvc0_query_get(chan, q, 0x30, 0x03806002); /* GP, LAUNCHES */
      nvc0_query_get(chan, q, 0x40, 0x04806002); /* GP, PRIMS_OUT */
      nvc0_query_get(chan, q, 0x50, 0x07804002); /* RAST, PRIMS_IN */
      nvc0_query_get(chan, q, 0x60, 0x08804002); /* RAST, PRIMS_OUT */
      nvc0_query_get(chan, q, 0x70, 0x0980a002); /* ROP, PIXELS */
      nvc0_query_get(chan, q, 0x80, 0x0d808002); /* TCP, LAUNCHES */
      nvc0_query_get(chan, q, 0x90, 0x0e809002); /* TEP, LAUNCHES */
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      /* indexed by TFB buffer instead of by vertex stream */
      nvc0_query_get(chan, q, 0x00, 0x0d005002 | (q->index << 5));
      break;
   default:
      assert(0);
      break;
   }
}

static INLINE boolean
nvc0_query_ready(struct nvc0_query *q)
{
   if (q->is64bit) {
      if (nouveau_bo_map(q->bo, NOUVEAU_BO_RD | NOUVEAU_BO_NOWAIT))
         return FALSE;
      nouveau_bo_unmap(q->bo);
      return TRUE;
   } else {
      return q->data[0] == q->sequence;
   }
}

static INLINE boolean
nvc0_query_wait(struct nvc0_query *q)
{
   int ret = nouveau_bo_map(q->bo, NOUVEAU_BO_RD);
   if (ret)
      return FALSE;
   nouveau_bo_unmap(q->bo);
   return TRUE;
}

static boolean
nvc0_query_result(struct pipe_context *pipe, struct pipe_query *pq,
                  boolean wait, void *result)
{
   struct nvc0_query *q = nvc0_query(pq);
   uint64_t *res64 = result;
   uint32_t *res32 = result;
   boolean *res8 = result;
   uint64_t *data64 = (uint64_t *)q->data;
   unsigned i;

   if (!q->ready) /* update ? */
      q->ready = nvc0_query_ready(q);
   if (!q->ready) {
      struct nouveau_channel *chan = nvc0_context(pipe)->screen->base.channel;
      if (!wait) {
         if (nouveau_bo_pending(q->bo) & NOUVEAU_BO_WR) /* for daft apps */
            FIRE_RING(chan);
         return FALSE;
      }
      if (!nvc0_query_wait(q))
         return FALSE;
   }
   q->ready = TRUE;

   switch (q->type) {
   case PIPE_QUERY_GPU_FINISHED:
      res8[0] = TRUE;
      break;
   case PIPE_QUERY_OCCLUSION_COUNTER: /* u32 sequence, u32 count, u64 time */
      res64[0] = q->data[1] - q->data[5];
      break;
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      res8[0] = q->data[1] != q->data[5];
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED: /* u64 count, u64 time */
   case PIPE_QUERY_PRIMITIVES_EMITTED: /* u64 count, u64 time */
      res64[0] = data64[0] - data64[2];
      break;
   case PIPE_QUERY_SO_STATISTICS:
      res64[0] = data64[0] - data64[4];
      res64[1] = data64[2] - data64[6];
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      res8[0] = data64[0] != data64[2];
      break;
   case PIPE_QUERY_TIMESTAMP:
      res64[0] = data64[1];
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT: /* u32 sequence, u32 0, u64 time */
      res64[0] = 1000000000;
      res8[8] = (data64[1] == data64[3]) ? FALSE : TRUE;
      break;
   case PIPE_QUERY_TIME_ELAPSED:
      res64[0] = data64[1] - data64[3];
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      for (i = 0; i < 10; ++i)
         res64[i] = data64[i * 2] - data64[24 + i * 2];
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      res32[0] = q->data[1];
      break;
   default:
      return FALSE;
   }

   return TRUE;
}

void
nvc0_query_fifo_wait(struct nouveau_channel *chan, struct pipe_query *pq)
{
   struct nvc0_query *q = nvc0_query(pq);
   unsigned offset = q->offset;

   if (q->type == PIPE_QUERY_SO_OVERFLOW_PREDICATE) offset += 0x20;

   MARK_RING (chan, 5, 2);
   BEGIN_RING(chan, RING_3D_(NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH), 4);
   OUT_RELOCh(chan, q->bo, offset, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, q->bo, offset, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   OUT_RING  (chan, q->sequence);
   OUT_RING  (chan, (1 << 12) |
              NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_EQUAL);
}

static void
nvc0_render_condition(struct pipe_context *pipe,
                      struct pipe_query *pq, uint mode)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_query *q;
   uint32_t cond;
   boolean negated = FALSE;
   boolean wait =
      mode != PIPE_RENDER_COND_NO_WAIT &&
      mode != PIPE_RENDER_COND_BY_REGION_NO_WAIT;

   if (!pq) {
      IMMED_RING(chan, RING_3D(COND_MODE), NVC0_3D_COND_MODE_ALWAYS);
      return;
   }
   q = nvc0_query(pq);

   /* NOTE: comparison of 2 queries only works if both have completed */
   switch (q->type) {
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      cond = negated ? NVC0_3D_COND_MODE_EQUAL :
                       NVC0_3D_COND_MODE_NOT_EQUAL;
      wait = TRUE;
      break;
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      if (likely(!negated)) {
         if (unlikely(q->nesting))
            cond = wait ? NVC0_3D_COND_MODE_NOT_EQUAL :
                          NVC0_3D_COND_MODE_ALWAYS;
         else
            cond = NVC0_3D_COND_MODE_RES_NON_ZERO;
      } else {
         cond = wait ? NVC0_3D_COND_MODE_EQUAL : NVC0_3D_COND_MODE_ALWAYS;
      }
      break;
   default:
      assert(!"render condition query not a predicate");
      mode = NVC0_3D_COND_MODE_ALWAYS;
      break;
   }

   if (wait)
      nvc0_query_fifo_wait(chan, pq);

   MARK_RING (chan, 4, 2);
   BEGIN_RING(chan, RING_3D(COND_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, q->bo, q->offset, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, q->bo, q->offset, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   OUT_RING  (chan, cond);
}

void
nvc0_query_pushbuf_submit(struct nouveau_channel *chan,
                          struct pipe_query *pq, unsigned result_offset)
{
   struct nvc0_query *q = nvc0_query(pq);

#define NVC0_IB_ENTRY_1_NO_PREFETCH (1 << (31 - 8))

   nouveau_pushbuf_submit(chan, q->bo, q->offset + result_offset, 4 |
                          NVC0_IB_ENTRY_1_NO_PREFETCH);
}

void
nvc0_so_target_save_offset(struct pipe_context *pipe,
                           struct pipe_stream_output_target *ptarg,
                           unsigned index, boolean *serialize)
{
   struct nvc0_so_target *targ = nvc0_so_target(ptarg);

   if (*serialize) {
      struct nouveau_channel *chan = nvc0_context(pipe)->screen->base.channel;
      *serialize = FALSE;
      IMMED_RING(chan, RING_3D(SERIALIZE), 0);
   }

   nvc0_query(targ->pq)->index = index;

   nvc0_query_end(pipe, targ->pq);
}

void
nvc0_init_query_functions(struct nvc0_context *nvc0)
{
   struct pipe_context *pipe = &nvc0->base.pipe;

   pipe->create_query = nvc0_query_create;
   pipe->destroy_query = nvc0_query_destroy;
   pipe->begin_query = nvc0_query_begin;
   pipe->end_query = nvc0_query_end;
   pipe->get_query_result = nvc0_query_result;
   pipe->render_condition = nvc0_render_condition;
}
