/*
 * Copyright 2012 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Tom Stellard <thomas.stellard@amd.com>
 *	Michel Dänzer <michel.daenzer@amd.com>
 *      Christian König <christian.koenig@amd.com>
 */

#ifndef RADEONSI_SHADER_H
#define RADEONSI_SHADER_H

#include <llvm-c/Core.h> /* LLVMModuleRef */

#define SI_SGPR_CONST		0
#define SI_SGPR_SAMPLER		2
#define SI_SGPR_RESOURCE	4
#define SI_SGPR_VERTEX_BUFFER	6
#define SI_SGPR_START_INSTANCE	8

#define SI_VS_NUM_USER_SGPR	9
#define SI_PS_NUM_USER_SGPR	6

/* LLVM function parameter indices */
#define SI_PARAM_CONST		0
#define SI_PARAM_SAMPLER	1
#define SI_PARAM_RESOURCE	2

/* VS only parameters */
#define SI_PARAM_VERTEX_BUFFER	3
#define SI_PARAM_START_INSTANCE	4
#define SI_PARAM_VERTEX_ID	5
#define SI_PARAM_DUMMY_0	6
#define SI_PARAM_DUMMY_1	7
#define SI_PARAM_INSTANCE_ID	8

/* PS only parameters */
#define SI_PARAM_PRIM_MASK		3
#define SI_PARAM_PERSP_SAMPLE		4
#define SI_PARAM_PERSP_CENTER		5
#define SI_PARAM_PERSP_CENTROID		6
#define SI_PARAM_PERSP_PULL_MODEL	7
#define SI_PARAM_LINEAR_SAMPLE		8
#define SI_PARAM_LINEAR_CENTER		9
#define SI_PARAM_LINEAR_CENTROID	10
#define SI_PARAM_LINE_STIPPLE_TEX	11
#define SI_PARAM_POS_X_FLOAT		12
#define SI_PARAM_POS_Y_FLOAT		13
#define SI_PARAM_POS_Z_FLOAT		14
#define SI_PARAM_POS_W_FLOAT		15
#define SI_PARAM_FRONT_FACE		16
#define SI_PARAM_ANCILLARY		17
#define SI_PARAM_SAMPLE_COVERAGE	18
#define SI_PARAM_POS_FIXED_PT		19

struct si_shader_io {
	unsigned		name;
	int			sid;
	unsigned		param_offset;
	unsigned		interpolate;
	bool			centroid;
};

struct si_pipe_shader;

struct si_pipe_shader_selector {
	struct si_pipe_shader *current;

	struct tgsi_token       *tokens;
	struct pipe_stream_output_info  so;

	unsigned	num_shaders;

	/* PIPE_SHADER_[VERTEX|FRAGMENT|...] */
	unsigned	type;

	/* 1 when the shader contains
	 * TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS, otherwise it's 0.
	 * Used to determine whether we need to include nr_cbufs in the key */
	unsigned	fs_write_all;
};

struct si_shader {
	unsigned		ninput;
	struct si_shader_io	input[32];

	unsigned		noutput;
	struct si_shader_io	output[32];

	unsigned		ninterp;
	bool			uses_kill;
	bool			uses_instanceid;
	bool			fs_write_all;
	unsigned		nr_cbufs;
};

union si_shader_key {
	struct {
		unsigned	export_16bpc:8;
		unsigned	nr_cbufs:4;
		unsigned	color_two_side:1;
		unsigned	alpha_func:3;
		unsigned	flatshade:1;
		float		alpha_ref;
	} ps;
	struct {
		unsigned	instance_divisors[PIPE_MAX_ATTRIBS];
	} vs;
};

struct si_pipe_shader {
	struct si_pipe_shader_selector	*selector;
	struct si_pipe_shader		*next_variant;
	struct si_shader		shader;
	struct si_pm4_state		*pm4;
	struct si_resource		*bo;
	unsigned			num_sgprs;
	unsigned			num_vgprs;
	unsigned			spi_ps_input_ena;
	unsigned			spi_shader_col_format;
	unsigned			sprite_coord_enable;
	unsigned			so_strides[4];
	union si_shader_key		key;
};

/* radeonsi_shader.c */
int si_pipe_shader_create(struct pipe_context *ctx, struct si_pipe_shader *shader);
int si_pipe_shader_create(struct pipe_context *ctx, struct si_pipe_shader *shader);
int si_compile_llvm(struct r600_context *rctx, struct si_pipe_shader *shader,
							LLVMModuleRef mod);
void si_pipe_shader_destroy(struct pipe_context *ctx, struct si_pipe_shader *shader);

#endif
