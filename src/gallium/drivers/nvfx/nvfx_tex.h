#ifndef NVFX_TEX_H_
#define NVFX_TEX_H_

static inline unsigned
nvfx_tex_wrap_mode(unsigned wrap) {
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
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		ret = NV40TCL_TEX_WRAP_S_MIRROR_CLAMP_TO_EDGE;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
		ret = NV40TCL_TEX_WRAP_S_MIRROR_CLAMP_TO_BORDER;
		break;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
		ret = NV40TCL_TEX_WRAP_S_MIRROR_CLAMP;
		break;
	default:
		NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
		ret = NV34TCL_TX_WRAP_S_REPEAT;
		break;
	}

	return ret >> NV34TCL_TX_WRAP_S_SHIFT;
}

static inline unsigned
nvfx_tex_wrap_compare_mode(const struct pipe_sampler_state* cso)
{
	if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
		switch (cso->compare_func) {
		case PIPE_FUNC_NEVER:
			return NV34TCL_TX_WRAP_RCOMP_NEVER;
		case PIPE_FUNC_GREATER:
			return NV34TCL_TX_WRAP_RCOMP_GREATER;
		case PIPE_FUNC_EQUAL:
			return NV34TCL_TX_WRAP_RCOMP_EQUAL;
		case PIPE_FUNC_GEQUAL:
			return NV34TCL_TX_WRAP_RCOMP_GEQUAL;
		case PIPE_FUNC_LESS:
			return NV34TCL_TX_WRAP_RCOMP_LESS;
		case PIPE_FUNC_NOTEQUAL:
			return NV34TCL_TX_WRAP_RCOMP_NOTEQUAL;
		case PIPE_FUNC_LEQUAL:
			return NV34TCL_TX_WRAP_RCOMP_LEQUAL;
		case PIPE_FUNC_ALWAYS:
			return NV34TCL_TX_WRAP_RCOMP_ALWAYS;
		default:
			break;
		}
	}
	return 0;
}

static inline unsigned nvfx_tex_filter(const struct pipe_sampler_state* cso)
{
	unsigned filter = 0;
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
};

#endif /* NVFX_TEX_H_ */
