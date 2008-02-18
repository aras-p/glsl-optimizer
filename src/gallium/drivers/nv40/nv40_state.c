#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv40_context.h"
#include "nv40_state.h"

static void *
nv40_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nouveau_grobj *curie = nv40->hw->curie;
	struct nouveau_stateobj *so = so_new(16, 0);

	if (cso->blend_enable) {
		so_method(so, curie, NV40TCL_BLEND_ENABLE, 3);
		so_data  (so, 1);
		so_data  (so, (nvgl_blend_func(cso->alpha_src_factor) << 16) |
			       nvgl_blend_func(cso->rgb_src_factor));
		so_data  (so, nvgl_blend_func(cso->alpha_dst_factor) << 16 |
			      nvgl_blend_func(cso->rgb_dst_factor));
		so_method(so, curie, NV40TCL_BLEND_EQUATION, 1);
		so_data  (so, nvgl_blend_eqn(cso->alpha_func) << 16 |
			      nvgl_blend_eqn(cso->rgb_func));
	} else {
		so_method(so, curie, NV40TCL_BLEND_ENABLE, 1);
		so_data  (so, 0);
	}

	so_method(so, curie, NV40TCL_COLOR_MASK, 1);
	so_data  (so, (((cso->colormask & PIPE_MASK_A) ? (0x01 << 24) : 0) |
		       ((cso->colormask & PIPE_MASK_R) ? (0x01 << 16) : 0) |
		       ((cso->colormask & PIPE_MASK_G) ? (0x01 <<  8) : 0) |
		       ((cso->colormask & PIPE_MASK_B) ? (0x01 <<  0) : 0)));

	if (cso->logicop_enable) {
		so_method(so, curie, NV40TCL_COLOR_LOGIC_OP_ENABLE, 2);
		so_data  (so, 1);
		so_data  (so, nvgl_logicop_func(cso->logicop_func));
	} else {
		so_method(so, curie, NV40TCL_COLOR_LOGIC_OP_ENABLE, 1);
		so_data  (so, 0);
	}

	so_method(so, curie, NV40TCL_DITHER_ENABLE, 1);
	so_data  (so, cso->dither ? 1 : 0);

	return (void *)so;
}

static void
nv40_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	so_ref(hwcso, &nv40->so_blend);
	nv40->dirty |= NV40_NEW_BLEND;
}

static void
nv40_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nouveau_stateobj *so = hwcso;

	so_ref(NULL, &so);
}


static INLINE unsigned
wrap_mode(unsigned wrap) {
	unsigned ret;

	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		ret = NV40TCL_TEX_WRAP_S_REPEAT;
		break;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		ret = NV40TCL_TEX_WRAP_S_MIRRORED_REPEAT;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		ret = NV40TCL_TEX_WRAP_S_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		ret = NV40TCL_TEX_WRAP_S_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_CLAMP:
		ret = NV40TCL_TEX_WRAP_S_CLAMP;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		ret = NV40TCL_TEX_WRAP_S_MIRROR_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		ret = NV40TCL_TEX_WRAP_S_MIRROR_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		ret = NV40TCL_TEX_WRAP_S_MIRROR_CLAMP;
		break;
	default:
		NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
		ret = NV40TCL_TEX_WRAP_S_REPEAT;
		break;
	}

	return ret >> NV40TCL_TEX_WRAP_S_SHIFT;
}

