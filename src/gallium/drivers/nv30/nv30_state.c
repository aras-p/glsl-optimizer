#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "tgsi/tgsi_parse.h"

#include "nv30_context.h"
#include "nvfx_state.h"

static INLINE unsigned
wrap_mode(unsigned wrap) {
	unsigned ret;

	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		ret = NV34TCL_TX_WRAP_S_REPEAT;
		break;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		ret = NV34TCL_TX_WRAP_S_MIRRORED_REPEAT;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		ret = NV34TCL_TX_WRAP_S_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		ret = NV34TCL_TX_WRAP_S_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_CLAMP:
		ret = NV34TCL_TX_WRAP_S_CLAMP;
		break;
/*	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		ret = NV34TCL_TX_WRAP_S_MIRROR_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		ret = NV34TCL_TX_WRAP_S_MIRROR_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		ret = NV34TCL_TX_WRAP_S_MIRROR_CLAMP;
		break;*/
	default:
		NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
		ret = NV34TCL_TX_WRAP_S_REPEAT;
		break;
	}

	return ret >> NV34TCL_TX_WRAP_S_SHIFT;
}

void *
nv30_sampler_state_create(struct pipe_context *pipe,
			  const struct pipe_sampler_state *cso)
{
	struct nvfx_sampler_state *ps;
	uint32_t filter = 0;

	ps = MALLOC(sizeof(struct nvfx_sampler_state));

	ps->fmt = 0;
	/* TODO: Not all RECTs formats have this bit set, bits 15-8 of format
	   are the tx format to use. We should store normalized coord flag
	   in sampler state structure, and set appropriate format in
	   nvxx_fragtex_build()
	 */
	/*NV34TCL_TX_FORMAT_RECT*/
	/*if (!cso->normalized_coords) {
		ps->fmt |= (1<<14) ;
	}*/

	ps->wrap = ((wrap_mode(cso->wrap_s) << NV34TCL_TX_WRAP_S_SHIFT) |
		    (wrap_mode(cso->wrap_t) << NV34TCL_TX_WRAP_T_SHIFT) |
		    (wrap_mode(cso->wrap_r) << NV34TCL_TX_WRAP_R_SHIFT));

	ps->en = 0;

	if (cso->max_anisotropy >= 8) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_8X;
	} else
	if (cso->max_anisotropy >= 4) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_4X;
	} else
	if (cso->max_anisotropy >= 2) {
		ps->en |= NV34TCL_TX_ENABLE_ANISO_2X;
	}

	switch (cso->mag_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		filter |= NV34TCL_TX_FILTER_MAGNIFY_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		filter |= NV34TCL_TX_FILTER_MAGNIFY_NEAREST;
		break;
	}

	switch (cso->min_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV34TCL_TX_FILTER_MINIFY_LINEAR_MIPMAP_NEAREST;
			break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV34TCL_TX_FILTER_MINIFY_LINEAR_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV34TCL_TX_FILTER_MINIFY_LINEAR;
			break;
		}
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV34TCL_TX_FILTER_MINIFY_NEAREST_MIPMAP_NEAREST;
		break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV34TCL_TX_FILTER_MINIFY_NEAREST_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV34TCL_TX_FILTER_MINIFY_NEAREST;
			break;
		}
		break;
	}

	ps->filt = filter;

	{
		float limit;

		limit = CLAMP(cso->lod_bias, -16.0, 15.0);
		ps->filt |= (int)(cso->lod_bias * 256.0) & 0x1fff;

		limit = CLAMP(cso->max_lod, 0.0, 15.0);
		ps->en |= (int)(limit) << 14 /*NV34TCL_TX_ENABLE_MIPMAP_MAX_LOD_SHIFT*/;

		limit = CLAMP(cso->min_lod, 0.0, 15.0);
		ps->en |= (int)(limit) << 26 /*NV34TCL_TX_ENABLE_MIPMAP_MIN_LOD_SHIFT*/;
	}

	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
		switch (cso->compare_func) {
		case PIPE_FUNC_NEVER:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_NEVER;
			break;
		case PIPE_FUNC_GREATER:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_GREATER;
			break;
		case PIPE_FUNC_EQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_EQUAL;
			break;
		case PIPE_FUNC_GEQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_GEQUAL;
			break;
		case PIPE_FUNC_LESS:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_LESS;
			break;
		case PIPE_FUNC_NOTEQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_NOTEQUAL;
			break;
		case PIPE_FUNC_LEQUAL:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_LEQUAL;
			break;
		case PIPE_FUNC_ALWAYS:
			ps->wrap |= NV34TCL_TX_WRAP_RCOMP_ALWAYS;
			break;
		default:
			break;
		}
	}

	ps->bcol = ((float_to_ubyte(cso->border_color[3]) << 24) |
		    (float_to_ubyte(cso->border_color[0]) << 16) |
		    (float_to_ubyte(cso->border_color[1]) <<  8) |
		    (float_to_ubyte(cso->border_color[2]) <<  0));

	return (void *)ps;
}

