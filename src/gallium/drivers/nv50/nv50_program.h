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

struct nv50_sreg4 {
	uint8_t hw;
	uint8_t id_vp;
	uint8_t id_fp;

	uint8_t mask;
	boolean linear;
};

struct nv50_program {
	struct pipe_shader_state pipe;
	struct tgsi_shader_info info;
	boolean translated;

	unsigned type;
	struct nv50_program_exec *exec_head;
	struct nv50_program_exec *exec_tail;
	unsigned exec_size;
	struct nouveau_resource *data[1];
	unsigned data_start[1];

	struct nouveau_bo *bo;

	float *immd;
	unsigned immd_nr;
	unsigned param_nr;

	struct {
		unsigned high_temp;
		unsigned high_result;

		uint32_t attr[2];
		uint32_t regs[4];

		/* for VPs, io_nr doesn't count 'private' results (PSIZ etc.) */
		unsigned io_nr;
		struct nv50_sreg4 io[PIPE_MAX_SHADER_OUTPUTS];

		/* FP colour inputs, VP/GP back colour outputs */
		struct nv50_sreg4 two_side[2];

		/* VP only */
		uint8_t clpd, clpd_nr;
		uint8_t psiz;
	} cfg;
};

#endif
