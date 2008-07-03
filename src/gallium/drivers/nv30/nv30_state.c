#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv30_context.h"
#include "nv30_state.h"

static void *
nv30_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nv30_blend_state *cb;

	cb = malloc(sizeof(struct nv30_blend_state));

	cb->b_enable = cso->blend_enable ? 1 : 0;
	cb->b_srcfunc = ((nvgl_blend_func(cso->alpha_src_factor)<<16) |
			 (nvgl_blend_func(cso->rgb_src_factor)));
	cb->b_dstfunc = ((nvgl_blend_func(cso->alpha_dst_factor)<<16) |
			 (nvgl_blend_func(cso->rgb_dst_factor)));
	cb->b_eqn = ((nvgl_blend_eqn(cso->alpha_func) << 16) |
		     (nvgl_blend_eqn(cso->rgb_func)));

	cb->l_enable = cso->logicop_enable ? 1 : 0;
	cb->l_op = nvgl_logicop_func(cso->logicop_func);

	cb->c_mask = (((cso->colormask & PIPE_MASK_A) ? (0x01<<24) : 0) |
		      ((cso->colormask & PIPE_MASK_R) ? (0x01<<16) : 0) |
		      ((cso->colormask & PIPE_MASK_G) ? (0x01<< 8) : 0) |
		      ((cso->colormask & PIPE_MASK_B) ? (0x01<< 0) : 0));

	cb->d_enable = cso->dither ? 1 : 0;

	return (void *)cb;
}

static void
nv30_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_blend_state *cb = hwcso;

	if (!hwcso) {
		return;
	}

	BEGIN_RING(rankine, NV34TCL_DITHER_ENABLE, 1);
	OUT_RING  (cb->d_enable);

	BEGIN_RING(rankine, NV34TCL_BLEND_FUNC_ENABLE, 3);
	OUT_RING  (cb->b_enable);
	OUT_RING  (cb->b_srcfunc);
	OUT_RING  (cb->b_dstfunc);
	BEGIN_RING(rankine, NV34TCL_BLEND_EQUATION, 1);
	OUT_RING  (cb->b_eqn);

	BEGIN_RING(rankine, NV34TCL_COLOR_MASK, 1);
	OUT_RING  (cb->c_mask);

	BEGIN_RING(rankine, NV34TCL_COLOR_LOGIC_OP_ENABLE, 2);
	OUT_RING  (cb->l_enable);
	OUT_RING  (cb->l_op);
}

static void
nv30_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
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

	ps = malloc(sizeof(struct nv30_sampler_state));

	ps->fmt = 0;
	if (!cso->normalized_coords)
		ps->fmt |= NV34TCL_TX_FORMAT_RECT;

	ps->wrap = ((wrap_mode(cso->wrap_s) << NV34TCL_TX_WRAP_S_SHIFT) |
		    (wrap_mode(cso->wrap_t) << NV34TCL_TX_WRAP_T_SHIFT) |
		    (wrap_mode(cso->wrap_r) << NV34TCL_TX_WRAP_R_SHIFT));

	ps->en = 0;
	if (cso->max_anisotropy >= 2.0) {
		/* no idea, binary driver sets it, works without it.. meh.. */
		ps->wrap |= (1 << 5);

/*		if (cso->max_anisotropy >= 16.0) {
			ps->en |= NV34TCL_TX_ENABLE_ANISO_16X;
		} else
		if (cso->max_anisotropy >= 12.0) {
			ps->en |= NV34TCL_TX_ENABLE_ANISO_12X;
		} else
		if (cso->max_anisotropy >= 10.0) {
			ps->en |= NV34TCL_TX_ENABLE_ANISO_10X;
		} else
		if (cso->max_anisotropy >= 8.0) {
			ps->en |= NV34TCL_TX_ENABLE_ANISO_8X;
		} else
		if (cso->max_anisotropy >= 6.0) {
			ps->en |= NV34TCL_TX_ENABLE_ANISO_6X;
		} else
		if (cso->max_anisotropy >= 4.0) {
			ps->en |= NV34TCL_TX_ENABLE_ANISO_4X;
		} else {
			ps->en |= NV34TCL_TX_ENABLE_ANISO_2X;
		}*/
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

/*	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
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
	}*/

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

	if (!sampler) {
		return;
	}

	for (unit = 0; unit < nr; unit++) {
		nv30->tex_sampler[unit] = sampler[unit];
		nv30->dirty_samplers |= (1 << unit);
	}
}

