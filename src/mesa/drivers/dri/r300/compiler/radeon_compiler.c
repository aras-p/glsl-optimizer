/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "radeon_compiler.h"

#include <stdarg.h>

#include "radeon_program.h"


void rc_init(struct radeon_compiler * c)
{
	memset(c, 0, sizeof(*c));

	memory_pool_init(&c->Pool);
	c->Program.Instructions.Prev = &c->Program.Instructions;
	c->Program.Instructions.Next = &c->Program.Instructions;
	c->Program.Instructions.I.Opcode = OPCODE_END;
}

void rc_destroy(struct radeon_compiler * c)
{
	rc_constants_destroy(&c->Program.Constants);
	memory_pool_destroy(&c->Pool);
	free(c->ErrorMsg);
}

void rc_debug(struct radeon_compiler * c, const char * fmt, ...)
{
	va_list ap;

	if (!c->Debug)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void rc_error(struct radeon_compiler * c, const char * fmt, ...)
{
	va_list ap;

	c->Error = GL_TRUE;

	if (!c->ErrorMsg) {
		/* Only remember the first error */
		char buf[1024];
		int written;

		va_start(ap, fmt);
		written = vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);

		if (written < sizeof(buf)) {
			c->ErrorMsg = strdup(buf);
		} else {
			c->ErrorMsg = malloc(written + 1);

			va_start(ap, fmt);
			vsnprintf(c->ErrorMsg, written + 1, fmt, ap);
			va_end(ap);
		}
	}

	if (c->Debug) {
		fprintf(stderr, "r300compiler error: ");

		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}

/**
 * Rewrite the program such that everything that source the given input
 * register will source new_input instead.
 */
void rc_move_input(struct radeon_compiler * c, unsigned input, struct prog_src_register new_input)
{
	struct rc_instruction * inst;

	c->Program.InputsRead &= ~(1 << input);

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const unsigned numsrcs = _mesa_num_inst_src_regs(inst->I.Opcode);
		unsigned i;

		for(i = 0; i < numsrcs; ++i) {
			if (inst->I.SrcReg[i].File == PROGRAM_INPUT && inst->I.SrcReg[i].Index == input) {
				inst->I.SrcReg[i].File = new_input.File;
				inst->I.SrcReg[i].Index = new_input.Index;
				inst->I.SrcReg[i].Swizzle = combine_swizzles(new_input.Swizzle, inst->I.SrcReg[i].Swizzle);
				if (!inst->I.SrcReg[i].Abs) {
					inst->I.SrcReg[i].Negate ^= new_input.Negate;
					inst->I.SrcReg[i].Abs = new_input.Abs;
				}

				c->Program.InputsRead |= 1 << new_input.Index;
			}
		}
	}
}


/**
 * Rewrite the program such that everything that writes into the given
 * output register will instead write to new_output. The new_output
 * writemask is honoured.
 */
void rc_move_output(struct radeon_compiler * c, unsigned output, unsigned new_output, unsigned writemask)
{
	struct rc_instruction * inst;

	c->Program.OutputsWritten &= ~(1 << output);

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const unsigned numdsts = _mesa_num_inst_dst_regs(inst->I.Opcode);

		if (numdsts) {
			if (inst->I.DstReg.File == PROGRAM_OUTPUT && inst->I.DstReg.Index == output) {
				inst->I.DstReg.Index = new_output;
				inst->I.DstReg.WriteMask &= writemask;

				c->Program.OutputsWritten |= 1 << new_output;
			}
		}
	}
}


/**
 * Rewrite the program such that a given output is duplicated.
 */
