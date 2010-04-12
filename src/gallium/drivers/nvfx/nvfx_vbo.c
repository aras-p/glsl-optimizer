#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "nvfx_context.h"
#include "nvfx_state.h"
#include "nvfx_resource.h"

#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_pushbuf.h"
#include "nouveau/nouveau_util.h"

static INLINE int
nvfx_vbo_format_to_hw(enum pipe_format pipe, unsigned *fmt, unsigned *ncomp)
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
nvfx_vbo_set_idxbuf(struct nvfx_context *nvfx, struct pipe_resource *ib,
		    unsigned ib_size)
{
	struct pipe_screen *pscreen = &nvfx->screen->base.base;
	unsigned type;

	if (!ib) {
		nvfx->idxbuf = NULL;
		nvfx->idxbuf_format = 0xdeadbeef;
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

	if (ib != nvfx->idxbuf ||
	    type != nvfx->idxbuf_format) {
		nvfx->dirty |= NVFX_NEW_ARRAYS;
		nvfx->idxbuf = ib;
		nvfx->idxbuf_format = type;
	}

	return TRUE;
}

// type must be floating point
static inline void
nvfx_vbo_static_attrib(struct nvfx_context *nvfx,
		       int attrib, struct pipe_vertex_element *ve,
		       struct pipe_vertex_buffer *vb, unsigned ncomp)
{
	struct pipe_transfer *transfer;
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	void *map;

	map  = pipe_buffer_map(&nvfx->pipe, vb->buffer, PIPE_TRANSFER_READ, &transfer);
	map += vb->buffer_offset + ve->src_offset;

	float *v = map;

	switch (ncomp) {
	case 4:
		OUT_RING(chan, RING_3D(NV34TCL_VTX_ATTR_4F_X(attrib), 4));
		OUT_RING(chan, fui(v[0]));
		OUT_RING(chan, fui(v[1]));
		OUT_RING(chan,  fui(v[2]));
		OUT_RING(chan,  fui(v[3]));
		break;
	case 3:
		OUT_RING(chan, RING_3D(NV34TCL_VTX_ATTR_3F_X(attrib), 3));
		OUT_RING(chan,  fui(v[0]));
		OUT_RING(chan,  fui(v[1]));
		OUT_RING(chan,  fui(v[2]));
		break;
	case 2:
		OUT_RING(chan, RING_3D(NV34TCL_VTX_ATTR_2F_X(attrib), 2));
		OUT_RING(chan,  fui(v[0]));
		OUT_RING(chan,  fui(v[1]));
		break;
	case 1:
		OUT_RING(chan, RING_3D(NV34TCL_VTX_ATTR_1F(attrib), 1));
		OUT_RING(chan,  fui(v[0]));
		break;
	}

	pipe_buffer_unmap(&nvfx->pipe, vb->buffer, transfer);
}

void
nvfx_draw_arrays(struct pipe_context *pipe,
		 unsigned mode, unsigned start, unsigned count)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	unsigned restart = 0;

	nvfx_vbo_set_idxbuf(nvfx, NULL, 0);
	if (nvfx->screen->force_swtnl || !nvfx_state_validate(nvfx)) {
		nvfx_draw_elements_swtnl(pipe, NULL, 0,
                                           mode, start, count);
                return;
	}

	while (count) {
		unsigned vc, nr;

		nvfx_state_emit(nvfx);

		unsigned avail = AVAIL_RING(chan);
		avail -= 16 + (avail >> 10); /* for the BEGIN_RING_NIs, conservatively assuming one every 1024, plus 16 for safety */

		vc = nouveau_vbuf_split(avail, 6, 256,
					mode, start, count, &restart);
		if (!vc) {
			FIRE_RING(chan);
			continue;
		}

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, nvgl_primitive(mode));

		nr = (vc & 0xff);
		if (nr) {
			OUT_RING(chan, RING_3D(NV34TCL_VB_VERTEX_BATCH, 1));
			OUT_RING  (chan, ((nr - 1) << 24) | start);
			start += nr;
		}

		nr = vc >> 8;
		while (nr) {
			unsigned push = nr > 2047 ? 2047 : nr;

			nr -= push;

			OUT_RING(chan, RING_3D_NI(NV34TCL_VB_VERTEX_BATCH, push));
			while (push--) {
				OUT_RING(chan, ((0x100 - 1) << 24) | start);
				start += 0x100;
			}
		}

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, 0);

		count -= vc;
		start = restart;
	}

	pipe->flush(pipe, 0, NULL);
}

