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
 * representation until the program is in a form where we can translate
 * it more or less directly into machine-readable form.
 *
 * \author Ben Skeggs <darktama@iinet.net.au>
 * \author Jerome Glisse <j.glisse@gmail.com>
 */

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "r300_context.h"
#include "r300_fragprog.h"
#include "r300_fragprog_swizzle.h"
#include "r300_state.h"

#include "radeon_nqssadce.h"
#include "radeon_program_alu.h"


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
static GLboolean transform_TEX(
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
	GLuint tempregi = _mesa_find_free_register(compiler->program, PROGRAM_TEMPORARY);

	_mesa_insert_instructions(compiler->program, 0, 3);
	fpi = compiler->program->Instructions;

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
	window_index = _mesa_add_state_reference(compiler->program->Parameters, tokens);

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

	for (; i < compiler->program->NumInstructions; ++i) {
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


static void nqssadce_init(struct nqssadce_state* s)
{
	s->Outputs[FRAG_RESULT_COLR].Sourced = WRITEMASK_XYZW;
	s->Outputs[FRAG_RESULT_DEPR].Sourced = WRITEMASK_W;
}


static GLuint build_dtm(GLuint depthmode)
{
	switch(depthmode) {
	default:
	case GL_LUMINANCE: return 0;
	case GL_INTENSITY: return 1;
	case GL_ALPHA: return 2;
	}
}

static GLuint build_func(GLuint comparefunc)
{
	return comparefunc - GL_NEVER;
}


/**
 * Collect all external state that is relevant for compiling the given
 * fragment program.
 */
static void build_state(
	r300ContextPtr r300,
	struct r300_fragment_program *fp,
	struct r300_fragment_program_external_state *state)
{
	int unit;

	_mesa_bzero(state, sizeof(*state));

	for(unit = 0; unit < 16; ++unit) {
		if (fp->mesa_program.Base.ShadowSamplers & (1 << unit)) {
			struct gl_texture_object* tex = r300->radeon.glCtx->Texture.Unit[unit]._Current;

			state->unit[unit].depth_texture_mode = build_dtm(tex->DepthMode);
			state->unit[unit].texture_compare_func = build_func(tex->CompareFunc);
		}
	}
}


void r300TranslateFragmentShader(r300ContextPtr r300,
				 struct r300_fragment_program *fp)
{
	struct r300_fragment_program_external_state state;

	build_state(r300, fp, &state);
	if (_mesa_memcmp(&fp->state, &state, sizeof(state))) {
		/* TODO: cache compiled programs */
		fp->translated = GL_FALSE;
		_mesa_memcpy(&fp->state, &state, sizeof(state));
	}

	if (!fp->translated) {
		struct r300_fragment_program_compiler compiler;

		compiler.r300 = r300;
		compiler.fp = fp;
		compiler.code = &fp->code;
		compiler.program = _mesa_clone_program(r300->radeon.glCtx, &fp->mesa_program.Base);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Fragment Program: Initial program:\n");
			_mesa_print_program(compiler.program);
		}

		insert_WPOS_trailer(&compiler);

		struct radeon_program_transformation transformations[] = {
			{ &transform_TEX, &compiler },
			{ &radeonTransformALU, 0 },
			{ &radeonTransformTrigSimple, 0 }
		};
		radeonLocalTransform(
			r300->radeon.glCtx,
			compiler.program,
			3, transformations);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Fragment Program: After native rewrite:\n");
			_mesa_print_program(compiler.program);
		}

		struct radeon_nqssadce_descr nqssadce = {
			.Init = &nqssadce_init,
			.IsNativeSwizzle = &r300FPIsNativeSwizzle,
			.BuildSwizzle = &r300FPBuildSwizzle,
			.RewriteDepthOut = GL_TRUE
		};
		radeonNqssaDce(r300->radeon.glCtx, compiler.program, &nqssadce);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Compiler: after NqSSA-DCE:\n");
			_mesa_print_program(compiler.program);
		}

		if (!r300FragmentProgramEmit(&compiler))
			fp->error = GL_TRUE;

		/* Subtle: Rescue any parameters that have been added during transformations */
		_mesa_free_parameter_list(fp->mesa_program.Base.Parameters);
		fp->mesa_program.Base.Parameters = compiler.program->Parameters;
		compiler.program->Parameters = 0;

		_mesa_reference_program(r300->radeon.glCtx, &compiler.program, NULL);

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
