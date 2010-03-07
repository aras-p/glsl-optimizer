#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "tgsi/tgsi_parse.h"

#include "nv30_context.h"
#include "nv30_state.h"

static void *
nv30_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nouveau_grobj *rankine = nv30->screen->rankine;
	struct nv30_blend_state *bso = CALLOC(1, sizeof(*bso));
	struct nouveau_stateobj *so = so_new(5, 8, 0);

	if (cso->rt[0].blend_enable) {
		so_method(so, rankine, NV34TCL_BLEND_FUNC_ENABLE, 3);
		so_data  (so, 1);
		so_data  (so, (nvgl_blend_func(cso->rt[0].alpha_src_factor) << 16) |
			       nvgl_blend_func(cso->rt[0].rgb_src_factor));
		so_data  (so, nvgl_blend_func(cso->rt[0].alpha_dst_factor) << 16 |
			      nvgl_blend_func(cso->rt[0].rgb_dst_factor));
		/* FIXME: Gallium assumes GL_EXT_blend_func_separate.
		   It is not the case for NV30 */
		so_method(so, rankine, NV34TCL_BLEND_EQUATION, 1);
		so_data  (so, nvgl_blend_eqn(cso->rt[0].rgb_func));
	} else {
		so_method(so, rankine, NV34TCL_BLEND_FUNC_ENABLE, 1);
		so_data  (so, 0);
	}

	so_method(so, rankine, NV34TCL_COLOR_MASK, 1);
	so_data  (so, (((cso->rt[0].colormask & PIPE_MASK_A) ? (0x01 << 24) : 0) |
		       ((cso->rt[0].colormask & PIPE_MASK_R) ? (0x01 << 16) : 0) |
		       ((cso->rt[0].colormask & PIPE_MASK_G) ? (0x01 <<  8) : 0) |
		       ((cso->rt[0].colormask & PIPE_MASK_B) ? (0x01 <<  0) : 0)));

	if (cso->logicop_enable) {
		so_method(so, rankine, NV34TCL_COLOR_LOGIC_OP_ENABLE, 2);
		so_data  (so, 1);
		so_data  (so, nvgl_logicop_func(cso->logicop_func));
	} else {
		so_method(so, rankine, NV34TCL_COLOR_LOGIC_OP_ENABLE, 1);
		so_data  (so, 0);
	}

	so_method(so, rankine, NV34TCL_DITHER_ENABLE, 1);
	so_data  (so, cso->dither ? 1 : 0);

	so_ref(so, &bso->so);
	so_ref(NULL, &so);
	bso->pipe = *cso;
	return (void *)bso;
}

static void
nv30_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->blend = hwcso;
	nv30->dirty |= NV30_NEW_BLEND;
}

static void
nv30_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_blend_state *bso = hwcso;

	so_ref(NULL, &bso->so);
	FREE(bso);
}


static INLINE unsigned
wrap_mode(unsigned wrap) {
	unsigned ret;

	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		ret = NV34TCL_TX_WRAP_S_REPEAT;
		break;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		ret = NV34TCL_TX_WRAP_S_MIRRORED_REPEAT;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		ret = NV34TCL_TX_WRAP_S_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		ret = NV34TCL_TX_WRAP_S_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_CLAMP:
		ret = NV34TCL_TX_WRAP_S_CLAMP;
		break;
/*	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		ret = NV34TCL_TX_WRAP_S_MIRROR_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		ret = NV34TCL_TX_WRAP_S_MIRROR_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		ret = NV34TCL_TX_WRAP_S_MIRROR_CLAMP;
		break;*/
	default:
		NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
		ret = NV34TCL_TX_WRAP_S_REPEAT;
		break;
	}

	return ret >> NV34TCL_TX_WRAP_S_SHIFT;
}