static void *
nv40_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	struct nv40_sampler_state *ps;
	uint32_t filter = 0;

	ps = MALLOC(sizeof(struct nv40_sampler_state));

	ps->fmt = 0;
	if (!cso->normalized_coords)
		ps->fmt |= NV40TCL_TEX_FORMAT_RECT;

	ps->wrap = ((wrap_mode(cso->wrap_s) << NV40TCL_TEX_WRAP_S_SHIFT) |
		    (wrap_mode(cso->wrap_t) << NV40TCL_TEX_WRAP_T_SHIFT) |
		    (wrap_mode(cso->wrap_r) << NV40TCL_TEX_WRAP_R_SHIFT));

	ps->en = 0;
	if (cso->max_anisotropy >= 2.0) {
		/* no idea, binary driver sets it, works without it.. meh.. */
		ps->wrap |= (1 << 5);

		if (cso->max_anisotropy >= 16.0) {
			ps->en |= NV40TCL_TEX_ENABLE_ANISO_16X;
		} else
		if (cso->max_anisotropy >= 12.0) {
			ps->en |= NV40TCL_TEX_ENABLE_ANISO_12X;
		} else
		if (cso->max_anisotropy >= 10.0) {
			ps->en |= NV40TCL_TEX_ENABLE_ANISO_10X;
		} else
		if (cso->max_anisotropy >= 8.0) {
			ps->en |= NV40TCL_TEX_ENABLE_ANISO_8X;
		} else
		if (cso->max_anisotropy >= 6.0) {
			ps->en |= NV40TCL_TEX_ENABLE_ANISO_6X;
		} else
		if (cso->max_anisotropy >= 4.0) {
			ps->en |= NV40TCL_TEX_ENABLE_ANISO_4X;
		} else {
			ps->en |= NV40TCL_TEX_ENABLE_ANISO_2X;
		}
	}

	switch (cso->mag_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		filter |= NV40TCL_TEX_FILTER_MAG_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		filter |= NV40TCL_TEX_FILTER_MAG_NEAREST;
		break;
	}

	switch (cso->min_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV40TCL_TEX_FILTER_MIN_LINEAR_MIPMAP_NEAREST;
			break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV40TCL_TEX_FILTER_MIN_LINEAR_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV40TCL_TEX_FILTER_MIN_LINEAR;
			break;
		}
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV40TCL_TEX_FILTER_MIN_NEAREST_MIPMAP_NEAREST;
		break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV40TCL_TEX_FILTER_MIN_NEAREST_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV40TCL_TEX_FILTER_MIN_NEAREST;
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
		ps->en |= (int)(limit * 256.0) << 7;

		limit = CLAMP(cso->min_lod, 0.0, 15.0);
		ps->en |= (int)(limit * 256.0) << 19;
	}


	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
		switch (cso->compare_func) {
		case PIPE_FUNC_NEVER:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_NEVER;
			break;
		case PIPE_FUNC_GREATER:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_GREATER;
			break;
		case PIPE_FUNC_EQUAL:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_EQUAL;
			break;
		case PIPE_FUNC_GEQUAL:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_GEQUAL;
			break;
		case PIPE_FUNC_LESS:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_LESS;
			break;
		case PIPE_FUNC_NOTEQUAL:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_NOTEQUAL;
			break;
		case PIPE_FUNC_LEQUAL:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_LEQUAL;
			break;
		case PIPE_FUNC_ALWAYS:
			ps->wrap |= NV40TCL_TEX_WRAP_RCOMP_ALWAYS;
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
nv40_sampler_state_bind(struct pipe_context *pipe, unsigned unit,
			void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_sampler_state *ps = hwcso;

	nv40->tex_sampler[unit] = ps;
	nv40->dirty_samplers |= (1 << unit);
}

static void
nv40_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void
nv40_set_sampler_texture(struct pipe_context *pipe, unsigned unit,
			 struct pipe_texture *miptree)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	nv40->tex_miptree[unit] = (struct nv40_miptree *)miptree;
	nv40->dirty_samplers |= (1 << unit);
}

