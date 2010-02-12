#include "pipe/p_shader_tokens.h"
#include "util/u_inlines.h"

#include "util/u_pack_color.h"

#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pipe.h"

#include "nv40_context.h"
#define NV40_SHADER_NO_FUCKEDNESS
#include "nv40_shader.h"

/* Simple, but crappy, swtnl path, hopefully we wont need to hit this very
 * often at all.  Uses "quadro style" vertex submission + a fixed vertex
 * layout to avoid the need to generate a vertex program or vtxfmt.
 */

struct nv40_render_stage {
	struct draw_stage stage;
	struct nv40_context *nv40;
	unsigned prim;
};

static INLINE struct nv40_render_stage *
nv40_render_stage(struct draw_stage *stage)
{
	return (struct nv40_render_stage *)stage;
}

static INLINE void
nv40_render_vertex(struct nv40_context *nv40, const struct vertex_header *v)
{
	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;
	unsigned i;

	for (i = 0; i < nv40->swtnl.nr_attribs; i++) {
		unsigned idx = nv40->swtnl.draw[i];
		unsigned hw = nv40->swtnl.hw[i];

		switch (nv40->swtnl.emit[i]) {
		case EMIT_OMIT:
			break;
		case EMIT_1F:
			BEGIN_RING(chan, curie, NV40TCL_VTX_ATTR_1F(hw), 1);
			OUT_RING  (chan, fui(v->data[idx][0]));
			break;
		case EMIT_2F:
			BEGIN_RING(chan, curie, NV40TCL_VTX_ATTR_2F_X(hw), 2);
			OUT_RING  (chan, fui(v->data[idx][0]));
			OUT_RING  (chan, fui(v->data[idx][1]));
			break;
		case EMIT_3F:
			BEGIN_RING(chan, curie, NV40TCL_VTX_ATTR_3F_X(hw), 3);
			OUT_RING  (chan, fui(v->data[idx][0]));
			OUT_RING  (chan, fui(v->data[idx][1]));
			OUT_RING  (chan, fui(v->data[idx][2]));
			break;
		case EMIT_4F:
			BEGIN_RING(chan, curie, NV40TCL_VTX_ATTR_4F_X(hw), 4);
			OUT_RING  (chan, fui(v->data[idx][0]));
			OUT_RING  (chan, fui(v->data[idx][1]));
			OUT_RING  (chan, fui(v->data[idx][2]));
			OUT_RING  (chan, fui(v->data[idx][3]));
			break;
		case EMIT_4UB:
			BEGIN_RING(chan, curie, NV40TCL_VTX_ATTR_4UB(hw), 1);
			OUT_RING  (chan, pack_ub4(float_to_ubyte(v->data[idx][0]),
					    float_to_ubyte(v->data[idx][1]),
					    float_to_ubyte(v->data[idx][2]),
					    float_to_ubyte(v->data[idx][3])));
			break;
		default:
			assert(0);
			break;
		}
	}
}

static INLINE void
nv40_render_prim(struct draw_stage *stage, struct prim_header *prim,
	       unsigned mode, unsigned count)
{
	struct nv40_render_stage *rs = nv40_render_stage(stage);
	struct nv40_context *nv40 = rs->nv40;

	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;
	unsigned i;

	/* Ensure there's room for 4xfloat32 + potentially 3 begin/end */
	if (AVAIL_RING(chan) < ((count * 20) + 6)) {
		if (rs->prim != NV40TCL_BEGIN_END_STOP) {
			NOUVEAU_ERR("AIII, missed flush\n");
			assert(0);
		}
		FIRE_RING(chan);
		nv40_state_emit(nv40);
	}

	/* Switch primitive modes if necessary */
	if (rs->prim != mode) {
		if (rs->prim != NV40TCL_BEGIN_END_STOP) {
			BEGIN_RING(chan, curie, NV40TCL_BEGIN_END, 1);
			OUT_RING  (chan, NV40TCL_BEGIN_END_STOP);
		}

		BEGIN_RING(chan, curie, NV40TCL_BEGIN_END, 1);
		OUT_RING  (chan, mode);
		rs->prim = mode;
	}

	/* Emit vertex data */
	for (i = 0; i < count; i++)
		nv40_render_vertex(nv40, prim->v[i]);

	/* If it's likely we'll need to empty the push buffer soon, finish
	 * off the primitive now.
	 */
	if (AVAIL_RING(chan) < ((count * 20) + 6)) {
		BEGIN_RING(chan, curie, NV40TCL_BEGIN_END, 1);
		OUT_RING  (chan, NV40TCL_BEGIN_END_STOP);
		rs->prim = NV40TCL_BEGIN_END_STOP;
	}
}

