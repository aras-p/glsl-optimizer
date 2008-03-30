#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_state.h"

#include "nouveau/nouveau_stateobj.h"

static void *
nv50_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nouveau_stateobj *so = so_new(64, 0);
	struct nouveau_grobj *tesla = nv50_context(pipe)->screen->tesla;
	struct nv50_blend_stateobj *bso = CALLOC_STRUCT(nv50_blend_stateobj);
	unsigned cmask = 0, i;

	/*XXX ignored:
	 * 	- dither
	 */

	if (cso->blend_enable == 0) {
		so_method(so, tesla, NV50TCL_BLEND_ENABLE(0), 8);
		for (i = 0; i < 8; i++)
			so_data(so, 0);
	} else {
		so_method(so, tesla, NV50TCL_BLEND_ENABLE(0), 8);
		for (i = 0; i < 8; i++)
			so_data(so, 1);
		so_method(so, tesla, NV50TCL_BLEND_EQUATION_RGB, 5);
		so_data  (so, nvgl_blend_eqn(cso->rgb_func));
		so_data  (so, nvgl_blend_func(cso->rgb_src_factor));
		so_data  (so, nvgl_blend_func(cso->rgb_dst_factor));
		so_data  (so, nvgl_blend_eqn(cso->alpha_func));
		so_data  (so, nvgl_blend_func(cso->alpha_src_factor));
		so_method(so, tesla, NV50TCL_BLEND_FUNC_DST_ALPHA, 1);
		so_data  (so, nvgl_blend_func(cso->alpha_dst_factor));
	}

	if (cso->logicop_enable == 0 ) {
		so_method(so, tesla, NV50TCL_LOGIC_OP_ENABLE, 1);
		so_data  (so, 0);
	} else {
		so_method(so, tesla, NV50TCL_LOGIC_OP_ENABLE, 2);
		so_data  (so, 1);
		so_data  (so, nvgl_logicop_func(cso->logicop_func));
	}

	if (cso->colormask & PIPE_MASK_R)
		cmask |= (1 << 0);
	if (cso->colormask & PIPE_MASK_G)
		cmask |= (1 << 4);
	if (cso->colormask & PIPE_MASK_B)
		cmask |= (1 << 8);
	if (cso->colormask & PIPE_MASK_A)
		cmask |= (1 << 12);
	so_method(so, tesla, NV50TCL_COLOR_MASK(0), 8);
	for (i = 0; i < 8; i++)
		so_data(so, cmask);

	bso->pipe = *cso;
	so_ref(so, &bso->so);
	return (void *)bso;
}

static void
nv50_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->blend = hwcso;
	nv50->dirty |= NV50_NEW_BLEND;
}

static void
nv50_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_blend_stateobj *bso = hwcso;

	so_ref(NULL, &bso->so);
	FREE(bso);
}

static void *
nv50_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	return NULL;
}

static void
nv50_sampler_state_bind(struct pipe_context *pipe, unsigned nr, void **sampler)
{
}

static void
nv50_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_set_sampler_texture(struct pipe_context *pipe, unsigned nr,
			 struct pipe_texture **pt)
{
}

