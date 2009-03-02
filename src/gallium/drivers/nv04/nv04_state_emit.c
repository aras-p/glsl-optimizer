#include "nv04_context.h"
#include "nv04_state.h"

static void nv04_vertex_layout(struct pipe_context* pipe)
{
	struct nv04_context *nv04 = nv04_context(pipe);
	struct nv04_fragment_program *fp = nv04->fragprog.current;
	uint32_t src = 0;
	int i;
	struct vertex_info vinfo;

	memset(&vinfo, 0, sizeof(vinfo));

	for (i = 0; i < fp->info.num_inputs; i++) {
		int isn = fp->info.input_semantic_name[i];
		int isi = fp->info.input_semantic_index[i];
		switch (isn) {
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

	printf("%d vertex input\n",fp->info.num_inputs);
	draw_compute_vertex_size(&vinfo);
}

static uint32_t nv04_blend_func(uint32_t f)
{
	switch ( f ) {
		case PIPE_BLENDFACTOR_ZERO:			return 0x1;
		case PIPE_BLENDFACTOR_ONE:			return 0x2;
		case PIPE_BLENDFACTOR_SRC_COLOR:		return 0x3;
		case PIPE_BLENDFACTOR_INV_SRC_COLOR:		return 0x4;
		case PIPE_BLENDFACTOR_SRC_ALPHA:		return 0x5;
		case PIPE_BLENDFACTOR_INV_SRC_ALPHA:		return 0x6;
		case PIPE_BLENDFACTOR_DST_ALPHA:		return 0x7;
		case PIPE_BLENDFACTOR_INV_DST_ALPHA:		return 0x8;
		case PIPE_BLENDFACTOR_DST_COLOR:		return 0x9;
		case PIPE_BLENDFACTOR_INV_DST_COLOR:		return 0xA;
		case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:	return 0xB;
	}
	NOUVEAU_MSG("Unable to find the blend function 0x%x\n",f);
	return 0;
}

static void nv04_emit_control(struct nv04_context* nv04)
{
	uint32_t control = nv04->dsa->control;

	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_CONTROL, 1);
	OUT_RING(control);
}

static void nv04_emit_blend(struct nv04_context* nv04)
{
	uint32_t blend;

	blend=0x4; // texture MODULATE_ALPHA
	blend|=0x20; // alpha is MSB
	blend|=(2<<6); // flat shading
	blend|=(1<<8); // persp correct
	blend|=(0<<16); // no fog
	blend|=(nv04->blend->b_enable<<20);
	blend|=(nv04_blend_func(nv04->blend->b_src)<<24);
	blend|=(nv04_blend_func(nv04->blend->b_dst)<<28);

	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_BLEND, 1);
	OUT_RING(blend);
}

static void nv04_emit_sampler(struct nv04_context *nv04, int unit)
{
	struct nv04_miptree *nv04mt = nv04->tex_miptree[unit];
	struct pipe_texture *pt = &nv04mt->base;

	BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_OFFSET, 3);
	OUT_RELOCl(nv04mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);
	OUT_RELOCd(nv04mt->buffer, (nv04->fragtex.format | nv04->sampler[unit]->format), NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_OR | NOUVEAU_BO_RD, 1/*VRAM*/,2/*TT*/);
	OUT_RING(nv04->sampler[unit]->filter);
}

