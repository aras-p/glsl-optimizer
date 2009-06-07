#include "draw/draw_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_inlines.h"

#include "tgsi/tgsi_parse.h"

#include "nv10_context.h"
#include "nv10_state.h"

static void *
nv10_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nv10_blend_state *cb;

	cb = MALLOC(sizeof(struct nv10_blend_state));

	cb->b_enable = cso->blend_enable ? 1 : 0;
	cb->b_srcfunc = ((nvgl_blend_func(cso->alpha_src_factor)<<16) |
			 (nvgl_blend_func(cso->rgb_src_factor)));
	cb->b_dstfunc = ((nvgl_blend_func(cso->alpha_dst_factor)<<16) |
			 (nvgl_blend_func(cso->rgb_dst_factor)));

	cb->c_mask = (((cso->colormask & PIPE_MASK_A) ? (0x01<<24) : 0) |
		      ((cso->colormask & PIPE_MASK_R) ? (0x01<<16) : 0) |
		      ((cso->colormask & PIPE_MASK_G) ? (0x01<< 8) : 0) |
		      ((cso->colormask & PIPE_MASK_B) ? (0x01<< 0) : 0));

	cb->d_enable = cso->dither ? 1 : 0;

	return (void *)cb;
}

static void
nv10_blend_state_bind(struct pipe_context *pipe, void *blend)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	nv10->blend = (struct nv10_blend_state*)blend;

	nv10->dirty |= NV10_NEW_BLEND;
}

static void
nv10_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}


static INLINE unsigned
wrap_mode(unsigned wrap) {
	unsigned ret;

	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		ret = NV10TCL_TX_FORMAT_WRAP_S_REPEAT;
		break;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		ret = NV10TCL_TX_FORMAT_WRAP_S_MIRRORED_REPEAT;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		ret = NV10TCL_TX_FORMAT_WRAP_S_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		ret = NV10TCL_TX_FORMAT_WRAP_S_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_CLAMP:
		ret = NV10TCL_TX_FORMAT_WRAP_S_CLAMP;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
	default:
		NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
		ret = NV10TCL_TX_FORMAT_WRAP_S_REPEAT;
		break;
	}

	return ret >> NV10TCL_TX_FORMAT_WRAP_S_SHIFT;
}

static void *
nv10_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	struct nv10_sampler_state *ps;
	uint32_t filter = 0;

	ps = MALLOC(sizeof(struct nv10_sampler_state));

	ps->wrap = ((wrap_mode(cso->wrap_s) << NV10TCL_TX_FORMAT_WRAP_S_SHIFT) |
		    (wrap_mode(cso->wrap_t) << NV10TCL_TX_FORMAT_WRAP_T_SHIFT));

	ps->en = 0;
	if (cso->max_anisotropy > 1.0) {
		/* no idea, binary driver sets it, works without it.. meh.. */
		ps->wrap |= (1 << 5);

/*		if (cso->max_anisotropy >= 16.0) {
			ps->en |= NV10TCL_TX_ENABLE_ANISO_16X;
		} else
		if (cso->max_anisotropy >= 12.0) {
			ps->en |= NV10TCL_TX_ENABLE_ANISO_12X;
		} else
		if (cso->max_anisotropy >= 10.0) {
			ps->en |= NV10TCL_TX_ENABLE_ANISO_10X;
		} else
		if (cso->max_anisotropy >= 8.0) {
			ps->en |= NV10TCL_TX_ENABLE_ANISO_8X;
		} else
		if (cso->max_anisotropy >= 6.0) {
			ps->en |= NV10TCL_TX_ENABLE_ANISO_6X;
		} else
		if (cso->max_anisotropy >= 4.0) {
			ps->en |= NV10TCL_TX_ENABLE_ANISO_4X;
		} else {
			ps->en |= NV10TCL_TX_ENABLE_ANISO_2X;
		}*/
	}

	switch (cso->mag_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		filter |= NV10TCL_TX_FILTER_MAGNIFY_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		filter |= NV10TCL_TX_FILTER_MAGNIFY_NEAREST;
		break;
	}

	switch (cso->min_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV10TCL_TX_FILTER_MINIFY_LINEAR_MIPMAP_NEAREST;
			break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV10TCL_TX_FILTER_MINIFY_LINEAR_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV10TCL_TX_FILTER_MINIFY_LINEAR;
			break;
		}
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV10TCL_TX_FILTER_MINIFY_NEAREST_MIPMAP_NEAREST;
		break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV10TCL_TX_FILTER_MINIFY_NEAREST_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV10TCL_TX_FILTER_MINIFY_NEAREST;
			break;
		}
		break;
	}

	ps->filt = filter;