static INLINE void
nvfx_draw_elements_u08(struct nvfx_context *nvfx, void *ib,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;

	while (count) {
		uint8_t *elts = (uint8_t *)ib + start;
		unsigned vc, push, restart = 0;

		nvfx_state_emit(nvfx);

		unsigned avail = AVAIL_RING(chan);
		avail -= 16 + (avail >> 10); /* for the BEGIN_RING_NIs, conservatively assuming one every 1024, plus 16 for safety */

		vc = nouveau_vbuf_split(avail, 6, 2,
					mode, start, count, &restart);
		if (vc == 0) {
			FIRE_RING(chan);
			continue;
		}
		count -= vc;

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, nvgl_primitive(mode));

		if (vc & 1) {
			OUT_RING(chan, RING_3D(NV34TCL_VB_ELEMENT_U32, 1));
			OUT_RING  (chan, elts[0]);
			elts++; vc--;
		}

		while (vc) {
			unsigned i;

			push = MIN2(vc, 2047 * 2);

			OUT_RING(chan, RING_3D_NI(NV34TCL_VB_ELEMENT_U16, push >> 1));
			for (i = 0; i < push; i+=2)
				OUT_RING(chan, (elts[i+1] << 16) | elts[i]);

			vc -= push;
			elts += push;
		}

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, 0);

		start = restart;
	}
}

static INLINE void
nvfx_draw_elements_u16(struct nvfx_context *nvfx, void *ib,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;

	while (count) {
		uint16_t *elts = (uint16_t *)ib + start;
		unsigned vc, push, restart = 0;

		nvfx_state_emit(nvfx);

		unsigned avail = AVAIL_RING(chan);
		avail -= 16 + (avail >> 10); /* for the BEGIN_RING_NIs, conservatively assuming one every 1024, plus 16 for safety */

		vc = nouveau_vbuf_split(avail, 6, 2,
					mode, start, count, &restart);
		if (vc == 0) {
			FIRE_RING(chan);
			continue;
		}
		count -= vc;

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, nvgl_primitive(mode));

		if (vc & 1) {
			OUT_RING(chan, RING_3D(NV34TCL_VB_ELEMENT_U32, 1));
			OUT_RING  (chan, elts[0]);
			elts++; vc--;
		}

		while (vc) {
			unsigned i;

			push = MIN2(vc, 2047 * 2);

			OUT_RING(chan, RING_3D_NI(NV34TCL_VB_ELEMENT_U16, push >> 1));
			for (i = 0; i < push; i+=2)
				OUT_RING(chan, (elts[i+1] << 16) | elts[i]);

			vc -= push;
			elts += push;
		}

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, 0);

		start = restart;
	}
}

static INLINE void
nvfx_draw_elements_u32(struct nvfx_context *nvfx, void *ib,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;

	while (count) {
		uint32_t *elts = (uint32_t *)ib + start;
		unsigned vc, push, restart = 0;

		nvfx_state_emit(nvfx);

		unsigned avail = AVAIL_RING(chan);
		avail -= 16 + (avail >> 10); /* for the BEGIN_RING_NIs, conservatively assuming one every 1024, plus 16 for safety */

		vc = nouveau_vbuf_split(avail, 5, 1,
					mode, start, count, &restart);
		if (vc == 0) {
			FIRE_RING(chan);
			continue;
		}
		count -= vc;

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, nvgl_primitive(mode));

		while (vc) {
			push = MIN2(vc, 2047);

			OUT_RING(chan, RING_3D_NI(NV34TCL_VB_ELEMENT_U32, push));
			OUT_RINGp    (chan, elts, push);

			vc -= push;
			elts += push;
		}

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, 0);

		start = restart;
	}
}

