#ifndef __NV40_STATE_H__
#define __NV40_STATE_H__

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"

struct nv40_sampler_state {
	uint32_t fmt;
	uint32_t wrap;
	uint32_t en;
	uint32_t filt;
	uint32_t bcol;
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
	struct pipe_shader_state pipe;

	struct draw_vertex_shader *draw;

	boolean translated;

	struct pipe_clip_state ucp;

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
	uint32_t clip_ctrl;
	struct nouveau_stateobj *so;
};

struct nv40_fragment_program_data {
	unsigned offset;
	unsigned index;
};

struct nv40_fragment_program {
	struct pipe_shader_state pipe;
	struct tgsi_shader_info info;

	boolean translated;
	unsigned samplers;

	uint32_t *insn;
	int       insn_len;

	struct nv40_fragment_program_data *consts;
	unsigned nr_consts;

	struct pipe_buffer *buffer;

	uint32_t fp_control;
	struct nouveau_stateobj *so;
};

struct nv40_miptree {
	struct pipe_texture base;
	struct nouveau_bo *bo;

	struct pipe_buffer *buffer;
	uint total_size;

	struct {
		uint pitch;
		uint *image_offset;
	} level[PIPE_MAX_TEXTURE_LEVELS];
};

#endif
