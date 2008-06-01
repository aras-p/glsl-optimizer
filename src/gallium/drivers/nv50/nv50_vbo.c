#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_state.h"

static INLINE unsigned
nv50_prim(unsigned mode)
{
	switch (mode) {
	case PIPE_PRIM_POINTS: return NV50TCL_VERTEX_BEGIN_POINTS;
	case PIPE_PRIM_LINES: return NV50TCL_VERTEX_BEGIN_LINES;
	case PIPE_PRIM_LINE_LOOP: return NV50TCL_VERTEX_BEGIN_LINE_LOOP;
	case PIPE_PRIM_LINE_STRIP: return NV50TCL_VERTEX_BEGIN_LINE_STRIP;
	case PIPE_PRIM_TRIANGLES: return NV50TCL_VERTEX_BEGIN_TRIANGLES;
	case PIPE_PRIM_TRIANGLE_STRIP:
		return NV50TCL_VERTEX_BEGIN_TRIANGLE_STRIP;
	case PIPE_PRIM_TRIANGLE_FAN: return NV50TCL_VERTEX_BEGIN_TRIANGLE_FAN;
	case PIPE_PRIM_QUADS: return NV50TCL_VERTEX_BEGIN_QUADS;
	case PIPE_PRIM_QUAD_STRIP: return NV50TCL_VERTEX_BEGIN_QUAD_STRIP;
	case PIPE_PRIM_POLYGON: return NV50TCL_VERTEX_BEGIN_POLYGON;
	default:
		break;
	}

	NOUVEAU_ERR("invalid primitive type %d\n", mode);
	return NV50TCL_VERTEX_BEGIN_POINTS;
}

boolean
nv50_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50_state_validate(nv50);
	NOUVEAU_ERR("unimplemented\n");

	BEGIN_RING(tesla, 0x142c, 1);
	OUT_RING  (0);
	BEGIN_RING(tesla, 0x142c, 1);
	OUT_RING  (0);

	BEGIN_RING(tesla, NV50TCL_VERTEX_BEGIN, 1);
	OUT_RING  (nv50_prim(mode));
	BEGIN_RING(tesla, NV50TCL_VERTEX_BUFFER_FIRST, 2);
	OUT_RING  (start);
	OUT_RING  (count);
	BEGIN_RING(tesla, NV50TCL_VERTEX_END, 1);
	OUT_RING  (0);

	pipe->flush(pipe, 0, NULL);
	return TRUE;
}

boolean
nv50_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50_state_validate(nv50);

	NOUVEAU_ERR("unimplemented\n");
	return TRUE;
}

void
nv50_vbo_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *vtxbuf, *vtxfmt;
	int i, vpi = 0;

	vtxbuf = so_new(nv50->vtxelt_nr * 4, nv50->vtxelt_nr * 2);
	vtxfmt = so_new(nv50->vtxelt_nr + 1, 0);
	so_method(vtxfmt, tesla, 0x1ac0, nv50->vtxelt_nr);

	for (i = 0; i < nv50->vtxelt_nr; i++) {
		struct pipe_vertex_element *ve = &nv50->vtxelt[i];
		struct pipe_vertex_buffer *vb =
			&nv50->vtxbuf[ve->vertex_buffer_index];

		switch (ve->src_format) {
		case PIPE_FORMAT_R32G32B32_FLOAT:
			so_data(vtxfmt, 0x7e100000 | i);
			break;
		default:
		{
			char fmt[128];
			pf_sprint_name(fmt, ve->src_format);
			NOUVEAU_ERR("invalid vbo format %s\n", fmt);
			assert(0);
			return;
		}
		}

		so_method(vtxbuf, tesla, 0x900 + (i * 16), 3);
		so_data  (vtxbuf, 0x20000000 | vb->pitch);
		so_reloc (vtxbuf, vb->buffer, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
			  NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, vb->buffer, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
			  NOUVEAU_BO_LOW, 0, 0);
	}

	so_emit(nv50->screen->nvws, vtxfmt);
	so_ref (NULL, &vtxfmt);
	so_emit(nv50->screen->nvws, vtxbuf);
	so_ref (NULL, &vtxbuf);
}