static void
nv40_render_point(struct draw_stage *draw, struct prim_header *prim)
{
	nv40_render_prim(draw, prim, NV40TCL_BEGIN_END_POINTS, 1);
}

static void
nv40_render_line(struct draw_stage *draw, struct prim_header *prim)
{
	nv40_render_prim(draw, prim, NV40TCL_BEGIN_END_LINES, 2);
}

static void
nv40_render_tri(struct draw_stage *draw, struct prim_header *prim)
{
	nv40_render_prim(draw, prim, NV40TCL_BEGIN_END_TRIANGLES, 3);
}

static void
nv40_render_flush(struct draw_stage *draw, unsigned flags)
{
	struct nv40_render_stage *rs = nv40_render_stage(draw);
	struct nv40_context *nv40 = rs->nv40;
	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;

	if (rs->prim != NV40TCL_BEGIN_END_STOP) {
		BEGIN_RING(chan, curie, NV40TCL_BEGIN_END, 1);
		OUT_RING  (chan, NV40TCL_BEGIN_END_STOP);
		rs->prim = NV40TCL_BEGIN_END_STOP;
	}
}

static void
nv40_render_reset_stipple_counter(struct draw_stage *draw)
{
}

static void
nv40_render_destroy(struct draw_stage *draw)
{
	FREE(draw);
}

static INLINE void
emit_mov(struct nv40_vertex_program *vp,
	 unsigned dst, unsigned src, unsigned vor, unsigned mask)
{
	struct nv40_vertex_program_exec *inst;

	vp->insns = realloc(vp->insns,
			    sizeof(struct nv40_vertex_program_exec) *
			    ++vp->nr_insns);
	inst = &vp->insns[vp->nr_insns - 1];

	inst->data[0] = 0x401f9c6c;
	inst->data[1] = 0x0040000d | (src << 8);
	inst->data[2] = 0x8106c083;
	inst->data[3] = 0x6041ff80 | (dst << 2) | (mask << 13);
	inst->const_index = -1;
	inst->has_branch_offset = FALSE;

	vp->ir |= (1 << src);
	if (vor != ~0)
		vp->or |= (1 << vor);
}

static struct nv40_vertex_program *
create_drawvp(struct nv40_context *nv40)
{
	struct nv40_vertex_program *vp = CALLOC_STRUCT(nv40_vertex_program);
	unsigned i;

	emit_mov(vp, NV40_VP_INST_DEST_POS, 0, ~0, 0xf);
	emit_mov(vp, NV40_VP_INST_DEST_COL0, 3, 0, 0xf);
	emit_mov(vp, NV40_VP_INST_DEST_COL1, 4, 1, 0xf);
	emit_mov(vp, NV40_VP_INST_DEST_BFC0, 3, 2, 0xf);
	emit_mov(vp, NV40_VP_INST_DEST_BFC1, 4, 3, 0xf);
	emit_mov(vp, NV40_VP_INST_DEST_FOGC, 5, 4, 0x8);
	for (i = 0; i < 8; i++)
		emit_mov(vp, NV40_VP_INST_DEST_TC(i), 8 + i, 14 + i, 0xf);

	vp->insns[vp->nr_insns - 1].data[3] |= 1;
	vp->translated = TRUE;
	return vp;
}

struct draw_stage *
nv40_draw_render_stage(struct nv40_context *nv40)
{
	struct nv40_render_stage *render = CALLOC_STRUCT(nv40_render_stage);

	if (!nv40->swtnl.vertprog)
		nv40->swtnl.vertprog = create_drawvp(nv40);

	render->nv40 = nv40;
	render->stage.draw = nv40->draw;
	render->stage.point = nv40_render_point;
	render->stage.line = nv40_render_line;
	render->stage.tri = nv40_render_tri;
	render->stage.flush = nv40_render_flush;
	render->stage.reset_stipple_counter = nv40_render_reset_stipple_counter;
	render->stage.destroy = nv40_render_destroy;

	return &render->stage;
}

