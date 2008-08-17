/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r500_fragprog.h"

#include "radeon_nqssadce.h"
#include "radeon_program_alu.h"


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
 */
static GLboolean transform_TEX(
	struct radeon_transform_context *t,
	struct prog_instruction* orig_inst, void* data)
{
	struct r500_fragment_program_compiler *compiler =
		(struct r500_fragment_program_compiler*)data;
	struct prog_instruction inst = *orig_inst;
	struct prog_instruction* tgt;
	GLboolean destredirect = GL_FALSE;

	if (inst.Opcode != OPCODE_TEX &&
	    inst.Opcode != OPCODE_TXB &&
	    inst.Opcode != OPCODE_TXP &&
	    inst.Opcode != OPCODE_KIL)
		return GL_FALSE;

	/* ARB_shadow & EXT_shadow_funcs */
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
	} else if (inst.Opcode != OPCODE_KIL && inst.DstReg.File != PROGRAM_TEMPORARY) {
		int tempreg = radeonFindFreeTemporary(t);

		inst.DstReg.File = PROGRAM_TEMPORARY;
		inst.DstReg.Index = tempreg;
		inst.DstReg.WriteMask = WRITEMASK_XYZW;
		destredirect = GL_TRUE;
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


static void update_params(r300ContextPtr r300, struct r500_fragment_program *fp)
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
static void insert_WPOS_trailer(struct r500_fragment_program_compiler *compiler)
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

static GLboolean is_native_swizzle(GLuint opcode, struct prog_src_register reg)
{
	GLuint relevant;
	int i;

	if (opcode == OPCODE_TEX ||
	    opcode == OPCODE_TXB ||
	    opcode == OPCODE_TXP ||
	    opcode == OPCODE_KIL) {
		if (reg.Abs)
			return GL_FALSE;

		if (reg.NegateAbs)
			reg.NegateBase ^= 15;

		if (opcode == OPCODE_KIL) {
			if (reg.Swizzle != SWIZZLE_NOOP)
				return GL_FALSE;
		} else {
			for(i = 0; i < 4; ++i) {
				GLuint swz = GET_SWZ(reg.Swizzle, i);
				if (swz == SWIZZLE_NIL) {
					reg.NegateBase &= ~(1 << i);
					continue;
				}
				if (swz >= 4)
					return GL_FALSE;
			}
		}

		if (reg.NegateBase)
			return GL_FALSE;

		return GL_TRUE;
	} else if (opcode == OPCODE_DDX || opcode == OPCODE_DDY) {
		/* DDX/MDH and DDY/MDV explicitly ignore incoming swizzles;
		 * if it doesn't fit perfectly into a .xyzw case... */
		if (reg.Swizzle == SWIZZLE_NOOP && !reg.Abs
				&& !reg.NegateBase && !reg.NegateAbs)
			return GL_TRUE;

		return GL_FALSE;
	} else {
		/* ALU instructions support almost everything */
		if (reg.Abs)
			return GL_TRUE;

		relevant = 0;
		for(i = 0; i < 3; ++i) {
			GLuint swz = GET_SWZ(reg.Swizzle, i);
			if (swz != SWIZZLE_NIL && swz != SWIZZLE_ZERO)
				relevant |= 1 << i;
		}
		if ((reg.NegateBase & relevant) && ((reg.NegateBase & relevant) != relevant))
			return GL_FALSE;

		return GL_TRUE;
	}
}

/**
 * Implement a MOV with a potentially non-native swizzle.
 *
 * The only thing we *cannot* do in an ALU instruction is per-component
 * negation. Therefore, we split the MOV into two instructions when necessary.
 */
static void nqssadce_build_swizzle(struct nqssadce_state *s,
	struct prog_dst_register dst, struct prog_src_register src)
{
	struct prog_instruction *inst;
	GLuint negatebase[2] = { 0, 0 };
	int i;

	for(i = 0; i < 4; ++i) {
		GLuint swz = GET_SWZ(src.Swizzle, i);
		if (swz == SWIZZLE_NIL)
			continue;
		negatebase[GET_BIT(src.NegateBase, i)] |= 1 << i;
	}

	_mesa_insert_instructions(s->Program, s->IP, (negatebase[0] ? 1 : 0) + (negatebase[1] ? 1 : 0));
	inst = s->Program->Instructions + s->IP;

	for(i = 0; i <= 1; ++i) {
		if (!negatebase[i])
			continue;

		inst->Opcode = OPCODE_MOV;
		inst->DstReg = dst;
		inst->DstReg.WriteMask = negatebase[i];
		inst->SrcReg[0] = src;
		inst++;
		s->IP++;
	}
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
	struct r500_fragment_program *fp,
	struct r500_fragment_program_external_state *state)
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

static void dump_program(struct r500_fragment_program_code *code);

void r500TranslateFragmentShader(r300ContextPtr r300,
				 struct r500_fragment_program *fp)
{
	struct r500_fragment_program_external_state state;

	build_state(r300, fp, &state);
	if (_mesa_memcmp(&fp->state, &state, sizeof(state))) {
		/* TODO: cache compiled programs */
		fp->translated = GL_FALSE;
		_mesa_memcpy(&fp->state, &state, sizeof(state));
	}

	if (!fp->translated) {
		struct r500_fragment_program_compiler compiler;

		compiler.r300 = r300;
		compiler.fp = fp;
		compiler.code = &fp->code;
		compiler.program = _mesa_clone_program(r300->radeon.glCtx, &fp->mesa_program.Base);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Compiler: Initial program:\n");
			_mesa_print_program(compiler.program);
		}

		insert_WPOS_trailer(&compiler);

		struct radeon_program_transformation transformations[] = {
			{ &transform_TEX, &compiler },
			{ &radeonTransformALU, 0 },
			{ &radeonTransformDeriv, 0 },
			{ &radeonTransformTrigScale, 0 }
		};
		radeonLocalTransform(r300->radeon.glCtx, compiler.program,
			4, transformations);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Compiler: after native rewrite:\n");
			_mesa_print_program(compiler.program);
		}

		struct radeon_nqssadce_descr nqssadce = {
			.Init = &nqssadce_init,
			.IsNativeSwizzle = &is_native_swizzle,
			.BuildSwizzle = &nqssadce_build_swizzle,
			.RewriteDepthOut = GL_TRUE
		};
		radeonNqssaDce(r300->radeon.glCtx, compiler.program, &nqssadce);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			_mesa_printf("Compiler: after NqSSA-DCE:\n");
			_mesa_print_program(compiler.program);
		}

		fp->translated = r500FragmentProgramEmit(&compiler);

		/* Subtle: Rescue any parameters that have been added during transformations */
		_mesa_free_parameter_list(fp->mesa_program.Base.Parameters);
		fp->mesa_program.Base.Parameters = compiler.program->Parameters;
		compiler.program->Parameters = 0;

		_mesa_reference_program(r300->radeon.glCtx, &compiler.program, 0);

		r300UpdateStateParameters(r300->radeon.glCtx, _NEW_PROGRAM);

		if (RADEON_DEBUG & DEBUG_PIXEL) {
			if (fp->translated) {
				_mesa_printf("Machine-readable code:\n");
				dump_program(&fp->code);
			}
		}

	}

	update_params(r300, fp);

}

