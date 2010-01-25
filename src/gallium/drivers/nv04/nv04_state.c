#include "draw/draw_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_inlines.h"

#include "tgsi/tgsi_parse.h"

#include "nv04_context.h"
#include "nv04_state.h"

static void *
nv04_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
	struct nv04_blend_state *cb;

	cb = MALLOC(sizeof(struct nv04_blend_state));

	cb->b_enable = cso->rt[0].blend_enable ? 1 : 0;
	cb->b_src = ((nvgl_blend_func(cso->rt[0].alpha_src_factor)<<16) |
			 (nvgl_blend_func(cso->rt[0].rgb_src_factor)));
	cb->b_dst = ((nvgl_blend_func(cso->rt[0].alpha_dst_factor)<<16) |
			 (nvgl_blend_func(cso->rt[0].rgb_dst_factor)));
	

	return (void *)cb;
}

static void
nv04_blend_state_bind(struct pipe_context *pipe, void *blend)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	nv04->blend = (struct nv04_blend_state*)blend;

	nv04->dirty |= NV04_NEW_BLEND;
}

static void
nv04_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}


static INLINE unsigned
wrap_mode(unsigned wrap) {
	unsigned ret;

	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		ret = NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_REPEAT;
		break;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		ret = NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_MIRRORED_REPEAT;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		ret = NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		ret = NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_CLAMP:
		ret = NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_CLAMP;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
	default:
		NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
		ret = NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_CLAMP;
	}
	return ret >> NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_SHIFT;
}

static void *
nv04_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{

	struct nv04_sampler_state *ss;
	uint32_t filter = 0;

	ss = MALLOC(sizeof(struct nv04_sampler_state));

	ss->format = ((wrap_mode(cso->wrap_s) << NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSU_SHIFT) |
		    (wrap_mode(cso->wrap_t) << NV04_TEXTURED_TRIANGLE_FORMAT_ADDRESSV_SHIFT));

	if (cso->max_anisotropy > 1.0) {
		filter |= NV04_TEXTURED_TRIANGLE_FILTER_ANISOTROPIC_MINIFY_ENABLE | NV04_TEXTURED_TRIANGLE_FILTER_ANISOTROPIC_MAGNIFY_ENABLE;
	}

	switch (cso->mag_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		filter |= NV04_TEXTURED_TRIANGLE_FILTER_MAGNIFY_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		filter |= NV04_TEXTURED_TRIANGLE_FILTER_MAGNIFY_NEAREST;
		break;
	}

	switch (cso->min_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_LINEAR_MIPMAP_NEAREST;
			break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_LINEAR_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_LINEAR;
			break;
		}
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_NEAREST_MIPMAP_NEAREST;
		break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_NEAREST_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV04_TEXTURED_TRIANGLE_FILTER_MINIFY_NEAREST;
			break;
		}
		break;
	}

	ss->filter = filter;

	return (void *)ss;
}

static void
nv04_sampler_state_bind(struct pipe_context *pipe, unsigned nr, void **sampler)
{
	struct nv04_context *nv04 = nv04_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nv04->sampler[unit] = sampler[unit];
		nv04->dirty_samplers |= (1 << unit);
	}
}

static void
nv04_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void
nv04_set_sampler_texture(struct pipe_context *pipe, unsigned nr,
			 struct pipe_texture **miptree)
{
	struct nv04_context *nv04 = nv04_context(pipe);
	unsigned unit;

	for (unit = 0; unit < nr; unit++) {
		nv04->tex_miptree[unit] = (struct nv04_miptree *)miptree[unit];
		nv04->dirty_samplers |= (1 << unit);
	}
}

static void *
nv04_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
	struct nv04_rasterizer_state *rs;

	/*XXX: ignored:
	 * 	scissor
	 * 	points/lines (no hw support, emulated with tris in gallium)
	 */
	rs = MALLOC(sizeof(struct nv04_rasterizer_state));

	rs->blend = cso->flatshade ? NV04_TEXTURED_TRIANGLE_BLEND_SHADE_MODE_FLAT : NV04_TEXTURED_TRIANGLE_BLEND_SHADE_MODE_GOURAUD;

	return (void *)rs;
}

static void
nv04_rasterizer_state_bind(struct pipe_context *pipe, void *rast)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	nv04->rast = (struct nv04_rasterizer_state*)rast;

	draw_set_rasterizer_state(nv04->draw, (nv04->rast ? nv04->rast->templ : NULL));

	nv04->dirty |= NV04_NEW_RAST | NV04_NEW_BLEND;
}

