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
#include <stdio.h>
#include <stdlib.h>

#include "radeon_program.h"


void rc_init(struct radeon_compiler * c)
{
	memset(c, 0, sizeof(*c));

	memory_pool_init(&c->Pool);
	c->Program.Instructions.Prev = &c->Program.Instructions;
	c->Program.Instructions.Next = &c->Program.Instructions;
	c->Program.Instructions.U.I.Opcode = RC_OPCODE_ILLEGAL_OPCODE;
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

	c->Error = 1;

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

int rc_if_fail_helper(struct radeon_compiler * c, const char * file, int line, const char * assertion)
{
	rc_error(c, "ICE at %s:%i: assertion failed: %s\n", file, line, assertion);
	return 1;
}

/**
 * Recompute c->Program.InputsRead and c->Program.OutputsWritten
 * based on which inputs and outputs are actually referenced
 * in program instructions.
 */
void rc_calculate_inputs_outputs(struct radeon_compiler * c)
{
	struct rc_instruction *inst;

	c->Program.InputsRead = 0;
	c->Program.OutputsWritten = 0;

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next)
	{
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		int i;

		for (i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_INPUT)
				c->Program.InputsRead |= 1 << inst->U.I.SrcReg[i].Index;
		}

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_OUTPUT)
				c->Program.OutputsWritten |= 1 << inst->U.I.DstReg.Index;
		}
	}
}

/**
 * Rewrite the program such that everything that source the given input
 * register will source new_input instead.
 */
void rc_move_input(struct radeon_compiler * c, unsigned input, struct rc_src_register new_input)
{
	struct rc_instruction * inst;

	c->Program.InputsRead &= ~(1 << input);

	for(inst = c->Program.Instructions.Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		unsigned i;

		for(i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_INPUT && inst->U.I.SrcReg[i].Index == input) {
				inst->U.I.SrcReg[i].File = new_input.File;
				inst->U.I.SrcReg[i].Index = new_input.Index;
				inst->U.I.SrcReg[i].Swizzle = combine_swizzles(new_input.Swizzle, inst->U.I.SrcReg[i].Swizzle);
				if (!inst->U.I.SrcReg[i].Abs) {
					inst->U.I.SrcReg[i].Negate ^= new_input.Negate;
					inst->U.I.SrcReg[i].Abs = new_input.Abs;
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
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_OUTPUT && inst->U.I.DstReg.Index == output) {
				inst->U.I.DstReg.Index = new_output;
				inst->U.I.DstReg.WriteMask &= writemask;

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
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_OUTPUT && inst->U.I.DstReg.Index == output) {
				inst->U.I.DstReg.File = RC_FILE_TEMPORARY;
				inst->U.I.DstReg.Index = tempreg;
			}
		}
	}

	inst = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
	inst->U.I.Opcode = RC_OPCODE_MOV;
	inst->U.I.DstReg.File = RC_FILE_OUTPUT;
	inst->U.I.DstReg.Index = output;

	inst->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
	inst->U.I.SrcReg[0].Index = tempreg;
	inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

	inst = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
	inst->U.I.Opcode = RC_OPCODE_MOV;
	inst->U.I.DstReg.File = RC_FILE_OUTPUT;
	inst->U.I.DstReg.Index = dup_output;

	inst->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
	inst->U.I.SrcReg[0].Index = tempreg;
	inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

	c->Program.OutputsWritten |= 1 << dup_output;
}


/**
 * Introduce standard code fragment to deal with fragment.position.
 */
void rc_transform_fragment_wpos(struct radeon_compiler * c, unsigned wpos, unsigned new_input,
                                int full_vtransform)
{
	unsigned tempregi = rc_find_free_temporary(c);
	struct rc_instruction * inst_rcp;
	struct rc_instruction * inst_mul;
	struct rc_instruction * inst_mad;
	struct rc_instruction * inst;

	c->Program.InputsRead &= ~(1 << wpos);
	c->Program.InputsRead |= 1 << new_input;

	/* perspective divide */
	inst_rcp = rc_insert_new_instruction(c, &c->Program.Instructions);
	inst_rcp->U.I.Opcode = RC_OPCODE_RCP;

	inst_rcp->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_rcp->U.I.DstReg.Index = tempregi;
	inst_rcp->U.I.DstReg.WriteMask = RC_MASK_W;

	inst_rcp->U.I.SrcReg[0].File = RC_FILE_INPUT;
	inst_rcp->U.I.SrcReg[0].Index = new_input;
	inst_rcp->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_WWWW;

	inst_mul = rc_insert_new_instruction(c, inst_rcp);
	inst_mul->U.I.Opcode = RC_OPCODE_MUL;

	inst_mul->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_mul->U.I.DstReg.Index = tempregi;
	inst_mul->U.I.DstReg.WriteMask = RC_MASK_XYZ;

	inst_mul->U.I.SrcReg[0].File = RC_FILE_INPUT;
	inst_mul->U.I.SrcReg[0].Index = new_input;

	inst_mul->U.I.SrcReg[1].File = RC_FILE_TEMPORARY;
	inst_mul->U.I.SrcReg[1].Index = tempregi;
	inst_mul->U.I.SrcReg[1].Swizzle = RC_SWIZZLE_WWWW;

	/* viewport transformation */
	inst_mad = rc_insert_new_instruction(c, inst_mul);
	inst_mad->U.I.Opcode = RC_OPCODE_MAD;

	inst_mad->U.I.DstReg.File = RC_FILE_TEMPORARY;
	inst_mad->U.I.DstReg.Index = tempregi;
	inst_mad->U.I.DstReg.WriteMask = RC_MASK_XYZ;

	inst_mad->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
	inst_mad->U.I.SrcReg[0].Index = tempregi;
	inst_mad->U.I.SrcReg[0].Swizzle = RC_MAKE_SWIZZLE(RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_ZERO);

	inst_mad->U.I.SrcReg[1].File = RC_FILE_CONSTANT;
	inst_mad->U.I.SrcReg[1].Swizzle = RC_MAKE_SWIZZLE(RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_ZERO);

	inst_mad->U.I.SrcReg[2].File = RC_FILE_CONSTANT;
	inst_mad->U.I.SrcReg[2].Swizzle = RC_MAKE_SWIZZLE(RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_ZERO);

	if (full_vtransform) {
		inst_mad->U.I.SrcReg[1].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_VIEWPORT_SCALE, 0);
		inst_mad->U.I.SrcReg[2].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_VIEWPORT_OFFSET, 0);
	} else {
		inst_mad->U.I.SrcReg[1].Index =
		inst_mad->U.I.SrcReg[2].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_WINDOW_DIMENSION, 0);
	}

	for (inst = inst_mad->Next; inst != &c->Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		unsigned i;

		for(i = 0; i < opcode->NumSrcRegs; i++) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_INPUT &&
			    inst->U.I.SrcReg[i].Index == wpos) {
				inst->U.I.SrcReg[i].File = RC_FILE_TEMPORARY;
				inst->U.I.SrcReg[i].Index = tempregi;
			}
		}
	}
}