static void *
nv50_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nouveau_stateobj *so = so_new(64, 0);
	struct nouveau_grobj *tesla = nv50_context(pipe)->screen->tesla;
	struct nv50_rasterizer_stateobj *rso =
		CALLOC_STRUCT(nv50_rasterizer_stateobj);
	unsigned i;

	/*XXX: ignored
	 * 	- light_twosize
	 * 	- point_smooth
	 * 	- multisample
	 * 	- point_sprite / sprite_coord_mode
	 */

	so_method(so, tesla, NV50TCL_SHADE_MODEL, 1);
	so_data  (so, cso->flatshade ? NV50TCL_SHADE_MODEL_FLAT :
				       NV50TCL_SHADE_MODEL_SMOOTH);

	so_method(so, tesla, NV50TCL_LINE_WIDTH, 1);
	so_data  (so, fui(cso->line_width));
	so_method(so, tesla, NV50TCL_LINE_SMOOTH_ENABLE, 1);
	so_data  (so, cso->line_smooth ? 1 : 0);
	if (cso->line_stipple_enable) {
		so_method(so, tesla, NV50TCL_LINE_STIPPLE_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, tesla, NV50TCL_LINE_STIPPLE_PATTERN, 1);
		so_data  (so, (cso->line_stipple_pattern << 16) |
			       cso->line_stipple_factor);
	} else {
		so_method(so, tesla, NV50TCL_LINE_STIPPLE_ENABLE, 1);
		so_data  (so, 0);
	}

	so_method(so, tesla, NV50TCL_POINT_SIZE, 1);
	so_data  (so, fui(cso->point_size));

	so_method(so, tesla, NV50TCL_POLYGON_MODE_FRONT, 3);
	if (cso->front_winding == PIPE_WINDING_CCW) {
		so_data(so, nvgl_polygon_mode(cso->fill_ccw));
		so_data(so, nvgl_polygon_mode(cso->fill_cw));
	} else {
		so_data(so, nvgl_polygon_mode(cso->fill_cw));
		so_data(so, nvgl_polygon_mode(cso->fill_ccw));
	}
	so_data(so, cso->poly_smooth ? 1 : 0);

	so_method(so, tesla, NV50TCL_CULL_FACE_ENABLE, 3);
	so_data  (so, cso->cull_mode != PIPE_WINDING_NONE);
	if (cso->front_winding == PIPE_WINDING_CCW) {
		so_data(so, NV50TCL_FRONT_FACE_CCW);
		switch (cso->cull_mode) {
		case PIPE_WINDING_CCW:
			so_data(so, NV50TCL_CULL_FACE_FRONT);
			break;
		case PIPE_WINDING_CW:
			so_data(so, NV50TCL_CULL_FACE_BACK);
			break;
		case PIPE_WINDING_BOTH:
			so_data(so, NV50TCL_CULL_FACE_FRONT_AND_BACK);
			break;
		default:
			so_data(so, NV50TCL_CULL_FACE_BACK);
			break;
		}
	} else {
		so_data(so, NV50TCL_FRONT_FACE_CW);
		switch (cso->cull_mode) {
		case PIPE_WINDING_CCW:
			so_data(so, NV50TCL_CULL_FACE_BACK);
			break;
		case PIPE_WINDING_CW:
			so_data(so, NV50TCL_CULL_FACE_FRONT);
			break;
		case PIPE_WINDING_BOTH:
			so_data(so, NV50TCL_CULL_FACE_FRONT_AND_BACK);
			break;
		default:
			so_data(so, NV50TCL_CULL_FACE_BACK);
			break;
		}
	}

	so_method(so, tesla, NV50TCL_POLYGON_STIPPLE_ENABLE, 1);
	so_data  (so, cso->poly_stipple_enable ? 1 : 0);

	so_method(so, tesla, NV50TCL_POLYGON_OFFSET_POINT_ENABLE, 3);
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
		so_method(so, tesla, NV50TCL_POLYGON_OFFSET_FACTOR, 1);
		so_data  (so, fui(cso->offset_scale));
		so_method(so, tesla, NV50TCL_POLYGON_OFFSET_UNITS, 1);
		so_data  (so, fui(cso->offset_units * 2));
	}

	rso->pipe = *cso;
	so_ref(so, &rso->so);
	return (void *)rso;
}

static void
nv50_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->rasterizer = hwcso;
	nv50->dirty |= NV50_NEW_RASTERIZER;
}

static void
nv50_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_rasterizer_stateobj *rso = hwcso;

	so_ref(NULL, &rso->so);
	FREE(rso);
}

static void *
nv50_depth_stencil_alpha_state_create(struct pipe_context *pipe,
			const struct pipe_depth_stencil_alpha_state *cso)
{
	struct nouveau_grobj *tesla = nv50_context(pipe)->screen->tesla;
	struct nv50_zsa_stateobj *zsa = CALLOC_STRUCT(nv50_zsa_stateobj);
	struct nouveau_stateobj *so = so_new(64, 0);

	so_method(so, tesla, NV50TCL_DEPTH_WRITE_ENABLE, 1);
	so_data  (so, cso->depth.writemask ? 1 : 0);
	if (cso->depth.enabled) {
		so_method(so, tesla, NV50TCL_DEPTH_TEST_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, tesla, NV50TCL_DEPTH_TEST_FUNC, 1);
		so_data  (so, nvgl_comparison_op(cso->depth.func));
	} else {
		so_method(so, tesla, NV50TCL_DEPTH_TEST_ENABLE, 1);
		so_data  (so, 0);
	}

	if (cso->stencil[0].enabled) {
		so_method(so, tesla, NV50TCL_STENCIL_FRONT_ENABLE, 5);
		so_data  (so, 1);
		so_data  (so, nvgl_stencil_op(cso->stencil[0].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
		so_data  (so, nvgl_comparison_op(cso->stencil[0].func));
		so_method(so, tesla, NV50TCL_STENCIL_FRONT_FUNC_REF, 3);
		so_data  (so, cso->stencil[0].ref_value);
		so_data  (so, cso->stencil[0].write_mask);
		so_data  (so, cso->stencil[0].value_mask);
	} else {
		so_method(so, tesla, NV50TCL_STENCIL_FRONT_ENABLE, 1);
		so_data  (so, 0);
	}

	if (cso->stencil[1].enabled) {
		so_method(so, tesla, NV50TCL_STENCIL_BACK_ENABLE, 8);
		so_data  (so, 1);
		so_data  (so, nvgl_stencil_op(cso->stencil[1].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zpass_op));
		so_data  (so, nvgl_comparison_op(cso->stencil[1].func));
		so_data  (so, cso->stencil[1].ref_value);
		so_data  (so, cso->stencil[1].write_mask);
		so_data  (so, cso->stencil[1].value_mask);
	} else {
		so_method(so, tesla, NV50TCL_STENCIL_BACK_ENABLE, 1);
		so_data  (so, 0);
	}

	if (cso->alpha.enabled) {
		so_method(so, tesla, NV50TCL_ALPHA_TEST_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, tesla, NV50TCL_ALPHA_TEST_REF, 2);
		so_data  (so, fui(cso->alpha.ref));
		so_data  (so, nvgl_comparison_op(cso->alpha.func));
	} else {
		so_method(so, tesla, NV50TCL_ALPHA_TEST_ENABLE, 1);
		so_data  (so, 0);
	}

	zsa->pipe = *cso;
	so_ref(so, &zsa->so);
	return (void *)zsa;
}

static void
nv50_depth_stencil_alpha_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->zsa = hwcso;
	nv50->dirty |= NV50_NEW_ZSA;
}

static void
nv50_depth_stencil_alpha_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_zsa_stateobj *zsa = hwcso;

	so_ref(NULL, &zsa->so);
	FREE(zsa);
}

