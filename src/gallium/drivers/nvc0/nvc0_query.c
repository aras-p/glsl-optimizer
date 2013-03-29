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

#define NVC0_PUSH_EXPLICIT_SPACE_CHECKING

#include "nvc0_context.h"
#include "nouveau/nv_object.xml.h"
#include "nve4_compute.xml.h"

#define NVC0_QUERY_STATE_READY   0
#define NVC0_QUERY_STATE_ACTIVE  1
#define NVC0_QUERY_STATE_ENDED   2
#define NVC0_QUERY_STATE_FLUSHED 3

struct nvc0_query {
   uint32_t *data;
   uint16_t type;
   uint16_t index;
   int8_t ctr[4];
   uint32_t sequence;
   struct nouveau_bo *bo;
   uint32_t base;
   uint32_t offset; /* base + i * rotate */
   uint8_t state;
   boolean is64bit;
   uint8_t rotate;
   int nesting; /* only used for occlusion queries */
   struct nouveau_fence *fence;
   struct nouveau_mm_allocation *mm;
};

#define NVC0_QUERY_ALLOC_SPACE 256

static void nve4_mp_pm_query_begin(struct nvc0_context *, struct nvc0_query *);
static void nve4_mp_pm_query_end(struct nvc0_context *, struct nvc0_query *);
static boolean nve4_mp_pm_query_result(struct nvc0_context *,
                                       struct nvc0_query *, void *, boolean);

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
         if (q->state == NVC0_QUERY_STATE_READY)
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

      ret = nouveau_bo_map(q->bo, 0, screen->base.client);
      if (ret) {
         nvc0_query_allocate(nvc0, q, 0);
         return FALSE;
      }
      q->data = (uint32_t *)((uint8_t *)q->bo->map + q->base);
   }
   return TRUE;
}

