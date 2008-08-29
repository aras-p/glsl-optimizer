#define VL_INTERNAL
#include "vl_r16snorm_mc_buf.h"
#include <assert.h>
#include <stdlib.h>
#include <pipe/p_context.h>
#include <pipe/p_winsys.h>
#include <pipe/p_screen.h>
#include <pipe/p_state.h>
#include <pipe/p_util.h>
#include <pipe/p_inlines.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>
#include "vl_render.h"
#include "vl_shader_build.h"
#include "vl_surface.h"
#include "vl_util.h"
#include "vl_types.h"
#include "vl_defs.h"

#define NUM_BUF_SETS 4	/* Number of rotating buffer sets to use */

enum vlMacroBlockTypeEx
{
	vlMacroBlockExTypeIntra,
	vlMacroBlockExTypeFwdPredictedFrame,
	vlMacroBlockExTypeFwdPredictedField,
	vlMacroBlockExTypeBkwdPredictedFrame,
	vlMacroBlockExTypeBkwdPredictedField,
	vlMacroBlockExTypeBiPredictedFrame,
	vlMacroBlockExTypeBiPredictedField,

	vlNumMacroBlockExTypes
};

struct vlVertexShaderConsts
{
	struct vlVertex4f denorm;
};

struct vlFragmentShaderConsts
{
	struct vlVertex4f multiplier;
	struct vlVertex4f div;
};

struct vlR16SnormBufferedMC
{
	struct vlRender				base;

	unsigned int				video_width, video_height;
	enum vlFormat				video_format;

	unsigned int				cur_buf;
	struct vlSurface			*buffered_surface;
	struct vlSurface			*past_surface, *future_surface;
	struct vlVertex2f			surface_tex_inv_size;
	unsigned int				num_macroblocks[vlNumMacroBlockExTypes];
	unsigned int				total_num_macroblocks;

	struct pipe_context			*pipe;
	struct pipe_viewport_state		viewport;
	struct pipe_framebuffer_state		render_target;
	struct pipe_sampler_state		*samplers[5];
	struct pipe_texture			*textures[NUM_BUF_SETS][5];
	void					*i_vs, *p_vs[2], *b_vs[2];
	void					*i_fs, *p_fs[2], *b_fs[2];
	struct pipe_vertex_buffer 		vertex_bufs[NUM_BUF_SETS][vlNumMacroBlockExTypes][3];
	struct pipe_vertex_element		vertex_elems[5];
	struct pipe_constant_buffer		vs_const_buf, fs_const_buf;
};