static void *
nv30_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	struct nv30_sampler_state *ps;
	uint32_t filter = 0;

	ps = MALLOC(sizeof(struct nv30_sampler_state));

	ps->fmt = 0;
	/* TODO: Not all RECTs formats have this bit set, bits 15-8 of format
	   are the tx format to use. We should store normalized coord flag
	   in sampler state structure, and set appropriate format in
	   nvxx_fragtex_build()
	 */
	/*NV34TCL_TX_FORMAT_RECT*/
	/*if (!cso->normalized_coords) {
		ps->fmt |= (1<<14) ;
	}*/

	ps->wrap = ((wrap_mode(cso->wrap_s) << NV34TCL_TX_WRAP_S_SHIFT) |
		    (wrap_mode(cso->wrap_t) << NV34TCL_TX_WRAP_T_SHIFT) |
		    (wrap_mode(cso->wrap_r) << NV34TCL_TX_WRAP_R_SHIFT));

	ps->en = 0;

	if (cso->max_anisotropy >= 8) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_8X;
	} else
	if (cso->max_anisotropy >= 4) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_4X;
	} else
	if (cso->max_anisotropy >= 2) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_2X;
	}

	switch (cso->mag_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		filter |= NV34TCL_TX_FILTER_MAGNIFY_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		filter |= NV34TCL_TX_FILTER_MAGNIFY_NEAREST;
		break;
	}

	switch (cso->min_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV34TCL_TX_FILTER_MINIFY_LINEAR_MIPMAP_NEAREST;
			break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV34TCL_TX_FILTER_MINIFY_LINEAR_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV34TCL_TX_FILTER_MINIFY_LINEAR;
			break;
		}
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV34TCL_TX_FILTER_MINIFY_NEAREST_MIPMAP_NEAREST;
		break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV34TCL_TX_FILTER_MINIFY_NEAREST_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV34TCL_TX_FILTER_MINIFY_NEAREST;
			break;
		}
		break;
	}

	ps->filt = filter;

	{
		float limit;

		limit = CLAMP(cso->lod_bias, -16.0, 15.0);
		ps->filt |= (int)(cso->lod_bias * 256.0) & 0x1fff;

		limit = CLAMP(cso->max_lod, 0.0, 15.0);
		ps->en |= (int)(limit) << 14 /*NV34TCL_TX_ENABLE_MIPMAP_MAX_LOD_SHIFT*/;

		limit = CLAMP(cso->min_lod, 0.0, 15.0);
		ps->en |= (int)(limit) << 26 /*NV34TCL_TX_ENABLE_MIPMAP_MIN_LOD_SHIFT*/;
	}

	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
		switch (cso->compare_func) {
		case PIPE_FUNC_NEVER:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_NEVER;
			break;
		case PIPE_FUNC_GREATER:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_GREATER;
			break;
		case PIPE_FUNC_EQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_EQUAL;
			break;
		case PIPE_FUNC_GEQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_GEQUAL;
			break;
		case PIPE_FUNC_LESS:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_LESS;
			break;
		case PIPE_FUNC_NOTEQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_NOTEQUAL;
			break;
		case PIPE_FUNC_LEQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_LEQUAL;
			break;
		case PIPE_FUNC_ALWAYS:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_ALWAYS;
			break;
		default:
			break;
		}
	}

	ps->bcol = ((float_to_ubyte(cso->border_color[3]) << 24) |
		    (float_to_ubyte(cso->border_color[0]) << 16) |
		    (float_to_ubyte(cso->border_color[1]) <<  8) |
		    (float_to_ubyte(cso->border_color[2]) <<  0));

	return (void *)ps;
}

static void
nv30_sampler_state_bind(struct pipe_context *pipe, unsigned nr, void **sampler)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nv30->tex_sampler[unit] = sampler[unit];
		nv30->dirty_samplers |= (1 << unit);
	}

	for (unit = nr; unit < nv30->nr_samplers; unit++) {
		nv30->tex_sampler[unit] = NULL;
		nv30->dirty_samplers |= (1 << unit);
	}

	nv30->nr_samplers = nr;
	nv30->dirty |= NV30_NEW_SAMPLER;
}

static void
nv30_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void
nv30_set_sampler_texture(struct pipe_context *pipe, unsigned nr,
			 struct pipe_texture **miptree)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		pipe_texture_reference((struct pipe_texture **)
				       &nv30->tex_miptree[unit], miptree[unit]);
		nv30->dirty_samplers |= (1 << unit);
	}

	for (unit = nr; unit < nv30->nr_textures; unit++) {
		pipe_texture_reference((struct pipe_texture **)
				       &nv30->tex_miptree[unit], NULL);
		nv30->dirty_samplers |= (1 << unit);
	}

	nv30->nr_textures = nr;
	nv30->dirty |= NV30_NEW_SAMPLER;
}