static void *
nv50_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	return NULL;
}

static void
nv50_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void *
nv50_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	return NULL;
}

static void
nv50_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->blend_colour = *bcol;
	nv50->dirty |= NV50_NEW_BLEND_COLOUR;
}

static void
nv50_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
}

static void
nv50_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 const struct pipe_constant_buffer *buf )
{
}

static void
nv50_set_framebuffer_state(struct pipe_context *pipe,
			   const struct pipe_framebuffer_state *fb)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->framebuffer = *fb;
	nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

static void
nv50_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->stipple = *stipple;
	nv50->dirty |= NV50_NEW_STIPPLE;
}

static void
nv50_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->scissor = *s;
	nv50->dirty |= NV50_NEW_SCISSOR;
}

static void
nv50_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->viewport = *vpt;
	nv50->dirty |= NV50_NEW_VIEWPORT;
}

static void
nv50_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
			const struct pipe_vertex_buffer *vb)
{
}

static void
nv50_set_vertex_elements(struct pipe_context *pipe, unsigned count,
			 const struct pipe_vertex_element *ve)
{
}

void
nv50_init_state_functions(struct nv50_context *nv50)
{
	nv50->pipe.create_blend_state = nv50_blend_state_create;
	nv50->pipe.bind_blend_state = nv50_blend_state_bind;
	nv50->pipe.delete_blend_state = nv50_blend_state_delete;

	nv50->pipe.create_sampler_state = nv50_sampler_state_create;
	nv50->pipe.bind_sampler_states = nv50_sampler_state_bind;
	nv50->pipe.delete_sampler_state = nv50_sampler_state_delete;
	nv50->pipe.set_sampler_textures = nv50_set_sampler_texture;

	nv50->pipe.create_rasterizer_state = nv50_rasterizer_state_create;
	nv50->pipe.bind_rasterizer_state = nv50_rasterizer_state_bind;
	nv50->pipe.delete_rasterizer_state = nv50_rasterizer_state_delete;

	nv50->pipe.create_depth_stencil_alpha_state =
		nv50_depth_stencil_alpha_state_create;
	nv50->pipe.bind_depth_stencil_alpha_state =
		nv50_depth_stencil_alpha_state_bind;
	nv50->pipe.delete_depth_stencil_alpha_state =
		nv50_depth_stencil_alpha_state_delete;

	nv50->pipe.create_vs_state = nv50_vp_state_create;
	nv50->pipe.bind_vs_state = nv50_vp_state_bind;
	nv50->pipe.delete_vs_state = nv50_vp_state_delete;

	nv50->pipe.create_fs_state = nv50_fp_state_create;
	nv50->pipe.bind_fs_state = nv50_fp_state_bind;
	nv50->pipe.delete_fs_state = nv50_fp_state_delete;

	nv50->pipe.set_blend_color = nv50_set_blend_color;
	nv50->pipe.set_clip_state = nv50_set_clip_state;
	nv50->pipe.set_constant_buffer = nv50_set_constant_buffer;
	nv50->pipe.set_framebuffer_state = nv50_set_framebuffer_state;
	nv50->pipe.set_polygon_stipple = nv50_set_polygon_stipple;
	nv50->pipe.set_scissor_state = nv50_set_scissor_state;
	nv50->pipe.set_viewport_state = nv50_set_viewport_state;

	nv50->pipe.set_vertex_buffers = nv50_set_vertex_buffers;
	nv50->pipe.set_vertex_elements = nv50_set_vertex_elements;
}

