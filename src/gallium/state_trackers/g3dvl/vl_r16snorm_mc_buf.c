#define VL_INTERNAL
#include "vl_r16snorm_mc_buf.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <pipe/p_state.h>
#include <pipe/p_inlines.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>
#include <util/u_math.h>
#include <util/u_memory.h>
#include "vl_render.h"
#include "vl_shader_build.h"
#include "vl_surface.h"
#include "vl_util.h"
#include "vl_types.h"
#include "vl_defs.h"

const unsigned int DEFAULT_BUF_ALIGNMENT = 1;

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

struct vlMacroBlockVertexStream0
{
	struct vlVertex2f pos;
	struct vlVertex2f luma_tc;
	struct vlVertex2f cb_tc;
	struct vlVertex2f cr_tc;
};

struct vlR16SnormBufferedMC
{
	struct vlRender				base;

	unsigned int				picture_width;
	unsigned int				picture_height;
	enum vlFormat				picture_format;
	unsigned int				macroblocks_per_picture;

	struct vlSurface			*buffered_surface;
	struct vlSurface			*past_surface;
	struct vlSurface			*future_surface;
	struct vlVertex2f			surface_tex_inv_size;
	struct vlVertex2f			zero_block[3];
	unsigned int				num_macroblocks;
	struct vlMpeg2MacroBlock		*macroblocks;
	struct pipe_transfer			*tex_transfer[3];
	short					*texels[3];

	struct pipe_context			*pipe;
	struct pipe_viewport_state		viewport;
	struct pipe_framebuffer_state		render_target;

	union
	{
		void					*all[5];
		struct
		{
			void				*y;
			void				*cb;
			void				*cr;
			void				*ref[2];
		};
	} samplers;

	union
	{
		struct pipe_texture			*all[5];
		struct
		{
			struct pipe_texture		*y;
			struct pipe_texture		*cb;
			struct pipe_texture		*cr;
			struct pipe_texture		*ref[2];
		};
	} textures;

	union
	{
		struct pipe_vertex_buffer 		all[3];
		struct
		{
			struct pipe_vertex_buffer	ycbcr;
			struct pipe_vertex_buffer	ref[2];
		};
	} vertex_bufs;

	void					*i_vs, *p_vs[2], *b_vs[2];
	void					*i_fs, *p_fs[2], *b_fs[2];
	struct pipe_vertex_element		vertex_elems[8];
	struct pipe_constant_buffer		vs_const_buf;
	struct pipe_constant_buffer		fs_const_buf;
};

static inline int vlBegin
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
	short			*texels;
	unsigned int		tex_pitch;
	unsigned int		x, y, tb = 0, sb = 0;
	unsigned int		mbpx = mbx * VL_MACROBLOCK_WIDTH, mbpy = mby * VL_MACROBLOCK_HEIGHT;

	assert(mc);
	assert(blocks);

	tex_pitch = mc->tex_transfer[0]->stride / mc->tex_transfer[0]->block.size;
	texels = mc->texels[0] + mbpy * tex_pitch + mbpx;

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

	/* TODO: Implement 422, 444 */
	mbpx >>= 1;
	mbpy >>= 1;

	for (tb = 0; tb < 2; ++tb)
	{
		tex_pitch = mc->tex_transfer[tb + 1]->stride / mc->tex_transfer[tb + 1]->block.size;
		texels = mc->texels[tb + 1] + mbpy * tex_pitch + mbpx;

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
	assert(mc->num_macroblocks < mc->macroblocks_per_picture);

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
	do {															\
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
	}															\
	} while (0)

