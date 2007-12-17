#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv40_context.h"
#include "nv40_dma.h"
#include "nv40_state.h"

static void *
nv40_alpha_test_state_create(struct pipe_context *pipe,
			     const struct pipe_alpha_test_state *cso)
{
	struct nv40_alpha_test_state *at;

	at = malloc(sizeof(struct nv40_alpha_test_state));

	at->enabled = cso->enabled ? 1 : 0;
	at->func = nvgl_comparison_op(cso->func);
	at->ref  = float_to_ubyte(cso->ref);

	return (void *)at;
}

static void
nv40_alpha_test_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_alpha_test_state *at = hwcso;

	BEGIN_RING(curie, NV40TCL_ALPHA_TEST_ENABLE, 3);
	OUT_RING  (at->enabled);
	OUT_RING  (at->func);
	OUT_RING  (at->ref);
}

static void
nv40_alpha_test_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void *
nv40_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nv40_blend_state *cb;

	cb = malloc(sizeof(struct nv40_blend_state));

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
nv40_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_blend_state *cb = hwcso;

	BEGIN_RING(curie, NV40TCL_DITHER_ENABLE, 1);
	OUT_RING  (cb->d_enable);

	BEGIN_RING(curie, NV40TCL_BLEND_ENABLE, 3);
	OUT_RING  (cb->b_enable);
	OUT_RING  (cb->b_srcfunc);
	OUT_RING  (cb->b_dstfunc);
	BEGIN_RING(curie, NV40TCL_BLEND_EQUATION, 1);
	OUT_RING  (cb->b_eqn);

	BEGIN_RING(curie, NV40TCL_COLOR_MASK, 1);
	OUT_RING  (cb->c_mask);

	BEGIN_RING(curie, NV40TCL_COLOR_LOGIC_OP_ENABLE, 2);
	OUT_RING  (cb->l_enable);
	OUT_RING  (cb->l_op);
}

