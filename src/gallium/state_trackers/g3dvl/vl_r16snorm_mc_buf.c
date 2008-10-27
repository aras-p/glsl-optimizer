#define VL_INTERNAL
#include "vl_r16snorm_mc_buf.h"
#include <assert.h>
#include <stdlib.h>
#include <pipe/p_context.h>
#include <pipe/p_winsys.h>
#include <pipe/p_screen.h>
#include <pipe/p_state.h>
#include <pipe/p_inlines.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>
#include <util/u_math.h>
#include "vl_render.h"
#include "vl_shader_build.h"
#include "vl_surface.h"
#include "vl_util.h"
#include "vl_types.h"
#include "vl_defs.h"

/*
 * TODO: Dynamically determine number of buf sets to use, based on
 * video size and available mem, since we can easily run out of memory
 * for high res videos.
 * Note: Destroying previous frame's buffers and creating new ones
 * doesn't work, since the buffer are not actually destroyed until their
 * fence is signalled, and if we render fast enough we will create faster
 * than we destroy.
 */
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

	unsigned int				picture_width, picture_height;
	enum vlFormat				picture_format;

	unsigned int				cur_buf;
	struct vlSurface			*buffered_surface;
	struct vlSurface			*past_surface, *future_surface;
	struct vlVertex2f			surface_tex_inv_size;
	struct vlVertex2f			zero_block[3];
	unsigned int				num_macroblocks;
	struct vlMpeg2MacroBlock		*macroblocks;

	struct pipe_context			*pipe;
	struct pipe_viewport_state		viewport;
	struct pipe_framebuffer_state		render_target;
	struct pipe_sampler_state		*samplers[5];
	struct pipe_texture			*textures[NUM_BUF_SETS][5];
	void					*i_vs, *p_vs[2], *b_vs[2];
	void					*i_fs, *p_fs[2], *b_fs[2];
	struct pipe_vertex_buffer 		vertex_bufs[NUM_BUF_SETS][3];
	struct pipe_vertex_element		vertex_elems[8];
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

static inline int vlGrabFrameCodedBlock(short *src, short *dst, unsigned int dst_pitch)
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

static inline int vlGrabFieldCodedBlock(short *src, short *dst, unsigned int dst_pitch)
{
	unsigned int y;

	for (y = 0; y < VL_BLOCK_HEIGHT; ++y)
		memcpy
		(
			dst + y * dst_pitch * 2,
			src + y * VL_BLOCK_WIDTH,
			VL_BLOCK_WIDTH * 2
		);

	return 0;
}

static inline int vlGrabNoBlock(short *dst, unsigned int dst_pitch)
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

static inline int vlGrabBlocks
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
			else if (mc->zero_block[0].x < 0.0f)
			{
				vlGrabNoBlock(texels + y * tex_pitch * VL_BLOCK_HEIGHT + x * VL_BLOCK_WIDTH, tex_pitch);

				mc->zero_block[0].x = (mbpx + x * 8) * mc->surface_tex_inv_size.x;
				mc->zero_block[0].y = (mbpy + y * 8) * mc->surface_tex_inv_size.y;
			}
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
		else if (mc->zero_block[tb + 1].x < 0.0f)
		{
			vlGrabNoBlock(texels, tex_pitch);

			mc->zero_block[tb + 1].x = (mbpx << 1) * mc->surface_tex_inv_size.x;
			mc->zero_block[tb + 1].y = (mbpy << 1) * mc->surface_tex_inv_size.y;
		}

		pipe_surface_unmap(tex_surface);
	}

	return 0;
}

static inline enum vlMacroBlockTypeEx vlGetMacroBlockTypeEx(struct vlMpeg2MacroBlock *mb)
{
	assert(mb);

	switch (mb->mb_type)
	{
		case vlMacroBlockTypeIntra:
			return vlMacroBlockExTypeIntra;
		case vlMacroBlockTypeFwdPredicted:
			return mb->mo_type == vlMotionTypeFrame ?
				vlMacroBlockExTypeFwdPredictedFrame : vlMacroBlockExTypeFwdPredictedField;
		case vlMacroBlockTypeBkwdPredicted:
			return mb->mo_type == vlMotionTypeFrame ?
				vlMacroBlockExTypeBkwdPredictedFrame : vlMacroBlockExTypeBkwdPredictedField;
		case vlMacroBlockTypeBiPredicted:
			return mb->mo_type == vlMotionTypeFrame ?
				vlMacroBlockExTypeBiPredictedFrame : vlMacroBlockExTypeBiPredictedField;
		default:
			assert(0);
	}

	/* Unreachable */
	return -1;
}