void rc_copy_output(struct radeon_compiler * c, unsigned output, unsigned dup_output)
{
	unsigned tempreg = rc_find_free_temporary(c);
	struct rc_instruction * inst;

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const unsigned numdsts = _mesa_num_inst_dst_regs(inst->I.Opcode);

		if (numdsts) {
			if (inst->I.DstReg.File == PROGRAM_OUTPUT && inst->I.DstReg.Index == output) {
				inst->I.DstReg.File = PROGRAM_TEMPORARY;
				inst->I.DstReg.Index = tempreg;
			}
		}
	}

	inst = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
	inst->I.Opcode = OPCODE_MOV;
	inst->I.DstReg.File = PROGRAM_OUTPUT;
	inst->I.DstReg.Index = output;

	inst->I.SrcReg[0].File = PROGRAM_TEMPORARY;
	inst->I.SrcReg[0].Index = tempreg;
	inst->I.SrcReg[0].Swizzle = SWIZZLE_XYZW;

	inst = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
	inst->I.Opcode = OPCODE_MOV;
	inst->I.DstReg.File = PROGRAM_OUTPUT;
	inst->I.DstReg.Index = dup_output;

	inst->I.SrcReg[0].File = PROGRAM_TEMPORARY;
	inst->I.SrcReg[0].Index = tempreg;
	inst->I.SrcReg[0].Swizzle = SWIZZLE_XYZW;

	c->Program.OutputsWritten |= 1 << dup_output;
}


/**
 * Introduce standard code fragment to deal with fragment.position.
 */
void rc_transform_fragment_wpos(struct radeon_compiler * c, unsigned wpos, unsigned new_input)
{
	unsigned tempregi = rc_find_free_temporary(c);

	c->Program.InputsRead &= ~(1 << wpos);
	c->Program.InputsRead |= 1 << new_input;

	/* perspective divide */
	struct rc_instruction * inst_rcp = rc_insert_new_instruction(c, &c->Program.Instructions);
	inst_rcp->I.Opcode = OPCODE_RCP;

	inst_rcp->I.DstReg.File = PROGRAM_TEMPORARY;
	inst_rcp->I.DstReg.Index = tempregi;
	inst_rcp->I.DstReg.WriteMask = WRITEMASK_W;

	inst_rcp->I.SrcReg[0].File = PROGRAM_INPUT;
	inst_rcp->I.SrcReg[0].Index = new_input;
	inst_rcp->I.SrcReg[0].Swizzle = SWIZZLE_WWWW;

	struct rc_instruction * inst_mul = rc_insert_new_instruction(c, inst_rcp);
	inst_mul->I.Opcode = OPCODE_MUL;

	inst_mul->I.DstReg.File = PROGRAM_TEMPORARY;
	inst_mul->I.DstReg.Index = tempregi;
	inst_mul->I.DstReg.WriteMask = WRITEMASK_XYZ;

	inst_mul->I.SrcReg[0].File = PROGRAM_INPUT;
	inst_mul->I.SrcReg[0].Index = new_input;

	inst_mul->I.SrcReg[1].File = PROGRAM_TEMPORARY;
	inst_mul->I.SrcReg[1].Index = tempregi;
	inst_mul->I.SrcReg[1].Swizzle = SWIZZLE_WWWW;

	/* viewport transformation */
	struct rc_instruction * inst_mad = rc_insert_new_instruction(c, inst_mul);
	inst_mad->I.Opcode = OPCODE_MAD;

	inst_mad->I.DstReg.File = PROGRAM_TEMPORARY;
	inst_mad->I.DstReg.Index = tempregi;
	inst_mad->I.DstReg.WriteMask = WRITEMASK_XYZ;

	inst_mad->I.SrcReg[0].File = PROGRAM_TEMPORARY;
	inst_mad->I.SrcReg[0].Index = tempregi;
	inst_mad->I.SrcReg[0].Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	inst_mad->I.SrcReg[1].File = PROGRAM_STATE_VAR;
	inst_mad->I.SrcReg[1].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_WINDOW_DIMENSION, 0);
	inst_mad->I.SrcReg[1].Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	inst_mad->I.SrcReg[2].File = PROGRAM_STATE_VAR;
	inst_mad->I.SrcReg[2].Index = inst_mad->I.SrcReg[1].Index;
	inst_mad->I.SrcReg[2].Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	struct rc_instruction * inst;
	for (inst = inst_mad->Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const unsigned numsrcs = _mesa_num_inst_src_regs(inst->I.Opcode);
		unsigned i;

		for(i = 0; i < numsrcs; i++) {
			if (inst->I.SrcReg[i].File == PROGRAM_INPUT &&
			    inst->I.SrcReg[i].Index == wpos) {
				inst->I.SrcReg[i].File = PROGRAM_TEMPORARY;
				inst->I.SrcReg[i].Index = tempregi;
			}
		}
	}
}

