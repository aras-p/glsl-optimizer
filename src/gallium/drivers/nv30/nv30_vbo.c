#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "nv30_context.h"
#include "nv30_state.h"

#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_pushbuf.h"
#include "nouveau/nouveau_util.h"

#define FORCE_SWTNL 0

static INLINE int
nv30_vbo_format_to_hw(enum pipe_format pipe, unsigned *fmt, unsigned *ncomp)
{
	switch (pipe) {
	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R32G32B32_FLOAT:
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
		*fmt = NV34TCL_VTXFMT_TYPE_FLOAT;
		break;
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R8G8B8_UNORM:
	case PIPE_FORMAT_R8G8B8A8_UNORM:
		*fmt = NV34TCL_VTXFMT_TYPE_UBYTE;
		break;
	case PIPE_FORMAT_R16_SSCALED:
	case PIPE_FORMAT_R16G16_SSCALED:
	case PIPE_FORMAT_R16G16B16_SSCALED:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
		*fmt = NV34TCL_VTXFMT_TYPE_USHORT;
		break;
	default:
		NOUVEAU_ERR("Unknown format %s\n", util_format_name(pipe));
		return 1;
	}

	switch (pipe) {
	case PIPE_FORMAT_R8_UNORM:
	case PIPE_FORMAT_R32_FLOAT:
	case PIPE_FORMAT_R16_SSCALED:
		*ncomp = 1;
		break;
	case PIPE_FORMAT_R8G8_UNORM:
	case PIPE_FORMAT_R32G32_FLOAT:
	case PIPE_FORMAT_R16G16_SSCALED:
		*ncomp = 2;
		break;
	case PIPE_FORMAT_R8G8B8_UNORM:
	case PIPE_FORMAT_R32G32B32_FLOAT:
	case PIPE_FORMAT_R16G16B16_SSCALED:
		*ncomp = 3;
		break;
	case PIPE_FORMAT_R8G8B8A8_UNORM:
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R16G16B16A16_SSCALED:
		*ncomp = 4;
		break;
	default:
		NOUVEAU_ERR("Unknown format %s\n", util_format_name(pipe));
		return 1;
	}

	return 0;
}

static boolean
nv30_vbo_set_idxbuf(struct nv30_context *nv30, struct pipe_buffer *ib,
		    unsigned ib_size)
{
	struct pipe_screen *pscreen = &nv30->screen->base.base;
	unsigned type;

	if (!ib) {
		nv30->idxbuf = NULL;
		nv30->idxbuf_format = 0xdeadbeef;
		return FALSE;
	}

	if (!pscreen->get_param(pscreen, NOUVEAU_CAP_HW_IDXBUF) || ib_size == 1)
		return FALSE;

	switch (ib_size) {
	case 2:
		type = NV34TCL_IDXBUF_FORMAT_TYPE_U16;
		break;
	case 4:
		type = NV34TCL_IDXBUF_FORMAT_TYPE_U32;
		break;
	default:
		return FALSE;
	}

	if (ib != nv30->idxbuf ||
	    type != nv30->idxbuf_format) {
		nv30->dirty |= NV30_NEW_ARRAYS;
		nv30->idxbuf = ib;
		nv30->idxbuf_format = type;
	}

	return TRUE;
}

static boolean
nv30_vbo_static_attrib(struct nv30_context *nv30, struct nouveau_stateobj *so,
		       int attrib, struct pipe_vertex_element *ve,
		       struct pipe_vertex_buffer *vb)
{
	struct pipe_screen *pscreen = nv30->pipe.screen;
	struct nouveau_grobj *rankine = nv30->screen->rankine;
	unsigned type, ncomp;
	void *map;

	if (nv30_vbo_format_to_hw(ve->src_format, &type, &ncomp))
		return FALSE;

	map  = pipe_buffer_map(pscreen, vb->buffer, PIPE_BUFFER_USAGE_CPU_READ);
	map += vb->buffer_offset + ve->src_offset;