/*	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
		switch (cso->compare_func) {
		case PIPE_FUNC_NEVER:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_NEVER;
			break;
		case PIPE_FUNC_GREATER:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_GREATER;
			break;
		case PIPE_FUNC_EQUAL:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_EQUAL;
			break;
		case PIPE_FUNC_GEQUAL:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_GEQUAL;
			break;
		case PIPE_FUNC_LESS:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_LESS;
			break;
		case PIPE_FUNC_NOTEQUAL:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_NOTEQUAL;
			break;
		case PIPE_FUNC_LEQUAL:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_LEQUAL;
			break;
		case PIPE_FUNC_ALWAYS:
			ps->wrap |= NV10TCL_TX_WRAP_RCOMP_ALWAYS;
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
nv10_sampler_state_bind(struct pipe_context *pipe, unsigned nr, void **sampler)
{
	struct nv10_context *nv10 = nv10_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nv10->tex_sampler[unit] = sampler[unit];
		nv10->dirty_samplers |= (1 << unit);
	}
}

static void
nv10_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void
nv10_set_sampler_texture(struct pipe_context *pipe, unsigned nr,
			 struct pipe_texture **miptree)
{
	struct nv10_context *nv10 = nv10_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nv10->tex_miptree[unit] = (struct nv10_miptree *)miptree[unit];
		nv10->dirty_samplers |= (1 << unit);
	}
}

static void *
nv10_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nv10_rasterizer_state *rs;
	int i;

	/*XXX: ignored:
	 * 	light_twoside
	 * 	offset_cw/ccw -nohw
	 * 	scissor
	 * 	point_smooth -nohw
	 * 	multisample
	 * 	offset_units / offset_scale
	 */
	rs = MALLOC(sizeof(struct nv10_rasterizer_state));

	rs->templ = cso;
	
	rs->shade_model = cso->flatshade ? 0x1d00 : 0x1d01;

	rs->line_width = (unsigned char)(cso->line_width * 8.0) & 0xff;
	rs->line_smooth_en = cso->line_smooth ? 1 : 0;

	rs->point_size = *(uint32_t*)&cso->point_size;

	rs->poly_smooth_en = cso->poly_smooth ? 1 : 0;

	if (cso->front_winding == PIPE_WINDING_CCW) {
		rs->front_face = NV10TCL_FRONT_FACE_CCW;
		rs->poly_mode_front = nvgl_polygon_mode(cso->fill_ccw);
		rs->poly_mode_back  = nvgl_polygon_mode(cso->fill_cw);
	} else {
		rs->front_face = NV10TCL_FRONT_FACE_CW;
		rs->poly_mode_front = nvgl_polygon_mode(cso->fill_cw);
		rs->poly_mode_back  = nvgl_polygon_mode(cso->fill_ccw);
	}

	switch (cso->cull_mode) {
	case PIPE_WINDING_CCW:
		rs->cull_face_en = 1;
		if (cso->front_winding == PIPE_WINDING_CCW)
			rs->cull_face    = NV10TCL_CULL_FACE_FRONT;
		else
			rs->cull_face    = NV10TCL_CULL_FACE_BACK;
		break;
	case PIPE_WINDING_CW:
		rs->cull_face_en = 1;
		if (cso->front_winding == PIPE_WINDING_CW)
			rs->cull_face    = NV10TCL_CULL_FACE_FRONT;
		else
			rs->cull_face    = NV10TCL_CULL_FACE_BACK;
		break;
	case PIPE_WINDING_BOTH:
		rs->cull_face_en = 1;
		rs->cull_face    = NV10TCL_CULL_FACE_FRONT_AND_BACK;
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
nv10_rasterizer_state_bind(struct pipe_context *pipe, void *rast)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	nv10->rast = (struct nv10_rasterizer_state*)rast;

	draw_set_rasterizer_state(nv10->draw, (nv10->rast ? nv10->rast->templ : NULL));

	nv10->dirty |= NV10_NEW_RAST;
}

