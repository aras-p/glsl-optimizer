#ifndef NVFX_TEX_H_
#define NVFX_TEX_H_

#include "util/u_math.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"


static inline unsigned
nvfx_tex_wrap_mode(unsigned wrap) {
	unsigned ret;

	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		ret = NV30_3D_TEX_WRAP_S_REPEAT;
		break;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		ret = NV30_3D_TEX_WRAP_S_MIRRORED_REPEAT;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		ret = NV30_3D_TEX_WRAP_S_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		ret = NV30_3D_TEX_WRAP_S_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_CLAMP:
		ret = NV30_3D_TEX_WRAP_S_CLAMP;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		ret = NV40_3D_TEX_WRAP_S_MIRROR_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		ret = NV40_3D_TEX_WRAP_S_MIRROR_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		ret = NV40_3D_TEX_WRAP_S_MIRROR_CLAMP;
		break;
	default:
		assert(0);
		ret = NV30_3D_TEX_WRAP_S_REPEAT;
		break;
	}

	return ret >> NV30_3D_TEX_WRAP_S__SHIFT;
}

static inline unsigned
nvfx_tex_wrap_compare_mode(unsigned func)
{
	switch (func) {
	case PIPE_FUNC_NEVER:
		return NV30_3D_TEX_WRAP_RCOMP_NEVER;
	case PIPE_FUNC_GREATER:
		return NV30_3D_TEX_WRAP_RCOMP_GREATER;
	case PIPE_FUNC_EQUAL:
		return NV30_3D_TEX_WRAP_RCOMP_EQUAL;
	case PIPE_FUNC_GEQUAL:
		return NV30_3D_TEX_WRAP_RCOMP_GEQUAL;
	case PIPE_FUNC_LESS:
		return NV30_3D_TEX_WRAP_RCOMP_LESS;
	case PIPE_FUNC_NOTEQUAL:
		return NV30_3D_TEX_WRAP_RCOMP_NOTEQUAL;
	case PIPE_FUNC_LEQUAL:
		return NV30_3D_TEX_WRAP_RCOMP_LEQUAL;
	case PIPE_FUNC_ALWAYS:
		return NV30_3D_TEX_WRAP_RCOMP_ALWAYS;
	default:
		assert(0);
		return 0;
	}
}

static inline unsigned nvfx_tex_filter(const struct pipe_sampler_state* cso)
{
	unsigned filter = 0;
	switch (cso->mag_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		filter |= NV30_3D_TEX_FILTER_MAG_LINEAR;
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		filter |= NV30_3D_TEX_FILTER_MAG_NEAREST;
		break;
	}

	switch (cso->min_img_filter) {
	case PIPE_TEX_FILTER_LINEAR:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV30_3D_TEX_FILTER_MIN_LINEAR_MIPMAP_NEAREST;
			break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV30_3D_TEX_FILTER_MIN_LINEAR_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV30_3D_TEX_FILTER_MIN_LINEAR;
			break;
		}
		break;
	case PIPE_TEX_FILTER_NEAREST:
	default:
		switch (cso->min_mip_filter) {
		case PIPE_TEX_MIPFILTER_NEAREST:
			filter |= NV30_3D_TEX_FILTER_MIN_NEAREST_MIPMAP_NEAREST;
		break;
		case PIPE_TEX_MIPFILTER_LINEAR:
			filter |= NV30_3D_TEX_FILTER_MIN_NEAREST_MIPMAP_LINEAR;
			break;
		case PIPE_TEX_MIPFILTER_NONE:
		default:
			filter |= NV30_3D_TEX_FILTER_MIN_NEAREST;
			break;
		}
		break;
	}
	return filter;
}

static inline unsigned nvfx_tex_border_color(const float* border_color)
{
	return ((float_to_ubyte(border_color[3]) << 24) |
		    (float_to_ubyte(border_color[0]) << 16) |
		    (float_to_ubyte(border_color[1]) <<  8) |
		    (float_to_ubyte(border_color[2]) <<  0));
}

struct nvfx_sampler_state {
	uint32_t fmt;
	uint32_t wrap;
	uint32_t en;
	uint32_t filt;
	uint32_t bcol;
	uint32_t min_lod;
	uint32_t max_lod;
	boolean compare;
};

struct nvfx_sampler_view {
	struct pipe_sampler_view base;
	int offset;
	uint32_t swizzle;
	uint32_t npot_size;
	uint32_t filt;
	uint32_t wrap_mask;
	uint32_t wrap;
	uint32_t lod_offset;
	uint32_t max_lod_limit;
	union
	{
		struct
		{
			uint32_t fmt[4]; /* nv30 has 4 entries, nv40 one */
			int rect;
		} nv30;
		struct
		{
			uint32_t fmt[2]; /* nv30 has 4 entries, nv40 one */
			uint32_t npot_size2; /* nv40 only */
		} nv40;
		uint32_t init_fmt;
	} u;
};

struct nvfx_texture_format {
	int fmt[6];
	unsigned sign;
	unsigned wrap;
	unsigned char src[6];
	unsigned char comp[6];
};

extern struct nvfx_texture_format nvfx_texture_formats[PIPE_FORMAT_COUNT];

#endif /* NVFX_TEX_H_ */
