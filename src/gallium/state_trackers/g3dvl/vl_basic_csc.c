#define VL_INTERNAL
#include "vl_basic_csc.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_state.h>
#include <pipe/p_inlines.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>
#include <util/u_memory.h>
#include "vl_csc.h"
#include "vl_surface.h"
#include "vl_shader_build.h"
#include "vl_types.h"

struct vlVertexShaderConsts
{
	struct vlVertex4f	dst_scale;
	struct vlVertex4f	dst_trans;
	struct vlVertex4f	src_scale;
	struct vlVertex4f	src_trans;
};

struct vlFragmentShaderConsts
{
	struct vlVertex4f	bias;
	float			matrix[16];
};

struct vlBasicCSC
{
	struct vlCSC				base;

	struct pipe_context			*pipe;
	struct pipe_viewport_state		viewport;
	struct pipe_framebuffer_state		framebuffer;
	struct pipe_texture			*framebuffer_tex;
	void					*sampler;
	void					*vertex_shader, *fragment_shader;
	struct pipe_vertex_buffer 		vertex_bufs[2];
	struct pipe_vertex_element		vertex_elems[2];
	struct pipe_constant_buffer		vs_const_buf, fs_const_buf;
};

static int vlResizeFrameBuffer
(
	struct vlCSC *csc,
	unsigned int width,
	unsigned int height
)
{
	struct vlBasicCSC	*basic_csc;
	struct pipe_context	*pipe;
	struct pipe_texture	template;

	assert(csc);

	basic_csc = (struct vlBasicCSC*)csc;
	pipe = basic_csc->pipe;

	if (basic_csc->framebuffer.width == width && basic_csc->framebuffer.height == height)
		return 0;

	basic_csc->viewport.scale[0] = width;
	basic_csc->viewport.scale[1] = height;
	basic_csc->viewport.scale[2] = 1;
	basic_csc->viewport.scale[3] = 1;
	basic_csc->viewport.translate[0] = 0;
	basic_csc->viewport.translate[1] = 0;
	basic_csc->viewport.translate[2] = 0;
	basic_csc->viewport.translate[3] = 0;
	
	if (basic_csc->framebuffer_tex)
	{
		pipe_surface_reference(&basic_csc->framebuffer.cbufs[0], NULL);
		pipe_texture_reference(&basic_csc->framebuffer_tex, NULL);
	}
	
	memset(&template, 0, sizeof(struct pipe_texture));
	template.target = PIPE_TEXTURE_2D;
	template.format = PIPE_FORMAT_A8R8G8B8_UNORM;
	template.last_level = 0;
	template.width[0] = width;
	template.height[0] = height;
	template.depth[0] = 1;
	template.compressed = 0;
	pf_get_block(template.format, &template.block);
	template.tex_usage = PIPE_TEXTURE_USAGE_DISPLAY_TARGET;

	basic_csc->framebuffer_tex = pipe->screen->texture_create(pipe->screen, &template);

	basic_csc->framebuffer.width = width;
	basic_csc->framebuffer.height = height;
	basic_csc->framebuffer.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		basic_csc->framebuffer_tex,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);

	/* Clear to black, in case video doesn't fill the entire window */
	pipe->set_framebuffer_state(pipe, &basic_csc->framebuffer);
	pipe->clear(pipe, PIPE_CLEAR_COLOR, 0, 0.0f, 0);

	return 0;
}

static int vlBegin
(
	struct vlCSC *csc
)
{
	struct vlBasicCSC	*basic_csc;
	struct pipe_context	*pipe;

	assert(csc);

	basic_csc = (struct vlBasicCSC*)csc;
	pipe = basic_csc->pipe;

	pipe->set_framebuffer_state(pipe, &basic_csc->framebuffer);
	pipe->set_viewport_state(pipe, &basic_csc->viewport);
	pipe->bind_sampler_states(pipe, 1, (void**)&basic_csc->sampler);
	/* Source texture set in vlPutPictureCSC() */
	pipe->bind_vs_state(pipe, basic_csc->vertex_shader);
	pipe->bind_fs_state(pipe, basic_csc->fragment_shader);
	pipe->set_vertex_buffers(pipe, 2, basic_csc->vertex_bufs);
	pipe->set_vertex_elements(pipe, 2, basic_csc->vertex_elems);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_VERTEX, 0, &basic_csc->vs_const_buf);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, 0, &basic_csc->fs_const_buf);

	return 0;
}

