/*
 * Copyright (C) 2009 Nicolai Haehnle.
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

#include "radeon_program_pair.h"

#include "radeon_compiler.h"


/**
 * Finally rewrite ADD, MOV, MUL as the appropriate native instruction
 * and reverse the order of arguments for CMP.
 */
static void final_rewrite(struct rc_sub_instruction *inst)
{
	struct rc_src_register tmp;

	switch(inst->Opcode) {
	case RC_OPCODE_ADD:
		inst->SrcReg[2] = inst->SrcReg[1];
		inst->SrcReg[1].File = RC_FILE_NONE;
		inst->SrcReg[1].Swizzle = RC_SWIZZLE_1111;
		inst->SrcReg[1].Negate = RC_MASK_NONE;
		inst->Opcode = RC_OPCODE_MAD;
		break;
	case RC_OPCODE_CMP:
		tmp = inst->SrcReg[2];
		inst->SrcReg[2] = inst->SrcReg[0];
		inst->SrcReg[0] = tmp;
		break;
	case RC_OPCODE_MOV:
		/* AMD say we should use CMP.
		 * However, when we transform
		 *  KIL -r0;
		 * into
		 *  CMP tmp, -r0, -r0, 0;
		 *  KIL tmp;
		 * we get incorrect behaviour on R500 when r0 == 0.0.
		 * It appears that the R500 KIL hardware treats -0.0 as less
		 * than zero.
		 */
		inst->SrcReg[1].File = RC_FILE_NONE;
		inst->SrcReg[1].Swizzle = RC_SWIZZLE_1111;
		inst->SrcReg[2].File = RC_FILE_NONE;
		inst->SrcReg[2].Swizzle = RC_SWIZZLE_0000;
		inst->Opcode = RC_OPCODE_MAD;
		break;
	case RC_OPCODE_MUL:
		inst->SrcReg[2].File = RC_FILE_NONE;
		inst->SrcReg[2].Swizzle = RC_SWIZZLE_0000;
		inst->Opcode = RC_OPCODE_MAD;
		break;
	default:
		/* nothing to do */
		break;
	}
}


/**
 * Classify an instruction according to which ALUs etc. it needs
 */
static void classify_instruction(struct rc_sub_instruction * inst,
	int * needrgb, int * needalpha, int * istranscendent)
{
	*needrgb = (inst->DstReg.WriteMask & RC_MASK_XYZ) ? 1 : 0;
	*needalpha = (inst->DstReg.WriteMask & RC_MASK_W) ? 1 : 0;
	*istranscendent = 0;

	if (inst->WriteALUResult == RC_ALURESULT_X)
		*needrgb = 1;
	else if (inst->WriteALUResult == RC_ALURESULT_W)
		*needalpha = 1;

	switch(inst->Opcode) {
	case RC_OPCODE_ADD:
	case RC_OPCODE_CMP:
	case RC_OPCODE_DDX:
	case RC_OPCODE_DDY:
	case RC_OPCODE_FRC:
	case RC_OPCODE_MAD:
	case RC_OPCODE_MAX:
	case RC_OPCODE_MIN:
	case RC_OPCODE_MOV:
	case RC_OPCODE_MUL:
		break;
	case RC_OPCODE_COS:
	case RC_OPCODE_EX2:
	case RC_OPCODE_LG2:
	case RC_OPCODE_RCP:
	case RC_OPCODE_RSQ:
	case RC_OPCODE_SIN:
		*istranscendent = 1;
		*needalpha = 1;
		break;
	case RC_OPCODE_DP4:
		*needalpha = 1;
		/* fall through */
	case RC_OPCODE_DP3:
		*needrgb = 1;
		break;
	default:
		break;
	}
}


/**
 * Fill the given ALU instruction's opcodes and source operands into the given pair,
 * if possible.
 */
static void set_pair_instruction(struct r300_fragment_program_compiler *c,
	struct rc_pair_instruction * pair,
	struct rc_sub_instruction * inst)
{
	memset(pair, 0, sizeof(struct rc_pair_instruction));

	int needrgb, needalpha, istranscendent;
	classify_instruction(inst, &needrgb, &needalpha, &istranscendent);

