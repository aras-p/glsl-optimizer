#include "pipe/p_shader_tokens.h"
#include "util/u_inlines.h"

#include "util/u_pack_color.h"

#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pipe.h"

#include "nvfx_context.h"
#include "nvfx_resource.h"

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

static void
nvfx_render_flush(struct draw_stage *stage, unsigned flags)
{
	struct nvfx_render_stage *rs = nvfx_render_stage(stage);
	struct nvfx_context *nvfx = rs->nvfx;
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;

	if (rs->prim != NV30_3D_VERTEX_BEGIN_END_STOP) {
		BEGIN_RING(chan, eng3d, NV30_3D_VERTEX_BEGIN_END, 1);
		OUT_RING(chan, NV30_3D_VERTEX_BEGIN_END_STOP);
		rs->prim = NV30_3D_VERTEX_BEGIN_END_STOP;
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
	boolean no_elements = nvfx->vertprog->draw_no_elements;
	unsigned num_attribs = nvfx->vertprog->draw_elements;

	/* we need to account the flush as well here even if it is done afterthis
	 * function
	 */
	if (AVAIL_RING(chan) < ((1 + count * num_attribs * 4) + 6 + 64)) {
		nvfx_render_flush(stage, 0);
		FIRE_RING(chan);
		nvfx_state_emit(nvfx);

		assert(AVAIL_RING(chan) >= ((1 + count * num_attribs * 4) + 6 + 64));
	}

	/* Switch primitive modes if necessary */
	if (rs->prim != mode) {
		if (rs->prim != NV30_3D_VERTEX_BEGIN_END_STOP) {
			BEGIN_RING(chan, eng3d, NV30_3D_VERTEX_BEGIN_END, 1);
			OUT_RING(chan, NV30_3D_VERTEX_BEGIN_END_STOP);
		}

		/* XXX: any command a lot of times seems to (mostly) fix corruption that would otherwise happen */
		/* this seems to cause issues on nv3x, and also be unneeded there */
		if(nvfx->is_nv4x)
		{
			int i;
			for(i = 0; i < 32; ++i)
			{
				BEGIN_RING(chan, eng3d, 0x1dac, 1);
				OUT_RING(chan, 0);
			}
		}

		BEGIN_RING(chan, eng3d, NV30_3D_VERTEX_BEGIN_END, 1);
		OUT_RING  (chan, mode);
		rs->prim = mode;
	}

	if(no_elements) {
		BEGIN_RING_NI(chan, eng3d, NV30_3D_VERTEX_DATA, 4);
		OUT_RING(chan, 0);
		OUT_RING(chan, 0);
		OUT_RING(chan, 0);
		OUT_RING(chan, 0);
	} else {
		BEGIN_RING_NI(chan, eng3d, NV30_3D_VERTEX_DATA, num_attribs * 4 * count);
		for (unsigned i = 0; i < count; ++i)
		{
			struct vertex_header* v = prim->v[i];
			/* TODO: disable divide where it's causing the problem, and remove this hack */
			OUT_RING(chan, fui(v->data[0][0] / v->data[0][3]));
			OUT_RING(chan, fui(v->data[0][1] / v->data[0][3]));
			OUT_RING(chan, fui(v->data[0][2] / v->data[0][3]));
			OUT_RING(chan, fui(1.0f / v->data[0][3]));
			OUT_RINGp(chan, &v->data[1][0], 4 * (num_attribs - 1));
		}
	}
}

static void
nvfx_render_point(struct draw_stage *draw, struct prim_header *prim)
{
	nvfx_render_prim(draw, prim, NV30_3D_VERTEX_BEGIN_END_POINTS, 1);
}

static void
nvfx_render_line(struct draw_stage *draw, struct prim_header *prim)
{
	nvfx_render_prim(draw, prim, NV30_3D_VERTEX_BEGIN_END_LINES, 2);
}

static void
nvfx_render_tri(struct draw_stage *draw, struct prim_header *prim)
{
	nvfx_render_prim(draw, prim, NV30_3D_VERTEX_BEGIN_END_TRIANGLES, 3);
}

static void
nvfx_render_reset_stipple_counter(struct draw_stage *draw)
{
	/* this doesn't really seem to work, but it matters rather little */
	nvfx_render_flush(draw, 0);
}

static void
nvfx_render_destroy(struct draw_stage *draw)
{
	FREE(draw);
}

struct draw_stage *
nvfx_draw_render_stage(struct nvfx_context *nvfx)
{
	struct nvfx_render_stage *render = CALLOC_STRUCT(nvfx_render_stage);

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