static int vlBegin
(
	struct vlRender *render
)
{
	assert(render);

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
	struct vlR16SnormBufferedMC *mc,
	unsigned int mbx,
	unsigned int mby,
	enum vlDCTType dct_type,
	unsigned int coded_block_pattern,
	short *blocks
)
{
	struct pipe_surface	*tex_surface;
	short			*texels;
	unsigned int		tex_pitch;
	unsigned int		x, y, tb = 0, sb = 0;
	unsigned int		mbpx = mbx * VL_MACROBLOCK_WIDTH, mbpy = mby * VL_MACROBLOCK_HEIGHT;

	assert(mc);
	assert(blocks);

	tex_surface = mc->pipe->screen->get_tex_surface
	(
		mc->pipe->screen,
		mc->textures[mc->cur_buf % NUM_BUF_SETS][0],
		0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
	);

	texels = pipe_surface_map(tex_surface, PIPE_BUFFER_USAGE_CPU_WRITE);
	tex_pitch = tex_surface->stride / tex_surface->block.size;

	texels += mbpy * tex_pitch + mbpx;

	for (y = 0; y < 2; ++y)
	{
		for (x = 0; x < 2; ++x, ++tb)
		{
			if ((coded_block_pattern >> (5 - tb)) & 1)
			{
				short *cur_block = blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT;

				if (dct_type == vlDCTTypeFrameCoded)
				{
					vlGrabFrameCodedBlock
					(
						cur_block,
						texels + y * tex_pitch * VL_BLOCK_HEIGHT + x * VL_BLOCK_WIDTH,
						tex_pitch
					);
				}
				else
				{
					vlGrabFieldCodedBlock
					(
						cur_block,
						texels + y * tex_pitch + x * VL_BLOCK_WIDTH,
						tex_pitch
					);
				}

				++sb;
			}
			else
				vlGrabNoBlock(texels + y * tex_pitch * VL_BLOCK_HEIGHT + x * VL_BLOCK_WIDTH, tex_pitch);
		}
	}

	pipe_surface_unmap(tex_surface);

	/* TODO: Implement 422, 444 */
	mbpx >>= 1;
	mbpy >>= 1;

	for (tb = 0; tb < 2; ++tb)
	{
		tex_surface = mc->pipe->screen->get_tex_surface
		(
			mc->pipe->screen,
			mc->textures[mc->cur_buf % NUM_BUF_SETS][tb + 1],
			0, 0, 0, PIPE_BUFFER_USAGE_CPU_WRITE
		);

		texels = pipe_surface_map(tex_surface, PIPE_BUFFER_USAGE_CPU_WRITE);
		tex_pitch = tex_surface->stride / tex_surface->block.size;

		texels += mbpy * tex_pitch + mbpx;

		if ((coded_block_pattern >> (1 - tb)) & 1)
		{
			short *cur_block = blocks + sb * VL_BLOCK_WIDTH * VL_BLOCK_HEIGHT;

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

static int vlGrabMacroBlock
(
	struct vlR16SnormBufferedMC *mc,
	struct vlMpeg2MacroBlock *macroblock
)
{
	const struct vlVertex2f	unit =
	{
		mc->surface_tex_inv_size.x * VL_MACROBLOCK_WIDTH,
		mc->surface_tex_inv_size.y * VL_MACROBLOCK_HEIGHT
	};
	const struct vlVertex2f half =
	{
		mc->surface_tex_inv_size.x * (VL_MACROBLOCK_WIDTH / 2),
		mc->surface_tex_inv_size.y * (VL_MACROBLOCK_HEIGHT / 2)
	};

	struct vlVertex2f	*vb;
	enum vlMacroBlockTypeEx	mb_type_ex;
	struct vlVertex2f	mo_vec[2];
	unsigned int		i;

	assert(mc);
	assert(macroblock);

	switch (macroblock->mb_type)
	{
		case vlMacroBlockTypeIntra:
		{
			mb_type_ex = vlMacroBlockExTypeIntra;
			break;
		}
		case vlMacroBlockTypeFwdPredicted:
		{
			mb_type_ex = macroblock->mo_type == vlMotionTypeFrame ?
				vlMacroBlockExTypeFwdPredictedFrame : vlMacroBlockExTypeFwdPredictedField;
			break;
		}
		case vlMacroBlockTypeBkwdPredicted:
		{
			mb_type_ex = macroblock->mo_type == vlMotionTypeFrame ?
				vlMacroBlockExTypeBkwdPredictedFrame : vlMacroBlockExTypeBkwdPredictedField;
			break;
		}
		case vlMacroBlockTypeBiPredicted:
		{
			mb_type_ex = macroblock->mo_type == vlMotionTypeFrame ?
				vlMacroBlockExTypeBiPredictedFrame : vlMacroBlockExTypeBiPredictedField;
			break;
		}
		default:
			assert(0);
	}

	switch (macroblock->mb_type)
	{
		case vlMacroBlockTypeBiPredicted:
		{
			vb = (struct vlVertex2f*)mc->pipe->winsys->buffer_map
			(
				mc->pipe->winsys,
				mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][mb_type_ex][2].buffer,
				PIPE_BUFFER_USAGE_CPU_WRITE
			) + mc->num_macroblocks[mb_type_ex] * 2 * 24;

			mo_vec[0].x = macroblock->PMV[0][1][0] * 0.5f * mc->surface_tex_inv_size.x;
			mo_vec[0].y = macroblock->PMV[0][1][1] * 0.5f * mc->surface_tex_inv_size.y;

			if (macroblock->mo_type == vlMotionTypeFrame)
			{
				for (i = 0; i < 24 * 2; i += 2)
				{
					vb[i].x = mo_vec[0].x;
					vb[i].y = mo_vec[0].y;
				}
			}
			else
			{
				mo_vec[1].x = macroblock->PMV[1][1][0] * 0.5f * mc->surface_tex_inv_size.x;
				mo_vec[1].y = macroblock->PMV[1][1][1] * 0.5f * mc->surface_tex_inv_size.y;

				for (i = 0; i < 24 * 2; i += 2)
				{
					vb[i].x = mo_vec[0].x;
					vb[i].y = mo_vec[0].y;
					vb[i + 1].x = mo_vec[1].x;
					vb[i + 1].y = mo_vec[1].y;
				}
			}

			mc->pipe->winsys->buffer_unmap(mc->pipe->winsys, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][mb_type_ex][2].buffer);

			/* fall-through */
		}
		case vlMacroBlockTypeFwdPredicted:
		case vlMacroBlockTypeBkwdPredicted:
		{
			vb = (struct vlVertex2f*)mc->pipe->winsys->buffer_map
			(
				mc->pipe->winsys,
				mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][mb_type_ex][1].buffer,
				PIPE_BUFFER_USAGE_CPU_WRITE
			) + mc->num_macroblocks[mb_type_ex] * 2 * 24;

			if (macroblock->mb_type == vlMacroBlockTypeBkwdPredicted)
			{
				mo_vec[0].x = macroblock->PMV[0][1][0] * 0.5f * mc->surface_tex_inv_size.x;
				mo_vec[0].y = macroblock->PMV[0][1][1] * 0.5f * mc->surface_tex_inv_size.y;

				if (macroblock->mo_type == vlMotionTypeField)
				{
					mo_vec[1].x = macroblock->PMV[1][1][0] * 0.5f * mc->surface_tex_inv_size.x;
					mo_vec[1].y = macroblock->PMV[1][1][1] * 0.5f * mc->surface_tex_inv_size.y;
				}
			}
			else
			{
				mo_vec[0].x = macroblock->PMV[0][0][0] * 0.5f * mc->surface_tex_inv_size.x;
				mo_vec[0].y = macroblock->PMV[0][0][1] * 0.5f * mc->surface_tex_inv_size.y;

				if (macroblock->mo_type == vlMotionTypeField)
				{
					mo_vec[1].x = macroblock->PMV[1][0][0] * 0.5f * mc->surface_tex_inv_size.x;
					mo_vec[1].y = macroblock->PMV[1][0][1] * 0.5f * mc->surface_tex_inv_size.y;
				}
			}

			if (macroblock->mo_type == vlMotionTypeFrame)
			{
				for (i = 0; i < 24 * 2; i += 2)
				{
					vb[i].x = mo_vec[0].x;
					vb[i].y = mo_vec[0].y;
				}
			}
			else
			{
				for (i = 0; i < 24 * 2; i += 2)
				{
					vb[i].x = mo_vec[0].x;
					vb[i].y = mo_vec[0].y;
					vb[i + 1].x = mo_vec[1].x;
					vb[i + 1].y = mo_vec[1].y;
				}
			}

			mc->pipe->winsys->buffer_unmap(mc->pipe->winsys, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][mb_type_ex][1].buffer);

			/* fall-through */
		}
		case vlMacroBlockTypeIntra:
		{
			vb = (struct vlVertex2f*)mc->pipe->winsys->buffer_map
			(
				mc->pipe->winsys,
				mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][mb_type_ex][0].buffer,
				PIPE_BUFFER_USAGE_CPU_WRITE
			) + mc->num_macroblocks[mb_type_ex] * 24;

			vb[0].x = macroblock->mbx * unit.x;		vb[0].y = macroblock->mby * unit.y;
			vb[1].x = macroblock->mbx * unit.x;		vb[1].y = macroblock->mby * unit.y + half.y;
			vb[2].x = macroblock->mbx * unit.x + half.x;	vb[2].y = macroblock->mby * unit.y;

			vb[3].x = macroblock->mbx * unit.x + half.x;	vb[3].y = macroblock->mby * unit.y;
			vb[4].x = macroblock->mbx * unit.x;		vb[4].y = macroblock->mby * unit.y + half.y;
			vb[5].x = macroblock->mbx * unit.x + half.x;	vb[5].y = macroblock->mby * unit.y + half.y;

			vb[6].x = macroblock->mbx * unit.x + half.x;	vb[6].y = macroblock->mby * unit.y;
			vb[7].x = macroblock->mbx * unit.x + half.x;	vb[7].y = macroblock->mby * unit.y + half.y;
			vb[8].x = macroblock->mbx * unit.x + unit.x;	vb[8].y = macroblock->mby * unit.y;

			vb[9].x = macroblock->mbx * unit.x + unit.x;	vb[9].y = macroblock->mby * unit.y;
			vb[10].x = macroblock->mbx * unit.x + half.x;	vb[10].y = macroblock->mby * unit.y + half.y;
			vb[11].x = macroblock->mbx * unit.x + unit.x;	vb[11].y = macroblock->mby * unit.y + half.y;

			vb[12].x = macroblock->mbx * unit.x;		vb[12].y = macroblock->mby * unit.y + half.y;
			vb[13].x = macroblock->mbx * unit.x;		vb[13].y = macroblock->mby * unit.y + unit.y;
			vb[14].x = macroblock->mbx * unit.x + half.x;	vb[14].y = macroblock->mby * unit.y + half.y;

			vb[15].x = macroblock->mbx * unit.x + half.x;	vb[15].y = macroblock->mby * unit.y + half.y;
			vb[16].x = macroblock->mbx * unit.x;		vb[16].y = macroblock->mby * unit.y + unit.y;
			vb[17].x = macroblock->mbx * unit.x + half.x;	vb[17].y = macroblock->mby * unit.y + unit.y;

			vb[18].x = macroblock->mbx * unit.x + half.x;	vb[18].y = macroblock->mby * unit.y + half.y;
			vb[19].x = macroblock->mbx * unit.x + half.x;	vb[19].y = macroblock->mby * unit.y + unit.y;
			vb[20].x = macroblock->mbx * unit.x + unit.x;	vb[20].y = macroblock->mby * unit.y + half.y;

			vb[21].x = macroblock->mbx * unit.x + unit.x;	vb[21].y = macroblock->mby * unit.y + half.y;
			vb[22].x = macroblock->mbx * unit.x + half.x;	vb[22].y = macroblock->mby * unit.y + unit.y;
			vb[23].x = macroblock->mbx * unit.x + unit.x;	vb[23].y = macroblock->mby * unit.y + unit.y;

			mc->pipe->winsys->buffer_unmap(mc->pipe->winsys, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][mb_type_ex][0].buffer);

			break;
		}
		default:
			assert(0);
	}

	vlGrabBlocks
	(
		mc,
		macroblock->mbx,
		macroblock->mby,
		macroblock->dct_type,
		macroblock->cbp,
		macroblock->blocks
	);

	mc->num_macroblocks[mb_type_ex]++;
	mc->total_num_macroblocks++;

	return 0;
}

