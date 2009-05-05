#ifndef __NV04_STATE_H__
#define __NV04_STATE_H__

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"

struct nv04_blend_state {
	uint32_t b_enable;
	uint32_t b_src;
	uint32_t b_dst;
};

struct nv04_fragtex_state {
	uint32_t format;
};

struct nv04_sampler_state {
	uint32_t filter;
	uint32_t format;
};

struct nv04_depth_stencil_alpha_state {
	uint32_t control;
};

struct nv04_rasterizer_state {
	uint32_t blend;

	const struct pipe_rasterizer_state *templ;
};

struct nv04_miptree {
	struct pipe_texture base;

	struct pipe_buffer *buffer;
	uint total_size;

	struct {
		uint pitch;
		uint *image_offset;
	} level[PIPE_MAX_TEXTURE_LEVELS];
};

struct nv04_fragment_program_data {
	unsigned offset;
	unsigned index;
};

struct nv04_fragment_program {
	struct pipe_shader_state pipe;
	struct tgsi_shader_info info;

	boolean translated;
	boolean on_hw;
	unsigned samplers;

	uint32_t *insn;
	int       insn_len;

	struct nv04_fragment_program_data *consts;
	unsigned nr_consts;

	struct pipe_buffer *buffer;

	uint32_t fp_control;
	uint32_t fp_reg_control;
};



#endif
