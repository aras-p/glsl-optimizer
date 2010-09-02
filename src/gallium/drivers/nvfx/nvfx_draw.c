#include "pipe/p_shader_tokens.h"
#include "util/u_inlines.h"
#include "tgsi/tgsi_ureg.h"

#include "util/u_pack_color.h"

#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pipe.h"

#include "nvfx_context.h"
#include "nvfx_resource.h"

/* Simple, but crappy, swtnl path, hopefully we wont need to hit this very
 * often at all.  Uses "quadro style" vertex submission + a fixed vertex
 * layout to avoid the need to generate a vertex program or vtxfmt.
 */

struct nvfx_render_stage {
	struct draw_stage stage;
	struct nvfx_context *nvfx;
	unsigned prim;
};

static INLINE struct nvfx_render_stage *
nvfx_render_stage(struct draw_stage *stage)
{
	return (struct nvfx_render_stage *)stage;
}

static INLINE void
nvfx_render_vertex(struct nvfx_context *nvfx, const struct vertex_header *v)
{
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;
	unsigned i;

	for (i = 0; i < nvfx->swtnl.nr_attribs; i++) {
		unsigned idx = nvfx->swtnl.draw[i];
		unsigned hw = nvfx->swtnl.hw[i];

		WAIT_RING(chan, 5);
		switch (nvfx->swtnl.emit[i]) {
		case EMIT_OMIT:
			break;
		case EMIT_1F:
			nvfx_emit_vtx_attr(chan, hw, v->data[idx], 1);
			break;
		case EMIT_2F:
			nvfx_emit_vtx_attr(chan, hw, v->data[idx], 2);
			break;
		case EMIT_3F:
			nvfx_emit_vtx_attr(chan, hw, v->data[idx], 3);
			break;
		case EMIT_4F:
			nvfx_emit_vtx_attr(chan, hw, v->data[idx], 4);
			break;
		case 0xff:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_4F_X(hw), 4);
			OUT_RING  (chan, fui(v->data[idx][0] / v->data[idx][3]));
			OUT_RING  (chan, fui(v->data[idx][1] / v->data[idx][3]));
			OUT_RING  (chan, fui(v->data[idx][2] / v->data[idx][3]));
			OUT_RING  (chan, fui(1.0f / v->data[idx][3]));
			break;
		case EMIT_4UB:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_4UB(hw), 1);
			OUT_RING  (chan, pack_ub4(float_to_ubyte(v->data[idx][0]),
					    float_to_ubyte(v->data[idx][1]),
					    float_to_ubyte(v->data[idx][2]),
					    float_to_ubyte(v->data[idx][3])));
		case EMIT_4UB_BGRA:
			BEGIN_RING(chan, eng3d, NV34TCL_VTX_ATTR_4UB(hw), 1);
			OUT_RING  (chan, pack_ub4(float_to_ubyte(v->data[idx][2]),
					    float_to_ubyte(v->data[idx][1]),
					    float_to_ubyte(v->data[idx][0]),
					    float_to_ubyte(v->data[idx][3])));
			break;
		default:
			assert(0);
			break;
		}
	}
}

static INLINE void
nvfx_render_prim(struct draw_stage *stage, struct prim_header *prim,
	       unsigned mode, unsigned count)
{
	struct nvfx_render_stage *rs = nvfx_render_stage(stage);
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
		nvfx_state_emit(nvfx);
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
		nvfx_render_vertex(nvfx, prim->v[i]);

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
nvfx_render_point(struct draw_stage *draw, struct prim_header *prim)
{
	nvfx_render_prim(draw, prim, NV34TCL_VERTEX_BEGIN_END_POINTS, 1);
}

static void
nvfx_render_line(struct draw_stage *draw, struct prim_header *prim)
{
	nvfx_render_prim(draw, prim, NV34TCL_VERTEX_BEGIN_END_LINES, 2);
}

static void
nvfx_render_tri(struct draw_stage *draw, struct prim_header *prim)
{
	nvfx_render_prim(draw, prim, NV34TCL_VERTEX_BEGIN_END_TRIANGLES, 3);
}

static void
nvfx_render_flush(struct draw_stage *draw, unsigned flags)
{
	struct nvfx_render_stage *rs = nvfx_render_stage(draw);
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
nvfx_render_reset_stipple_counter(struct draw_stage *draw)
{
}

static void
nvfx_render_destroy(struct draw_stage *draw)
{
	FREE(draw);
}

static struct nvfx_vertex_program *
nvfx_create_drawvp(struct nvfx_context *nvfx)
{
	struct ureg_program *ureg;
	uint i;

	ureg = ureg_create( TGSI_PROCESSOR_VERTEX );
	if (ureg == NULL)
		return NULL;

	ureg_MOV(ureg, ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0), ureg_DECL_vs_input(ureg, 0));
	ureg_MOV(ureg, ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0), ureg_DECL_vs_input(ureg, 3));
	ureg_MOV(ureg, ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 1), ureg_DECL_vs_input(ureg, 4));
	ureg_MOV(ureg, ureg_DECL_output(ureg, TGSI_SEMANTIC_BCOLOR, 0), ureg_DECL_vs_input(ureg, 3));
	ureg_MOV(ureg, ureg_DECL_output(ureg, TGSI_SEMANTIC_BCOLOR, 1), ureg_DECL_vs_input(ureg, 4));
	ureg_MOV(ureg,
		   ureg_writemask(ureg_DECL_output(ureg, TGSI_SEMANTIC_FOG, 1), TGSI_WRITEMASK_X),
		   ureg_DECL_vs_input(ureg, 5));
	for (i = 0; i < 8; ++i)
		ureg_MOV(ureg, ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, i), ureg_DECL_vs_input(ureg, 8 + i));

	ureg_END( ureg );

	return ureg_create_shader_and_destroy( ureg, &nvfx->pipe );
}