static inline int vlGrabMacroBlock
(
	struct vlR16SnormBufferedMC *mc,
	struct vlMpeg2MacroBlock *macroblock
)
{
	assert(mc);
	assert(macroblock);

	mc->macroblocks[mc->num_macroblocks].mbx = macroblock->mbx;
	mc->macroblocks[mc->num_macroblocks].mby = macroblock->mby;
	mc->macroblocks[mc->num_macroblocks].mb_type = macroblock->mb_type;
	mc->macroblocks[mc->num_macroblocks].mo_type = macroblock->mo_type;
	mc->macroblocks[mc->num_macroblocks].dct_type = macroblock->dct_type;
	mc->macroblocks[mc->num_macroblocks].PMV[0][0][0] = macroblock->PMV[0][0][0];
	mc->macroblocks[mc->num_macroblocks].PMV[0][0][1] = macroblock->PMV[0][0][1];
	mc->macroblocks[mc->num_macroblocks].PMV[0][1][0] = macroblock->PMV[0][1][0];
	mc->macroblocks[mc->num_macroblocks].PMV[0][1][1] = macroblock->PMV[0][1][1];
	mc->macroblocks[mc->num_macroblocks].PMV[1][0][0] = macroblock->PMV[1][0][0];
	mc->macroblocks[mc->num_macroblocks].PMV[1][0][1] = macroblock->PMV[1][0][1];
	mc->macroblocks[mc->num_macroblocks].PMV[1][1][0] = macroblock->PMV[1][1][0];
	mc->macroblocks[mc->num_macroblocks].PMV[1][1][1] = macroblock->PMV[1][1][1];
	mc->macroblocks[mc->num_macroblocks].cbp = macroblock->cbp;
	mc->macroblocks[mc->num_macroblocks].blocks = macroblock->blocks;

	vlGrabBlocks
	(
		mc,
		macroblock->mbx,
		macroblock->mby,
		macroblock->dct_type,
		macroblock->cbp,
		macroblock->blocks
	);

	mc->num_macroblocks++;

	return 0;
}

