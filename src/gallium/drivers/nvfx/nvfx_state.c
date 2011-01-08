#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_framebuffer.h"

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
		sb_method(sb, NV30_3D_BLEND_FUNC_ENABLE, 3);
		sb_data(sb, 1);
		sb_data(sb, (nvgl_blend_func(cso->rt[0].alpha_src_factor) << 16) |
			       nvgl_blend_func(cso->rt[0].rgb_src_factor));
		sb_data(sb, nvgl_blend_func(cso->rt[0].alpha_dst_factor) << 16 |
			      nvgl_blend_func(cso->rt[0].rgb_dst_factor));
		if(nvfx->screen->base.device->chipset < 0x40) {
			sb_method(sb, NV30_3D_BLEND_EQUATION, 1);
			sb_data(sb, nvgl_blend_eqn(cso->rt[0].rgb_func));
		} else {
			sb_method(sb, NV40_3D_BLEND_EQUATION, 1);
			sb_data(sb, nvgl_blend_eqn(cso->rt[0].alpha_func) << 16 |
			      nvgl_blend_eqn(cso->rt[0].rgb_func));
		}
	} else {
		sb_method(sb, NV30_3D_BLEND_FUNC_ENABLE, 1);
		sb_data(sb, 0);
	}

	sb_method(sb, NV30_3D_COLOR_MASK, 1);
	sb_data(sb, (((cso->rt[0].colormask & PIPE_MASK_A) ? (0x01 << 24) : 0) |
	       ((cso->rt[0].colormask & PIPE_MASK_R) ? (0x01 << 16) : 0) |
	       ((cso->rt[0].colormask & PIPE_MASK_G) ? (0x01 <<  8) : 0) |
	       ((cso->rt[0].colormask & PIPE_MASK_B) ? (0x01 <<  0) : 0)));

	/* TODO: add NV40 MRT color mask */

	if (cso->logicop_enable) {
		sb_method(sb, NV30_3D_COLOR_LOGIC_OP_ENABLE, 2);
		sb_data(sb, 1);
		sb_data(sb, nvgl_logicop_func(cso->logicop_func));
	} else {
		sb_method(sb, NV30_3D_COLOR_LOGIC_OP_ENABLE, 1);
		sb_data(sb, 0);
	}

	sb_method(sb, NV30_3D_DITHER_ENABLE, 1);
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
nvfx_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nvfx_rasterizer_state *rsso = CALLOC(1, sizeof(*rsso));
	struct nouveau_statebuf_builder sb = sb_init(rsso->sb);

	/*XXX: ignored:
	 * 	point_smooth -nohw
	 * 	multisample
	 *     sprite_coord_origin
	 */

	sb_method(sb, NV30_3D_SHADE_MODEL, 1);
	sb_data(sb, cso->flatshade ? NV30_3D_SHADE_MODEL_FLAT :
				       NV30_3D_SHADE_MODEL_SMOOTH);

	sb_method(sb, NV30_3D_VERTEX_TWO_SIDE_ENABLE, 1);
	sb_data(sb, cso->light_twoside);

	sb_method(sb, NV30_3D_LINE_WIDTH, 2);
	sb_data(sb, (unsigned char)(cso->line_width * 8.0) & 0xff);
	sb_data(sb, cso->line_smooth ? 1 : 0);
	sb_method(sb, NV30_3D_LINE_STIPPLE_ENABLE, 2);
	sb_data(sb, cso->line_stipple_enable ? 1 : 0);
	sb_data(sb, (cso->line_stipple_pattern << 16) |
		       cso->line_stipple_factor);

	sb_method(sb, NV30_3D_POINT_SIZE, 1);
	sb_data(sb, fui(cso->point_size));

	sb_method(sb, NV30_3D_POLYGON_MODE_FRONT, 6);
        sb_data(sb, nvgl_polygon_mode(cso->fill_front));
        sb_data(sb, nvgl_polygon_mode(cso->fill_back));
	switch (cso->cull_face) {
	case PIPE_FACE_FRONT:
		sb_data(sb, NV30_3D_CULL_FACE_FRONT);
		break;
	case PIPE_FACE_BACK:
		sb_data(sb, NV30_3D_CULL_FACE_BACK);
		break;
	case PIPE_FACE_FRONT_AND_BACK:
		sb_data(sb, NV30_3D_CULL_FACE_FRONT_AND_BACK);
		break;
	default:
		sb_data(sb, NV30_3D_CULL_FACE_BACK);
		break;
	}
	if (cso->front_ccw) {
		sb_data(sb, NV30_3D_FRONT_FACE_CCW);
	} else {
		sb_data(sb, NV30_3D_FRONT_FACE_CW);
	}
	sb_data(sb, cso->poly_smooth ? 1 : 0);
	sb_data(sb, (cso->cull_face != PIPE_FACE_NONE) ? 1 : 0);

	sb_method(sb, NV30_3D_POLYGON_STIPPLE_ENABLE, 1);
	sb_data(sb, cso->poly_stipple_enable ? 1 : 0);

	sb_method(sb, NV30_3D_POLYGON_OFFSET_POINT_ENABLE, 3);
        sb_data(sb, cso->offset_point);
        sb_data(sb, cso->offset_line);
        sb_data(sb, cso->offset_tri);

	if (cso->offset_point || cso->offset_line || cso->offset_tri) {
		sb_method(sb, NV30_3D_POLYGON_OFFSET_FACTOR, 2);
		sb_data(sb, fui(cso->offset_scale));
		sb_data(sb, fui(cso->offset_units * 2));
	}

	sb_method(sb, NV30_3D_FLATSHADE_FIRST, 1);
	sb_data(sb, cso->flatshade_first);

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

		if(((struct nvfx_rasterizer_state*)hwcso)->pipe.point_quad_rasterization != nvfx->rasterizer->pipe.point_quad_rasterization
				|| ((struct nvfx_rasterizer_state*)hwcso)->pipe.sprite_coord_enable != nvfx->rasterizer->pipe.sprite_coord_enable
				|| ((struct nvfx_rasterizer_state*)hwcso)->pipe.sprite_coord_mode != nvfx->rasterizer->pipe.sprite_coord_mode)
		{
			nvfx->dirty |= NVFX_NEW_SPRITE;
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

	sb_method(sb, NV30_3D_DEPTH_FUNC, 1);
	sb_data  (sb, nvgl_comparison_op(cso->depth.func));

	sb_method(sb, NV30_3D_ALPHA_FUNC_ENABLE, 3);
	sb_data  (sb, cso->alpha.enabled ? 1 : 0);
	sb_data  (sb, nvgl_comparison_op(cso->alpha.func));
	sb_data  (sb, float_to_ubyte(cso->alpha.ref_value));

	if (cso->stencil[0].enabled) {
		sb_method(sb, NV30_3D_STENCIL_ENABLE(0), 3);
		sb_data  (sb, cso->stencil[0].enabled ? 1 : 0);
		sb_data  (sb, cso->stencil[0].writemask);
		sb_data  (sb, nvgl_comparison_op(cso->stencil[0].func));
		sb_method(sb, NV30_3D_STENCIL_FUNC_MASK(0), 4);
		sb_data  (sb, cso->stencil[0].valuemask);
		sb_data  (sb, nvgl_stencil_op(cso->stencil[0].fail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[0].zfail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[0].zpass_op));
	} else {
		sb_method(sb, NV30_3D_STENCIL_ENABLE(0), 1);
		sb_data  (sb, 0);
	}

	if (cso->stencil[1].enabled) {
		sb_method(sb, NV30_3D_STENCIL_ENABLE(1), 3);
		sb_data  (sb, cso->stencil[1].enabled ? 1 : 0);
		sb_data  (sb, cso->stencil[1].writemask);
		sb_data  (sb, nvgl_comparison_op(cso->stencil[1].func));
		sb_method(sb, NV30_3D_STENCIL_FUNC_MASK(1), 4);
		sb_data  (sb, cso->stencil[1].valuemask);
		sb_data  (sb, nvgl_stencil_op(cso->stencil[1].fail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[1].zfail_op));
		sb_data  (sb, nvgl_stencil_op(cso->stencil[1].zpass_op));
	} else {
		sb_method(sb, NV30_3D_STENCIL_ENABLE(1), 1);
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

	pipe_resource_reference(&nvfx->constbuf[shader], buf);
	nvfx->constbuf_nr[shader] = buf ? (buf->width0 / (4 * sizeof(float))) : 0;

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

	if(fb)
		util_copy_framebuffer_state(&nvfx->framebuffer, fb);
	else
		util_unreference_framebuffer_state(&nvfx->framebuffer);
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

void
nvfx_init_state_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.create_blend_state = nvfx_blend_state_create;
	nvfx->pipe.bind_blend_state = nvfx_blend_state_bind;
	nvfx->pipe.delete_blend_state = nvfx_blend_state_delete;

	nvfx->pipe.create_rasterizer_state = nvfx_rasterizer_state_create;
	nvfx->pipe.bind_rasterizer_state = nvfx_rasterizer_state_bind;
	nvfx->pipe.delete_rasterizer_state = nvfx_rasterizer_state_delete;

	nvfx->pipe.create_depth_stencil_alpha_state =
		nvfx_depth_stencil_alpha_state_create;
	nvfx->pipe.bind_depth_stencil_alpha_state =
		nvfx_depth_stencil_alpha_state_bind;
	nvfx->pipe.delete_depth_stencil_alpha_state =
		nvfx_depth_stencil_alpha_state_delete;

	nvfx->pipe.set_blend_color = nvfx_set_blend_color;
        nvfx->pipe.set_stencil_ref = nvfx_set_stencil_ref;
	nvfx->pipe.set_clip_state = nvfx_set_clip_state;
	nvfx->pipe.set_sample_mask = nvfx_set_sample_mask;
	nvfx->pipe.set_constant_buffer = nvfx_set_constant_buffer;
	nvfx->pipe.set_framebuffer_state = nvfx_set_framebuffer_state;
	nvfx->pipe.set_polygon_stipple = nvfx_set_polygon_stipple;
	nvfx->pipe.set_scissor_state = nvfx_set_scissor_state;
	nvfx->pipe.set_viewport_state = nvfx_set_viewport_state;
}