static void
nvc0_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
   nvc0_query_allocate(nvc0_context(pipe), nvc0_query(pq), 0);
   nouveau_fence_ref(NULL, &nvc0_query(pq)->fence);
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
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      q->is64bit = TRUE;
      space = 32;
      break;
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_GPU_FINISHED:
      space = 32;
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      space = 16;
      break;
   default:
      if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS &&
          nvc0->screen->base.device->drm_version >= 0x01000101) {
         if (type >= NVE4_PM_QUERY(0) &&
             type <= NVE4_PM_QUERY_MAX) {
            /* 8 counters per MP + clock */
            space = 12 * nvc0->screen->mp_count * sizeof(uint32_t);
            break;
         }
      }
      debug_printf("invalid query type: %u\n", type);
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
nvc0_query_get(struct nouveau_pushbuf *push, struct nvc0_query *q,
               unsigned offset, uint32_t get)
{
   offset += q->offset;

   PUSH_SPACE(push, 5);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   BEGIN_NVC0(push, NVC0_3D(QUERY_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, q->bo->offset + offset);
   PUSH_DATA (push, q->bo->offset + offset);
   PUSH_DATA (push, q->sequence);
   PUSH_DATA (push, get);
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
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
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
         nvc0_query_get(push, q, 0x10, 0x0100f002);
      } else {
         PUSH_SPACE(push, 3);
         BEGIN_NVC0(push, NVC0_3D(COUNTER_RESET), 1);
         PUSH_DATA (push, NVC0_3D_COUNTER_RESET_SAMPLECNT);
         IMMED_NVC0(push, NVC0_3D(SAMPLECNT_ENABLE), 1);
      }
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_query_get(push, q, 0x10, 0x09005002 | (q->index << 5));
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_query_get(push, q, 0x10, 0x05805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nvc0_query_get(push, q, 0x20, 0x05805002 | (q->index << 5));
      nvc0_query_get(push, q, 0x30, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      nvc0_query_get(push, q, 0x10, 0x03005002 | (q->index << 5));
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nvc0_query_get(push, q, 0x10, 0x00005002);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_query_get(push, q, 0xc0 + 0x00, 0x00801002); /* VFETCH, VERTICES */
      nvc0_query_get(push, q, 0xc0 + 0x10, 0x01801002); /* VFETCH, PRIMS */
      nvc0_query_get(push, q, 0xc0 + 0x20, 0x02802002); /* VP, LAUNCHES */
      nvc0_query_get(push, q, 0xc0 + 0x30, 0x03806002); /* GP, LAUNCHES */
      nvc0_query_get(push, q, 0xc0 + 0x40, 0x04806002); /* GP, PRIMS_OUT */
      nvc0_query_get(push, q, 0xc0 + 0x50, 0x07804002); /* RAST, PRIMS_IN */
      nvc0_query_get(push, q, 0xc0 + 0x60, 0x08804002); /* RAST, PRIMS_OUT */
      nvc0_query_get(push, q, 0xc0 + 0x70, 0x0980a002); /* ROP, PIXELS */
      nvc0_query_get(push, q, 0xc0 + 0x80, 0x0d808002); /* TCP, LAUNCHES */
      nvc0_query_get(push, q, 0xc0 + 0x90, 0x0e809002); /* TEP, LAUNCHES */
      break;
   default:
      if (q->type >= NVE4_PM_QUERY(0) && q->type <= NVE4_PM_QUERY_MAX)
         nve4_mp_pm_query_begin(nvc0, q);
      break;
   }
   q->state = NVC0_QUERY_STATE_ACTIVE;
}

static void
nvc0_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_query *q = nvc0_query(pq);

   if (q->state != NVC0_QUERY_STATE_ACTIVE) {
      /* some queries don't require 'begin' to be called (e.g. GPU_FINISHED) */
      if (q->rotate)
         nvc0_query_rotate(nvc0, q);
      q->sequence++;
   }
   q->state = NVC0_QUERY_STATE_ENDED;

   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      nvc0_query_get(push, q, 0, 0x0100f002);
      if (--nvc0->screen->num_occlusion_queries_active == 0) {
         PUSH_SPACE(push, 1);
         IMMED_NVC0(push, NVC0_3D(SAMPLECNT_ENABLE), 0);
      }
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_query_get(push, q, 0, 0x09005002 | (q->index << 5));
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_query_get(push, q, 0, 0x05805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_STATISTICS:
      nvc0_query_get(push, q, 0x00, 0x05805002 | (q->index << 5));
      nvc0_query_get(push, q, 0x10, 0x06805002 | (q->index << 5));
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      /* TODO: How do we sum over all streams for render condition ? */
      /* PRIMS_DROPPED doesn't write sequence, use a ZERO query to sync on */
      nvc0_query_get(push, q, 0x00, 0x03005002 | (q->index << 5));
      nvc0_query_get(push, q, 0x20, 0x00005002);
      break;
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_TIME_ELAPSED:
      nvc0_query_get(push, q, 0, 0x00005002);
      break;
   case PIPE_QUERY_GPU_FINISHED:
      nvc0_query_get(push, q, 0, 0x1000f010);
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_query_get(push, q, 0x00, 0x00801002); /* VFETCH, VERTICES */
      nvc0_query_get(push, q, 0x10, 0x01801002); /* VFETCH, PRIMS */
      nvc0_query_get(push, q, 0x20, 0x02802002); /* VP, LAUNCHES */
      nvc0_query_get(push, q, 0x30, 0x03806002); /* GP, LAUNCHES */
      nvc0_query_get(push, q, 0x40, 0x04806002); /* GP, PRIMS_OUT */
      nvc0_query_get(push, q, 0x50, 0x07804002); /* RAST, PRIMS_IN */
      nvc0_query_get(push, q, 0x60, 0x08804002); /* RAST, PRIMS_OUT */
      nvc0_query_get(push, q, 0x70, 0x0980a002); /* ROP, PIXELS */
      nvc0_query_get(push, q, 0x80, 0x0d808002); /* TCP, LAUNCHES */
      nvc0_query_get(push, q, 0x90, 0x0e809002); /* TEP, LAUNCHES */
      break;
   case NVC0_QUERY_TFB_BUFFER_OFFSET:
      /* indexed by TFB buffer instead of by vertex stream */
      nvc0_query_get(push, q, 0x00, 0x0d005002 | (q->index << 5));
      break;
   default:
      if (q->type >= NVE4_PM_QUERY(0) && q->type <= NVE4_PM_QUERY_MAX)
         nve4_mp_pm_query_end(nvc0, q);
      break;
   }
   if (q->is64bit)
      nouveau_fence_ref(nvc0->screen->base.fence.current, &q->fence);
}