static int vlFlush
(
	struct vlRender *render
)
{
	struct vlR16SnormBufferedMC	*mc;
	struct pipe_context		*pipe;
	struct vlVertexShaderConsts	*vs_consts;

	assert(mc);

	mc = (struct vlR16SnormBufferedMC*)render;
	pipe = mc->pipe;

	mc->render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		mc->buffered_surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);

	pipe->set_framebuffer_state(pipe, &mc->render_target);
	pipe->set_viewport_state(pipe, &mc->viewport);
	vs_consts = pipe->winsys->buffer_map
	(
		pipe->winsys,
		mc->vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE
	);

	vs_consts->denorm.x = mc->buffered_surface->texture->width[0];
	vs_consts->denorm.y = mc->buffered_surface->texture->height[0];

	pipe->winsys->buffer_unmap(pipe->winsys, mc->vs_const_buf.buffer);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, 0, &mc->fs_const_buf);

	if (mc->num_macroblocks[vlMacroBlockExTypeIntra] > 0)
	{
		pipe->set_vertex_buffers(pipe, 1, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][vlMacroBlockExTypeIntra]);
		pipe->set_vertex_elements(pipe, 1, mc->vertex_elems);
		pipe->set_sampler_textures(pipe, 3, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 3, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->i_vs);
		pipe->bind_fs_state(pipe, mc->i_fs);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, mc->num_macroblocks[vlMacroBlockExTypeIntra] * 24);
	}

	if (mc->num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][vlMacroBlockExTypeFwdPredictedFrame]);
		pipe->set_vertex_elements(pipe, 3, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[0]);
		pipe->bind_fs_state(pipe, mc->p_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, mc->num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] * 24);
	}

	if (mc->num_macroblocks[vlMacroBlockExTypeFwdPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][vlMacroBlockExTypeFwdPredictedField]);
		pipe->set_vertex_elements(pipe, 3, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[1]);
		pipe->bind_fs_state(pipe, mc->p_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, mc->num_macroblocks[vlMacroBlockExTypeFwdPredictedField] * 24);
	}

	if (mc->num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][vlMacroBlockExTypeBkwdPredictedFrame]);
		pipe->set_vertex_elements(pipe, 3, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[0]);
		pipe->bind_fs_state(pipe, mc->p_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, mc->num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] * 24);
	}

	if (mc->num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][vlMacroBlockExTypeBkwdPredictedField]);
		pipe->set_vertex_elements(pipe, 3, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[1]);
		pipe->bind_fs_state(pipe, mc->p_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, mc->num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] * 24);
	}

	if (mc->num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 3, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][vlMacroBlockExTypeBiPredictedFrame]);
		pipe->set_vertex_elements(pipe, 5, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		mc->textures[mc->cur_buf % NUM_BUF_SETS][4] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 5, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 5, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->b_vs[0]);
		pipe->bind_fs_state(pipe, mc->b_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, mc->num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] * 24);
	}

	if (mc->num_macroblocks[vlMacroBlockExTypeBiPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 3, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][vlMacroBlockExTypeBiPredictedField]);
		pipe->set_vertex_elements(pipe, 5, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		mc->textures[mc->cur_buf % NUM_BUF_SETS][4] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 5, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 5, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->b_vs[1]);
		pipe->bind_fs_state(pipe, mc->b_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, 0, mc->num_macroblocks[vlMacroBlockExTypeBiPredictedField] * 24);
	}

	memset(mc->num_macroblocks, 0, sizeof(unsigned int) * 7);
	mc->total_num_macroblocks = 0;
	mc->cur_buf++;

	return 0;
}