static void
nv10_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void *
nv10_depth_stencil_alpha_state_create(struct pipe_context *pipe,
			const struct pipe_depth_stencil_alpha_state *cso)
{
	struct nv10_depth_stencil_alpha_state *hw;

	hw = MALLOC(sizeof(struct nv10_depth_stencil_alpha_state));

	hw->depth.func		= nvgl_comparison_op(cso->depth.func);
	hw->depth.write_enable	= cso->depth.writemask ? 1 : 0;
	hw->depth.test_enable	= cso->depth.enabled ? 1 : 0;

	hw->stencil.enable = cso->stencil[0].enabled ? 1 : 0;
	hw->stencil.wmask = cso->stencil[0].writemask;
	hw->stencil.func = nvgl_comparison_op(cso->stencil[0].func);
	hw->stencil.ref	= cso->stencil[0].ref_value;
	hw->stencil.vmask = cso->stencil[0].valuemask;
	hw->stencil.fail = nvgl_stencil_op(cso->stencil[0].fail_op);
	hw->stencil.zfail = nvgl_stencil_op(cso->stencil[0].zfail_op);
	hw->stencil.zpass = nvgl_stencil_op(cso->stencil[0].zpass_op);

	hw->alpha.enabled = cso->alpha.enabled ? 1 : 0;
	hw->alpha.func = nvgl_comparison_op(cso->alpha.func);
	hw->alpha.ref  = float_to_ubyte(cso->alpha.ref_value);

	return (void *)hw;
}

static void
nv10_depth_stencil_alpha_state_bind(struct pipe_context *pipe, void *dsa)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	nv10->dsa = (struct nv10_depth_stencil_alpha_state*)dsa;

	nv10->dirty |= NV10_NEW_DSA;
}

static void
nv10_depth_stencil_alpha_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void *
nv10_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *templ)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	return draw_create_vertex_shader(nv10->draw, templ);
}

static void
nv10_vp_state_bind(struct pipe_context *pipe, void *shader)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	draw_bind_vertex_shader(nv10->draw, (struct draw_vertex_shader *) shader);

	nv10->dirty |= NV10_NEW_VERTPROG;
}

static void
nv10_vp_state_delete(struct pipe_context *pipe, void *shader)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	draw_delete_vertex_shader(nv10->draw, (struct draw_vertex_shader *) shader);
}

static void *
nv10_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv10_fragment_program *fp;

	fp = CALLOC(1, sizeof(struct nv10_fragment_program));
	fp->pipe.tokens = tgsi_dup_tokens(cso->tokens);
	
	tgsi_scan_shader(cso->tokens, &fp->info);

	return (void *)fp;
}

static void
nv10_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv10_context *nv10 = nv10_context(pipe);
	struct nv10_fragment_program *fp = hwcso;

	nv10->fragprog.current = fp;
	nv10->dirty |= NV10_NEW_FRAGPROG;
}

static void
nv10_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv10_context *nv10 = nv10_context(pipe);
	struct nv10_fragment_program *fp = hwcso;

	nv10_fragprog_destroy(nv10, fp);
	FREE((void*)fp->pipe.tokens);
	FREE(fp);
}

static void
nv10_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	nv10->blend_color = (struct pipe_blend_color*)bcol;

	nv10->dirty |= NV10_NEW_BLENDCOL;
}

static void
nv10_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	draw_set_clip_state(nv10->draw, clip);
}

static void
nv10_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 const struct pipe_constant_buffer *buf )
{
	struct nv10_context *nv10 = nv10_context(pipe);
	struct pipe_screen *pscreen = pipe->screen;

	assert(shader < PIPE_SHADER_TYPES);
	assert(index == 0);

	if (buf) {
		void *mapped;
		if (buf->buffer && buf->buffer->size &&
                    (mapped = pipe_buffer_map(pscreen, buf->buffer, PIPE_BUFFER_USAGE_CPU_READ)))
		{
			memcpy(nv10->constbuf[shader], mapped, buf->buffer->size);
			nv10->constbuf_nr[shader] =
				buf->buffer->size / (4 * sizeof(float));
			pipe_buffer_unmap(pscreen, buf->buffer);
		}
	}
}

