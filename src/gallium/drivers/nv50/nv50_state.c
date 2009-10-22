/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "tgsi/tgsi_parse.h"

#include "nv50_context.h"
#include "nv50_texture.h"

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
		so_data  (so, 0x4000 | nvgl_blend_func(cso->rgb_src_factor));
		so_data  (so, 0x4000 | nvgl_blend_func(cso->rgb_dst_factor));
		so_data  (so, nvgl_blend_eqn(cso->alpha_func));
		so_data  (so, 0x4000 | nvgl_blend_func(cso->alpha_src_factor));
		so_method(so, tesla, NV50TCL_BLEND_FUNC_DST_ALPHA, 1);
		so_data  (so, 0x4000 | nvgl_blend_func(cso->alpha_dst_factor));
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
	so_ref(NULL, &so);
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

static INLINE unsigned
wrap_mode(unsigned wrap)
{
	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		return NV50TSC_1_0_WRAPS_REPEAT;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		return NV50TSC_1_0_WRAPS_MIRROR_REPEAT;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		return NV50TSC_1_0_WRAPS_CLAMP_TO_EDGE;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		return NV50TSC_1_0_WRAPS_CLAMP_TO_BORDER;
	case PIPE_TEX_WRAP_CLAMP:
		return NV50TSC_1_0_WRAPS_CLAMP;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		return NV50TSC_1_0_WRAPS_MIRROR_CLAMP_TO_EDGE;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		return NV50TSC_1_0_WRAPS_MIRROR_CLAMP_TO_BORDER;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		return NV50TSC_1_0_WRAPS_MIRROR_CLAMP;
	default:
		NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
		return NV50TSC_1_0_WRAPS_REPEAT;
	}
}
static void *
nv50_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	struct nv50_sampler_stateobj *sso = CALLOC(1, sizeof(*sso));
	unsigned *tsc = sso->tsc;
	float limit;

	tsc[0] = (0x00026000 |
		  (wrap_mode(cso->wrap_s) << 0) |
		  (wrap_mode(cso->wrap_t) << 3) |
		  (wrap_mode(cso->wrap_r) << 6));

	switch (cso->mag_img_filter) {
	case PIPE_TEX_FILTER_ANISO:
	case PIPE_TEX_FILTER_LINEAR:
		tsc[1] |= NV50TSC_1_1_MAGF_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		tsc[1] |= NV50TSC_1_1_MAGF_NEAREST;
		break;
	}

	switch (cso->min_img_filter) {
	case PIPE_TEX_FILTER_ANISO:
	case PIPE_TEX_FILTER_LINEAR:
		tsc[1] |= NV50TSC_1_1_MINF_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		tsc[1] |= NV50TSC_1_1_MINF_NEAREST;
		break;
	}

	switch (cso->min_mip_filter) {
	case PIPE_TEX_MIPFILTER_LINEAR:
		tsc[1] |= NV50TSC_1_1_MIPF_LINEAR;
		break;
	case PIPE_TEX_MIPFILTER_NEAREST:
		tsc[1] |= NV50TSC_1_1_MIPF_NEAREST;
		break;
	case PIPE_TEX_MIPFILTER_NONE:
	default:
		tsc[1] |= NV50TSC_1_1_MIPF_NONE;
		break;
	}

	if (cso->max_anisotropy >= 16.0)
		tsc[0] |= (7 << 20);
	else
	if (cso->max_anisotropy >= 12.0)
		tsc[0] |= (6 << 20);
	else {
		tsc[0] |= (int)(cso->max_anisotropy * 0.5f) << 20;

		if (cso->max_anisotropy >= 4.0)
			tsc[1] |= NV50TSC_1_1_UNKN_ANISO_35;
		else
		if (cso->max_anisotropy >= 2.0)
			tsc[1] |= NV50TSC_1_1_UNKN_ANISO_15;
	}

	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
		tsc[0] |= (1 << 8);
		tsc[0] |= (nvgl_comparison_op(cso->compare_func) & 0x7);
	}

	limit = CLAMP(cso->lod_bias, -16.0, 15.0);
	tsc[1] |= ((int)(limit * 256.0) & 0x1fff) << 12;

	tsc[2] |= ((int)CLAMP(cso->max_lod, 0.0, 15.0) << 20) |
		  ((int)CLAMP(cso->min_lod, 0.0, 15.0) << 8);

	tsc[4] = fui(cso->border_color[0]);
	tsc[5] = fui(cso->border_color[1]);
	tsc[6] = fui(cso->border_color[2]);
	tsc[7] = fui(cso->border_color[3]);

	sso->normalized = cso->normalized_coords;
	return (void *)sso;
}

