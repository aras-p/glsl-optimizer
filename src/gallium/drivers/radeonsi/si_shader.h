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

#ifndef SI_SHADER_H
#define SI_SHADER_H

#include <llvm-c/Core.h> /* LLVMModuleRef */

#define SI_SGPR_CONST		0
#define SI_SGPR_SAMPLER		2
#define SI_SGPR_RESOURCE	4
#define SI_SGPR_RW_BUFFERS	6  /* rings (& stream-out, VS only) */
#define SI_SGPR_VERTEX_BUFFER	8  /* VS only */
#define SI_SGPR_START_INSTANCE	10 /* VS only */
#define SI_SGPR_ALPHA_REF	8  /* PS only */

#define SI_VS_NUM_USER_SGPR	11
#define SI_GS_NUM_USER_SGPR	8
#define SI_PS_NUM_USER_SGPR	9

/* LLVM function parameter indices */
#define SI_PARAM_CONST		0
#define SI_PARAM_SAMPLER	1
#define SI_PARAM_RESOURCE	2
#define SI_PARAM_RW_BUFFERS	3

/* VS only parameters */
#define SI_PARAM_VERTEX_BUFFER	4
#define SI_PARAM_START_INSTANCE	5
/* the other VS parameters are assigned dynamically */

/* ES only parameters */
#define SI_PARAM_ES2GS_OFFSET	6

/* GS only parameters */
#define SI_PARAM_GS2VS_OFFSET	4
#define SI_PARAM_GS_WAVE_ID	5
#define SI_PARAM_VTX0_OFFSET	6
#define SI_PARAM_VTX1_OFFSET	7
#define SI_PARAM_PRIMITIVE_ID	8
#define SI_PARAM_VTX2_OFFSET	9
#define SI_PARAM_VTX3_OFFSET	10
#define SI_PARAM_VTX4_OFFSET	11
#define SI_PARAM_VTX5_OFFSET	12
#define SI_PARAM_GS_INSTANCE_ID	13

/* PS only parameters */
#define SI_PARAM_ALPHA_REF		4
#define SI_PARAM_PRIM_MASK		5
#define SI_PARAM_PERSP_SAMPLE		6
#define SI_PARAM_PERSP_CENTER		7
#define SI_PARAM_PERSP_CENTROID		8
#define SI_PARAM_PERSP_PULL_MODEL	9
#define SI_PARAM_LINEAR_SAMPLE		10
#define SI_PARAM_LINEAR_CENTER		11
#define SI_PARAM_LINEAR_CENTROID	12
#define SI_PARAM_LINE_STIPPLE_TEX	13
#define SI_PARAM_POS_X_FLOAT		14
#define SI_PARAM_POS_Y_FLOAT		15
#define SI_PARAM_POS_Z_FLOAT		16
#define SI_PARAM_POS_W_FLOAT		17
#define SI_PARAM_FRONT_FACE		18
#define SI_PARAM_ANCILLARY		19
#define SI_PARAM_SAMPLE_COVERAGE	20
#define SI_PARAM_POS_FIXED_PT		21

#define SI_NUM_PARAMS (SI_PARAM_POS_FIXED_PT + 1)

struct si_shader_input {
	unsigned		name;
	int			sid;
	unsigned		param_offset;
	unsigned		index;
	unsigned		interpolate;
	bool			centroid;
};

struct si_shader_output {
	unsigned		name;
	int			sid;
	unsigned		param_offset;
	unsigned		index;
	unsigned		usage;
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
	struct si_shader_input	input[40];

	unsigned		noutput;
	struct si_shader_output	output[40];

	/* geometry shader properties */
	unsigned		gs_input_prim;
	unsigned		gs_output_prim;
	unsigned		gs_max_out_vertices;

	unsigned		nparam;
	bool			uses_kill;
	bool			uses_instanceid;
	bool			fs_write_all;
	bool			vs_out_misc_write;
	bool			vs_out_point_size;
	bool			vs_out_edgeflag;
	bool			vs_out_layer;
	unsigned		nr_pos_exports;
	unsigned		clip_dist_write;
};

union si_shader_key {
	struct {
		unsigned	export_16bpc:8;
		unsigned	nr_cbufs:4;
		unsigned	color_two_side:1;
		unsigned	alpha_func:3;
		unsigned	flatshade:1;
		unsigned	alpha_to_one:1;
	} ps;
	struct {
		unsigned	instance_divisors[PIPE_MAX_ATTRIBS];
		unsigned	ucps_enabled:2;
		unsigned	as_es:1;
	} vs;
};

struct si_pipe_shader {
	struct si_pipe_shader_selector	*selector;
	struct si_pipe_shader		*next_variant;
	struct si_pipe_shader		*gs_copy_shader;
	struct si_shader		shader;
	struct si_pm4_state		*pm4;
	struct r600_resource		*bo;
	unsigned			num_sgprs;
	unsigned			num_vgprs;
	unsigned			lds_size;
	unsigned			spi_ps_input_ena;
	unsigned			spi_shader_col_format;
	unsigned			cb_shader_mask;
	bool				cb0_is_integer;
	unsigned			sprite_coord_enable;
	union si_shader_key		key;
};

static inline struct si_shader* si_get_vs_state(struct si_context *sctx)
{
	if (sctx->gs_shader)
		return &sctx->gs_shader->current->gs_copy_shader->shader;
	else
		return &sctx->vs_shader->current->shader;
}

/* radeonsi_shader.c */
int si_pipe_shader_create(struct pipe_context *ctx, struct si_pipe_shader *shader);
int si_pipe_shader_create(struct pipe_context *ctx, struct si_pipe_shader *shader);
int si_compile_llvm(struct si_context *sctx, struct si_pipe_shader *shader,
							LLVMModuleRef mod);
void si_pipe_shader_destroy(struct pipe_context *ctx, struct si_pipe_shader *shader);

#endif