static void
nv04_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static INLINE uint32_t nv04_compare_func(uint32_t f)
{
	switch ( f ) {
		case PIPE_FUNC_NEVER:		return 1;
		case PIPE_FUNC_LESS:		return 2;
		case PIPE_FUNC_EQUAL:		return 3;
		case PIPE_FUNC_LEQUAL:		return 4;
		case PIPE_FUNC_GREATER:		return 5;
		case PIPE_FUNC_NOTEQUAL:	return 6;
		case PIPE_FUNC_GEQUAL:		return 7;
		case PIPE_FUNC_ALWAYS:		return 8;
	}
	NOUVEAU_MSG("Unable to find the function\n");
	return 0;
}

static void *
nv04_depth_stencil_alpha_state_create(struct pipe_context *pipe,
			const struct pipe_depth_stencil_alpha_state *cso)
{
	struct nv04_depth_stencil_alpha_state *hw;

	hw = MALLOC(sizeof(struct nv04_depth_stencil_alpha_state));

	hw->control = float_to_ubyte(cso->alpha.ref_value);
	hw->control |= ( nv04_compare_func(cso->alpha.func) << NV04_TEXTURED_TRIANGLE_CONTROL_ALPHA_FUNC_SHIFT );
	hw->control |= cso->alpha.enabled ? NV04_TEXTURED_TRIANGLE_CONTROL_ALPHA_ENABLE : 0;
	hw->control |= NV04_TEXTURED_TRIANGLE_CONTROL_ORIGIN;
	hw->control |= cso->depth.enabled ? NV04_TEXTURED_TRIANGLE_CONTROL_Z_ENABLE : 0;
	hw->control |= ( nv04_compare_func(cso->depth.func)<< NV04_TEXTURED_TRIANGLE_CONTROL_Z_FUNC_SHIFT );
	hw->control |= 1 << NV04_TEXTURED_TRIANGLE_CONTROL_CULL_MODE_SHIFT; // no culling, handled by the draw module
	hw->control |= NV04_TEXTURED_TRIANGLE_CONTROL_DITHER_ENABLE;
	hw->control |= NV04_TEXTURED_TRIANGLE_CONTROL_Z_PERSPECTIVE_ENABLE;
	hw->control |= cso->depth.writemask ? NV04_TEXTURED_TRIANGLE_CONTROL_Z_WRITE : 0;
	hw->control |= 1 << NV04_TEXTURED_TRIANGLE_CONTROL_Z_FORMAT_SHIFT; // integer zbuffer format

	return (void *)hw;
}

static void
nv04_depth_stencil_alpha_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	nv04->dsa = hwcso;
	nv04->dirty |= NV04_NEW_CONTROL;
}

static void
nv04_depth_stencil_alpha_state_delete(struct pipe_context *pipe, void *hwcso)
{
	free(hwcso);
}

static void *
nv04_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *templ)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	return draw_create_vertex_shader(nv04->draw, templ);
}

static void
nv04_vp_state_bind(struct pipe_context *pipe, void *shader)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	draw_bind_vertex_shader(nv04->draw, (struct draw_vertex_shader *) shader);

	nv04->dirty |= NV04_NEW_VERTPROG;
}

static void
nv04_vp_state_delete(struct pipe_context *pipe, void *shader)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	draw_delete_vertex_shader(nv04->draw, (struct draw_vertex_shader *) shader);
}

static void *
nv04_fp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
	struct nv04_fragment_program *fp;

	fp = CALLOC(1, sizeof(struct nv04_fragment_program));
	fp->pipe.tokens = tgsi_dup_tokens(cso->tokens);

	return (void *)fp;
}

static void
nv04_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nv04_context *nv04 = nv04_context(pipe);
	struct nv04_fragment_program *fp = hwcso;

	nv04->fragprog.current = fp;
	nv04->dirty |= NV04_NEW_FRAGPROG;
}

static void
nv04_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nv04_context *nv04 = nv04_context(pipe);
	struct nv04_fragment_program *fp = hwcso;

	nv04_fragprog_destroy(nv04, fp);
	free((void*)fp->pipe.tokens);
	free(fp);
}

static void
nv04_set_blend_color(struct pipe_context *pipe,
		     const struct pipe_blend_color *bcol)
{
}

static void
nv04_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
}

static void
nv04_set_constant_buffer(struct pipe_context *pipe, uint shader, uint index,
			 struct pipe_buffer *buf )
{
	struct nv04_context *nv04 = nv04_context(pipe);
	struct pipe_screen *pscreen = pipe->screen;

	assert(shader < PIPE_SHADER_TYPES);
	assert(index == 0);

