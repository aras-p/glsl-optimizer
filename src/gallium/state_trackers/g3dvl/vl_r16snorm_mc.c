#define VL_INTERNAL
#include "vl_r16snorm_mc.h"
#include <assert.h>
#include <stdlib.h>
#include <pipe/p_context.h>
#include <pipe/p_winsys.h>
#include <pipe/p_state.h>
#include <pipe/p_inlines.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>
#include "vl_render.h"
#include "vl_shader_build.h"
#include "vl_surface.h"
#include "vl_util.h"
#include "vl_types.h"
#include "vl_defs.h"

#define NUM_BUFS 4	/* Number of rotating buffers to use */

struct vlVertexShaderConsts
{
	/*struct vlVertex4f scale;
	struct vlVertex4f denorm;*/
	struct vlVertex4f	scale;
	struct vlVertex4f	mb_pos_trans;
	struct vlVertex4f	denorm;
	struct
	{
		struct vlVertex4f	top_field;
		struct vlVertex4f	bottom_field;
	} mb_tc_trans[2];
};

struct vlFragmentShaderConsts
{
	struct vlVertex4f multiplier;
	struct vlVertex4f div;
};

struct vlR16SnormMC
{
	struct vlRender				base;

	unsigned int				video_width, video_height;
	enum vlFormat				video_format;
	unsigned int				cur_buf;

	struct pipe_context			*pipe;
	struct pipe_viewport_state		viewport;
	struct pipe_framebuffer_state		render_target;
	struct pipe_sampler_state		*samplers[5];
	struct pipe_texture			*textures[NUM_BUFS][5];
	void					*i_vs, *p_vs[2], *b_vs[2];
	void					*i_fs, *p_fs[2], *b_fs[2];
	struct pipe_vertex_buffer 		vertex_bufs[3];
	struct pipe_vertex_element		vertex_elems[3];
	struct pipe_constant_buffer		vs_const_buf, fs_const_buf;
};

static int vlBegin
(
	struct vlRender *render
)
{
	struct vlR16SnormMC	*mc;
	struct pipe_context	*pipe;

	assert(render);

	mc = (struct vlR16SnormMC*)render;
	pipe = mc->pipe;

	/* Frame buffer set in vlRender*Macroblock() */
	/* Shaders, samplers, textures set in vlRender*Macroblock() */
	pipe->set_vertex_buffers(pipe, 3, mc->vertex_bufs);
	pipe->set_vertex_elements(pipe, 3, mc->vertex_elems);
	pipe->set_viewport_state(pipe, &mc->viewport);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_VERTEX, 0, &mc->vs_const_buf);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, 0, &mc->fs_const_buf);

	return 0;
}

/*static int vlGrabMacroBlock
(
	struct vlR16SnormMC *mc,
	struct vlMpeg2MacroBlock *macroblock
)
{
	assert(mc);
	assert(macroblock);



	return 0;
}*/

/*#define DO_IDCT*/

#ifdef DO_IDCT
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
#endif

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

enum vlSampleType
{
	vlSampleTypeFull,
	vlSampleTypeDiff
};

