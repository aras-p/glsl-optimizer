#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "draw/draw_context.h"

#include "tgsi/tgsi_parse.h"

#include "nvfx_context.h"
#include "nvfx_state.h"
#include "nvfx_tex.h"

static void *
nvfx_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_blend_state *bso = CALLOC(1, sizeof(*bso));
	struct nouveau_statebuf_builder sb = sb_init(bso->sb);

	if (cso->rt[0].blend_enable) {
		sb_method(sb, NV34TCL_BLEND_FUNC_ENABLE, 3);
		sb_data(sb, 1);
		sb_data(sb, (nvgl_blend_func(cso->rt[0].alpha_src_factor) << 16) |
			       nvgl_blend_func(cso->rt[0].rgb_src_factor));
		sb_data(sb, nvgl_blend_func(cso->rt[0].alpha_dst_factor) << 16 |
			      nvgl_blend_func(cso->rt[0].rgb_dst_factor));
		if(nvfx->screen->base.device->chipset < 0x40) {
			sb_method(sb, NV34TCL_BLEND_EQUATION, 1);
			sb_data(sb, nvgl_blend_eqn(cso->rt[0].rgb_func));
		} else {
			sb_method(sb, NV40TCL_BLEND_EQUATION, 1);
			sb_data(sb, nvgl_blend_eqn(cso->rt[0].alpha_func) << 16 |
			      nvgl_blend_eqn(cso->rt[0].rgb_func));
		}
	} else {
		sb_method(sb, NV34TCL_BLEND_FUNC_ENABLE, 1);
		sb_data(sb, 0);
	}

	sb_method(sb, NV34TCL_COLOR_MASK, 1);
	sb_data(sb, (((cso->rt[0].colormask & PIPE_MASK_A) ? (0x01 << 24) : 0) |
	       ((cso->rt[0].colormask & PIPE_MASK_R) ? (0x01 << 16) : 0) |
	       ((cso->rt[0].colormask & PIPE_MASK_G) ? (0x01 <<  8) : 0) |
	       ((cso->rt[0].colormask & PIPE_MASK_B) ? (0x01 <<  0) : 0)));

	/* TODO: add NV40 MRT color mask */

	if (cso->logicop_enable) {
		sb_method(sb, NV34TCL_COLOR_LOGIC_OP_ENABLE, 2);
		sb_data(sb, 1);
		sb_data(sb, nvgl_logicop_func(cso->logicop_func));
	} else {
		sb_method(sb, NV34TCL_COLOR_LOGIC_OP_ENABLE, 1);
		sb_data(sb, 0);
	}

	sb_method(sb, NV34TCL_DITHER_ENABLE, 1);
	sb_data(sb, cso->dither ? 1 : 0);

	bso->sb_len = sb_len(sb, bso->sb);
	bso->pipe = *cso;
	return (void *)bso;
}

static void
nvfx_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->blend = hwcso;
	nvfx->dirty |= NVFX_NEW_BLEND;
}

static void
nvfx_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_blend_state *bso = hwcso;

	FREE(bso);
}

static void *
nvfx_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_sampler_state *ps;

	ps = MALLOC(sizeof(struct nvfx_sampler_state));

	/* on nv30, we use this as an internal flag */
	ps->fmt = cso->normalized_coords ? 0 : NV40TCL_TEX_FORMAT_RECT;
	ps->en = 0;
	ps->filt = nvfx_tex_filter(cso);
	ps->wrap = (nvfx_tex_wrap_mode(cso->wrap_s) << NV34TCL_TX_WRAP_S_SHIFT) |
		    (nvfx_tex_wrap_mode(cso->wrap_t) << NV34TCL_TX_WRAP_T_SHIFT) |
		    (nvfx_tex_wrap_mode(cso->wrap_r) << NV34TCL_TX_WRAP_R_SHIFT) |
		    nvfx_tex_wrap_compare_mode(cso);
	ps->bcol = nvfx_tex_border_color(cso->border_color);

	if(nvfx->is_nv4x)
		nv40_sampler_state_init(pipe, ps, cso);
	else
		nv30_sampler_state_init(pipe, ps, cso);

	return (void *)ps;
}