static void *
nv30_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_rasterizer_state *rsso = CALLOC(1, sizeof(*rsso));
	struct nouveau_stateobj *so = so_new(9, 19, 0);
	struct nouveau_grobj *rankine = nv30->screen->rankine;

	/*XXX: ignored:
	 * 	light_twoside
	 * 	point_smooth -nohw
	 * 	multisample
	 */

	so_method(so, rankine, NV34TCL_SHADE_MODEL, 1);
	so_data  (so, cso->flatshade ? NV34TCL_SHADE_MODEL_FLAT :
				       NV34TCL_SHADE_MODEL_SMOOTH);

	so_method(so, rankine, NV34TCL_LINE_WIDTH, 2);
	so_data  (so, (unsigned char)(cso->line_width * 8.0) & 0xff);
	so_data  (so, cso->line_smooth ? 1 : 0);
	so_method(so, rankine, NV34TCL_LINE_STIPPLE_ENABLE, 2);
	so_data  (so, cso->line_stipple_enable ? 1 : 0);
	so_data  (so, (cso->line_stipple_pattern << 16) |
		       cso->line_stipple_factor);

	so_method(so, rankine, NV34TCL_POINT_SIZE, 1);
	so_data  (so, fui(cso->point_size));

	so_method(so, rankine, NV34TCL_POLYGON_MODE_FRONT, 6);
	if (cso->front_winding == PIPE_WINDING_CCW) {
		so_data(so, nvgl_polygon_mode(cso->fill_ccw));
		so_data(so, nvgl_polygon_mode(cso->fill_cw));
		switch (cso->cull_mode) {
		case PIPE_WINDING_CCW:
			so_data(so, NV34TCL_CULL_FACE_FRONT);
			break;
		case PIPE_WINDING_CW:
			so_data(so, NV34TCL_CULL_FACE_BACK);
			break;
		case PIPE_WINDING_BOTH:
			so_data(so, NV34TCL_CULL_FACE_FRONT_AND_BACK);
			break;
		default:
			so_data(so, NV34TCL_CULL_FACE_BACK);
			break;
		}
		so_data(so, NV34TCL_FRONT_FACE_CCW);
	} else {
		so_data(so, nvgl_polygon_mode(cso->fill_cw));
		so_data(so, nvgl_polygon_mode(cso->fill_ccw));
		switch (cso->cull_mode) {
		case PIPE_WINDING_CCW:
			so_data(so, NV34TCL_CULL_FACE_BACK);
			break;
		case PIPE_WINDING_CW:
			so_data(so, NV34TCL_CULL_FACE_FRONT);
			break;
		case PIPE_WINDING_BOTH:
			so_data(so, NV34TCL_CULL_FACE_FRONT_AND_BACK);
			break;
		default:
			so_data(so, NV34TCL_CULL_FACE_BACK);
			break;
		}
		so_data(so, NV34TCL_FRONT_FACE_CW);
	}
	so_data(so, cso->poly_smooth ? 1 : 0);
	so_data(so, (cso->cull_mode != PIPE_WINDING_NONE) ? 1 : 0);

	so_method(so, rankine, NV34TCL_POLYGON_STIPPLE_ENABLE, 1);
	so_data  (so, cso->poly_stipple_enable ? 1 : 0);

	so_method(so, rankine, NV34TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
	if ((cso->offset_cw && cso->fill_cw == PIPE_POLYGON_MODE_POINT) ||
	    (cso->offset_ccw && cso->fill_ccw == PIPE_POLYGON_MODE_POINT))
		so_data(so, 1);
	else
		so_data(so, 0);
	if ((cso->offset_cw && cso->fill_cw == PIPE_POLYGON_MODE_LINE) ||
	    (cso->offset_ccw && cso->fill_ccw == PIPE_POLYGON_MODE_LINE))
		so_data(so, 1);
	else
		so_data(so, 0);
	if ((cso->offset_cw && cso->fill_cw == PIPE_POLYGON_MODE_FILL) ||
	    (cso->offset_ccw && cso->fill_ccw == PIPE_POLYGON_MODE_FILL))
		so_data(so, 1);
	else
		so_data(so, 0);
	if (cso->offset_cw || cso->offset_ccw) {
		so_method(so, rankine, NV34TCL_POLYGON_OFFSET_FACTOR, 2);
		so_data  (so, fui(cso->offset_scale));
		so_data  (so, fui(cso->offset_units * 2));
	}

	so_method(so, rankine, NV34TCL_POINT_SPRITE, 1);
	if (cso->point_quad_rasterization) {
		unsigned psctl = (1 << 0), i;

		for (i = 0; i < 8; i++) {
			if ((cso->sprite_coord_enable >> i) & 1)
				psctl |= (1 << (8 + i));
		}

		so_data(so, psctl);
	} else {
		so_data(so, 0);
	}

	so_ref(so, &rsso->so);
	so_ref(NULL, &so);
	rsso->pipe = *cso;
	return (void *)rsso;
}

static void
nv30_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->rasterizer = hwcso;
	nv30->dirty |= NV30_NEW_RAST;
	/*nv30->draw_dirty |= NV30_NEW_RAST;*/
}

static void
nv30_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_rasterizer_state *rsso = hwcso;

	so_ref(NULL, &rsso->so);
	FREE(rsso);
}

