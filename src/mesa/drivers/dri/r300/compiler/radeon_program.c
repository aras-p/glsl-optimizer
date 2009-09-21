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
#include "shader/prog_parameter.h"
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
	struct radeon_compiler * c,
	int num_transformations,
	struct radeon_program_transformation* transformations)
{
	struct rc_instruction * inst = c->Program.Instructions.Next;

	while(inst != &c->Program.Instructions) {
		struct rc_instruction * current = inst;
		int i;

		inst = inst->Next;

		for(i = 0; i < num_transformations; ++i) {
			struct radeon_program_transformation* t = transformations + i;

			if (t->function(c, current, t->userData))
				break;
		}
	}
}


GLint rc_find_free_temporary(struct radeon_compiler * c)
{
	GLboolean used[MAX_PROGRAM_TEMPS];
	GLuint i;

	memset(used, 0, sizeof(used));

	for (struct rc_instruction * rcinst = c->Program.Instructions.Next; rcinst != &c->Program.Instructions; rcinst = rcinst->Next) {
		const struct prog_instruction *inst = &rcinst->I;
		const GLuint nsrc = _mesa_num_inst_src_regs(inst->Opcode);
		const GLuint ndst = _mesa_num_inst_dst_regs(inst->Opcode);
		GLuint k;

		for (k = 0; k < nsrc; k++) {
			if (inst->SrcReg[k].File == PROGRAM_TEMPORARY)
				used[inst->SrcReg[k].Index] = GL_TRUE;
		}

		if (ndst) {
			if (inst->DstReg.File == PROGRAM_TEMPORARY)
				used[inst->DstReg.Index] = GL_TRUE;
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
	unsigned int i;

	for(source = program->Instructions; source->Opcode != OPCODE_END; ++source) {
		struct rc_instruction * dest = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
		dest->I = *source;
	}

	c->Program.ShadowSamplers = program->ShadowSamplers;
	c->Program.InputsRead = program->InputsRead;
	c->Program.OutputsWritten = program->OutputsWritten;

	int isNVProgram = 0;

	if (program->Target == GL_VERTEX_PROGRAM_ARB) {
		struct gl_vertex_program * vp = (struct gl_vertex_program *) program;
		isNVProgram = vp->IsNVProgram;
	}

	if (isNVProgram) {
		/* NV_vertex_program has a fixed-sized constant environment.
		 * This could be handled more efficiently for programs that
		 * do not use relative addressing.
		 */
		for(i = 0; i < 96; ++i) {
			struct rc_constant constant;

			constant.Type = RC_CONSTANT_EXTERNAL;
			constant.Size = 4;
			constant.u.External = i;

			rc_constants_add(&c->Program.Constants, &constant);
		}
	} else {
		for(i = 0; i < program->Parameters->NumParameters; ++i) {
			struct rc_constant constant;

			constant.Type = RC_CONSTANT_EXTERNAL;
			constant.Size = 4;
			constant.u.External = i;

			rc_constants_add(&c->Program.Constants, &constant);
		}
	}
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
