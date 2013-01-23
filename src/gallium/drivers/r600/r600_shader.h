/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 */
#ifndef R600_SHADER_H
#define R600_SHADER_H

#include "r600_asm.h"

struct r600_shader_io {
	unsigned		name;
	unsigned		gpr;
	unsigned		done;
	int			sid;
	int			spi_sid;
	unsigned		interpolate;
	unsigned		ij_index;
	boolean                 centroid;
	unsigned		lds_pos; /* for evergreen */
	unsigned		back_color_input;
	unsigned		write_mask;
};

struct r600_shader {
	unsigned		processor_type;
	struct r600_bytecode		bc;
	unsigned		ninput;
	unsigned		noutput;
	unsigned		nlds;
	struct r600_shader_io	input[32];
	struct r600_shader_io	output[32];
	boolean			uses_kill;
	boolean			fs_write_all;
	boolean			two_side;
	/* Number of color outputs in the TGSI shader,
	 * sometimes it could be higher than nr_cbufs (bug?).
	 * Also with writes_all property on eg+ it will be set to max CB number */
	unsigned		nr_ps_max_color_exports;
	/* Real number of ps color exports compiled in the bytecode */
	unsigned		nr_ps_color_exports;
	/* bit n is set if the shader writes gl_ClipDistance[n] */
	unsigned		clip_dist_write;
	/* flag is set if the shader writes VS_OUT_MISC_VEC (e.g. for PSIZE) */
	boolean			vs_out_misc_write;
	boolean			vs_out_point_size;
	boolean			has_txq_cube_array_z_comp;
	boolean			uses_tex_buffers;
};

struct r600_shader_key {
	unsigned color_two_side:1;
	unsigned alpha_to_one:1;
	unsigned nr_cbufs:4;
};

struct r600_pipe_shader {
	struct r600_pipe_shader_selector *selector;
	struct r600_pipe_shader	*next_variant;
	struct r600_shader	shader;
	struct r600_pipe_state	rstate;
	struct r600_resource	*bo;
	unsigned		sprite_coord_enable;
	unsigned		flatshade;
	unsigned		pa_cl_vs_out_cntl;
	unsigned		nr_ps_color_outputs;
	struct r600_shader_key	key;
	unsigned		db_shader_control;
	unsigned		ps_depth_export;
};

#endif