struct draw_stage *
nvfx_draw_render_stage(struct nvfx_context *nvfx)
{
	struct nvfx_render_stage *render = CALLOC_STRUCT(nvfx_render_stage);

	if (!nvfx->swtnl.vertprog)
		nvfx->swtnl.vertprog = nvfx_create_drawvp(nvfx);

	render->nvfx = nvfx;
	render->stage.draw = nvfx->draw;
	render->stage.point = nvfx_render_point;
	render->stage.line = nvfx_render_line;
	render->stage.tri = nvfx_render_tri;
	render->stage.flush = nvfx_render_flush;
	render->stage.reset_stipple_counter = nvfx_render_reset_stipple_counter;
	render->stage.destroy = nvfx_render_destroy;

	return &render->stage;
}

void
nvfx_draw_vbo_swtnl(struct pipe_context *pipe, const struct pipe_draw_info* info)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	unsigned i;
	void *map;

	if (!nvfx_state_validate_swtnl(nvfx))
		return;
	nvfx_state_emit(nvfx);

	/* these must be passed without adding the offsets */
	for (i = 0; i < nvfx->vtxbuf_nr; i++) {
		map = nvfx_buffer(nvfx->vtxbuf[i].buffer)->data;
		draw_set_mapped_vertex_buffer(nvfx->draw, i, map);
	}

	map = NULL;
	if (info->indexed && nvfx->idxbuf.buffer)
		map = nvfx_buffer(nvfx->idxbuf.buffer)->data;
	draw_set_mapped_index_buffer(nvfx->draw, map);

	if (nvfx->constbuf[PIPE_SHADER_VERTEX]) {
		const unsigned nr = nvfx->constbuf_nr[PIPE_SHADER_VERTEX];

		map = nvfx_buffer(nvfx->constbuf[PIPE_SHADER_VERTEX])->data;
		draw_set_mapped_constant_buffer(nvfx->draw, PIPE_SHADER_VERTEX, 0,
                                                map, nr);
	}

	draw_vbo(nvfx->draw, info);

	draw_flush(nvfx->draw);
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

void
nvfx_vtxfmt_validate(struct nvfx_context *nvfx)
{
	struct nvfx_pipe_fragment_program *pfp = nvfx->fragprog;
	unsigned colour = 0, texcoords = 0, fog = 0, i;

	/* Determine needed fragprog inputs */
	for (i = 0; i < pfp->info.num_inputs; i++) {
		switch (pfp->info.input_semantic_name[i]) {
		case TGSI_SEMANTIC_POSITION:
			break;
		case TGSI_SEMANTIC_COLOR:
			colour |= (1 << pfp->info.input_semantic_index[i]);
			break;
		case TGSI_SEMANTIC_GENERIC:
			texcoords |= (1 << pfp->info.input_semantic_index[i]);
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
		emit_attrib(nvfx, 3 + i, EMIT_4F, TGSI_SEMANTIC_COLOR, i);
	}

	for (i = 0; i < 8; i++) {
		if (!(texcoords & (1 << i)))
			continue;
		emit_attrib(nvfx, 8 + i, EMIT_4F, TGSI_SEMANTIC_GENERIC, i);
	}

	if (fog) {
		emit_attrib(nvfx, 5, EMIT_1F, TGSI_SEMANTIC_FOG, 0);
	}

	emit_attrib(nvfx, 0, 0xff, TGSI_SEMANTIC_POSITION, 0);
}
