#include "draw/draw_pipe.h"
#include "pipe/p_util.h"

#include "nv50_context.h"

struct nv50_render_stage {
	struct draw_stage stage;
	struct nv50_context *nv50;
};

static INLINE struct nv50_render_stage *
nv50_render_stage(struct draw_stage *stage)
{
	return (struct nv50_render_stage *)stage;
}

static void
nv50_render_point(struct draw_stage *stage, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_render_line(struct draw_stage *stage, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_render_tri(struct draw_stage *stage, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_render_flush(struct draw_stage *stage, unsigned flags)
{
}

static void
nv50_render_reset_stipple_counter(struct draw_stage *stage)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_render_destroy(struct draw_stage *stage)
{
	free(stage);
}

struct draw_stage *
nv50_draw_render_stage(struct nv50_context *nv50)
{
	struct nv50_render_stage *rs = CALLOC_STRUCT(nv50_render_stage);

	rs->nv50 = nv50;
	rs->stage.draw = nv50->draw;
	rs->stage.destroy = nv50_render_destroy;
	rs->stage.point = nv50_render_point;
	rs->stage.line = nv50_render_line;
	rs->stage.tri = nv50_render_tri;
	rs->stage.flush = nv50_render_flush;
	rs->stage.reset_stipple_counter = nv50_render_reset_stipple_counter;

	return &rs->stage;
}

