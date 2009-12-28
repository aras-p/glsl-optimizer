#include "nv20_context.h"
#include "nv20_state.h"
#include "draw/draw_context.h"

static void nv20_state_emit_blend(struct nv20_context* nv20)
{
	struct nv20_blend_state *b = nv20->blend;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;

	BEGIN_RING(chan, kelvin, NV20TCL_DITHER_ENABLE, 1);
	OUT_RING  (chan, b->d_enable);

	BEGIN_RING(chan, kelvin, NV20TCL_BLEND_FUNC_ENABLE, 1);
	OUT_RING  (chan, b->b_enable);

	BEGIN_RING(chan, kelvin, NV20TCL_BLEND_FUNC_SRC, 2);
	OUT_RING  (chan, b->b_srcfunc);
	OUT_RING  (chan, b->b_dstfunc);

	BEGIN_RING(chan, kelvin, NV20TCL_COLOR_MASK, 1);
	OUT_RING  (chan, b->c_mask);
}

static void nv20_state_emit_blend_color(struct nv20_context* nv20)
{
	struct pipe_blend_color *c = nv20->blend_color;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;

	BEGIN_RING(chan, kelvin, NV20TCL_BLEND_COLOR, 1);
	OUT_RING  (chan,
		   (float_to_ubyte(c->color[3]) << 24)|
		   (float_to_ubyte(c->color[0]) << 16)|
		   (float_to_ubyte(c->color[1]) << 8) |
		   (float_to_ubyte(c->color[2]) << 0));
}

