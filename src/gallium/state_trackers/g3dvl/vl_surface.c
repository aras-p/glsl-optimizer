#include "vl_surface.h"
#include <assert.h>
#include <stdlib.h>
#include <pipe/p_context.h>
#include <pipe/p_state.h>
#include <pipe/p_format.h>
#include <pipe/p_inlines.h>
#include <vl_winsys.h>
#include "vl_context.h"
#include "vl_defs.h"
#include "vl_util.h"

static int vlTransformBlock(short *src, short *dst, short bias)
{
	static const float basis[8][8] =
	{
		{0.3536,   0.4904,   0.4619,   0.4157,   0.3536,   0.2778,   0.1913,   0.0975},
		{0.3536,   0.4157,   0.1913,  -0.0975,  -0.3536,  -0.4904,  -0.4619,  -0.2778},
		{0.3536,   0.2778,  -0.1913,  -0.4904,  -0.3536,   0.0975,   0.4619,   0.4157},
		{0.3536,   0.0975,  -0.4619,  -0.2778,   0.3536,   0.4157,  -0.1913,  -0.4904},
		{0.3536,  -0.0975,  -0.4619,   0.2778,   0.3536,  -0.4157,  -0.1913,   0.4904},
		{0.3536,  -0.2778,  -0.1913,   0.4904,  -0.3536,  -0.0975,   0.4619,  -0.4157},
		{0.3536,  -0.4157,   0.1913,   0.0975,  -0.3536,   0.4904,  -0.4619,   0.2778},
		{0.3536,  -0.4904,   0.4619,  -0.4157,   0.3536,  -0.2778,   0.1913,  -0.0975}
	};
	
	unsigned int	x, y;
	short		tmp[64];
	
	for (y = 0; y < VL_BLOCK_HEIGHT; ++y)
		for (x = 0; x < VL_BLOCK_WIDTH; ++x)
			tmp[y * VL_BLOCK_WIDTH + x] = (short)
			(
				src[y * VL_BLOCK_WIDTH + 0] * basis[x][0] +
				src[y * VL_BLOCK_WIDTH + 1] * basis[x][1] +
				src[y * VL_BLOCK_WIDTH + 2] * basis[x][2] +
				src[y * VL_BLOCK_WIDTH + 3] * basis[x][3] +
				src[y * VL_BLOCK_WIDTH + 4] * basis[x][4] +
				src[y * VL_BLOCK_WIDTH + 5] * basis[x][5] +
				src[y * VL_BLOCK_WIDTH + 6] * basis[x][6] +
				src[y * VL_BLOCK_WIDTH + 7] * basis[x][7]
			);

	for (x = 0; x < VL_BLOCK_WIDTH; ++x)
		for (y = 0; y < VL_BLOCK_HEIGHT; ++y)
		{
			dst[y * VL_BLOCK_WIDTH + x] = bias + (short)
			(
				tmp[0 * VL_BLOCK_WIDTH + x] * basis[y][0] +
				tmp[1 * VL_BLOCK_WIDTH + x] * basis[y][1] +
				tmp[2 * VL_BLOCK_WIDTH + x] * basis[y][2] +
				tmp[3 * VL_BLOCK_WIDTH + x] * basis[y][3] +
				tmp[4 * VL_BLOCK_WIDTH + x] * basis[y][4] +
				tmp[5 * VL_BLOCK_WIDTH + x] * basis[y][5] +
				tmp[6 * VL_BLOCK_WIDTH + x] * basis[y][6] +
				tmp[7 * VL_BLOCK_WIDTH + x] * basis[y][7]
			);
			if (dst[y * VL_BLOCK_WIDTH + x] > 255)
				dst[y * VL_BLOCK_WIDTH + x] = 255;
			else if (bias > 0 && dst[y * VL_BLOCK_WIDTH + x] < 0)
				dst[y * VL_BLOCK_WIDTH + x] = 0;
		}
	return 0;
}

static int vlGrabFrameCodedBlock(short *src, short *dst, unsigned int dst_pitch)
{
	unsigned int y;
	
	for (y = 0; y < VL_BLOCK_HEIGHT; ++y)
		memcpy
		(
			dst + y * dst_pitch,
			src + y * VL_BLOCK_WIDTH,
			VL_BLOCK_WIDTH * 2
		);
	
	return 0;
}

