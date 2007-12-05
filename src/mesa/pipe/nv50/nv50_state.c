#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_dma.h"
#include "nv50_state.h"

static void *
nv50_alpha_test_state_create(struct pipe_context *pipe,
			     const struct pipe_alpha_test_state *cso)
{
}

static void
nv50_alpha_test_state_bind(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_alpha_test_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void *
nv50_blend_state_create(struct pipe_context *pipe,
			const struct pipe_blend_state *cso)
{
}

static void
nv50_blend_state_bind(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_blend_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void *
nv50_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
}

static void
nv50_sampler_state_bind(struct pipe_context *pipe, unsigned unit,
			void *hwcso)
{
}

static void
nv50_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void *
nv50_rasterizer_state_create(struct pipe_context *pipe,
			     const struct pipe_rasterizer_state *cso)
{
}

static void
nv50_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void *
nv50_depth_stencil_state_create(struct pipe_context *pipe,
				const struct pipe_depth_stencil_state *cso)
{
}

static void
nv50_depth_stencil_state_bind(struct pipe_context *pipe, void *hwcso)
{
}

static void
nv50_depth_stencil_state_delete(struct pipe_context *pipe, void *hwcso)
{
}

static void *
nv50_vp_state_create(struct pipe_context *pipe,
		     const struct pipe_shader_state *cso)
{
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
}

static void
nv50_set_clip_state(struct pipe_context *pipe,
		    const struct pipe_clip_state *clip)
{
}

static void
nv50_set_clear_color_state(struct pipe_context *pipe,
			   const struct pipe_clear_color_state *ccol)
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
}

static void
nv50_set_polygon_stipple(struct pipe_context *pipe,
			 const struct pipe_poly_stipple *stipple)
{
}

static void
nv50_set_sampler_units(struct pipe_context *pipe,
		       uint num_samplers, const uint *units)
{
}

static void
nv50_set_scissor_state(struct pipe_context *pipe,
		       const struct pipe_scissor_state *s)
{
}

static void
nv50_set_texture_state(struct pipe_context *pipe, unsigned unit,
		       struct pipe_mipmap_tree *miptree)
{
}

static void
nv50_set_viewport_state(struct pipe_context *pipe,
			const struct pipe_viewport_state *vpt)
{
}

static void
nv50_set_vertex_buffer(struct pipe_context *pipe, unsigned index,
		       const struct pipe_vertex_buffer *vb)
{
}

static void
nv50_set_vertex_element(struct pipe_context *pipe, unsigned index,
			const struct pipe_vertex_element *ve)
{
}

void
nv50_init_state_functions(struct nv50_context *nv50)
{
	nv50->pipe.create_alpha_test_state = nv50_alpha_test_state_create;
	nv50->pipe.bind_alpha_test_state = nv50_alpha_test_state_bind;
	nv50->pipe.delete_alpha_test_state = nv50_alpha_test_state_delete;

	nv50->pipe.create_blend_state = nv50_blend_state_create;
	nv50->pipe.bind_blend_state = nv50_blend_state_bind;
	nv50->pipe.delete_blend_state = nv50_blend_state_delete;

	nv50->pipe.create_sampler_state = nv50_sampler_state_create;
	nv50->pipe.bind_sampler_state = nv50_sampler_state_bind;
	nv50->pipe.delete_sampler_state = nv50_sampler_state_delete;

	nv50->pipe.create_rasterizer_state = nv50_rasterizer_state_create;
	nv50->pipe.bind_rasterizer_state = nv50_rasterizer_state_bind;
	nv50->pipe.delete_rasterizer_state = nv50_rasterizer_state_delete;

	nv50->pipe.create_depth_stencil_state = nv50_depth_stencil_state_create;
	nv50->pipe.bind_depth_stencil_state = nv50_depth_stencil_state_bind;
	nv50->pipe.delete_depth_stencil_state = nv50_depth_stencil_state_delete;

	nv50->pipe.create_vs_state = nv50_vp_state_create;
	nv50->pipe.bind_vs_state = nv50_vp_state_bind;
	nv50->pipe.delete_vs_state = nv50_vp_state_delete;

	nv50->pipe.create_fs_state = nv50_fp_state_create;
	nv50->pipe.bind_fs_state = nv50_fp_state_bind;
	nv50->pipe.delete_fs_state = nv50_fp_state_delete;

	nv50->pipe.set_blend_color = nv50_set_blend_color;
	nv50->pipe.set_clip_state = nv50_set_clip_state;
	nv50->pipe.set_clear_color_state = nv50_set_clear_color_state;
	nv50->pipe.set_constant_buffer = nv50_set_constant_buffer;
	nv50->pipe.set_framebuffer_state = nv50_set_framebuffer_state;
	nv50->pipe.set_polygon_stipple = nv50_set_polygon_stipple;
	nv50->pipe.set_sampler_units = nv50_set_sampler_units;
	nv50->pipe.set_scissor_state = nv50_set_scissor_state;
	nv50->pipe.set_texture_state = nv50_set_texture_state;
	nv50->pipe.set_viewport_state = nv50_set_viewport_state;

	nv50->pipe.set_vertex_buffer = nv50_set_vertex_buffer;
	nv50->pipe.set_vertex_element = nv50_set_vertex_element;

//	nv50->pipe.set_feedback_state = nv50_set_feedback_state;
//	nv50->pipe.set_feedback_buffer = nv50_set_feedback_buffer;
}