static void nv20_state_emit_rast(struct nv20_context* nv20)
{
	struct nv20_rasterizer_state *r = nv20->rast;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;

	BEGIN_RING(chan, kelvin, NV20TCL_SHADE_MODEL, 2);
	OUT_RING  (chan, r->shade_model);
	OUT_RING  (chan, r->line_width);


	BEGIN_RING(chan, kelvin, NV20TCL_POINT_SIZE, 1);
	OUT_RING  (chan, r->point_size);

	BEGIN_RING(chan, kelvin, NV20TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING  (chan, r->poly_mode_front);
	OUT_RING  (chan, r->poly_mode_back);


	BEGIN_RING(chan, kelvin, NV20TCL_CULL_FACE, 2);
	OUT_RING  (chan, r->cull_face);
	OUT_RING  (chan, r->front_face);

	BEGIN_RING(chan, kelvin, NV20TCL_LINE_SMOOTH_ENABLE, 2);
	OUT_RING  (chan, r->line_smooth_en);
	OUT_RING  (chan, r->poly_smooth_en);

	BEGIN_RING(chan, kelvin, NV20TCL_CULL_FACE_ENABLE, 1);
	OUT_RING  (chan, r->cull_face_en);
}

static void nv20_state_emit_dsa(struct nv20_context* nv20)
{
	struct nv20_depth_stencil_alpha_state *d = nv20->dsa;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;

	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_FUNC, 1);
	OUT_RING (chan, d->depth.func);

	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING (chan, d->depth.write_enable);

	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING (chan, d->depth.test_enable);

	BEGIN_RING(chan, kelvin, NV20TCL_DEPTH_UNK17D8, 1);
	OUT_RING (chan, 1);

#if 0
	BEGIN_RING(chan, kelvin, NV20TCL_STENCIL_ENABLE, 1);
	OUT_RING (chan, d->stencil.enable);
	BEGIN_RING(chan, kelvin, NV20TCL_STENCIL_MASK, 7);
	OUT_RINGp (chan, (uint32_t *)&(d->stencil.wmask), 7);
#endif

	BEGIN_RING(chan, kelvin, NV20TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING (chan, d->alpha.enabled);

	BEGIN_RING(chan, kelvin, NV20TCL_ALPHA_FUNC_FUNC, 1);
	OUT_RING (chan, d->alpha.func);

	BEGIN_RING(chan, kelvin, NV20TCL_ALPHA_FUNC_REF, 1);
	OUT_RING (chan, d->alpha.ref);
}

static void nv20_state_emit_viewport(struct nv20_context* nv20)
{
}

static void nv20_state_emit_scissor(struct nv20_context* nv20)
{
	/* NV20TCL_SCISSOR_* is probably a software method */
/*	struct pipe_scissor_state *s = nv20->scissor;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;

	BEGIN_RING(chan, kelvin, NV20TCL_SCISSOR_HORIZ, 2);
	OUT_RING  (chan, ((s->maxx - s->minx) << 16) | s->minx);
	OUT_RING  (chan, ((s->maxy - s->miny) << 16) | s->miny);*/
}

static void nv20_state_emit_framebuffer(struct nv20_context* nv20)
{
	struct pipe_framebuffer_state* fb = nv20->framebuffer;
	struct nv04_surface *rt, *zeta = NULL;
	uint32_t rt_format, w, h;
	int colour_format = 0, zeta_format = 0;
	struct nv20_miptree *nv20mt = 0;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;

	w = fb->cbufs[0]->width;
	h = fb->cbufs[0]->height;
	colour_format = fb->cbufs[0]->format;
	rt = (struct nv04_surface *)fb->cbufs[0];

	if (fb->zsbuf) {
		if (colour_format) {
			assert(w == fb->zsbuf->width);
			assert(h == fb->zsbuf->height);
		} else {
			w = fb->zsbuf->width;
			h = fb->zsbuf->height;
		}

		zeta_format = fb->zsbuf->format;
		zeta = (struct nv04_surface *)fb->zsbuf;
	}

	rt_format = NV20TCL_RT_FORMAT_TYPE_LINEAR | 0x20;

	switch (colour_format) {
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		rt_format |= NV20TCL_RT_FORMAT_COLOR_X8R8G8B8;
		break;
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case 0:
		rt_format |= NV20TCL_RT_FORMAT_COLOR_A8R8G8B8;
		break;
	case PIPE_FORMAT_R5G6B5_UNORM:
		rt_format |= NV20TCL_RT_FORMAT_COLOR_R5G6B5;
		break;
	default:
		assert(0);
	}

	if (zeta) {
		BEGIN_RING(chan, kelvin, NV20TCL_RT_PITCH, 1);
		OUT_RING  (chan, rt->pitch | (zeta->pitch << 16));
	} else {
		BEGIN_RING(chan, kelvin, NV20TCL_RT_PITCH, 1);
		OUT_RING  (chan, rt->pitch | (rt->pitch << 16));
	}

	nv20mt = (struct nv20_miptree *)rt->base.texture;
	nv20->rt[0] = nv20mt->buffer;

	if (zeta_format)
	{
		nv20mt = (struct nv20_miptree *)zeta->base.texture;
		nv20->zeta = nv20mt->buffer;
	}

	BEGIN_RING(chan, kelvin, NV20TCL_RT_HORIZ, 3);
	OUT_RING  (chan, (w << 16) | 0);
	OUT_RING  (chan, (h << 16) | 0); /*NV20TCL_RT_VERT */
	OUT_RING  (chan, rt_format); /* NV20TCL_RT_FORMAT */
	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	OUT_RING  (chan, ((w - 1) << 16) | 0);
	OUT_RING  (chan, ((h - 1) << 16) | 0);
}

static void nv20_vertex_layout(struct nv20_context *nv20)
{
	struct nv20_fragment_program *fp = nv20->fragprog.current;
	struct draw_context *dc = nv20->draw;
	int src;
	int i;
	struct vertex_info *vinfo = &nv20->vertex_info;
	const enum interp_mode colorInterp = INTERP_LINEAR;
	boolean colors[2] = { FALSE };
	boolean generics[12] = { FALSE };
	boolean fog = FALSE;

	memset(vinfo, 0, sizeof(*vinfo));

	/*
	 * Assumed NV20 hardware vertex attribute order:
	 * 0 position, 1 ?, 2 ?, 3 col0,
	 * 4 col1?, 5 ?, 6 ?, 7 ?,
	 * 8 ?, 9 tex0, 10 tex1, 11 tex2,
	 * 12 tex3, 13 ?, 14 ?, 15 ?
	 * unaccounted: wgh, nor, fog
	 * There are total 16 attrs.
	 * vinfo->hwfmt[0] has a used-bit corresponding to each of these.
	 * relation to TGSI_SEMANTIC_*:
	 * - POSITION: position (always used)
	 * - COLOR: col1, col0
	 * - GENERIC: tex3, tex2, tex1, tex0, normal, weight
	 * - FOG: fog
	 */

	for (i = 0; i < fp->info.num_inputs; i++) {
		int isn = fp->info.input_semantic_name[i];
		int isi = fp->info.input_semantic_index[i];
		switch (isn) {
		case TGSI_SEMANTIC_POSITION:
			break;
		case TGSI_SEMANTIC_COLOR:
			assert(isi < 2);
			colors[isi] = TRUE;
			break;
		case TGSI_SEMANTIC_GENERIC:
			assert(isi < 12);
			generics[isi] = TRUE;
			break;
		case TGSI_SEMANTIC_FOG:
			fog = TRUE;
			break;
		default:
			assert(0 && "unknown input_semantic_name");
		}
	}

	/* always do position */ {
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_POSITION, 0);
		draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_LINEAR, src);
		vinfo->hwfmt[0] |= (1 << 0);
	}

	/* two unnamed generics */
	for (i = 4; i < 6; i++) {
		if (!generics[i])
			continue;
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_GENERIC, i);
		draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
		vinfo->hwfmt[0] |= (1 << (i - 3));
	}

	if (colors[0]) {
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_COLOR, 0);
		draw_emit_vertex_attr(vinfo, EMIT_4F, colorInterp, src);
		vinfo->hwfmt[0] |= (1 << 3);
	}

	if (colors[1]) {
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_COLOR, 1);
		draw_emit_vertex_attr(vinfo, EMIT_4F, colorInterp, src);
		vinfo->hwfmt[0] |= (1 << 4);
	}

	/* four unnamed generics */
	for (i = 6; i < 10; i++) {
		if (!generics[i])
			continue;
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_GENERIC, i);
		draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
		vinfo->hwfmt[0] |= (1 << (i - 1));
	}

	/* tex0, tex1, tex2, tex3 */
	for (i = 0; i < 4; i++) {
		if (!generics[i])
			continue;
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_GENERIC, i);
		draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
		vinfo->hwfmt[0] |= (1 << (i + 9));
	}

	/* two unnamed generics */
	for (i = 10; i < 12; i++) {
		if (!generics[i])
			continue;
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_GENERIC, i);
		draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, src);
		vinfo->hwfmt[0] |= (1 << (i + 3));
	}

	if (fog) {
		src = draw_find_shader_output(dc, TGSI_SEMANTIC_FOG, 0);
		draw_emit_vertex_attr(vinfo, EMIT_1F, INTERP_PERSPECTIVE, src);
		vinfo->hwfmt[0] |= (1 << 15);
	}

	draw_compute_vertex_size(vinfo);
}

