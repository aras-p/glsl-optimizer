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

#include "nv50_fence.h"
#include "nv50_context.h"
#include "nv50_screen.h"

#ifdef PIPE_OS_UNIX
#include <sched.h>
#endif

boolean
nv50_screen_fence_new(struct nv50_screen *screen, struct nv50_fence **fence,
                      boolean emit)
{
   *fence = CALLOC_STRUCT(nv50_fence);
   if (!*fence)
      return FALSE;

   (*fence)->screen = screen;
   (*fence)->ref = 1;

   if (emit)
      nv50_fence_emit(*fence);

   return TRUE;
}

void
nv50_fence_emit(struct nv50_fence *fence)
{
   struct nv50_screen *screen = fence->screen;
   struct nouveau_channel *chan = screen->base.channel;

   fence->sequence = ++screen->fence.sequence;

   assert(fence->state == NV50_FENCE_STATE_AVAILABLE);

   MARK_RING (chan, 5, 2);
   BEGIN_RING(chan, RING_3D(QUERY_ADDRESS_HIGH), 4);
   OUT_RELOCh(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RELOCl(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RING  (chan, fence->sequence);
   OUT_RING  (chan, 
              NV50_3D_QUERY_GET_MODE_WRITE_UNK0 |
              NV50_3D_QUERY_GET_UNK4 |
              NV50_3D_QUERY_GET_UNIT_CROP |
              NV50_3D_QUERY_GET_TYPE_QUERY |
              NV50_3D_QUERY_GET_QUERY_SELECT_ZERO |
              NV50_3D_QUERY_GET_SHORT);


   ++fence->ref;

   if (screen->fence.tail)
      screen->fence.tail->next = fence;
   else
      screen->fence.head = fence;

   screen->fence.tail = fence;

   fence->state = NV50_FENCE_STATE_EMITTED;
}

static void
nv50_fence_trigger_release_buffers(struct nv50_fence *fence);

void
nv50_fence_del(struct nv50_fence *fence)
{
   struct nv50_fence *it;
   struct nv50_screen *screen = fence->screen;

   if (fence->state == NV50_FENCE_STATE_EMITTED ||
       fence->state == NV50_FENCE_STATE_FLUSHED) {
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

   if (fence->buffers) {
      debug_printf("WARNING: deleting fence with buffers "
                   "still hooked to it !\n");
      nv50_fence_trigger_release_buffers(fence);
   }

   FREE(fence);
}

static void
nv50_fence_trigger_release_buffers(struct nv50_fence *fence)
{
   struct nv50_mm_allocation *alloc = fence->buffers;

   while (alloc) {
      struct nv50_mm_allocation *next = alloc->next;
      nv50_mm_free(alloc);
      alloc = next;
   };
   fence->buffers = NULL;
}

void
nv50_screen_fence_update(struct nv50_screen *screen, boolean flushed)
{
   struct nv50_fence *fence;
   struct nv50_fence *next = NULL;
   uint32_t sequence = screen->fence.map[0];

   if (screen->fence.sequence_ack == sequence)
      return;
   screen->fence.sequence_ack = sequence;

   for (fence = screen->fence.head; fence; fence = next) {
      next = fence->next;
      sequence = fence->sequence;

      fence->state = NV50_FENCE_STATE_SIGNALLED;

      if (fence->buffers)
         nv50_fence_trigger_release_buffers(fence);

      nv50_fence_reference(&fence, NULL);

      if (sequence == screen->fence.sequence_ack)
         break;
   }
   screen->fence.head = next;
   if (!next)
      screen->fence.tail = NULL;

   if (flushed) {
      for (fence = next; fence; fence = fence->next)
         fence->state = NV50_FENCE_STATE_FLUSHED;
   }
}

#define NV50_FENCE_MAX_SPINS (1 << 31)

boolean
nv50_fence_signalled(struct nv50_fence *fence)
{
   struct nv50_screen *screen = fence->screen;

   if (fence->state >= NV50_FENCE_STATE_EMITTED)
      nv50_screen_fence_update(screen, FALSE);

   return fence->state == NV50_FENCE_STATE_SIGNALLED;
}

boolean
nv50_fence_wait(struct nv50_fence *fence)
{
   struct nv50_screen *screen = fence->screen;
   uint32_t spins = 0;

   if (fence->state < NV50_FENCE_STATE_EMITTED) {
      nv50_fence_emit(fence);

      if (fence == screen->fence.current)
         nv50_screen_fence_new(screen, &screen->fence.current, FALSE);
   }
   if (fence->state < NV50_FENCE_STATE_FLUSHED)
      FIRE_RING(screen->base.channel);

   do {
      nv50_screen_fence_update(screen, FALSE);

      if (fence->state == NV50_FENCE_STATE_SIGNALLED)
         return TRUE;
      spins++;
#ifdef PIPE_OS_UNIX
      if (!(spins % 8)) /* donate a few cycles */
         sched_yield();
#endif
   } while (spins < NV50_FENCE_MAX_SPINS);

   debug_printf("Wait on fence %u (ack = %u, next = %u) timed out !\n",
                fence->sequence,
                screen->fence.sequence_ack, screen->fence.sequence);

   return FALSE;
}

void
nv50_screen_fence_next(struct nv50_screen *screen)
{
   nv50_fence_emit(screen->fence.current);
   nv50_screen_fence_new(screen, &screen->fence.current, FALSE);
}