static void
nvfx_sampler_state_bind(struct pipe_context *pipe, unsigned nr, void **sampler)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nvfx->tex_sampler[unit] = sampler[unit];
		nvfx->dirty_samplers |= (1 << unit);
	}

	for (unit = nr; unit < nvfx->nr_samplers; unit++) {
		nvfx->tex_sampler[unit] = NULL;
		nvfx->dirty_samplers |= (1 << unit);
	}

	nvfx->nr_samplers = nr;
	nvfx->dirty |= NVFX_NEW_SAMPLER;
}

static void
nvfx_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void
nvfx_set_fragment_sampler_views(struct pipe_context *pipe,
				unsigned nr,
				struct pipe_sampler_view **views)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		pipe_sampler_view_reference(&nvfx->fragment_sampler_views[unit],
                                            views[unit]);
		nvfx->dirty_samplers |= (1 << unit);
	}

	for (unit = nr; unit < nvfx->nr_textures; unit++) {
		pipe_sampler_view_reference(&nvfx->fragment_sampler_views[unit],
                                            NULL);
		nvfx->dirty_samplers |= (1 << unit);
	}

	nvfx->nr_textures = nr;
	nvfx->dirty |= NVFX_NEW_SAMPLER;
}


static struct pipe_sampler_view *
nvfx_create_sampler_view(struct pipe_context *pipe,
			 struct pipe_resource *texture,
			 const struct pipe_sampler_view *templ)
{
	struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);

	if (view) {
		*view = *templ;
		view->reference.count = 1;
		view->texture = NULL;
		pipe_resource_reference(&view->texture, texture);
		view->context = pipe;
	}

	return view;
}


static void
nvfx_sampler_view_destroy(struct pipe_context *pipe,
			  struct pipe_sampler_view *view)
{
	pipe_resource_reference(&view->texture, NULL);
	FREE(view);
}

static void *
nvfx_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nvfx_rasterizer_state *rsso = CALLOC(1, sizeof(*rsso));
	struct nouveau_statebuf_builder sb = sb_init(rsso->sb);

	/*XXX: ignored:
	 * 	point_smooth -nohw
	 * 	multisample
	 */

	sb_method(sb, NV34TCL_SHADE_MODEL, 1);
	sb_data(sb, cso->flatshade ? NV34TCL_SHADE_MODEL_FLAT :
				       NV34TCL_SHADE_MODEL_SMOOTH);

	sb_method(sb, NV34TCL_VERTEX_TWO_SIDE_ENABLE, 1);
	sb_data(sb, cso->light_twoside);

	sb_method(sb, NV34TCL_LINE_WIDTH, 2);
	sb_data(sb, (unsigned char)(cso->line_width * 8.0) & 0xff);
	sb_data(sb, cso->line_smooth ? 1 : 0);
	sb_method(sb, NV34TCL_LINE_STIPPLE_ENABLE, 2);
	sb_data(sb, cso->line_stipple_enable ? 1 : 0);
	sb_data(sb, (cso->line_stipple_pattern << 16) |
		       cso->line_stipple_factor);

	sb_method(sb, NV34TCL_POINT_SIZE, 1);
	sb_data(sb, fui(cso->point_size));

	sb_method(sb, NV34TCL_POLYGON_MODE_FRONT, 6);
        sb_data(sb, nvgl_polygon_mode(cso->fill_front));
        sb_data(sb, nvgl_polygon_mode(cso->fill_back));
	switch (cso->cull_face) {
	case PIPE_FACE_FRONT:
		sb_data(sb, NV34TCL_CULL_FACE_FRONT);
		break;
	case PIPE_FACE_BACK:
		sb_data(sb, NV34TCL_CULL_FACE_BACK);
		break;
	case PIPE_FACE_FRONT_AND_BACK:
		sb_data(sb, NV34TCL_CULL_FACE_FRONT_AND_BACK);
		break;
	default:
		sb_data(sb, NV34TCL_CULL_FACE_BACK);
		break;
	}
	if (cso->front_ccw) {
		sb_data(sb, NV34TCL_FRONT_FACE_CCW);
	} else {
		sb_data(sb, NV34TCL_FRONT_FACE_CW);
	}
	sb_data(sb, cso->poly_smooth ? 1 : 0);
	sb_data(sb, (cso->cull_face != PIPE_FACE_NONE) ? 1 : 0);

	sb_method(sb, NV34TCL_POLYGON_STIPPLE_ENABLE, 1);
	sb_data(sb, cso->poly_stipple_enable ? 1 : 0);

	sb_method(sb, NV34TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
        sb_data(sb, cso->offset_point);
        sb_data(sb, cso->offset_line);
        sb_data(sb, cso->offset_tri);

	if (cso->offset_point || cso->offset_line || cso->offset_tri) {
		sb_method(sb, NV34TCL_POLYGON_OFFSET_FACTOR, 2);
		sb_data(sb, fui(cso->offset_scale));
		sb_data(sb, fui(cso->offset_units * 2));
	}

	sb_method(sb, NV34TCL_POINT_SPRITE, 1);
	if (cso->point_quad_rasterization) {
		unsigned psctl = (1 << 0), i;

		for (i = 0; i < 8; i++) {
			if ((cso->sprite_coord_enable >> i) & 1)
				psctl |= (1 << (8 + i));
		}

		sb_data(sb, psctl);
	} else {
		sb_data(sb, 0);
	}

	rsso->pipe = *cso;
	rsso->sb_len = sb_len(sb, rsso->sb);
	return (void *)rsso;
}