static void
nv50_sampler_state_bind(struct pipe_context *pipe, unsigned nr, void **sampler)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	int i;

	nv50->sampler_nr = nr;
	for (i = 0; i < nv50->sampler_nr; i++)
		nv50->sampler[i] = sampler[i];

	nv50->dirty |= NV50_NEW_SAMPLER;
}

static void
nv50_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void
nv50_set_sampler_texture(struct pipe_context *pipe, unsigned nr,
			 struct pipe_texture **pt)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	int i;

	for (i = 0; i < nr; i++)
		pipe_texture_reference((void *)&nv50->miptree[i], pt[i]);
	for (i = nr; i < nv50->miptree_nr; i++)
		pipe_texture_reference((void *)&nv50->miptree[i], NULL);

	nv50->miptree_nr = nr;
	nv50->dirty |= NV50_NEW_TEXTURE;
}

static void *
nv50_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nouveau_stateobj *so = so_new(64, 0);
	struct nouveau_grobj *tesla = nv50_context(pipe)->screen->tesla;
	struct nv50_rasterizer_stateobj *rso =
		CALLOC_STRUCT(nv50_rasterizer_stateobj);

	/*XXX: ignored
	 * 	- light_twosize
	 * 	- point_smooth
	 * 	- multisample
	 * 	- point_sprite / sprite_coord_mode
	 */

	so_method(so, tesla, NV50TCL_SHADE_MODEL, 1);
	so_data  (so, cso->flatshade ? NV50TCL_SHADE_MODEL_FLAT :
				       NV50TCL_SHADE_MODEL_SMOOTH);
	so_method(so, tesla, 0x1684, 1);
	so_data  (so, cso->flatshade_first ? 0 : 1);

	so_method(so, tesla, NV50TCL_VERTEX_TWO_SIDE_ENABLE, 1);
	so_data  (so, cso->light_twoside);

	so_method(so, tesla, NV50TCL_LINE_WIDTH, 1);
	so_data  (so, fui(cso->line_width));
	so_method(so, tesla, NV50TCL_LINE_SMOOTH_ENABLE, 1);
	so_data  (so, cso->line_smooth ? 1 : 0);
	if (cso->line_stipple_enable) {
		so_method(so, tesla, NV50TCL_LINE_STIPPLE_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, tesla, NV50TCL_LINE_STIPPLE_PATTERN, 1);
		so_data  (so, (cso->line_stipple_pattern << 8) |
			       cso->line_stipple_factor);
	} else {
		so_method(so, tesla, NV50TCL_LINE_STIPPLE_ENABLE, 1);
		so_data  (so, 0);
	}

	so_method(so, tesla, NV50TCL_POINT_SIZE, 1);
	so_data  (so, fui(cso->point_size));

	so_method(so, tesla, NV50TCL_POINT_SPRITE_ENABLE, 1);
	so_data  (so, cso->point_sprite);

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
		so_data  (so, fui(cso->offset_units));
	}

	rso->pipe = *cso;
	so_ref(so, &rso->so);
	so_ref(NULL, &so);
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

	/* XXX: keep hex values until header is updated (names reversed) */
	if (cso->stencil[0].enabled) {
		so_method(so, tesla, 0x1380, 8);
		so_data  (so, 1);
		so_data  (so, nvgl_stencil_op(cso->stencil[0].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
		so_data  (so, nvgl_comparison_op(cso->stencil[0].func));
		so_data  (so, cso->stencil[0].ref_value);
		so_data  (so, cso->stencil[0].writemask);
		so_data  (so, cso->stencil[0].valuemask);
	} else {
		so_method(so, tesla, 0x1380, 1);
		so_data  (so, 0);
	}

	if (cso->stencil[1].enabled) {
		so_method(so, tesla, 0x1594, 5);
		so_data  (so, 1);
		so_data  (so, nvgl_stencil_op(cso->stencil[1].fail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zfail_op));
		so_data  (so, nvgl_stencil_op(cso->stencil[1].zpass_op));
		so_data  (so, nvgl_comparison_op(cso->stencil[1].func));
		so_method(so, tesla, 0x0f54, 3);
		so_data  (so, cso->stencil[1].ref_value);
		so_data  (so, cso->stencil[1].writemask);
		so_data  (so, cso->stencil[1].valuemask);
	} else {
		so_method(so, tesla, 0x1594, 1);
		so_data  (so, 0);
	}

	if (cso->alpha.enabled) {
		so_method(so, tesla, NV50TCL_ALPHA_TEST_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, tesla, NV50TCL_ALPHA_TEST_REF, 2);
		so_data  (so, fui(cso->alpha.ref_value));
		so_data  (so, nvgl_comparison_op(cso->alpha.func));
	} else {
		so_method(so, tesla, NV50TCL_ALPHA_TEST_ENABLE, 1);
		so_data  (so, 0);
	}

	zsa->pipe = *cso;
	so_ref(so, &zsa->so);
	so_ref(NULL, &so);
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
	struct nv50_program *p = CALLOC_STRUCT(nv50_program);

	p->pipe.tokens = tgsi_dup_tokens(cso->tokens);
	p->type = PIPE_SHADER_VERTEX;
	tgsi_scan_shader(p->pipe.tokens, &p->info);
	return (void *)p;
}

static void
nv50_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->vertprog = hwcso;
	nv50->dirty |= NV50_NEW_VERTPROG;
}

