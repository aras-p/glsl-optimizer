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

#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "r300_context.h"
#include "r300_fragprog.h"
#include "r300_fragprog_swizzle.h"
#include "r300_state.h"

#include "radeon_nqssadce.h"

static void reset_srcreg(struct prog_src_register* reg)
{
	_mesa_bzero(reg, sizeof(*reg));
	reg->Swizzle = SWIZZLE_NOOP;
}

static struct prog_src_register shadow_ambient(struct gl_program *program, int tmu)
{
	gl_state_index fail_value_tokens[STATE_LENGTH] = {
		STATE_INTERNAL, STATE_SHADOW_AMBIENT, 0, 0, 0
	};
	struct prog_src_register reg = { 0, };

	fail_value_tokens[2] = tmu;
	reg.File = PROGRAM_STATE_VAR;
	reg.Index = _mesa_add_state_reference(program->Parameters, fail_value_tokens);
	reg.Swizzle = SWIZZLE_WWWW;
	return reg;
}

/**
 * Transform TEX, TXP, TXB, and KIL instructions in the following way:
 *  - premultiply texture coordinates for RECT
 *  - extract operand swizzles
 *  - introduce a temporary register when write masks are needed
 *
 * \todo If/when r5xx uses the radeon_program architecture, this can probably
 * be reused.
 */
GLboolean r300_transform_TEX(
	struct radeon_transform_context *t,
	struct prog_instruction* orig_inst, void* data)
{
	struct r300_fragment_program_compiler *compiler =
		(struct r300_fragment_program_compiler*)data;
	struct prog_instruction inst = *orig_inst;
	struct prog_instruction* tgt;
	GLboolean destredirect = GL_FALSE;

	if (inst.Opcode != OPCODE_TEX &&
	    inst.Opcode != OPCODE_TXB &&
	    inst.Opcode != OPCODE_TXP &&
	    inst.Opcode != OPCODE_KIL)
		return GL_FALSE;

	if (inst.Opcode != OPCODE_KIL &&
	    t->Program->ShadowSamplers & (1 << inst.TexSrcUnit)) {
		GLuint comparefunc = GL_NEVER + compiler->fp->state.unit[inst.TexSrcUnit].texture_compare_func;

		if (comparefunc == GL_NEVER || comparefunc == GL_ALWAYS) {
			tgt = radeonAppendInstructions(t->Program, 1);

			tgt->Opcode = OPCODE_MOV;
			tgt->DstReg = inst.DstReg;
			if (comparefunc == GL_ALWAYS) {
				tgt->SrcReg[0].File = PROGRAM_BUILTIN;
				tgt->SrcReg[0].Swizzle = SWIZZLE_1111;
			} else {
				tgt->SrcReg[0] = shadow_ambient(t->Program, inst.TexSrcUnit);
			}
			return GL_TRUE;
		}

		inst.DstReg.File = PROGRAM_TEMPORARY;
		inst.DstReg.Index = radeonFindFreeTemporary(t);
		inst.DstReg.WriteMask = WRITEMASK_XYZW;
	}


	/* Hardware uses [0..1]x[0..1] range for rectangle textures
	 * instead of [0..Width]x[0..Height].
	 * Add a scaling instruction.
	 */
	if (inst.Opcode != OPCODE_KIL && inst.TexSrcTarget == TEXTURE_RECT_INDEX) {
		gl_state_index tokens[STATE_LENGTH] = {
			STATE_INTERNAL, STATE_R300_TEXRECT_FACTOR, 0, 0,
			0
		};

		int tempreg = radeonFindFreeTemporary(t);
		int factor_index;

		tokens[2] = inst.TexSrcUnit;
		factor_index = _mesa_add_state_reference(t->Program->Parameters, tokens);

		tgt = radeonAppendInstructions(t->Program, 1);

		tgt->Opcode = OPCODE_MUL;
		tgt->DstReg.File = PROGRAM_TEMPORARY;
		tgt->DstReg.Index = tempreg;
		tgt->SrcReg[0] = inst.SrcReg[0];
		tgt->SrcReg[1].File = PROGRAM_STATE_VAR;
		tgt->SrcReg[1].Index = factor_index;

		reset_srcreg(&inst.SrcReg[0]);
		inst.SrcReg[0].File = PROGRAM_TEMPORARY;
		inst.SrcReg[0].Index = tempreg;
	}

	if (inst.Opcode != OPCODE_KIL) {
		if (inst.DstReg.File != PROGRAM_TEMPORARY ||
		    inst.DstReg.WriteMask != WRITEMASK_XYZW) {
			int tempreg = radeonFindFreeTemporary(t);

			inst.DstReg.File = PROGRAM_TEMPORARY;
			inst.DstReg.Index = tempreg;
			inst.DstReg.WriteMask = WRITEMASK_XYZW;
			destredirect = GL_TRUE;
		}
	}