static void
nv30_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void
nv30_set_sampler_texture(struct pipe_context *pipe, unsigned nr,
			 struct pipe_texture **miptree)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nv30->tex_miptree[unit] = (struct nv30_miptree *)miptree[unit];
		nv30->dirty_samplers |= (1 << unit);
	}
}

static void *
nv30_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nv30_rasterizer_state *rs;
	int i;

	/*XXX: ignored:
	 * 	light_twoside
	 * 	offset_cw/ccw -nohw
	 * 	scissor
	 * 	point_smooth -nohw
	 * 	multisample
	 * 	offset_units / offset_scale
	 */
	rs = malloc(sizeof(struct nv30_rasterizer_state));

	rs->shade_model = cso->flatshade ? 0x1d00 : 0x1d01;

	rs->line_width = (unsigned char)(cso->line_width * 8.0) & 0xff;
	rs->line_smooth_en = cso->line_smooth ? 1 : 0;
	rs->line_stipple_en = cso->line_stipple_enable ? 1 : 0;
	rs->line_stipple = (cso->line_stipple_pattern << 16) |
			    cso->line_stipple_factor;

	rs->point_size = *(uint32_t*)&cso->point_size;

	rs->poly_smooth_en = cso->poly_smooth ? 1 : 0;
	rs->poly_stipple_en = cso->poly_stipple_enable ? 1 : 0;

	if (cso->front_winding == PIPE_WINDING_CCW) {
		rs->front_face = NV34TCL_FRONT_FACE_CCW;
		rs->poly_mode_front = nvgl_polygon_mode(cso->fill_ccw);
		rs->poly_mode_back  = nvgl_polygon_mode(cso->fill_cw);
	} else {
		rs->front_face = NV34TCL_FRONT_FACE_CW;
		rs->poly_mode_front = nvgl_polygon_mode(cso->fill_cw);
		rs->poly_mode_back  = nvgl_polygon_mode(cso->fill_ccw);
	}

	switch (cso->cull_mode) {
	case PIPE_WINDING_CCW:
		rs->cull_face_en = 1;
		if (cso->front_winding == PIPE_WINDING_CCW)
			rs->cull_face    = NV34TCL_CULL_FACE_FRONT;
		else
			rs->cull_face    = NV34TCL_CULL_FACE_BACK;
		break;
	case PIPE_WINDING_CW:
		rs->cull_face_en = 1;
		if (cso->front_winding == PIPE_WINDING_CW)
			rs->cull_face    = NV34TCL_CULL_FACE_FRONT;
		else
			rs->cull_face    = NV34TCL_CULL_FACE_BACK;
		break;
	case PIPE_WINDING_BOTH:
		rs->cull_face_en = 1;
		rs->cull_face    = NV34TCL_CULL_FACE_FRONT_AND_BACK;
		break;
	case PIPE_WINDING_NONE:
	default:
		rs->cull_face_en = 0;
		rs->cull_face    = 0;
		break;
	}

	if (cso->point_sprite) {
		rs->point_sprite = (1 << 0);
		for (i = 0; i < 8; i++) {
			if (cso->sprite_coord_mode[i] != PIPE_SPRITE_COORD_NONE)
				rs->point_sprite |= (1 << (8 + i));
		}
	} else {
		rs->point_sprite = 0;
	}

	return (void *)rs;
}

