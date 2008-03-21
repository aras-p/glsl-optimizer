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

#include "nouveau_drmif.h"
#include "nouveau_local.h"

int
nouveau_resource_init(struct nouveau_resource **heap,
		      unsigned start, unsigned size)
{
	struct nouveau_resource *r;

	r = calloc(1, sizeof(struct nouveau_resource));
	if (!r)
		return 1;

	r->start = start;
	r->size  = size;
	*heap = r;
	return 0;
}

int
nouveau_resource_alloc(struct nouveau_resource *heap, int size, void *priv,
		       struct nouveau_resource **res)
{
	struct nouveau_resource *r;

	if (!heap || !size || !res || *res)
		return 1;

	while (heap) {
		if (!heap->in_use && heap->size >= size) {
			r = calloc(1, sizeof(struct nouveau_resource));
			if (!r)
				return 1;

			r->start  = (heap->start + heap->size) - size;
			r->size   = size;
			r->in_use = 1;
			r->priv   = priv;

			heap->size -= size;

			r->next = heap->next;
			if (heap->next)
				heap->next->prev = r;
			r->prev = heap;
			heap->next = r;

			*res = r;
			return 0;
		}
			
		heap = heap->next;
	}

	return 1;
}

void
nouveau_resource_free(struct nouveau_resource **res)
{
	struct nouveau_resource *r;

	if (!res || !*res)
		return;
	r = *res;
	*res = NULL;

	r->in_use = 0;

	if (r->next && !r->next->in_use) {
		struct nouveau_resource *new = r->next;

		new->prev = r->prev;
		if (r->prev)
			r->prev->next = new;
		new->size += r->size;
		new->start = r->start;

		free(r);
		r = new;
	}

	if (r->prev && !r->prev->in_use) {
		r->prev->next = r->next;
		if (r->next)
			r->next->prev = r->prev;
		r->prev->size += r->size;
		free(r);
	}
	
}