static inline int vlGenMacroblockVerts
(
	struct vlR16SnormBufferedMC *mc,
	struct vlMpeg2MacroBlock *macroblock,
	unsigned int pos,
	struct vlMacroBlockVertexStream0 *ycbcr_vb,
	struct vlVertex2f **ref_vb
)
{
	struct vlVertex2f	mo_vec[2];
	unsigned int		i;

	assert(mc);
	assert(macroblock);
	assert(ycbcr_vb);
	assert(pos < mc->macroblocks_per_picture);

	switch (macroblock->mb_type)
	{
		case vlMacroBlockTypeBiPredicted:
		{
			struct vlVertex2f *vb;

			assert(ref_vb && ref_vb[1]);

			vb = ref_vb[1] + pos * 2 * 24;

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

			/* fall-through */
		}
		case vlMacroBlockTypeFwdPredicted:
		case vlMacroBlockTypeBkwdPredicted:
		{
			struct vlVertex2f *vb;

			assert(ref_vb && ref_vb[0]);

			vb = ref_vb[0] + pos * 2 * 24;

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

			struct vlMacroBlockVertexStream0 *vb;

			vb = ycbcr_vb + pos * 24;

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
	unsigned int			i;

	assert(render);

	mc = (struct vlR16SnormBufferedMC*)render;

	if (!mc->buffered_surface)
		return 0;

	if (mc->num_macroblocks < mc->macroblocks_per_picture)
		return 0;

	assert(mc->num_macroblocks <= mc->macroblocks_per_picture);

	pipe = mc->pipe;

	for (i = 0; i < mc->num_macroblocks; ++i)
	{
		enum vlMacroBlockTypeEx mb_type_ex = vlGetMacroBlockTypeEx(&mc->macroblocks[i]);

		num_macroblocks[mb_type_ex]++;
	}

	offset[0] = 0;

	for (i = 1; i < vlNumMacroBlockExTypes; ++i)
		offset[i] = offset[i - 1] + num_macroblocks[i - 1];

	{
		struct vlMacroBlockVertexStream0	*ycbcr_vb;
		struct vlVertex2f			*ref_vb[2];

		ycbcr_vb = (struct vlMacroBlockVertexStream0*)pipe_buffer_map
		(
			pipe->screen,
			mc->vertex_bufs.ycbcr.buffer,
			PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
		);

		for (i = 0; i < 2; ++i)
			ref_vb[i] = (struct vlVertex2f*)pipe_buffer_map
			(
				pipe->screen,
				mc->vertex_bufs.ref[i].buffer,
				PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
			);

		for (i = 0; i < mc->num_macroblocks; ++i)
		{
			enum vlMacroBlockTypeEx mb_type_ex = vlGetMacroBlockTypeEx(&mc->macroblocks[i]);

			vlGenMacroblockVerts(mc, &mc->macroblocks[i], offset[mb_type_ex], ycbcr_vb, ref_vb);

			offset[mb_type_ex]++;
		}

		pipe_buffer_unmap(pipe->screen, mc->vertex_bufs.ycbcr.buffer);
		for (i = 0; i < 2; ++i)
			pipe_buffer_unmap(pipe->screen, mc->vertex_bufs.ref[i].buffer);
	}

	for (i = 0; i < 3; ++i)
	{
		pipe->screen->transfer_unmap(pipe->screen, mc->tex_transfer[i]);
		pipe->screen->tex_transfer_destroy(mc->tex_transfer[i]);
	}

	mc->render_target.cbufs[0] = pipe->screen->get_tex_surface
	(
		pipe->screen,
		mc->buffered_surface->texture,
		0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE
	);

	pipe->set_framebuffer_state(pipe, &mc->render_target);
	pipe->set_viewport_state(pipe, &mc->viewport);
	vs_consts = pipe_buffer_map
	(
		pipe->screen,
		mc->vs_const_buf.buffer,
		PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_DISCARD
	);

	vs_consts->denorm.x = mc->buffered_surface->texture->width[0];
	vs_consts->denorm.y = mc->buffered_surface->texture->height[0];

	pipe_buffer_unmap(pipe->screen, mc->vs_const_buf.buffer);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_VERTEX, 0, &mc->vs_const_buf);
	pipe->set_constant_buffer(pipe, PIPE_SHADER_FRAGMENT, 0, &mc->fs_const_buf);

	if (num_macroblocks[vlMacroBlockExTypeIntra] > 0)
	{
		pipe->set_vertex_buffers(pipe, 1, mc->vertex_bufs.all);
		pipe->set_vertex_elements(pipe, 4, mc->vertex_elems);
		pipe->set_sampler_textures(pipe, 3, mc->textures.all);
		pipe->bind_sampler_states(pipe, 3, mc->samplers.all);
		pipe->bind_vs_state(pipe, mc->i_vs);
		pipe->bind_fs_state(pipe, mc->i_fs);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeIntra] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeIntra] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs.all);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures.ref[0] = mc->past_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures.all);
		pipe->bind_sampler_states(pipe, 4, mc->samplers.all);
		pipe->bind_vs_state(pipe, mc->p_vs[0]);
		pipe->bind_fs_state(pipe, mc->p_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeFwdPredictedFrame] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeFwdPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs.all);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures.ref[0] = mc->past_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures.all);
		pipe->bind_sampler_states(pipe, 4, mc->samplers.all);
		pipe->bind_vs_state(pipe, mc->p_vs[1]);
		pipe->bind_fs_state(pipe, mc->p_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeFwdPredictedField] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeFwdPredictedField] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs.all);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures.ref[0] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures.all);
		pipe->bind_sampler_states(pipe, 4, mc->samplers.all);
		pipe->bind_vs_state(pipe, mc->p_vs[0]);
		pipe->bind_fs_state(pipe, mc->p_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBkwdPredictedFrame] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 2, mc->vertex_bufs.all);
		pipe->set_vertex_elements(pipe, 6, mc->vertex_elems);
		mc->textures.ref[0] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 4, mc->textures.all);
		pipe->bind_sampler_states(pipe, 4, mc->samplers.all);
		pipe->bind_vs_state(pipe, mc->p_vs[1]);
		pipe->bind_fs_state(pipe, mc->p_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBkwdPredictedField] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] > 0)
	{
		pipe->set_vertex_buffers(pipe, 3, mc->vertex_bufs.all);
		pipe->set_vertex_elements(pipe, 8, mc->vertex_elems);
		mc->textures.ref[0] = mc->past_surface->texture;
		mc->textures.ref[1] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 5, mc->textures.all);
		pipe->bind_sampler_states(pipe, 5, mc->samplers.all);
		pipe->bind_vs_state(pipe, mc->b_vs[0]);
		pipe->bind_fs_state(pipe, mc->b_fs[0]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBiPredictedFrame] * 24;
	}

	if (num_macroblocks[vlMacroBlockExTypeBiPredictedField] > 0)
	{
		pipe->set_vertex_buffers(pipe, 3, mc->vertex_bufs.all);
		pipe->set_vertex_elements(pipe, 8, mc->vertex_elems);
		mc->textures.ref[0] = mc->past_surface->texture;
		mc->textures.ref[1] = mc->future_surface->texture;
		pipe->set_sampler_textures(pipe, 5, mc->textures.all);
		pipe->bind_sampler_states(pipe, 5, mc->samplers.all);
		pipe->bind_vs_state(pipe, mc->b_vs[1]);
		pipe->bind_fs_state(pipe, mc->b_fs[1]);

		pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLES, vb_start, num_macroblocks[vlMacroBlockExTypeBiPredictedField] * 24);
		vb_start += num_macroblocks[vlMacroBlockExTypeBiPredictedField] * 24;
	}

	pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, &mc->buffered_surface->render_fence);
	pipe_surface_reference(&mc->render_target.cbufs[0], NULL);

	for (i = 0; i < 3; ++i)
		mc->zero_block[i].x = -1.0f;

	mc->buffered_surface = NULL;
	mc->num_macroblocks = 0;

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
	bool				new_surface = false;
	unsigned int			i;

	assert(render);

	mc = (struct vlR16SnormBufferedMC*)render;

	if (mc->buffered_surface)
	{
		if (mc->buffered_surface != surface)
		{
			vlFlush(&mc->base);
			new_surface = true;
		}
	}
	else
		new_surface = true;

	if (new_surface)
	{
		mc->buffered_surface = surface;
		mc->past_surface = batch->past_surface;
		mc->future_surface = batch->future_surface;
		mc->surface_tex_inv_size.x = 1.0f / surface->texture->width[0];
		mc->surface_tex_inv_size.y = 1.0f / surface->texture->height[0];

		for (i = 0; i < 3; ++i)
		{
			mc->tex_transfer[i] = mc->pipe->screen->get_tex_transfer
			(
				mc->pipe->screen,
				mc->textures.all[i],
				0, 0, 0, PIPE_TRANSFER_WRITE, 0, 0,
				surface->texture->width[0],
				surface->texture->height[0]
			);

			mc->texels[i] = mc->pipe->screen->transfer_map(mc->pipe->screen, mc->tex_transfer[i]);
		}
	}

	for (i = 0; i < batch->num_macroblocks; ++i)
		vlGrabMacroBlock(mc, &batch->macroblocks[i]);

	return 0;
}