static void
nv40_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
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

	ps = malloc(sizeof(struct nv40_sampler_state));

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
			ps->en |= (7 << 4);
		} else
		if (cso->max_anisotropy >= 12.0) {
			ps->en |= (6 << 4);
		} else
		if (cso->max_anisotropy >= 10.0) {
			ps->en |= (5 << 4);
		} else
		if (cso->max_anisotropy >= 8.0) {
			ps->en |= (4 << 4);
		} else
		if (cso->max_anisotropy >= 6.0) {
			ps->en |= (3 << 4);
		} else
		if (cso->max_anisotropy >= 4.0) {
			ps->en |= (2 << 4);
		} else {
			ps->en |= (1 << 4); /* 2.0 */
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

	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
		switch (cso->compare_func) {
		case PIPE_FUNC_NEVER:
			ps->wrap |= (0x0 << 28);
			break;
		case PIPE_FUNC_GREATER:
			ps->wrap |= (0x1 << 28);
			break;
		case PIPE_FUNC_EQUAL:
			ps->wrap |= (0x2 << 28);
			break;
		case PIPE_FUNC_GEQUAL:
			ps->wrap |= (0x3 << 28);
			break;
		case PIPE_FUNC_LESS:
			ps->wrap |= (0x4 << 28);
			break;
		case PIPE_FUNC_NOTEQUAL:
			ps->wrap |= (0x5 << 28);
			break;
		case PIPE_FUNC_LEQUAL:
			ps->wrap |= (0x6 << 28);
			break;
		case PIPE_FUNC_ALWAYS:
			ps->wrap |= (0x7 << 28);
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
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_sampler_state *ps = hwcso;

	nv40->tex_sampler[unit]  = ps;
	nv40->tex_dirty         |= (1 << unit);

	nv40->dirty |= NV40_NEW_TEXTURE;
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
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	nv40->tex_miptree[unit]  = miptree;
	nv40->tex_dirty         |= (1 << unit);

	nv40->dirty |= NV40_NEW_TEXTURE;
}

static void *
nv40_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nv40_rasterizer_state *rs;
	int i;

	/*XXX: ignored:
	 * 	light_twoside
	 * 	offset_cw/ccw -nohw
	 * 	scissor
	 * 	point_smooth -nohw
	 * 	multisample
	 * 	offset_units / offset_scale
	 */
	rs = malloc(sizeof(struct nv40_rasterizer_state));

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
		rs->front_face = NV40TCL_FRONT_FACE_CCW;
		rs->poly_mode_front = nvgl_polygon_mode(cso->fill_ccw);
		rs->poly_mode_back  = nvgl_polygon_mode(cso->fill_cw);
	} else {
		rs->front_face = NV40TCL_FRONT_FACE_CW;
		rs->poly_mode_front = nvgl_polygon_mode(cso->fill_cw);
		rs->poly_mode_back  = nvgl_polygon_mode(cso->fill_ccw);
	}

	switch (cso->cull_mode) {
	case PIPE_WINDING_CCW:
		rs->cull_face_en = 1;
		if (cso->front_winding == PIPE_WINDING_CCW)
			rs->cull_face    = NV40TCL_CULL_FACE_FRONT;
		else
			rs->cull_face    = NV40TCL_CULL_FACE_BACK;
		break;
	case PIPE_WINDING_CW:
		rs->cull_face_en = 1;
		if (cso->front_winding == PIPE_WINDING_CW)
			rs->cull_face    = NV40TCL_CULL_FACE_FRONT;
		else
			rs->cull_face    = NV40TCL_CULL_FACE_BACK;
		break;
	case PIPE_WINDING_BOTH:
		rs->cull_face_en = 1;
		rs->cull_face    = NV40TCL_CULL_FACE_FRONT_AND_BACK;
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
nv40_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_rasterizer_state *rs = hwcso;

	BEGIN_RING(curie, NV40TCL_SHADE_MODEL, 1);
	OUT_RING  (rs->shade_model);

	BEGIN_RING(curie, NV40TCL_LINE_WIDTH, 2);
	OUT_RING  (rs->line_width);
	OUT_RING  (rs->line_smooth_en);
	BEGIN_RING(curie, NV40TCL_LINE_STIPPLE_ENABLE, 2);
	OUT_RING  (rs->line_stipple_en);
	OUT_RING  (rs->line_stipple);

	BEGIN_RING(curie, NV40TCL_POINT_SIZE, 1);
	OUT_RING  (rs->point_size);

	BEGIN_RING(curie, NV40TCL_POLYGON_MODE_FRONT, 6);
	OUT_RING  (rs->poly_mode_front);
	OUT_RING  (rs->poly_mode_back);
	OUT_RING  (rs->cull_face);
	OUT_RING  (rs->front_face);
	OUT_RING  (rs->poly_smooth_en);
	OUT_RING  (rs->cull_face_en);

	BEGIN_RING(curie, NV40TCL_POLYGON_STIPPLE_ENABLE, 1);
	OUT_RING  (rs->poly_stipple_en);

	BEGIN_RING(curie, NV40TCL_POINT_SPRITE, 1);
	OUT_RING  (rs->point_sprite);
}

static void
nv40_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void *
nv40_depth_stencil_state_create(struct pipe_context *pipe,
				const struct pipe_depth_stencil_state *cso)
{
	struct nv40_depth_stencil_state *zs;

	/*XXX: ignored:
	 * 	depth.occlusion_count
	 * 	depth.clear
	 * 	stencil.clear_value
	 */
	zs = malloc(sizeof(struct nv40_depth_stencil_state));

	zs->depth.func		= nvgl_comparison_op(cso->depth.func);
	zs->depth.write_enable	= cso->depth.writemask ? 1 : 0;
	zs->depth.test_enable	= cso->depth.enabled ? 1 : 0;

	zs->stencil.back.enable	= cso->stencil.back_enabled ? 1 : 0;
	zs->stencil.back.wmask	= cso->stencil.write_mask[1];
	zs->stencil.back.func	=
		nvgl_comparison_op(cso->stencil.back_func);
	zs->stencil.back.ref	= cso->stencil.ref_value[1];
	zs->stencil.back.vmask	= cso->stencil.value_mask[1];
	zs->stencil.back.fail	= nvgl_stencil_op(cso->stencil.back_fail_op);
	zs->stencil.back.zfail	= nvgl_stencil_op(cso->stencil.back_zfail_op);
	zs->stencil.back.zpass	= nvgl_stencil_op(cso->stencil.back_zpass_op);

	zs->stencil.front.enable= cso->stencil.front_enabled ? 1 : 0;
	zs->stencil.front.wmask	= cso->stencil.write_mask[0];
	zs->stencil.front.func	=
		nvgl_comparison_op(cso->stencil.front_func);
	zs->stencil.front.ref	= cso->stencil.ref_value[0];
	zs->stencil.front.vmask	= cso->stencil.value_mask[0];
	zs->stencil.front.fail	= nvgl_stencil_op(cso->stencil.front_fail_op);
	zs->stencil.front.zfail	= nvgl_stencil_op(cso->stencil.front_zfail_op);
	zs->stencil.front.zpass	= nvgl_stencil_op(cso->stencil.front_zpass_op);

	return (void *)zs;
}

