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

#include "radeon_compiler.h"
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
	struct gl_program *program,
	int num_transformations,
	struct radeon_program_transformation* transformations)
{
	struct radeon_transform_context ctx;
	int ip;

	ctx.Program = program;
	ctx.OldInstructions = program->Instructions;
	ctx.OldNumInstructions = program->NumInstructions;

	program->Instructions = 0;
	program->NumInstructions = 0;

	for(ip = 0; ip < ctx.OldNumInstructions; ++ip) {
		struct prog_instruction *instr = ctx.OldInstructions + ip;
		int i;

		for(i = 0; i < num_transformations; ++i) {
			struct radeon_program_transformation* t = transformations + i;

			if (t->function(&ctx, instr, t->userData))
				break;
		}

		if (i >= num_transformations) {
			struct prog_instruction* dest = radeonAppendInstructions(program, 1);
			_mesa_copy_instructions(dest, instr, 1);
		}
	}

	_mesa_free_instructions(ctx.OldInstructions, ctx.OldNumInstructions);
}


static void scan_instructions(GLboolean* used, const struct prog_instruction* insts, GLuint count)
{
	GLuint i;
	for (i = 0; i < count; i++) {
		const struct prog_instruction *inst = insts + i;
		const GLuint n = _mesa_num_inst_src_regs(inst->Opcode);
		GLuint k;

		for (k = 0; k < n; k++) {
			if (inst->SrcReg[k].File == PROGRAM_TEMPORARY)
				used[inst->SrcReg[k].Index] = GL_TRUE;
		}
	}
}

GLint radeonFindFreeTemporary(struct radeon_transform_context *t)
{
	GLboolean used[MAX_PROGRAM_TEMPS];
	GLuint i;

	_mesa_memset(used, 0, sizeof(used));
	scan_instructions(used, t->Program->Instructions, t->Program->NumInstructions);
	scan_instructions(used, t->OldInstructions, t->OldNumInstructions);

	for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
		if (!used[i])
			return i;
	}

	return -1;
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


GLint rc_find_free_temporary(struct radeon_compiler * c)
{
	GLboolean used[MAX_PROGRAM_TEMPS];
	GLuint i;

	memset(used, 0, sizeof(used));

	for (struct rc_instruction * rcinst = c->Program.Instructions.Next; rcinst != &c->Program.Instructions; rcinst = rcinst->Next) {
		const struct prog_instruction *inst = &rcinst->I;
		const GLuint n = _mesa_num_inst_src_regs(inst->Opcode);
		GLuint k;

		for (k = 0; k < n; k++) {
			if (inst->SrcReg[k].File == PROGRAM_TEMPORARY)
				used[inst->SrcReg[k].Index] = GL_TRUE;
		}
	}

	for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
		if (!used[i])
			return i;
	}

	return -1;
}


struct rc_instruction *rc_alloc_instruction(struct radeon_compiler * c)
{
	struct rc_instruction * inst = memory_pool_malloc(&c->Pool, sizeof(struct rc_instruction));

	inst->Prev = 0;
	inst->Next = 0;

	_mesa_init_instructions(&inst->I, 1);

	return inst;
}


struct rc_instruction *rc_insert_new_instruction(struct radeon_compiler * c, struct rc_instruction * after)
{
	struct rc_instruction * inst = rc_alloc_instruction(c);

	inst->Prev = after;
	inst->Next = after->Next;

	inst->Prev->Next = inst;
	inst->Next->Prev = inst;

	return inst;
}

void rc_remove_instruction(struct rc_instruction * inst)
{
	inst->Prev->Next = inst->Next;
	inst->Next->Prev = inst->Prev;
}


void rc_mesa_to_rc_program(struct radeon_compiler * c, struct gl_program * program)
{
	struct prog_instruction *source;

	for(source = program->Instructions; source->Opcode != OPCODE_END; ++source) {
		struct rc_instruction * dest = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
		dest->I = *source;
	}

	c->Program.ShadowSamplers = program->ShadowSamplers;
	c->Program.InputsRead = program->InputsRead;
	c->Program.OutputsWritten = program->OutputsWritten;
}


/**
 * Print program to stderr, default options.
 */
void rc_print_program(const struct rc_program *prog)
{
	GLuint indent = 0;
	GLuint linenum = 1;
	struct rc_instruction *inst;

	fprintf(stderr, "# Radeon Compiler Program\n");

	for(inst = prog->Instructions.Next; inst != &prog->Instructions; inst = inst->Next) {
		fprintf(stderr, "%3d: ", linenum);

		/* Massive hack: We rely on the fact that the printers do not actually
		 * use the gl_program argument (last argument) in debug mode */
		indent = _mesa_fprint_instruction_opt(
				stderr, &inst->I,
				indent, PROG_PRINT_DEBUG, 0);

		linenum++;
	}
}
