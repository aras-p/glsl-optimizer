#include "pipe/draw/draw_private.h"
#include "pipe/p_util.h"

#include "nv40_context.h"

struct nv40_draw_stage {
	struct draw_stage draw;
	struct nv40_context *nv40;
};

static void
nv40_draw_point(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv40_draw_line(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv40_draw_tri(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv40_draw_flush(struct draw_stage *draw, unsigned flags)
{
}

static void
nv40_draw_reset_stipple_counter(struct draw_stage *draw)
{
	NOUVEAU_ERR("\n");
}

static void
nv40_draw_destroy(struct draw_stage *draw)
{
	free(draw);
}

struct draw_stage *
nv40_draw_render_stage(struct nv40_context *nv40)
{
	struct nv40_draw_stage *nv40draw = CALLOC_STRUCT(nv40_draw_stage);

	nv40draw->nv40 = nv40;
	nv40draw->draw.draw = nv40->draw;
	nv40draw->draw.point = nv40_draw_point;
	nv40draw->draw.line = nv40_draw_line;
	nv40draw->draw.tri = nv40_draw_tri;
	nv40draw->draw.flush = nv40_draw_flush;
	nv40draw->draw.reset_stipple_counter = nv40_draw_reset_stipple_counter;
	nv40draw->draw.destroy = nv40_draw_destroy;

	return &nv40draw->draw;
}