static int vlGrabFieldCodedBlock(short *src, short *dst, unsigned int dst_pitch)
{
	unsigned int y;
	
	for (y = 0; y < VL_BLOCK_HEIGHT / 2; ++y)
		memcpy
		(
			dst + y * dst_pitch * 2,
			src + y * VL_BLOCK_WIDTH,
			VL_BLOCK_WIDTH * 2
		);
	
	dst += VL_BLOCK_HEIGHT * dst_pitch;
	
	for (; y < VL_BLOCK_HEIGHT; ++y)
		memcpy
		(
			dst + y * dst_pitch * 2,
			src + y * VL_BLOCK_WIDTH,
			VL_BLOCK_WIDTH * 2
		);
	
	return 0;
}

static int vlGrabNoBlock(short *dst, unsigned int dst_pitch)
{
	unsigned int y;
	
	for (y = 0; y < VL_BLOCK_HEIGHT; ++y)
		memset
		(
			dst + y * dst_pitch,
			0,
			VL_BLOCK_WIDTH * 2
		);
	
	return 0;
}

static int vlGrabBlocks
(
	struct VL_CONTEXT *context,
	unsigned int coded_block_pattern,
	enum VL_DCT_TYPE dct_type,
	enum VL_SAMPLE_TYPE sample_type,
	short *blocks
)
{
	struct pipe_surface	*tex_surface;
	short			*texels;
	unsigned int		tex_pitch;
	unsigned int		tb, sb = 0;
	
	short			temp_block[64];
	
	assert(context);
	assert(blocks);
	
	tex_surface = context->pipe->screen->get_tex_surface
	(
		context->pipe->screen,
		context->states.mc.textures[0],
		0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	texels = pipe_surface_map(tex_surface, PIPE_BUFFER_USAGE_CPU_WRITE);
	tex_pitch = tex_surface->stride / tex_surface->block.size;
	
	for (tb = 0; tb < 4; ++tb)
	{
		if ((coded_block_pattern >> (5 - tb)) & 1)
		{
			if (sample_type == VL_FULL_SAMPLE)
				vlTransformBlock(blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT, temp_block, 128);
			else
				vlTransformBlock(blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT, temp_block, 0);
			
			if (dct_type == VL_DCT_FRAME_CODED)
				vlGrabFrameCodedBlock
				(
					temp_block,
					texels + tb * tex_pitch * VL_BLOCK_HEIGHT,
					tex_pitch
				);
			else
				vlGrabFieldCodedBlock
				(
					blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT,
					texels + (tb % 2) * tex_pitch * VL_BLOCK_HEIGHT + (tb / 2) * tex_pitch,
					tex_pitch
				);
			
			++sb;
		}
		else
			vlGrabNoBlock(texels + tb * tex_pitch * VL_BLOCK_HEIGHT, tex_pitch);
	}
	
	pipe_surface_unmap(tex_surface);
	
	/* TODO: Implement 422, 444 */
	for (tb = 0; tb < 2; ++tb)
	{
		tex_surface = context->pipe->screen->get_tex_surface
			(
				context->pipe->screen,
				context->states.mc.textures[tb + 1],
				0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
			);
	
		texels = pipe_surface_map(tex_surface, PIPE_BUFFER_USAGE_CPU_WRITE);
		tex_pitch = tex_surface->stride / tex_surface->block.size;
		
		if ((coded_block_pattern >> (1 - tb)) & 1)
		{
			if (sample_type == VL_FULL_SAMPLE)
				vlTransformBlock(blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT, temp_block, 128);
			else
				vlTransformBlock(blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT, temp_block, 0);
			
			vlGrabFrameCodedBlock
			(
				temp_block,
				texels,
				tex_pitch
			);
			
			++sb;
		}
		else
			vlGrabNoBlock(texels, tex_pitch);
		
		pipe_surface_unmap(tex_surface);
	}
	
	return 0;
}

int vlCreateSurface(struct VL_CONTEXT *context, struct VL_SURFACE **surface)
{
	struct pipe_context	*pipe;
	struct pipe_texture	template;
	struct VL_SURFACE	*sfc;
	
	assert(context);
	assert(surface);
	
	pipe = context->pipe;
	
	sfc = calloc(1, sizeof(struct VL_SURFACE));
	
	sfc->context = context;
	sfc->width = vlRoundUpPOT(context->video_width);
	sfc->height = vlRoundUpPOT(context->video_height);
	sfc->format = context->video_format;
	
	memset(&template, 0, sizeof(struct pipe_texture));
	template.target = PIPE_TEXTURE_2D;
	template.format = PIPE_FORMAT_A8R8G8B8_UNORM;
	template.last_level = 0;
	template.width[0] = sfc->width;
	template.height[0] = sfc->height;
	template.depth[0] = 1;
	template.compressed = 0;
	pf_get_block(template.format, &template.block);
	/* XXX: Needed? */
	template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER | PIPE_TEXTURE_USAGE_RENDER_TARGET;
	
	sfc->texture = pipe->screen->texture_create(pipe->screen, &template);
	
	*surface = sfc;
	
	return 0;
}

int vlDestroySurface(struct VL_SURFACE *surface)
{
	assert(surface);
	pipe_texture_release(&surface->texture);
	free(surface);
	
	return 0;
}

int vlRenderIMacroBlock
(
	enum VL_PICTURE picture_type,
	enum VL_FIELD_ORDER field_order,
	unsigned int mbx,
	unsigned int mby,
	unsigned int coded_block_pattern,
	enum VL_DCT_TYPE dct_type,
	short *blocks,
	struct VL_SURFACE *surface
)
{
	struct pipe_context	*pipe;
	struct VL_MC_VS_CONSTS	*vs_consts;
	
	assert(blocks);
	assert(surface);
	
	/* TODO: Implement interlaced rendering */
	if (picture_type != VL_FRAME_PICTURE)
		return 0;
	
	vlGrabBlocks(surface->context, coded_block_pattern, dct_type, VL_FULL_SAMPLE, blocks);
	
	pipe = surface->context->pipe;
	
	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		surface->context->states.mc.vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	vs_consts->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->width;
	vs_consts->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->height;
	vs_consts->scale.z = 1.0f;
	vs_consts->scale.w = 1.0f;
	vs_consts->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->width;
	vs_consts->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->height;
	vs_consts->mb_pos_trans.z = 0.0f;
	vs_consts->mb_pos_trans.w = 0.0f;
	
	pipe->winsys->buffer_unmap(pipe->winsys, surface->context->states.mc.vs_const_buf.buffer);
	
	surface->context->states.mc.render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &surface->context->states.mc.render_target);
	pipe->set_sampler_textures(pipe, 3, surface->context->states.mc.textures);
	pipe->bind_sampler_states(pipe, 3, (void**)surface->context->states.mc.samplers);
	pipe->bind_vs_state(pipe, surface->context->states.mc.i_vs);
	pipe->bind_fs_state(pipe, surface->context->states.mc.i_fs);
	
	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, 24);
	
	return 0;
}