static void *
nv40_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_rasterizer_state *rsso = MALLOC(sizeof(*rsso));
	struct nouveau_stateobj *so = so_new(32, 0);

	/*XXX: ignored:
	 * 	light_twoside
	 * 	offset_cw/ccw -nohw
	 * 	point_smooth -nohw
	 * 	multisample
	 * 	offset_units / offset_scale
	 */

	so_method(so, nv40->hw->curie, NV40TCL_SHADE_MODEL, 1);
	so_data  (so, cso->flatshade ? NV40TCL_SHADE_MODEL_FLAT :
				       NV40TCL_SHADE_MODEL_SMOOTH);

	so_method(so, nv40->hw->curie, NV40TCL_LINE_WIDTH, 2);
	so_data  (so, (unsigned char)(cso->line_width * 8.0) & 0xff);
	so_data  (so, cso->line_smooth ? 1 : 0);
	so_method(so, nv40->hw->curie, NV40TCL_LINE_STIPPLE_ENABLE, 2);
	so_data  (so, cso->line_stipple_enable ? 1 : 0);
	so_data  (so, (cso->line_stipple_pattern << 16) |
		       cso->line_stipple_factor);

	so_method(so, nv40->hw->curie, NV40TCL_POINT_SIZE, 1);
	so_data  (so, fui(cso->point_size));

	so_method(so, nv40->hw->curie, NV40TCL_POLYGON_MODE_FRONT, 6);
	if (cso->front_winding == PIPE_WINDING_CCW) {
		so_data(so, nvgl_polygon_mode(cso->fill_ccw));
		so_data(so, nvgl_polygon_mode(cso->fill_cw));
		switch (cso->cull_mode) {
		case PIPE_WINDING_CCW:
			so_data(so, NV40TCL_CULL_FACE_FRONT);
			break;
		case PIPE_WINDING_CW:
			so_data(so, NV40TCL_CULL_FACE_BACK);
			break;
		case PIPE_WINDING_BOTH:
			so_data(so, NV40TCL_CULL_FACE_FRONT_AND_BACK);
			break;
		default:
			so_data(so, 0);
			break;
		}
		so_data(so, NV40TCL_FRONT_FACE_CCW);
	} else {
		so_data(so, nvgl_polygon_mode(cso->fill_cw));
		so_data(so, nvgl_polygon_mode(cso->fill_ccw));
		switch (cso->cull_mode) {
		case PIPE_WINDING_CCW:
			so_data(so, NV40TCL_CULL_FACE_BACK);
			break;
		case PIPE_WINDING_CW:
			so_data(so, NV40TCL_CULL_FACE_FRONT);
			break;
		case PIPE_WINDING_BOTH:
			so_data(so, NV40TCL_CULL_FACE_FRONT_AND_BACK);
			break;
		default:
			so_data(so, 0);
			break;
		}
		so_data(so, NV40TCL_FRONT_FACE_CW);
	}
	so_data(so, cso->poly_smooth ? 1 : 0);
	so_data(so, cso->cull_mode != PIPE_WINDING_NONE ? 1 : 0);

	so_method(so, nv40->hw->curie, NV40TCL_POLYGON_STIPPLE_ENABLE, 1);
	so_data  (so, cso->poly_stipple_enable ? 1 : 0);

	so_method(so, nv40->hw->curie, NV40TCL_POINT_SPRITE, 1);
	if (cso->point_sprite) {
		unsigned psctl = (1 << 0), i;

		for (i = 0; i < 8; i++) {
			if (cso->sprite_coord_mode[i] != PIPE_SPRITE_COORD_NONE)
				psctl |= (1 << (8 + i));
		}

		so_data(so, psctl);
	} else {
		so_data(so, 0);
	}

	rsso->so = so;
	rsso->pipe = *cso;
	return (void *)rsso;
}

static void
nv40_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_rasterizer_state *rsso = hwcso;

	so_ref(rsso->so, &nv40->so_rast);
	nv40->rasterizer = rsso;
	nv40->dirty |= NV40_NEW_RAST;
}

static void
nv40_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_rasterizer_state *rsso = hwcso;

	so_ref(NULL, &rsso->so);
	free(rsso);
}

