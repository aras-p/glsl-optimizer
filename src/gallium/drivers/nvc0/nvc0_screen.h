#ifndef __NVC0_SCREEN_H__
#define __NVC0_SCREEN_H__

#define NOUVEAU_NVC0
#include "nouveau/nouveau_screen.h"
#undef NOUVEAU_NVC0
#include "nvc0_winsys.h"
#include "nvc0_stateobj.h"

#define NVC0_TIC_MAX_ENTRIES 2048
#define NVC0_TSC_MAX_ENTRIES 2048

struct nvc0_mman;
struct nvc0_context;
struct nvc0_fence;

#define NVC0_SCRATCH_SIZE (2 << 20)
#define NVC0_SCRATCH_NR_BUFFERS 2

struct nvc0_screen {
   struct nouveau_screen base;
   struct nouveau_winsys *nvws;

   struct nvc0_context *cur_ctx;

   struct nouveau_bo *text;
   struct nouveau_bo *uniforms;
   struct nouveau_bo *tls;
   struct nouveau_bo *txc; /* TIC (offset 0) and TSC (65536) */
   struct nouveau_bo *mp_stack_bo;

   uint64_t tls_size;

   struct nouveau_resource *text_heap;

   struct {
      struct nouveau_bo *bo[NVC0_SCRATCH_NR_BUFFERS];
      uint8_t *buf;
      int index;
      uint32_t offset;
   } scratch;

   struct {
      void **entries;
      int next;
      uint32_t lock[NVC0_TIC_MAX_ENTRIES / 32];
   } tic;
   
   struct {
      void **entries;
      int next;
      uint32_t lock[NVC0_TSC_MAX_ENTRIES / 32];
   } tsc;

   struct {
      uint32_t *map;
      struct nvc0_fence *head;
      struct nvc0_fence *tail;
      struct nvc0_fence *current;
      uint32_t sequence;
      uint32_t sequence_ack;
      struct nouveau_bo *bo;
   } fence;

   struct nvc0_mman *mm_GART;
   struct nvc0_mman *mm_VRAM;
   struct nvc0_mman *mm_VRAM_fe0;

   struct nouveau_grobj *fermi;
   struct nouveau_grobj *eng2d;
   struct nouveau_grobj *m2mf;
};

static INLINE struct nvc0_screen *
nvc0_screen(struct pipe_screen *screen)
{
   return (struct nvc0_screen *)screen;
}

/* Since a resource can be migrated, we need to decouple allocations from
 * them. This struct is linked with fences for delayed freeing of allocs.
 */
struct nvc0_mm_allocation {
   struct nvc0_mm_allocation *next;
   void *priv;
   uint32_t offset;
};

static INLINE void
nvc0_fence_sched_release(struct nvc0_fence *nf, struct nvc0_mm_allocation *mm)
{
   mm->next = nf->buffers;
   nf->buffers = mm;
}

extern struct nvc0_mman *
nvc0_mm_create(struct nouveau_device *, uint32_t domain, uint32_t storage_type);

extern void
nvc0_mm_destroy(struct nvc0_mman *);

extern struct nvc0_mm_allocation *
nvc0_mm_allocate(struct nvc0_mman *,
                 uint32_t size, struct nouveau_bo **, uint32_t *offset);
extern void
nvc0_mm_free(struct nvc0_mm_allocation *);

void nvc0_screen_make_buffers_resident(struct nvc0_screen *);

int nvc0_screen_tic_alloc(struct nvc0_screen *, void *);
int nvc0_screen_tsc_alloc(struct nvc0_screen *, void *);

static INLINE void
nvc0_resource_fence(struct nvc0_resource *res, uint32_t flags)
{
   struct nvc0_screen *screen = nvc0_screen(res->base.screen);

   if (res->mm) {
      nvc0_fence_reference(&res->fence, screen->fence.current);

      if (flags & NOUVEAU_BO_WR)
         nvc0_fence_reference(&res->fence_wr, screen->fence.current);
   }
}

static INLINE void
nvc0_resource_validate(struct nvc0_resource *res, uint32_t flags)
{
   struct nvc0_screen *screen = nvc0_screen(res->base.screen);

   if (likely(res->bo)) {
      nouveau_bo_validate(screen->base.channel, res->bo, flags);

      nvc0_resource_fence(res, flags);
   }
}


boolean
nvc0_screen_fence_new(struct nvc0_screen *, struct nvc0_fence **, boolean emit);

void
nvc0_screen_fence_next(struct nvc0_screen *);

static INLINE boolean
nvc0_screen_fence_emit(struct nvc0_screen *screen)
{
   nvc0_fence_emit(screen->fence.current);

   return nvc0_screen_fence_new(screen, &screen->fence.current, FALSE);
}

struct nvc0_format {
   uint32_t rt;
   uint32_t tic;
   uint32_t vtx;
   uint32_t usage;
};

extern const struct nvc0_format nvc0_format_table[];

static INLINE void
nvc0_screen_tic_unlock(struct nvc0_screen *screen, struct nvc0_tic_entry *tic)
{
   if (tic->id >= 0)
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
}

static INLINE void
nvc0_screen_tsc_unlock(struct nvc0_screen *screen, struct nvc0_tsc_entry *tsc)
{
   if (tsc->id >= 0)
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
}

static INLINE void
nvc0_screen_tic_free(struct nvc0_screen *screen, struct nvc0_tic_entry *tic)
{
   if (tic->id >= 0) {
      screen->tic.entries[tic->id] = NULL;
      screen->tic.lock[tic->id / 32] &= ~(1 << (tic->id % 32));
   }
}

static INLINE void
nvc0_screen_tsc_free(struct nvc0_screen *screen, struct nvc0_tsc_entry *tsc)
{
   if (tsc->id >= 0) {
      screen->tsc.entries[tsc->id] = NULL;
      screen->tsc.lock[tsc->id / 32] &= ~(1 << (tsc->id % 32));
   }
}

#endif