void
nv20_emit_hw_state(struct nv20_context *nv20)
{
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;
	struct nouveau_bo *rt_bo;
	int i;

	if (nv20->dirty & NV20_NEW_VERTPROG) {
		//nv20_vertprog_bind(nv20, nv20->vertprog.current);
		nv20->dirty &= ~NV20_NEW_VERTPROG;
	}

	if (nv20->dirty & NV20_NEW_FRAGPROG) {
		nv20_fragprog_bind(nv20, nv20->fragprog.current);
		/*XXX: clear NV20_NEW_FRAGPROG if no new program uploaded */
		nv20->dirty_samplers |= (1<<10);
		nv20->dirty_samplers = 0;
	}

	if (nv20->dirty_samplers || (nv20->dirty & NV20_NEW_FRAGPROG)) {
		nv20_fragtex_bind(nv20);
		nv20->dirty &= ~NV20_NEW_FRAGPROG;
	}

	if (nv20->dirty & NV20_NEW_VTXARRAYS) {
		nv20->dirty &= ~NV20_NEW_VTXARRAYS;
		nv20_vertex_layout(nv20);
		nv20_vtxbuf_bind(nv20);
	}

	if (nv20->dirty & NV20_NEW_BLEND) {
		nv20->dirty &= ~NV20_NEW_BLEND;
		nv20_state_emit_blend(nv20);
	}

	if (nv20->dirty & NV20_NEW_BLENDCOL) {
		nv20->dirty &= ~NV20_NEW_BLENDCOL;
		nv20_state_emit_blend_color(nv20);
	}

	if (nv20->dirty & NV20_NEW_RAST) {
		nv20->dirty &= ~NV20_NEW_RAST;
		nv20_state_emit_rast(nv20);
	}

	if (nv20->dirty & NV20_NEW_DSA) {
		nv20->dirty &= ~NV20_NEW_DSA;
		nv20_state_emit_dsa(nv20);
	}

 	if (nv20->dirty & NV20_NEW_VIEWPORT) {
		nv20->dirty &= ~NV20_NEW_VIEWPORT;
		nv20_state_emit_viewport(nv20);
	}

 	if (nv20->dirty & NV20_NEW_SCISSOR) {
		nv20->dirty &= ~NV20_NEW_SCISSOR;
		nv20_state_emit_scissor(nv20);
	}

 	if (nv20->dirty & NV20_NEW_FRAMEBUFFER) {
		nv20->dirty &= ~NV20_NEW_FRAMEBUFFER;
		nv20_state_emit_framebuffer(nv20);
	}

	/* Emit relocs for every referenced buffer.
	 * This is to ensure the bufmgr has an accurate idea of how
	 * the buffer is used.  This isn't very efficient, but we don't
	 * seem to take a significant performance hit.  Will be improved
	 * at some point.  Vertex arrays are emitted by nv20_vbo.c
	 */

	/* Render target */
	rt_bo = nouveau_bo(nv20->rt[0]);
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_COLOR, 1);
	OUT_RELOCo(chan, rt_bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, kelvin, NV20TCL_COLOR_OFFSET, 1);
	OUT_RELOCl(chan, rt_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	if (nv20->zeta) {
		struct nouveau_bo *zeta_bo = nouveau_bo(nv20->zeta);
		BEGIN_RING(chan, kelvin, NV20TCL_DMA_ZETA, 1);
		OUT_RELOCo(chan, zeta_bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(chan, kelvin, NV20TCL_ZETA_OFFSET, 1);
		OUT_RELOCl(chan, zeta_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		/* XXX for when we allocate LMA on nv17 */
/*		BEGIN_RING(chan, kelvin, NV10TCL_LMA_DEPTH_BUFFER_OFFSET, 1);
		OUT_RELOCl(chan, nouveau_bo(nv20->zeta + lma_offset));*/
	}

	/* Vertex buffer */
	BEGIN_RING(chan, kelvin, NV20TCL_DMA_VTXBUF0, 1);
	OUT_RELOCo(chan, rt_bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, kelvin, NV20TCL_COLOR_OFFSET, 1);
	OUT_RELOCl(chan, rt_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	/* Texture images */
	for (i = 0; i < 2; i++) {
		if (!(nv20->fp_samplers & (1 << i)))
			continue;
		struct nouveau_bo *bo = nouveau_bo(nv20->tex[i].buffer);
		BEGIN_RING(chan, kelvin, NV20TCL_TX_OFFSET(i), 1);
		OUT_RELOCl(chan, bo, 0, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		BEGIN_RING(chan, kelvin, NV20TCL_TX_FORMAT(i), 1);
		OUT_RELOCd(chan, bo, nv20->tex[i].format,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			   NOUVEAU_BO_OR, NV20TCL_TX_FORMAT_DMA0,
			   NV20TCL_TX_FORMAT_DMA1);
	}
}

