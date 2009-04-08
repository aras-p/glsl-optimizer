/*
 * Copyright (C) 2008 Nicolai Haehnle.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __RADEON_PROGRAM_NQSSADCE_H_
#define __RADEON_PROGRAM_NQSSADCE_H_

#include "radeon_program.h"


struct register_state {
	/**
	 * Bitmask indicating which components of the register are sourced
	 * by later instructions.
	 */
	GLuint Sourced : 4;
};

/**
 * Maintain state such as which registers are used, which registers are
 * read from, etc.
 */
struct nqssadce_state {
	GLcontext *Ctx;
	struct gl_program *Program;
	struct radeon_nqssadce_descr *Descr;

	/**
	 * All instructions after this instruction pointer have been dealt with.
	 */
	int IP;

	/**
	 * Which registers are read by subsequent instructions?
	 */
	struct register_state Temps[MAX_PROGRAM_TEMPS];
	struct register_state Outputs[VERT_RESULT_MAX];
};


/**
 * This structure contains a description of the hardware in-so-far as
 * it is required for the NqSSA-DCE pass.
 */
struct radeon_nqssadce_descr {
	/**
	 * Fill in which outputs
	 */
	void (*Init)(struct nqssadce_state *);

	/**
	 * Check whether the given swizzle, absolute and negate combination
	 * can be implemented natively by the hardware for this opcode.
	 */
	GLboolean (*IsNativeSwizzle)(GLuint opcode, struct prog_src_register reg);

	/**
	 * Emit (at the current IP) the instruction MOV dst, src;
	 * The transformation will work recursively on the emitted instruction(s).
	 */
	void (*BuildSwizzle)(struct nqssadce_state*, struct prog_dst_register dst, struct prog_src_register src);

	/**
	 * Rewrite instructions that write to DEPR.z to write to DEPR.w
	 * instead (rewriting is done *before* the WriteMask test).
	 */
	GLboolean RewriteDepthOut;
	void *Data;
};

void radeonNqssaDce(GLcontext *ctx, struct gl_program *p, struct radeon_nqssadce_descr* descr);

#endif /* __RADEON_PROGRAM_NQSSADCE_H_ */
