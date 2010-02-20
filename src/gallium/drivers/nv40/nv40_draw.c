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
	struct nvfx_context *nvfx;
	unsigned prim;
};

static INLINE struct nv40_render_stage *
nv40_render_stage(struct draw_stage *stage)
{
	return (struct nv40_render_stage *)stage;
}

static INLINE void
nv40_render_vertex(struct nvfx_context *nvfx, const struct vertex_header *v)
{
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;
	unsigned i;

	for (i = 0; i < nvfx->swtnl.nr_attribs; i++) {
		unsigned idx = nvfx->swtnl.draw[i];
		unsigned hw = nvfx->swtnl.hw[i];

		switch (nvfx->swtnl.emit[i]) {
		case EMIT_OMIT:
			break;
		case EMIT_1F:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_1F(hw), 1);
			OUT_RING  (chan, fui(v->data[idx][0]));
			break;
		case EMIT_2F:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_2F_X(hw), 2);
			OUT_RING  (chan, fui(v->data[idx][0]));
			OUT_RING  (chan, fui(v->data[idx][1]));
			break;
		case EMIT_3F:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_3F_X(hw), 3);
			OUT_RING  (chan, fui(v->data[idx][0]));
			OUT_RING  (chan, fui(v->data[idx][1]));
			OUT_RING  (chan, fui(v->data[idx][2]));
			break;
		case EMIT_4F:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_4F_X(hw), 4);
			OUT_RING  (chan, fui(v->data[idx][0]));
			OUT_RING  (chan, fui(v->data[idx][1]));
			OUT_RING  (chan, fui(v->data[idx][2]));
			OUT_RING  (chan, fui(v->data[idx][3]));
			break;
		case EMIT_4UB:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_4UB(hw), 1);
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
	struct nvfx_context *nvfx = rs->nvfx;

	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;
	unsigned i;

	/* Ensure there's room for 4xfloat32 + potentially 3 begin/end */
	if (AVAIL_RING(chan) < ((count * 20) + 6)) {
		if (rs->prim != NV34TCL_VERTEX_BEGIN_END_STOP) {
			NOUVEAU_ERR("AIII, missed flush\n");
			assert(0);
		}
		FIRE_RING(chan);
		nv40_state_emit(nvfx);
	}

	/* Switch primitive modes if necessary */
	if (rs->prim != mode) {
		if (rs->prim != NV34TCL_VERTEX_BEGIN_END_STOP) {
			BEGIN_RING(chan, eng3d, NV34TCL_VERTEX_BEGIN_END, 1);
			OUT_RING  (chan, NV34TCL_VERTEX_BEGIN_END_STOP);
		}

		BEGIN_RING(chan, eng3d, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, mode);
		rs->prim = mode;
	}

	/* Emit vertex data */
	for (i = 0; i < count; i++)
		nv40_render_vertex(nvfx, prim->v[i]);

	/* If it's likely we'll need to empty the push buffer soon, finish
	 * off the primitive now.
	 */
	if (AVAIL_RING(chan) < ((count * 20) + 6)) {
		BEGIN_RING(chan, eng3d, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, NV34TCL_VERTEX_BEGIN_END_STOP);
		rs->prim = NV34TCL_VERTEX_BEGIN_END_STOP;
	}
}

static void
nv40_render_point(struct draw_stage *draw, struct prim_header *prim)
{
	nv40_render_prim(draw, prim, NV34TCL_VERTEX_BEGIN_END_POINTS, 1);
}

static void
nv40_render_line(struct draw_stage *draw, struct prim_header *prim)
{
	nv40_render_prim(draw, prim, NV34TCL_VERTEX_BEGIN_END_LINES, 2);
}

static void
nv40_render_tri(struct draw_stage *draw, struct prim_header *prim)
{
	nv40_render_prim(draw, prim, NV34TCL_VERTEX_BEGIN_END_TRIANGLES, 3);
}