static char *toswiz(int swiz_val) {
  switch(swiz_val) {
  case 0: return "R";
  case 1: return "G";
  case 2: return "B";
  case 3: return "A";
  case 4: return "0";
  case 5: return "1/2";
  case 6: return "1";
  case 7: return "U";
  }
  return NULL;
}

static char *toop(int op_val)
{
  char *str = NULL;
  switch (op_val) {
  case 0: str = "MAD"; break;
  case 1: str = "DP3"; break;
  case 2: str = "DP4"; break;
  case 3: str = "D2A"; break;
  case 4: str = "MIN"; break;
  case 5: str = "MAX"; break;
  case 6: str = "Reserved"; break;
  case 7: str = "CND"; break;
  case 8: str = "CMP"; break;
  case 9: str = "FRC"; break;
  case 10: str = "SOP"; break;
  case 11: str = "MDH"; break;
  case 12: str = "MDV"; break;
  }
  return str;
}

static char *to_alpha_op(int op_val)
{
  char *str = NULL;
  switch (op_val) {
  case 0: str = "MAD"; break;
  case 1: str = "DP"; break;
  case 2: str = "MIN"; break;
  case 3: str = "MAX"; break;
  case 4: str = "Reserved"; break;
  case 5: str = "CND"; break;
  case 6: str = "CMP"; break;
  case 7: str = "FRC"; break;
  case 8: str = "EX2"; break;
  case 9: str = "LN2"; break;
  case 10: str = "RCP"; break;
  case 11: str = "RSQ"; break;
  case 12: str = "SIN"; break;
  case 13: str = "COS"; break;
  case 14: str = "MDH"; break;
  case 15: str = "MDV"; break;
  }
  return str;
}

