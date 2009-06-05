#ifndef __NOUVEAU_STATEOBJ_H__
#define __NOUVEAU_STATEOBJ_H__

#include "util/u_debug.h"

struct nouveau_stateobj_reloc {
	struct nouveau_bo *bo;

	unsigned offset;
	unsigned packet;

	unsigned data;
	unsigned flags;
	unsigned vor;
	unsigned tor;
};

struct nouveau_stateobj {
	struct pipe_reference reference;

	unsigned *push;
	struct nouveau_stateobj_reloc *reloc;

	unsigned *cur;
	unsigned cur_packet;
	unsigned cur_reloc;
};

static INLINE struct nouveau_stateobj *
so_new(unsigned push, unsigned reloc)
{
	struct nouveau_stateobj *so;

	so = MALLOC(sizeof(struct nouveau_stateobj));
	pipe_reference_init(&so->reference, 1);
	so->push = MALLOC(sizeof(unsigned) * push);
	so->reloc = MALLOC(sizeof(struct nouveau_stateobj_reloc) * reloc);

	so->cur = so->push;
	so->cur_reloc = so->cur_packet = 0;

	return so;
}

static INLINE void
so_ref(struct nouveau_stateobj *ref, struct nouveau_stateobj **pso)
{
	struct nouveau_stateobj *so = *pso;
	int i;

        if (pipe_reference((struct pipe_reference**)pso, &ref->reference)) {
		free(so->push);
		for (i = 0; i < so->cur_reloc; i++)
			nouveau_bo_ref(NULL, &so->reloc[i].bo);
		free(so->reloc);
		free(so);
	}
}

static INLINE void
so_data(struct nouveau_stateobj *so, unsigned data)
{
	(*so->cur++) = (data);
	so->cur_packet += 4;
}

static INLINE void
so_datap(struct nouveau_stateobj *so, unsigned *data, unsigned size)
{
	so->cur_packet += (4 * size);
	while (size--)
		(*so->cur++) = (*data++);
}

static INLINE void
so_method(struct nouveau_stateobj *so, struct nouveau_grobj *gr,
	  unsigned mthd, unsigned size)
{
	so->cur_packet = (gr->subc << 13) | (1 << 18) | (mthd - 4);
	so_data(so, (gr->subc << 13) | (size << 18) | mthd);
}

static INLINE void
so_reloc(struct nouveau_stateobj *so, struct nouveau_bo *bo,
	 unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	struct nouveau_stateobj_reloc *r = &so->reloc[so->cur_reloc++];
	
	r->bo = NULL;
	nouveau_bo_ref(bo, &r->bo);
	r->offset = so->cur - so->push;
	r->packet = so->cur_packet;
	r->data = data;
	r->flags = flags;
	r->vor = vor;
	r->tor = tor;
	so_data(so, data);
}

static INLINE void
so_dump(struct nouveau_stateobj *so)
{
	unsigned i, nr = so->cur - so->push;

	for (i = 0; i < nr; i++)
		debug_printf("+0x%04x: 0x%08x\n", i, so->push[i]);
}

static INLINE void
so_emit(struct nouveau_channel *chan, struct nouveau_stateobj *so)
{
	struct nouveau_pushbuf *pb = chan->pushbuf;
	unsigned nr, i;

	nr = so->cur - so->push;
	if (pb->remaining < nr)
		nouveau_pushbuf_flush(chan, nr);
	pb->remaining -= nr;

	memcpy(pb->cur, so->push, nr * 4);
	for (i = 0; i < so->cur_reloc; i++) {
		struct nouveau_stateobj_reloc *r = &so->reloc[i];

		nouveau_pushbuf_emit_reloc(chan, pb->cur + r->offset,
					   r->bo, r->data, 0, r->flags,
					   r->vor, r->tor);
	}
	pb->cur += nr;
}

static INLINE void
so_emit_reloc_markers(struct nouveau_channel *chan, struct nouveau_stateobj *so)
{
	struct nouveau_pushbuf *pb = chan->pushbuf;
	unsigned i;

	if (!so)
		return;

	i = so->cur_reloc << 1;
	if (pb->remaining < i)
		nouveau_pushbuf_flush(chan, i);
	pb->remaining -= i;

	for (i = 0; i < so->cur_reloc; i++) {
		struct nouveau_stateobj_reloc *r = &so->reloc[i];

		nouveau_pushbuf_emit_reloc(chan, pb->cur++, r->bo, r->packet, 0,
					   (r->flags & (NOUVEAU_BO_VRAM |
							NOUVEAU_BO_GART |
							NOUVEAU_BO_RDWR)) |
					   NOUVEAU_BO_DUMMY, 0, 0);
		nouveau_pushbuf_emit_reloc(chan, pb->cur++, r->bo, r->data, 0,
					   r->flags | NOUVEAU_BO_DUMMY,
					   r->vor, r->tor);
	}
}

#endif