static void
nv40_depth_stencil_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_depth_stencil_state *zs = hwcso;

	BEGIN_RING(curie, NV40TCL_DEPTH_FUNC, 3);
	OUT_RINGp ((uint32_t *)&zs->depth, 3);
	BEGIN_RING(curie, NV40TCL_STENCIL_BACK_ENABLE, 16);
	OUT_RINGp ((uint32_t *)&zs->stencil.back, 8);
	OUT_RINGp ((uint32_t *)&zs->stencil.front, 8);
}

static void
nv40_depth_stencil_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void *
nv40_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv40_vertex_program *vp;

	vp = calloc(1, sizeof(struct nv40_vertex_program));
	vp->pipe = cso;

	return (void *)vp;
}

static void
nv40_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_vertex_program *vp = hwcso;

	nv40->vertprog.current = vp;
	nv40->dirty |= NV40_NEW_VERTPROG;
}

static void
nv40_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_vertex_program *vp = hwcso;

	nv40_vertprog_destroy(nv40, vp);
	free(vp);
}

static void *
nv40_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv40_fragment_program *fp;

	fp = calloc(1, sizeof(struct nv40_fragment_program));
	fp->pipe = cso;

	return (void *)fp;
}

static void
nv40_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_fragment_program *fp = hwcso;

	nv40->fragprog.current = fp;
	nv40->dirty |= NV40_NEW_FRAGPROG;
}

static void
nv40_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_fragment_program *fp = hwcso;

	nv40_fragprog_destroy(nv40, fp);
	free(fp);
}

static void
nv40_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	BEGIN_RING(curie, NV40TCL_BLEND_COLOR, 1);
	OUT_RING  ((float_to_ubyte(bcol->color[3]) << 24) |
		   (float_to_ubyte(bcol->color[0]) << 16) |
		   (float_to_ubyte(bcol->color[1]) <<  8) |
		   (float_to_ubyte(bcol->color[2]) <<  0));
}

static void
nv40_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	
	nv40->dirty |= NV40_NEW_VERTPROG;
}