static void
nv30_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_rasterizer_state *rs = hwcso;

	if (!hwcso) {
		return;
	}

	BEGIN_RING(rankine, NV34TCL_SHADE_MODEL, 1);
	OUT_RING  (rs->shade_model);

	BEGIN_RING(rankine, NV34TCL_LINE_WIDTH, 2);
	OUT_RING  (rs->line_width);
	OUT_RING  (rs->line_smooth_en);
	BEGIN_RING(rankine, NV34TCL_LINE_STIPPLE_ENABLE, 2);
	OUT_RING  (rs->line_stipple_en);
	OUT_RING  (rs->line_stipple);

	BEGIN_RING(rankine, NV34TCL_POINT_SIZE, 1);
	OUT_RING  (rs->point_size);

	BEGIN_RING(rankine, NV34TCL_POLYGON_MODE_FRONT, 6);
	OUT_RING  (rs->poly_mode_front);
	OUT_RING  (rs->poly_mode_back);
	OUT_RING  (rs->cull_face);
	OUT_RING  (rs->front_face);
	OUT_RING  (rs->poly_smooth_en);
	OUT_RING  (rs->cull_face_en);

	BEGIN_RING(rankine, NV34TCL_POLYGON_STIPPLE_ENABLE, 1);
	OUT_RING  (rs->poly_stipple_en);

	BEGIN_RING(rankine, NV34TCL_POINT_SPRITE, 1);
	OUT_RING  (rs->point_sprite);
}

static void
nv30_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void
nv30_translate_stencil(const struct pipe_depth_stencil_alpha_state *cso,
		       unsigned idx, struct nv30_stencil_push *hw)
{
	hw->enable = cso->stencil[idx].enabled ? 1 : 0;
	hw->wmask = cso->stencil[idx].write_mask;
	hw->func = nvgl_comparison_op(cso->stencil[idx].func);
	hw->ref	= cso->stencil[idx].ref_value;
	hw->vmask = cso->stencil[idx].value_mask;
	hw->fail = nvgl_stencil_op(cso->stencil[idx].fail_op);
	hw->zfail = nvgl_stencil_op(cso->stencil[idx].zfail_op);
	hw->zpass = nvgl_stencil_op(cso->stencil[idx].zpass_op);
}

static void *
nv30_depth_stencil_alpha_state_create(struct pipe_context *pipe,
			const struct pipe_depth_stencil_alpha_state *cso)
{
	struct nv30_depth_stencil_alpha_state *hw;

	hw = malloc(sizeof(struct nv30_depth_stencil_alpha_state));

	hw->depth.func		= nvgl_comparison_op(cso->depth.func);
	hw->depth.write_enable	= cso->depth.writemask ? 1 : 0;
	hw->depth.test_enable	= cso->depth.enabled ? 1 : 0;

	nv30_translate_stencil(cso, 0, &hw->stencil.front);
	nv30_translate_stencil(cso, 1, &hw->stencil.back);

	hw->alpha.enabled = cso->alpha.enabled ? 1 : 0;
	hw->alpha.func = nvgl_comparison_op(cso->alpha.func);
	hw->alpha.ref  = float_to_ubyte(cso->alpha.ref);

	return (void *)hw;
}

static void
nv30_depth_stencil_alpha_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_depth_stencil_alpha_state *hw = hwcso;

	if (!hwcso) {
		return;
	}

	BEGIN_RING(rankine, NV34TCL_DEPTH_FUNC, 3);
	OUT_RINGp ((uint32_t *)&hw->depth, 3);
	BEGIN_RING(rankine, NV34TCL_STENCIL_BACK_ENABLE, 16);
	OUT_RINGp ((uint32_t *)&hw->stencil.back, 8);
	OUT_RINGp ((uint32_t *)&hw->stencil.front, 8);
	BEGIN_RING(rankine, NV34TCL_ALPHA_FUNC_ENABLE, 3);
	OUT_RINGp ((uint32_t *)&hw->alpha.enabled, 3);
}

static void
nv30_depth_stencil_alpha_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void *
nv30_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv30_vertex_program *vp;

	vp = CALLOC(1, sizeof(struct nv30_vertex_program));
	vp->pipe = *cso;

	return (void *)vp;
}

static void
nv30_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_vertex_program *vp = hwcso;

	if (!hwcso) {
		return;
	}

	nv30->vertprog.current = vp;
	nv30->dirty |= NV30_NEW_VERTPROG;
}

static void
nv30_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_vertex_program *vp = hwcso;

	nv30_vertprog_destroy(nv30, vp);
	FREE(vp);
}

static void *
nv30_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv30_fragment_program *fp;

	fp = CALLOC(1, sizeof(struct nv30_fragment_program));
	fp->pipe = *cso;

	return (void *)fp;
}