int vlRenderPMacroBlock
(
	enum VL_PICTURE picture_type,
	enum VL_FIELD_ORDER field_order,
	unsigned int mbx,
	unsigned int mby,
	enum VL_MC_TYPE mc_type,
	struct VL_MOTION_VECTOR *motion_vector,
	unsigned int coded_block_pattern,
	enum VL_DCT_TYPE dct_type,
	short *blocks,
	struct VL_SURFACE *ref_surface,
	struct VL_SURFACE *surface
)
{
	struct pipe_context	*pipe;
	struct VL_MC_VS_CONSTS	*vs_consts;
	
	assert(motion_vectors);
	assert(blocks);
	assert(ref_surface);
	assert(surface);
	
	/* TODO: Implement interlaced rendering */
	if (picture_type != VL_FRAME_PICTURE)
		return 0;
	/* TODO: Implement other MC types */
	if (mc_type != VL_FRAME_MC && mc_type != VL_FIELD_MC)
		return 0;
	
	vlGrabBlocks(surface->context, coded_block_pattern, dct_type, VL_DIFFERENCE_SAMPLE, blocks);
	
	pipe = surface->context->pipe;
	
	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		surface->context->states.mc.vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	vs_consts->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->width;
	vs_consts->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->height;
	vs_consts->scale.z = 1.0f;
	vs_consts->scale.w = 1.0f;
	vs_consts->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->width;
	vs_consts->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->height;
	vs_consts->mb_pos_trans.z = 0.0f;
	vs_consts->mb_pos_trans.w = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector->top_field.x * 0.5f) / (float)surface->width;
	vs_consts->mb_tc_trans[0].top_field.y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector->top_field.y * 0.5f) / (float)surface->height;
	vs_consts->mb_tc_trans[0].top_field.z = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.w = 0.0f;
	
	if (mc_type == VL_FIELD_MC)
	{
		vs_consts->denorm.x = (float)surface->width;
		vs_consts->denorm.y = (float)surface->height;
		
		vs_consts->mb_tc_trans[0].bottom_field.x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector->bottom_field.x * 0.5f) / (float)surface->width;
		vs_consts->mb_tc_trans[0].bottom_field.y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector->bottom_field.y * 0.5f) / (float)surface->height;
		vs_consts->mb_tc_trans[0].bottom_field.z = 0.0f;
		vs_consts->mb_tc_trans[0].bottom_field.w = 0.0f;
		
		pipe->bind_vs_state(pipe, surface->context->states.mc.p_vs[1]);
		pipe->bind_fs_state(pipe, surface->context->states.mc.p_fs[1]);
	}
	else
	{
		pipe->bind_vs_state(pipe, surface->context->states.mc.p_vs[0]);
		pipe->bind_fs_state(pipe, surface->context->states.mc.p_fs[0]);
	}
	
	pipe->winsys->buffer_unmap(pipe->winsys, surface->context->states.mc.vs_const_buf.buffer);
	
	surface->context->states.mc.render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &surface->context->states.mc.render_target);
	
	surface->context->states.mc.textures[3] = ref_surface->texture;
	pipe->set_sampler_textures(pipe, 4, surface->context->states.mc.textures);
	pipe->bind_sampler_states(pipe, 4, (void**)surface->context->states.mc.samplers);
	
	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, 24);
	
	return 0;
}