static inline int vlEnd
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
	unsigned int			i;

	assert(render);

	mc = (struct vlR16SnormBufferedMC*)render;
	pipe = mc->pipe;

	for (i = 0; i < 5; ++i)
		pipe->delete_sampler_state(pipe, mc->samplers.all[i]);

	for (i = 0; i < 3; ++i)
		pipe_buffer_reference(&mc->vertex_bufs.all[i].buffer, NULL);

	/* Textures 3 & 4 are not created directly, no need to release them here */
	for (i = 0; i < 3; ++i)
		pipe_texture_reference(&mc->textures.all[i], NULL);

	pipe->delete_vs_state(pipe, mc->i_vs);
	pipe->delete_fs_state(pipe, mc->i_fs);

	for (i = 0; i < 2; ++i)
	{
		pipe->delete_vs_state(pipe, mc->p_vs[i]);
		pipe->delete_fs_state(pipe, mc->p_fs[i]);
		pipe->delete_vs_state(pipe, mc->b_vs[i]);
		pipe->delete_fs_state(pipe, mc->b_fs[i]);
	}

	pipe_buffer_reference(&mc->vs_const_buf.buffer, NULL);
	pipe_buffer_reference(&mc->fs_const_buf.buffer, NULL);

	FREE(mc->macroblocks);
	FREE(mc);

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

#include "vl_r16snorm_mc_buf_shaders.inc"

