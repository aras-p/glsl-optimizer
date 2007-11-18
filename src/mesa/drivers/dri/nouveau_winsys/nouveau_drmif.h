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

#include "nouveau_device.h"
#include "pipe/nouveau/nouveau_channel.h"
#include "pipe/nouveau/nouveau_grobj.h"
#include "pipe/nouveau/nouveau_notifier.h"
#include "pipe/nouveau/nouveau_bo.h"

struct nouveau_device_priv {
	struct nouveau_device base;

	int fd;
	drm_context_t ctx;
	drmLock *lock;
	int needs_close;

	struct {
		struct nouveau_channel *channel;
		struct nouveau_notifier *notify;
		struct nouveau_grobj *m2mf;
	} bufmgr;
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

struct nouveau_channel_priv {
	struct nouveau_channel base;

	struct drm_nouveau_channel_alloc drm;

	struct {
		struct nouveau_grobj *grobj;
		uint32_t seq;
	} subchannel[8];
	uint32_t subc_sequence;

	uint32_t *pushbuf;
	void     *notifier_block;

	volatile uint32_t *user;
	volatile uint32_t *put;
	volatile uint32_t *get;
	volatile uint32_t *ref_cnt;

	struct {
		uint32_t base, max;
		uint32_t cur, put;
		uint32_t free;

		int push_free;
	} dma;

	struct {
		struct nouveau_bo_priv *bo;
		uint32_t flags;
	} buffers[128];
	int nr_buffers;

	struct nouveau_bo_reloc *relocs;
	int num_relocs;
	int max_relocs;
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

	struct drm_nouveau_mem_alloc drm;
	void *map;

	void *sysmem;
	int user;

	int refcount;
};

struct nouveau_bo_reloc {
	struct nouveau_bo_priv *bo;
	uint32_t *ptr;
	uint32_t flags;
	uint32_t data, vor, tor;
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
nouveau_bo_resize(struct nouveau_bo *, int size);

extern void
nouveau_bo_del(struct nouveau_bo **);

extern int
nouveau_bo_map(struct nouveau_bo *, uint32_t flags);

extern void
nouveau_bo_unmap(struct nouveau_bo *);

extern void
nouveau_bo_emit_reloc(struct nouveau_channel *chan, void *ptr,
		      struct nouveau_bo *, uint32_t data, uint32_t flags,
		      uint32_t vor, uint32_t tor);

extern void
nouveau_bo_validate(struct nouveau_channel *);

#endif
