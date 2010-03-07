/*
 * Copyright (C) 2005 Ben Skeggs.
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

#include "r300_fragprog.h"

#include <stdio.h>

#include "../r300_reg.h"

static struct rc_src_register shadow_ambient(struct radeon_compiler * c, int tmu)
{
	struct rc_src_register reg = { 0, };

	reg.File = RC_FILE_CONSTANT;
	reg.Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_SHADOW_AMBIENT, tmu);
	reg.Swizzle = RC_SWIZZLE_WWWW;
	return reg;
}

/**
 * Transform TEX, TXP, TXB, and KIL instructions in the following way:
 *  - premultiply texture coordinates for RECT
 *  - extract operand swizzles
 *  - introduce a temporary register when write masks are needed
 */
int r300_transform_TEX(
	struct radeon_compiler * c,
	struct rc_instruction* inst,
	void* data)
{
	struct r300_fragment_program_compiler *compiler =
		(struct r300_fragment_program_compiler*)data;

	if (inst->U.I.Opcode != RC_OPCODE_TEX &&
	    inst->U.I.Opcode != RC_OPCODE_TXB &&
	    inst->U.I.Opcode != RC_OPCODE_TXP &&
	    inst->U.I.Opcode != RC_OPCODE_KIL)
		return 0;

	/* ARB_shadow & EXT_shadow_funcs */
	if (inst->U.I.Opcode != RC_OPCODE_KIL &&
	    c->Program.ShadowSamplers & (1 << inst->U.I.TexSrcUnit)) {
		rc_compare_func comparefunc = compiler->state.unit[inst->U.I.TexSrcUnit].texture_compare_func;

		if (comparefunc == RC_COMPARE_FUNC_NEVER || comparefunc == RC_COMPARE_FUNC_ALWAYS) {
			inst->U.I.Opcode = RC_OPCODE_MOV;

			if (comparefunc == RC_COMPARE_FUNC_ALWAYS) {
				inst->U.I.SrcReg[0].File = RC_FILE_NONE;
				inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_1111;
			} else {
				inst->U.I.SrcReg[0] = shadow_ambient(c, inst->U.I.TexSrcUnit);
			}

			return 1;
		} else {
			rc_compare_func comparefunc = compiler->state.unit[inst->U.I.TexSrcUnit].texture_compare_func;
			unsigned int depthmode = compiler->state.unit[inst->U.I.TexSrcUnit].depth_texture_mode;
			struct rc_instruction * inst_rcp = rc_insert_new_instruction(c, inst);
			struct rc_instruction * inst_mad = rc_insert_new_instruction(c, inst_rcp);
			struct rc_instruction * inst_cmp = rc_insert_new_instruction(c, inst_mad);
			int pass, fail;

			inst_rcp->U.I.Opcode = RC_OPCODE_RCP;
			inst_rcp->U.I.DstReg.File = RC_FILE_TEMPORARY;
			inst_rcp->U.I.DstReg.Index = rc_find_free_temporary(c);
			inst_rcp->U.I.DstReg.WriteMask = RC_MASK_W;
			inst_rcp->U.I.SrcReg[0] = inst->U.I.SrcReg[0];
			inst_rcp->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_WWWW;

			inst_cmp->U.I.DstReg = inst->U.I.DstReg;
			inst->U.I.DstReg.File = RC_FILE_TEMPORARY;
			inst->U.I.DstReg.Index = rc_find_free_temporary(c);
			inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;

			inst_mad->U.I.Opcode = RC_OPCODE_MAD;
			inst_mad->U.I.DstReg.File = RC_FILE_TEMPORARY;
			inst_mad->U.I.DstReg.Index = rc_find_free_temporary(c);
			inst_mad->U.I.SrcReg[0] = inst->U.I.SrcReg[0];
			inst_mad->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_ZZZZ;
			inst_mad->U.I.SrcReg[1].File = RC_FILE_TEMPORARY;
			inst_mad->U.I.SrcReg[1].Index = inst_rcp->U.I.DstReg.Index;
			inst_mad->U.I.SrcReg[1].Swizzle = RC_SWIZZLE_WWWW;
			inst_mad->U.I.SrcReg[2].File = RC_FILE_TEMPORARY;
			inst_mad->U.I.SrcReg[2].Index = inst->U.I.DstReg.Index;
			if (depthmode == 0) /* GL_LUMINANCE */
				inst_mad->U.I.SrcReg[2].Swizzle = RC_MAKE_SWIZZLE(RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_Z);
			else if (depthmode == 2) /* GL_ALPHA */
				inst_mad->U.I.SrcReg[2].Swizzle = RC_SWIZZLE_WWWW;

			/* Recall that SrcReg[0] is tex, SrcReg[2] is r and:
			 *   r  < tex  <=>      -tex+r < 0
			 *   r >= tex  <=> not (-tex+r < 0 */
			if (comparefunc == RC_COMPARE_FUNC_LESS || comparefunc == RC_COMPARE_FUNC_GEQUAL)
				inst_mad->U.I.SrcReg[2].Negate = inst_mad->U.I.SrcReg[2].Negate ^ RC_MASK_XYZW;
			else
				inst_mad->U.I.SrcReg[0].Negate = inst_mad->U.I.SrcReg[0].Negate ^ RC_MASK_XYZW;

			inst_cmp->U.I.Opcode = RC_OPCODE_CMP;
			/* DstReg has been filled out above */
			inst_cmp->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
			inst_cmp->U.I.SrcReg[0].Index = inst_mad->U.I.DstReg.Index;

			if (comparefunc == RC_COMPARE_FUNC_LESS || comparefunc == RC_COMPARE_FUNC_GREATER) {
				pass = 1;
				fail = 2;
			} else {
				pass = 2;
				fail = 1;
			}

			inst_cmp->U.I.SrcReg[pass].File = RC_FILE_NONE;
			inst_cmp->U.I.SrcReg[pass].Swizzle = RC_SWIZZLE_1111;
			inst_cmp->U.I.SrcReg[fail] = shadow_ambient(c, inst->U.I.TexSrcUnit);
		}
	}

	/* Hardware uses [0..1]x[0..1] range for rectangle textures
	 * instead of [0..Width]x[0..Height].
	 * Add a scaling instruction.
	 */
	if (inst->U.I.Opcode != RC_OPCODE_KIL && inst->U.I.TexSrcTarget == RC_TEXTURE_RECT) {
		struct rc_instruction * inst_mul = rc_insert_new_instruction(c, inst->Prev);

		inst_mul->U.I.Opcode = RC_OPCODE_MUL;
		inst_mul->U.I.DstReg.File = RC_FILE_TEMPORARY;
		inst_mul->U.I.DstReg.Index = rc_find_free_temporary(c);
		inst_mul->U.I.SrcReg[0] = inst->U.I.SrcReg[0];
		inst_mul->U.I.SrcReg[1].File = RC_FILE_CONSTANT;
		inst_mul->U.I.SrcReg[1].Index = rc_constants_add_state(&c->Program.Constants, RC_STATE_R300_TEXRECT_FACTOR, inst->U.I.TexSrcUnit);

		reset_srcreg(&inst->U.I.SrcReg[0]);
		inst->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
		inst->U.I.SrcReg[0].Index = inst_mul->U.I.DstReg.Index;
	}

	/* Cannot write texture to output registers or with masks */
	if (inst->U.I.Opcode != RC_OPCODE_KIL &&
	    (inst->U.I.DstReg.File != RC_FILE_TEMPORARY || inst->U.I.DstReg.WriteMask != RC_MASK_XYZW)) {
		struct rc_instruction * inst_mov = rc_insert_new_instruction(c, inst);

		inst_mov->U.I.Opcode = RC_OPCODE_MOV;
		inst_mov->U.I.DstReg = inst->U.I.DstReg;
		inst_mov->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
		inst_mov->U.I.SrcReg[0].Index = rc_find_free_temporary(c);

		inst->U.I.DstReg.File = RC_FILE_TEMPORARY;
		inst->U.I.DstReg.Index = inst_mov->U.I.SrcReg[0].Index;
		inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;
	}


	/* Cannot read texture coordinate from constants file */
	if (inst->U.I.SrcReg[0].File != RC_FILE_TEMPORARY && inst->U.I.SrcReg[0].File != RC_FILE_INPUT) {
		struct rc_instruction * inst_mov = rc_insert_new_instruction(c, inst->Prev);

		inst_mov->U.I.Opcode = RC_OPCODE_MOV;
		inst_mov->U.I.DstReg.File = RC_FILE_TEMPORARY;
		inst_mov->U.I.DstReg.Index = rc_find_free_temporary(c);
		inst_mov->U.I.SrcReg[0] = inst->U.I.SrcReg[0];

		reset_srcreg(&inst->U.I.SrcReg[0]);
		inst->U.I.SrcReg[0].File = RC_FILE_TEMPORARY;
		inst->U.I.SrcReg[0].Index = inst_mov->U.I.DstReg.Index;
	}

	return 1;
}

