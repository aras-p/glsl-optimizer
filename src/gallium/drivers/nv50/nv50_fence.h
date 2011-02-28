
#ifndef __NV50_FENCE_H__
#define __NV50_FENCE_H__

#include "util/u_inlines.h"
#include "util/u_double_list.h"

#define NV50_FENCE_STATE_AVAILABLE 0
#define NV50_FENCE_STATE_EMITTED   1
#define NV50_FENCE_STATE_FLUSHED   2
#define NV50_FENCE_STATE_SIGNALLED 3

struct nv50_mm_allocation;

struct nv50_fence {
   struct nv50_fence *next;
   struct nv50_screen *screen;
   int state;
   int ref;
   uint32_t sequence;
   struct nv50_mm_allocation *buffers;
};

void nv50_fence_emit(struct nv50_fence *);
void nv50_fence_del(struct nv50_fence *);

boolean nv50_fence_wait(struct nv50_fence *);
boolean nv50_fence_signalled(struct nv50_fence *);

static INLINE void
nv50_fence_reference(struct nv50_fence **ref, struct nv50_fence *fence)
{
   if (*ref) {
      if (--(*ref)->ref == 0)
         nv50_fence_del(*ref);
   }
   if (fence)
      ++fence->ref;

   *ref = fence;
}

static INLINE struct nv50_fence *
nv50_fence(struct pipe_fence_handle *fence)
{
   return (struct nv50_fence *)fence;
}

#endif // __NV50_FENCE_H__
