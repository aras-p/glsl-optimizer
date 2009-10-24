#include "nv10_context.h"
#include "nv10_state.h"

static void nv10_state_emit_blend(struct nv10_context* nv10)
{
	struct nv10_blend_state *b = nv10->blend;

	BEGIN_RING(celsius, NV10TCL_DITHER_ENABLE, 1);
	OUT_RING  (b->d_enable);

	BEGIN_RING(celsius, NV10TCL_BLEND_FUNC_ENABLE, 3);
	OUT_RING  (b->b_enable);
	OUT_RING  (b->b_srcfunc);
	OUT_RING  (b->b_dstfunc);

	BEGIN_RING(celsius, NV10TCL_COLOR_MASK, 1);
	OUT_RING  (b->c_mask);
}

static void nv10_state_emit_blend_color(struct nv10_context* nv10)
{
	struct pipe_blend_color *c = nv10->blend_color;

	BEGIN_RING(celsius, NV10TCL_BLEND_COLOR, 1);
	OUT_RING  ((float_to_ubyte(c->color[3]) << 24)|
		   (float_to_ubyte(c->color[0]) << 16)|
		   (float_to_ubyte(c->color[1]) << 8) |
		   (float_to_ubyte(c->color[2]) << 0));
}

static void nv10_state_emit_rast(struct nv10_context* nv10)
{
	struct nv10_rasterizer_state *r = nv10->rast;

	BEGIN_RING(celsius, NV10TCL_SHADE_MODEL, 2);
	OUT_RING  (r->shade_model);
	OUT_RING  (r->line_width);


	BEGIN_RING(celsius, NV10TCL_POINT_SIZE, 1);
	OUT_RING  (r->point_size);

	BEGIN_RING(celsius, NV10TCL_POLYGON_MODE_FRONT, 2);
	OUT_RING  (r->poly_mode_front);
	OUT_RING  (r->poly_mode_back);


	BEGIN_RING(celsius, NV10TCL_CULL_FACE, 2);
	OUT_RING  (r->cull_face);
	OUT_RING  (r->front_face);

	BEGIN_RING(celsius, NV10TCL_LINE_SMOOTH_ENABLE, 2);
	OUT_RING  (r->line_smooth_en);
	OUT_RING  (r->poly_smooth_en);

	BEGIN_RING(celsius, NV10TCL_CULL_FACE_ENABLE, 1);
	OUT_RING  (r->cull_face_en);
}

static void nv10_state_emit_dsa(struct nv10_context* nv10)
{
	struct nv10_depth_stencil_alpha_state *d = nv10->dsa;

	BEGIN_RING(celsius, NV10TCL_DEPTH_FUNC, 1);
	OUT_RING (d->depth.func);

	BEGIN_RING(celsius, NV10TCL_DEPTH_WRITE_ENABLE, 1);
	OUT_RING (d->depth.write_enable);

	BEGIN_RING(celsius, NV10TCL_DEPTH_TEST_ENABLE, 1);
	OUT_RING (d->depth.test_enable);

#if 0
	BEGIN_RING(celsius, NV10TCL_STENCIL_ENABLE, 1);
	OUT_RING (d->stencil.enable);
	BEGIN_RING(celsius, NV10TCL_STENCIL_MASK, 7);
	OUT_RINGp ((uint32_t *)&(d->stencil.wmask), 7);
#endif

	BEGIN_RING(celsius, NV10TCL_ALPHA_FUNC_ENABLE, 1);
	OUT_RING (d->alpha.enabled);

	BEGIN_RING(celsius, NV10TCL_ALPHA_FUNC_FUNC, 1);
	OUT_RING (d->alpha.func);

	BEGIN_RING(celsius, NV10TCL_ALPHA_FUNC_REF, 1);
	OUT_RING (d->alpha.ref);
}

static void nv10_state_emit_viewport(struct nv10_context* nv10)
{
}

