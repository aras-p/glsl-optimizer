#ifndef NOUVEAU_WINSYS_H
#define NOUVEAU_WINSYS_H

#include <stdint.h>
#include "pipe/p_winsys.h"
#include "pipe/p_defines.h"

#include "nouveau/nouveau_bo.h"
#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_class.h"
#include "nouveau/nouveau_grobj.h"
#include "nouveau/nouveau_notifier.h"
#include "nouveau/nouveau_resource.h"
#include "nouveau/nouveau_pushbuf.h"

struct nouveau_winsys {
	struct nouveau_context *nv;

	struct nouveau_channel *channel;

	int  (*res_init)(struct nouveau_resource **heap, unsigned start,
			 unsigned size);
	int  (*res_alloc)(struct nouveau_resource *heap, int size, void *priv,
			  struct nouveau_resource **);
	void (*res_free)(struct nouveau_resource **);

	int  (*push_reloc)(struct nouveau_winsys *, void *ptr,
			   struct pipe_buffer *, uint32_t data,
			   uint32_t flags, uint32_t vor, uint32_t tor);
	int  (*push_flush)(struct nouveau_winsys *, unsigned size);
			       
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

	int (*surface_copy)(struct nouveau_winsys *, struct pipe_surface *,
			    unsigned, unsigned, struct pipe_surface *,
			    unsigned, unsigned, unsigned, unsigned);
	int (*surface_fill)(struct nouveau_winsys *, struct pipe_surface *,
			    unsigned, unsigned, unsigned, unsigned, unsigned);
};

extern struct pipe_screen *
nv10_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *,
		   unsigned chipset);

extern struct pipe_context *
nv10_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv30_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *,
		   unsigned chipset);

extern struct pipe_context *
nv30_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv40_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *,
		   unsigned chipset);

extern struct pipe_context *
nv40_create(struct pipe_screen *, unsigned pctx_id);

extern struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *,
		   unsigned chipset);

extern struct pipe_context *
nv50_create(struct pipe_screen *, unsigned pctx_id);

#endif