static int vlGrabBlocks
(
	struct vlR16SnormMC *mc,
	unsigned int coded_block_pattern,
	enum vlDCTType dct_type,
	enum vlSampleType sample_type,
	short *blocks
)
{
	struct pipe_surface	*tex_surface;
	short			*texels;
	unsigned int		tex_pitch;
	unsigned int		tb, sb = 0;

	assert(mc);
	assert(blocks);

	tex_surface = mc->pipe->screen->get_tex_surface
	(
		mc->pipe->screen,
		mc->textures[mc->cur_buf % NUM_BUFS][0],
		0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
	);

	texels = pipe_surface_map(tex_surface, PIPE_BUFFER_USAGE_CPU_WRITE);
	tex_pitch = tex_surface->stride / tex_surface->block.size;

	for (tb = 0; tb < 4; ++tb)
	{
		if ((coded_block_pattern >> (5 - tb)) & 1)
		{
			short *cur_block = blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT;

#ifdef DO_IDCT
			vlTransformBlock(cur_block, cur_block, sample_type == vlSampleTypeFull ? 128 : 0);
#endif

			if (dct_type == vlDCTTypeFrameCoded)
				vlGrabFrameCodedBlock
				(
					cur_block,
					texels + tb * tex_pitch * VL_BLOCK_HEIGHT,
					tex_pitch
				);
			else
				vlGrabFieldCodedBlock
				(
					cur_block,
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
		tex_surface = mc->pipe->screen->get_tex_surface
		(
			mc->pipe->screen,
			mc->textures[mc->cur_buf % NUM_BUFS][tb + 1],
			0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
		);

		texels = pipe_surface_map(tex_surface, PIPE_BUFFER_USAGE_CPU_WRITE);
		tex_pitch = tex_surface->stride / tex_surface->block.size;

		if ((coded_block_pattern >> (1 - tb)) & 1)
		{
			short *cur_block = blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT;

#ifdef DO_IDCT
			vlTransformBlock(cur_block, cur_block, sample_type == vlSampleTypeFull ? 128 : 0);
#endif

			vlGrabFrameCodedBlock
			(
				cur_block,
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

static int vlRenderIMacroBlock
(
	struct vlR16SnormMC *mc,
	enum vlPictureType picture_type,
	enum vlFieldOrder field_order,
	unsigned int mbx,
	unsigned int mby,
	unsigned int coded_block_pattern,
	enum vlDCTType dct_type,
	short *blocks,
	struct vlSurface *surface
)
{
	struct pipe_context		*pipe;
	struct vlVertexShaderConsts	*vs_consts;

	assert(blocks);
	assert(surface);

	/* TODO: Implement interlaced rendering */
	if (picture_type != vlPictureTypeFrame)
		return 0;

	vlGrabBlocks(mc, coded_block_pattern, dct_type, vlSampleTypeFull, blocks);

	pipe = mc->pipe;

	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		mc->vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);

	vs_consts->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->texture->width[0];
	vs_consts->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->texture->height[0];
	vs_consts->scale.z = 1.0f;
	vs_consts->scale.w = 1.0f;
	vs_consts->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->texture->width[0];
	vs_consts->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->texture->height[0];
	vs_consts->mb_pos_trans.z = 0.0f;
	vs_consts->mb_pos_trans.w = 0.0f;

	pipe->winsys->buffer_unmap(pipe->winsys, mc->vs_const_buf.buffer);

	mc->render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &mc->render_target);
	pipe->set_sampler_textures(pipe, 3, mc->textures[mc->cur_buf % NUM_BUFS]);
	pipe->bind_sampler_states(pipe, 3, (void**)mc->samplers);
	pipe->bind_vs_state(pipe, mc->i_vs);
	pipe->bind_fs_state(pipe, mc->i_fs);

	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, 24);

	mc->cur_buf++;

	return 0;
}

static int vlRenderPMacroBlock
(
	struct vlR16SnormMC *mc,
	enum vlPictureType picture_type,
	enum vlFieldOrder field_order,
	unsigned int mbx,
	unsigned int mby,
	enum vlMotionType mc_type,
	short top_x,
	short top_y,
	short bottom_x,
	short bottom_y,
	unsigned int coded_block_pattern,
	enum vlDCTType dct_type,
	short *blocks,
	struct vlSurface *ref_surface,
	struct vlSurface *surface
)
{
	struct pipe_context		*pipe;
	struct vlVertexShaderConsts	*vs_consts;

	assert(motion_vectors);
	assert(blocks);
	assert(ref_surface);
	assert(surface);

	/* TODO: Implement interlaced rendering */
	if (picture_type != vlPictureTypeFrame)
		return 0;
	/* TODO: Implement other MC types */
	if (mc_type != vlMotionTypeFrame && mc_type != vlMotionTypeField)
		return 0;

	vlGrabBlocks(mc, coded_block_pattern, dct_type, vlSampleTypeDiff, blocks);

	pipe = mc->pipe;

	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		mc->vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);

	vs_consts->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->texture->width[0];
	vs_consts->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->texture->height[0];
	vs_consts->scale.z = 1.0f;
	vs_consts->scale.w = 1.0f;
	vs_consts->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->texture->width[0];
	vs_consts->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->texture->height[0];
	vs_consts->mb_pos_trans.z = 0.0f;
	vs_consts->mb_pos_trans.w = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.x = (mbx * VL_MACROBLOCK_WIDTH + top_x * 0.5f) / (float)surface->texture->width[0];
	vs_consts->mb_tc_trans[0].top_field.y = (mby * VL_MACROBLOCK_HEIGHT + top_y * 0.5f) / (float)surface->texture->height[0];
	vs_consts->mb_tc_trans[0].top_field.z = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.w = 0.0f;

	if (mc_type == vlMotionTypeField)
	{
		vs_consts->denorm.x = (float)surface->texture->width[0];
		vs_consts->denorm.y = (float)surface->texture->height[0];

		vs_consts->mb_tc_trans[0].bottom_field.x = (mbx * VL_MACROBLOCK_WIDTH + bottom_x * 0.5f) / (float)surface->texture->width[0];
		vs_consts->mb_tc_trans[0].bottom_field.y = (mby * VL_MACROBLOCK_HEIGHT + bottom_y * 0.5f) / (float)surface->texture->height[0];
		vs_consts->mb_tc_trans[0].bottom_field.z = 0.0f;
		vs_consts->mb_tc_trans[0].bottom_field.w = 0.0f;

		pipe->bind_vs_state(pipe, mc->p_vs[1]);
		pipe->bind_fs_state(pipe, mc->p_fs[1]);
	}
	else
	{
		pipe->bind_vs_state(pipe, mc->p_vs[0]);
		pipe->bind_fs_state(pipe, mc->p_fs[0]);
	}

	pipe->winsys->buffer_unmap(pipe->winsys, mc->vs_const_buf.buffer);

	mc->render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &mc->render_target);

	mc->textures[mc->cur_buf % NUM_BUFS][3] = ref_surface->texture;
	pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUFS]);
	pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);

	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, 24);

	mc->cur_buf++;

	return 0;
}

