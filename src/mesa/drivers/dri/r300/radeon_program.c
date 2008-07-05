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

#include "radeon_program.h"

#include "shader/prog_print.h"


/**
 * Transform the given clause in the following way:
 *  1. Replace it with an empty clause
 *  2. For every instruction in the original clause, try the given
 *     transformations in order.
 *  3. If one of the transformations returns GL_TRUE, assume that it
 *     has emitted the appropriate instruction(s) into the new clause;
 *     otherwise, copy the instruction verbatim.
 *
 * \note The transformation is currently not recursive; in other words,
 * instructions emitted by transformations are not transformed.
 *
 * \note The transform is called 'local' because it can only look at
 * one instruction at a time.
 */
void radeonLocalTransform(
	GLcontext *ctx,
	struct gl_program *program,
	int num_transformations,
	struct radeon_program_transformation* transformations)
{
	struct prog_instruction *source;
	int numinstructions;
	int ip;

	source = program->Instructions;
	numinstructions = program->NumInstructions;

	program->Instructions = 0;
	program->NumInstructions = 0;

	for(ip = 0; ip < numinstructions; ++ip) {
		struct prog_instruction *instr = source + ip;
		int i;

		for(i = 0; i < num_transformations; ++i) {
			struct radeon_program_transformation* t = transformations + i;

			if (t->function(ctx, program, instr, t->userData))
				break;
		}

		if (i >= num_transformations) {
			struct prog_instruction* dest = radeonAppendInstructions(program, 1);
			_mesa_copy_instructions(dest, instr, 1);
		}
	}

	_mesa_free_instructions(source, numinstructions);
}


/**
 * Append the given number of instructions to the program and return a
 * pointer to the first new instruction.
 */
struct prog_instruction *radeonAppendInstructions(struct gl_program *program, int count)
{
	int oldnum = program->NumInstructions;
	_mesa_insert_instructions(program, oldnum, count);
	return program->Instructions + oldnum;
}