static int vlPutPictureCSC
(
	struct vlCSC *csc,
	struct vlSurface *surface,
	int srcx,
	int srcy,
	int srcw,
	int srch,
	int destx,
	int desty,
	int destw,
	int desth,
	enum vlPictureType picture_type
)
{
	struct vlBasicCSC		*basic_csc;
	struct pipe_context		*pipe;
	struct vlVertexShaderConsts	*vs_consts;

	assert(csc);
	assert(surface);

	basic_csc = (struct vlBasicCSC*)csc;
	pipe = basic_csc->pipe;

	vs_consts = pipe_buffer_map
	(
		pipe->screen,
		basic_csc->vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
	);

	vs_consts->dst_scale.x = destw / (float)basic_csc->framebuffer.cbufs[0]->width;
	vs_consts->dst_scale.y = desth / (float)basic_csc->framebuffer.cbufs[0]->height;
	vs_consts->dst_scale.z = 1;
	vs_consts->dst_scale.w = 1;
	vs_consts->dst_trans.x = destx / (float)basic_csc->framebuffer.cbufs[0]->width;
	vs_consts->dst_trans.y = desty / (float)basic_csc->framebuffer.cbufs[0]->height;
	vs_consts->dst_trans.z = 0;
	vs_consts->dst_trans.w = 0;

	vs_consts->src_scale.x = srcw / (float)surface->texture->width[0];
	vs_consts->src_scale.y = srch / (float)surface->texture->height[0];
	vs_consts->src_scale.z = 1;
	vs_consts->src_scale.w = 1;
	vs_consts->src_trans.x = srcx / (float)surface->texture->width[0];
	vs_consts->src_trans.y = srcy / (float)surface->texture->height[0];
	vs_consts->src_trans.z = 0;
	vs_consts->src_trans.w = 0;

	pipe_buffer_unmap(pipe->screen, basic_csc->vs_const_buf.buffer);

	pipe->set_sampler_textures(pipe, 1, &surface->texture);
	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLE_STRIP, 0, 4);

	return 0;
}

static int vlEnd
(
	struct vlCSC *csc
)
{
	assert(csc);

	return 0;
}

static struct pipe_surface* vlGetFrameBuffer
(
	struct vlCSC *csc
)
{
	struct vlBasicCSC	*basic_csc;

	assert(csc);

	basic_csc = (struct vlBasicCSC*)csc;

	return basic_csc->framebuffer.cbufs[0];
}

static int vlDestroy
(
	struct vlCSC *csc
)
{
	struct vlBasicCSC	*basic_csc;
	struct pipe_context	*pipe;
	unsigned int		i;

	assert(csc);

	basic_csc = (struct vlBasicCSC*)csc;
	pipe = basic_csc->pipe;

	if (basic_csc->framebuffer_tex)
	{
		pipe_surface_reference(&basic_csc->framebuffer.cbufs[0], NULL);
		pipe_texture_reference(&basic_csc->framebuffer_tex, NULL);
	}

	pipe->delete_sampler_state(pipe, basic_csc->sampler);
	pipe->delete_vs_state(pipe, basic_csc->vertex_shader);
	pipe->delete_fs_state(pipe, basic_csc->fragment_shader);

	for (i = 0; i < 2; ++i)
		pipe_buffer_reference(&basic_csc->vertex_bufs[i].buffer, NULL);

	pipe_buffer_reference(&basic_csc->vs_const_buf.buffer, NULL);
	pipe_buffer_reference(&basic_csc->fs_const_buf.buffer, NULL);

	FREE(basic_csc);

	return 0;
}

/*
 * Represents 2 triangles in a strip in normalized coords.
 * Used to render the surface onto the frame buffer.
 */
static const struct vlVertex2f surface_verts[4] =
{
	{0.0f, 0.0f},
	{0.0f, 1.0f},
	{1.0f, 0.0f},
	{1.0f, 1.0f}
};

/*
 * Represents texcoords for the above. We can use the position values directly.
 * TODO: Duplicate these in the shader, no need to create a buffer.
 */
static const struct vlVertex2f *surface_texcoords = surface_verts;

/*
 * Identity color conversion constants, for debugging
 */
static const struct vlFragmentShaderConsts identity =
{
	{
		0.0f, 0.0f, 0.0f, 0.0f
	},
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	}
};

/*
 * Converts ITU-R BT.601 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [16,235]
 */
