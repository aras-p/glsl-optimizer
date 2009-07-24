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

#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"

#include "radeon_nqssadce.h"
#include "radeon_program_alu.h"
#include "r300_fragprog.h"
#include "r300_fragprog_swizzle.h"
#include "r500_fragprog.h"


static void nqssadce_init(struct nqssadce_state* s)
{
	s->Outputs[FRAG_RESULT_COLOR].Sourced = WRITEMASK_XYZW;
	s->Outputs[FRAG_RESULT_DEPTH].Sourced = WRITEMASK_W;
}

/**
 * Transform the program to support fragment.position.
 *
 * Introduce a small fragment at the start of the program that will be
 * the only code that directly reads the FRAG_ATTRIB_WPOS input.
 * All other code pieces that reference that input will be rewritten
 * to read from a newly allocated temporary.
 *
 */
static void insert_WPOS_trailer(struct r300_fragment_program_compiler *compiler)
{
	int i;

	if (!(compiler->Base.Program.InputsRead & FRAG_BIT_WPOS)) {
		compiler->code->wpos_attr = FRAG_ATTRIB_MAX;
		return;
	}

	for (i = FRAG_ATTRIB_TEX0; i <= FRAG_ATTRIB_TEX7; ++i)
	{
		if (!(compiler->Base.Program.InputsRead & (1 << i))) {
			compiler->code->wpos_attr = i;
			break;
		}
	}

	rc_transform_fragment_wpos(&compiler->Base, FRAG_ATTRIB_WPOS, compiler->code->wpos_attr);
}

/**
 * Rewrite fragment.fogcoord to use a texture coordinate slot.
 * Note that fogcoord is forced into an X001 pattern, and this enforcement
 * is done here.
 *
 * See also the counterpart rewriting for vertex programs.
 */
static void rewriteFog(struct r300_fragment_program_compiler *compiler)
{
	struct rX00_fragment_program_code *code = compiler->code;
	struct prog_src_register src;
	int i;

	if (!(compiler->Base.Program.InputsRead & FRAG_BIT_FOGC)) {
		code->fog_attr = FRAG_ATTRIB_MAX;
		return;
	}

	for (i = FRAG_ATTRIB_TEX0; i <= FRAG_ATTRIB_TEX7; ++i)
	{
		if (!(compiler->Base.Program.InputsRead & (1 << i))) {
			code->fog_attr = i;
			break;
		}
	}

	reset_srcreg(&src);
	src.File = PROGRAM_INPUT;
	src.Index = code->fog_attr;
	src.Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ONE);
	rc_move_input(&compiler->Base, FRAG_ATTRIB_FOGC, src);
}


static void rewrite_depth_out(struct r300_fragment_program_compiler * c)
{
	struct rc_instruction *rci;

	for (rci = c->Base.Program.Instructions.Next; rci != &c->Base.Program.Instructions; rci = rci->Next) {
		struct prog_instruction * inst = &rci->I;

		if (inst->DstReg.File != PROGRAM_OUTPUT || inst->DstReg.Index != FRAG_RESULT_DEPTH)
			continue;

		if (inst->DstReg.WriteMask & WRITEMASK_Z) {
			inst->DstReg.WriteMask = WRITEMASK_W;
		} else {
			inst->DstReg.WriteMask = 0;
			continue;
		}

		switch (inst->Opcode) {
			case OPCODE_FRC:
			case OPCODE_MOV:
				inst->SrcReg[0] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[0]);
				break;
			case OPCODE_ADD:
			case OPCODE_MAX:
			case OPCODE_MIN:
			case OPCODE_MUL:
				inst->SrcReg[0] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[0]);
				inst->SrcReg[1] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[1]);
				break;
			case OPCODE_CMP:
			case OPCODE_MAD:
				inst->SrcReg[0] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[0]);
				inst->SrcReg[1] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[1]);
				inst->SrcReg[2] = lmul_swizzle(SWIZZLE_ZZZZ, inst->SrcReg[2]);
				break;
			default:
				// Scalar instructions needn't be reswizzled
				break;
		}
	}
}

void r3xx_compile_fragment_program(struct r300_fragment_program_compiler* c)
{
	if (c->Base.Debug) {
		fflush(stdout);
		_mesa_printf("Fragment Program: Initial program:\n");
		_mesa_print_program(c->program);
		fflush(stdout);
	}

	rc_mesa_to_rc_program(&c->Base, c->program);

	insert_WPOS_trailer(c);

	rewriteFog(c);

	rewrite_depth_out(c);

	if (c->is_r500) {
		struct radeon_program_transformation transformations[] = {
			{ &r500_transform_TEX, c },
			{ &radeonTransformALU, 0 },
			{ &radeonTransformDeriv, 0 },
			{ &radeonTransformTrigScale, 0 }
		};
		radeonLocalTransform(&c->Base, 4, transformations);
	} else {
		struct radeon_program_transformation transformations[] = {
			{ &r300_transform_TEX, c },
			{ &radeonTransformALU, 0 },
			{ &radeonTransformTrigSimple, 0 }
		};
		radeonLocalTransform(&c->Base, 3, transformations);
	}

	if (c->Base.Debug) {
		_mesa_printf("Fragment Program: After native rewrite:\n");
		rc_print_program(&c->Base.Program);
		fflush(stdout);
	}

	if (c->is_r500) {
		struct radeon_nqssadce_descr nqssadce = {
			.Init = &nqssadce_init,
			.IsNativeSwizzle = &r500FPIsNativeSwizzle,
			.BuildSwizzle = &r500FPBuildSwizzle
		};
		radeonNqssaDce(&c->Base, &nqssadce, 0);
	} else {
		struct radeon_nqssadce_descr nqssadce = {
			.Init = &nqssadce_init,
			.IsNativeSwizzle = &r300FPIsNativeSwizzle,
			.BuildSwizzle = &r300FPBuildSwizzle
		};
		radeonNqssaDce(&c->Base, &nqssadce, 0);
	}

	if (c->Base.Debug) {
		_mesa_printf("Compiler: after NqSSA-DCE:\n");
		rc_print_program(&c->Base.Program);
		fflush(stdout);
	}

	if (c->is_r500) {
		r500BuildFragmentProgramHwCode(c);
	} else {
		r300BuildFragmentProgramHwCode(c);
	}

	rc_constants_copy(&c->code->constants, &c->Base.Program.Constants);

	if (c->Base.Debug) {
		if (c->is_r500) {
			r500FragmentProgramDump(c->code);
		} else {
			r300FragmentProgramDump(c->code);
		}
	}
}
