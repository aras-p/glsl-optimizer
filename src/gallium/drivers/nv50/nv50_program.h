#ifndef __NV50_PROGRAM_H__
#define __NV50_PROGRAM_H__

#include "pipe/p_state.h"
#include "tgsi/util/tgsi_scan.h"

struct nv50_program_data {
	int index;
	int component;
	float value;
};

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
	struct nv50_program_data *data;
	struct nouveau_resource *data_res;
	unsigned data_nr;
	unsigned data_start;

	struct pipe_buffer *buffer;

	float *immd;
	unsigned immd_nr;

	struct {
		unsigned high_temp;
		struct {
			unsigned attr[2];
		} vp;
	} cfg;
};

#endif