	switch (type) {
	case NV34TCL_VTXFMT_TYPE_FLOAT:
	{
		float *v = map;

		switch (ncomp) {
		case 4:
			so_method(so, rankine, NV34TCL_VTX_ATTR_4F_X(attrib), 4);
			so_data  (so, fui(v[0]));
			so_data  (so, fui(v[1]));
			so_data  (so, fui(v[2]));
			so_data  (so, fui(v[3]));
			break;
		case 3:
			so_method(so, rankine, NV34TCL_VTX_ATTR_3F_X(attrib), 3);
			so_data  (so, fui(v[0]));
			so_data  (so, fui(v[1]));
			so_data  (so, fui(v[2]));
			break;
		case 2:
			so_method(so, rankine, NV34TCL_VTX_ATTR_2F_X(attrib), 2);
			so_data  (so, fui(v[0]));
			so_data  (so, fui(v[1]));
			break;
		case 1:
			so_method(so, rankine, NV34TCL_VTX_ATTR_1F(attrib), 1);
			so_data  (so, fui(v[0]));
			break;
		default:
			pipe_buffer_unmap(pscreen, vb->buffer);
			return FALSE;
		}
	}
		break;
	default:
		pipe_buffer_unmap(pscreen, vb->buffer);
		return FALSE;
	}

	pipe_buffer_unmap(pscreen, vb->buffer);
	return TRUE;
}

void
nv30_draw_arrays(struct pipe_context *pipe,
		 unsigned mode, unsigned start, unsigned count)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;
	unsigned restart = 0;

	nv30_vbo_set_idxbuf(nv30, NULL, 0);
	if (FORCE_SWTNL || !nv30_state_validate(nv30)) {
		/*return nv30_draw_elements_swtnl(pipe, NULL, 0,
						mode, start, count);*/
		return;
	}

	while (count) {
		unsigned vc, nr;

		nv30_state_emit(nv30);

		vc = nouveau_vbuf_split(AVAIL_RING(chan), 6, 256,
					mode, start, count, &restart);
		if (!vc) {
			FIRE_RING(chan);
			continue;
		}

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, nvgl_primitive(mode));

		nr = (vc & 0xff);
		if (nr) {
			BEGIN_RING(chan, rankine, NV34TCL_VB_VERTEX_BATCH, 1);
			OUT_RING  (chan, ((nr - 1) << 24) | start);
			start += nr;
		}

		nr = vc >> 8;
		while (nr) {
			unsigned push = nr > 2047 ? 2047 : nr;

			nr -= push;

			BEGIN_RING_NI(chan, rankine, NV34TCL_VB_VERTEX_BATCH, push);
			while (push--) {
				OUT_RING(chan, ((0x100 - 1) << 24) | start);
				start += 0x100;
			}
		}

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, 0);

		count -= vc;
		start = restart;
	}

	pipe->flush(pipe, 0, NULL);
}

static INLINE void
nv30_draw_elements_u08(struct nv30_context *nv30, void *ib,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;

	while (count) {
		uint8_t *elts = (uint8_t *)ib + start;
		unsigned vc, push, restart = 0;

		nv30_state_emit(nv30);

		vc = nouveau_vbuf_split(AVAIL_RING(chan), 6, 2,
					mode, start, count, &restart);
		if (vc == 0) {
			FIRE_RING(chan);
			continue;
		}
		count -= vc;

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, nvgl_primitive(mode));

		if (vc & 1) {
			BEGIN_RING(chan, rankine, NV34TCL_VB_ELEMENT_U32, 1);
			OUT_RING  (chan, elts[0]);
			elts++; vc--;
		}

		while (vc) {
			unsigned i;

			push = MIN2(vc, 2047 * 2);

			BEGIN_RING_NI(chan, rankine, NV34TCL_VB_ELEMENT_U16, push >> 1);
			for (i = 0; i < push; i+=2)
				OUT_RING(chan, (elts[i+1] << 16) | elts[i]);

			vc -= push;
			elts += push;
		}

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, 0);

		start = restart;
	}
}