static INLINE void
nvc0_query_update(struct nouveau_client *cli, struct nvc0_query *q)
{
   if (q->is64bit) {
      if (nouveau_fence_signalled(q->fence))
         q->state = NVC0_QUERY_STATE_READY;
   } else {
      if (q->data[0] == q->sequence)
         q->state = NVC0_QUERY_STATE_READY;
   }
}

static boolean
nvc0_query_result(struct pipe_context *pipe, struct pipe_query *pq,
                  boolean wait, union pipe_query_result *result)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_query *q = nvc0_query(pq);
   uint64_t *res64 = (uint64_t*)result;
   uint32_t *res32 = (uint32_t*)result;
   boolean *res8 = (boolean*)result;
   uint64_t *data64 = (uint64_t *)q->data;
   unsigned i;

   if (q->type >= NVE4_PM_QUERY(0) && q->type <= NVE4_PM_QUERY_MAX)
      return nve4_mp_pm_query_result(nvc0, q, result, wait);

   if (q->state != NVC0_QUERY_STATE_READY)
      nvc0_query_update(nvc0->screen->base.client, q);

   if (q->state != NVC0_QUERY_STATE_READY) {
      if (!wait) {
         if (q->state != NVC0_QUERY_STATE_FLUSHED) {
            q->state = NVC0_QUERY_STATE_FLUSHED;
            /* flush for silly apps that spin on GL_QUERY_RESULT_AVAILABLE */
            PUSH_KICK(nvc0->base.pushbuf);
         }
         return FALSE;
      }
      if (nouveau_bo_wait(q->bo, NOUVEAU_BO_RD, nvc0->screen->base.client))
         return FALSE;
   }
   q->state = NVC0_QUERY_STATE_READY;

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
      assert(0); /* can't happen, we don't create queries with invalid type */
      return FALSE;
   }

   return TRUE;
}