static void
nvfx_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	if(nvfx->rasterizer && hwcso)
	{
		if(!nvfx->rasterizer || ((struct nvfx_rasterizer_state*)hwcso)->pipe.scissor
					!= nvfx->rasterizer->pipe.scissor)
		{
			nvfx->dirty |= NVFX_NEW_SCISSOR;
			nvfx->draw_dirty |= NVFX_NEW_SCISSOR;
		}

		if(((struct nvfx_rasterizer_state*)hwcso)->pipe.poly_stipple_enable
					!= nvfx->rasterizer->pipe.poly_stipple_enable)
		{
			nvfx->dirty |= NVFX_NEW_STIPPLE;
			nvfx->draw_dirty |= NVFX_NEW_STIPPLE;
		}
	}

	nvfx->rasterizer = hwcso;
	nvfx->dirty |= NVFX_NEW_RAST;
	nvfx->draw_dirty |= NVFX_NEW_RAST;
}

static void
nvfx_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_rasterizer_state *rsso = hwcso;

	FREE(rsso);
}

static void *
nvfx_depth_stencil_alpha_state_create(struct pipe_context *pipe,
			const struct pipe_depth_stencil_alpha_state *cso)
{
	struct nvfx_zsa_state *zsaso = CALLOC(1, sizeof(*zsaso));
	struct nouveau_statebuf_builder sb = sb_init(zsaso->sb);

	sb_method(sb, NV34TCL_DEPTH_FUNC, 3);
	sb_data  (sb, nvgl_comparison_op(cso->depth.func));
	sb_data  (sb, cso->depth.writemask ? 1 : 0);
	sb_data  (sb, cso->depth.enabled ? 1 : 0);

	sb_method(sb, NV34TCL_ALPHA_FUNC_ENABLE, 3);
	sb_data  (sb, cso->alpha.enabled ? 1 : 0);
	sb_data  (sb, nvgl_comparison_op(cso->alpha.func));
	sb_data  (sb, float_to_ubyte(cso->alpha.ref_value));

	if (cso->stencil[0].enabled) {
		sb_method(sb, NV34TCL_STENCIL_FRONT_ENABLE, 3);
		sb_data  (sb, cso->stencil[0].enabled ? 1 : 0);
		sb_data  (sb, cso->stencil[0].writemask);
		sb_data  (sb, nvgl_comparison_op(cso->stencil[0].func));
		sb_method(sb, NV34TCL_STENCIL_FRONT_FUNC_MASK, 4);
		sb_data  (sb, cso->stencil[0].valuemask);
		sb_data  (sb, nvgl_stencil_op(cso->stencil[0].fail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[0].zfail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[0].zpass_op));
	} else {
		sb_method(sb, NV34TCL_STENCIL_FRONT_ENABLE, 1);
		sb_data  (sb, 0);
	}

	if (cso->stencil[1].enabled) {
		sb_method(sb, NV34TCL_STENCIL_BACK_ENABLE, 3);
		sb_data  (sb, cso->stencil[1].enabled ? 1 : 0);
		sb_data  (sb, cso->stencil[1].writemask);
		sb_data  (sb, nvgl_comparison_op(cso->stencil[1].func));
		sb_method(sb, NV34TCL_STENCIL_BACK_FUNC_MASK, 4);
		sb_data  (sb, cso->stencil[1].valuemask);
		sb_data  (sb, nvgl_stencil_op(cso->stencil[1].fail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[1].zfail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[1].zpass_op));
	} else {
		sb_method(sb, NV34TCL_STENCIL_BACK_ENABLE, 1);
		sb_data  (sb, 0);
	}

	zsaso->pipe = *cso;
	zsaso->sb_len = sb_len(sb, zsaso->sb);
	return (void *)zsaso;
}