static int vlRenderMacroBlocksMpeg2R16SnormBuffered
(
	struct vlRender *render,
	struct vlMpeg2MacroBlockBatch *batch,
	struct vlSurface *surface
)
{
	struct vlR16SnormBufferedMC	*mc;
	unsigned int			i;

	assert(render);

	mc = (struct vlR16SnormBufferedMC*)render;

	if (mc->buffered_surface)
	{
		if
		(
			mc->buffered_surface != surface /*||
			mc->past_surface != batch->past_surface ||
			mc->future_surface != batch->future_surface*/
		)
		{
			vlFlush(&mc->base);
			mc->buffered_surface = surface;
			mc->past_surface = batch->past_surface;
			mc->future_surface = batch->future_surface;
			mc->surface_tex_inv_size.x = 1.0f / surface->texture->width[0];
			mc->surface_tex_inv_size.y = 1.0f / surface->texture->height[0];
		}
	}
	else
	{
		mc->buffered_surface = surface;
		mc->past_surface = batch->past_surface;
		mc->future_surface = batch->future_surface;
		mc->surface_tex_inv_size.x = 1.0f / surface->texture->width[0];
		mc->surface_tex_inv_size.y = 1.0f / surface->texture->height[0];
	}

	for (i = 0; i < batch->num_macroblocks; ++i)
		vlGrabMacroBlock(mc, &batch->macroblocks[i]);

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

static int vlDestroy
(
	struct vlRender *render
)
{
	struct vlR16SnormBufferedMC	*mc;
	struct pipe_context		*pipe;
	unsigned int			g, h, i;

	assert(render);

	mc = (struct vlR16SnormBufferedMC*)render;
	pipe = mc->pipe;

	for (i = 0; i < 5; ++i)
		pipe->delete_sampler_state(pipe, mc->samplers[i]);

	for (g = 0; g < NUM_BUF_SETS; ++g)
		for (h = 0; h < 7; ++h)
			for (i = 0; i < 3; ++i)
				pipe->winsys->buffer_destroy(pipe->winsys, mc->vertex_bufs[g][h][i].buffer);

	/* Textures 3 & 4 are not created directly, no need to release them here */
	for (i = 0; i < NUM_BUF_SETS; ++i)
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
 * Muliplier renormalizes block samples from 16 bits to 12 bits.
 * Divider is used when calculating Y % 2 for choosing top or bottom
 * field for P or B macroblocks.
 * TODO: Use immediates.
 */
static const struct vlFragmentShaderConsts fs_consts =
{
	{32767.0f / 255.0f, 32767.0f / 255.0f, 32767.0f / 255.0f, 0.0f},
	{0.5f, 2.0f, 0.0f, 0.0f}
};

static int vlCreateVertexShaderIMB
(
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0		; Vertex pos, luma & chroma texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma/chroma texcoords
	 */
	for (i = 0; i < 2; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i0		; Move input luma/chroma texcoords to output
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, 0);
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
	struct vlR16SnormBufferedMC *mc
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

	/* decl i0			; Luma/chroma texcoords */
	decl = vl_decl_interpolated_input(TGSI_SEMANTIC_GENERIC, 1, 0, 0, TGSI_INTERPOLATE_LINEAR);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

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
	 * tex2d t1, i0, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i0, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, i);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0		; Vertex pos, luma/chroma texcoords
	 * decl i1		; Ref surface top field texcoords
	 * decl i2		; Ref surface bottom field texcoords (unused, packed in the same stream)
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma/chroma texcoords
	 * decl o2		; Ref macroblock texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i0		; Move input luma/chroma texcoords to output
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, 0);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o2, i0, i1	; Translate vertex pos by motion vec to form ref macroblock texcoords */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 2, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, 1);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0		; Vertex pos, luma/chroma texcoords
	 * decl i1              ; Ref surface top field texcoords
	 * decl i2              ; Ref surface bottom field texcoords
	 */
	for (i = 0; i < 3; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0		; Texcoord denorm coefficients */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma/chroma texcoords
	 * decl o2		; Top field ref macroblock texcoords
	 * decl o3		; Bottom field ref macroblock texcoords
	 * decl o4		; Denormalized vertex pos
	 */
	for (i = 0; i < 5; i++)
	{
		decl = vl_decl_output((i == 0 || i == 5) ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i0		; Move input luma/chroma texcoords to output
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i == 0 ? 0 : i - 1);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o2, i0, i1	; Translate vertex pos by motion vec to form top field macroblock texcoords
	 * add o3, i0, i2	; Translate vertex pos by motion vec to form bottom field macroblock texcoords
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 2, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, i + 1);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o4, i0, c0	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 5, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0			; Texcoords for s0, s1, s2
	 * decl i1			; Texcoords for s3
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
	 * tex2d t1, i0, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i0, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, i);
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

	/* tex2d t1, i1, s3		; Read texel from ref macroblock */
	inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 1, TGSI_FILE_SAMPLER, 3);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0			; Texcoords for s0, s1, s2
	 * decl i1			; Texcoords for s3
	 * decl i2			; Texcoords for s3
	 * decl i3			; Denormalized vertex pos
	 */
	for (i = 0; i < 4; ++i)
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
	 * tex2d t1, i0, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i0, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, i);
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
	 * tex2d t1, i1, s3		; Read texel from ref macroblock top field
	 * tex2d t2, i2, s3		; Read texel from ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 1, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* XXX: Pos values off by 0.5? */
	/* sub t4, i3.y, c1.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 3, TGSI_FILE_CONSTANT, 1);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0		; Vertex pos, luma/chroma texcoords
	 * decl i1              ; First ref surface top field texcoords
	 * decl i2              ; First ref surface bottom field texcoords (unused, packed in the same stream)
	 * decl i3		; Second ref surface top field texcoords
	 * decl i4		; Second ref surface bottom field texcoords (unused, packed in the same stream)
	 */
	for (i = 0; i < 5; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma/chroma texcoords
	 * decl o2		; First ref macroblock texcoords
	 * decl o3		; Second ref macroblock texcoords
	 */
	for (i = 0; i < 4; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i0		; Move input luma/chroma texcoords to output
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, 0);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o2, i0, i1	; Translate vertex pos by motion vec to form first ref macroblock texcoords
	 * add o3, i0, i3	; Translate vertex pos by motion vec to form second ref macroblock texcoords
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 2, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, i * 2 + 1);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0		; Vertex pos, Luma/chroma texcoords
	 * decl i1              ; First ref surface top field texcoords
	 * decl i2              ; First ref surface bottom field texcoords
	 * decl i3              ; Second ref surface top field texcoords
	 * decl i4              ; Second ref surface bottom field texcoords
	 */
	for (i = 0; i < 5; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0		; Denorm coefficients */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 6);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma/chroma texcoords
	 * decl o2		; Top field past ref macroblock texcoords
	 * decl o3		; Bottom field past ref macroblock texcoords
	 * decl o4		; Top field future ref macroblock texcoords
	 * decl o5		; Bottom field future ref macroblock texcoords
	 * decl o6		; Denormalized vertex pos
	 */
	for (i = 0; i < 7; i++)
	{
		decl = vl_decl_output((i == 0 || i == 7) ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i0		; Move input luma/chroma texcoords to output
	 * mov o2, i1		; Move past top field texcoords to output
	 * mov o3, i2		; Move past bottom field texcoords to output
	 * mov o4, i3		; Move future top field texcoords to output
	 * mov o5, i4		; Move future bottom field texcoords to output
	 */
	for (i = 0; i < 6; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, 0);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o2, i0, i1	; Translate vertex pos by motion vec to form first top field macroblock texcoords
	 * add o3, i0, i2	; Translate vertex pos by motion vec to form first bottom field macroblock texcoords
	 * add o4, i0, i3	; Translate vertex pos by motion vec to form second top field macroblock texcoords
	 * add o5, i0, i4	; Translate vertex pos by motion vec to form second bottom field macroblock texcoords
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 2, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, i + 1);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o6, i0, c0	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 6, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0			; Texcoords for s0, s1, s2
	 * decl i1			; Texcoords for s3
	 * decl i2			; Texcoords for s4
	 */
	for (i = 0; i < 3; ++i)
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
	 * tex2d t1, i0, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i0, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, i);
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
	 * tex2d t1, i1, s3		; Read texel from past ref macroblock
	 * tex2d t2, i2, s4		; Read texel from future ref macroblock
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 1, TGSI_FILE_SAMPLER, i + 3);
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
	struct vlR16SnormBufferedMC *mc
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
	 * decl i0			; Texcoords for s0, s1, s2
	 * decl i1			; Texcoords for s3
	 * decl i2			; Texcoords for s3
	 * decl i3			; Texcoords for s4
	 * decl i4			; Texcoords for s4
	 * decl i5			; Denormalized vertex pos
	 */
	for (i = 0; i < 6; ++i)
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
	 * tex2d t1, i0, s1		; Read texel from chroma Cb texture
	 * mov t0.y, t1.x		; Move Cb sample into .y component
	 * tex2d t1, i0, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 0, TGSI_FILE_SAMPLER, i);
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
	/* sub t4, i5.y, c1.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 5, TGSI_FILE_CONSTANT, 1);
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
	 * tex2d t1, i1, s3		; Read texel from past ref macroblock top field
	 * tex2d t2, i2, s3		; Read texel from past ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 1, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t1, t3, t1, t2		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t4, i3, s4		; Read texel from future ref macroblock top field
	 * tex2d t5, i4, s4		; Read texel from future ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 4, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, 4);
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
	struct vlR16SnormBufferedMC *mc
)
{
	const unsigned int	mbw = align(mc->video_width, VL_MACROBLOCK_WIDTH) / VL_MACROBLOCK_WIDTH;
	const unsigned int	mbh = align(mc->video_height, VL_MACROBLOCK_HEIGHT) / VL_MACROBLOCK_HEIGHT;
	const unsigned int	num_mb_per_frame = mbw * mbh;

	struct pipe_context	*pipe;
	unsigned int		g, h, i;

	assert(mc);

	pipe = mc->pipe;

	for (g = 0; g < NUM_BUF_SETS; ++g)
	{
		for (h = 0; h < 7; ++h)
		{
			/* Create our vertex buffer and vertex buffer element */
			mc->vertex_bufs[g][h][0].pitch = sizeof(struct vlVertex2f);
			mc->vertex_bufs[g][h][0].max_index = 24 * num_mb_per_frame - 1;
			mc->vertex_bufs[g][h][0].buffer_offset = 0;
			mc->vertex_bufs[g][h][0].buffer = pipe->winsys->buffer_create
			(
				pipe->winsys,
				1,
				PIPE_BUFFER_USAGE_VERTEX,
				sizeof(struct vlVertex2f) * 24 * num_mb_per_frame
			);
		}
	}

	/* Position & block luma, block chroma texcoord element */
	mc->vertex_elems[0].src_offset = 0;
	mc->vertex_elems[0].vertex_buffer_index = 0;
	mc->vertex_elems[0].nr_components = 2;
	mc->vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

	for (g = 0; g < NUM_BUF_SETS; ++g)
	{
		for (h = 0; h < 7; ++h)
		{
			for (i = 1; i < 3; ++i)
			{
				mc->vertex_bufs[g][h][i].pitch = sizeof(struct vlVertex2f) * 2;
				mc->vertex_bufs[g][h][i].max_index = 24 * num_mb_per_frame - 1;
				mc->vertex_bufs[g][h][i].buffer_offset = 0;
				mc->vertex_bufs[g][h][i].buffer = pipe->winsys->buffer_create
				(
					pipe->winsys,
					1,
					PIPE_BUFFER_USAGE_VERTEX,
					sizeof(struct vlVertex2f) * 2 * 24 * num_mb_per_frame
				);
			}
		}
	}

	/* First ref surface top field texcoord element */
	mc->vertex_elems[1].src_offset = 0;
	mc->vertex_elems[1].vertex_buffer_index = 1;
	mc->vertex_elems[1].nr_components = 2;
	mc->vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* First ref surface bottom field texcoord element */
	mc->vertex_elems[2].src_offset = sizeof(struct vlVertex2f);
	mc->vertex_elems[2].vertex_buffer_index = 1;
	mc->vertex_elems[2].nr_components = 2;
	mc->vertex_elems[2].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Second ref surface top field texcoord element */
	mc->vertex_elems[3].src_offset = 0;
	mc->vertex_elems[3].vertex_buffer_index = 2;
	mc->vertex_elems[3].nr_components = 2;
	mc->vertex_elems[3].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Second ref surface bottom field texcoord element */
	mc->vertex_elems[4].src_offset = sizeof(struct vlVertex2f);
	mc->vertex_elems[4].vertex_buffer_index = 2;
	mc->vertex_elems[4].nr_components = 2;
	mc->vertex_elems[4].src_format = PIPE_FORMAT_R32G32_FLOAT;

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
	struct vlR16SnormBufferedMC *mc
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
	/* FB for MC stage is a vlSurface created by the user, set at render time */
	mc->render_target.zsbuf = NULL;

	filters[0] = PIPE_TEX_FILTER_NEAREST;
	/* FIXME: Linear causes discoloration around block edges */
	filters[1] = /*mc->video_format == vlFormatYCbCr444 ?*/ PIPE_TEX_FILTER_NEAREST /*: PIPE_TEX_FILTER_LINEAR*/;
	filters[2] = /*mc->video_format == vlFormatYCbCr444 ?*/ PIPE_TEX_FILTER_NEAREST /*: PIPE_TEX_FILTER_LINEAR*/;
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
	template.width[0] = vlRoundUpPOT(mc->video_width);
	template.height[0] = vlRoundUpPOT(mc->video_height);
	template.depth[0] = 1;
	template.compressed = 0;
	pf_get_block(template.format, &template.block);

	for (i = 0; i < NUM_BUF_SETS; ++i)
		mc->textures[i][0] = pipe->screen->texture_create(pipe->screen, &template);

	if (mc->video_format == vlFormatYCbCr420)
	{
		template.width[0] = vlRoundUpPOT(mc->video_width / 2);
		template.height[0] = vlRoundUpPOT(mc->video_height / 2);
	}
	else if (mc->video_format == vlFormatYCbCr422)
		template.height[0] = vlRoundUpPOT(mc->video_height / 2);

	for (i = 0; i < NUM_BUF_SETS; ++i)
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

int vlCreateR16SNormBufferedMC
(
	struct pipe_context *pipe,
	unsigned int video_width,
	unsigned int video_height,
	enum vlFormat video_format,
	struct vlRender **render
)
{
	struct vlR16SnormBufferedMC *mc;

	assert(pipe);
	assert(render);

	mc = calloc(1, sizeof(struct vlR16SnormBufferedMC));

	mc->base.vlBegin = &vlBegin;
	mc->base.vlRenderMacroBlocksMpeg2 = &vlRenderMacroBlocksMpeg2R16SnormBuffered;
	mc->base.vlEnd = &vlEnd;
	mc->base.vlFlush = &vlFlush;
	mc->base.vlDestroy = &vlDestroy;
	mc->pipe = pipe;
	mc->video_width = video_width;
	mc->video_height = video_height;

	mc->cur_buf = 0;
	mc->buffered_surface = NULL;
	mc->past_surface = NULL;
	mc->future_surface = NULL;
	memset(mc->num_macroblocks, 0, sizeof(unsigned int) * 7);
	mc->total_num_macroblocks = 0;

	vlInit(mc);

	*render = &mc->base;

	return 0;
}