static INLINE void
nv30_draw_elements_u16(struct nv30_context *nv30, void *ib,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;

	while (count) {
		uint16_t *elts = (uint16_t *)ib + start;
		unsigned vc, push, restart = 0;

		nv30_state_emit(nv30);

		vc = nouveau_vbuf_split(AVAIL_RING(chan), 6, 2,
					mode, start, count, &restart);
		if (vc == 0) {
			FIRE_RING(chan);
			continue;
		}
		count -= vc;

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, nvgl_primitive(mode));

		if (vc & 1) {
			BEGIN_RING(chan, rankine, NV34TCL_VB_ELEMENT_U32, 1);
			OUT_RING  (chan, elts[0]);
			elts++; vc--;
		}

		while (vc) {
			unsigned i;

			push = MIN2(vc, 2047 * 2);

			BEGIN_RING_NI(chan, rankine, NV34TCL_VB_ELEMENT_U16, push >> 1);
			for (i = 0; i < push; i+=2)
				OUT_RING(chan, (elts[i+1] << 16) | elts[i]);

			vc -= push;
			elts += push;
		}

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, 0);

		start = restart;
	}
}

static INLINE void
nv30_draw_elements_u32(struct nv30_context *nv30, void *ib,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;

	while (count) {
		uint32_t *elts = (uint32_t *)ib + start;
		unsigned vc, push, restart = 0;

		nv30_state_emit(nv30);

		vc = nouveau_vbuf_split(AVAIL_RING(chan), 5, 1,
					mode, start, count, &restart);
		if (vc == 0) {
			FIRE_RING(chan);
			continue;
		}
		count -= vc;

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, nvgl_primitive(mode));

		while (vc) {
			push = MIN2(vc, 2047);

			BEGIN_RING_NI(chan, rankine, NV34TCL_VB_ELEMENT_U32, push);
			OUT_RINGp    (chan, elts, push);

			vc -= push;
			elts += push;
		}

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, 0);

		start = restart;
	}
}

static void
nv30_draw_elements_inline(struct pipe_context *pipe,
			  struct pipe_buffer *ib, unsigned ib_size,
			  unsigned mode, unsigned start, unsigned count)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct pipe_screen *pscreen = pipe->screen;
	void *map;

	map = pipe_buffer_map(pscreen, ib, PIPE_BUFFER_USAGE_CPU_READ);
	if (!ib) {
		NOUVEAU_ERR("failed mapping ib\n");
		return;
	}

	switch (ib_size) {
	case 1:
		nv30_draw_elements_u08(nv30, map, mode, start, count);
		break;
	case 2:
		nv30_draw_elements_u16(nv30, map, mode, start, count);
		break;
	case 4:
		nv30_draw_elements_u32(nv30, map, mode, start, count);
		break;
	default:
		NOUVEAU_ERR("invalid idxbuf fmt %d\n", ib_size);
		break;
	}

	pipe_buffer_unmap(pscreen, ib);
}

static void
nv30_draw_elements_vbo(struct pipe_context *pipe,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;
	unsigned restart = 0;

	while (count) {
		unsigned nr, vc;

		nv30_state_emit(nv30);

		vc = nouveau_vbuf_split(AVAIL_RING(chan), 6, 256,
					mode, start, count, &restart);
		if (!vc) {
			FIRE_RING(chan);
			continue;
		}

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, nvgl_primitive(mode));

		nr = (vc & 0xff);
		if (nr) {
			BEGIN_RING(chan, rankine, NV34TCL_VB_INDEX_BATCH, 1);
			OUT_RING  (chan, ((nr - 1) << 24) | start);
			start += nr;
		}

		nr = vc >> 8;
		while (nr) {
			unsigned push = nr > 2047 ? 2047 : nr;

			nr -= push;

			BEGIN_RING_NI(chan, rankine, NV34TCL_VB_INDEX_BATCH, push);
			while (push--) {
				OUT_RING(chan, ((0x100 - 1) << 24) | start);
				start += 0x100;
			}
		}

		BEGIN_RING(chan, rankine, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, 0);

		count -= vc;
		start = restart;
	}
}