static void
nv10_set_framebuffer_state(struct pipe_context *pipe,
			   const struct pipe_framebuffer_state *fb)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	nv10->framebuffer = (struct pipe_framebuffer_state*)fb;

	nv10->dirty |= NV10_NEW_FRAMEBUFFER;
}

static void
nv10_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
	NOUVEAU_ERR("line stipple hahaha\n");
}

static void
nv10_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	nv10->scissor = (struct pipe_scissor_state*)s;

	nv10->dirty |= NV10_NEW_SCISSOR;
}

static void
nv10_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	nv10->viewport = (struct pipe_viewport_state*)vpt;

	draw_set_viewport_state(nv10->draw, nv10->viewport);

	nv10->dirty |= NV10_NEW_VIEWPORT;
}

static void
nv10_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
			const struct pipe_vertex_buffer *vb)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	memcpy(nv10->vtxbuf, vb, sizeof(*vb) * count);
	nv10->dirty |= NV10_NEW_VTXARRAYS;

	draw_set_vertex_buffers(nv10->draw, count, vb);
}

static void
nv10_set_vertex_elements(struct pipe_context *pipe, unsigned count,
			 const struct pipe_vertex_element *ve)
{
	struct nv10_context *nv10 = nv10_context(pipe);

	memcpy(nv10->vtxelt, ve, sizeof(*ve) * count);
	nv10->dirty |= NV10_NEW_VTXARRAYS;

	draw_set_vertex_elements(nv10->draw, count, ve);
}

void
nv10_init_state_functions(struct nv10_context *nv10)
{
	nv10->pipe.create_blend_state = nv10_blend_state_create;
	nv10->pipe.bind_blend_state = nv10_blend_state_bind;
	nv10->pipe.delete_blend_state = nv10_blend_state_delete;

	nv10->pipe.create_sampler_state = nv10_sampler_state_create;
	nv10->pipe.bind_sampler_states = nv10_sampler_state_bind;
	nv10->pipe.delete_sampler_state = nv10_sampler_state_delete;
	nv10->pipe.set_sampler_textures = nv10_set_sampler_texture;

	nv10->pipe.create_rasterizer_state = nv10_rasterizer_state_create;
	nv10->pipe.bind_rasterizer_state = nv10_rasterizer_state_bind;
	nv10->pipe.delete_rasterizer_state = nv10_rasterizer_state_delete;

	nv10->pipe.create_depth_stencil_alpha_state =
		nv10_depth_stencil_alpha_state_create;
	nv10->pipe.bind_depth_stencil_alpha_state =
		nv10_depth_stencil_alpha_state_bind;
	nv10->pipe.delete_depth_stencil_alpha_state =
		nv10_depth_stencil_alpha_state_delete;

	nv10->pipe.create_vs_state = nv10_vp_state_create;
	nv10->pipe.bind_vs_state = nv10_vp_state_bind;
	nv10->pipe.delete_vs_state = nv10_vp_state_delete;

	nv10->pipe.create_fs_state = nv10_fp_state_create;
	nv10->pipe.bind_fs_state = nv10_fp_state_bind;
	nv10->pipe.delete_fs_state = nv10_fp_state_delete;

	nv10->pipe.set_blend_color = nv10_set_blend_color;
	nv10->pipe.set_clip_state = nv10_set_clip_state;
	nv10->pipe.set_constant_buffer = nv10_set_constant_buffer;
	nv10->pipe.set_framebuffer_state = nv10_set_framebuffer_state;
	nv10->pipe.set_polygon_stipple = nv10_set_polygon_stipple;
	nv10->pipe.set_scissor_state = nv10_set_scissor_state;
	nv10->pipe.set_viewport_state = nv10_set_viewport_state;

	nv10->pipe.set_vertex_buffers = nv10_set_vertex_buffers;
	nv10->pipe.set_vertex_elements = nv10_set_vertex_elements;
}

