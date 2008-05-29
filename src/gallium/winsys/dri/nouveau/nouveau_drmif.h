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

#ifndef __NOUVEAU_DRMIF_H__
#define __NOUVEAU_DRMIF_H__

#include <stdint.h>
#include <xf86drm.h>
#include <nouveau_drm.h>

#include "nouveau/nouveau_device.h"
#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_grobj.h"
#include "nouveau/nouveau_notifier.h"
#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_resource.h"
#include "nouveau/nouveau_pushbuf.h"

struct nouveau_device_priv {
	struct nouveau_device base;

	int fd;
	drm_context_t ctx;
	drmLock *lock;
	int needs_close;

	struct drm_nouveau_mem_alloc sa;
	void *sa_map;
	struct nouveau_resource *sa_heap;
};
#define nouveau_device(n) ((struct nouveau_device_priv *)(n))

extern int
nouveau_device_open_existing(struct nouveau_device **, int close,
			     int fd, drm_context_t ctx);

extern int
nouveau_device_open(struct nouveau_device **, const char *busid);

extern void
nouveau_device_close(struct nouveau_device **);

extern int
nouveau_device_get_param(struct nouveau_device *, uint64_t param, uint64_t *v);

extern int
nouveau_device_set_param(struct nouveau_device *, uint64_t param, uint64_t val);

struct nouveau_fence {
	struct nouveau_channel *channel;
};

struct nouveau_fence_cb {
	struct nouveau_fence_cb *next;
	void (*func)(void *);
	void *priv;
};

struct nouveau_fence_priv {
	struct nouveau_fence base;
	int refcount;

	struct nouveau_fence *next;
	struct nouveau_fence_cb *signal_cb;

	uint32_t sequence;
	int emitted;
	int signalled;
};
#define nouveau_fence(n) ((struct nouveau_fence_priv *)(n))

extern int
nouveau_fence_new(struct nouveau_channel *, struct nouveau_fence **);

extern int
nouveau_fence_ref(struct nouveau_fence *, struct nouveau_fence **);

extern int
nouveau_fence_signal_cb(struct nouveau_fence *, void (*)(void *), void *);

extern void
nouveau_fence_emit(struct nouveau_fence *);

extern int
nouveau_fence_wait(struct nouveau_fence **);

extern void
nouveau_fence_flush(struct nouveau_channel *);

struct nouveau_pushbuf_reloc {
	struct nouveau_pushbuf_bo *pbbo;
	uint32_t *ptr;
	uint32_t flags;
	uint32_t data;
	uint32_t vor;
	uint32_t tor;
};

struct nouveau_pushbuf_bo {
	struct nouveau_channel *channel;
	struct nouveau_bo *bo;
	unsigned flags;
	unsigned handled;
};

#define NOUVEAU_PUSHBUF_MAX_BUFFERS 1024
#define NOUVEAU_PUSHBUF_MAX_RELOCS 1024
struct nouveau_pushbuf_priv {
	struct nouveau_pushbuf base;

	struct nouveau_fence *fence;

	unsigned nop_jump;
	unsigned start;
	unsigned size;

	struct nouveau_pushbuf_bo *buffers;
	unsigned nr_buffers;
	struct nouveau_pushbuf_reloc *relocs;
	unsigned nr_relocs;
};
#define nouveau_pushbuf(n) ((struct nouveau_pushbuf_priv *)(n))

#define pbbo_to_ptr(o) ((uint64_t)(unsigned long)(o))
#define ptr_to_pbbo(h) ((struct nouveau_pushbuf_bo *)(unsigned long)(h))
#define pbrel_to_ptr(o) ((uint64_t)(unsigned long)(o))
#define ptr_to_pbrel(h) ((struct nouveau_pushbuf_reloc *)(unsigned long)(h))
#define bo_to_ptr(o) ((uint64_t)(unsigned long)(o))
#define ptr_to_bo(h) ((struct nouveau_bo_priv *)(unsigned long)(h))

extern int
nouveau_pushbuf_init(struct nouveau_channel *);

extern int
nouveau_pushbuf_flush(struct nouveau_channel *, unsigned min);

extern int
nouveau_pushbuf_emit_reloc(struct nouveau_channel *, void *ptr,
			   struct nouveau_bo *, uint32_t data, uint32_t flags,
			   uint32_t vor, uint32_t tor);