void
nv30_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	boolean idxbuf;

	idxbuf = nv30_vbo_set_idxbuf(nv30, indexBuffer, indexSize);
	if (FORCE_SWTNL || !nv30_state_validate(nv30)) {
		/*return nv30_draw_elements_swtnl(pipe, NULL, 0,
						mode, start, count);*/
		return;
	}

	if (idxbuf) {
		nv30_draw_elements_vbo(pipe, mode, start, count);
	} else {
		nv30_draw_elements_inline(pipe, indexBuffer, indexSize,
					  mode, start, count);
	}

	pipe->flush(pipe, 0, NULL);
}

static boolean
nv30_vbo_validate(struct nv30_context *nv30)
{
	struct nouveau_stateobj *vtxbuf, *vtxfmt, *sattr = NULL;
	struct nouveau_grobj *rankine = nv30->screen->rankine;
	struct pipe_buffer *ib = nv30->idxbuf;
	unsigned ib_format = nv30->idxbuf_format;
	unsigned vb_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;
	int hw;

	vtxbuf = so_new(3, 17, 18);
	so_method(vtxbuf, rankine, NV34TCL_VTXBUF_ADDRESS(0), nv30->vtxelt_nr);
	vtxfmt = so_new(1, 16, 0);
	so_method(vtxfmt, rankine, NV34TCL_VTXFMT(0), nv30->vtxelt_nr);

	for (hw = 0; hw < nv30->vtxelt_nr; hw++) {
		struct pipe_vertex_element *ve;
		struct pipe_vertex_buffer *vb;
		unsigned type, ncomp;

		ve = &nv30->vtxelt[hw];
		vb = &nv30->vtxbuf[ve->vertex_buffer_index];

		if (!vb->stride) {
			if (!sattr)
				sattr = so_new(16, 16 * 4, 0);

			if (nv30_vbo_static_attrib(nv30, sattr, hw, ve, vb)) {
				so_data(vtxbuf, 0);
				so_data(vtxfmt, NV34TCL_VTXFMT_TYPE_FLOAT);
				continue;
			}
		}

		if (nv30_vbo_format_to_hw(ve->src_format, &type, &ncomp)) {
			/*nv30->fallback_swtnl |= NV30_NEW_ARRAYS;*/
			so_ref(NULL, &vtxbuf);
			so_ref(NULL, &vtxfmt);
			return FALSE;
		}

		so_reloc(vtxbuf, nouveau_bo(vb->buffer), vb->buffer_offset +
				 ve->src_offset, vb_flags | NOUVEAU_BO_LOW |
				 NOUVEAU_BO_OR, 0, NV34TCL_VTXBUF_ADDRESS_DMA1);
		so_data (vtxfmt, ((vb->stride << NV34TCL_VTXFMT_STRIDE_SHIFT) |
				  (ncomp << NV34TCL_VTXFMT_SIZE_SHIFT) | type));
	}

	if (ib) {
		struct nouveau_bo *bo = nouveau_bo(ib);

		so_method(vtxbuf, rankine, NV34TCL_IDXBUF_ADDRESS, 2);
		so_reloc (vtxbuf, bo, 0, vb_flags | NOUVEAU_BO_LOW, 0, 0);
		so_reloc (vtxbuf, bo, ib_format, vb_flags | NOUVEAU_BO_OR,
				  0, NV34TCL_IDXBUF_FORMAT_DMA1);
	}

	so_method(vtxbuf, rankine, 0x1710, 1);
	so_data  (vtxbuf, 0);

	so_ref(vtxbuf, &nv30->state.hw[NV30_STATE_VTXBUF]);
	so_ref(NULL, &vtxbuf);
	nv30->state.dirty |= (1ULL << NV30_STATE_VTXBUF);
	so_ref(vtxfmt, &nv30->state.hw[NV30_STATE_VTXFMT]);
	so_ref(NULL, &vtxfmt);
	nv30->state.dirty |= (1ULL << NV30_STATE_VTXFMT);
	so_ref(sattr, &nv30->state.hw[NV30_STATE_VTXATTR]);
	so_ref(NULL, &sattr);
	nv30->state.dirty |= (1ULL << NV30_STATE_VTXATTR);
	return FALSE;
}

struct nv30_state_entry nv30_state_vbo = {
	.validate = nv30_vbo_validate,
	.dirty = {
		.pipe = NV30_NEW_ARRAYS,
		.hw = 0,
	}
};