static void
nv50_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nv50_program *p = hwcso;

	nv50_program_destroy(nv50, p);
	FREE((void*)p->pipe.tokens);
	FREE(p);
}

static void *
nv50_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv50_program *p = CALLOC_STRUCT(nv50_program);

	p->pipe.tokens = tgsi_dup_tokens(cso->tokens);
	p->type = PIPE_SHADER_FRAGMENT;
	tgsi_scan_shader(p->pipe.tokens, &p->info);
	return (void *)p;
}

static void
nv50_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	nv50->fragprog = hwcso;
	nv50->dirty |= NV50_NEW_FRAGPROG;
}

static void
nv50_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nv50_program *p = hwcso;

	nv50_program_destroy(nv50, p);
	FREE((void*)p->pipe.tokens);
	FREE(p);
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
	struct nv50_context *nv50 = nv50_context(pipe);

	if (shader == PIPE_SHADER_VERTEX) {
		nv50->constbuf[PIPE_SHADER_VERTEX] = buf->buffer;
		nv50->dirty |= NV50_NEW_VERTPROG_CB;
	} else
	if (shader == PIPE_SHADER_FRAGMENT) {
		nv50->constbuf[PIPE_SHADER_FRAGMENT] = buf->buffer;
		nv50->dirty |= NV50_NEW_FRAGPROG_CB;
	}
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
	struct nv50_context *nv50 = nv50_context(pipe);

	memcpy(nv50->vtxbuf, vb, sizeof(*vb) * count);
	nv50->vtxbuf_nr = count;

	nv50->dirty |= NV50_NEW_ARRAYS;
}

static void
nv50_set_vertex_elements(struct pipe_context *pipe, unsigned count,
			 const struct pipe_vertex_element *ve)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	memcpy(nv50->vtxelt, ve, sizeof(*ve) * count);
	nv50->vtxelt_nr = count;

	nv50->dirty |= NV50_NEW_ARRAYS;
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

