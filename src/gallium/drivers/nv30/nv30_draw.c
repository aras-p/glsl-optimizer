#include "draw/draw_pipe.h"

#include "nv30_context.h"

struct nv30_draw_stage {
	struct draw_stage draw;
	struct nv30_context *nv30;
};

static void
nv30_draw_point(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv30_draw_line(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv30_draw_tri(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv30_draw_flush(struct draw_stage *draw, unsigned flags)
{
}

static void
nv30_draw_reset_stipple_counter(struct draw_stage *draw)
{
	NOUVEAU_ERR("\n");
}

static void
nv30_draw_destroy(struct draw_stage *draw)
{
	FREE(draw);
}

struct draw_stage *
nv30_draw_render_stage(struct nv30_context *nv30)
{
	struct nv30_draw_stage *nv30draw = CALLOC_STRUCT(nv30_draw_stage);

	nv30draw->nv30 = nv30;
	nv30draw->draw.draw = nv30->draw;
	nv30draw->draw.point = nv30_draw_point;
	nv30draw->draw.line = nv30_draw_line;
	nv30draw->draw.tri = nv30_draw_tri;
	nv30draw->draw.flush = nv30_draw_flush;
	nv30draw->draw.reset_stipple_counter = nv30_draw_reset_stipple_counter;
	nv30draw->draw.destroy = nv30_draw_destroy;

	return &nv30draw->draw;
}