static void *
nv40_depth_stencil_alpha_state_create(struct pipe_context *pipe,
			const struct pipe_depth_stencil_alpha_state *cso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nouveau_stateobj *so = so_new(32, 0);

	so_method(so, nv40->hw->curie, NV40TCL_DEPTH_FUNC, 3);
	so_data  (so, nvgl_comparison_op(cso->depth.func));
	so_data  (so, cso->depth.writemask ? 1 : 0);
	so_data  (so, cso->depth.enabled ? 1 : 0);

	so_method(so, nv40->hw->curie, NV40TCL_ALPHA_TEST_ENABLE, 3);
	so_data  (so, cso->alpha.enabled ? 1 : 0);
	so_data  (so, nvgl_comparison_op(cso->alpha.func));
	so_data  (so, float_to_ubyte(cso->alpha.ref));

	if (cso->stencil[0].enabled) {
		so_method(so, nv40->hw->curie, NV40TCL_STENCIL_FRONT_ENABLE, 8);
		so_data  (so, cso->stencil[0].enabled ? 1 : 0);
		so_data  (so, cso->stencil[0].write_mask);
		so_data  (so, nvgl_comparison_op(cso->stencil[0].func));
		so_data  (so, cso->stencil[0].ref_value);
		so_data  (so, cso->stencil[0].value_mask);
		so_data  (so, nvgl_stencil_op(cso->stencil[0].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
	} else {
		so_method(so, nv40->hw->curie, NV40TCL_STENCIL_FRONT_ENABLE, 1);
		so_data  (so, 0);
	}

	if (cso->stencil[1].enabled) {
		so_method(so, nv40->hw->curie, NV40TCL_STENCIL_BACK_ENABLE, 8);
		so_data  (so, cso->stencil[1].enabled ? 1 : 0);
		so_data  (so, cso->stencil[1].write_mask);
		so_data  (so, nvgl_comparison_op(cso->stencil[1].func));
		so_data  (so, cso->stencil[1].ref_value);
		so_data  (so, cso->stencil[1].value_mask);
		so_data  (so, nvgl_stencil_op(cso->stencil[1].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zpass_op));
	} else {
		so_method(so, nv40->hw->curie, NV40TCL_STENCIL_BACK_ENABLE, 1);
		so_data  (so, 0);
	}

	return (void *)so;
}

static void
nv40_depth_stencil_alpha_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	so_ref(hwcso, &nv40->so_zsa);
	nv40->dirty |= NV40_NEW_ZSA;
}

static void
nv40_depth_stencil_alpha_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nouveau_stateobj *so = hwcso;

	so_ref(NULL, &so);
}

static void *
nv40_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv40_vertex_program *vp;

	vp = CALLOC(1, sizeof(struct nv40_vertex_program));
	vp->pipe = cso;

	return (void *)vp;
}

static void
nv40_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_vertex_program *vp = hwcso;

	nv40->vertprog.current = vp;
	nv40->dirty |= NV40_NEW_VERTPROG;
}

static void
nv40_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_vertex_program *vp = hwcso;

	nv40_vertprog_destroy(nv40, vp);
	free(vp);
}

static void *
nv40_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv40_fragment_program *fp;

	fp = CALLOC(1, sizeof(struct nv40_fragment_program));
	fp->pipe = cso;

	return (void *)fp;
}

static void
nv40_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_fragment_program *fp = hwcso;

	nv40->fragprog.current = fp;
	nv40->dirty |= NV40_NEW_FRAGPROG;
}

static void
nv40_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_fragment_program *fp = hwcso;

	nv40_fragprog_destroy(nv40, fp);
	free(fp);
}

static void
nv40_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nouveau_stateobj *so = so_new(2, 0);

	so_method(so, nv40->hw->curie, NV40TCL_BLEND_COLOR, 1);
	so_data  (so, ((float_to_ubyte(bcol->color[3]) << 24) |
		       (float_to_ubyte(bcol->color[0]) << 16) |
		       (float_to_ubyte(bcol->color[1]) <<  8) |
		       (float_to_ubyte(bcol->color[2]) <<  0)));

	so_ref(so, &nv40->so_bcol);
	so_ref(NULL, &so);
	nv40->dirty |= NV40_NEW_BCOL;
}

static void
nv40_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
}

static void
nv40_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 const struct pipe_constant_buffer *buf )
{
	struct nv40_context *nv40 = nv40_context(pipe);

	if (shader == PIPE_SHADER_VERTEX) {
		nv40->vertprog.constant_buf = buf->buffer;
		nv40->dirty |= NV40_NEW_VERTPROG;
	} else
	if (shader == PIPE_SHADER_FRAGMENT) {
		nv40->fragprog.constant_buf = buf->buffer;
		nv40->dirty |= NV40_NEW_FRAGPROG;
	}
}