static const struct vlFragmentShaderConsts bt_601 =
{
	{
		0.0f,		0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.0f,		0.0f,		1.371f,		0.0f,
		1.0f,		-0.336f,	-0.698f,	0.0f,
		1.0f,		1.732f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

/*
 * Converts ITU-R BT.601 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [0,255]
 */
static const struct vlFragmentShaderConsts bt_601_full =
{
	{
		0.062745098f,	0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.164f,		0.0f,		1.596f,		0.0f,
		1.164f,		-0.391f,	-0.813f,	0.0f,
		1.164f,		2.018f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

/*
 * Converts ITU-R BT.709 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [16,235]
 */
static const struct vlFragmentShaderConsts bt_709 =
{
	{
		0.0f,		0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.0f,		0.0f,		1.540f,		0.0f,
		1.0f,		-0.183f,	-0.459f,	0.0f,
		1.0f,		1.816f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

/*
 * Converts ITU-R BT.709 YCbCr pixels to RGB pixels where:
 * Y is in [16,235], Cb and Cr are in [16,240]
 * R, G, and B are in [0,255]
 */
const struct vlFragmentShaderConsts bt_709_full =
{
	{
		0.062745098f,	0.501960784f,	0.501960784f,	0.0f
	},
	{
		1.164f,		0.0f,		1.793f,		0.0f,
		1.164f,		-0.213f,	-0.534f,	0.0f,
		1.164f,		2.115f,		0.0f,		0.0f,
		0.0f,		0.0f,		0.0f,		1.0f
	}
};

static int vlCreateVertexShader
(
	struct vlBasicCSC *csc
)
{
	const unsigned int		max_tokens = 50;

	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(csc);

	pipe = csc->pipe;
	tokens = (struct tgsi_token*)MALLOC(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_VERTEX, header);

	ti = 3;

	/*
	 * decl i0		; Vertex pos
	 * decl i1		; Vertex texcoords
	 */
	for (i = 0; i < 2; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0		; Scaling vector to scale vertex pos rect to destination size
	 * decl c1		; Translation vector to move vertex pos rect into position
	 * decl c2		; Scaling vector to scale texcoord rect to source size
	 * decl c3		; Translation vector to move texcoord rect into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 3);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Vertex texcoords
	 */
	for (i = 0; i < 2; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * madd o0, i0, c0, c1	; Scale and translate unit output rect to destination size and pos
	 * madd o1, i1, c2, c3	; Scale and translate unit texcoord rect to source size and pos
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst4(TGSI_OPCODE_MADD, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i, TGSI_FILE_CONSTANT, i * 2, TGSI_FILE_CONSTANT, i * 2 + 1);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	csc->vertex_shader = pipe->create_vs_state(pipe, &vs);
	FREE(tokens);

	return 0;
}

static int vlCreateFragmentShader
(
	struct vlBasicCSC *csc
)
{
	const unsigned int		max_tokens = 50;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(csc);

	pipe = csc->pipe;
	tokens = (struct tgsi_token*)MALLOC(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/* decl i0		; Texcoords for s0 */
	decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, 1, 0, 0, TGSI_INTERPOLATE_LINEAR);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl c0		; Bias vector for CSC
	 * decl c1-c4		; CSC matrix c1-c4
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 4);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0		; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0 */
	decl = vl_decl_temps(0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl s0		; Sampler for tex containing picture to display */
	decl = vl_decl_samplers(0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* tex2d t0, i0, s0	; Read src pixel */
	inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* sub t0, t0, c0	; Subtract bias vector from pixel */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * dp4 o0.x, t0, c1	; Multiply pixel by the color conversion matrix
	 * dp4 o0.y, t0, c2
	 * dp4 o0.z, t0, c3
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_DP4, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i + 1);
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	csc->fragment_shader = pipe->create_fs_state(pipe, &fs);
	FREE(tokens);

	return 0;
}

static int vlCreateDataBufs
(
	struct vlBasicCSC *csc
)
{
	struct pipe_context *pipe;

	assert(csc);

	pipe = csc->pipe;

	/*
	 * Create our vertex buffer and vertex buffer element
	 * VB contains 4 vertices that render a quad covering the entire window
	 * to display a rendered surface
	 * Quad is rendered as a tri strip
	 */
	csc->vertex_bufs[0].stride = sizeof(struct vlVertex2f);
	csc->vertex_bufs[0].max_index = 3;
	csc->vertex_bufs[0].buffer_offset = 0;
	csc->vertex_bufs[0].buffer = pipe_buffer_create
	(
		pipe->screen,
		1,
		PIPE_BUFFER_USAGE_VERTEX,
		sizeof(struct vlVertex2f) * 4
	);

	memcpy
	(
		pipe_buffer_map(pipe->screen, csc->vertex_bufs[0].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		surface_verts,
		sizeof(struct vlVertex2f) * 4
	);

	pipe_buffer_unmap(pipe->screen, csc->vertex_bufs[0].buffer);

	csc->vertex_elems[0].src_offset = 0;
	csc->vertex_elems[0].vertex_buffer_index = 0;
	csc->vertex_elems[0].nr_components = 2;
	csc->vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/*
	 * Create our texcoord buffer and texcoord buffer element
	 * Texcoord buffer contains the TCs for mapping the rendered surface to the 4 vertices
	 */
	csc->vertex_bufs[1].stride = sizeof(struct vlVertex2f);
	csc->vertex_bufs[1].max_index = 3;
	csc->vertex_bufs[1].buffer_offset = 0;
	csc->vertex_bufs[1].buffer = pipe_buffer_create
	(
		pipe->screen,
		1,
		PIPE_BUFFER_USAGE_VERTEX,
		sizeof(struct vlVertex2f) * 4
	);

	memcpy
	(
		pipe_buffer_map(pipe->screen, csc->vertex_bufs[1].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		surface_texcoords,
		sizeof(struct vlVertex2f) * 4
	);

	pipe_buffer_unmap(pipe->screen, csc->vertex_bufs[1].buffer);

	csc->vertex_elems[1].src_offset = 0;
	csc->vertex_elems[1].vertex_buffer_index = 1;
	csc->vertex_elems[1].nr_components = 2;
	csc->vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/*
	 * Create our vertex shader's constant buffer
	 * Const buffer contains scaling and translation vectors
	 */
	csc->vs_const_buf.buffer = pipe_buffer_create
	(
		pipe->screen,
		1,
		PIPE_BUFFER_USAGE_CONSTANT | PIPE_BUFFER_USAGE_DISCARD,
		sizeof(struct vlVertexShaderConsts)
	);

	/*
	 * Create our fragment shader's constant buffer
	 * Const buffer contains the color conversion matrix and bias vectors
	 */
	csc->fs_const_buf.buffer = pipe_buffer_create
	(
		pipe->screen,
		1,
		PIPE_BUFFER_USAGE_CONSTANT,
		sizeof(struct vlFragmentShaderConsts)
	);

	/*
	 * TODO: Refactor this into a seperate function,
	 * allow changing the CSC matrix at runtime to switch between regular & full versions
	 */
	memcpy
	(
		pipe_buffer_map(pipe->screen, csc->fs_const_buf.buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		&bt_601_full,
		sizeof(struct vlFragmentShaderConsts)
	);

	pipe_buffer_unmap(pipe->screen, csc->fs_const_buf.buffer);

	return 0;
}

static int vlInit
(
	struct vlBasicCSC *csc
)
{
	struct pipe_context		*pipe;
	struct pipe_sampler_state	sampler;

	assert(csc);

	pipe = csc->pipe;

	/* Delay creating the FB until vlPutPictureCSC() so we know window size */
	csc->framebuffer_tex = NULL;
	csc->framebuffer.width = 0;
	csc->framebuffer.height = 0;
	csc->framebuffer.nr_cbufs = 1;
	csc->framebuffer.cbufs[0] = NULL;
	csc->framebuffer.zsbuf = NULL;

	sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
	sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
	sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
	sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
	sampler.compare_func = PIPE_FUNC_ALWAYS;
	sampler.normalized_coords = 1;
	/*sampler.prefilter = ;*/
	/*sampler.shadow_ambient = ;*/
	/*sampler.lod_bias = ;*/
	/*sampler.min_lod = ;*/
	/*sampler.max_lod = ;*/
	/*sampler.border_color[i] = ;*/
	/*sampler.max_anisotropy = ;*/
	csc->sampler = pipe->create_sampler_state(pipe, &sampler);

	vlCreateVertexShader(csc);
	vlCreateFragmentShader(csc);
	vlCreateDataBufs(csc);

	return 0;
}

int vlCreateBasicCSC
(
	struct pipe_context *pipe,
	struct vlCSC **csc
)
{
	struct vlBasicCSC *basic_csc;

	assert(pipe);
	assert(csc);

	basic_csc = CALLOC_STRUCT(vlBasicCSC);

	if (!basic_csc)
		return 1;

	basic_csc->base.vlResizeFrameBuffer = &vlResizeFrameBuffer;
	basic_csc->base.vlBegin = &vlBegin;
	basic_csc->base.vlPutPicture = &vlPutPictureCSC;
	basic_csc->base.vlEnd = &vlEnd;
	basic_csc->base.vlGetFrameBuffer = &vlGetFrameBuffer;
	basic_csc->base.vlDestroy = &vlDestroy;
	basic_csc->pipe = pipe;

	vlInit(basic_csc);

	*csc = &basic_csc->base;

	return 0;
}