static void
nvfx_depth_stencil_alpha_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->zsa = hwcso;
	nvfx->dirty |= NVFX_NEW_ZSA;
}

static void
nvfx_depth_stencil_alpha_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_zsa_state *zsaso = hwcso;

	FREE(zsaso);
}

static void *
nvfx_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_vertex_program *vp;

	vp = CALLOC(1, sizeof(struct nvfx_vertex_program));
	vp->pipe.tokens = tgsi_dup_tokens(cso->tokens);
	vp->draw = draw_create_vertex_shader(nvfx->draw, &vp->pipe);

	return (void *)vp;
}

static void
nvfx_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->vertprog = hwcso;
	nvfx->dirty |= NVFX_NEW_VERTPROG;
	nvfx->draw_dirty |= NVFX_NEW_VERTPROG;
}

static void
nvfx_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_vertex_program *vp = hwcso;

	draw_delete_vertex_shader(nvfx->draw, vp->draw);
	nvfx_vertprog_destroy(nvfx, vp);
	FREE((void*)vp->pipe.tokens);
	FREE(vp);
}

static void *
nvfx_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nvfx_fragment_program *fp;

	fp = CALLOC(1, sizeof(struct nvfx_fragment_program));
	fp->pipe.tokens = tgsi_dup_tokens(cso->tokens);

	tgsi_scan_shader(fp->pipe.tokens, &fp->info);

	return (void *)fp;
}

static void
nvfx_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->fragprog = hwcso;
	nvfx->dirty |= NVFX_NEW_FRAGPROG;
}

static void
nvfx_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_fragment_program *fp = hwcso;

	nvfx_fragprog_destroy(nvfx, fp);
	FREE((void*)fp->pipe.tokens);
	FREE(fp);
}

static void
nvfx_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->blend_colour = *bcol;
	nvfx->dirty |= NVFX_NEW_BCOL;
}

static void
nvfx_set_stencil_ref(struct pipe_context *pipe,
		     const struct pipe_stencil_ref *sr)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->stencil_ref = *sr;
	nvfx->dirty |= NVFX_NEW_SR;
}

static void
nvfx_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->clip = *clip;
	nvfx->dirty |= NVFX_NEW_UCP;
	nvfx->draw_dirty |= NVFX_NEW_UCP;
}

static void
nvfx_set_sample_mask(struct pipe_context *pipe,
		     unsigned sample_mask)
{
}

static void
nvfx_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 struct pipe_resource *buf )
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->constbuf[shader] = buf;
	nvfx->constbuf_nr[shader] = buf->width0 / (4 * sizeof(float));

	if (shader == PIPE_SHADER_VERTEX) {
		nvfx->dirty |= NVFX_NEW_VERTCONST;
	} else
	if (shader == PIPE_SHADER_FRAGMENT) {
		nvfx->dirty |= NVFX_NEW_FRAGCONST;
	}
}

static void
nvfx_set_framebuffer_state(struct pipe_context *pipe,
			   const struct pipe_framebuffer_state *fb)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->framebuffer = *fb;
	nvfx->dirty |= NVFX_NEW_FB;
}

static void
nvfx_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	memcpy(nvfx->stipple, stipple->stipple, 4 * 32);
	nvfx->dirty |= NVFX_NEW_STIPPLE;
}

static void
nvfx_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->scissor = *s;
	nvfx->dirty |= NVFX_NEW_SCISSOR;
}

static void
nvfx_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->viewport = *vpt;
	nvfx->dirty |= NVFX_NEW_VIEWPORT;
	nvfx->draw_dirty |= NVFX_NEW_VIEWPORT;
}