static void
nvfx_draw_elements_inline(struct pipe_context *pipe,
			  struct pipe_resource *ib, unsigned ib_size,
			  unsigned mode, unsigned start, unsigned count)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct pipe_transfer *transfer;
	void *map;

	map = pipe_buffer_map(pipe, ib, PIPE_TRANSFER_READ, &transfer);
	if (!ib) {
		NOUVEAU_ERR("failed mapping ib\n");
		return;
	}

	switch (ib_size) {
	case 1:
		nvfx_draw_elements_u08(nvfx, map, mode, start, count);
		break;
	case 2:
		nvfx_draw_elements_u16(nvfx, map, mode, start, count);
		break;
	case 4:
		nvfx_draw_elements_u32(nvfx, map, mode, start, count);
		break;
	default:
		NOUVEAU_ERR("invalid idxbuf fmt %d\n", ib_size);
		break;
	}

	pipe_buffer_unmap(pipe, ib, transfer);
}

static void
nvfx_draw_elements_vbo(struct pipe_context *pipe,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	unsigned restart = 0;

	while (count) {
		unsigned nr, vc;

		nvfx_state_emit(nvfx);

		unsigned avail = AVAIL_RING(chan);
		avail -= 16 + (avail >> 10); /* for the BEGIN_RING_NIs, conservatively assuming one every 1024, plus 16 for safety */

		vc = nouveau_vbuf_split(avail, 6, 256,
					mode, start, count, &restart);
		if (!vc) {
			FIRE_RING(chan);
			continue;
		}

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, nvgl_primitive(mode));

		nr = (vc & 0xff);
		if (nr) {
			OUT_RING(chan, RING_3D(NV34TCL_VB_INDEX_BATCH, 1));
			OUT_RING  (chan, ((nr - 1) << 24) | start);
			start += nr;
		}

		nr = vc >> 8;
		while (nr) {
			unsigned push = nr > 2047 ? 2047 : nr;

			nr -= push;

			OUT_RING(chan, RING_3D_NI(NV34TCL_VB_INDEX_BATCH, push));
			while (push--) {
				OUT_RING(chan, ((0x100 - 1) << 24) | start);
				start += 0x100;
			}
		}

		OUT_RING(chan, RING_3D(NV34TCL_VERTEX_BEGIN_END, 1));
		OUT_RING  (chan, 0);

		count -= vc;
		start = restart;
	}
}

void
nvfx_draw_elements(struct pipe_context *pipe,
		   struct pipe_resource *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	boolean idxbuf;

	idxbuf = nvfx_vbo_set_idxbuf(nvfx, indexBuffer, indexSize);
	if (nvfx->screen->force_swtnl || !nvfx_state_validate(nvfx)) {
		nvfx_draw_elements_swtnl(pipe, indexBuffer, indexSize,
                                           mode, start, count);
		return;
	}

	if (idxbuf) {
		nvfx_draw_elements_vbo(pipe, mode, start, count);
	} else {
		nvfx_draw_elements_inline(pipe, indexBuffer, indexSize,
					  mode, start, count);
	}

	pipe->flush(pipe, 0, NULL);
}