/* just some random things... */
void r300FragmentProgramDump(struct rX00_fragment_program_code *c)
{
	struct r300_fragment_program_code *code = &c->code.r300;
	int n, i, j;
	static int pc = 0;

	fprintf(stderr, "pc=%d*************************************\n", pc++);

	fprintf(stderr, "Hardware program\n");
	fprintf(stderr, "----------------\n");

	for (n = 0; n <= (code->config & 3); n++) {
		uint32_t code_addr = code->code_addr[3 - (code->config & 3) + n];
		int alu_offset = (code_addr & R300_ALU_START_MASK) >> R300_ALU_START_SHIFT;
		int alu_end = (code_addr & R300_ALU_SIZE_MASK) >> R300_ALU_SIZE_SHIFT;
		int tex_offset = (code_addr & R300_TEX_START_MASK) >> R300_TEX_START_SHIFT;
		int tex_end = (code_addr & R300_TEX_SIZE_MASK) >> R300_TEX_SIZE_SHIFT;

		fprintf(stderr, "NODE %d: alu_offset: %d, tex_offset: %d, "
			"alu_end: %d, tex_end: %d  (code_addr: %08x)\n", n,
			alu_offset, tex_offset, alu_end, tex_end, code_addr);

		if (n > 0 || (code->config & R300_PFS_CNTL_FIRST_NODE_HAS_TEX)) {
			fprintf(stderr, "  TEX:\n");
			for (i = tex_offset;
			     i <= tex_offset + tex_end;
			     ++i) {
				const char *instr;

				switch ((code->tex.
					 inst[i] >> R300_TEX_INST_SHIFT) &
					15) {
				case R300_TEX_OP_LD:
					instr = "TEX";
					break;
				case R300_TEX_OP_KIL:
					instr = "KIL";
					break;
				case R300_TEX_OP_TXP:
					instr = "TXP";
					break;
				case R300_TEX_OP_TXB:
					instr = "TXB";
					break;
				default:
					instr = "UNKNOWN";
				}

				fprintf(stderr,
					"    %s t%i, %c%i, texture[%i]   (%08x)\n",
					instr,
					(code->tex.
					 inst[i] >> R300_DST_ADDR_SHIFT) & 31,
					't',
					(code->tex.
					 inst[i] >> R300_SRC_ADDR_SHIFT) & 31,
					(code->tex.
					 inst[i] & R300_TEX_ID_MASK) >>
					R300_TEX_ID_SHIFT,
					code->tex.inst[i]);
			}
		}

		for (i = alu_offset;
		     i <= alu_offset + alu_end; ++i) {
			char srcc[3][10], dstc[20];
			char srca[3][10], dsta[20];
			char argc[3][20];
			char arga[3][20];
			char flags[5], tmp[10];

			for (j = 0; j < 3; ++j) {
				int regc = code->alu.inst[i].rgb_addr >> (j * 6);
				int rega = code->alu.inst[i].alpha_addr >> (j * 6);

				sprintf(srcc[j], "%c%i",
					(regc & 32) ? 'c' : 't', regc & 31);
				sprintf(srca[j], "%c%i",
					(rega & 32) ? 'c' : 't', rega & 31);
			}

			dstc[0] = 0;
			sprintf(flags, "%s%s%s",
				(code->alu.inst[i].
				 rgb_addr & R300_ALU_DSTC_REG_X) ? "x" : "",
				(code->alu.inst[i].
				 rgb_addr & R300_ALU_DSTC_REG_Y) ? "y" : "",
				(code->alu.inst[i].
				 rgb_addr & R300_ALU_DSTC_REG_Z) ? "z" : "");
			if (flags[0] != 0) {
				sprintf(dstc, "t%i.%s ",
					(code->alu.inst[i].
					 rgb_addr >> R300_ALU_DSTC_SHIFT) & 31,
					flags);
			}
			sprintf(flags, "%s%s%s",
				(code->alu.inst[i].
				 rgb_addr & R300_ALU_DSTC_OUTPUT_X) ? "x" : "",
				(code->alu.inst[i].
				 rgb_addr & R300_ALU_DSTC_OUTPUT_Y) ? "y" : "",
				(code->alu.inst[i].
				 rgb_addr & R300_ALU_DSTC_OUTPUT_Z) ? "z" : "");
			if (flags[0] != 0) {
				sprintf(tmp, "o%i.%s",
					(code->alu.inst[i].
					 rgb_addr >> 29) & 3,
					flags);
				strcat(dstc, tmp);
			}

			dsta[0] = 0;
			if (code->alu.inst[i].alpha_addr & R300_ALU_DSTA_REG) {
				sprintf(dsta, "t%i.w ",
					(code->alu.inst[i].
					 alpha_addr >> R300_ALU_DSTA_SHIFT) & 31);
			}
			if (code->alu.inst[i].alpha_addr & R300_ALU_DSTA_OUTPUT) {
				sprintf(tmp, "o%i.w ",
					(code->alu.inst[i].
					 alpha_addr >> 25) & 3);
				strcat(dsta, tmp);
			}
			if (code->alu.inst[i].alpha_addr & R300_ALU_DSTA_DEPTH) {
				strcat(dsta, "Z");
			}

			fprintf(stderr,
				"%3i: xyz: %3s %3s %3s -> %-20s (%08x)\n"
				"       w: %3s %3s %3s -> %-20s (%08x)\n", i,
				srcc[0], srcc[1], srcc[2], dstc,
				code->alu.inst[i].rgb_addr, srca[0], srca[1],
				srca[2], dsta, code->alu.inst[i].alpha_addr);

			for (j = 0; j < 3; ++j) {
				int regc = code->alu.inst[i].rgb_inst >> (j * 7);
				int rega = code->alu.inst[i].alpha_inst >> (j * 7);
				int d;
				char buf[20];

				d = regc & 31;
				if (d < 12) {
					switch (d % 4) {
					case R300_ALU_ARGC_SRC0C_XYZ:
						sprintf(buf, "%s.xyz",
							srcc[d / 4]);
						break;
					case R300_ALU_ARGC_SRC0C_XXX:
						sprintf(buf, "%s.xxx",
							srcc[d / 4]);
						break;
					case R300_ALU_ARGC_SRC0C_YYY:
						sprintf(buf, "%s.yyy",
							srcc[d / 4]);
						break;
					case R300_ALU_ARGC_SRC0C_ZZZ:
						sprintf(buf, "%s.zzz",
							srcc[d / 4]);
						break;
					}
				} else if (d < 15) {
					sprintf(buf, "%s.www", srca[d - 12]);
				} else if (d == 20) {
					sprintf(buf, "0.0");
				} else if (d == 21) {
					sprintf(buf, "1.0");
				} else if (d == 22) {
					sprintf(buf, "0.5");
				} else if (d >= 23 && d < 32) {
					d -= 23;
					switch (d / 3) {
					case 0:
						sprintf(buf, "%s.yzx",
							srcc[d % 3]);
						break;
					case 1:
						sprintf(buf, "%s.zxy",
							srcc[d % 3]);
						break;
					case 2:
						sprintf(buf, "%s.Wzy",
							srcc[d % 3]);
						break;
					}
				} else {
					sprintf(buf, "%i", d);
				}

				sprintf(argc[j], "%s%s%s%s",
					(regc & 32) ? "-" : "",
					(regc & 64) ? "|" : "",
					buf, (regc & 64) ? "|" : "");

				d = rega & 31;
				if (d < 9) {
					sprintf(buf, "%s.%c", srcc[d / 3],
						'x' + (char)(d % 3));
				} else if (d < 12) {
					sprintf(buf, "%s.w", srca[d - 9]);
				} else if (d == 16) {
					sprintf(buf, "0.0");
				} else if (d == 17) {
					sprintf(buf, "1.0");
				} else if (d == 18) {
					sprintf(buf, "0.5");
				} else {
					sprintf(buf, "%i", d);
				}

				sprintf(arga[j], "%s%s%s%s",
					(rega & 32) ? "-" : "",
					(rega & 64) ? "|" : "",
					buf, (rega & 64) ? "|" : "");
			}

			fprintf(stderr, "     xyz: %8s %8s %8s    op: %08x\n"
				"       w: %8s %8s %8s    op: %08x\n",
				argc[0], argc[1], argc[2],
				code->alu.inst[i].rgb_inst, arga[0], arga[1],
				arga[2], code->alu.inst[i].alpha_inst);
		}
	}
}