static void
nv30_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_fragment_program *fp = hwcso;

	if (!hwcso) {
		return;
	}

	nv30->fragprog.current = fp;
	nv30->dirty |= NV30_NEW_FRAGPROG;
}

static void
nv30_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_fragment_program *fp = hwcso;

	nv30_fragprog_destroy(nv30, fp);
	FREE(fp);
}

static void
nv30_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	BEGIN_RING(rankine, NV34TCL_BLEND_COLOR, 1);
	OUT_RING  ((float_to_ubyte(bcol->color[3]) << 24) |
		   (float_to_ubyte(bcol->color[0]) << 16) |
		   (float_to_ubyte(bcol->color[1]) <<  8) |
		   (float_to_ubyte(bcol->color[2]) <<  0));
}

static void
nv30_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
}

static void
nv30_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 const struct pipe_constant_buffer *buf )
{
	struct nv30_context *nv30 = nv30_context(pipe);

	if (shader == PIPE_SHADER_VERTEX) {
		nv30->vertprog.constant_buf = buf->buffer;
		nv30->dirty |= NV30_NEW_VERTPROG;
	} else
	if (shader == PIPE_SHADER_FRAGMENT) {
		nv30->fragprog.constant_buf = buf->buffer;
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

	BEGIN_RING(rankine, NV34TCL_POLYGON_STIPPLE_PATTERN(0), 32);
	OUT_RINGp ((uint32_t *)stipple->stipple, 32);
}

static void
nv30_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	BEGIN_RING(rankine, NV34TCL_SCISSOR_HORIZ, 2);
	OUT_RING  (((s->maxx - s->minx) << 16) | s->minx);
	OUT_RING  (((s->maxy - s->miny) << 16) | s->miny);
}

static void
nv30_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	BEGIN_RING(rankine, NV34TCL_VIEWPORT_TRANSLATE_X, 8);
	OUT_RINGf (vpt->translate[0]);
	OUT_RINGf (vpt->translate[1]);
	OUT_RINGf (vpt->translate[2]);
	OUT_RINGf (vpt->translate[3]);
	OUT_RINGf (vpt->scale[0]);
	OUT_RINGf (vpt->scale[1]);
	OUT_RINGf (vpt->scale[2]);
	OUT_RINGf (vpt->scale[3]);
}

static void
nv30_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
			const struct pipe_vertex_buffer *vb)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	memcpy(nv30->vtxbuf, vb, sizeof(*vb) * count);
	nv30->dirty |= NV30_NEW_ARRAYS;
}

static void
nv30_set_vertex_elements(struct pipe_context *pipe, unsigned count,
			 const struct pipe_vertex_element *ve)
{
	struct nv30_context *nv30 = nv30_context(pipe);

	memcpy(nv30->vtxelt, ve, sizeof(*ve) * count);
	nv30->dirty |= NV30_NEW_ARRAYS;
}

void
nv30_init_state_functions(struct nv30_context *nv30)
{
	nv30->pipe.create_blend_state = nv30_blend_state_create;
	nv30->pipe.bind_blend_state = nv30_blend_state_bind;
	nv30->pipe.delete_blend_state = nv30_blend_state_delete;

	nv30->pipe.create_sampler_state = nv30_sampler_state_create;
	nv30->pipe.bind_sampler_states = nv30_sampler_state_bind;
	nv30->pipe.delete_sampler_state = nv30_sampler_state_delete;
	nv30->pipe.set_sampler_textures = nv30_set_sampler_texture;

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
	nv30->pipe.set_clip_state = nv30_set_clip_state;
	nv30->pipe.set_constant_buffer = nv30_set_constant_buffer;
	nv30->pipe.set_framebuffer_state = nv30_set_framebuffer_state;
	nv30->pipe.set_polygon_stipple = nv30_set_polygon_stipple;
	nv30->pipe.set_scissor_state = nv30_set_scissor_state;
	nv30->pipe.set_viewport_state = nv30_set_viewport_state;

	nv30->pipe.set_vertex_buffers = nv30_set_vertex_buffers;
	nv30->pipe.set_vertex_elements = nv30_set_vertex_elements;
}

