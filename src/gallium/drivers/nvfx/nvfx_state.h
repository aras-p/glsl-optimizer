#ifndef __NVFX_STATE_H__
#define __NVFX_STATE_H__

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"
#include "nouveau/nouveau_statebuf.h"
#include "util/u_dynarray.h"
#include "util/u_linkage.h"

struct nvfx_vertex_program_exec {
	uint32_t data[4];
};

struct nvfx_vertex_program_data {
	int index; /* immediates == -1 */
	float value[4];
};

struct nvfx_vertex_program {
	unsigned long long id;

	struct nvfx_vertex_program_exec *insns;
	unsigned nr_insns;
	struct nvfx_vertex_program_data *consts;
	unsigned nr_consts;

	char generic_to_fp_input[256];
	int sprite_fp_input;

	struct nouveau_resource *exec;
	unsigned exec_start;
	struct nouveau_resource *data;
	unsigned data_start;
	unsigned data_start_min;

	uint32_t ir;
	uint32_t or;
	int clip_nr;

	struct util_dynarray branch_relocs;
	struct util_dynarray const_relocs;
};

#define NVFX_VP_FAILED ((struct nvfx_vertex_program*)-1)

struct nvfx_pipe_vertex_program {
	struct pipe_shader_state pipe;
	struct tgsi_shader_info info;

	unsigned draw_elements;
	boolean draw_no_elements;
	struct draw_vertex_shader *draw_vs;
	struct nvfx_vertex_program* draw_vp;

	struct nvfx_vertex_program* vp;
};

struct nvfx_fragment_program_data {
	unsigned offset;
	unsigned index;
};

struct nvfx_fragment_program_bo {
	struct nvfx_fragment_program_bo* next;
	struct nouveau_bo* bo;
	unsigned char* slots;
	char insn[] __attribute__((aligned(16)));
};

struct nvfx_fragment_program {
	unsigned samplers;
	unsigned point_sprite_control;
	unsigned or;
	unsigned coord_conventions;

	uint32_t *insn;
	int       insn_len;

	struct nvfx_fragment_program_data *consts;
	unsigned nr_consts;

	/* the slot at num_slots is for the sprite coordinate, if any */
	unsigned num_slots; /* how many input semantics? */
	unsigned char slot_to_generic[10]; /* semantics */
	unsigned char slot_to_fp_input[11]; /* current assignment of slots for each used semantic */
	struct util_dynarray slot_relocations[11];

	/* This is reset to progs on any relocation update, and decreases every time we
	 * move to a new prog due to a constant update
	 * When this is the same as progs, applying relocations is no longer necessary.
	 */
	unsigned progs_left_with_obsolete_slot_assignments;

	unsigned long long last_vp_id;
	unsigned last_sprite_coord_enable;

	uint32_t fp_control;

	unsigned bo_prog_idx;
	unsigned prog_size;
	unsigned progs_per_bo;
	unsigned progs;

	struct nvfx_fragment_program_bo* fpbo;
};

struct nvfx_pipe_fragment_program {
        struct pipe_shader_state pipe;
        struct tgsi_shader_info info;

        struct nvfx_fragment_program* fps[2];
};

#endif
