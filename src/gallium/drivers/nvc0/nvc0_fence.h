
#ifndef __NVC0_FENCE_H__
#define __NVC0_FENCE_H__

#include "util/u_inlines.h"

struct nvc0_fence_trigger {
   void (*func)(void *);
   void *arg;
   struct nvc0_fence_trigger *next;
};

#define NVC0_FENCE_STATE_EMITTED   1
#define NVC0_FENCE_STATE_SIGNALLED 2

/* reference first, so pipe_reference works directly */
struct nvc0_fence {
   struct pipe_reference reference;
   struct nvc0_fence *next;
   struct nvc0_screen *screen;
   int state;
   uint32_t sequence;
   struct nvc0_fence_trigger trigger;
};

void nvc0_fence_emit(struct nvc0_fence *);
void nvc0_fence_del(struct nvc0_fence *);

boolean nvc0_fence_wait(struct nvc0_fence *);

static INLINE void
nvc0_fence_reference(struct nvc0_fence **ref, struct nvc0_fence *fence)
{
   if (pipe_reference(&(*ref)->reference, &fence->reference))
      nvc0_fence_del(*ref);

   *ref = fence;
}

#endif // __NVC0_FENCE_H__
