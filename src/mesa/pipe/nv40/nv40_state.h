#ifndef __NV40_STATE_H__
#define __NV40_STATE_H__

#include "pipe/p_state.h"

struct nv40_alpha_test_state {
	uint32_t enabled;
	uint32_t func;
	uint32_t ref;
};

struct nv40_blend_state {
	uint32_t b_enable;
	uint32_t b_srcfunc;
	uint32_t b_dstfunc;
	uint32_t b_eqn;

	uint32_t l_enable;
	uint32_t l_op;

	uint32_t c_mask;

	uint32_t d_enable;
};

struct nv40_sampler_state {
	uint32_t fmt;
	uint32_t wrap;
	uint32_t en;
	uint32_t filt;
	uint32_t bcol;
};

struct nv40_rasterizer_state {
	uint32_t shade_model;

	uint32_t line_width;
	uint32_t line_smooth_en;
	uint32_t line_stipple_en;
	uint32_t line_stipple;

	uint32_t point_size;

	uint32_t poly_smooth_en;
	uint32_t poly_stipple_en;
	
	uint32_t poly_mode_front;
	uint32_t poly_mode_back;

	uint32_t front_face;
	uint32_t cull_face;
	uint32_t cull_face_en;

	uint32_t point_sprite;
};

struct nv40_vertex_program_exec {
	uint32_t data[4];
	boolean has_branch_offset;
	int const_index;
};

struct nv40_vertex_program_data {
	int index; /* immediates == -1 */
	float value[4];
};

struct nv40_vertex_program {
	const struct pipe_shader_state *pipe;

	boolean translated;
	struct nv40_vertex_program_exec *insns;
	unsigned nr_insns;
	struct nv40_vertex_program_data *consts;
	unsigned nr_consts;

	struct nouveau_resource *exec;
	unsigned exec_start;
	struct nouveau_resource *data;
	unsigned data_start;
	unsigned data_start_min;

	uint32_t ir;
	uint32_t or;
};

struct nv40_fragment_program_data {
	unsigned offset;
	unsigned index;
};

struct nv40_fragment_program {
	const struct pipe_shader_state *pipe;

	boolean translated;
	boolean on_hw;

	uint32_t *insn;
	int       insn_len;

	struct nv40_fragment_program_data *consts;
	unsigned nr_consts;

	struct pipe_buffer_handle *buffer;

	boolean uses_kil;
	boolean writes_depth;
	int     num_regs;
};

struct nv40_depth_push {
	uint32_t func;
	uint32_t write_enable;
	uint32_t test_enable;
};

struct nv40_stencil_push {
	uint32_t enable;
	uint32_t wmask;
	uint32_t func;
	uint32_t ref;
	uint32_t vmask;
	uint32_t fail;
	uint32_t zfail;
	uint32_t zpass;
};

struct nv40_depth_stencil_state {
	struct nv40_depth_push depth;
	union {
		struct nv40_stencil_push back;
		struct nv40_stencil_push front;
	} stencil;
};

struct nv40_miptree {
	struct pipe_texture base;

	struct pipe_buffer_handle *buffer;
	uint total_size;

	struct {
		uint pitch;
		uint *image_offset;
	} level[PIPE_MAX_TEXTURE_LEVELS];
};

#endif
