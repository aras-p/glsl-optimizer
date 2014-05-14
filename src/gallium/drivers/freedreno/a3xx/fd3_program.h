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
#include "fd3_util.h"
#include "ir3.h"
#include "disasm.h"

typedef uint16_t fd3_semantic;  /* semantic name + index */
static inline fd3_semantic
fd3_semantic_name(uint8_t name, uint16_t index)
{
	return (name << 8) | (index & 0xff);
}

static inline uint8_t sem2name(fd3_semantic sem)
{
	return sem >> 8;
}

static inline uint16_t sem2idx(fd3_semantic sem)
{
	return sem & 0xff;
}

struct fd3_shader_variant {
	struct fd_bo *bo;

	struct fd3_shader_key key;

	struct ir3_shader_info info;
	struct ir3_shader *ir;

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

	/* for frag shader, pos_regid holds the frag_pos, ie. what is passed
	 * to bary.f instructions
	 */
	uint8_t pos_regid;
	bool frag_coord, frag_face;

	/* varyings/outputs: */
	unsigned outputs_count;
	struct {
		fd3_semantic semantic;
		uint8_t regid;
	} outputs[16 + 2];  /* +POSITION +PSIZE */
	bool writes_pos, writes_psize;

	/* vertices/inputs: */
	unsigned inputs_count;
	struct {
		fd3_semantic semantic;
		uint8_t regid;
		uint8_t compmask;
		uint8_t ncomp;
		/* in theory inloc of fs should match outloc of vs: */
		uint8_t inloc;
		uint8_t bary;
	} inputs[16 + 2];  /* +POSITION +FACE */

	unsigned total_in;       /* sum of inputs (scalar) */

	/* do we have one or more texture sample instructions: */
	bool has_samp;

	/* const reg # of first immediate, ie. 1 == c1
	 * (not regid, because TGSI thinks in terms of vec4 registers,
	 * not scalar registers)
	 */
	unsigned first_immediate;
	unsigned immediates_count;
	struct {
		uint32_t val[4];
	} immediates[64];

	/* shader varients form a linked list: */
	struct fd3_shader_variant *next;

	/* replicated here to avoid passing extra ptrs everywhere: */
	enum shader_t type;
	struct fd3_shader_stateobj *so;
};

struct fd3_shader_stateobj {
	enum shader_t type;

	struct pipe_context *pctx;
	const struct tgsi_token *tokens;

	struct fd3_shader_variant *variants;

	/* so far, only used for blit_prog shader.. values for
	 * VPC_VARYING_INTERP[i].MODE and VPC_VARYING_PS_REPL[i].MODE
	 *
	 * Possibly should be in fd3_program_variant?
	 */
	uint32_t vinterp[4], vpsrepl[4];
};

struct fd3_shader_variant * fd3_shader_variant(struct fd3_shader_stateobj *so,
		struct fd3_shader_key key);

void fd3_program_emit(struct fd_ringbuffer *ring,
		struct fd_program_stateobj *prog, struct fd3_shader_key key);

void fd3_prog_init(struct pipe_context *pctx);

#endif /* FD3_PROGRAM_H_ */
