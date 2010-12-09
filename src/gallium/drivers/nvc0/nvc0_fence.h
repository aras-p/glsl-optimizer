
#ifndef __NVC0_FENCE_H__
#define __NVC0_FENCE_H__

#include "util/u_inlines.h"
#include "util/u_double_list.h"

#define NVC0_FENCE_STATE_AVAILABLE 0
#define NVC0_FENCE_STATE_EMITTED   1
#define NVC0_FENCE_STATE_SIGNALLED 2

struct nvc0_mm_allocation;

struct nvc0_fence {
   struct nvc0_fence *next;
   struct nvc0_screen *screen;
   int state;
   int ref;
   uint32_t sequence;
   struct nvc0_mm_allocation *buffers;
};

void nvc0_fence_emit(struct nvc0_fence *);
void nvc0_fence_del(struct nvc0_fence *);

boolean nvc0_fence_wait(struct nvc0_fence *);

static INLINE void
nvc0_fence_reference(struct nvc0_fence **ref, struct nvc0_fence *fence)
{
   if (*ref) {
      if (--(*ref)->ref == 0)
         nvc0_fence_del(*ref);
   }
   if (fence)
      ++fence->ref;

   *ref = fence;
}

static INLINE struct nvc0_fence *
nvc0_fence(struct pipe_fence_handle *fence)
{
   return (struct nvc0_fence *)fence;
}

#endif // __NVC0_FENCE_H__
