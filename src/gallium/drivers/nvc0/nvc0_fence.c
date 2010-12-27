/*
 * Copyright 2010 Christoph Bumiller
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
 */

#include "nvc0_fence.h"
#include "nvc0_context.h"
#include "nvc0_screen.h"

#ifdef PIPE_OS_UNIX
#include <sched.h>
#endif

boolean
nvc0_screen_fence_new(struct nvc0_screen *screen, struct nvc0_fence **fence,
                      boolean emit)
{
   *fence = CALLOC_STRUCT(nvc0_fence);
   if (!*fence)
      return FALSE;

   (*fence)->screen = screen;
   (*fence)->ref = 1;

   if (emit)
      nvc0_fence_emit(*fence);

   return TRUE;
}

void
nvc0_fence_emit(struct nvc0_fence *fence)
{
   struct nvc0_screen *screen = fence->screen;
   struct nouveau_channel *chan = screen->base.channel;

   fence->sequence = ++screen->fence.sequence;

   assert(fence->state == NVC0_FENCE_STATE_AVAILABLE);

   BEGIN_RING(chan, RING_3D(QUERY_ADDRESS_HIGH), 4);
   OUT_RELOCh(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RELOCl(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RING  (chan, fence->sequence);
   OUT_RING  (chan, NVC0_3D_QUERY_GET_FENCE);

   ++fence->ref;

   if (screen->fence.tail)
      screen->fence.tail->next = fence;
   else
      screen->fence.head = fence;

   screen->fence.tail = fence;

   fence->state = NVC0_FENCE_STATE_EMITTED;
}

void
nvc0_fence_del(struct nvc0_fence *fence)
{
   struct nvc0_fence *it;
   struct nvc0_screen *screen = fence->screen;

   if (fence->state == NVC0_FENCE_STATE_EMITTED) {
      if (fence == screen->fence.head) {
         screen->fence.head = fence->next;
         if (!screen->fence.head)
            screen->fence.tail = NULL;
      } else {
         for (it = screen->fence.head; it && it->next != fence; it = it->next);
         it->next = fence->next;
         if (screen->fence.tail == fence)
            screen->fence.tail = it;
      }
   }
   FREE(fence);
}

static void
nvc0_fence_trigger_release_buffers(struct nvc0_fence *fence)
{
   struct nvc0_mm_allocation *alloc = fence->buffers;

   while (alloc) {
      struct nvc0_mm_allocation *next = alloc->next;
      nvc0_mm_free(alloc);
      alloc = next;
   };
}

static void
nvc0_screen_fence_update(struct nvc0_screen *screen)
{
   struct nvc0_fence *fence;
   struct nvc0_fence *next = NULL;
   uint32_t sequence = screen->fence.map[0];

   if (screen->fence.sequence_ack == sequence)
      return;
   screen->fence.sequence_ack = sequence;

   for (fence = screen->fence.head; fence; fence = next) {
      next = fence->next;
      sequence = fence->sequence;

      fence->state = NVC0_FENCE_STATE_SIGNALLED;

      if (fence->buffers)
         nvc0_fence_trigger_release_buffers(fence);

      nvc0_fence_reference(&fence, NULL);

      if (sequence == screen->fence.sequence_ack)
         break;
   }
   screen->fence.head = next;
   if (!next)
      screen->fence.tail = NULL;
}

#define NVC0_FENCE_MAX_SPINS (1 << 17)

boolean
nvc0_fence_signalled(struct nvc0_fence *fence)
{
   struct nvc0_screen *screen = fence->screen;

   if (fence->state == NVC0_FENCE_STATE_EMITTED)
      nvc0_screen_fence_update(screen);

   return fence->state == NVC0_FENCE_STATE_SIGNALLED;
}

boolean
nvc0_fence_wait(struct nvc0_fence *fence)
{
   struct nvc0_screen *screen = fence->screen;
   int spins = 0;

   if (fence->state == NVC0_FENCE_STATE_AVAILABLE) {
      nvc0_fence_emit(fence);

      FIRE_RING(screen->base.channel);

      if (fence == screen->fence.current)
         nvc0_screen_fence_new(screen, &screen->fence.current, FALSE);
   }

   do {
      nvc0_screen_fence_update(screen);

      if (fence->state == NVC0_FENCE_STATE_SIGNALLED)
         return TRUE;
      spins++;
#ifdef PIPE_OS_UNIX
      if (!(spins % 8)) /* donate a few cycles */
         sched_yield();
#endif
   } while (spins < NVC0_FENCE_MAX_SPINS);

   if (spins > 9000)
      NOUVEAU_ERR("fence %x: been spinning too long\n", fence->sequence);

   return FALSE;
}

void
nvc0_screen_fence_next(struct nvc0_screen *screen)
{
   nvc0_fence_emit(screen->fence.current);
   nvc0_screen_fence_new(screen, &screen->fence.current, FALSE);
   nvc0_screen_fence_update(screen);
}