#define SET_BLOCK(vb, cbp, mbx, mby, unitx, unity, ofsx, ofsy, hx, hy, lm, cbm, crm, zb)					\
	(vb)[0].pos.x = (mbx) * (unitx) + (ofsx);		(vb)[0].pos.y = (mby) * (unity) + (ofsy);			\
	(vb)[1].pos.x = (mbx) * (unitx) + (ofsx);		(vb)[1].pos.y = (mby) * (unity) + (ofsy) + (hy);		\
	(vb)[2].pos.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].pos.y = (mby) * (unity) + (ofsy);			\
	(vb)[3].pos.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].pos.y = (mby) * (unity) + (ofsy);			\
	(vb)[4].pos.x = (mbx) * (unitx) + (ofsx);		(vb)[4].pos.y = (mby) * (unity) + (ofsy) + (hy);		\
	(vb)[5].pos.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].pos.y = (mby) * (unity) + (ofsy) + (hy);		\
																\
	if ((cbp) & (lm))													\
	{															\
		(vb)[0].luma_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[0].luma_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[1].luma_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[1].luma_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[2].luma_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].luma_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[3].luma_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].luma_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[4].luma_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[4].luma_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[5].luma_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].luma_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
	}															\
	else															\
	{															\
		(vb)[0].luma_tc.x = (zb)[0].x;		(vb)[0].luma_tc.y = (zb)[0].y;						\
		(vb)[1].luma_tc.x = (zb)[0].x;		(vb)[1].luma_tc.y = (zb)[0].y + (hy);					\
		(vb)[2].luma_tc.x = (zb)[0].x + (hx);	(vb)[2].luma_tc.y = (zb)[0].y;						\
		(vb)[3].luma_tc.x = (zb)[0].x + (hx);	(vb)[3].luma_tc.y = (zb)[0].y;						\
		(vb)[4].luma_tc.x = (zb)[0].x;		(vb)[4].luma_tc.y = (zb)[0].y + (hy);					\
		(vb)[5].luma_tc.x = (zb)[0].x + (hx);	(vb)[5].luma_tc.y = (zb)[0].y + (hy);					\
	}															\
																\
	if ((cbp) & (cbm))													\
	{															\
		(vb)[0].cb_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[0].cb_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[1].cb_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[1].cb_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[2].cb_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].cb_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[3].cb_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].cb_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[4].cb_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[4].cb_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[5].cb_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].cb_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
	}															\
	else															\
	{															\
		(vb)[0].cb_tc.x = (zb)[1].x;		(vb)[0].cb_tc.y = (zb)[1].y;						\
		(vb)[1].cb_tc.x = (zb)[1].x;		(vb)[1].cb_tc.y = (zb)[1].y + (hy);					\
		(vb)[2].cb_tc.x = (zb)[1].x + (hx);	(vb)[2].cb_tc.y = (zb)[1].y;						\
		(vb)[3].cb_tc.x = (zb)[1].x + (hx);	(vb)[3].cb_tc.y = (zb)[1].y;						\
		(vb)[4].cb_tc.x = (zb)[1].x;		(vb)[4].cb_tc.y = (zb)[1].y + (hy);					\
		(vb)[5].cb_tc.x = (zb)[1].x + (hx);	(vb)[5].cb_tc.y = (zb)[1].y + (hy);					\
	}															\
																\
	if ((cbp) & (crm))													\
	{															\
		(vb)[0].cr_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[0].cr_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[1].cr_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[1].cr_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[2].cr_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[2].cr_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[3].cr_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[3].cr_tc.y = (mby) * (unity) + (ofsy);		\
		(vb)[4].cr_tc.x = (mbx) * (unitx) + (ofsx);		(vb)[4].cr_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
		(vb)[5].cr_tc.x = (mbx) * (unitx) + (ofsx) + (hx);	(vb)[5].cr_tc.y = (mby) * (unity) + (ofsy) + (hy);	\
	}															\
	else															\
	{															\
		(vb)[0].cr_tc.x = (zb)[2].x;		(vb)[0].cr_tc.y = (zb)[2].y;						\
		(vb)[1].cr_tc.x = (zb)[2].x;		(vb)[1].cr_tc.y = (zb)[2].y + (hy);					\
		(vb)[2].cr_tc.x = (zb)[2].x + (hx);	(vb)[2].cr_tc.y = (zb)[2].y;						\
		(vb)[3].cr_tc.x = (zb)[2].x + (hx);	(vb)[3].cr_tc.y = (zb)[2].y;						\
		(vb)[4].cr_tc.x = (zb)[2].x;		(vb)[4].cr_tc.y = (zb)[2].y + (hy);					\
		(vb)[5].cr_tc.x = (zb)[2].x + (hx);	(vb)[5].cr_tc.y = (zb)[2].y + (hy);					\
	}