void
nvc0_query_fifo_wait(struct nouveau_pushbuf *push, struct pipe_query *pq)
{
   struct nvc0_query *q = nvc0_query(pq);
   unsigned offset = q->offset;

   if (q->type == PIPE_QUERY_SO_OVERFLOW_PREDICATE) offset += 0x20;

   PUSH_SPACE(push, 5);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NVC0(push, SUBC_3D(NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, q->bo->offset + offset);
   PUSH_DATA (push, q->bo->offset + offset);
   PUSH_DATA (push, q->sequence);
   PUSH_DATA (push, (1 << 12) |
              NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_EQUAL);
}

static void
nvc0_render_condition(struct pipe_context *pipe,
                      struct pipe_query *pq, uint mode)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_query *q;
   uint32_t cond;
   boolean negated = FALSE;
   boolean wait =
      mode != PIPE_RENDER_COND_NO_WAIT &&
      mode != PIPE_RENDER_COND_BY_REGION_NO_WAIT;

   nvc0->cond_query = pq;
   nvc0->cond_mode = mode;

   if (!pq) {
      PUSH_SPACE(push, 1);
      IMMED_NVC0(push, NVC0_3D(COND_MODE), NVC0_3D_COND_MODE_ALWAYS);
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
      nvc0_query_fifo_wait(push, pq);

   PUSH_SPACE(push, 4);
   PUSH_REFN (push, q->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NVC0(push, NVC0_3D(COND_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, q->bo->offset + q->offset);
   PUSH_DATA (push, q->bo->offset + q->offset);
   PUSH_DATA (push, cond);
}

void
nvc0_query_pushbuf_submit(struct nouveau_pushbuf *push,
                          struct pipe_query *pq, unsigned result_offset)
{
   struct nvc0_query *q = nvc0_query(pq);

#define NVC0_IB_ENTRY_1_NO_PREFETCH (1 << (31 - 8))

   nouveau_pushbuf_space(push, 0, 0, 1);
   nouveau_pushbuf_data(push, q->bo, q->offset + result_offset, 4 |
                        NVC0_IB_ENTRY_1_NO_PREFETCH);
}

void
nvc0_so_target_save_offset(struct pipe_context *pipe,
                           struct pipe_stream_output_target *ptarg,
                           unsigned index, boolean *serialize)
{
   struct nvc0_so_target *targ = nvc0_so_target(ptarg);

   if (*serialize) {
      *serialize = FALSE;
      PUSH_SPACE(nvc0_context(pipe)->base.pushbuf, 1);
      IMMED_NVC0(nvc0_context(pipe)->base.pushbuf, NVC0_3D(SERIALIZE), 0);
   }

   nvc0_query(targ->pq)->index = index;

   nvc0_query_end(pipe, targ->pq);
}


/* === PERFORMANCE MONITORING COUNTERS === */

/* Code to read out MP counters: They are accessible via mmio, too, but let's
 * just avoid mapping registers in userspace. We'd have to know which MPs are
 * enabled/present, too, and that information is not presently exposed.
 * We could add a kernel interface for it, but reading the counters like this
 * has the advantage of being async (if get_result isn't called immediately).
 */
static const uint64_t nve4_read_mp_pm_counters_code[] =
{
   0x2042004270420047ULL, /* sched */
   0x2800400000001de4ULL, /* mov b32 $r0 c0[0] (04) */
   0x2c0000000c009c04ULL, /* mov b32 $r2 $physid (20) */
   0x2800400010005de4ULL, /* mov b32 $r1 c0[4] (04) */
   0x2c0000008400dc04ULL, /* mov b32 $r3 $tidx (27) */
   0x7000c01050209c03ULL, /* ext u32 $r2 $r2 0x0414 (04) */
   0x2c00000010011c04ULL, /* mov b32 $r4 $pm0 (20) */
   0x190e0000fc33dc03ULL, /* set $p1 eq u32 $r3 0 (04) */
   0x2280428042804277ULL, /* sched */
   0x2c00000014015c04ULL, /* mov b32 $r5 $pm1 (27) */
   0x10000000c0209c02ULL, /* mul $r2 u32 $r2 u32 48 (04) */
   0x2c00000018019c04ULL, /* mov b32 $r6 $pm2 (28) */
   0x4801000008001c03ULL, /* add b32 ($r0 $c) $r0 $r2 (04) */
   0x2c0000001c01dc04ULL, /* mov b32 $r7 $pm3 (28) */
   0x0800000000105c42ULL, /* add b32 $r1 $r1 0 $c (04) */
   0x2c00000140009c04ULL, /* mov b32 $r2 $clock (28) */
   0x2042804200420047ULL, /* sched */
   0x94000000000107c5ULL, /* $p1 st b128 wt g[$r0d] $r4q (04) */
   0x2c00000020011c04ULL, /* mov b32 $r4 $pm4 (20) */
   0x2c00000024015c04ULL, /* mov b32 $r5 $pm5 (04) */
   0x2c00000028019c04ULL, /* mov b32 $r6 $pm6 (20) */
   0x2c0000002c01dc04ULL, /* mov b32 $r7 $pm7 (04) */
   0x2c0000014400dc04ULL, /* mov b32 $r3 $clockhi (28) */
   0x94000000400107c5ULL, /* $p1 st b128 wt g[$r0d+16] $r4q (04) */
   0x200002e042804207ULL, /* sched */
   0x2800400020011de4ULL, /* mov b32 $r4 c0[8] (20) */
   0x2c0000000c015c04ULL, /* mov b32 $r5 $physid (04) */
   0x94000000800087a5ULL, /* $p1 st b64 wt g[$r0d+32] $r2d (28) */
   0x94000000a00107a5ULL, /* $p1 st b64 wt g[$r0d+40] $r4d (04) */
   0x8000000000001de7ULL  /* exit (2e) */
};

/* NOTE: intentionally using the same names as NV */
static const char *nve4_pm_query_names[] =
{
   /* MP counters */
   "prof_trigger_00",
   "prof_trigger_01",
   "prof_trigger_02",
   "prof_trigger_03",
   "prof_trigger_04",
   "prof_trigger_05",
   "prof_trigger_06",
   "prof_trigger_07",
   "warps_launched",
   "threads_launched",
   "sm_cta_launched",
   "inst_issued1",
   "inst_issued2",
   "inst_executed",
   "local_load",
   "local_store",
   "shared_load",
   "shared_store",
   "l1_local_load_hit",
   "l1_local_load_miss",
   "l1_local_store_hit",
   "l1_local_store_miss",
   "gld_request",
   "gst_request",
   "l1_global_load_hit",
   "l1_global_load_miss",
   "uncached_global_load_transaction",
   "global_store_transaction",
   "branch",
   "divergent_branch",
   "active_warps",
   "active_cycles"
};

/* For simplicity, we will allocate as many group slots as we allocate counter
 * slots. This means that a single counter which wants to source from 2 groups
 * will have to be declared as using 2 counter slots. This shouldn't really be
 * a problem because such queries don't make much sense ... (unless someone is
 * really creative).
 */
struct nve4_mp_counter_cfg
{
   uint32_t func    : 16; /* mask or 4-bit logic op (depending on mode) */
   uint32_t mode    : 4;  /* LOGOP,B6,LOGOP_B6(_PULSE) */
   uint32_t pad     : 3;
   uint32_t sig_dom : 1;  /* if 0, MP_PM_A, if 1, MP_PM_B */
   uint32_t sig_sel : 8;  /* signal group */
   uint32_t src_sel : 32; /* signal selection for up to 5 sources */
};

struct nve4_mp_pm_query_cfg
{
   struct nve4_mp_counter_cfg ctr[4];
   uint8_t num_counters;
   uint8_t op; /* PIPE_LOGICOP_CLEAR(for ADD),OR,AND */
};

#define _Q1A(n, f, m, g, s) [NVE4_PM_QUERY_##n] = { { { f, NVE4_COMPUTE_MP_PM_FUNC_MODE_##m, 0, 0, NVE4_COMPUTE_MP_PM_A_SIGSEL_##g, s }, {}, {}, {} }, 1, PIPE_LOGICOP_CLEAR }
#define _Q1B(n, f, m, g, s) [NVE4_PM_QUERY_##n] = { { { f, NVE4_COMPUTE_MP_PM_FUNC_MODE_##m, 0, 1, NVE4_COMPUTE_MP_PM_B_SIGSEL_##g, s }, {}, {}, {} }, 1, PIPE_LOGICOP_CLEAR }

static const struct nve4_mp_pm_query_cfg nve4_mp_pm_queries[] =
{
   _Q1A(PROF_TRIGGER_0, 0x0001, B6, USER, 0x00000000),
   _Q1A(PROF_TRIGGER_1, 0x0001, B6, USER, 0x00000004),
   _Q1A(PROF_TRIGGER_2, 0x0001, B6, USER, 0x00000008),
   _Q1A(PROF_TRIGGER_3, 0x0001, B6, USER, 0x0000000c),
   _Q1A(PROF_TRIGGER_4, 0x0001, B6, USER, 0x00000010),
   _Q1A(PROF_TRIGGER_5, 0x0001, B6, USER, 0x00000014),
   _Q1A(PROF_TRIGGER_6, 0x0001, B6, USER, 0x00000018),
   _Q1A(PROF_TRIGGER_7, 0x0001, B6, USER, 0x0000001c),
   _Q1A(LAUNCHED_WARPS,    0x0001, B6, LAUNCH, 0x00000004),
   _Q1A(LAUNCHED_THREADS,  0x003f, B6, LAUNCH, 0x398a4188),
   _Q1B(LAUNCHED_CTA,      0x0001, B6, WARP, 0x0000001c),
   _Q1A(INST_ISSUED1,  0x0001, B6, ISSUE, 0x00000004),
   _Q1A(INST_ISSUED2,  0x0001, B6, ISSUE, 0x00000008),
   _Q1A(INST_EXECUTED, 0x0003, B6, EXEC,  0x00000398),
   _Q1A(LD_SHARED,   0x0001, B6, LDST, 0x00000000),
   _Q1A(ST_SHARED,   0x0001, B6, LDST, 0x00000004),
   _Q1A(LD_LOCAL,    0x0001, B6, LDST, 0x00000008),
   _Q1A(ST_LOCAL,    0x0001, B6, LDST, 0x0000000c),
   _Q1A(GLD_REQUEST, 0x0001, B6, LDST, 0x00000010),
   _Q1A(GST_REQUEST, 0x0001, B6, LDST, 0x00000014),
   _Q1B(L1_LOCAL_LOAD_HIT,   0x0001, B6, L1, 0x00000000),
   _Q1B(L1_LOCAL_LOAD_MISS,  0x0001, B6, L1, 0x00000004),
   _Q1B(L1_LOCAL_STORE_HIT,  0x0001, B6, L1, 0x00000008),
   _Q1B(L1_LOCAL_STORE_MISS, 0x0001, B6, L1, 0x0000000c),
   _Q1B(L1_GLOBAL_LOAD_HIT,  0x0001, B6, L1, 0x00000010),
   _Q1B(L1_GLOBAL_LOAD_MISS, 0x0001, B6, L1, 0x00000014),
   _Q1B(GLD_TRANSACTIONS_UNCACHED, 0x0001, B6, MEM, 0x00000000),
   _Q1B(GST_TRANSACTIONS,          0x0001, B6, MEM, 0x00000004),
   _Q1A(BRANCH,           0x0001, B6, BRANCH, 0x0000000c),
   _Q1A(BRANCH_DIVERGENT, 0x0001, B6, BRANCH, 0x00000010),
   _Q1B(ACTIVE_WARPS,  0x003f, B6, WARP, 0x398a4188),
   _Q1B(ACTIVE_CYCLES, 0x0001, B6, WARP, 0x00000004)
};

#undef _Q1A
#undef _Q1B

void
nve4_mp_pm_query_begin(struct nvc0_context *nvc0, struct nvc0_query *q)
{
   struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   const struct nve4_mp_pm_query_cfg *cfg;
   unsigned i, c;
   unsigned num_ab[2] = { 0, 0 };

   cfg = &nve4_mp_pm_queries[q->type - PIPE_QUERY_DRIVER_SPECIFIC];

   /* check if we have enough free counter slots */
   for (i = 0; i < cfg->num_counters; ++i)
      num_ab[cfg->ctr[i].sig_dom]++;

   if (screen->pm.num_mp_pm_active[0] + num_ab[0] > 4 ||
       screen->pm.num_mp_pm_active[1] + num_ab[1] > 4) {
      NOUVEAU_ERR("Not enough free MP counter slots !\n");
      return;
   }

   assert(cfg->num_counters <= 4);
   PUSH_SPACE(push, 4 * 8 + 6);

   if (!screen->pm.mp_counters_enabled) {
      screen->pm.mp_counters_enabled = TRUE;
      BEGIN_NVC0(push, SUBC_SW(0x06ac), 1);
      PUSH_DATA (push, 0x1fcb);
   }

   /* set sequence field to 0 (used to check if result is available) */
   for (i = 0; i < screen->mp_count; ++i)
      q->data[i * 10 + 10] = 0;

   for (i = 0; i < cfg->num_counters; ++i) {
      const unsigned d = cfg->ctr[i].sig_dom;

      if (!screen->pm.num_mp_pm_active[d]) {
         uint32_t m = (1 << 22) | (1 << (7 + (8 * !d)));
         if (screen->pm.num_mp_pm_active[!d])
            m |= 1 << (7 + (8 * d));
         BEGIN_NVC0(push, SUBC_SW(0x0600), 1);
         PUSH_DATA (push, m);
      }
      screen->pm.num_mp_pm_active[d]++;

      for (c = d * 4; c < (d * 4 + 4); ++c) {
         if (!screen->pm.mp_counter[c]) {
            q->ctr[i] = c;
            screen->pm.mp_counter[c] = (struct pipe_query *)q;
            break;
         }
      }
      assert(c <= (d * 4 + 3)); /* must succeed, already checked for space */

      /* configure and reset the counter(s) */
      if (d == 0)
         BEGIN_NVC0(push, NVE4_COMPUTE(MP_PM_A_SIGSEL(c & 3)), 1);
      else
         BEGIN_NVC0(push, NVE4_COMPUTE(MP_PM_B_SIGSEL(c & 3)), 1);
      PUSH_DATA (push, cfg->ctr[i].sig_sel);
      BEGIN_NVC0(push, NVE4_COMPUTE(MP_PM_SRCSEL(c)), 1);
      PUSH_DATA (push, cfg->ctr[i].src_sel + 0x2108421 * (c & 3));
      BEGIN_NVC0(push, NVE4_COMPUTE(MP_PM_FUNC(c)), 1);
      PUSH_DATA (push, (cfg->ctr[i].func << 4) | cfg->ctr[i].mode);
      BEGIN_NVC0(push, NVE4_COMPUTE(MP_PM_SET(c)), 1);
      PUSH_DATA (push, 0);
   }
}

static void
nve4_mp_pm_query_end(struct nvc0_context *nvc0, struct nvc0_query *q)
{
   struct nvc0_screen *screen = nvc0->screen;
   struct pipe_context *pipe = &nvc0->base.pipe;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   uint32_t mask;
   uint32_t input[3];
   const uint block[3] = { 32, 1, 1 };
   const uint grid[3] = { screen->mp_count, 1, 1 };
   unsigned c;
   const struct nve4_mp_pm_query_cfg *cfg;

   cfg = &nve4_mp_pm_queries[q->type - PIPE_QUERY_DRIVER_SPECIFIC];

   if (unlikely(!screen->pm.prog)) {
      struct nvc0_program *prog = CALLOC_STRUCT(nvc0_program);
      prog->type = PIPE_SHADER_COMPUTE;
      prog->translated = TRUE;
      prog->num_gprs = 8;
      prog->code = (uint32_t *)nve4_read_mp_pm_counters_code;
      prog->code_size = sizeof(nve4_read_mp_pm_counters_code);
      prog->parm_size = 12;
      screen->pm.prog = prog;
   }

   /* disable all counting */
   PUSH_SPACE(push, 8);
   for (c = 0; c < 8; ++c)
      if (screen->pm.mp_counter[c])
         IMMED_NVC0(push, NVE4_COMPUTE(MP_PM_FUNC(c)), 0);
   /* release counters for this query */
   for (c = 0; c < 8; ++c) {
      if (nvc0_query(screen->pm.mp_counter[c]) == q) {
         screen->pm.num_mp_pm_active[c / 4]--;
         screen->pm.mp_counter[c] = NULL;
      }
   }

   BCTX_REFN_bo(nvc0->bufctx_cp, CP_QUERY, NOUVEAU_BO_GART | NOUVEAU_BO_WR,
                q->bo);

   pipe->bind_compute_state(pipe, screen->pm.prog);
   input[0] = (q->bo->offset + q->base);
   input[1] = (q->bo->offset + q->base) >> 32;
   input[2] = q->sequence;
   pipe->launch_grid(pipe, block, grid, 0, input);

   nouveau_bufctx_reset(nvc0->bufctx_cp, NVC0_BIND_CP_QUERY);

   /* re-activate other counters */
   PUSH_SPACE(push, 16);
   mask = 0;
   for (c = 0; c < 8; ++c) {
      unsigned i;
      q = nvc0_query(screen->pm.mp_counter[c]);
      if (!q)
         continue;
      cfg = &nve4_mp_pm_queries[q->type - PIPE_QUERY_DRIVER_SPECIFIC];
      for (i = 0; i < cfg->num_counters; ++i) {
         if (mask & (1 << q->ctr[i]))
            break;
         mask |= 1 << q->ctr[i];
         BEGIN_NVC0(push, NVE4_COMPUTE(MP_PM_FUNC(q->ctr[i])), 1);
         PUSH_DATA (push, (cfg->ctr[i].func << 4) | cfg->ctr[i].mode);
      }
   }
}

static boolean
nve4_mp_pm_query_result(struct nvc0_context *nvc0, struct nvc0_query *q,
                        void *result, boolean wait)
{
   uint32_t count[4];
   uint64_t value = 0;
   unsigned p, c;
   const struct nve4_mp_pm_query_cfg *cfg;

   cfg = &nve4_mp_pm_queries[q->type - PIPE_QUERY_DRIVER_SPECIFIC];

   for (p = 0; p < nvc0->screen->mp_count_compute; ++p) {
      uint64_t clock;
      const unsigned b = p * 12;

      clock = *(uint64_t *)&q->data[b + 8];
      (void)clock; /* might be interesting one day */

      if (q->data[b + 10] != q->sequence) {
         /* WARNING: This will spin forever if you loop with wait == FALSE and
          * the push buffer hasn't been flushed !
          */
         if (!wait)
            return FALSE;
         if (nouveau_bo_wait(q->bo, NOUVEAU_BO_RD, nvc0->base.client))
            return FALSE;
      }

      for (c = 0; c < cfg->num_counters; ++c)
         count[c] = q->data[b + q->ctr[c]];
      for (; c < 4; ++c)
         count[c] = 0;

      switch (cfg->op) {
      case PIPE_LOGICOP_AND:
         value &= count[0] & count[1] & count[2] & count[3];
         break;
      case PIPE_LOGICOP_OR:
         value |= count[0] | count[1] | count[2] | count[3];
         break;
      case PIPE_LOGICOP_CLEAR: /* abused as ADD */
      default:
         value += count[0] + count[1] + count[2] + count[3];
         break;
      }
   }
   *(uint64_t *)result = value;
   return TRUE;
}

int
nvc0_screen_get_driver_query_info(struct pipe_screen *pscreen,
                                  unsigned id,
                                  struct pipe_driver_query_info *info)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);

   if (screen->base.class_3d >= NVE4_3D_CLASS) {
      unsigned count = 0;
      if (screen->base.device->drm_version >= 0x01000101)
         count = NVE4_PM_QUERY_COUNT;
      if (!info)
         return count;
      if (id < count) {
         info->name = nve4_pm_query_names[id];
         info->query_type = NVE4_PM_QUERY(id);
         info->max_value = ~0ULL;
         info->uses_byte_units = FALSE;
         return 1;
      }
   } else {
      if (!info)
         return 0;
   }
   /* user asked for info about non-existing query */
   info->name = "this_is_not_the_query_you_are_looking_for";
   info->query_type = 0xdeadd01d;
   info->max_value = 0;
   info->uses_byte_units = FALSE;
   return 0;
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
