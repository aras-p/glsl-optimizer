#include "vl_surface.h"
#include <assert.h>
#include <stdlib.h>
#include <pipe/p_context.h>
#include <pipe/p_state.h>
#include <pipe/p_format.h>
#include <pipe/p_inlines.h>
#include "vl_context.h"
#include "vl_defs.h"

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
	unsigned int		b, x, y, y2;
	
	assert(context);
	assert(blocks);
	
	tex_surface = context->pipe->screen->get_tex_surface
	(
		context->pipe->screen,
		context->states.mc.textures[0],
		0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	texels = pipe_surface_map(tex_surface, 0);
	
	for (b = 0; b < 4; ++b)
	{
		if ((coded_block_pattern >> b) & 1)
		{
			if (dct_type == VL_DCT_FRAME_CODED)
			{
				if (sample_type == VL_FULL_SAMPLE)
				{
					for (y = VL_BLOCK_HEIGHT * b; y < VL_BLOCK_HEIGHT * (b + 1); ++y)
						memcpy
						(
							texels + y * tex_surface->pitch,
							blocks + y * VL_BLOCK_WIDTH,
							VL_BLOCK_WIDTH * 2
						);
				}
				else
				{
					for (y = VL_BLOCK_HEIGHT * b; y < VL_BLOCK_HEIGHT * (b + 1); ++y)
						for (x = 0; x < VL_BLOCK_WIDTH; ++x)
							texels[y * tex_surface->pitch + x] =
							blocks[y * VL_BLOCK_WIDTH + x] + 0x100;
				}
			}
			else
			{
				if (sample_type == VL_FULL_SAMPLE)
				{
					for
					(
						y = VL_BLOCK_HEIGHT * (b % 2), y2 = VL_BLOCK_HEIGHT * b;
						y < VL_BLOCK_HEIGHT * ((b % 2) + 1);
						y += 2, ++y2
					)
						memcpy
						(
							texels + y * tex_surface->pitch,
							blocks + y2 * VL_BLOCK_WIDTH,
							VL_BLOCK_WIDTH * 2
						);
					for
					(
						y = VL_BLOCK_HEIGHT * ((b % 2) + 2);
						y < VL_BLOCK_HEIGHT * (((b % 2) + 2) + 1);
						y += 2, ++y2
					)
						memcpy
						(
							texels + y * tex_surface->pitch,
							blocks + y2 * VL_BLOCK_WIDTH,
							VL_BLOCK_WIDTH * 2
						);
				}
				else
				{
					for
					(
						y = VL_BLOCK_HEIGHT * (b % 2), y2 = VL_BLOCK_HEIGHT * b;
						y < VL_BLOCK_HEIGHT * ((b % 2) + 1);
						y += 2, ++y2
					)
						for (x = 0; x < VL_BLOCK_WIDTH; ++x)
							texels[y * tex_surface->pitch + x] =
							blocks[y2 * VL_BLOCK_WIDTH + x] + 0x100;
					for
					(
						y = VL_BLOCK_HEIGHT * ((b % 2) + 2);
						y < VL_BLOCK_HEIGHT * (((b % 2) + 2) + 1);
						y += 2, ++y2
					)
						for (x = 0; x < VL_BLOCK_WIDTH; ++x)
							texels[y * tex_surface->pitch + x] =
							blocks[y2 * VL_BLOCK_WIDTH + x] + 0x100;
				}
			}
		}
		else
		{
			for (y = VL_BLOCK_HEIGHT * b; y < VL_BLOCK_HEIGHT * (b + 1); ++y)
			{
				for (x = 0; x < VL_BLOCK_WIDTH; ++x)
					texels[y * tex_surface->pitch + x] = 0x100;
			}
		}
	}
	
	pipe_surface_unmap(tex_surface);
	
	/* TODO: Implement 422, 444 */
	for (b = 0; b < 2; ++b)
	{
		tex_surface = context->pipe->screen->get_tex_surface
		(
			context->pipe->screen,
			context->states.mc.textures[b + 1],
			0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
		);
	
		texels = pipe_surface_map(tex_surface, 0);
		
		if ((coded_block_pattern >> (b + 4)) & 1)
		{
			if (sample_type == VL_FULL_SAMPLE)
			{
				for (y = 0; y < tex_surface->height; ++y)
					memcpy
					(
						texels + y * tex_surface->pitch,
						blocks + VL_BLOCK_SIZE * (b + 4) + y * VL_BLOCK_WIDTH,
						VL_BLOCK_WIDTH * 2
					);
			}
			else
			{
				for (y = 0; y < tex_surface->height; ++y)
					for (x = 0; x < VL_BLOCK_WIDTH; ++x)
						texels[y * tex_surface->pitch + x] =
						blocks[VL_BLOCK_SIZE * (b + 4) + y * VL_BLOCK_WIDTH + x] + 0x100;
			}
		}
		else
		{
			for (y = 0; y < tex_surface->height; ++y)
			{
				for (x = 0; x < VL_BLOCK_WIDTH; ++x)
					texels[y * tex_surface->pitch + x] = 0x100;
			}
		}
		
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
	sfc->width = context->video_width;
	sfc->height = context->video_height;
	sfc->format = context->video_format;
	
	memset(&template, 0, sizeof(struct pipe_texture));
	template.target = PIPE_TEXTURE_2D;
	template.format = PIPE_FORMAT_A8R8G8B8_UNORM;
	template.last_level = 0;
	template.width[0] = sfc->width;
	template.height[0] = sfc->height;
	template.depth[0] = 1;
	template.compressed = 0;
	template.cpp = 4;
	
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
	struct VL_MC_VS_CONSTS	*vscbdata;
	
	assert(blocks);
	assert(surface);
	
	/* TODO: Implement interlaced rendering */
	/*assert(picture_type == VL_FRAME_PICTURE);*/
	if (picture_type != VL_FRAME_PICTURE)
	{
		/*fprintf(stderr, "field picture (I) unimplemented, ignoring\n");*/
		return 0;
	}
	
	pipe = surface->context->pipe;
	
	vscbdata = pipe->winsys->buffer_map
	(
		pipe->winsys,
		surface->context->states.mc.vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	vscbdata->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->width;
	vscbdata->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->height;
	vscbdata->scale.z = 1.0f;
	vscbdata->scale.w = 1.0f;
	vscbdata->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->width;
	vscbdata->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->height;
	vscbdata->mb_pos_trans.z = 0.0f;
	vscbdata->mb_pos_trans.w = 0.0f;
	
	pipe->winsys->buffer_unmap(pipe->winsys, surface->context->states.mc.vs_const_buf.buffer);
	
	vlGrabBlocks(surface->context, coded_block_pattern, dct_type, VL_FULL_SAMPLE, blocks);
	
	surface->context->states.mc.render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_CPU_READ | PIPE_BUFFER_USAGE_CPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &surface->context->states.mc.render_target);
	pipe->set_sampler_textures(pipe, 3, surface->context->states.mc.textures);
	pipe->bind_sampler_states(pipe, 3, (void**)surface->context->states.mc.samplers);
	pipe->set_vertex_buffers(pipe, 4, surface->context->states.mc.vertex_bufs);
	pipe->set_vertex_elements(pipe, 4, surface->context->states.mc.vertex_buf_elems);
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
	struct VL_MC_VS_CONSTS	*vscbdata;
	
	assert(motion_vectors);
	assert(blocks);
	assert(ref_surface);
	assert(surface);
	
	/* TODO: Implement interlaced rendering */
	/*assert(picture_type == VL_FRAME_PICTURE);*/
	if (picture_type != VL_FRAME_PICTURE)
	{
		/*fprintf(stderr, "field picture (P) unimplemented, ignoring\n");*/
		return 0;
	}
	/* TODO: Implement field based motion compensation */
	/*assert(mc_type == VL_FRAME_MC);*/
	if (mc_type != VL_FRAME_MC)
	{
		/*fprintf(stderr, "field MC (P) unimplemented, ignoring\n");*/
		return 0;
	}
	
	pipe = surface->context->pipe;
	
	vscbdata = pipe->winsys->buffer_map
	(
		pipe->winsys,
		surface->context->states.mc.vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	vscbdata->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->width;
	vscbdata->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->height;
	vscbdata->scale.z = 1.0f;
	vscbdata->scale.w = 1.0f;
	vscbdata->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->width;
	vscbdata->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->height;
	vscbdata->mb_pos_trans.z = 0.0f;
	vscbdata->mb_pos_trans.w = 0.0f;
	vscbdata->mb_tc_trans[0].x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector->top_field.x * 0.5f) / (float)surface->width;
	vscbdata->mb_tc_trans[0].y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector->top_field.y * 0.5f) / (float)surface->height;
	vscbdata->mb_tc_trans[0].z = 0.0f;
	vscbdata->mb_tc_trans[0].w = 0.0f;
	
	pipe->winsys->buffer_unmap(pipe->winsys, surface->context->states.mc.vs_const_buf.buffer);
	
	vlGrabBlocks(surface->context, coded_block_pattern, dct_type, VL_DIFFERENCE_SAMPLE, blocks);
	
	surface->context->states.mc.render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_CPU_READ | PIPE_BUFFER_USAGE_CPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &surface->context->states.mc.render_target);
	
	surface->context->states.mc.textures[3] = ref_surface->texture;
	pipe->set_sampler_textures(pipe, 4, surface->context->states.mc.textures);
	pipe->bind_sampler_states(pipe, 4, (void**)surface->context->states.mc.samplers);
	pipe->set_vertex_buffers(pipe, 5, surface->context->states.mc.vertex_bufs);
	pipe->set_vertex_elements(pipe, 5, surface->context->states.mc.vertex_buf_elems);
	pipe->bind_vs_state(pipe, surface->context->states.mc.p_vs);
	pipe->bind_fs_state(pipe, surface->context->states.mc.p_fs);
	
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
	struct VL_MC_VS_CONSTS	*vscbdata;
	
	assert(motion_vectors);
	assert(blocks);
	assert(ref_surface);
	assert(surface);
	
	/* TODO: Implement interlaced rendering */
	/*assert(picture_type == VL_FRAME_PICTURE);*/
	if (picture_type != VL_FRAME_PICTURE)
	{
		/*fprintf(stderr, "field picture (B) unimplemented, ignoring\n");*/
		return 0;
	}
	/* TODO: Implement field based motion compensation */
	/*assert(mc_type == VL_FRAME_MC);*/
	if (mc_type != VL_FRAME_MC)
	{
		/*fprintf(stderr, "field MC (B) unimplemented, ignoring\n");*/
		return 0;
	}
	
	pipe = surface->context->pipe;
	
	vscbdata = pipe->winsys->buffer_map
	(
		pipe->winsys,
		surface->context->states.mc.vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);
	
	vscbdata->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->width;
	vscbdata->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->height;
	vscbdata->scale.z = 1.0f;
	vscbdata->scale.w = 1.0f;
	vscbdata->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->width;
	vscbdata->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->height;
	vscbdata->mb_pos_trans.z = 0.0f;
	vscbdata->mb_pos_trans.w = 0.0f;
	vscbdata->mb_tc_trans[0].x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector[0].top_field.x * 0.5f) / (float)surface->width;
	vscbdata->mb_tc_trans[0].y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector[0].top_field.y * 0.5f) / (float)surface->height;
	vscbdata->mb_tc_trans[0].z = 0.0f;
	vscbdata->mb_tc_trans[0].w = 0.0f;
	vscbdata->mb_tc_trans[1].x = (mbx * VL_MACROBLOCK_WIDTH + motion_vector[1].top_field.x * 0.5f) / (float)surface->width;
	vscbdata->mb_tc_trans[1].y = (mby * VL_MACROBLOCK_HEIGHT + motion_vector[1].top_field.y * 0.5f) / (float)surface->height;
	vscbdata->mb_tc_trans[1].z = 0.0f;
	vscbdata->mb_tc_trans[1].w = 0.0f;
	
	pipe->winsys->buffer_unmap(pipe->winsys, surface->context->states.mc.vs_const_buf.buffer);
	
	vlGrabBlocks(surface->context, coded_block_pattern, dct_type, VL_DIFFERENCE_SAMPLE, blocks);
	
	surface->context->states.mc.render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_CPU_READ | PIPE_BUFFER_USAGE_CPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &surface->context->states.mc.render_target);
	
	surface->context->states.mc.textures[3] = past_surface->texture;
	surface->context->states.mc.textures[4] = future_surface->texture;
	pipe->set_sampler_textures(pipe, 5, surface->context->states.mc.textures);
	pipe->bind_sampler_states(pipe, 5, (void**)surface->context->states.mc.samplers);
	pipe->set_vertex_buffers(pipe, 6, surface->context->states.mc.vertex_bufs);
	pipe->set_vertex_elements(pipe, 6, surface->context->states.mc.vertex_buf_elems);
	pipe->bind_vs_state(pipe, surface->context->states.mc.b_vs);
	pipe->bind_fs_state(pipe, surface->context->states.mc.b_fs);
	
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
			PIPE_BUFFER_USAGE_CPU_READ | PIPE_BUFFER_USAGE_CPU_WRITE,
			0
		);
	}
	
	vlEndRender(surface->context);
	
	pipe->set_sampler_textures(pipe, 1, &surface->texture);
	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLE_STRIP, 0, 4);
	pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);
	pipe->winsys->flush_frontbuffer
	(
		pipe->winsys,
		surface->context->states.csc.framebuffer.cbufs[0],
		&drawable
	);
	
	vlBeginRender(surface->context);
	
	return 0;
}