static void
nv40_render_flush(struct draw_stage *draw, unsigned flags)
{
	struct nv40_render_stage *rs = nv40_render_stage(draw);
	struct nvfx_context *nvfx = rs->nvfx;
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;

	if (rs->prim != NV34TCL_VERTEX_BEGIN_END_STOP) {
		BEGIN_RING(chan, eng3d, NV34TCL_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, NV34TCL_VERTEX_BEGIN_END_STOP);
		rs->prim = NV34TCL_VERTEX_BEGIN_END_STOP;
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
emit_mov(struct nvfx_vertex_program *vp,
	 unsigned dst, unsigned src, unsigned vor, unsigned mask)
{
	struct nvfx_vertex_program_exec *inst;

	vp->insns = realloc(vp->insns,
			    sizeof(struct nvfx_vertex_program_exec) *
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

static struct nvfx_vertex_program *
create_drawvp(struct nvfx_context *nvfx)
{
	struct nvfx_vertex_program *vp = CALLOC_STRUCT(nvfx_vertex_program);
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
nv40_draw_render_stage(struct nvfx_context *nvfx)
{
	struct nv40_render_stage *render = CALLOC_STRUCT(nv40_render_stage);

	if (!nvfx->swtnl.vertprog)
		nvfx->swtnl.vertprog = create_drawvp(nvfx);

	render->nvfx = nvfx;
	render->stage.draw = nvfx->draw;
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
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct pipe_screen *pscreen = pipe->screen;
	unsigned i;
	void *map;

	if (!nv40_state_validate_swtnl(nvfx))
		return;
	nvfx->state.dirty &= ~(1ULL << NVFX_STATE_VTXBUF);
	nv40_state_emit(nvfx);

	for (i = 0; i < nvfx->vtxbuf_nr; i++) {
		map = pipe_buffer_map(pscreen, nvfx->vtxbuf[i].buffer,
                                      PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_vertex_buffer(nvfx->draw, i, map);
	}

	if (idxbuf) {
		map = pipe_buffer_map(pscreen, idxbuf,
				      PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_element_buffer(nvfx->draw, idxbuf_size, map);
	} else {
		draw_set_mapped_element_buffer(nvfx->draw, 0, NULL);
	}

	if (nvfx->constbuf[PIPE_SHADER_VERTEX]) {
		const unsigned nr = nvfx->constbuf_nr[PIPE_SHADER_VERTEX];

		map = pipe_buffer_map(pscreen,
				      nvfx->constbuf[PIPE_SHADER_VERTEX],
				      PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_constant_buffer(nvfx->draw, PIPE_SHADER_VERTEX, 0,
                                                map, nr);
	}

	draw_arrays(nvfx->draw, mode, start, count);

	for (i = 0; i < nvfx->vtxbuf_nr; i++)
		pipe_buffer_unmap(pscreen, nvfx->vtxbuf[i].buffer);

	if (idxbuf)
		pipe_buffer_unmap(pscreen, idxbuf);

	if (nvfx->constbuf[PIPE_SHADER_VERTEX])
		pipe_buffer_unmap(pscreen, nvfx->constbuf[PIPE_SHADER_VERTEX]);

	draw_flush(nvfx->draw);
	pipe->flush(pipe, 0, NULL);
}

static INLINE void
emit_attrib(struct nvfx_context *nvfx, unsigned hw, unsigned emit,
	    unsigned semantic, unsigned index)
{
	unsigned draw_out = draw_find_shader_output(nvfx->draw, semantic, index);
	unsigned a = nvfx->swtnl.nr_attribs++;

	nvfx->swtnl.hw[a] = hw;
	nvfx->swtnl.emit[a] = emit;
	nvfx->swtnl.draw[a] = draw_out;
}

static boolean
nv40_state_vtxfmt_validate(struct nvfx_context *nvfx)
{
	struct nvfx_fragment_program *fp = nvfx->fragprog;
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

	nvfx->swtnl.nr_attribs = 0;

	/* Map draw vtxprog output to hw attribute IDs */
	for (i = 0; i < 2; i++) {
		if (!(colour & (1 << i)))
			continue;
		emit_attrib(nvfx, 3 + i, EMIT_4UB, TGSI_SEMANTIC_COLOR, i);
	}

	for (i = 0; i < 8; i++) {
		if (!(texcoords & (1 << i)))
			continue;
		emit_attrib(nvfx, 8 + i, EMIT_4F, TGSI_SEMANTIC_GENERIC, i);
	}

	if (fog) {
		emit_attrib(nvfx, 5, EMIT_1F, TGSI_SEMANTIC_FOG, 0);
	}

	emit_attrib(nvfx, 0, EMIT_3F, TGSI_SEMANTIC_POSITION, 0);

	return FALSE;
}

struct nvfx_state_entry nv40_state_vtxfmt = {
	.validate = nv40_state_vtxfmt_validate,
	.dirty = {
		.pipe = NVFX_NEW_ARRAYS | NVFX_NEW_FRAGPROG,
		.hw = 0
	}
};