static void
nvfx_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
			const struct pipe_vertex_buffer *vb)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	memcpy(nvfx->vtxbuf, vb, sizeof(*vb) * count);
	nvfx->vtxbuf_nr = count;

	nvfx->dirty |= NVFX_NEW_ARRAYS;
	nvfx->draw_dirty |= NVFX_NEW_ARRAYS;
}

static void *
nvfx_vtxelts_state_create(struct pipe_context *pipe,
			  unsigned num_elements,
			  const struct pipe_vertex_element *elements)
{
	struct nvfx_vtxelt_state *cso = CALLOC_STRUCT(nvfx_vtxelt_state);

	assert(num_elements < 16); /* not doing fallbacks yet */
	cso->num_elements = num_elements;
	memcpy(cso->pipe, elements, num_elements * sizeof(*elements));

/*	nvfx_vtxelt_construct(cso);*/

	return (void *)cso;
}

static void
nvfx_vtxelts_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void
nvfx_vtxelts_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->vtxelt = hwcso;
	nvfx->dirty |= NVFX_NEW_ARRAYS;
	/*nvfx->draw_dirty |= NVFX_NEW_ARRAYS;*/
}

void
nvfx_init_state_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.create_blend_state = nvfx_blend_state_create;
	nvfx->pipe.bind_blend_state = nvfx_blend_state_bind;
	nvfx->pipe.delete_blend_state = nvfx_blend_state_delete;

	nvfx->pipe.create_sampler_state = nvfx_sampler_state_create;
	nvfx->pipe.bind_fragment_sampler_states = nvfx_sampler_state_bind;
	nvfx->pipe.delete_sampler_state = nvfx_sampler_state_delete;
	nvfx->pipe.set_fragment_sampler_views = nvfx_set_fragment_sampler_views;
        nvfx->pipe.create_sampler_view = nvfx_create_sampler_view;
        nvfx->pipe.sampler_view_destroy = nvfx_sampler_view_destroy;

	nvfx->pipe.create_rasterizer_state = nvfx_rasterizer_state_create;
	nvfx->pipe.bind_rasterizer_state = nvfx_rasterizer_state_bind;
	nvfx->pipe.delete_rasterizer_state = nvfx_rasterizer_state_delete;

	nvfx->pipe.create_depth_stencil_alpha_state =
		nvfx_depth_stencil_alpha_state_create;
	nvfx->pipe.bind_depth_stencil_alpha_state =
		nvfx_depth_stencil_alpha_state_bind;
	nvfx->pipe.delete_depth_stencil_alpha_state =
		nvfx_depth_stencil_alpha_state_delete;

	nvfx->pipe.create_vs_state = nvfx_vp_state_create;
	nvfx->pipe.bind_vs_state = nvfx_vp_state_bind;
	nvfx->pipe.delete_vs_state = nvfx_vp_state_delete;

	nvfx->pipe.create_fs_state = nvfx_fp_state_create;
	nvfx->pipe.bind_fs_state = nvfx_fp_state_bind;
	nvfx->pipe.delete_fs_state = nvfx_fp_state_delete;

	nvfx->pipe.set_blend_color = nvfx_set_blend_color;
        nvfx->pipe.set_stencil_ref = nvfx_set_stencil_ref;
	nvfx->pipe.set_clip_state = nvfx_set_clip_state;
	nvfx->pipe.set_sample_mask = nvfx_set_sample_mask;
	nvfx->pipe.set_constant_buffer = nvfx_set_constant_buffer;
	nvfx->pipe.set_framebuffer_state = nvfx_set_framebuffer_state;
	nvfx->pipe.set_polygon_stipple = nvfx_set_polygon_stipple;
	nvfx->pipe.set_scissor_state = nvfx_set_scissor_state;
	nvfx->pipe.set_viewport_state = nvfx_set_viewport_state;

	nvfx->pipe.create_vertex_elements_state = nvfx_vtxelts_state_create;
	nvfx->pipe.delete_vertex_elements_state = nvfx_vtxelts_state_delete;
	nvfx->pipe.bind_vertex_elements_state = nvfx_vtxelts_state_bind;

	nvfx->pipe.set_vertex_buffers = nvfx_set_vertex_buffers;
}