static int vlRenderBMacroBlock
(
	struct vlR16SnormMC *mc,
	enum vlPictureType picture_type,
	enum vlFieldOrder field_order,
	unsigned int mbx,
	unsigned int mby,
	enum vlMotionType mc_type,
	short top_past_x,
	short top_past_y,
	short bottom_past_x,
	short bottom_past_y,
	short top_future_x,
	short top_future_y,
	short bottom_future_x,
	short bottom_future_y,
	unsigned int coded_block_pattern,
	enum vlDCTType dct_type,
	short *blocks,
	struct vlSurface *past_surface,
	struct vlSurface *future_surface,
	struct vlSurface *surface
)
{
	struct pipe_context		*pipe;
	struct vlVertexShaderConsts	*vs_consts;

	assert(motion_vectors);
	assert(blocks);
	assert(ref_surface);
	assert(surface);

	/* TODO: Implement interlaced rendering */
	if (picture_type != vlPictureTypeFrame)
		return 0;
	/* TODO: Implement other MC types */
	if (mc_type != vlMotionTypeFrame && mc_type != vlMotionTypeField)
		return 0;

	vlGrabBlocks(mc, coded_block_pattern, dct_type, vlSampleTypeDiff, blocks);

	pipe = mc->pipe;

	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		mc->vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);

	vs_consts->scale.x = VL_MACROBLOCK_WIDTH / (float)surface->texture->width[0];
	vs_consts->scale.y = VL_MACROBLOCK_HEIGHT / (float)surface->texture->height[0];
	vs_consts->scale.z = 1.0f;
	vs_consts->scale.w = 1.0f;
	vs_consts->mb_pos_trans.x = (mbx * VL_MACROBLOCK_WIDTH) / (float)surface->texture->width[0];
	vs_consts->mb_pos_trans.y = (mby * VL_MACROBLOCK_HEIGHT) / (float)surface->texture->height[0];
	vs_consts->mb_pos_trans.z = 0.0f;
	vs_consts->mb_pos_trans.w = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.x = (mbx * VL_MACROBLOCK_WIDTH + top_past_x * 0.5f) / (float)surface->texture->width[0];
	vs_consts->mb_tc_trans[0].top_field.y = (mby * VL_MACROBLOCK_HEIGHT + top_past_y * 0.5f) / (float)surface->texture->height[0];
	vs_consts->mb_tc_trans[0].top_field.z = 0.0f;
	vs_consts->mb_tc_trans[0].top_field.w = 0.0f;
	vs_consts->mb_tc_trans[1].top_field.x = (mbx * VL_MACROBLOCK_WIDTH + top_future_x * 0.5f) / (float)surface->texture->width[0];
	vs_consts->mb_tc_trans[1].top_field.y = (mby * VL_MACROBLOCK_HEIGHT + top_future_y * 0.5f) / (float)surface->texture->height[0];
	vs_consts->mb_tc_trans[1].top_field.z = 0.0f;
	vs_consts->mb_tc_trans[1].top_field.w = 0.0f;

	if (mc_type == vlMotionTypeField)
	{
		vs_consts->denorm.x = (float)surface->texture->width[0];
		vs_consts->denorm.y = (float)surface->texture->height[0];

		vs_consts->mb_tc_trans[0].bottom_field.x = (mbx * VL_MACROBLOCK_WIDTH + bottom_past_x * 0.5f) / (float)surface->texture->width[0];
		vs_consts->mb_tc_trans[0].bottom_field.y = (mby * VL_MACROBLOCK_HEIGHT + bottom_past_y * 0.5f) / (float)surface->texture->height[0];
		vs_consts->mb_tc_trans[0].bottom_field.z = 0.0f;
		vs_consts->mb_tc_trans[0].bottom_field.w = 0.0f;
		vs_consts->mb_tc_trans[1].bottom_field.x = (mbx * VL_MACROBLOCK_WIDTH + bottom_future_x * 0.5f) / (float)surface->texture->width[0];
		vs_consts->mb_tc_trans[1].bottom_field.y = (mby * VL_MACROBLOCK_HEIGHT + bottom_future_y * 0.5f) / (float)surface->texture->height[0];
		vs_consts->mb_tc_trans[1].bottom_field.z = 0.0f;
		vs_consts->mb_tc_trans[1].bottom_field.w = 0.0f;

		pipe->bind_vs_state(pipe, mc->b_vs[1]);
		pipe->bind_fs_state(pipe, mc->b_fs[1]);
	}
	else
	{
		pipe->bind_vs_state(pipe, mc->b_vs[0]);
		pipe->bind_fs_state(pipe, mc->b_fs[0]);
	}

	pipe->winsys->buffer_unmap(pipe->winsys, mc->vs_const_buf.buffer);

	mc->render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);
	pipe->set_framebuffer_state(pipe, &mc->render_target);

	mc->textures[mc->cur_buf % NUM_BUFS][3] = past_surface->texture;
	mc->textures[mc->cur_buf % NUM_BUFS][4] = future_surface->texture;
	pipe->set_sampler_textures(pipe, 5, mc->textures[mc->cur_buf % NUM_BUFS]);
	pipe->bind_sampler_states(pipe, 5, (void**)mc->samplers);

	pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, 24);

	mc->cur_buf++;

	return 0;
}