static void nv04_state_emit_framebuffer(struct nv04_context* nv04)
{
	struct pipe_framebuffer_state* fb = nv04->framebuffer;
	struct nv04_surface *rt, *zeta;
	uint32_t rt_format, w, h;
	int colour_format = 0, zeta_format = 0;
	struct nv04_miptree *nv04mt = 0;

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

	switch (colour_format) {
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case 0:
		rt_format = 0x108;
		break;
	case PIPE_FORMAT_R5G6B5_UNORM:
		rt_format = 0x103;
		break;
	default:
		assert(0);
	}

	BEGIN_RING(context_surfaces_3d, NV04_CONTEXT_SURFACES_3D_FORMAT, 1);
	OUT_RING(rt_format);

	nv04mt = (struct nv04_miptree *)rt->base.texture;
	/* FIXME pitches have to be aligned ! */
	BEGIN_RING(context_surfaces_3d, NV04_CONTEXT_SURFACES_3D_PITCH, 2);
	OUT_RING(rt->pitch|(zeta->pitch<<16));
	OUT_RELOCl(nv04mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	if (fb->zsbuf) {
		nv04mt = (struct nv04_miptree *)zeta->base.texture;
		BEGIN_RING(context_surfaces_3d, NV04_CONTEXT_SURFACES_3D_OFFSET_ZETA, 1);
		OUT_RELOCl(nv04mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}
}

void
nv04_emit_hw_state(struct nv04_context *nv04)
{
	int i;

	if (nv04->dirty & NV04_NEW_VERTPROG) {
		//nv04_vertprog_bind(nv04, nv04->vertprog.current);
		nv04->dirty &= ~NV04_NEW_VERTPROG;
	}

	if (nv04->dirty & NV04_NEW_FRAGPROG) {
		nv04_fragprog_bind(nv04, nv04->fragprog.current);
		nv04->dirty &= ~NV04_NEW_FRAGPROG;
		nv04->dirty_samplers |= (1<<10);
		nv04->dirty_samplers = 0;
	}

	if (nv04->dirty & NV04_NEW_CONTROL) {
		nv04->dirty &= ~NV04_NEW_CONTROL;

		BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_CONTROL, 1);
		OUT_RING(nv04->dsa->control);
	}

	if (nv04->dirty & NV04_NEW_BLEND) {
		nv04->dirty &= ~NV04_NEW_BLEND;

		nv04_emit_blend(nv04);
	}

	if (nv04->dirty & NV04_NEW_VTXARRAYS) {
		nv04->dirty &= ~NV04_NEW_VTXARRAYS;
		nv04_vertex_layout(nv04);
	}

	if (nv04->dirty & NV04_NEW_SAMPLER) {
		nv04->dirty &= ~NV04_NEW_SAMPLER;

		nv04_emit_sampler(nv04, 0);
	}

	if (nv04->dirty & NV04_NEW_VIEWPORT) {
		nv04->dirty &= ~NV04_NEW_VIEWPORT;
//		nv04_state_emit_viewport(nv04);
	}

 	if (nv04->dirty & NV04_NEW_FRAMEBUFFER) {
		nv04->dirty &= ~NV04_NEW_FRAMEBUFFER;
		nv04_state_emit_framebuffer(nv04);
	}

	/* Emit relocs for every referenced buffer.
	 * This is to ensure the bufmgr has an accurate idea of how
	 * the buffer is used.  This isn't very efficient, but we don't
	 * seem to take a significant performance hit.  Will be improved
	 * at some point.  Vertex arrays are emitted by nv04_vbo.c
	 */

	/* Render target */
	unsigned rt_pitch = ((struct nv04_surface *)nv04->rt)->pitch;
	unsigned zeta_pitch = ((struct nv04_surface *)nv04->zeta)->pitch;

	BEGIN_RING(context_surfaces_3d, NV04_CONTEXT_SURFACES_3D_PITCH, 2);
	OUT_RING(rt_pitch|(zeta_pitch<<16));
	OUT_RELOCl(nv04->rt, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	if (nv04->zeta) {
		BEGIN_RING(context_surfaces_3d, NV04_CONTEXT_SURFACES_3D_OFFSET_ZETA, 1);
		OUT_RELOCl(nv04->zeta, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	/* Texture images */
	for (i = 0; i < 1; i++) {
		if (!(nv04->fp_samplers & (1 << i)))
			continue;
		struct nv04_miptree *nv04mt = nv04->tex_miptree[i];
		BEGIN_RING(fahrenheit, NV04_DX5_TEXTURED_TRIANGLE_OFFSET, 2);
		OUT_RELOCl(nv04mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		OUT_RELOCd(nv04mt->buffer, (nv04->fragtex.format | nv04->sampler[i]->format), NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_OR | NOUVEAU_BO_RD, 1/*VRAM*/,2/*TT*/);
	}
}

