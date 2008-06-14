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

/**
 * \file
 *
 * Fragment program compiler. Perform transformations on the intermediate
 * \ref radeon_program representation (which is essentially the Mesa
 * program representation plus the notion of clauses) until the program
 * is in a form where we can translate it more or less directly into
 * machine-readable form.
 *
 * \author Ben Skeggs <darktama@iinet.net.au>
 * \author Jerome Glisse <j.glisse@gmail.com>
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "r300_context.h"
#include "r300_fragprog.h"
#include "r300_state.h"


static void reset_srcreg(struct prog_src_register* reg)
{
	_mesa_bzero(reg, sizeof(*reg));
	reg->Swizzle = SWIZZLE_NOOP;
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
static GLboolean transform_TEX(
	struct radeon_program_transform_context* context,
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

	/* Hardware uses [0..1]x[0..1] range for rectangle textures
	 * instead of [0..Width]x[0..Height].
	 * Add a scaling instruction.
	 */
	if (inst.Opcode != OPCODE_KIL && inst.TexSrcTarget == TEXTURE_RECT_INDEX) {
		gl_state_index tokens[STATE_LENGTH] = {
			STATE_INTERNAL, STATE_R300_TEXRECT_FACTOR, 0, 0,
			0
		};

		int tempreg = radeonCompilerAllocateTemporary(context->compiler);
		int factor_index;

		tokens[2] = inst.TexSrcUnit;
		factor_index =
			_mesa_add_state_reference(
				compiler->fp->mesa_program.Base.Parameters, tokens);

		tgt = radeonClauseInsertInstructions(context->compiler, context->dest,
			context->dest->NumInstructions, 1);

		tgt->Opcode = OPCODE_MAD;
		tgt->DstReg.File = PROGRAM_TEMPORARY;
		tgt->DstReg.Index = tempreg;
		tgt->SrcReg[0] = inst.SrcReg[0];
		tgt->SrcReg[1].File = PROGRAM_STATE_VAR;
		tgt->SrcReg[1].Index = factor_index;
		tgt->SrcReg[2].File = PROGRAM_BUILTIN;
		tgt->SrcReg[2].Swizzle = SWIZZLE_0000;

		reset_srcreg(&inst.SrcReg[0]);
		inst.SrcReg[0].File = PROGRAM_TEMPORARY;
		inst.SrcReg[0].Index = tempreg;
	}

	/* Texture operations do not support swizzles etc. in hardware,
	 * so emit an additional arithmetic operation if necessary.
	 */
	if (inst.SrcReg[0].Swizzle != SWIZZLE_NOOP ||
	    inst.SrcReg[0].Abs || inst.SrcReg[0].NegateBase || inst.SrcReg[0].NegateAbs) {
		int tempreg = radeonCompilerAllocateTemporary(context->compiler);

		tgt = radeonClauseInsertInstructions(context->compiler, context->dest,
			context->dest->NumInstructions, 1);

		tgt->Opcode = OPCODE_MAD;
		tgt->DstReg.File = PROGRAM_TEMPORARY;
		tgt->DstReg.Index = tempreg;
		tgt->SrcReg[0] = inst.SrcReg[0];
		tgt->SrcReg[1].File = PROGRAM_BUILTIN;
		tgt->SrcReg[1].Swizzle = SWIZZLE_1111;
		tgt->SrcReg[2].File = PROGRAM_BUILTIN;
		tgt->SrcReg[2].Swizzle = SWIZZLE_0000;

		reset_srcreg(&inst.SrcReg[0]);
		inst.SrcReg[0].File = PROGRAM_TEMPORARY;
		inst.SrcReg[0].Index = tempreg;
	}

	if (inst.Opcode != OPCODE_KIL) {
		if (inst.DstReg.File != PROGRAM_TEMPORARY ||
		    inst.DstReg.WriteMask != WRITEMASK_XYZW) {
			int tempreg = radeonCompilerAllocateTemporary(context->compiler);

			inst.DstReg.File = PROGRAM_TEMPORARY;
			inst.DstReg.Index = tempreg;
			inst.DstReg.WriteMask = WRITEMASK_XYZW;
			destredirect = GL_TRUE;
		}
	}

	tgt = radeonClauseInsertInstructions(context->compiler, context->dest,
		context->dest->NumInstructions, 1);
	_mesa_copy_instructions(tgt, &inst, 1);

	if (destredirect) {
		tgt = radeonClauseInsertInstructions(context->compiler, context->dest,
			context->dest->NumInstructions, 1);

		tgt->Opcode = OPCODE_MAD;
		tgt->DstReg = orig_inst->DstReg;
		tgt->SrcReg[0].File = PROGRAM_TEMPORARY;
		tgt->SrcReg[0].Index = inst.DstReg.Index;
		tgt->SrcReg[1].File = PROGRAM_BUILTIN;
		tgt->SrcReg[1].Swizzle = SWIZZLE_1111;
		tgt->SrcReg[2].File = PROGRAM_BUILTIN;
		tgt->SrcReg[2].Swizzle = SWIZZLE_0000;
	}

	return GL_TRUE;
}


static void update_params(r300ContextPtr r300, struct r300_fragment_program *fp)
{
	struct gl_fragment_program *mp = &fp->mesa_program;

	/* Ask Mesa nicely to fill in ParameterValues for us */
	if (mp->Base.Parameters)
		_mesa_load_state_parameters(r300->radeon.glCtx, mp->Base.Parameters);
}


