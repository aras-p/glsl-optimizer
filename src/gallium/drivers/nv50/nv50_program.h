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
	struct nouveau_resource *data;
	unsigned data_start;

	struct pipe_buffer *buffer;

	float *immd;
	unsigned immd_nr;
	unsigned param_nr;

	struct {
		unsigned high_temp;
		unsigned high_result;
		struct {
			unsigned attr[2];
		} vp;
	} cfg;
};

#endif
