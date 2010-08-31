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

#include <stdio.h>

#include "radeon_dataflow.h"
#include "radeon_emulate_branches.h"
#include "radeon_emulate_loops.h"
#include "radeon_program_alu.h"
#include "radeon_program_tex.h"
#include "radeon_rename_regs.h"
#include "radeon_remove_constants.h"
#include "r300_fragprog.h"
#include "r300_fragprog_swizzle.h"
#include "r500_fragprog.h"


static void dataflow_outputs_mark_use(void * userdata, void * data,
		void (*callback)(void *, unsigned int, unsigned int))
{
	struct r300_fragment_program_compiler * c = userdata;
	callback(data, c->OutputColor[0], RC_MASK_XYZW);
	callback(data, c->OutputColor[1], RC_MASK_XYZW);
	callback(data, c->OutputColor[2], RC_MASK_XYZW);
	callback(data, c->OutputColor[3], RC_MASK_XYZW);
	callback(data, c->OutputDepth, RC_MASK_W);
}

static void rewrite_depth_out(struct r300_fragment_program_compiler * c)
{
	struct rc_instruction *rci;

	for (rci = c->Base.Program.Instructions.Next; rci != &c->Base.Program.Instructions; rci = rci->Next) {
		struct rc_sub_instruction * inst = &rci->U.I;

		if (inst->DstReg.File != RC_FILE_OUTPUT || inst->DstReg.Index != c->OutputDepth)
			continue;

		if (inst->DstReg.WriteMask & RC_MASK_Z) {
			inst->DstReg.WriteMask = RC_MASK_W;
		} else {
			inst->DstReg.WriteMask = 0;
			continue;
		}

		switch (inst->Opcode) {
			case RC_OPCODE_FRC:
			case RC_OPCODE_MOV:
				inst->SrcReg[0] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[0]);
				break;
			case RC_OPCODE_ADD:
			case RC_OPCODE_MAX:
			case RC_OPCODE_MIN:
			case RC_OPCODE_MUL:
				inst->SrcReg[0] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[0]);
				inst->SrcReg[1] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[1]);
				break;
			case RC_OPCODE_CMP:
			case RC_OPCODE_MAD:
				inst->SrcReg[0] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[0]);
				inst->SrcReg[1] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[1]);
				inst->SrcReg[2] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[2]);
				break;
			default:
				// Scalar instructions needn't be reswizzled
				break;
		}
	}
}

static void debug_program_log(struct r300_fragment_program_compiler* c, const char * where)
{
	if (c->Base.Debug) {
		fprintf(stderr, "Fragment Program: %s\n", where);
		rc_print_program(&c->Base.Program);
	}
}

void r3xx_compile_fragment_program(struct r300_fragment_program_compiler* c)
{
	rewrite_depth_out(c);

	/* This transformation needs to be done before any of the IF
	 * instructions are modified. */
	radeonTransformKILP(&c->Base);

	debug_program_log(c, "before compilation");

	if (c->Base.is_r500){
		rc_unroll_loops(&c->Base, R500_PFS_MAX_INST);
		debug_program_log(c, "after unroll loops");
	}
	else{
		rc_transform_loops(&c->Base, -1);
		debug_program_log(c, "after transform loops");

		rc_emulate_branches(&c->Base);
		debug_program_log(c, "after emulate branches");
	}

	if (c->Base.is_r500) {
		struct radeon_program_transformation transformations[] = {
			{ &r500_transform_IF, 0 },
			{ &radeonTransformALU, 0 },
			{ &radeonTransformDeriv, 0 },
			{ &radeonTransformTrigScale, 0 }
		};
		radeonLocalTransform(&c->Base, 4, transformations);

		debug_program_log(c, "after native rewrite part 1");

		c->Base.SwizzleCaps = &r500_swizzle_caps;
	} else {
		struct radeon_program_transformation transformations[] = {
			{ &radeonTransformALU, 0 },
			{ &radeonTransformTrigSimple, 0 }
		};
		radeonLocalTransform(&c->Base, 2, transformations);

		debug_program_log(c, "after native rewrite part 1");

		c->Base.SwizzleCaps = &r300_swizzle_caps;
	}

	/* Run the common transformations too.
	 * Remember, lowering comes last! */
	struct radeon_program_transformation common_transformations[] = {
		{ &radeonTransformTEX, c },
	};
	radeonLocalTransform(&c->Base, 1, common_transformations);

	common_transformations[0].function = &radeonTransformALU;
	radeonLocalTransform(&c->Base, 1, common_transformations);

	if (c->Base.Error)
		return;

	debug_program_log(c, "after native rewrite part 2");

	rc_dataflow_deadcode(&c->Base, &dataflow_outputs_mark_use, c);
	if (c->Base.Error)
		return;

	debug_program_log(c, "after deadcode");

	if (!c->Base.is_r500) {
		rc_emulate_loops(&c->Base, R300_PFS_MAX_ALU_INST);
		debug_program_log(c, "after emulate loops");
	}

	rc_optimize(&c->Base);

	debug_program_log(c, "after dataflow optimize");

	rc_dataflow_swizzles(&c->Base);
	if (c->Base.Error)
		return;

	debug_program_log(c, "after dataflow passes");

	if (c->Base.remove_unused_constants) {
		rc_remove_unused_constants(&c->Base,
					   &c->code->constants_remap_table);

		debug_program_log(c, "after constants cleanup");
	}

	if (!c->Base.is_r500) {
		/* This pass makes it easier for the scheduler to group TEX
		 * instructions and reduces the chances of creating too
		 * many texture indirections.*/
		rc_rename_regs(&c->Base);
		if (c->Base.Error)
			return;
		debug_program_log(c, "after register rename");
	}

	rc_pair_translate(c);
	if (c->Base.Error)
		return;

	debug_program_log(c, "after pair translate");

	rc_pair_schedule(c);
	if (c->Base.Error)
		return;

	debug_program_log(c, "after pair scheduling");

	rc_pair_regalloc(c, c->Base.max_temp_regs);

	if (c->Base.Error)
		return;

	debug_program_log(c, "after register allocation");

	if (c->Base.is_r500) {
		r500BuildFragmentProgramHwCode(c);
	} else {
		r300BuildFragmentProgramHwCode(c);
	}

	rc_constants_copy(&c->code->constants, &c->Base.Program.Constants);

	if (c->Base.Debug) {
		if (c->Base.is_r500) {
			r500FragmentProgramDump(c->code);
		} else {
			r300FragmentProgramDump(c->code);
		}
	}

	/* Check the number of constants. */
	if (!c->Base.Error) {
		unsigned max = c->Base.is_r500 ? R500_PFS_NUM_CONST_REGS : R300_PFS_NUM_CONST_REGS;

		if (c->Base.Program.Constants.Count > max) {
			rc_error(&c->Base, "Too many constants. Max: %i, Got: %i\n",
				 max, c->Base.Program.Constants.Count);
		}
	}
}
