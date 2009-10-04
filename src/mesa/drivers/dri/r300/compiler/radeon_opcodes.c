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

#include "radeon_opcodes.h"

#include "radeon_program_constants.h"

struct rc_opcode_info rc_opcodes[MAX_RC_OPCODE] = {
	{
		.Opcode = RC_OPCODE_NOP,
		.Name = "NOP"
	},
	{
		.Opcode = RC_OPCODE_ILLEGAL_OPCODE,
		.Name = "ILLEGAL OPCODE"
	},
	{
		.Opcode = RC_OPCODE_ABS,
		.Name = "ABS",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_ADD,
		.Name = "ADD",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_ARL,
		.Name = "ARL",
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_CMP,
		.Name = "CMP",
		.NumSrcRegs = 3,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_COS,
		.Name = "COS",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsStandardScalar = 1
	},
	{
		.Opcode = RC_OPCODE_DDX,
		.Name = "DDX",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_DDY,
		.Name = "DDY",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_DP3,
		.Name = "DP3",
		.NumSrcRegs = 2,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_DP4,
		.Name = "DP4",
		.NumSrcRegs = 2,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_DPH,
		.Name = "DPH",
		.NumSrcRegs = 2,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_DST,
		.Name = "DST",
		.NumSrcRegs = 2,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_EX2,
		.Name = "EX2",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsStandardScalar = 1
	},
	{
		.Opcode = RC_OPCODE_EXP,
		.Name = "EXP",
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_FLR,
		.Name = "FLR",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_FRC,
		.Name = "FRC",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_KIL,
		.Name = "KIL",
		.NumSrcRegs = 1
	},
	{
		.Opcode = RC_OPCODE_LG2,
		.Name = "LG2",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsStandardScalar = 1
	},
	{
		.Opcode = RC_OPCODE_LIT,
		.Name = "LIT",
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_LOG,
		.Name = "LOG",
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_LRP,
		.Name = "LRP",
		.NumSrcRegs = 3,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_MAD,
		.Name = "MAD",
		.NumSrcRegs = 3,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_MAX,
		.Name = "MAX",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_MIN,
		.Name = "MIN",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_MOV,
		.Name = "MOV",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_MUL,
		.Name = "MUL",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_POW,
		.Name = "POW",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsStandardScalar = 1
	},
	{
		.Opcode = RC_OPCODE_RCP,
		.Name = "RCP",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsStandardScalar = 1
	},
	{
		.Opcode = RC_OPCODE_RSQ,
		.Name = "RSQ",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsStandardScalar = 1
	},
	{
		.Opcode = RC_OPCODE_SCS,
		.Name = "SCS",
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_SEQ,
		.Name = "SEQ",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SFL,
		.Name = "SFL",
		.NumSrcRegs = 0,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SGE,
		.Name = "SGE",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SGT,
		.Name = "SGT",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SIN,
		.Name = "SIN",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsStandardScalar = 1
	},
	{
		.Opcode = RC_OPCODE_SLE,
		.Name = "SLE",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SLT,
		.Name = "SLT",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SNE,
		.Name = "SNE",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SUB,
		.Name = "SUB",
		.NumSrcRegs = 2,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_SWZ,
		.Name = "SWZ",
		.NumSrcRegs = 1,
		.HasDstReg = 1,
		.IsComponentwise = 1
	},
	{
		.Opcode = RC_OPCODE_XPD,
		.Name = "XPD",
		.NumSrcRegs = 2,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_TEX,
		.Name = "TEX",
		.HasTexture = 1,
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_TXB,
		.Name = "TXB",
		.HasTexture = 1,
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_TXD,
		.Name = "TXD",
		.HasTexture = 1,
		.NumSrcRegs = 3,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_TXL,
		.Name = "TXL",
		.HasTexture = 1,
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_TXP,
		.Name = "TXP",
		.HasTexture = 1,
		.NumSrcRegs = 1,
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_IF,
		.Name = "IF",
		.IsFlowControl = 1,
		.NumSrcRegs = 1
	},
	{
		.Opcode = RC_OPCODE_ELSE,
		.Name = "ELSE",
		.IsFlowControl = 1,
		.NumSrcRegs = 0
	},
	{
		.Opcode = RC_OPCODE_ENDIF,
		.Name = "ENDIF",
		.IsFlowControl = 1,
		.NumSrcRegs = 0
	},
	{
		.Opcode = RC_OPCODE_REPL_ALPHA,
		.Name = "REPL_ALPHA",
		.HasDstReg = 1
	},
	{
		.Opcode = RC_OPCODE_BEGIN_TEX,
		.Name = "BEGIN_TEX"
	}
};

void rc_compute_sources_for_writemask(
		const struct rc_opcode_info * opcode,
		unsigned int writemask,
		unsigned int *srcmasks)
{
	srcmasks[0] = 0;
	srcmasks[1] = 0;
	srcmasks[2] = 0;

	if (opcode->Opcode == RC_OPCODE_KIL)
		srcmasks[0] |= RC_MASK_XYZW;
	else if (opcode->Opcode == RC_OPCODE_IF)
		srcmasks[0] |= RC_MASK_X;

	if (!writemask)
		return;

	if (opcode->IsComponentwise) {
		for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src)
			srcmasks[src] |= writemask;
	} else if (opcode->IsStandardScalar) {
		for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src)
			srcmasks[src] |= RC_MASK_X;
	} else {
		switch(opcode->Opcode) {
		case RC_OPCODE_ARL:
			srcmasks[0] |= RC_MASK_X;
			break;
		case RC_OPCODE_DP3:
			srcmasks[0] |= RC_MASK_XYZ;
			srcmasks[1] |= RC_MASK_XYZ;
			break;
		case RC_OPCODE_DP4:
			srcmasks[0] |= RC_MASK_XYZW;
			srcmasks[1] |= RC_MASK_XYZW;
			break;
		case RC_OPCODE_TEX:
		case RC_OPCODE_TXB:
		case RC_OPCODE_TXP:
			srcmasks[0] |= RC_MASK_XYZW;
			break;
		case RC_OPCODE_DST:
			srcmasks[0] |= 0x6;
			srcmasks[1] |= 0xa;
			break;
		case RC_OPCODE_EXP:
		case RC_OPCODE_LOG:
			srcmasks[0] |= RC_MASK_XY;
			break;
		case RC_OPCODE_LIT:
			srcmasks[0] |= 0xb;
			break;
		default:
			break;
		}
	}
}