int vlRenderBMacroBlock
(
	enum VL_PICTURE picture_type,
	enum VL_FIELD_ORDER field_order,
	unsigned int mbx,
	unsigned int mby,
	enum VL_MC_TYPE mc_type,
	struct VL_MOTION_VECTOR *motion_vector,
	unsigned int coded_block_pattern,
	enum VL_DCT_TYPE dct_type,
	short *blocks,
	struct VL_SURFACE *past_surface,
	struct VL_SURFACE *future_surface,
	struct VL_SURFACE *surface
)
{
	struct pipe_context	*pipe;
	struct VL_MC_VS_CONSTS	*vs_consts;
	
	assert(motion_vectors);
	assert(blocks);
	assert(ref_surface);
	assert(surface);
	
	/* TODO: Implement interlaced rendering */
	if (picture_type != VL_FRAME_PICTURE)
		return 0;
	/* TODO: Implement other MC types */
	if (mc_type != VL_FRAME_MC && mc_type != VL_FIELD_MC)
		return 0;
	
	vlGrabBlocks(surface->context, coded_block_pattern, dct_type, VL_DIFFERENCE_SAMPLE, blocks);
	
	pipe = surface->context->pipe;
	
	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		surface->context->states.mc.vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	vs_consts->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->width;
	vs_consts->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->height;
	vs_consts->scale.z = 1.0f;
	vs_consts->scale.w = 1.0f;
	vs_consts->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->width;
	vs_consts->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->height;
	vs_consts->mb_pos_trans.z = 0.0f;
	vs_consts->mb_pos_trans.w = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector[0].top_field.x * 0.5f) / (float)surface->width;
	vs_consts->mb_tc_trans[0].top_field.y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector[0].top_field.y * 0.5f) / (float)surface->height;
	vs_consts->mb_tc_trans[0].top_field.z = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.w = 0.0f;
	vs_consts->mb_tc_trans[1].top_field.x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector[1].top_field.x * 0.5f) / (float)surface->width;
	vs_consts->mb_tc_trans[1].top_field.y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector[1].top_field.y * 0.5f) / (float)surface->height;
	vs_consts->mb_tc_trans[1].top_field.z = 0.0f;
	vs_consts->mb_tc_trans[1].top_field.w = 0.0f;
	
	if (mc_type == VL_FIELD_MC)
	{
		vs_consts->denorm.x = (float)surface->width;
		vs_consts->denorm.y = (float)surface->height;
		
		vs_consts->mb_tc_trans[0].bottom_field.x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector[0].bottom_field.x * 0.5f) / (float)surface->width;
		vs_consts->mb_tc_trans[0].bottom_field.y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector[0].bottom_field.y * 0.5f) / (float)surface->height;
		vs_consts->mb_tc_trans[0].bottom_field.z = 0.0f;
		vs_consts->mb_tc_trans[0].bottom_field.w = 0.0f;
		vs_consts->mb_tc_trans[1].bottom_field.x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector[1].bottom_field.x * 0.5f) / (float)surface->width;
		vs_consts->mb_tc_trans[1].bottom_field.y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector[1].bottom_field.y * 0.5f) / (float)surface->height;
		vs_consts->mb_tc_trans[1].bottom_field.z = 0.0f;
		vs_consts->mb_tc_trans[1].bottom_field.w = 0.0f;
		
		pipe->bind_vs_state(pipe, surface->context->states.mc.b_vs[1]);
		pipe->bind_fs_state(pipe, surface->context->states.mc.b_fs[1]);
	}
	else
	{
		pipe->bind_vs_state(pipe, surface->context->states.mc.b_vs[0]);
		pipe->bind_fs_state(pipe, surface->context->states.mc.b_fs[0]);
	}
	
	pipe->winsys->buffer_unmap(pipe->winsys, surface->context->states.mc.vs_const_buf.buffer);
	
	surface->context->states.mc.render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &surface->context->states.mc.render_target);
	
	surface->context->states.mc.textures[3] = past_surface->texture;
	surface->context->states.mc.textures[4] = future_surface->texture;
	pipe->set_sampler_textures(pipe, 5, surface->context->states.mc.textures);
	pipe->bind_sampler_states(pipe, 5, (void**)surface->context->states.mc.samplers);
	
	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, 24);
	
	return 0;
}