static void *
nv30_depth_stencil_alpha_state_create(struct pipe_context *pipe,
			const struct pipe_depth_stencil_alpha_state *cso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_zsa_state *zsaso = CALLOC(1, sizeof(*zsaso));
	struct nouveau_stateobj *so = so_new(6, 20, 0);
	struct nouveau_grobj *rankine = nv30->screen->rankine;

	so_method(so, rankine, NV34TCL_DEPTH_FUNC, 3);
	so_data  (so, nvgl_comparison_op(cso->depth.func));
	so_data  (so, cso->depth.writemask ? 1 : 0);
	so_data  (so, cso->depth.enabled ? 1 : 0);

	so_method(so, rankine, NV34TCL_ALPHA_FUNC_ENABLE, 3);
	so_data  (so, cso->alpha.enabled ? 1 : 0);
	so_data  (so, nvgl_comparison_op(cso->alpha.func));
	so_data  (so, float_to_ubyte(cso->alpha.ref_value));

	if (cso->stencil[0].enabled) {
		so_method(so, rankine, NV34TCL_STENCIL_FRONT_ENABLE, 3);
		so_data  (so, cso->stencil[0].enabled ? 1 : 0);
		so_data  (so, cso->stencil[0].writemask);
		so_data  (so, nvgl_comparison_op(cso->stencil[0].func));
		so_method(so, rankine, NV34TCL_STENCIL_FRONT_FUNC_MASK, 4);
		so_data  (so, cso->stencil[0].valuemask);
		so_data  (so, nvgl_stencil_op(cso->stencil[0].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
	} else {
		so_method(so, rankine, NV34TCL_STENCIL_FRONT_ENABLE, 1);
		so_data  (so, 0);
	}

	if (cso->stencil[1].enabled) {
		so_method(so, rankine, NV34TCL_STENCIL_BACK_ENABLE, 3);
		so_data  (so, cso->stencil[1].enabled ? 1 : 0);
		so_data  (so, cso->stencil[1].writemask);
		so_data  (so, nvgl_comparison_op(cso->stencil[1].func));
		so_method(so, rankine, NV34TCL_STENCIL_BACK_FUNC_MASK, 4);
		so_data  (so, cso->stencil[1].valuemask);
		so_data  (so, nvgl_stencil_op(cso->stencil[1].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zpass_op));
	} else {
		so_method(so, rankine, NV34TCL_STENCIL_BACK_ENABLE, 1);
		so_data  (so, 0);
	}

	so_ref(so, &zsaso->so);
	so_ref(NULL, &so);
	zsaso->pipe = *cso;
	return (void *)zsaso;
}

static void
nv30_depth_stencil_alpha_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->zsa = hwcso;
	nv30->dirty |= NV30_NEW_ZSA;
}

static void
nv30_depth_stencil_alpha_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_zsa_state *zsaso = hwcso;

	so_ref(NULL, &zsaso->so);
	FREE(zsaso);
}

static void *
nv30_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	/*struct nv30_context *nv30 = nv30_context(pipe);*/
	struct nv30_vertex_program *vp;

	vp = CALLOC(1, sizeof(struct nv30_vertex_program));
	vp->pipe.tokens = tgsi_dup_tokens(cso->tokens);
	/*vp->draw = draw_create_vertex_shader(nv30->draw, &vp->pipe);*/

	return (void *)vp;
}

static void
nv30_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->vertprog = hwcso;
	nv30->dirty |= NV30_NEW_VERTPROG;
	/*nv30->draw_dirty |= NV30_NEW_VERTPROG;*/
}

static void
nv30_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_vertex_program *vp = hwcso;

	/*draw_delete_vertex_shader(nv30->draw, vp->draw);*/
	nv30_vertprog_destroy(nv30, vp);
	FREE((void*)vp->pipe.tokens);
	FREE(vp);
}

static void *
nv30_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv30_fragment_program *fp;

	fp = CALLOC(1, sizeof(struct nv30_fragment_program));
	fp->pipe.tokens = tgsi_dup_tokens(cso->tokens);

	tgsi_scan_shader(fp->pipe.tokens, &fp->info);

	return (void *)fp;
}

static void
nv30_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->fragprog = hwcso;
	nv30->dirty |= NV30_NEW_FRAGPROG;
}

static void
nv30_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_fragment_program *fp = hwcso;

	nv30_fragprog_destroy(nv30, fp);
	FREE((void*)fp->pipe.tokens);
	FREE(fp);
}

static void
nv30_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->blend_colour = *bcol;
	nv30->dirty |= NV30_NEW_BCOL;
}