static void nv10_state_emit_scissor(struct nv10_context* nv10)
{
	// XXX this is so not working
/*	struct pipe_scissor_state *s = nv10->scissor;
	BEGIN_RING(celsius, NV10TCL_SCISSOR_HORIZ, 2);
	OUT_RING  (((s->maxx - s->minx) << 16) | s->minx);
	OUT_RING  (((s->maxy - s->miny) << 16) | s->miny);*/
}

static void nv10_state_emit_framebuffer(struct nv10_context* nv10)
{
	struct pipe_framebuffer_state* fb = nv10->framebuffer;
	struct nv04_surface *rt, *zeta = NULL;
	uint32_t rt_format, w, h;
	int colour_format = 0, zeta_format = 0;
        struct nv10_miptree *nv10mt = 0;

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

	rt_format = NV10TCL_RT_FORMAT_TYPE_LINEAR;

	switch (colour_format) {
	case PIPE_FORMAT_X8R8G8B8_UNORM:
		rt_format |= NV10TCL_RT_FORMAT_COLOR_X8R8G8B8;
		break;
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case 0:
		rt_format |= NV10TCL_RT_FORMAT_COLOR_A8R8G8B8;
		break;
	case PIPE_FORMAT_R5G6B5_UNORM:
		rt_format |= NV10TCL_RT_FORMAT_COLOR_R5G6B5;
		break;
	default:
		assert(0);
	}

	if (zeta) {
		BEGIN_RING(celsius, NV10TCL_RT_PITCH, 1);
		OUT_RING  (rt->pitch | (zeta->pitch << 16));
	} else {
		BEGIN_RING(celsius, NV10TCL_RT_PITCH, 1);
		OUT_RING  (rt->pitch | (rt->pitch << 16));
	}

	nv10mt = (struct nv10_miptree *)rt->base.texture;
	nv10->rt[0] = nv10mt->buffer;

	if (zeta_format)
	{
		nv10mt = (struct nv10_miptree *)zeta->base.texture;
		nv10->zeta = nv10mt->buffer;
	}

	BEGIN_RING(celsius, NV10TCL_RT_HORIZ, 3);
	OUT_RING  ((w << 16) | 0);
	OUT_RING  ((h << 16) | 0);
	OUT_RING  (rt_format);
	BEGIN_RING(celsius, NV10TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	OUT_RING  (((w - 1) << 16) | 0 | 0x08000800);
	OUT_RING  (((h - 1) << 16) | 0 | 0x08000800);
}

static void nv10_vertex_layout(struct nv10_context *nv10)
{
	struct nv10_fragment_program *fp = nv10->fragprog.current;
	uint32_t src = 0;
	int i;
	struct vertex_info vinfo;

	memset(&vinfo, 0, sizeof(vinfo));

	for (i = 0; i < fp->info.num_inputs; i++) {
		switch (fp->info.input_semantic_name[i]) {
			case TGSI_SEMANTIC_POSITION:
				draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR, src++);
				break;
			case TGSI_SEMANTIC_COLOR:
				draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR, src++);
				break;
			default:
			case TGSI_SEMANTIC_GENERIC:
				draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_PERSPECTIVE, src++);
				break;
			case TGSI_SEMANTIC_FOG:
				draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_PERSPECTIVE, src++);
				break;
		}
	}
	draw_compute_vertex_size(&vinfo);
}