static inline int vlGrabMacroBlockVB
(
	struct vlR16SnormBufferedMC *mc,
	struct vlMpeg2MacroBlock *macroblock,
	unsigned int pos
)
{
	struct vlVertex2f	mo_vec[2];
	unsigned int		i;

	assert(mc);
	assert(macroblock);

	switch (macroblock->mb_type)
	{
		case vlMacroBlockTypeBiPredicted:
		{
			struct vlVertex2f *vb;

			vb = (struct vlVertex2f*)mc->pipe->winsys->buffer_map
			(
				mc->pipe->winsys,
				mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][2].buffer,
				PIPE_BUFFER_USAGE_CPU_WRITE
			) + pos * 2 * 24;

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

			mc->pipe->winsys->buffer_unmap(mc->pipe->winsys, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][2].buffer);

			/* fall-through */
		}
		case vlMacroBlockTypeFwdPredicted:
		case vlMacroBlockTypeBkwdPredicted:
		{
			struct vlVertex2f *vb;

			vb = (struct vlVertex2f*)mc->pipe->winsys->buffer_map
			(
				mc->pipe->winsys,
				mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][1].buffer,
				PIPE_BUFFER_USAGE_CPU_WRITE
			) + pos * 2 * 24;

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

			mc->pipe->winsys->buffer_unmap(mc->pipe->winsys, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][1].buffer);

			/* fall-through */
		}
		case vlMacroBlockTypeIntra:
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

			struct vlMacroBlockVertexStream0
			{
				struct vlVertex2f pos;
				struct vlVertex2f luma_tc;
				struct vlVertex2f cb_tc;
				struct vlVertex2f cr_tc;
			} *vb;

			vb = (struct vlMacroBlockVertexStream0*)mc->pipe->winsys->buffer_map
			(
				mc->pipe->winsys,
				mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][0].buffer,
				PIPE_BUFFER_USAGE_CPU_WRITE
			) + pos * 24;

			SET_BLOCK
			(
				vb,
				macroblock->cbp, macroblock->mbx, macroblock->mby,
				unit.x, unit.y, 0, 0, half.x, half.y,
				32, 2, 1, mc->zero_block
			);

			SET_BLOCK
			(
				vb + 6,
				macroblock->cbp, macroblock->mbx, macroblock->mby,
				unit.x, unit.y, half.x, 0, half.x, half.y,
				16, 2, 1, mc->zero_block
			);

			SET_BLOCK
			(
				vb + 12,
				macroblock->cbp, macroblock->mbx, macroblock->mby,
				unit.x, unit.y, 0, half.y, half.x, half.y,
				8, 2, 1, mc->zero_block
			);

			SET_BLOCK
			(
				vb + 18,
				macroblock->cbp, macroblock->mbx, macroblock->mby,
				unit.x, unit.y, half.x, half.y, half.x, half.y,
				4, 2, 1, mc->zero_block
			);

			mc->pipe->winsys->buffer_unmap(mc->pipe->winsys, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS][0].buffer);

			break;
		}
		default:
			assert(0);
	}

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
	unsigned int			num_macroblocks[vlNumMacroBlockExTypes] = {0};
	unsigned int			offset[vlNumMacroBlockExTypes];
	unsigned int			vb_start = 0;
	unsigned int			mbw;
	unsigned int			mbh;
	unsigned int			num_mb_per_frame;
	unsigned int			i;

	assert(render);

	mc = (struct vlR16SnormBufferedMC*)render;

	if (!mc->buffered_surface)
		return 0;

	mbw = align(mc->picture_width, VL_MACROBLOCK_WIDTH) / VL_MACROBLOCK_WIDTH;
	mbh = align(mc->picture_height, VL_MACROBLOCK_HEIGHT) / VL_MACROBLOCK_HEIGHT;
	num_mb_per_frame = mbw * mbh;

	if (mc->num_macroblocks < num_mb_per_frame)
		return 0;

	pipe = mc->pipe;

	for (i = 0; i < mc->num_macroblocks; ++i)
	{
		enum vlMacroBlockTypeEx mb_type_ex = vlGetMacroBlockTypeEx(&mc->macroblocks[i]);

		num_macroblocks[mb_type_ex]++;
	}

	offset[0] = 0;

	for (i = 1; i < vlNumMacroBlockExTypes; ++i)
		offset[i] = offset[i - 1] + num_macroblocks[i - 1];

	for (i = 0; i < mc->num_macroblocks; ++i)
	{
		enum vlMacroBlockTypeEx mb_type_ex = vlGetMacroBlockTypeEx(&mc->macroblocks[i]);

		vlGrabMacroBlockVB(mc, &mc->macroblocks[i], offset[mb_type_ex]);

		offset[mb_type_ex]++;
	}

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
	pipe->set_constant_buffer(pipe, PIPE_SHADER_VERTEX, 0, &mc->vs_const_buf);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, 0, &mc->fs_const_buf);

	if (num_macroblocks[vlMacroBlockExTypeIntra] > 0)
	{
		pipe->set_vertex_buffers(pipe, 1, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS]);
		pipe->set_vertex_elements(pipe, 4, mc->vertex_elems);
		pipe->set_sampler_textures(pipe, 3, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 3, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->i_vs);
		pipe->bind_fs_state(pipe, mc->i_fs);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeIntra] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeIntra] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS]);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[0]);
		pipe->bind_fs_state(pipe, mc->p_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeFwdPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS]);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[1]);
		pipe->bind_fs_state(pipe, mc->p_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeFwdPredictedField] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeFwdPredictedField] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS]);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[0]);
		pipe->bind_fs_state(pipe, mc->p_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS]);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 4, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->p_vs[1]);
		pipe->bind_fs_state(pipe, mc->p_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 3, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS]);
		pipe->set_vertex_elements(pipe, 8, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		mc->textures[mc->cur_buf % NUM_BUF_SETS][4] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 5, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 5, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->b_vs[0]);
		pipe->bind_fs_state(pipe, mc->b_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBiPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 3, mc->vertex_bufs[mc->cur_buf % NUM_BUF_SETS]);
		pipe->set_vertex_elements(pipe, 8, mc->vertex_elems);
		mc->textures[mc->cur_buf % NUM_BUF_SETS][3] = mc->past_surface->texture;
		mc->textures[mc->cur_buf % NUM_BUF_SETS][4] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 5, mc->textures[mc->cur_buf % NUM_BUF_SETS]);
		pipe->bind_sampler_states(pipe, 5, (void**)mc->samplers);
		pipe->bind_vs_state(pipe, mc->b_vs[1]);
		pipe->bind_fs_state(pipe, mc->b_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBiPredictedField] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBiPredictedField] * 24;
	}

	pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, &mc->buffered_surface->render_fence);

	for (i = 0; i < 3; ++i)
		mc->zero_block[i].x = -1.0f;

	mc->buffered_surface = NULL;
	mc->num_macroblocks = 0;
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
		if (mc->buffered_surface != surface)
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
	unsigned int			h, i;

	assert(render);

	mc = (struct vlR16SnormBufferedMC*)render;
	pipe = mc->pipe;

	for (i = 0; i < 5; ++i)
		pipe->delete_sampler_state(pipe, mc->samplers[i]);

	for (h = 0; h < NUM_BUF_SETS; ++h)
			for (i = 0; i < 3; ++i)
				pipe->winsys->buffer_destroy(pipe->winsys, mc->vertex_bufs[h][i].buffer);

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

	free(mc->macroblocks);
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
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 */
	for (i = 0; i < 4; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 */
	for (i = 0; i < 4; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
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
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
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
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4		; Ref surface top field texcoords
	 * decl i5		; Ref surface bottom field texcoords (unused, packed in the same stream)
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; Ref macroblock texcoords
	 */
	for (i = 0; i < 5; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* add o4, i0, i4	; Translate vertex pos by motion vec to form ref macroblock texcoords */
	inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, 4);
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
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4              ; Ref macroblock top field texcoords
	 * decl i5              ; Ref macroblock bottom field texcoords
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0		; Render target dimensions */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; Ref macroblock top field texcoords
	 * decl o5		; Ref macroblock bottom field texcoords
	 * decl o6		; Denormalized vertex pos
	 */
	for (i = 0; i < 7; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o4, i0, i4	; Translate vertex pos by motion vec to form top field macroblock texcoords
	 * add o5, i0, i5	; Translate vertex pos by motion vec to form bottom field macroblock texcoords
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, i + 4);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o6, i0, c0	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 6, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; Ref macroblock texcoords
	 */
	for (i = 0; i < 4; ++i)
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
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
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

	/* tex2d t1, i3, s3		; Read texel from ref macroblock */
	inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, 3, TGSI_FILE_SAMPLER, 3);
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; Ref macroblock top field texcoords
	 * decl i4			; Ref macroblock bottom field texcoords
	 * decl i5			; Denormalized vertex pos
	 */
	for (i = 0; i < 6; ++i)
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
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
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
	 * tex2d t1, i3, s3		; Read texel from ref macroblock top field
	 * tex2d t2, i4, s3		; Read texel from ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

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
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4              ; First ref macroblock top field texcoords
	 * decl i5              ; First ref macroblock bottom field texcoords (unused, packed in the same stream)
	 * decl i6		; Second ref macroblock top field texcoords
	 * decl i7		; Second ref macroblock bottom field texcoords (unused, packed in the same stream)
	 */
	for (i = 0; i < 8; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; First ref macroblock texcoords
	 * decl o5		; Second ref macroblock texcoords
	 */
	for (i = 0; i < 6; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o4, i0, i4	; Translate vertex pos by motion vec to form first ref macroblock texcoords
	 * add o5, i0, i6	; Translate vertex pos by motion vec to form second ref macroblock texcoords
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, (i + 2) * 2);
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
	 * decl i0		; Vertex pos
	 * decl i1		; Luma texcoords
	 * decl i2		; Chroma Cb texcoords
	 * decl i3		; Chroma Cr texcoords
	 * decl i4              ; First ref macroblock top field texcoords
	 * decl i5              ; First ref macroblock bottom field texcoords
	 * decl i6              ; Second ref macroblock top field texcoords
	 * decl i7              ; Second ref macroblock bottom field texcoords
	 */
	for (i = 0; i < 8; i++)
	{
		decl = vl_decl_input(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl c0		; Render target dimensions */
	decl = vl_decl_constants(TGSI_SEMANTIC_GENERIC, 0, 0, 0);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * decl o0		; Vertex pos
	 * decl o1		; Luma texcoords
	 * decl o2		; Chroma Cb texcoords
	 * decl o3		; Chroma Cr texcoords
	 * decl o4		; First ref macroblock top field texcoords
	 * decl o5		; First ref macroblock Bottom field texcoords
	 * decl o6		; Second ref macroblock top field texcoords
	 * decl o7		; Second ref macroblock Bottom field texcoords
	 * decl o8		; Denormalized vertex pos
	 */
	for (i = 0; i < 9; i++)
	{
		decl = vl_decl_output(i == 0 ? TGSI_SEMANTIC_POSITION : TGSI_SEMANTIC_GENERIC, i, i, i);
		ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);
	}

	/* decl t0, t1 */
	decl = vl_decl_temps(0, 1);
	ti += tgsi_build_full_declaration(&decl, &tokens[ti], header, max_tokens - ti);

	/*
	 * mov o0, i0		; Move input vertex pos to output
	 * mov o1, i1		; Move input luma texcoords to output
	 * mov o2, i2		; Move input chroma Cb texcoords to output
	 * mov o3, i3		; Move input chroma Cr texcoords to output
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst2(TGSI_OPCODE_MOV, TGSI_FILE_OUTPUT, i, TGSI_FILE_INPUT, i);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/*
	 * add o4, i0, i4	; Translate vertex pos by motion vec to form first top field macroblock texcoords
	 * add o5, i0, i5	; Translate vertex pos by motion vec to form first bottom field macroblock texcoords
	 * add o6, i0, i6	; Translate vertex pos by motion vec to form second top field macroblock texcoords
	 * add o7, i0, i7	; Translate vertex pos by motion vec to form second bottom field macroblock texcoords
	 */
	for (i = 0; i < 4; ++i)
	{
		inst = vl_inst3(TGSI_OPCODE_ADD, TGSI_FILE_OUTPUT, i + 4, TGSI_FILE_INPUT, 0, TGSI_FILE_INPUT, i + 4);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* mul o8, i0, c0	; Denorm vertex pos */
	inst = vl_inst3(TGSI_OPCODE_MUL, TGSI_FILE_OUTPUT, 8, TGSI_FILE_INPUT, 0, TGSI_FILE_CONSTANT, 0);
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; First ref macroblock texcoords
	 * decl i4			; Second ref macroblock texcoords
	 */
	for (i = 0; i < 5; ++i)
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
	 * decl s3			; Sampler for first ref surface texture
	 * decl s4			; Sampler for second ref surface texture
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
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
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
	 * tex2d t1, i3, s3		; Read texel from first ref macroblock
	 * tex2d t2, i4, s4		; Read texel from second ref macroblock
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, i + 3);
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
	 * decl i0			; Luma texcoords
	 * decl i1			; Chroma Cb texcoords
	 * decl i2			; Chroma Cr texcoords
	 * decl i3			; First ref macroblock top field texcoords
	 * decl i4			; First ref macroblock bottom field texcoords
	 * decl i5			; Second ref macroblock top field texcoords
	 * decl i6			; Second ref macroblock bottom field texcoords
	 * decl i7			; Denormalized vertex pos
	 */
	for (i = 0; i < 8; ++i)
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
	 * decl s3			; Sampler for first ref surface texture
	 * decl s4			; Sampler for second ref surface texture
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
	 * tex2d t1, i2, s2		; Read texel from chroma Cr texture
	 * mov t0.z, t1.x		; Move Cr sample into .z component
	 */
	for (i = 0; i < 3; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_INPUT, i, TGSI_FILE_SAMPLER, i);
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
	/* sub t4, i7.y, c1.x		; Sub 0.5 from denormalized pos */
	inst = vl_inst3(TGSI_OPCODE_SUB, TGSI_FILE_TEMPORARY, 4, TGSI_FILE_INPUT, 7, TGSI_FILE_CONSTANT, 1);
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
	 * tex2d t1, i3, s3		; Read texel from past ref macroblock top field
	 * tex2d t2, i4, s3		; Read texel from past ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 1, TGSI_FILE_INPUT, i + 3, TGSI_FILE_SAMPLER, 3);
		ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);
	}

	/* TODO: Move to conditional tex fetch on t3 instead of lerp */
	/* lerp t1, t3, t1, t2		; Choose between top and bottom fields based on Y % 2 */
	inst = vl_inst4(TGSI_OPCODE_LERP, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 3, TGSI_FILE_TEMPORARY, 1, TGSI_FILE_TEMPORARY, 2);
	ti += tgsi_build_full_instruction(&inst, &tokens[ti], header, max_tokens - ti);

	/*
	 * tex2d t4, i5, s4		; Read texel from future ref macroblock top field
	 * tex2d t5, i6, s4		; Read texel from future ref macroblock bottom field
	 */
	for (i = 0; i < 2; ++i)
	{
		inst = vl_tex(TGSI_TEXTURE_2D, TGSI_FILE_TEMPORARY, i + 4, TGSI_FILE_INPUT, i + 5, TGSI_FILE_SAMPLER, 4);
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
	const unsigned int	mbw = align(mc->picture_width, VL_MACROBLOCK_WIDTH) / VL_MACROBLOCK_WIDTH;
	const unsigned int	mbh = align(mc->picture_height, VL_MACROBLOCK_HEIGHT) / VL_MACROBLOCK_HEIGHT;
	const unsigned int	num_mb_per_frame = mbw * mbh;

	struct pipe_context	*pipe;
	unsigned int		h, i;

	assert(mc);

	pipe = mc->pipe;

	/* Create our vertex buffers */
	for (h = 0; h < NUM_BUF_SETS; ++h)
	{
		mc->vertex_bufs[h][0].pitch = sizeof(struct vlVertex2f) * 4;
		mc->vertex_bufs[h][0].max_index = 24 * num_mb_per_frame - 1;
		mc->vertex_bufs[h][0].buffer_offset = 0;
		mc->vertex_bufs[h][0].buffer = pipe->winsys->buffer_create
		(
			pipe->winsys,
			1,
			PIPE_BUFFER_USAGE_VERTEX,
			sizeof(struct vlVertex2f) * 4 * 24 * num_mb_per_frame
		);

		for (i = 1; i < 3; ++i)
		{
			mc->vertex_bufs[h][i].pitch = sizeof(struct vlVertex2f) * 2;
			mc->vertex_bufs[h][i].max_index = 24 * num_mb_per_frame - 1;
			mc->vertex_bufs[h][i].buffer_offset = 0;
			mc->vertex_bufs[h][i].buffer = pipe->winsys->buffer_create
			(
				pipe->winsys,
				1,
				PIPE_BUFFER_USAGE_VERTEX,
				sizeof(struct vlVertex2f) * 2 * 24 * num_mb_per_frame
			);
		}
	}

	/* Position element */
	mc->vertex_elems[0].src_offset = 0;
	mc->vertex_elems[0].vertex_buffer_index = 0;
	mc->vertex_elems[0].nr_components = 2;
	mc->vertex_elems[0].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Luma, texcoord element */
	mc->vertex_elems[1].src_offset = sizeof(struct vlVertex2f);
	mc->vertex_elems[1].vertex_buffer_index = 0;
	mc->vertex_elems[1].nr_components = 2;
	mc->vertex_elems[1].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Chroma Cr texcoord element */
	mc->vertex_elems[2].src_offset = sizeof(struct vlVertex2f) * 2;
	mc->vertex_elems[2].vertex_buffer_index = 0;
	mc->vertex_elems[2].nr_components = 2;
	mc->vertex_elems[2].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Chroma Cb texcoord element */
	mc->vertex_elems[3].src_offset = sizeof(struct vlVertex2f) * 3;
	mc->vertex_elems[3].vertex_buffer_index = 0;
	mc->vertex_elems[3].nr_components = 2;
	mc->vertex_elems[3].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* First ref surface top field texcoord element */
	mc->vertex_elems[4].src_offset = 0;
	mc->vertex_elems[4].vertex_buffer_index = 1;
	mc->vertex_elems[4].nr_components = 2;
	mc->vertex_elems[4].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* First ref surface bottom field texcoord element */
	mc->vertex_elems[5].src_offset = sizeof(struct vlVertex2f);
	mc->vertex_elems[5].vertex_buffer_index = 1;
	mc->vertex_elems[5].nr_components = 2;
	mc->vertex_elems[5].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Second ref surface top field texcoord element */
	mc->vertex_elems[6].src_offset = 0;
	mc->vertex_elems[6].vertex_buffer_index = 2;
	mc->vertex_elems[6].nr_components = 2;
	mc->vertex_elems[6].src_format = PIPE_FORMAT_R32G32_FLOAT;

	/* Second ref surface bottom field texcoord element */
	mc->vertex_elems[7].src_offset = sizeof(struct vlVertex2f);
	mc->vertex_elems[7].vertex_buffer_index = 2;
	mc->vertex_elems[7].nr_components = 2;
	mc->vertex_elems[7].src_format = PIPE_FORMAT_R32G32_FLOAT;

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

	mc->macroblocks = malloc(sizeof(struct vlMpeg2MacroBlock) * num_mb_per_frame);

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
	mc->viewport.scale[0] = vlRoundUpPOT(mc->picture_width);
	mc->viewport.scale[1] = vlRoundUpPOT(mc->picture_height);
	mc->viewport.scale[2] = 1;
	mc->viewport.scale[3] = 1;
	mc->viewport.translate[0] = 0;
	mc->viewport.translate[1] = 0;
	mc->viewport.translate[2] = 0;
	mc->viewport.translate[3] = 0;

	mc->render_target.width = vlRoundUpPOT(mc->picture_width);
	mc->render_target.height = vlRoundUpPOT(mc->picture_height);
	mc->render_target.num_cbufs = 1;
	/* FB for MC stage is a vlSurface created by the user, set at render time */
	mc->render_target.zsbuf = NULL;

	filters[0] = PIPE_TEX_FILTER_NEAREST;
	/* FIXME: Linear causes discoloration around block edges */
	filters[1] = /*mc->picture_format == vlFormatYCbCr444 ?*/ PIPE_TEX_FILTER_NEAREST /*: PIPE_TEX_FILTER_LINEAR*/;
	filters[2] = /*mc->picture_format == vlFormatYCbCr444 ?*/ PIPE_TEX_FILTER_NEAREST /*: PIPE_TEX_FILTER_LINEAR*/;
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
	template.width[0] = vlRoundUpPOT(mc->picture_width);
	template.height[0] = vlRoundUpPOT(mc->picture_height);
	template.depth[0] = 1;
	template.compressed = 0;
	pf_get_block(template.format, &template.block);

	for (i = 0; i < NUM_BUF_SETS; ++i)
		mc->textures[i][0] = pipe->screen->texture_create(pipe->screen, &template);

	if (mc->picture_format == vlFormatYCbCr420)
	{
		template.width[0] = vlRoundUpPOT(mc->picture_width / 2);
		template.height[0] = vlRoundUpPOT(mc->picture_height / 2);
	}
	else if (mc->picture_format == vlFormatYCbCr422)
		template.height[0] = vlRoundUpPOT(mc->picture_height / 2);

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
	unsigned int picture_width,
	unsigned int picture_height,
	enum vlFormat picture_format,
	struct vlRender **render
)
{
	struct vlR16SnormBufferedMC	*mc;
	unsigned int			i;

	assert(pipe);
	assert(render);

	mc = calloc(1, sizeof(struct vlR16SnormBufferedMC));

	mc->base.vlBegin = &vlBegin;
	mc->base.vlRenderMacroBlocksMpeg2 = &vlRenderMacroBlocksMpeg2R16SnormBuffered;
	mc->base.vlEnd = &vlEnd;
	mc->base.vlFlush = &vlFlush;
	mc->base.vlDestroy = &vlDestroy;
	mc->pipe = pipe;
	mc->picture_width = picture_width;
	mc->picture_height = picture_height;

	mc->cur_buf = 0;
	mc->buffered_surface = NULL;
	mc->past_surface = NULL;
	mc->future_surface = NULL;
	for (i = 0; i < 3; ++i)
		mc->zero_block[i].x = -1.0f;
	mc->num_macroblocks = 0;

	vlInit(mc);

	*render = &mc->base;

	return 0;
}