static void
nv40_set_framebuffer_state(struct pipe_context *pipe,
			   const struct pipe_framebuffer_state *fb)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct pipe_surface *rt[4], *zeta;
	uint32_t rt_enable, rt_format, w, h;
	int i, colour_format = 0, zeta_format = 0;
	struct nouveau_stateobj *so = so_new(64, 10);
	unsigned rt_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;

	rt_enable = 0;
	for (i = 0; i < 4; i++) {
		if (!fb->cbufs[i])
			continue;

		if (colour_format) {
			assert(w == fb->cbufs[i]->width);
			assert(h == fb->cbufs[i]->height);
			assert(colour_format == fb->cbufs[i]->format);
		} else {
			w = fb->cbufs[i]->width;
			h = fb->cbufs[i]->height;
			colour_format = fb->cbufs[i]->format;
			rt_enable |= (NV40TCL_RT_ENABLE_COLOR0 << i);
			rt[i] = fb->cbufs[i];
		}
	}

	if (rt_enable & (NV40TCL_RT_ENABLE_COLOR1 | NV40TCL_RT_ENABLE_COLOR2 |
			 NV40TCL_RT_ENABLE_COLOR3))
		rt_enable |= NV40TCL_RT_ENABLE_MRT;

	if (fb->zsbuf) {
		if (colour_format) {
			assert(w == fb->zsbuf->width);
			assert(h == fb->zsbuf->height);
		} else {
			w = fb->zsbuf->width;
			h = fb->zsbuf->height;
		}

		zeta_format = fb->zsbuf->format;
		zeta = fb->zsbuf;
	}

	rt_format = NV40TCL_RT_FORMAT_TYPE_LINEAR;

	switch (colour_format) {
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case 0:
		rt_format |= NV40TCL_RT_FORMAT_COLOR_A8R8G8B8;
		break;
	case PIPE_FORMAT_R5G6B5_UNORM:
		rt_format |= NV40TCL_RT_FORMAT_COLOR_R5G6B5;
		break;
	default:
		assert(0);
	}

	switch (zeta_format) {
	case PIPE_FORMAT_Z16_UNORM:
		rt_format |= NV40TCL_RT_FORMAT_ZETA_Z16;
		break;
	case PIPE_FORMAT_Z24S8_UNORM:
	case 0:
		rt_format |= NV40TCL_RT_FORMAT_ZETA_Z24S8;
		break;
	default:
		assert(0);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR0) {
		so_method(so, nv40->hw->curie, NV40TCL_DMA_COLOR0, 1);
		so_reloc (so, rt[0]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->hw->curie, NV40TCL_COLOR0_PITCH, 2);
		so_data  (so, rt[0]->pitch * rt[0]->cpp);
		so_reloc (so, rt[0]->buffer, rt[0]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR1) {
		so_method(so, nv40->hw->curie, NV40TCL_DMA_COLOR1, 1);
		so_reloc (so, rt[1]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->hw->curie, NV40TCL_COLOR1_OFFSET, 2);
		so_reloc (so, rt[1]->buffer, rt[1]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_data  (so, rt[1]->pitch * rt[1]->cpp);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR2) {
		so_method(so, nv40->hw->curie, NV40TCL_DMA_COLOR2, 1);
		so_reloc (so, rt[2]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->hw->curie, NV40TCL_COLOR2_OFFSET, 1);
		so_reloc (so, rt[2]->buffer, rt[2]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_method(so, nv40->hw->curie, NV40TCL_COLOR2_PITCH, 1);
		so_data  (so, rt[2]->pitch * rt[2]->cpp);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR3) {
		so_method(so, nv40->hw->curie, NV40TCL_DMA_COLOR3, 1);
		so_reloc (so, rt[3]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->hw->curie, NV40TCL_COLOR3_OFFSET, 1);
		so_reloc (so, rt[3]->buffer, rt[3]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_method(so, nv40->hw->curie, NV40TCL_COLOR3_PITCH, 1);
		so_data  (so, rt[3]->pitch * rt[3]->cpp);
	}

	if (zeta_format) {
		so_method(so, nv40->hw->curie, NV40TCL_DMA_ZETA, 1);
		so_reloc (so, zeta->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->hw->curie, NV40TCL_ZETA_OFFSET, 1);
		so_reloc (so, zeta->buffer, zeta->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_method(so, nv40->hw->curie, NV40TCL_ZETA_PITCH, 1);
		so_data  (so, zeta->pitch * zeta->cpp);
	}

	so_method(so, nv40->hw->curie, NV40TCL_RT_ENABLE, 1);
	so_data  (so, rt_enable);
	so_method(so, nv40->hw->curie, NV40TCL_RT_HORIZ, 3);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_data  (so, rt_format);
	so_method(so, nv40->hw->curie, NV40TCL_VIEWPORT_HORIZ, 2);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_method(so, nv40->hw->curie, NV40TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	so_data  (so, ((w - 1) << 16) | 0);
	so_data  (so, ((h - 1) << 16) | 0);

	so_ref(so, &nv40->so_framebuffer);
	so_ref(NULL, &so);
	nv40->dirty |= NV40_NEW_FB;
}

static void
nv40_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	memcpy(nv40->pipe_state.stipple, stipple->stipple, 4 * 32);
	nv40->dirty |= NV40_NEW_STIPPLE;
}

static void
nv40_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	nv40->pipe_state.scissor = *s;
	nv40->dirty |= NV40_NEW_SCISSOR;
}

static void
nv40_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nouveau_stateobj *so = so_new(9, 0);

	so_method(so, nv40->hw->curie, NV40TCL_VIEWPORT_TRANSLATE_X, 8);
	so_data  (so, fui(vpt->translate[0]));
	so_data  (so, fui(vpt->translate[1]));
	so_data  (so, fui(vpt->translate[2]));
	so_data  (so, fui(vpt->translate[3]));
	so_data  (so, fui(vpt->scale[0]));
	so_data  (so, fui(vpt->scale[1]));
	so_data  (so, fui(vpt->scale[2]));
	so_data  (so, fui(vpt->scale[3]));

	so_ref(so, &nv40->so_viewport);
	so_ref(NULL, &so);
	nv40->dirty |= NV40_NEW_VIEWPORT;
}

static void
nv40_set_vertex_buffer(struct pipe_context *pipe, unsigned index,
		       const struct pipe_vertex_buffer *vb)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	nv40->vtxbuf[index] = *vb;

	nv40->dirty |= NV40_NEW_ARRAYS;
}

static void
nv40_set_vertex_element(struct pipe_context *pipe, unsigned index,
			const struct pipe_vertex_element *ve)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	nv40->vtxelt[index] = *ve;

	nv40->dirty |= NV40_NEW_ARRAYS;
}