int vlPutSurface
(
	struct VL_SURFACE *surface,
	Drawable drawable,
	unsigned int srcx,
	unsigned int srcy,
	unsigned int srcw,
	unsigned int srch,
	unsigned int destx,
	unsigned int desty,
	unsigned int destw,
	unsigned int desth,
	enum VL_PICTURE picture_type
)
{
	unsigned int		create_fb = 0;
	struct pipe_context	*pipe;
	struct VL_CSC_VS_CONSTS	*vs_consts;
	
	assert(surface);
	
	pipe = surface->context->pipe;
	
	if (!surface->context->states.csc.framebuffer.cbufs[0])
		create_fb = 1;
	else if
	(
		surface->context->states.csc.framebuffer.width != destw ||
		surface->context->states.csc.framebuffer.height != desth
	)
	{
		pipe->winsys->surface_release
		(
			pipe->winsys,
			&surface->context->states.csc.framebuffer.cbufs[0]
		);
		
		create_fb = 1;
	}
	
	if (create_fb)
	{
		surface->context->states.csc.viewport.scale[0] = destw;
		surface->context->states.csc.viewport.scale[1] = desth;
		surface->context->states.csc.viewport.scale[2] = 1;
		surface->context->states.csc.viewport.scale[3] = 1;
		surface->context->states.csc.viewport.translate[0] = 0;
		surface->context->states.csc.viewport.translate[1] = 0;
		surface->context->states.csc.viewport.translate[2] = 0;
		surface->context->states.csc.viewport.translate[3] = 0;
		
		surface->context->states.csc.framebuffer.width = destw;
		surface->context->states.csc.framebuffer.height = desth;
		surface->context->states.csc.framebuffer.cbufs[0] = pipe->winsys->surface_alloc(pipe->winsys);
		pipe->winsys->surface_alloc_storage
		(
			pipe->winsys,
			surface->context->states.csc.framebuffer.cbufs[0],
			destw,
			desth,
			PIPE_FORMAT_A8R8G8B8_UNORM,
			/* XXX: SoftPipe doesn't change GPU usage to CPU like it does for textures */
			PIPE_BUFFER_USAGE_CPU_READ | PIPE_BUFFER_USAGE_CPU_WRITE,
			0
		);
	}
	
	vlEndRender(surface->context);
	
	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		surface->context->states.csc.vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	vs_consts->src_scale.x = srcw / (float)surface->width;
	vs_consts->src_scale.y = srch / (float)surface->height;
	vs_consts->src_scale.z = 1;
	vs_consts->src_scale.w = 1;
	vs_consts->src_trans.x = srcx / (float)surface->width;
	vs_consts->src_trans.y = srcy / (float)surface->height;
	vs_consts->src_trans.z = 0;
	vs_consts->src_trans.w = 0;
	
	pipe->winsys->buffer_unmap(pipe->winsys, surface->context->states.csc.vs_const_buf.buffer);
	
	pipe->set_sampler_textures(pipe, 1, &surface->texture);
	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLE_STRIP, 0, 4);
	pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);
	bind_pipe_drawable(pipe, drawable);
	/* TODO: Need to take destx, desty into consideration */
	pipe->winsys->flush_frontbuffer
	(
		pipe->winsys,
		surface->context->states.csc.framebuffer.cbufs[0],
		pipe->priv
	);
	
	vlBeginRender(surface->context);
	
	return 0;
}