	if (needrgb) {
		if (istranscendent)
			pair->RGB.Opcode = RC_OPCODE_REPL_ALPHA;
		else
			pair->RGB.Opcode = inst->Opcode;
		if (inst->SaturateMode == RC_SATURATE_ZERO_ONE)
			pair->RGB.Saturate = 1;
	}
	if (needalpha) {
		pair->Alpha.Opcode = inst->Opcode;
		if (inst->SaturateMode == RC_SATURATE_ZERO_ONE)
			pair->Alpha.Saturate = 1;
	}

	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->Opcode);
	int nargs = opcode->NumSrcRegs;
	int i;

	/* Special case for DDX/DDY (MDH/MDV). */
	if (inst->Opcode == RC_OPCODE_DDX || inst->Opcode == RC_OPCODE_DDY) {
		nargs++;
	}

	for(i = 0; i < opcode->NumSrcRegs; ++i) {
		int source;
		if (needrgb && !istranscendent) {
			unsigned int srcrgb = 0;
			unsigned int srcalpha = 0;
			int j;
			for(j = 0; j < 3; ++j) {
				unsigned int swz = GET_SWZ(inst->SrcReg[i].Swizzle, j);
				if (swz < 3)
					srcrgb = 1;
				else if (swz < 4)
					srcalpha = 1;
			}
			source = rc_pair_alloc_source(pair, srcrgb, srcalpha,
							inst->SrcReg[i].File, inst->SrcReg[i].Index);
			pair->RGB.Arg[i].Source = source;
			pair->RGB.Arg[i].Swizzle = inst->SrcReg[i].Swizzle & 0x1ff;
			pair->RGB.Arg[i].Abs = inst->SrcReg[i].Abs;
			pair->RGB.Arg[i].Negate = !!(inst->SrcReg[i].Negate & (RC_MASK_X | RC_MASK_Y | RC_MASK_Z));
		}
		if (needalpha) {
			unsigned int srcrgb = 0;
			unsigned int srcalpha = 0;
			unsigned int swz = GET_SWZ(inst->SrcReg[i].Swizzle, istranscendent ? 0 : 3);
			if (swz < 3)
				srcrgb = 1;
			else if (swz < 4)
				srcalpha = 1;
			source = rc_pair_alloc_source(pair, srcrgb, srcalpha,
							inst->SrcReg[i].File, inst->SrcReg[i].Index);
			pair->Alpha.Arg[i].Source = source;
			pair->Alpha.Arg[i].Swizzle = swz;
			pair->Alpha.Arg[i].Abs = inst->SrcReg[i].Abs;
			pair->Alpha.Arg[i].Negate = !!(inst->SrcReg[i].Negate & RC_MASK_W);
		}
	}

	/* Destination handling */
	if (inst->DstReg.File == RC_FILE_OUTPUT) {
        if (inst->DstReg.Index == c->OutputDepth) {
            pair->Alpha.DepthWriteMask |= GET_BIT(inst->DstReg.WriteMask, 3);
        } else {
            for (i = 0; i < 4; i++) {
                if (inst->DstReg.Index == c->OutputColor[i]) {
                    pair->RGB.Target = i;
                    pair->Alpha.Target = i;
                    pair->RGB.OutputWriteMask |=
                        inst->DstReg.WriteMask & RC_MASK_XYZ;
                    pair->Alpha.OutputWriteMask |=
                        GET_BIT(inst->DstReg.WriteMask, 3);
                    break;
                }
            }
        }
	} else {
		if (needrgb) {
			pair->RGB.DestIndex = inst->DstReg.Index;
			pair->RGB.WriteMask |= inst->DstReg.WriteMask & RC_MASK_XYZ;
		}
		if (needalpha) {
			pair->Alpha.DestIndex = inst->DstReg.Index;
			pair->Alpha.WriteMask |= GET_BIT(inst->DstReg.WriteMask, 3);
		}
	}

	if (inst->WriteALUResult) {
		pair->WriteALUResult = inst->WriteALUResult;
		pair->ALUResultCompare = inst->ALUResultCompare;
	}
}


/**
 * Translate all ALU instructions into corresponding pair instructions,
 * performing no other changes.
 */
void rc_pair_translate(struct r300_fragment_program_compiler *c)
{
	for(struct rc_instruction * inst = c->Base.Program.Instructions.Next;
	    inst != &c->Base.Program.Instructions;
	    inst = inst->Next) {
		if (inst->Type != RC_INSTRUCTION_NORMAL)
			continue;

		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		if (opcode->HasTexture || opcode->IsFlowControl || opcode->Opcode == RC_OPCODE_KIL)
			continue;

		struct rc_sub_instruction copy = inst->U.I;

		final_rewrite(&copy);
		inst->Type = RC_INSTRUCTION_PAIR;
		set_pair_instruction(c, &inst->U.P, &copy);
	}
}