	if (inst.SrcReg[0].File != PROGRAM_TEMPORARY && inst.SrcReg[0].File != PROGRAM_INPUT) {
		int tmpreg = radeonFindFreeTemporary(t);
		tgt = radeonAppendInstructions(t->Program, 1);
		tgt->Opcode = OPCODE_MOV;
		tgt->DstReg.File = PROGRAM_TEMPORARY;
		tgt->DstReg.Index = tmpreg;
		tgt->SrcReg[0] = inst.SrcReg[0];

		reset_srcreg(&inst.SrcReg[0]);
		inst.SrcReg[0].File = PROGRAM_TEMPORARY;
		inst.SrcReg[0].Index = tmpreg;
	}

	tgt = radeonAppendInstructions(t->Program, 1);
	_mesa_copy_instructions(tgt, &inst, 1);

	if (inst.Opcode != OPCODE_KIL &&
	    t->Program->ShadowSamplers & (1 << inst.TexSrcUnit)) {
		GLuint comparefunc = GL_NEVER + compiler->fp->state.unit[inst.TexSrcUnit].texture_compare_func;
		GLuint depthmode = compiler->fp->state.unit[inst.TexSrcUnit].depth_texture_mode;
		int rcptemp = radeonFindFreeTemporary(t);
		int pass, fail;

		tgt = radeonAppendInstructions(t->Program, 3);

		tgt[0].Opcode = OPCODE_RCP;
		tgt[0].DstReg.File = PROGRAM_TEMPORARY;
		tgt[0].DstReg.Index = rcptemp;
		tgt[0].DstReg.WriteMask = WRITEMASK_W;
		tgt[0].SrcReg[0] = inst.SrcReg[0];
		tgt[0].SrcReg[0].Swizzle = SWIZZLE_WWWW;

		tgt[1].Opcode = OPCODE_MAD;
		tgt[1].DstReg = inst.DstReg;
		tgt[1].DstReg.WriteMask = orig_inst->DstReg.WriteMask;
		tgt[1].SrcReg[0] = inst.SrcReg[0];
		tgt[1].SrcReg[0].Swizzle = SWIZZLE_ZZZZ;
		tgt[1].SrcReg[1].File = PROGRAM_TEMPORARY;
		tgt[1].SrcReg[1].Index = rcptemp;
		tgt[1].SrcReg[1].Swizzle = SWIZZLE_WWWW;
		tgt[1].SrcReg[2].File = PROGRAM_TEMPORARY;
		tgt[1].SrcReg[2].Index = inst.DstReg.Index;
		if (depthmode == 0) /* GL_LUMINANCE */
			tgt[1].SrcReg[2].Swizzle = MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z);
		else if (depthmode == 2) /* GL_ALPHA */
			tgt[1].SrcReg[2].Swizzle = SWIZZLE_WWWW;

		/* Recall that SrcReg[0] is tex, SrcReg[2] is r and:
		 *   r  < tex  <=>      -tex+r < 0
		 *   r >= tex  <=> not (-tex+r < 0 */
		if (comparefunc == GL_LESS || comparefunc == GL_GEQUAL)
			tgt[1].SrcReg[2].NegateBase = tgt[0].SrcReg[2].NegateBase ^ NEGATE_XYZW;
		else
			tgt[1].SrcReg[0].NegateBase = tgt[0].SrcReg[0].NegateBase ^ NEGATE_XYZW;

		tgt[2].Opcode = OPCODE_CMP;
		tgt[2].DstReg = orig_inst->DstReg;
		tgt[2].SrcReg[0].File = PROGRAM_TEMPORARY;
		tgt[2].SrcReg[0].Index = tgt[1].DstReg.Index;

		if (comparefunc == GL_LESS || comparefunc == GL_GREATER) {
			pass = 1;
			fail = 2;
		} else {
			pass = 2;
			fail = 1;
		}

		tgt[2].SrcReg[pass].File = PROGRAM_BUILTIN;
		tgt[2].SrcReg[pass].Swizzle = SWIZZLE_1111;
		tgt[2].SrcReg[fail] = shadow_ambient(t->Program, inst.TexSrcUnit);
	} else if (destredirect) {
		tgt = radeonAppendInstructions(t->Program, 1);

		tgt->Opcode = OPCODE_MOV;
		tgt->DstReg = orig_inst->DstReg;
		tgt->SrcReg[0].File = PROGRAM_TEMPORARY;
		tgt->SrcReg[0].Index = inst.DstReg.Index;
	}

	return GL_TRUE;
}

