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

#define SI_SGPR_CONST		0
#define SI_SGPR_SAMPLER		2
#define SI_SGPR_RESOURCE	4
#define SI_SGPR_VERTEX_BUFFER	6

#define SI_VS_NUM_USER_SGPR	8
#define SI_PS_NUM_USER_SGPR	6

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
	bool			fs_write_all;
	unsigned		nr_cbufs;
};

struct si_shader_key {
	unsigned		export_16bpc:8;
	unsigned		nr_cbufs:4;
	unsigned		color_two_side:1;
	unsigned		alpha_func:3;
	float			alpha_ref;
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
	struct si_shader_key		key;
};

/* radeonsi_shader.c */
int si_pipe_shader_create(struct pipe_context *ctx, struct si_pipe_shader *shader,
			  struct si_shader_key key);
void si_pipe_shader_destroy(struct pipe_context *ctx, struct si_pipe_shader *shader);

#endif
