/*
 * Copyright 2007 Nouveau Project
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

#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "nouveau_drmif.h"
#include "nouveau_dma.h"
#include "nouveau_local.h"

static void
nouveau_fence_del_unsignalled(struct nouveau_fence *fence)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(fence->channel);
	struct nouveau_fence *le;

	if (nvchan->fence_head == fence) {
		nvchan->fence_head = nouveau_fence(fence)->next;
		if (nvchan->fence_head == NULL)
			nvchan->fence_tail = NULL;
		return;
	}

	le = nvchan->fence_head;
	while (le && nouveau_fence(le)->next != fence)
		le = nouveau_fence(le)->next;
	assert(le && nouveau_fence(le)->next == fence);
	nouveau_fence(le)->next = nouveau_fence(fence)->next;
	if (nvchan->fence_tail == fence)
		nvchan->fence_tail = le;
}

static void
nouveau_fence_del(struct nouveau_fence **fence)
{
	struct nouveau_fence_priv *nvfence;

	if (!fence || !*fence)
		return;
	nvfence = nouveau_fence(*fence);
	*fence = NULL;

	if (--nvfence->refcount)
		return;

	if (nvfence->emitted && !nvfence->signalled) {
		if (nvfence->signal_cb) {
			nvfence->refcount++;
			nouveau_fence_wait((void *)&nvfence);
			return;
		}

		nouveau_fence_del_unsignalled(&nvfence->base);
	}
	free(nvfence);
}

int
nouveau_fence_new(struct nouveau_channel *chan, struct nouveau_fence **fence)
{
	struct nouveau_fence_priv *nvfence;

	if (!chan || !fence || *fence)
		return -EINVAL;
	
	nvfence = calloc(1, sizeof(struct nouveau_fence_priv));
	if (!nvfence)
		return -ENOMEM;
	nvfence->base.channel = chan;
	nvfence->refcount = 1;

	*fence = &nvfence->base;
	return 0;
}

int
nouveau_fence_ref(struct nouveau_fence *ref, struct nouveau_fence **fence)
{
	struct nouveau_fence_priv *nvfence;

	if (!fence)
		return -EINVAL;

	if (*fence) {
		nouveau_fence_del(fence);
		*fence = NULL;
	}

	if (ref) {
		nvfence = nouveau_fence(ref);
		nvfence->refcount++;	
		*fence = &nvfence->base;
	}

	return 0;
}

int
nouveau_fence_signal_cb(struct nouveau_fence *fence, void (*func)(void *),
			void *priv)
{
	struct nouveau_fence_priv *nvfence = nouveau_fence(fence);
	struct nouveau_fence_cb *cb;

	if (!nvfence || !func)
		return -EINVAL;

	cb = malloc(sizeof(struct nouveau_fence_cb));
	if (!cb)
		return -ENOMEM;

	cb->func = func;
	cb->priv = priv;
	cb->next = nvfence->signal_cb;
	nvfence->signal_cb = cb;
	return 0;
}

void
nouveau_fence_emit(struct nouveau_fence *fence)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(fence->channel);
	struct nouveau_fence_priv *nvfence = nouveau_fence(fence);

	nvfence->emitted = 1;
	nvfence->sequence = ++nvchan->fence_sequence;
	if (nvfence->sequence == 0xffffffff)
		NOUVEAU_ERR("AII wrap unhandled\n");

	/*XXX: assumes subc 0 is populated */
	RING_SPACE_CH(fence->channel, 2);
	OUT_RING_CH  (fence->channel, 0x00040050);
	OUT_RING_CH  (fence->channel, nvfence->sequence);

	if (nvchan->fence_tail) {
		nouveau_fence(nvchan->fence_tail)->next = fence;
	} else {
		nvchan->fence_head = fence;
	}
	nvchan->fence_tail = fence;
}

void
nouveau_fence_flush(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);
	uint32_t sequence = *nvchan->ref_cnt;

	while (nvchan->fence_head) {
		struct nouveau_fence_priv *nvfence;
	
		nvfence = nouveau_fence(nvchan->fence_head);
		if (nvfence->sequence > sequence)
			break;
		nouveau_fence_del_unsignalled(&nvfence->base);
		nvfence->signalled = 1;

		if (nvfence->signal_cb) {
			struct nouveau_fence *fence = NULL;

			nouveau_fence_ref(&nvfence->base, &fence);

			while (nvfence->signal_cb) {
				struct nouveau_fence_cb *cb;
				
				cb = nvfence->signal_cb;
				nvfence->signal_cb = cb->next;
				cb->func(cb->priv);
				free(cb);
			}

			nouveau_fence_ref(NULL, &fence);
		}
	}
}

int
nouveau_fence_wait(struct nouveau_fence **fence)
{
	struct nouveau_fence_priv *nvfence;
	
	if (!fence || !*fence)
		return -EINVAL;
	nvfence = nouveau_fence(*fence);

	if (nvfence->emitted) {
		while (!nvfence->signalled)
			nouveau_fence_flush(nvfence->base.channel);
	}
	nouveau_fence_ref(NULL, fence);

	return 0;
}

