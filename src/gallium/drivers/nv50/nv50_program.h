#ifndef __NV50_PROGRAM_H__
#define __NV50_PROGRAM_H__

#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h"

struct nv50_program_exec {
	struct nv50_program_exec *next;

	unsigned inst[2];
	struct {
		int index;
		unsigned mask;
		unsigned shift;
	} param;
};

struct nv50_program {
	struct pipe_shader_state pipe;
	struct tgsi_shader_info info;
	boolean translated;

	unsigned type;
	struct nv50_program_exec *exec_head;
	struct nv50_program_exec *exec_tail;
	unsigned exec_size;
	struct nouveau_resource *data[2];
	unsigned data_start[2];

	struct nouveau_bo *bo;

	float *immd;
	unsigned immd_nr;
	unsigned param_nr;

	struct {
		unsigned high_temp;
		unsigned high_result;
		struct {
			unsigned attr[2];
		} vp;
		struct {
			unsigned regs[4];
			unsigned map[5];
			unsigned high_map;
		} fp;
	} cfg;
};

#endif
