#ifndef __NV50_STATE_H__
#define __NV50_STATE_H__

#include "pipe/p_state.h"
#include "tgsi/util/tgsi_scan.h"

struct nv50_program_data {
	int index;
	int component;
	float value;
};

struct nv50_program {
	struct pipe_shader_state pipe;
	struct tgsi_shader_info info;
	boolean translated;

	unsigned type;
	unsigned *insns;
	unsigned insns_nr;

	struct pipe_buffer *buffer;

	float *immd;
	unsigned immd_nr;

	struct nouveau_resource *data;

	struct {
		unsigned high_temp;
		struct {
			unsigned attr[2];
		} vp;
	} cfg;
};

#endif