/**
 * Transform the program to support fragment.position.
 *
 * Introduce a small fragment at the start of the program that will be
 * the only code that directly reads the FRAG_ATTRIB_WPOS input.
 * All other code pieces that reference that input will be rewritten
 * to read from a newly allocated temporary.
 *
 * \todo if/when r5xx supports the radeon_program architecture, this is a
 * likely candidate for code sharing.
 */
static void insert_WPOS_trailer(struct r300_fragment_program_compiler *compiler)
{
	GLuint InputsRead = compiler->fp->mesa_program.Base.InputsRead;

	if (!(InputsRead & FRAG_BIT_WPOS))
		return;

	static gl_state_index tokens[STATE_LENGTH] = {
		STATE_INTERNAL, STATE_R300_WINDOW_DIMENSION, 0, 0, 0
	};
	struct prog_instruction *fpi;
	GLuint window_index;
	int i = 0;
	GLuint tempregi = radeonCompilerAllocateTemporary(&compiler->compiler);

	fpi = radeonClauseInsertInstructions(&compiler->compiler, &compiler->compiler.Clauses[0], 0, 3);

	/* perspective divide */
	fpi[i].Opcode = OPCODE_RCP;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_W;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_INPUT;
	fpi[i].SrcReg[0].Index = FRAG_ATTRIB_WPOS;
	fpi[i].SrcReg[0].Swizzle = SWIZZLE_WWWW;
	i++;

	fpi[i].Opcode = OPCODE_MUL;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_XYZ;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_INPUT;
	fpi[i].SrcReg[0].Index = FRAG_ATTRIB_WPOS;
	fpi[i].SrcReg[0].Swizzle = SWIZZLE_XYZW;

	fpi[i].SrcReg[1].File = PROGRAM_TEMPORARY;
	fpi[i].SrcReg[1].Index = tempregi;
	fpi[i].SrcReg[1].Swizzle = SWIZZLE_WWWW;
	i++;

	/* viewport transformation */
	window_index = _mesa_add_state_reference(compiler->fp->mesa_program.Base.Parameters, tokens);

	fpi[i].Opcode = OPCODE_MAD;

	fpi[i].DstReg.File = PROGRAM_TEMPORARY;
	fpi[i].DstReg.Index = tempregi;
	fpi[i].DstReg.WriteMask = WRITEMASK_XYZ;
	fpi[i].DstReg.CondMask = COND_TR;

	fpi[i].SrcReg[0].File = PROGRAM_TEMPORARY;
	fpi[i].SrcReg[0].Index = tempregi;
	fpi[i].SrcReg[0].Swizzle =
	    MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	fpi[i].SrcReg[1].File = PROGRAM_STATE_VAR;
	fpi[i].SrcReg[1].Index = window_index;
	fpi[i].SrcReg[1].Swizzle =
	    MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	fpi[i].SrcReg[2].File = PROGRAM_STATE_VAR;
	fpi[i].SrcReg[2].Index = window_index;
	fpi[i].SrcReg[2].Swizzle =
	    MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);
	i++;

	for (; i < compiler->compiler.Clauses[0].NumInstructions; ++i) {
		int reg;
		for (reg = 0; reg < 3; reg++) {
			if (fpi[i].SrcReg[reg].File == PROGRAM_INPUT &&
			    fpi[i].SrcReg[reg].Index == FRAG_ATTRIB_WPOS) {
				fpi[i].SrcReg[reg].File = PROGRAM_TEMPORARY;
				fpi[i].SrcReg[reg].Index = tempregi;
			}
		}
	}
}


void r300TranslateFragmentShader(r300ContextPtr r300,
				 struct r300_fragment_program *fp)
{
	if (!fp->translated) {
		struct r300_fragment_program_compiler compiler;

		compiler.r300 = r300;
		compiler.fp = fp;
		compiler.code = &fp->code;

		radeonCompilerInit(&compiler.compiler, r300->radeon.glCtx, &fp->mesa_program.Base);

		insert_WPOS_trailer(&compiler);

		struct radeon_program_transformation transformations[1] = {
			{ &transform_TEX, &compiler }
		};
		radeonClauseLocalTransform(&compiler.compiler,
			&compiler.compiler.Clauses[0],
			1, transformations);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Compiler state after transformations:\n");
			radeonCompilerDump(&compiler.compiler);
		}

		if (!r300FragmentProgramEmit(&compiler))
			fp->error = GL_TRUE;

		radeonCompilerCleanup(&compiler.compiler);

		if (!fp->error)
			fp->translated = GL_TRUE;
		if (fp->error || (RADEON_DEBUG & DEBUG_PIXEL))
			r300FragmentProgramDump(fp, &fp->code);
		r300UpdateStateParameters(r300->radeon.glCtx, _NEW_PROGRAM);
	}

	update_params(r300, fp);
}

/* just some random things... */
void r300FragmentProgramDump(
	struct r300_fragment_program *fp,
	struct r300_fragment_program_code *code)
{
	int n, i, j;
	static int pc = 0;

	fprintf(stderr, "pc=%d*************************************\n", pc++);

	fprintf(stderr, "Mesa program:\n");
	fprintf(stderr, "-------------\n");
	_mesa_print_program(&fp->mesa_program.Base);
	fflush(stdout);

	fprintf(stderr, "Hardware program\n");
	fprintf(stderr, "----------------\n");

	for (n = 0; n < (code->cur_node + 1); n++) {
		fprintf(stderr, "NODE %d: alu_offset: %d, tex_offset: %d, "
			"alu_end: %d, tex_end: %d\n", n,
			code->node[n].alu_offset,
			code->node[n].tex_offset,
			code->node[n].alu_end, code->node[n].tex_end);

		if (code->tex.length) {
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