void
nv40_draw_elements_swtnl(struct pipe_context *pipe,
			 struct pipe_buffer *idxbuf, unsigned idxbuf_size,
			 unsigned mode, unsigned start, unsigned count)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct pipe_screen *pscreen = pipe->screen;
	unsigned i;
	void *map;

	if (!nv40_state_validate_swtnl(nv40))
		return;
	nv40->state.dirty &= ~(1ULL << NV40_STATE_VTXBUF);
	nv40_state_emit(nv40);

	for (i = 0; i < nv40->vtxbuf_nr; i++) {
		map = pipe_buffer_map(pscreen, nv40->vtxbuf[i].buffer,
                                      PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_vertex_buffer(nv40->draw, i, map);
	}

	if (idxbuf) {
		map = pipe_buffer_map(pscreen, idxbuf,
				      PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_element_buffer(nv40->draw, idxbuf_size, map);
	} else {
		draw_set_mapped_element_buffer(nv40->draw, 0, NULL);
	}

	if (nv40->constbuf[PIPE_SHADER_VERTEX]) {
		const unsigned nr = nv40->constbuf_nr[PIPE_SHADER_VERTEX];

		map = pipe_buffer_map(pscreen,
				      nv40->constbuf[PIPE_SHADER_VERTEX],
				      PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_constant_buffer(nv40->draw, PIPE_SHADER_VERTEX, 0,
                                                map, nr);
	}

	draw_arrays(nv40->draw, mode, start, count);

	for (i = 0; i < nv40->vtxbuf_nr; i++)
		pipe_buffer_unmap(pscreen, nv40->vtxbuf[i].buffer);

	if (idxbuf)
		pipe_buffer_unmap(pscreen, idxbuf);

	if (nv40->constbuf[PIPE_SHADER_VERTEX])
		pipe_buffer_unmap(pscreen, nv40->constbuf[PIPE_SHADER_VERTEX]);

	draw_flush(nv40->draw);
	pipe->flush(pipe, 0, NULL);
}

static INLINE void
emit_attrib(struct nv40_context *nv40, unsigned hw, unsigned emit,
	    unsigned semantic, unsigned index)
{
	unsigned draw_out = draw_find_shader_output(nv40->draw, semantic, index);
	unsigned a = nv40->swtnl.nr_attribs++;

	nv40->swtnl.hw[a] = hw;
	nv40->swtnl.emit[a] = emit;
	nv40->swtnl.draw[a] = draw_out;
}

static boolean
nv40_state_vtxfmt_validate(struct nv40_context *nv40)
{
	struct nv40_fragment_program *fp = nv40->fragprog;
	unsigned colour = 0, texcoords = 0, fog = 0, i;

	/* Determine needed fragprog inputs */
	for (i = 0; i < fp->info.num_inputs; i++) {
		switch (fp->info.input_semantic_name[i]) {
		case TGSI_SEMANTIC_POSITION:
			break;
		case TGSI_SEMANTIC_COLOR:
			colour |= (1 << fp->info.input_semantic_index[i]);
			break;
		case TGSI_SEMANTIC_GENERIC:
			texcoords |= (1 << fp->info.input_semantic_index[i]);
			break;
		case TGSI_SEMANTIC_FOG:
			fog = 1;
			break;
		default:
			assert(0);
		}
	}

	nv40->swtnl.nr_attribs = 0;

	/* Map draw vtxprog output to hw attribute IDs */
	for (i = 0; i < 2; i++) {
		if (!(colour & (1 << i)))
			continue;
		emit_attrib(nv40, 3 + i, EMIT_4UB, TGSI_SEMANTIC_COLOR, i);
	}

	for (i = 0; i < 8; i++) {
		if (!(texcoords & (1 << i)))
			continue;
		emit_attrib(nv40, 8 + i, EMIT_4F, TGSI_SEMANTIC_GENERIC, i);
	}

	if (fog) {
		emit_attrib(nv40, 5, EMIT_1F, TGSI_SEMANTIC_FOG, 0);
	}

	emit_attrib(nv40, 0, EMIT_3F, TGSI_SEMANTIC_POSITION, 0);

	return FALSE;
}

struct nv40_state_entry nv40_state_vtxfmt = {
	.validate = nv40_state_vtxfmt_validate,
	.dirty = {
		.pipe = NV40_NEW_ARRAYS | NV40_NEW_FRAGPROG,
		.hw = 0
	}
};