void
nv40_init_state_functions(struct nv40_context *nv40)
{
	nv40->pipe.create_blend_state = nv40_blend_state_create;
	nv40->pipe.bind_blend_state = nv40_blend_state_bind;
	nv40->pipe.delete_blend_state = nv40_blend_state_delete;

	nv40->pipe.create_sampler_state = nv40_sampler_state_create;
	nv40->pipe.bind_sampler_state = nv40_sampler_state_bind;
	nv40->pipe.delete_sampler_state = nv40_sampler_state_delete;
	nv40->pipe.set_sampler_texture = nv40_set_sampler_texture;

	nv40->pipe.create_rasterizer_state = nv40_rasterizer_state_create;
	nv40->pipe.bind_rasterizer_state = nv40_rasterizer_state_bind;
	nv40->pipe.delete_rasterizer_state = nv40_rasterizer_state_delete;

	nv40->pipe.create_depth_stencil_alpha_state =
		nv40_depth_stencil_alpha_state_create;
	nv40->pipe.bind_depth_stencil_alpha_state =
		nv40_depth_stencil_alpha_state_bind;
	nv40->pipe.delete_depth_stencil_alpha_state =
		nv40_depth_stencil_alpha_state_delete;

	nv40->pipe.create_vs_state = nv40_vp_state_create;
	nv40->pipe.bind_vs_state = nv40_vp_state_bind;
	nv40->pipe.delete_vs_state = nv40_vp_state_delete;

	nv40->pipe.create_fs_state = nv40_fp_state_create;
	nv40->pipe.bind_fs_state = nv40_fp_state_bind;
	nv40->pipe.delete_fs_state = nv40_fp_state_delete;

	nv40->pipe.set_blend_color = nv40_set_blend_color;
	nv40->pipe.set_clip_state = nv40_set_clip_state;
	nv40->pipe.set_constant_buffer = nv40_set_constant_buffer;
	nv40->pipe.set_framebuffer_state = nv40_set_framebuffer_state;
	nv40->pipe.set_polygon_stipple = nv40_set_polygon_stipple;
	nv40->pipe.set_scissor_state = nv40_set_scissor_state;
	nv40->pipe.set_viewport_state = nv40_set_viewport_state;

	nv40->pipe.set_vertex_buffer = nv40_set_vertex_buffer;
	nv40->pipe.set_vertex_element = nv40_set_vertex_element;
}

