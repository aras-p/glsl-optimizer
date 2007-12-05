#include "pipe/draw/draw_private.h"
#include "pipe/p_util.h"

#include "nv50_context.h"

struct nv50_draw_stage {
	struct draw_stage draw;
	struct nv50_context *nv50;
};

static void
nv50_draw_begin(struct draw_stage *draw)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_draw_end(struct draw_stage *draw)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_draw_point(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_draw_line(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_draw_tri(struct draw_stage *draw, struct prim_header *prim)
{
	NOUVEAU_ERR("\n");
}

static void
nv50_draw_reset_stipple_counter(struct draw_stage *draw)
{
	NOUVEAU_ERR("\n");
}

struct draw_stage *
nv50_draw_render_stage(struct nv50_context *nv50)
{
	struct nv50_draw_stage *nv50draw = CALLOC_STRUCT(nv50_draw_stage);

	nv50draw->nv50 = nv50;
	nv50draw->draw.draw = nv50->draw;
	nv50draw->draw.begin = nv50_draw_begin;
	nv50draw->draw.point = nv50_draw_point;
	nv50draw->draw.line = nv50_draw_line;
	nv50draw->draw.tri = nv50_draw_tri;
	nv50draw->draw.end = nv50_draw_end;
	nv50draw->draw.reset_stipple_counter = nv50_draw_reset_stipple_counter;

	return &nv50draw->draw;
}

