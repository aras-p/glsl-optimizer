/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef FD3_PROGRAM_H_
#define FD3_PROGRAM_H_

#include "pipe/p_context.h"

#include "freedreno_context.h"

#include "ir-a3xx.h"
#include "disasm.h"

struct fd3_shader_stateobj {
	enum shader_t type;

	struct fd_bo *bo;

	struct ir3_shader_info info;
	struct ir3_shader *ir;

	/* is shader using (or more precisely, is color_regid) half-
	 * precision register?
	 */
	bool half_precision;

	/* special output register locations: */
	uint8_t pos_regid, psize_regid, color_regid;

	/* the instructions length is in units of instruction groups
	 * (4 instructions, 8 dwords):
	 */
	unsigned instrlen;

	/* the constants length is in units of vec4's, and is the sum of
	 * the uniforms and the built-in compiler constants
	 */
	unsigned constlen;

	/* About Linkage:
	 *   + Let the frag shader determine the position/compmask for the
	 *     varyings, since it is the place where we know if the varying
	 *     is actually used, and if so, which components are used.  So
	 *     what the hw calls "outloc" is taken from the "inloc" of the
	 *     frag shader.
	 *   + From the vert shader, we only need the output regid
	 */

	/* varyings/outputs: */
	unsigned outputs_count;
	struct {
		uint8_t regid;
	} outputs[16];

	/* vertices/inputs: */
	unsigned inputs_count;
	struct {
		uint8_t regid;
		uint8_t compmask;
		/* in theory inloc of fs should match outloc of vs: */
		uint8_t inloc;
	} inputs[16];

	unsigned total_in;       /* sum of inputs (scalar) */

	/* samplers: */
	unsigned samplers_count;

	/* const reg # of first immediate, ie. 1 == c1
	 * (not regid, because TGSI thinks in terms of vec4 registers,
	 * not scalar registers)
	 */
	unsigned first_immediate;
	unsigned immediates_count;
	struct {
		uint32_t val[4];
	} immediates[64];

	/* so far, only used for blit_prog shader.. values for
	 * VPC_VARYING_INTERP[i].MODE and VPC_VARYING_PS_REPL[i].MODE
	 */
	uint32_t vinterp[4], vpsrepl[4];
};

void fd3_program_emit(struct fd_ringbuffer *ring,
		struct fd_program_stateobj *prog);

void fd3_prog_init(struct pipe_context *pctx);
void fd3_prog_fini(struct pipe_context *pctx);

#endif /* FD3_PROGRAM_H_ */
