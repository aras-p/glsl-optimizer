#ifndef vl_context_h
#define vl_context_h

#include <X11/Xlib.h>
#include <pipe/p_state.h>
#include "vl_types.h"

struct pipe_context;

struct VL_CONTEXT
{
	Display			*display;
	struct pipe_context	*pipe;
	unsigned int		video_width;
	unsigned int		video_height;
	enum VL_FORMAT		video_format;
	
	struct
	{
		struct
		{
			struct pipe_rasterizer_state		*raster;
			struct pipe_depth_stencil_alpha_state	*dsa;
			struct pipe_blend_state			*blend;
		} common;
		
		struct
		{
		} idct;
		
		struct
		{
			struct pipe_viewport_state		viewport;
			struct pipe_framebuffer_state		render_target;
			struct pipe_sampler_state		*samplers[5];
			struct pipe_texture			*textures[5];
			struct pipe_shader_state		*i_vs, *p_vs[2], *b_vs[2];
			struct pipe_shader_state		*i_fs, *p_fs[2], *b_fs[2];
			struct pipe_vertex_buffer 		vertex_bufs[3];
			struct pipe_vertex_element		vertex_buf_elems[3];
			struct pipe_constant_buffer		vs_const_buf, fs_const_buf;
		} mc;
		
		struct
		{
			struct pipe_viewport_state		viewport;
			struct pipe_framebuffer_state		framebuffer;
			struct pipe_sampler_state		*sampler;
			struct pipe_shader_state		*vertex_shader, *fragment_shader;
			struct pipe_vertex_buffer 		vertex_bufs[2];
			struct pipe_vertex_element		vertex_buf_elems[2];
			struct pipe_constant_buffer		fs_const_buf;
		} csc;
	} states;
};

int vlCreateContext
(
	Display *display,
	struct pipe_context *pipe,
	unsigned int video_width,
	unsigned int video_height,
	enum VL_FORMAT video_format,
	struct VL_CONTEXT **context
);

int vlDestroyContext(struct VL_CONTEXT *context);

int vlBeginRender(struct VL_CONTEXT *context);
int vlEndRender(struct VL_CONTEXT *context);

#endif