static int vlRenderMacroBlocksMpeg2R16Snorm
(
	struct vlRender *render,
	struct vlMpeg2MacroBlockBatch *batch,
	struct vlSurface *surface
)
{
	struct vlR16SnormMC	*mc;
	unsigned int		i;

	assert(render);

	mc = (struct vlR16SnormMC*)render;

	/*for (i = 0; i < batch->num_macroblocks; ++i)
		vlGrabMacroBlock(batch->macroblocks[i]);*/

	for (i = 0; i < batch->num_macroblocks; ++i)
	{
		switch (batch->macroblocks[i].mb_type)
		{
			case vlMacroBlockTypeIntra:
			{
				vlRenderIMacroBlock
				(
					mc,
					batch->picture_type,
					batch->field_order,
					batch->macroblocks[i].mbx,
					batch->macroblocks[i].mby,
					batch->macroblocks[i].cbp,
					batch->macroblocks[i].dct_type,
					batch->macroblocks[i].blocks,
					surface
				);
				break;
			}
			case vlMacroBlockTypeFwdPredicted:
			{
				vlRenderPMacroBlock
				(
					mc,
					batch->picture_type,
					batch->field_order,
					batch->macroblocks[i].mbx,
					batch->macroblocks[i].mby,
					batch->macroblocks[i].mo_type,
					batch->macroblocks[i].PMV[0][0][0],
					batch->macroblocks[i].PMV[0][0][1],
					batch->macroblocks[i].PMV[1][0][0],
					batch->macroblocks[i].PMV[1][0][1],
					batch->macroblocks[i].cbp,
					batch->macroblocks[i].dct_type,
					batch->macroblocks[i].blocks,
					batch->past_surface,
					surface
				);
				break;
			}
			case vlMacroBlockTypeBkwdPredicted:
			{
				vlRenderPMacroBlock
				(
					mc,
					batch->picture_type,
					batch->field_order,
					batch->macroblocks[i].mbx,
					batch->macroblocks[i].mby,
					batch->macroblocks[i].mo_type,
					batch->macroblocks[i].PMV[0][1][0],
					batch->macroblocks[i].PMV[0][1][1],
					batch->macroblocks[i].PMV[1][1][0],
					batch->macroblocks[i].PMV[1][1][1],
					batch->macroblocks[i].cbp,
					batch->macroblocks[i].dct_type,
					batch->macroblocks[i].blocks,
					batch->future_surface,
					surface
				);
				break;
			}
			case vlMacroBlockTypeBiPredicted:
			{
				vlRenderBMacroBlock
				(
					mc,
					batch->picture_type,
					batch->field_order,
					batch->macroblocks[i].mbx,
					batch->macroblocks[i].mby,
					batch->macroblocks[i].mo_type,
					batch->macroblocks[i].PMV[0][0][0],
					batch->macroblocks[i].PMV[0][0][1],
					batch->macroblocks[i].PMV[1][0][0],
					batch->macroblocks[i].PMV[1][0][1],
					batch->macroblocks[i].PMV[0][1][0],
					batch->macroblocks[i].PMV[0][1][1],
					batch->macroblocks[i].PMV[1][1][0],
					batch->macroblocks[i].PMV[1][1][1],
					batch->macroblocks[i].cbp,
					batch->macroblocks[i].dct_type,
					batch->macroblocks[i].blocks,
					batch->past_surface,
					batch->future_surface,
					surface
				);
				break;
			}
			default:
				assert(0);
		}
	}

	return 0;
}

static int vlEnd
(
	struct vlRender *render
)
{
	assert(render);

	return 0;
}

static int vlFlush
(
	struct vlRender *render
)
{
	assert(render);

	return 0;
}

static int vlDestroy
(
	struct vlRender *render
)
{
	struct vlR16SnormMC	*mc;
	struct pipe_context	*pipe;
	unsigned int		i;

	assert(render);

	mc = (struct vlR16SnormMC*)render;
	pipe = mc->pipe;

	for (i = 0; i < 5; ++i)
		pipe->delete_sampler_state(pipe, mc->samplers[i]);

	for (i = 0; i < 3; ++i)
		pipe->winsys->buffer_destroy(pipe->winsys, mc->vertex_bufs[i].buffer);

	/* Textures 3 & 4 are not created directly, no need to release them here */
	for (i = 0; i < NUM_BUFS; ++i)
	{
		pipe_texture_release(&mc->textures[i][0]);
		pipe_texture_release(&mc->textures[i][1]);
		pipe_texture_release(&mc->textures[i][2]);
	}

	pipe->delete_vs_state(pipe, mc->i_vs);
	pipe->delete_fs_state(pipe, mc->i_fs);

	for (i = 0; i < 2; ++i)
	{
		pipe->delete_vs_state(pipe, mc->p_vs[i]);
		pipe->delete_fs_state(pipe, mc->p_fs[i]);
		pipe->delete_vs_state(pipe, mc->b_vs[i]);
		pipe->delete_fs_state(pipe, mc->b_fs[i]);
	}

	pipe->winsys->buffer_destroy(pipe->winsys, mc->vs_const_buf.buffer);
	pipe->winsys->buffer_destroy(pipe->winsys, mc->fs_const_buf.buffer);

	free(mc);

	return 0;
}

/*
 * Represents 8 triangles (4 quads, 1 per block) in noormalized coords
 * that render a macroblock.
 * Need to be scaled to cover mbW*mbH macroblock pixels and translated into
 * position on target surface.
 */