struct nouveau_dma_priv {
	uint32_t base;
	uint32_t max;
	uint32_t cur;
	uint32_t put;
	uint32_t free;

	int push_free;
} dma;

struct nouveau_channel_priv {
	struct nouveau_channel base;

	struct drm_nouveau_channel_alloc drm;

	uint32_t *pushbuf;
	void     *notifier_block;

	volatile uint32_t *user;
	volatile uint32_t *put;
	volatile uint32_t *get;
	volatile uint32_t *ref_cnt;

	struct nouveau_dma_priv dma_master;
	struct nouveau_dma_priv dma_bufmgr;
	struct nouveau_dma_priv *dma;

	struct nouveau_fence *fence_head;
	struct nouveau_fence *fence_tail;
	uint32_t fence_sequence;

	struct nouveau_pushbuf_priv pb;

	unsigned user_charge;
};
#define nouveau_channel(n) ((struct nouveau_channel_priv *)(n))

extern int
nouveau_channel_alloc(struct nouveau_device *, uint32_t fb, uint32_t tt,
		      struct nouveau_channel **);

extern void
nouveau_channel_free(struct nouveau_channel **);

struct nouveau_grobj_priv {
	struct nouveau_grobj base;
};
#define nouveau_grobj(n) ((struct nouveau_grobj_priv *)(n))

extern int nouveau_grobj_alloc(struct nouveau_channel *, uint32_t handle,
			       int class, struct nouveau_grobj **);
extern int nouveau_grobj_ref(struct nouveau_channel *, uint32_t handle,
			     struct nouveau_grobj **);
extern void nouveau_grobj_free(struct nouveau_grobj **);


struct nouveau_notifier_priv {
	struct nouveau_notifier base;

	struct drm_nouveau_notifierobj_alloc drm;
	volatile void *map;
};
#define nouveau_notifier(n) ((struct nouveau_notifier_priv *)(n))

extern int
nouveau_notifier_alloc(struct nouveau_channel *, uint32_t handle, int count,
		       struct nouveau_notifier **);

extern void
nouveau_notifier_free(struct nouveau_notifier **);

extern void
nouveau_notifier_reset(struct nouveau_notifier *, int id);

extern uint32_t
nouveau_notifier_status(struct nouveau_notifier *, int id);

extern uint32_t
nouveau_notifier_return_val(struct nouveau_notifier *, int id);

extern int
nouveau_notifier_wait_status(struct nouveau_notifier *, int id, int status,
			     int timeout);

struct nouveau_bo_priv {
	struct nouveau_bo base;

	struct nouveau_pushbuf_bo *pending;
	struct nouveau_fence *fence;
	struct nouveau_fence *wr_fence;

	struct drm_nouveau_mem_alloc drm;
	void *map;

	void *sysmem;
	int user;

	int refcount;

	uint64_t offset;
	uint64_t flags;
};
#define nouveau_bo(n) ((struct nouveau_bo_priv *)(n))

extern int
nouveau_bo_init(struct nouveau_device *);

extern void
nouveau_bo_takedown(struct nouveau_device *);

extern int
nouveau_bo_new(struct nouveau_device *, uint32_t flags, int align, int size,
	       struct nouveau_bo **);

extern int
nouveau_bo_user(struct nouveau_device *, void *ptr, int size,
		struct nouveau_bo **);

extern int
nouveau_bo_ref(struct nouveau_device *, uint64_t handle, struct nouveau_bo **);

extern int
nouveau_bo_set_status(struct nouveau_bo *, uint32_t flags);

extern void
nouveau_bo_del(struct nouveau_bo **);

extern int
nouveau_bo_map(struct nouveau_bo *, uint32_t flags);

extern void
nouveau_bo_unmap(struct nouveau_bo *);

extern int
nouveau_bo_validate(struct nouveau_channel *, struct nouveau_bo *,
		    uint32_t flags);

extern int
nouveau_resource_init(struct nouveau_resource **heap, unsigned start,
		      unsigned size);

extern int
nouveau_resource_alloc(struct nouveau_resource *heap, int size, void *priv,
		       struct nouveau_resource **);

extern void
nouveau_resource_free(struct nouveau_resource **);

#endif