void
nv10_emit_hw_state(struct nv10_context *nv10)
{
	int i;

	if (nv10->dirty & NV10_NEW_VERTPROG) {
		//nv10_vertprog_bind(nv10, nv10->vertprog.current);
		nv10->dirty &= ~NV10_NEW_VERTPROG;
	}

	if (nv10->dirty & NV10_NEW_FRAGPROG) {
		nv10_fragprog_bind(nv10, nv10->fragprog.current);
		/*XXX: clear NV10_NEW_FRAGPROG if no new program uploaded */
		nv10->dirty_samplers |= (1<<10);
		nv10->dirty_samplers = 0;
	}

	if (nv10->dirty_samplers || (nv10->dirty & NV10_NEW_FRAGPROG)) {
		nv10_fragtex_bind(nv10);
		nv10->dirty &= ~NV10_NEW_FRAGPROG;
	}

	if (nv10->dirty & NV10_NEW_VTXARRAYS) {
		nv10->dirty &= ~NV10_NEW_VTXARRAYS;
		nv10_vertex_layout(nv10);
		nv10_vtxbuf_bind(nv10);
	}

	if (nv10->dirty & NV10_NEW_BLEND) {
		nv10->dirty &= ~NV10_NEW_BLEND;
		nv10_state_emit_blend(nv10);
	}

	if (nv10->dirty & NV10_NEW_BLENDCOL) {
		nv10->dirty &= ~NV10_NEW_BLENDCOL;
		nv10_state_emit_blend_color(nv10);
	}

	if (nv10->dirty & NV10_NEW_RAST) {
		nv10->dirty &= ~NV10_NEW_RAST;
		nv10_state_emit_rast(nv10);
	}

	if (nv10->dirty & NV10_NEW_DSA) {
		nv10->dirty &= ~NV10_NEW_DSA;
		nv10_state_emit_dsa(nv10);
	}

 	if (nv10->dirty & NV10_NEW_VIEWPORT) {
		nv10->dirty &= ~NV10_NEW_VIEWPORT;
		nv10_state_emit_viewport(nv10);
	}

 	if (nv10->dirty & NV10_NEW_SCISSOR) {
		nv10->dirty &= ~NV10_NEW_SCISSOR;
		nv10_state_emit_scissor(nv10);
	}

 	if (nv10->dirty & NV10_NEW_FRAMEBUFFER) {
		nv10->dirty &= ~NV10_NEW_FRAMEBUFFER;
		nv10_state_emit_framebuffer(nv10);
	}

	/* Emit relocs for every referenced buffer.
	 * This is to ensure the bufmgr has an accurate idea of how
	 * the buffer is used.  This isn't very efficient, but we don't
	 * seem to take a significant performance hit.  Will be improved
	 * at some point.  Vertex arrays are emitted by nv10_vbo.c
	 */

	/* Render target */
// XXX figre out who's who for NV10TCL_DMA_* and fill accordingly
//	BEGIN_RING(celsius, NV10TCL_DMA_COLOR0, 1);
//	OUT_RELOCo(nv10->rt[0], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(celsius, NV10TCL_COLOR_OFFSET, 1);
	OUT_RELOCl(nv10->rt[0], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	if (nv10->zeta) {
// XXX
//		BEGIN_RING(celsius, NV10TCL_DMA_ZETA, 1);
//		OUT_RELOCo(nv10->zeta, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(celsius, NV10TCL_ZETA_OFFSET, 1);
		OUT_RELOCl(nv10->zeta, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		/* XXX for when we allocate LMA on nv17 */
/*		BEGIN_RING(celsius, NV10TCL_LMA_DEPTH_BUFFER_OFFSET, 1);
		OUT_RELOCl(nv10->zeta + lma_offset);*/
	}

	/* Vertex buffer */
	BEGIN_RING(celsius, NV10TCL_DMA_VTXBUF0, 1);
	OUT_RELOCo(nv10->rt[0], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(celsius, NV10TCL_COLOR_OFFSET, 1);
	OUT_RELOCl(nv10->rt[0], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	/* Texture images */
	for (i = 0; i < 2; i++) {
		if (!(nv10->fp_samplers & (1 << i)))
			continue;
		BEGIN_RING(celsius, NV10TCL_TX_OFFSET(i), 1);
		OUT_RELOCl(nv10->tex[i].buffer, 0, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		BEGIN_RING(celsius, NV10TCL_TX_FORMAT(i), 1);
		OUT_RELOCd(nv10->tex[i].buffer, nv10->tex[i].format,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			   NOUVEAU_BO_OR, NV10TCL_TX_FORMAT_DMA0,
			   NV10TCL_TX_FORMAT_DMA1);
	}
}