/* just some random things... */
void r300FragmentProgramDump(union rX00_fragment_program_code *c)
{
	struct r300_fragment_program_code *code = &c->r300;
	int n, i, j;
	static int pc = 0;

	fprintf(stderr, "pc=%d*************************************\n", pc++);

	fprintf(stderr, "Hardware program\n");
	fprintf(stderr, "----------------\n");

	for (n = 0; n < (code->cur_node + 1); n++) {
		fprintf(stderr, "NODE %d: alu_offset: %d, tex_offset: %d, "
			"alu_end: %d, tex_end: %d, flags: %08x\n", n,
			code->node[n].alu_offset,
			code->node[n].tex_offset,
			code->node[n].alu_end, code->node[n].tex_end,
			code->node[n].flags);

		if (n > 0 || code->first_node_has_tex) {
			fprintf(stderr, "  TEX:\n");
			for (i = code->node[n].tex_offset;
			     i <= code->node[n].tex_offset + code->node[n].tex_end;
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

		for (i = code->node[n].alu_offset;
		     i <= code->node[n].alu_offset + code->node[n].alu_end; ++i) {
			char srcc[3][10], dstc[20];
			char srca[3][10], dsta[20];
			char argc[3][20];
			char arga[3][20];
			char flags[5], tmp[10];

			for (j = 0; j < 3; ++j) {
				int regc = code->alu.inst[i].inst1 >> (j * 6);
				int rega = code->alu.inst[i].inst3 >> (j * 6);

				sprintf(srcc[j], "%c%i",
					(regc & 32) ? 'c' : 't', regc & 31);
				sprintf(srca[j], "%c%i",
					(rega & 32) ? 'c' : 't', rega & 31);
			}

			dstc[0] = 0;
			sprintf(flags, "%s%s%s",
				(code->alu.inst[i].
				 inst1 & R300_ALU_DSTC_REG_X) ? "x" : "",
				(code->alu.inst[i].
				 inst1 & R300_ALU_DSTC_REG_Y) ? "y" : "",
				(code->alu.inst[i].
				 inst1 & R300_ALU_DSTC_REG_Z) ? "z" : "");
			if (flags[0] != 0) {
				sprintf(dstc, "t%i.%s ",
					(code->alu.inst[i].
					 inst1 >> R300_ALU_DSTC_SHIFT) & 31,
					flags);
			}
			sprintf(flags, "%s%s%s",
				(code->alu.inst[i].
				 inst1 & R300_ALU_DSTC_OUTPUT_X) ? "x" : "",
				(code->alu.inst[i].
				 inst1 & R300_ALU_DSTC_OUTPUT_Y) ? "y" : "",
				(code->alu.inst[i].
				 inst1 & R300_ALU_DSTC_OUTPUT_Z) ? "z" : "");
			if (flags[0] != 0) {
				sprintf(tmp, "o%i.%s",
					(code->alu.inst[i].
					 inst1 >> R300_ALU_DSTC_SHIFT) & 31,
					flags);
				strcat(dstc, tmp);
			}

			dsta[0] = 0;
			if (code->alu.inst[i].inst3 & R300_ALU_DSTA_REG) {
				sprintf(dsta, "t%i.w ",
					(code->alu.inst[i].
					 inst3 >> R300_ALU_DSTA_SHIFT) & 31);
			}
			if (code->alu.inst[i].inst3 & R300_ALU_DSTA_OUTPUT) {
				sprintf(tmp, "o%i.w ",
					(code->alu.inst[i].
					 inst3 >> R300_ALU_DSTA_SHIFT) & 31);
				strcat(dsta, tmp);
			}
			if (code->alu.inst[i].inst3 & R300_ALU_DSTA_DEPTH) {
				strcat(dsta, "Z");
			}

			fprintf(stderr,
				"%3i: xyz: %3s %3s %3s -> %-20s (%08x)\n"
				"       w: %3s %3s %3s -> %-20s (%08x)\n", i,
				srcc[0], srcc[1], srcc[2], dstc,
				code->alu.inst[i].inst1, srca[0], srca[1],
				srca[2], dsta, code->alu.inst[i].inst3);

			for (j = 0; j < 3; ++j) {
				int regc = code->alu.inst[i].inst0 >> (j * 7);
				int rega = code->alu.inst[i].inst2 >> (j * 7);
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
				code->alu.inst[i].inst0, arga[0], arga[1],
				arga[2], code->alu.inst[i].inst2);
		}
	}
}