static void
nv40_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 const struct pipe_constant_buffer *buf )
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

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
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct pipe_surface *rt[4], *zeta;
	uint32_t rt_enable, rt_format, w, h;
	int i, colour_format = 0, zeta_format = 0;

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

	if (fb->zbuf) {
		if (colour_format) {
			assert(w == fb->zbuf->width);
			assert(h == fb->zbuf->height);
		} else {
			w = fb->zbuf->width;
			h = fb->zbuf->height;
		}

		zeta_format = fb->zbuf->format;
		zeta = fb->zbuf;
	}

	if (fb->sbuf) {
		if (colour_format) {
			assert(w == fb->sbuf->width);
			assert(h == fb->sbuf->height);
		} else {
			w = fb->zbuf->width;
			h = fb->zbuf->height;
		}
		
		if (zeta_format) {
			assert(fb->sbuf->format == zeta_format);
			assert(fb->sbuf == zeta);
		} else {
			zeta_format = fb->sbuf->format;
			zeta = fb->sbuf;
		}
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
		BEGIN_RING(curie, NV40TCL_DMA_COLOR0, 1);
		OUT_RELOCo(rt[0]->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR0_PITCH, 2);
		OUT_RING  (rt[0]->pitch * rt[0]->cpp);
		OUT_RELOCl(rt[0]->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR1) {
		BEGIN_RING(curie, NV40TCL_DMA_COLOR1, 1);
		OUT_RELOCo(rt[1]->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR1_OFFSET, 2);
		OUT_RELOCl(rt[1]->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		OUT_RING  (rt[1]->pitch * rt[1]->cpp);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR2) {
		BEGIN_RING(curie, NV40TCL_DMA_COLOR2, 1);
		OUT_RELOCo(rt[2]->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR2_OFFSET, 1);
		OUT_RELOCl(rt[2]->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR2_PITCH, 1);
		OUT_RING  (rt[2]->pitch * rt[2]->cpp);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR3) {
		BEGIN_RING(curie, NV40TCL_DMA_COLOR3, 1);
		OUT_RELOCo(rt[3]->buffer, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR3_OFFSET, 1);
		OUT_RELOCl(rt[3]->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR3_PITCH, 1);
		OUT_RING  (rt[3]->pitch * rt[3]->cpp);
	}

	if (zeta_format) {
		BEGIN_RING(curie, NV40TCL_DMA_ZETA, 1);
		OUT_RELOCo(zeta->buffer,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_WR | NOUVEAU_BO_RD);
		BEGIN_RING(curie, NV40TCL_ZETA_OFFSET, 1);
		OUT_RELOCl(zeta->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR |
			   NOUVEAU_BO_RD);
		BEGIN_RING(curie, NV40TCL_ZETA_PITCH, 1);
		OUT_RING  (zeta->pitch * zeta->cpp);
	}

	BEGIN_RING(curie, NV40TCL_RT_ENABLE, 1);
	OUT_RING  (rt_enable);
	BEGIN_RING(curie, NV40TCL_RT_HORIZ, 3);
	OUT_RING  ((w << 16) | 0);
	OUT_RING  ((h << 16) | 0);
	OUT_RING  (rt_format);
	BEGIN_RING(curie, NV40TCL_VIEWPORT_HORIZ, 2);
	OUT_RING  ((w << 16) | 0);
	OUT_RING  ((h << 16) | 0);
	BEGIN_RING(curie, NV40TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	OUT_RING  (((w - 1) << 16) | 0);
	OUT_RING  (((h - 1) << 16) | 0);
}

static void
nv40_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	BEGIN_RING(curie, NV40TCL_POLYGON_STIPPLE_PATTERN(0), 32);
	OUT_RINGp ((uint32_t *)stipple->stipple, 32);
}

static void
nv40_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	BEGIN_RING(curie, NV40TCL_SCISSOR_HORIZ, 2);
	OUT_RING  (((s->maxx - s->minx) << 16) | s->minx);
	OUT_RING  (((s->maxy - s->miny) << 16) | s->miny);
}

static void
nv40_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	BEGIN_RING(curie, NV40TCL_VIEWPORT_TRANSLATE_X, 8);
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
nv40_set_vertex_buffer(struct pipe_context *pipe, unsigned index,
		       const struct pipe_vertex_buffer *vb)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	nv40->vtxbuf[index] = *vb;

	nv40->dirty |= NV40_NEW_ARRAYS;
}

static void
nv40_set_vertex_element(struct pipe_context *pipe, unsigned index,
			const struct pipe_vertex_element *ve)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	nv40->vtxelt[index] = *ve;

	nv40->dirty |= NV40_NEW_ARRAYS;
}

void
nv40_init_state_functions(struct nv40_context *nv40)
{
	nv40->pipe.create_alpha_test_state = nv40_alpha_test_state_create;
	nv40->pipe.bind_alpha_test_state = nv40_alpha_test_state_bind;
	nv40->pipe.delete_alpha_test_state = nv40_alpha_test_state_delete;

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

	nv40->pipe.create_depth_stencil_state = nv40_depth_stencil_state_create;
	nv40->pipe.bind_depth_stencil_state = nv40_depth_stencil_state_bind;
	nv40->pipe.delete_depth_stencil_state = nv40_depth_stencil_state_delete;

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