static char *to_mask(int val)
{
  char *str = NULL;
  switch(val) {
  case 0: str = "NONE"; break;
  case 1: str = "R"; break;
  case 2: str = "G"; break;
  case 3: str = "RG"; break;
  case 4: str = "B"; break;
  case 5: str = "RB"; break;
  case 6: str = "GB"; break;
  case 7: str = "RGB"; break;
  case 8: str = "A"; break;
  case 9: str = "AR"; break;
  case 10: str = "AG"; break;
  case 11: str = "ARG"; break;
  case 12: str = "AB"; break;
  case 13: str = "ARB"; break;
  case 14: str = "AGB"; break;
  case 15: str = "ARGB"; break;
  }
  return str;
}

static char *to_texop(int val)
{
  switch(val) {
  case 0: return "NOP";
  case 1: return "LD";
  case 2: return "TEXKILL";
  case 3: return "PROJ";
  case 4: return "LODBIAS";
  case 5: return "LOD";
  case 6: return "DXDY";
  }
  return NULL;
}

static void dump_program(struct r500_fragment_program_code *code)
{

  fprintf(stderr, "R500 Fragment Program:\n--------\n");

  int n;
  uint32_t inst;
  uint32_t inst0;
  char *str = NULL;

  if (code->const_nr) {
    fprintf(stderr, "--------\nConstants:\n");
    for (n = 0; n < code->const_nr; n++) {
      fprintf(stderr, "Constant %d: %i[%i]\n", n,
        code->constant[n].File, code->constant[n].Index);
    }
    fprintf(stderr, "--------\n");
  }

  for (n = 0; n < code->inst_end+1; n++) {
    inst0 = inst = code->inst[n].inst0;
    fprintf(stderr,"%d\t0:CMN_INST   0x%08x:", n, inst);
    switch(inst & 0x3) {
    case R500_INST_TYPE_ALU: str = "ALU"; break;
    case R500_INST_TYPE_OUT: str = "OUT"; break;
    case R500_INST_TYPE_FC: str = "FC"; break;
    case R500_INST_TYPE_TEX: str = "TEX"; break;
    };
    fprintf(stderr,"%s %s %s %s %s ", str,
	    inst & R500_INST_TEX_SEM_WAIT ? "TEX_WAIT" : "",
	    inst & R500_INST_LAST ? "LAST" : "",
	    inst & R500_INST_NOP ? "NOP" : "",
	    inst & R500_INST_ALU_WAIT ? "ALU WAIT" : "");
    fprintf(stderr,"wmask: %s omask: %s\n", to_mask((inst >> 11) & 0xf),
	    to_mask((inst >> 15) & 0xf));

    switch(inst0 & 0x3) {
    case 0:
    case 1:
      fprintf(stderr,"\t1:RGB_ADDR   0x%08x:", code->inst[n].inst1);
      inst = code->inst[n].inst1;

      fprintf(stderr,"Addr0: %d%c, Addr1: %d%c, Addr2: %d%c, srcp:%d\n",
	      inst & 0xff, (inst & (1<<8)) ? 'c' : 't',
	      (inst >> 10) & 0xff, (inst & (1<<18)) ? 'c' : 't',
	      (inst >> 20) & 0xff, (inst & (1<<28)) ? 'c' : 't',
	      (inst >> 30));

      fprintf(stderr,"\t2:ALPHA_ADDR 0x%08x:", code->inst[n].inst2);
      inst = code->inst[n].inst2;
      fprintf(stderr,"Addr0: %d%c, Addr1: %d%c, Addr2: %d%c, srcp:%d\n",
	      inst & 0xff, (inst & (1<<8)) ? 'c' : 't',
	      (inst >> 10) & 0xff, (inst & (1<<18)) ? 'c' : 't',
	      (inst >> 20) & 0xff, (inst & (1<<28)) ? 'c' : 't',
	      (inst >> 30));
      fprintf(stderr,"\t3 RGB_INST:  0x%08x:", code->inst[n].inst3);
      inst = code->inst[n].inst3;
      fprintf(stderr,"rgb_A_src:%d %s/%s/%s %d rgb_B_src:%d %s/%s/%s %d\n",
	      (inst) & 0x3, toswiz((inst >> 2) & 0x7), toswiz((inst >> 5) & 0x7), toswiz((inst >> 8) & 0x7),
	      (inst >> 11) & 0x3,
	      (inst >> 13) & 0x3, toswiz((inst >> 15) & 0x7), toswiz((inst >> 18) & 0x7), toswiz((inst >> 21) & 0x7),
	      (inst >> 24) & 0x3);


      fprintf(stderr,"\t4 ALPHA_INST:0x%08x:", code->inst[n].inst4);
      inst = code->inst[n].inst4;
      fprintf(stderr,"%s dest:%d%s alp_A_src:%d %s %d alp_B_src:%d %s %d w:%d\n", to_alpha_op(inst & 0xf),
	      (inst >> 4) & 0x7f, inst & (1<<11) ? "(rel)":"",
	      (inst >> 12) & 0x3, toswiz((inst >> 14) & 0x7), (inst >> 17) & 0x3,
	      (inst >> 19) & 0x3, toswiz((inst >> 21) & 0x7), (inst >> 24) & 0x3,
	      (inst >> 31) & 0x1);

      fprintf(stderr,"\t5 RGBA_INST: 0x%08x:", code->inst[n].inst5);
      inst = code->inst[n].inst5;
      fprintf(stderr,"%s dest:%d%s rgb_C_src:%d %s/%s/%s %d alp_C_src:%d %s %d\n", toop(inst & 0xf),
	      (inst >> 4) & 0x7f, inst & (1<<11) ? "(rel)":"",
	      (inst >> 12) & 0x3, toswiz((inst >> 14) & 0x7), toswiz((inst >> 17) & 0x7), toswiz((inst >> 20) & 0x7),
	      (inst >> 23) & 0x3,
	      (inst >> 25) & 0x3, toswiz((inst >> 27) & 0x7), (inst >> 30) & 0x3);
      break;
    case 2:
      break;
    case 3:
      inst = code->inst[n].inst1;
      fprintf(stderr,"\t1:TEX_INST:  0x%08x: id: %d op:%s, %s, %s %s\n", inst, (inst >> 16) & 0xf,
	      to_texop((inst >> 22) & 0x7), (inst & (1<<25)) ? "ACQ" : "",
	      (inst & (1<<26)) ? "IGNUNC" : "", (inst & (1<<27)) ? "UNSCALED" : "SCALED");
      inst = code->inst[n].inst2;
      fprintf(stderr,"\t2:TEX_ADDR:  0x%08x: src: %d%s %s/%s/%s/%s dst: %d%s %s/%s/%s/%s\n", inst,
	      inst & 127, inst & (1<<7) ? "(rel)" : "",
	      toswiz((inst >> 8) & 0x3), toswiz((inst >> 10) & 0x3),
	      toswiz((inst >> 12) & 0x3), toswiz((inst >> 14) & 0x3),
	      (inst >> 16) & 127, inst & (1<<23) ? "(rel)" : "",
	      toswiz((inst >> 24) & 0x3), toswiz((inst >> 26) & 0x3),
	      toswiz((inst >> 28) & 0x3), toswiz((inst >> 30) & 0x3));

      fprintf(stderr,"\t3:TEX_DXDY:  0x%08x\n", code->inst[n].inst3);
      break;
    }
    fprintf(stderr,"\n");
  }

}