static const struct vlVertex2f macroblock_verts[24] =
{
	{0.0f, 0.0f}, {0.0f, 0.5f}, {0.5f, 0.0f},
	{0.5f, 0.0f}, {0.0f, 0.5f}, {0.5f, 0.5f},

	{0.5f, 0.0f}, {0.5f, 0.5f}, {1.0f, 0.0f},
	{1.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 0.5f},

	{0.0f, 0.5f}, {0.0f, 1.0f}, {0.5f, 0.5f},
	{0.5f, 0.5f}, {0.0f, 1.0f}, {0.5f, 1.0f},

	{0.5f, 0.5f}, {0.5f, 1.0f}, {1.0f, 0.5f},
	{1.0f, 0.5f}, {0.5f, 1.0f}, {1.0f, 1.0f}
};

/*
 * Represents texcoords for the above for rendering 4 luma blocks arranged
 * in a bW*(bH*4) texture. First luma block located at 0,0->bW,bH; second at
 * 0,bH->bW,2bH; third at 0,2bH->bW,3bH; fourth at 0,3bH->bW,4bH.
 */
static const struct vlVertex2f macroblock_luma_texcoords[24] =
{
	{0.0f, 0.0f}, {0.0f, 0.25f}, {1.0f, 0.0f},
	{1.0f, 0.0f}, {0.0f, 0.25f}, {1.0f, 0.25f},

	{0.0f, 0.25f}, {0.0f, 0.5f}, {1.0f, 0.25f},
	{1.0f, 0.25f}, {0.0f, 0.5f}, {1.0f, 0.5f},

	{0.0f, 0.5f}, {0.0f, 0.75f}, {1.0f, 0.5f},
	{1.0f, 0.5f}, {0.0f, 0.75f}, {1.0f, 0.75f},

	{0.0f, 0.75f}, {0.0f, 1.0f}, {1.0f, 0.75f},
	{1.0f, 0.75f}, {0.0f, 1.0f}, {1.0f, 1.0f}
};

/*
 * Represents texcoords for the above for rendering 1 chroma block.
 * Straight forward 0,0->1,1 mapping so we can reuse the MB pos vectors.
 */
static const struct vlVertex2f *macroblock_chroma_420_texcoords = macroblock_verts;

/*
 * Represents texcoords for the above for rendering 2 chroma blocks arranged
 * in a bW*(bH*2) texture. First chroma block located at 0,0->bW,bH; second at
 * 0,bH->bW,2bH. We can render this with 0,0->1,1 mapping.
 * Straight forward 0,0->1,1 mapping so we can reuse MB pos vectors.
 */
static const struct vlVertex2f *macroblock_chroma_422_texcoords = macroblock_verts;

/*
 * Represents texcoords for the above for rendering 4 chroma blocks.
 * Same case as 4 luma blocks.
 */
static const struct vlVertex2f *macroblock_chroma_444_texcoords = macroblock_luma_texcoords;

/*
 * Used when rendering P and B macroblocks, multiplier is applied to the A channel,
 * which is then added to the L channel, then the bias is subtracted from that to
 * get back the differential. The differential is then added to the samples from the
 * reference surface(s).
 */
static const struct vlFragmentShaderConsts fs_consts =
{
	{32767.0f / 255.0f, 32767.0f / 255.0f, 32767.0f / 255.0f, 0.0f},
	{0.5f, 2.0f, 0.0f, 0.0f}
};

