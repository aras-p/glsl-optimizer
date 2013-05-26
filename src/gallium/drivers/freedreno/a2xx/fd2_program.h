/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012-2013 Rob Clark <robclark@freedesktop.org>
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

#ifndef FD2_PROGRAM_H_
#define FD2_PROGRAM_H_

#include "pipe/p_context.h"

#include "freedreno_context.h"

#include "ir-a2xx.h"
#include "disasm.h"

struct fd2_shader_stateobj {
	enum shader_t type;

	uint32_t *bin;

	struct tgsi_token *tokens;

	/* note that we defer compiling shader until we know both vs and ps..
	 * and if one changes, we potentially need to recompile in order to
	 * get varying linkages correct:
	 */
	struct ir2_shader_info info;
	struct ir2_shader *ir;

	/* for vertex shaders, the fetch instructions which need to be
	 * patched up before assembly:
	 */
	unsigned num_vfetch_instrs;
	struct ir2_instruction *vfetch_instrs[64];

	/* for all shaders, any tex fetch instructions which need to be
	 * patched before assembly:
	 */
	unsigned num_tfetch_instrs;
	struct {
		unsigned samp_id;
		struct ir2_instruction *instr;
	} tfetch_instrs[64];

	unsigned first_immediate;     /* const reg # of first immediate */
	unsigned num_immediates;
	struct {
		uint32_t val[4];
	} immediates[64];
};

void fd2_program_emit(struct fd_ringbuffer *ring,
		struct fd_program_stateobj *prog);
void fd2_program_validate(struct fd_context *ctx);

void fd2_prog_init(struct pipe_context *pctx);
void fd2_prog_fini(struct pipe_context *pctx);

#endif /* FD2_PROGRAM_H_ */