	if (buf) {
		void *mapped;
		if (buf && buf->size &&
                    (mapped = pipe_buffer_map(pscreen, buf, PIPE_BUFFER_USAGE_CPU_READ)))
		{
			memcpy(nv04->constbuf[shader], mapped, buf->size);
			nv04->constbuf_nr[shader] =
				buf->size / (4 * sizeof(float));
			pipe_buffer_unmap(pscreen, buf);
		}
	}
}

static void
nv04_set_framebuffer_state(struct pipe_context *pipe,
			   const struct pipe_framebuffer_state *fb)
{
	struct nv04_context *nv04 = nv04_context(pipe);
	
	nv04->framebuffer = (struct pipe_framebuffer_state*)fb;

	nv04->dirty |= NV04_NEW_FRAMEBUFFER;
}
static void
nv04_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
	NOUVEAU_ERR("line stipple hahaha\n");
}

static void
nv04_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
/*	struct nv04_context *nv04 = nv04_context(pipe);

	// XXX
	BEGIN_RING(fahrenheit, NV04_TEXTURED_TRIANGLE_SCISSOR_HORIZ, 2);
	OUT_RING  (((s->maxx - s->minx) << 16) | s->minx);
	OUT_RING  (((s->maxy - s->miny) << 16) | s->miny);*/
}

static void
nv04_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *viewport)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	nv04->viewport = *viewport;

	draw_set_viewport_state(nv04->draw, &nv04->viewport);
}

static void
nv04_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
		       const struct pipe_vertex_buffer *buffers)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	memcpy(nv04->vtxbuf, buffers, count * sizeof(buffers[0]));
	nv04->dirty |= NV04_NEW_VTXARRAYS;

	draw_set_vertex_buffers(nv04->draw, count, buffers);
}

static void
nv04_set_vertex_elements(struct pipe_context *pipe, unsigned count,
			const struct pipe_vertex_element *elements)
{
	struct nv04_context *nv04 = nv04_context(pipe);

	memcpy(nv04->vtxelt, elements, sizeof(*elements) * count);
	nv04->dirty |= NV04_NEW_VTXARRAYS;

	draw_set_vertex_elements(nv04->draw, count, elements);
}

void
nv04_init_state_functions(struct nv04_context *nv04)
{
	nv04->pipe.create_blend_state = nv04_blend_state_create;
	nv04->pipe.bind_blend_state = nv04_blend_state_bind;
	nv04->pipe.delete_blend_state = nv04_blend_state_delete;

	nv04->pipe.create_sampler_state = nv04_sampler_state_create;
	nv04->pipe.bind_fragment_sampler_states = nv04_sampler_state_bind;
	nv04->pipe.delete_sampler_state = nv04_sampler_state_delete;
	nv04->pipe.set_fragment_sampler_textures = nv04_set_sampler_texture;

	nv04->pipe.create_rasterizer_state = nv04_rasterizer_state_create;
	nv04->pipe.bind_rasterizer_state = nv04_rasterizer_state_bind;
	nv04->pipe.delete_rasterizer_state = nv04_rasterizer_state_delete;

	nv04->pipe.create_depth_stencil_alpha_state = nv04_depth_stencil_alpha_state_create;
	nv04->pipe.bind_depth_stencil_alpha_state = nv04_depth_stencil_alpha_state_bind;
	nv04->pipe.delete_depth_stencil_alpha_state = nv04_depth_stencil_alpha_state_delete;

	nv04->pipe.create_vs_state = nv04_vp_state_create;
	nv04->pipe.bind_vs_state = nv04_vp_state_bind;
	nv04->pipe.delete_vs_state = nv04_vp_state_delete;

	nv04->pipe.create_fs_state = nv04_fp_state_create;
	nv04->pipe.bind_fs_state = nv04_fp_state_bind;
	nv04->pipe.delete_fs_state = nv04_fp_state_delete;

	nv04->pipe.set_blend_color = nv04_set_blend_color;
	nv04->pipe.set_clip_state = nv04_set_clip_state;
	nv04->pipe.set_constant_buffer = nv04_set_constant_buffer;
	nv04->pipe.set_framebuffer_state = nv04_set_framebuffer_state;
	nv04->pipe.set_polygon_stipple = nv04_set_polygon_stipple;
	nv04->pipe.set_scissor_state = nv04_set_scissor_state;
	nv04->pipe.set_viewport_state = nv04_set_viewport_state;

	nv04->pipe.set_vertex_buffers = nv04_set_vertex_buffers;
	nv04->pipe.set_vertex_elements = nv04_set_vertex_elements;
}