static int vlCreateVertexShaderIMB
(
	struct vlR16SnormMC *mc
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

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

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
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0 */
	decl = vl_decl_temps(0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->i_vs = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderIMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 */
	for (i = 0; i < 2; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 */
	for (i = 0; i < 3; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header,max_tokens - ti);
	}

	/*
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i1, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	}

	/* mul o0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	mc->i_fs = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFramePMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

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
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Unused
	 * decl c3		; Translation vector to move ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 3);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Ref macroblock texcoords
	 */
	for (i = 0; i < 4; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0 */
	decl = vl_decl_temps(0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o3, t0, c3	; Translate rect into position on ref macroblock */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->p_vs[0] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFieldPMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

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
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration
		(
			&decl,
			&tokens[ti],
			header,
			max_tokens - ti
		);
	}

	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Denorm coefficients
	 * decl c3		; Translation vector to move top field ref macroblock texcoords into position
	 * decl c4		; Translation vector to move bottom field ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 4);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Top field ref macroblock texcoords
	 * decl o4		; Bottom field ref macroblock texcoords
	 * decl o5		; Denormalized vertex pos
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_output((i == 0 || i == 5) ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add t1, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mov o0, t1		; Move vertex pos to output */
	inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	mov o1, i1		; Move input luma texcoords to output
	mov o2, i2		; Move input chroma texcoords to output
	*/
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o3, t0, c3	; Translate top field rect into position on ref macroblock
	   add o4, t0, c4	; Translate bottom field rect into position on ref macroblock */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o5, t1, c2	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 5, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->p_vs[1] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFramePMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 */
	for (i = 0; i < 3; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for ref surface texture
	 */
	for (i = 0; i < 4; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i1, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* tex2d t1, i2, s3		; Read texel from ref macroblock */
	inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 2, TGSI_FILE_SAMPLER, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, t1		; Add ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	mc->p_fs[0] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFieldPMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 200;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 * decl i3			; Texcoords for s3
	 * decl i4			; Denormalized vertex pos
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm
	 * decl c1			; Constants 1/2 & 2 in .x, .y channels for Y-mod-2 top/bottom field selection
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0-t4 */
	decl = vl_decl_temps(0, 4);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for ref surface texture
	 */
	for (i = 0; i < 4; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i1, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t1, i2, s3		; Read texel from ref macroblock top field
	 * tex2d t2, i3, s3		; Read texel from ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 2, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* XXX: Pos values off by 0.5? */
	/* sub t4, i4.y, c1.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 4, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t4, c1.x		; Multiply pos Y-coord by 1/2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* floor t3, t3			; Get rid of fractional part */
	inst = vl_inst2(TGSI_OPCODE_FLOOR, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t3, c1.y		; Multiply by 2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* sub t3, t4, t3		; Subtract from original Y to get Y % 2 */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t1, t3, t1, t2		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, t1		; Add ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	mc->p_fs[1] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFrameBMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

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
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Unused
	 * decl c3		; Translation vector to move past ref macroblock texcoords into position
	 * decl c4		; Unused
	 * decl c5		; Translation vector to move future ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 5);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Past ref macroblock texcoords
	 * decl o4		; Future ref macroblock texcoords
	 */
	for (i = 0; i < 5; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0 */
	decl = vl_decl_temps(0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o3, t0, c3	; Translate rect into position on past ref macroblock
	   add o4, t0, c5	; Translate rect into position on future ref macroblock */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i * 2 + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->b_vs[0] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateVertexShaderFieldBMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	vs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

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
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0		; Scaling vector to scale unit rect to macroblock size
	 * decl c1		; Translation vector to move macroblock into position
	 * decl c2		; Denorm coefficients
	 * decl c3		; Translation vector to move top field past ref macroblock texcoords into position
	 * decl c4		; Translation vector to move bottom field past ref macroblock texcoords into position
	 * decl c5		; Translation vector to move top field future ref macroblock texcoords into position
	 * decl c6		; Translation vector to move bottom field future ref macroblock texcoords into position
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 6);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma texcoords
	 * decl o3		; Top field past ref macroblock texcoords
	 * decl o4		; Bottom field past ref macroblock texcoords
	 * decl o5		; Top field future ref macroblock texcoords
	 * decl o6		; Bottom field future ref macroblock texcoords
	 * decl o7		; Denormalized vertex pos
	 */
	for (i = 0; i < 8; i++)
	{
		decl = vl_decl_output((i == 0 || i == 7) ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* mul t0, i0, c0	; Scale unit rect to normalized MB size */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add t1, t0, c1	; Translate rect into position */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mov o0, t1		; Move vertex pos to output */
	inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma texcoords to output
	 */
	for (i = 1; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o3, t0, c3	; Translate top field rect into position on past ref macroblock
	 * add o4, t0, c4	; Translate bottom field rect into position on past ref macroblock
	 * add o5, t0, c5	; Translate top field rect into position on future ref macroblock
	 * add o6, t0, c6	; Translate bottom field rect into position on future ref macroblock
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 3, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, i + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o7, t1, c2	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 7, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	vs.tokens = tokens;
	mc->b_vs[1] = pipe->create_vs_state(pipe, &vs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFrameBMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 100;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 * decl i3			; Texcoords for s4
	 */
	for (i = 0; i < 4; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm
	 * decl c1			; Constant 1/2 in .x channel to use as weight to blend past and future texels
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0-t2 */
	decl = vl_decl_temps(0, 2);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for past ref surface texture
	 * decl s4			; Sampler for future ref surface texture
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i1, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t1, i2, s3		; Read texel from past ref macroblock
	 * tex2d t2, i3, s4		; Read texel from future ref macroblock
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 2, TGSI_FILE_SAMPLER, i + 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* lerp t1, c1.x, t1, t2	; Blend past and future texels */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, t1		; Add past/future ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	mc->b_fs[0] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateFragmentShaderFieldBMB
(
	struct vlR16SnormMC *mc
)
{
	const unsigned int		max_tokens = 200;

	struct pipe_context		*pipe;
	struct pipe_shader_state	fs;
	struct tgsi_token		*tokens;
	struct tgsi_header		*header;

	struct tgsi_full_declaration	decl;
	struct tgsi_full_instruction	inst;

	unsigned int			ti;
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;
	tokens = (struct tgsi_token*)malloc(max_tokens * sizeof(struct tgsi_token));

	/* Version */
	*(struct tgsi_version*)&tokens[0] = tgsi_build_version();
	/* Header */
	header = (struct tgsi_header*)&tokens[1];
	*header = tgsi_build_header();
	/* Processor */
	*(struct tgsi_processor*)&tokens[2] = tgsi_build_processor(TGSI_PROCESSOR_FRAGMENT, header);

	ti = 3;

	/*
	 * decl i0			; Texcoords for s0
	 * decl i1			; Texcoords for s1, s2
	 * decl i2			; Texcoords for s3
	 * decl i3			; Texcoords for s3
	 * decl i4			; Texcoords for s4
	 * decl i5			; Texcoords for s4
	 * decl i6			; Denormalized vertex pos
	 */
	for (i = 0; i < 7; ++i)
	{
		decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, i + 1, i, i, TGSI_INTERPOLATE_LINEAR);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl c0			; Scaling factor, rescales 16-bit snorm to 9-bit snorm
	 * decl c1			; Constants 1/2 & 2 in .x, .y channels to use as weight to blend past and future texels
	 *				; and for Y-mod-2 top/bottom field selection
	 */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl o0			; Fragment color */
	decl = vl_decl_output(TGSI_SEMANTIC_COLOR, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/* decl t0-t5 */
	decl = vl_decl_temps(0, 5);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl s0			; Sampler for luma texture
	 * decl s1			; Sampler for chroma Cb texture
	 * decl s2			; Sampler for chroma Cr texture
	 * decl s3			; Sampler for past ref surface texture
	 * decl s4			; Sampler for future ref surface texture
	 */
	for (i = 0; i < 5; ++i)
	{
		decl = vl_decl_samplers(i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * tex2d t1, i0, s0		; Read texel from luma texture
	 * mov t0.x, t1.x		; Move luma sample into .x component
	 * tex2d t1, i1, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i1, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i > 0 ? 1 : 0, TGSI_FILE_SAMPLER, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
		inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
		inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
		inst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X << i;
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	}

	/* mul t0, t0, c0		; Rescale texel to correct range */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_CONSTANT, 0);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* XXX: Pos values off by 0.5? */
	/* sub t4, i6.y, c1.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 6, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t4, c1.x		; Multiply pos Y-coord by 1/2 */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* floor t3, t3			; Get rid of fractional part */
	inst = vl_inst2(TGSI_OPCODE_FLOOR, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* mul t3, t3, c1.y		; Multiply by 2 */
	inst = vl_inst3( TGSI_OPCODE_MUL, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_CONSTANT, 1);
	inst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
	inst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* sub t3, t4, t3		; Subtract from original Y to get Y % 2 */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 3);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t1, i2, s3		; Read texel from past ref macroblock top field
	 * tex2d t2, i3, s3		; Read texel from past ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 2, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t1, t3, t1, t2		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t4, i4, s4		; Read texel from future ref macroblock top field
	 * tex2d t5, i5, s4		; Read texel from future ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 4, TGSI_FILE_INPUT, i + 4, TGSI_FILE_SAMPLER, 4);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t2, t3, t4, t5		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 2, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_TEMPORARY, 5);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* lerp t1, c1.x, t1, t2	; Blend past and future texels */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_CONSTANT, 1, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	inst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_X;
	inst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_X;
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* add o0, t0, t1		; Add past/future ref and differential to form final output */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 0, TGSI_FILE_TEMPORARY, 0, TGSI_FILE_TEMPORARY, 1);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/* end */
	inst = vl_end();
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	fs.tokens = tokens;
	mc->b_fs[1] = pipe->create_fs_state(pipe, &fs);
	free(tokens);

	return 0;
}

static int vlCreateDataBufs
(
	struct vlR16SnormMC *mc
)
{
	struct pipe_context	*pipe;
	unsigned int		i;

	assert(mc);

	pipe = mc->pipe;

	/* Create our vertex buffer and vertex buffer element */
	mc->vertex_bufs[0].pitch = sizeof(struct vlVertex2f);
	mc->vertex_bufs[0].max_index = 23;
	mc->vertex_bufs[0].buffer_offset = 0;
	mc->vertex_bufs[0].buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_VERTEX,
		sizeof(struct vlVertex2f) * 24
	);

	mc->vertex_elems[0].src_offset = 0;
	mc->vertex_elems[0].vertex_buffer_index = 0;
	mc->vertex_elems[0].nr_components = 2;
	mc->vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Create our texcoord buffers and texcoord buffer elements */
	for (i = 1; i < 3; ++i)
	{
		mc->vertex_bufs[i].pitch = sizeof(struct vlVertex2f);
		mc->vertex_bufs[i].max_index = 23;
		mc->vertex_bufs[i].buffer_offset = 0;
		mc->vertex_bufs[i].buffer = pipe->winsys->buffer_create
		(
			pipe->winsys,
			1,
			PIPE_BUFFER_USAGE_VERTEX,
			sizeof(struct vlVertex2f) * 24
		);

		mc->vertex_elems[i].src_offset = 0;
		mc->vertex_elems[i].vertex_buffer_index = i;
		mc->vertex_elems[i].nr_components = 2;
		mc->vertex_elems[i].src_format = PIPE_FORMAT_R32G32_FLOAT;
	}

	/* Fill buffers */
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, mc->vertex_bufs[0].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		macroblock_verts,
		sizeof(struct vlVertex2f) * 24
	);
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, mc->vertex_bufs[1].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		macroblock_luma_texcoords,
		sizeof(struct vlVertex2f) * 24
	);
	/* TODO: Accomodate 422, 444 */
	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, mc->vertex_bufs[2].buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		macroblock_chroma_420_texcoords,
		sizeof(struct vlVertex2f) * 24
	);

	for (i = 0; i < 3; ++i)
		pipe->winsys->buffer_unmap(pipe->winsys, mc->vertex_bufs[i].buffer);

	/* Create our constant buffer */
	mc->vs_const_buf.size = sizeof(struct vlVertexShaderConsts);
	mc->vs_const_buf.buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_CONSTANT,
		mc->vs_const_buf.size
	);

	mc->fs_const_buf.size = sizeof(struct vlFragmentShaderConsts);
	mc->fs_const_buf.buffer = pipe->winsys->buffer_create
	(
		pipe->winsys,
		1,
		PIPE_BUFFER_USAGE_CONSTANT,
		mc->fs_const_buf.size
	);

	memcpy
	(
		pipe->winsys->buffer_map(pipe->winsys, mc->fs_const_buf.buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		&fs_consts,
		sizeof(struct vlFragmentShaderConsts)
	);

	pipe->winsys->buffer_unmap(pipe->winsys, mc->fs_const_buf.buffer);

	return 0;
}

static int vlInit
(
	struct vlR16SnormMC *mc
)
{
	struct pipe_context		*pipe;
	struct pipe_sampler_state	sampler;
	struct pipe_texture		template;
	unsigned int			filters[5];
	unsigned int			i;

	assert(mc);

	pipe = mc->pipe;

	/* For MC we render to textures, which are rounded up to nearest POT */
	mc->viewport.scale[0] = vlRoundUpPOT(mc->video_width);
	mc->viewport.scale[1] = vlRoundUpPOT(mc->video_height);
	mc->viewport.scale[2] = 1;
	mc->viewport.scale[3] = 1;
	mc->viewport.translate[0] = 0;
	mc->viewport.translate[1] = 0;
	mc->viewport.translate[2] = 0;
	mc->viewport.translate[3] = 0;

	mc->render_target.width = vlRoundUpPOT(mc->video_width);
	mc->render_target.height = vlRoundUpPOT(mc->video_height);
	mc->render_target.num_cbufs = 1;
	/* FB for MC stage is a vlSurface, set in vlSetRenderSurface() */
	mc->render_target.zsbuf = NULL;

	filters[0] = PIPE_TEX_FILTER_NEAREST;
	filters[1] = mc->video_format == vlFormatYCbCr444 ? PIPE_TEX_FILTER_NEAREST : PIPE_TEX_FILTER_LINEAR;
	filters[2] = mc->video_format == vlFormatYCbCr444 ? PIPE_TEX_FILTER_NEAREST : PIPE_TEX_FILTER_LINEAR;
	filters[3] = PIPE_TEX_FILTER_LINEAR;
	filters[4] = PIPE_TEX_FILTER_LINEAR;

	for (i = 0; i < 5; ++i)
	{
		sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		sampler.min_img_filter = filters[i];
		sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
		sampler.mag_img_filter = filters[i];
		sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
		sampler.compare_func = PIPE_FUNC_ALWAYS;
		sampler.normalized_coords = 1;
		/*sampler.prefilter = ;*/
		/*sampler.shadow_ambient = ;*/
		/*sampler.lod_bias = ;*/
		sampler.min_lod = 0;
		/*sampler.max_lod = ;*/
		/*sampler.border_color[i] = ;*/
		/*sampler.max_anisotropy = ;*/
		mc->samplers[i] = pipe->create_sampler_state(pipe, &sampler);
	}

	memset(&template, 0, sizeof(struct pipe_texture));
	template.target = PIPE_TEXTURE_2D;
	template.format = PIPE_FORMAT_R16_SNORM;
	template.last_level = 0;
	template.width[0] = 8;
	template.height[0] = 8 * 4;
	template.depth[0] = 1;
	template.compressed = 0;
	pf_get_block(template.format, &template.block);

	for (i = 0; i < NUM_BUFS; ++i)
		mc->textures[i][0] = pipe->screen->texture_create(pipe->screen, &template);

	if (mc->video_format == vlFormatYCbCr420)
		template.height[0] = 8;
	else if (mc->video_format == vlFormatYCbCr422)
		template.height[0] = 8 * 2;
	else if (mc->video_format == vlFormatYCbCr444)
		template.height[0] = 8 * 4;
	else
		assert(0);

	for (i = 0; i < NUM_BUFS; ++i)
	{
		mc->textures[i][1] = pipe->screen->texture_create(pipe->screen, &template);
		mc->textures[i][2] = pipe->screen->texture_create(pipe->screen, &template);
	}

	/* textures[3] & textures[4] are assigned from vlSurfaces for P and B macroblocks at render time */

	vlCreateVertexShaderIMB(mc);
	vlCreateFragmentShaderIMB(mc);
	vlCreateVertexShaderFramePMB(mc);
	vlCreateVertexShaderFieldPMB(mc);
	vlCreateFragmentShaderFramePMB(mc);
	vlCreateFragmentShaderFieldPMB(mc);
	vlCreateVertexShaderFrameBMB(mc);
	vlCreateVertexShaderFieldBMB(mc);
	vlCreateFragmentShaderFrameBMB(mc);
	vlCreateFragmentShaderFieldBMB(mc);
	vlCreateDataBufs(mc);

	return 0;
}

int vlCreateR16SNormMC
(
	struct pipe_context *pipe,
	unsigned int video_width,
	unsigned int video_height,
	enum vlFormat video_format,
	struct vlRender **render
)
{
	struct vlR16SnormMC *mc;

	assert(pipe);
	assert(render);

	mc = calloc(1, sizeof(struct vlR16SnormMC));

	mc->base.vlBegin = &vlBegin;
	mc->base.vlRenderMacroBlocksMpeg2 = &vlRenderMacroBlocksMpeg2R16Snorm;
	mc->base.vlEnd = &vlEnd;
	mc->base.vlFlush = &vlFlush;
	mc->base.vlDestroy = &vlDestroy;
	mc->pipe = pipe;
	mc->video_width = video_width;
	mc->video_height = video_height;
	mc->cur_buf = 0;

	vlInit(mc);

	*render = &mc->base;

	return 0;
}