boolean
nvfx_vbo_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct pipe_resource *ib = nvfx->idxbuf;
	unsigned ib_format = nvfx->idxbuf_format;
	int i;
	int elements = MAX2(nvfx->vtxelt->num_elements, nvfx->hw_vtxelt_nr);
	unsigned long vtxfmt[16];
	unsigned vb_flags = nvfx->screen->vertex_buffer_flags | NOUVEAU_BO_RD;

	if (!elements)
		return TRUE;

	nvfx->vbo_bo = 0;

	MARK_RING(chan, (5 + 2) * 16 + 2 + 11, 16 + 2);
	for (i = 0; i < nvfx->vtxelt->num_elements; i++) {
		struct pipe_vertex_element *ve;
		struct pipe_vertex_buffer *vb;
		unsigned type, ncomp;

		ve = &nvfx->vtxelt->pipe[i];
		vb = &nvfx->vtxbuf[ve->vertex_buffer_index];

		if (nvfx_vbo_format_to_hw(ve->src_format, &type, &ncomp)) {
			MARK_UNDO(chan);
			nvfx->fallback_swtnl |= NVFX_NEW_ARRAYS;
			return FALSE;
		}

		if (!vb->stride && type == NV34TCL_VTXFMT_TYPE_FLOAT) {
			nvfx_vbo_static_attrib(nvfx, i, ve, vb, ncomp);
			vtxfmt[i] = type;
		} else {
			vtxfmt[i] = ((vb->stride << NV34TCL_VTXFMT_STRIDE_SHIFT) |
				(ncomp << NV34TCL_VTXFMT_SIZE_SHIFT) | type);
			nvfx->vbo_bo |= (1 << i);
		}
	}

	for(; i < elements; ++i)
		vtxfmt[i] = NV34TCL_VTXFMT_TYPE_FLOAT;

	OUT_RING(chan, RING_3D(NV34TCL_VTXFMT(0), elements));
	OUT_RINGp(chan, vtxfmt, elements);

	if(nvfx->is_nv4x) {
		unsigned i;
		/* seems to be some kind of cache flushing */
		for(i = 0; i < 3; ++i) {
			OUT_RING(chan, RING_3D(0x1718, 1));
			OUT_RING(chan, 0);
		}
	}

	OUT_RING(chan, RING_3D(NV34TCL_VTXBUF_ADDRESS(0), elements));
	for (i = 0; i < nvfx->vtxelt->num_elements; i++) {
		struct pipe_vertex_element *ve;
		struct pipe_vertex_buffer *vb;

		ve = &nvfx->vtxelt->pipe[i];
		vb = &nvfx->vtxbuf[ve->vertex_buffer_index];

		if (!(nvfx->vbo_bo & (1 << i)))
			OUT_RING(chan, 0);
		else
		{
			struct nouveau_bo* bo = nvfx_resource(vb->buffer)->bo;
			OUT_RELOC(chan, bo,
				 vb->buffer_offset + ve->src_offset,
				 vb_flags | NOUVEAU_BO_LOW | NOUVEAU_BO_OR,
				 0, NV34TCL_VTXBUF_ADDRESS_DMA1);
		}
	}

        for (; i < elements; i++)
		OUT_RING(chan, 0);

	OUT_RING(chan, RING_3D(0x1710, 1));
	OUT_RING(chan, 0);

	if (ib) {
		struct nouveau_bo* bo = nvfx_resource(ib)->bo;

		OUT_RING(chan, RING_3D(NV34TCL_IDXBUF_ADDRESS, 2));
		OUT_RELOC(chan, bo, 0, vb_flags | NOUVEAU_BO_LOW, 0, 0);
		OUT_RELOC(chan, bo, ib_format, vb_flags | NOUVEAU_BO_OR,
				  0, NV34TCL_IDXBUF_FORMAT_DMA1);
	}

	nvfx->hw_vtxelt_nr = nvfx->vtxelt->num_elements;
	return TRUE;
}

void
nvfx_vbo_relocate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	unsigned vb_flags = nvfx->screen->vertex_buffer_flags | NOUVEAU_BO_RD | NOUVEAU_BO_DUMMY;
	int i;

	MARK_RING(chan, 2 * 16 + 3, 2 * 16 + 3);
	for(i = 0; i < nvfx->vtxelt->num_elements; ++i) {
		if(nvfx->vbo_bo & (1 << i)) {
			struct pipe_vertex_element *ve = &nvfx->vtxelt->pipe[i];
			struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[ve->vertex_buffer_index];
			struct nouveau_bo* bo = nvfx_resource(vb->buffer)->bo;
			OUT_RELOC(chan, bo, RING_3D(NV34TCL_VTXBUF_ADDRESS(i), 1),
					vb_flags, 0, 0);
			OUT_RELOC(chan, bo, vb->buffer_offset + ve->src_offset,
					vb_flags | NOUVEAU_BO_LOW | NOUVEAU_BO_OR,
					0, NV34TCL_VTXBUF_ADDRESS_DMA1);
		}
	}

	if(nvfx->idxbuf)
	{
		struct nouveau_bo* bo = nvfx_resource(nvfx->idxbuf)->bo;

		OUT_RELOC(chan, bo, RING_3D(NV34TCL_IDXBUF_ADDRESS, 2),
				vb_flags, 0, 0);
		OUT_RELOC(chan, bo, 0,
				vb_flags | NOUVEAU_BO_LOW, 0, 0);
		OUT_RELOC(chan, bo, nvfx->idxbuf_format,
				vb_flags | NOUVEAU_BO_OR,
				0, NV34TCL_IDXBUF_FORMAT_DMA1);
	}
}
