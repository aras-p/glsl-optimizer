#ifndef NOUVEAU_WINSYS_H
#define NOUVEAU_WINSYS_H

#include <stdint.h>
#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"

#include "pipe/nouveau/nouveau_bo.h"
#include "pipe/nouveau/nouveau_channel.h"
#include "pipe/nouveau/nouveau_class.h"
#include "pipe/nouveau/nouveau_grobj.h"
#include "pipe/nouveau/nouveau_notifier.h"

struct nouveau_resource {
	struct nouveau_resource *prev;
	struct nouveau_resource *next;

	boolean in_use;
	void *priv;

	uint start;
	uint size;
};

struct nouveau_winsys {
	struct nouveau_context *nv;

	struct nouveau_channel *channel;

	int  (*res_init)(struct nouveau_resource **heap, int size);
	int  (*res_alloc)(struct nouveau_resource *heap, int size, void *priv,
			  struct nouveau_resource **);
	void (*res_free)(struct nouveau_resource **);

	/*XXX: this is crappy, and bound to be slow.. however, it's nice and
	 *     simple, it'll do for the moment*/
	uint32_t *(*begin_ring)(struct nouveau_grobj *, int mthd, int size);
	void      (*out_reloc)(struct nouveau_channel *, void *ptr,
			       struct nouveau_bo *, uint32_t data,
			       uint32_t flags, uint32_t vor, uint32_t tor);
	void      (*fire_ring)(struct nouveau_channel *);
			       
	int       (*grobj_alloc)(struct nouveau_winsys *, int grclass,
				 struct nouveau_grobj **);
	void      (*grobj_free)(struct nouveau_grobj **);

	int       (*notifier_alloc)(struct nouveau_winsys *, int count,
				    struct nouveau_notifier **);
	void      (*notifier_free)(struct nouveau_notifier **);
	void      (*notifier_reset)(struct nouveau_notifier *, int id);
	uint32_t  (*notifier_status)(struct nouveau_notifier *, int id);
	uint32_t  (*notifier_retval)(struct nouveau_notifier *, int id);
	int       (*notifier_wait)(struct nouveau_notifier *, int id,
				   int status, int timeout);

	int (*region_copy)(struct nouveau_context *, struct pipe_region *,
			   unsigned, unsigned, unsigned, struct pipe_region *,
			   unsigned, unsigned, unsigned, unsigned, unsigned);
	int (*region_fill)(struct nouveau_context *, struct pipe_region *,
			   unsigned, unsigned, unsigned, unsigned, unsigned,
			   unsigned);
	int (*region_data)(struct nouveau_context *, struct pipe_region *,
			   unsigned, unsigned, unsigned, const void *,
			   unsigned, unsigned, unsigned, unsigned, unsigned);
};

extern struct pipe_context *
nv40_create(struct pipe_winsys *, struct nouveau_winsys *, unsigned chipset);

#endif