static int vlCreateDataBufs
(
	struct vlR16SnormBufferedMC *mc
)
{
	const unsigned int	mbw = align(mc->picture_width, VL_MACROBLOCK_WIDTH) / VL_MACROBLOCK_WIDTH;
	const unsigned int	mbh = align(mc->picture_height, VL_MACROBLOCK_HEIGHT) / VL_MACROBLOCK_HEIGHT;

	struct pipe_context	*pipe;
	unsigned int		i;

	assert(mc);

	pipe = mc->pipe;
	mc->macroblocks_per_picture = mbw * mbh;

	/* Create our vertex buffers */
	mc->vertex_bufs.ycbcr.stride = sizeof(struct vlVertex2f) * 4;
	mc->vertex_bufs.ycbcr.max_index = 24 * mc->macroblocks_per_picture - 1;
	mc->vertex_bufs.ycbcr.buffer_offset = 0;
	mc->vertex_bufs.ycbcr.buffer = pipe_buffer_create
	(
		pipe->screen,
		DEFAULT_BUF_ALIGNMENT,
		PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_DISCARD,
		sizeof(struct vlVertex2f) * 4 * 24 * mc->macroblocks_per_picture
	);

	for (i = 1; i < 3; ++i)
	{
		mc->vertex_bufs.all[i].stride = sizeof(struct vlVertex2f) * 2;
		mc->vertex_bufs.all[i].max_index = 24 * mc->macroblocks_per_picture - 1;
		mc->vertex_bufs.all[i].buffer_offset = 0;
		mc->vertex_bufs.all[i].buffer = pipe_buffer_create
		(
			pipe->screen,
			DEFAULT_BUF_ALIGNMENT,
			PIPE_BUFFER_USAGE_VERTEX | PIPE_BUFFER_USAGE_DISCARD,
			sizeof(struct vlVertex2f) * 2 * 24 * mc->macroblocks_per_picture
		);
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
	mc->vs_const_buf.buffer = pipe_buffer_create
	(
		pipe->screen,
		DEFAULT_BUF_ALIGNMENT,
		PIPE_BUFFER_USAGE_CONSTANT | PIPE_BUFFER_USAGE_DISCARD,
		sizeof(struct vlVertexShaderConsts)
	);

	mc->fs_const_buf.buffer = pipe_buffer_create
	(
		pipe->screen,
		DEFAULT_BUF_ALIGNMENT,
		PIPE_BUFFER_USAGE_CONSTANT,
		sizeof(struct vlFragmentShaderConsts)
	);

	memcpy
	(
		pipe_buffer_map(pipe->screen, mc->fs_const_buf.buffer, PIPE_BUFFER_USAGE_CPU_WRITE),
		&fs_consts,
		sizeof(struct vlFragmentShaderConsts)
	);

	pipe_buffer_unmap(pipe->screen, mc->fs_const_buf.buffer);

	mc->macroblocks = MALLOC(sizeof(struct vlMpeg2MacroBlock) * mc->macroblocks_per_picture);

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

	mc->buffered_surface = NULL;
	mc->past_surface = NULL;
	mc->future_surface = NULL;
	for (i = 0; i < 3; ++i)
		mc->zero_block[i].x = -1.0f;
	mc->num_macroblocks = 0;

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
	mc->render_target.nr_cbufs = 1;
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
		mc->samplers.all[i] = pipe->create_sampler_state(pipe, &sampler);
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
	template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER | PIPE_TEXTURE_USAGE_DYNAMIC;

	mc->textures.y = pipe->screen->texture_create(pipe->screen, &template);

	if (mc->picture_format == vlFormatYCbCr420)
	{
		template.width[0] = vlRoundUpPOT(mc->picture_width / 2);
		template.height[0] = vlRoundUpPOT(mc->picture_height / 2);
	}
	else if (mc->picture_format == vlFormatYCbCr422)
		template.height[0] = vlRoundUpPOT(mc->picture_height / 2);

	mc->textures.cb = pipe->screen->texture_create(pipe->screen, &template);
	mc->textures.cr = pipe->screen->texture_create(pipe->screen, &template);

	/* textures.all[3] & textures.all[4] are assigned from vlSurfaces for P and B macroblocks at render time */

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
	struct vlR16SnormBufferedMC *mc;

	assert(pipe);
	assert(render);

	mc = CALLOC_STRUCT(vlR16SnormBufferedMC);

	mc->base.vlBegin = &vlBegin;
	mc->base.vlRenderMacroBlocksMpeg2 = &vlRenderMacroBlocksMpeg2R16SnormBuffered;
	mc->base.vlEnd = &vlEnd;
	mc->base.vlFlush = &vlFlush;
	mc->base.vlDestroy = &vlDestroy;
	mc->pipe = pipe;
	mc->picture_width = picture_width;
	mc->picture_height = picture_height;

	vlInit(mc);

	*render = &mc->base;

	return 0;
}
