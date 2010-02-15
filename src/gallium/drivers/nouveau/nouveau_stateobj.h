#ifndef __NOUVEAU_STATEOBJ_H__
#define __NOUVEAU_STATEOBJ_H__

#include "util/u_debug.h"

#ifdef DEBUG
#define DEBUG_NOUVEAU_STATEOBJ
#endif /* DEBUG */

struct nouveau_stateobj_reloc {
	struct nouveau_bo *bo;

	struct nouveau_grobj *gr;
	uint32_t push_offset;
	uint32_t mthd;

	uint32_t data;
	unsigned flags;
	unsigned vor;
	unsigned tor;
};

struct nouveau_stateobj_start {
	struct nouveau_grobj *gr;
	uint32_t mthd;
	uint32_t size;
	unsigned offset;
};

struct nouveau_stateobj {
	struct pipe_reference reference;

	struct nouveau_stateobj_start *start;
	struct nouveau_stateobj_reloc *reloc;

	/* Common memory pool for data. */
	uint32_t *pool;
	unsigned pool_cur;

#ifdef DEBUG_NOUVEAU_STATEOBJ
	unsigned start_alloc;
	unsigned reloc_alloc;
	unsigned pool_alloc;
#endif  /* DEBUG_NOUVEAU_STATEOBJ */

	unsigned total; /* includes begin_ring */
	unsigned cur; /* excludes begin_ring, offset from "cur_start" */
	unsigned cur_start;
	unsigned cur_reloc;
};

static INLINE void
so_dump(struct nouveau_stateobj *so)
{
	unsigned i, nr, total = 0;

	for (i = 0; i < so->cur_start; i++) {
		if (so->start[i].gr->subc > -1)
			debug_printf("+0x%04x: 0x%08x\n", total++,
				(so->start[i].size << 18) | (so->start[i].gr->subc << 13)
				| so->start[i].mthd);
		else
			debug_printf("+0x%04x: 0x%08x\n", total++,
				(so->start[i].size << 18) | so->start[i].mthd);
		for (nr = 0; nr < so->start[i].size; nr++, total++)
			debug_printf("+0x%04x: 0x%08x\n", total,
				so->pool[so->start[i].offset + nr]);
	}
}

static INLINE struct nouveau_stateobj *
so_new(unsigned start, unsigned push, unsigned reloc)
{
	struct nouveau_stateobj *so;

	so = MALLOC(sizeof(struct nouveau_stateobj));
	pipe_reference_init(&so->reference, 1);
	so->total = so->cur = so->cur_start = so->cur_reloc = 0;

#ifdef DEBUG_NOUVEAU_STATEOBJ
	so->start_alloc = start;
	so->reloc_alloc = reloc;
	so->pool_alloc = push;
#endif /* DEBUG_NOUVEAU_STATEOBJ */

	so->start = MALLOC(start * sizeof(struct nouveau_stateobj_start));
	so->reloc = MALLOC(reloc * sizeof(struct nouveau_stateobj_reloc));
	so->pool = MALLOC(push * sizeof(uint32_t));
	so->pool_cur = 0;

	if (!so->start || !so->reloc || !so->pool) {
		debug_printf("malloc failed\n");
		assert(0);
	}

	return so;
}

static INLINE void
so_ref(struct nouveau_stateobj *ref, struct nouveau_stateobj **pso)
{
	struct nouveau_stateobj *so = *pso;
	int i;

	if (pipe_reference(&(*pso)->reference, &ref->reference)) {
		FREE(so->start);
		for (i = 0; i < so->cur_reloc; i++)
			nouveau_bo_ref(NULL, &so->reloc[i].bo);
		FREE(so->reloc);
		FREE(so->pool);
		FREE(so);
	}
	*pso = ref;
}

static INLINE void
so_data(struct nouveau_stateobj *so, uint32_t data)
{
#ifdef DEBUG_NOUVEAU_STATEOBJ
	if (so->cur >= so->start[so->cur_start - 1].size) {
		debug_printf("exceeding specified size\n");
		assert(0);
	}
#endif /* DEBUG_NOUVEAU_STATEOBJ */

	so->pool[so->start[so->cur_start - 1].offset + so->cur++] = data;
}

static INLINE void
so_datap(struct nouveau_stateobj *so, uint32_t *data, unsigned size)
{
#ifdef DEBUG_NOUVEAU_STATEOBJ
	if ((so->cur + size) > so->start[so->cur_start - 1].size) {
		debug_printf("exceeding specified size\n");
		assert(0);
	}
#endif /* DEBUG_NOUVEAU_STATEOBJ */

	while (size--)
		so->pool[so->start[so->cur_start - 1].offset + so->cur++] =
			*data++;
}

static INLINE void
so_method(struct nouveau_stateobj *so, struct nouveau_grobj *gr,
	  unsigned mthd, unsigned size)
{
	struct nouveau_stateobj_start *start;

#ifdef DEBUG_NOUVEAU_STATEOBJ
	if (so->start_alloc <= so->cur_start) {
		debug_printf("exceeding num_start size\n");
		assert(0);
	} else
#endif /* DEBUG_NOUVEAU_STATEOBJ */
		start = so->start;

#ifdef DEBUG_NOUVEAU_STATEOBJ
	if (so->cur_start > 0 && start[so->cur_start - 1].size > so->cur) {
		debug_printf("previous so_method was not filled\n");
		assert(0);
	}
#endif /* DEBUG_NOUVEAU_STATEOBJ */

	so->start = start;
	start[so->cur_start].gr = gr;
	start[so->cur_start].mthd = mthd;
	start[so->cur_start].size = size;

#ifdef DEBUG_NOUVEAU_STATEOBJ
	if (so->pool_alloc < (size + so->pool_cur)) {
		debug_printf("exceeding num_pool size\n");
		assert(0);
	}
#endif /* DEBUG_NOUVEAU_STATEOBJ */

	start[so->cur_start].offset = so->pool_cur;
	so->pool_cur += size;

	so->cur_start++;
	/* The 1 is for *this* begin_ring. */
	so->total += so->cur + 1;
	so->cur = 0;
}