static void
nv30_set_stencil_ref(struct pipe_context *pipe,
		     const struct pipe_stencil_ref *sr)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->stencil_ref = *sr;
	nv30->dirty |= NV30_NEW_SR;
}

static void
nv30_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
}

static void
nv30_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 struct pipe_buffer *buf )
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->constbuf[shader] = buf;
	nv30->constbuf_nr[shader] = buf->size / (4 * sizeof(float));

	if (shader == PIPE_SHADER_VERTEX) {
		nv30->dirty |= NV30_NEW_VERTPROG;
	} else
	if (shader == PIPE_SHADER_FRAGMENT) {
		nv30->dirty |= NV30_NEW_FRAGPROG;
	}
}

static void
nv30_set_framebuffer_state(struct pipe_context *pipe,
			   const struct pipe_framebuffer_state *fb)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->framebuffer = *fb;
	nv30->dirty |= NV30_NEW_FB;
}

static void
nv30_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	memcpy(nv30->stipple, stipple->stipple, 4 * 32);
	nv30->dirty |= NV30_NEW_STIPPLE;
}

static void
nv30_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->scissor = *s;
	nv30->dirty |= NV30_NEW_SCISSOR;
}

static void
nv30_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	nv30->viewport = *vpt;
	nv30->dirty |= NV30_NEW_VIEWPORT;
	/*nv30->draw_dirty |= NV30_NEW_VIEWPORT;*/
}

static void
nv30_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
			const struct pipe_vertex_buffer *vb)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	memcpy(nv30->vtxbuf, vb, sizeof(*vb) * count);
	nv30->vtxbuf_nr = count;

	nv30->dirty |= NV30_NEW_ARRAYS;
	/*nv30->draw_dirty |= NV30_NEW_ARRAYS;*/
}

static void
nv30_set_vertex_elements(struct pipe_context *pipe, unsigned count,
			 const struct pipe_vertex_element *ve)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	memcpy(nv30->vtxelt, ve, sizeof(*ve) * count);
	nv30->vtxelt_nr = count;

	nv30->dirty |= NV30_NEW_ARRAYS;
	/*nv30->draw_dirty |= NV30_NEW_ARRAYS;*/
}

void
nv30_init_state_functions(struct nv30_context *nv30)
{
	nv30->pipe.create_blend_state = nv30_blend_state_create;
	nv30->pipe.bind_blend_state = nv30_blend_state_bind;
	nv30->pipe.delete_blend_state = nv30_blend_state_delete;

	nv30->pipe.create_sampler_state = nv30_sampler_state_create;
	nv30->pipe.bind_fragment_sampler_states = nv30_sampler_state_bind;
	nv30->pipe.delete_sampler_state = nv30_sampler_state_delete;
	nv30->pipe.set_fragment_sampler_textures = nv30_set_sampler_texture;

	nv30->pipe.create_rasterizer_state = nv30_rasterizer_state_create;
	nv30->pipe.bind_rasterizer_state = nv30_rasterizer_state_bind;
	nv30->pipe.delete_rasterizer_state = nv30_rasterizer_state_delete;

	nv30->pipe.create_depth_stencil_alpha_state =
		nv30_depth_stencil_alpha_state_create;
	nv30->pipe.bind_depth_stencil_alpha_state =
		nv30_depth_stencil_alpha_state_bind;
	nv30->pipe.delete_depth_stencil_alpha_state =
		nv30_depth_stencil_alpha_state_delete;

	nv30->pipe.create_vs_state = nv30_vp_state_create;
	nv30->pipe.bind_vs_state = nv30_vp_state_bind;
	nv30->pipe.delete_vs_state = nv30_vp_state_delete;

	nv30->pipe.create_fs_state = nv30_fp_state_create;
	nv30->pipe.bind_fs_state = nv30_fp_state_bind;
	nv30->pipe.delete_fs_state = nv30_fp_state_delete;

	nv30->pipe.set_blend_color = nv30_set_blend_color;
        nv30->pipe.set_stencil_ref = nv30_set_stencil_ref;
	nv30->pipe.set_clip_state = nv30_set_clip_state;
	nv30->pipe.set_constant_buffer = nv30_set_constant_buffer;
	nv30->pipe.set_framebuffer_state = nv30_set_framebuffer_state;
	nv30->pipe.set_polygon_stipple = nv30_set_polygon_stipple;
	nv30->pipe.set_scissor_state = nv30_set_scissor_state;
	nv30->pipe.set_viewport_state = nv30_set_viewport_state;

	nv30->pipe.set_vertex_buffers = nv30_set_vertex_buffers;
	nv30->pipe.set_vertex_elements = nv30_set_vertex_elements;
}

