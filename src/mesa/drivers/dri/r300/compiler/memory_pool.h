/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

struct memory_block;

/**
 * Provides a pool of memory that can quickly be allocated from, at the
 * cost of being unable to explicitly free one of the allocated blocks.
 * Instead, the entire pool can be freed at once.
 *
 * The idea is to allow one to quickly allocate a flexible amount of
 * memory during operations like shader compilation while avoiding
 * reference counting headaches.
 */
struct memory_pool {
	unsigned char * head;
	unsigned char * end;
	unsigned int total_allocated;
	struct memory_block * blocks;
};


void memory_pool_init(struct memory_pool * pool);
void memory_pool_destroy(struct memory_pool * pool);
void * memory_pool_malloc(struct memory_pool * pool, unsigned int bytes);

#endif /* MEMORY_POOL_H */