static INLINE void
so_reloc(struct nouveau_stateobj *so, struct nouveau_bo *bo,
	 unsigned data, unsigned flags, unsigned vor, unsigned tor)
{
	struct nouveau_stateobj_reloc *r;

#ifdef DEBUG_NOUVEAU_STATEOBJ
	if (so->reloc_alloc <= so->cur_reloc) {
		debug_printf("exceeding num_reloc size\n");
		assert(0);
	} else
#endif /* DEBUG_NOUVEAU_STATEOBJ */
		r = so->reloc;

	so->reloc = r;
	r[so->cur_reloc].bo = NULL;
	nouveau_bo_ref(bo, &(r[so->cur_reloc].bo));
	r[so->cur_reloc].gr = so->start[so->cur_start-1].gr;
	r[so->cur_reloc].push_offset = so->total + so->cur;
	r[so->cur_reloc].data = data;
	r[so->cur_reloc].flags = flags;
	r[so->cur_reloc].mthd = so->start[so->cur_start-1].mthd +
							(so->cur << 2);
	r[so->cur_reloc].vor = vor;
	r[so->cur_reloc].tor = tor;

	so_data(so, data);
	so->cur_reloc++;
}

/* Determine if this buffer object is referenced by this state object. */
static INLINE boolean
so_bo_is_reloc(struct nouveau_stateobj *so, struct nouveau_bo *bo)
{
	int i;

	for (i = 0; i < so->cur_reloc; i++)
		if (so->reloc[i].bo == bo)
			return true;

	return false;
}

static INLINE void
so_emit(struct nouveau_channel *chan, struct nouveau_stateobj *so)
{
	unsigned nr, i;
	int ret = 0;

#ifdef DEBUG_NOUVEAU_STATEOBJ
	if (so->start[so->cur_start - 1].size > so->cur) {
		debug_printf("emit: previous so_method was not filled\n");
		assert(0);
	}
#endif /* DEBUG_NOUVEAU_STATEOBJ */

	/* We cannot update total in case we so_emit again. */
	nr = so->total + so->cur;

	/* This will flush if we need space.
	 * We don't actually need the marker.
	 */
	if ((ret = nouveau_pushbuf_marker_emit(chan, nr, so->cur_reloc))) {
		debug_printf("so_emit failed marker emit with error %d\n", ret);
		assert(0);
	}

	/* Submit data. This will ensure proper binding of objects. */
	for (i = 0; i < so->cur_start; i++) {
		BEGIN_RING(chan, so->start[i].gr, so->start[i].mthd, so->start[i].size);
		OUT_RINGp(chan, &(so->pool[so->start[i].offset]), so->start[i].size);
	}

	for (i = 0; i < so->cur_reloc; i++) {
		struct nouveau_stateobj_reloc *r = &so->reloc[i];

		if ((ret = nouveau_pushbuf_emit_reloc(chan, chan->cur - nr +
						r->push_offset, r->bo, r->data,
						0, r->flags, r->vor, r->tor))) {
			debug_printf("so_emit failed reloc with error %d\n", ret);
			assert(0);
		}
	}
}

static INLINE void
so_emit_reloc_markers(struct nouveau_channel *chan, struct nouveau_stateobj *so)
{
	struct nouveau_grobj *gr = NULL;
	unsigned i;
	int ret = 0;

	if (!so)
		return;

	/* If we need to flush in flush notify, then we have a problem anyway. */
	for (i = 0; i < so->cur_reloc; i++) {
		struct nouveau_stateobj_reloc *r = &so->reloc[i];

#ifdef DEBUG_NOUVEAU_STATEOBJ
		if (r->mthd & 0x40000000) {
			debug_printf("error: NI mthd 0x%08X\n", r->mthd);
			continue;
		}
#endif /* DEBUG_NOUVEAU_STATEOBJ */

		/* The object needs to be bound and the system must know the
		 * subchannel is being used. Otherwise it will discard it.
		 */
		if (gr != r->gr) {
			BEGIN_RING(chan, r->gr, 0x100, 1);
			OUT_RING(chan, 0);
			gr = r->gr;
		}

		/* Some relocs really don't like to be hammered,
		 * NOUVEAU_BO_DUMMY makes sure it only
		 * happens when needed.
		 */
		ret = OUT_RELOC(chan, r->bo, (r->gr->subc << 13) | (1<< 18) |
			r->mthd, (r->flags & (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART
				| NOUVEAU_BO_RDWR)) | NOUVEAU_BO_DUMMY, 0, 0);
		if (ret) {
			debug_printf("OUT_RELOC failed %d\n", ret);
			assert(0);
		}

		ret = OUT_RELOC(chan, r->bo, r->data, r->flags |
			NOUVEAU_BO_DUMMY, r->vor, r->tor);
		if (ret) {
			debug_printf("OUT_RELOC failed %d\n", ret);
			assert(0);
		}
	}
}

#endif
