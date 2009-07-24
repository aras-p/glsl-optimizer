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

#include "../r300_reg.h"

#include "radeon_nqssadce.h"
#include "radeon_program.h"
#include "radeon_program_alu.h"

#include "shader/prog_print.h"


/*
 * Take an already-setup and valid source then swizzle it appropriately to
 * obtain a constant ZERO or ONE source.
 */
#define __CONST(x, y)	\
	(PVS_SRC_OPERAND(t_src_index(vp, &vpi->SrcReg[x]),	\
			   t_swizzle(y),	\
			   t_swizzle(y),	\
			   t_swizzle(y),	\
			   t_swizzle(y),	\
			   t_src_class(vpi->SrcReg[x].File), \
			   NEGATE_NONE) | (vpi->SrcReg[x].RelAddr << 4))


static unsigned long t_dst_mask(GLuint mask)
{
	/* WRITEMASK_* is equivalent to VSF_FLAG_* */
	return mask & WRITEMASK_XYZW;
}

static unsigned long t_dst_class(gl_register_file file)
{

	switch (file) {
	case PROGRAM_TEMPORARY:
		return PVS_DST_REG_TEMPORARY;
	case PROGRAM_OUTPUT:
		return PVS_DST_REG_OUT;
	case PROGRAM_ADDRESS:
		return PVS_DST_REG_A0;
		/*
		   case PROGRAM_INPUT:
		   case PROGRAM_LOCAL_PARAM:
		   case PROGRAM_ENV_PARAM:
		   case PROGRAM_NAMED_PARAM:
		   case PROGRAM_STATE_VAR:
		   case PROGRAM_WRITE_ONLY:
		   case PROGRAM_ADDRESS:
		 */
	default:
		fprintf(stderr, "problem in %s", __FUNCTION__);
		_mesa_exit(-1);
		return -1;
	}
}

static unsigned long t_dst_index(struct r300_vertex_program_code *vp,
				 struct prog_dst_register *dst)
{
	if (dst->File == PROGRAM_OUTPUT)
		return vp->outputs[dst->Index];

	return dst->Index;
}

static unsigned long t_src_class(gl_register_file file)
{
	switch (file) {
	case PROGRAM_TEMPORARY:
		return PVS_SRC_REG_TEMPORARY;
	case PROGRAM_INPUT:
		return PVS_SRC_REG_INPUT;
	case PROGRAM_LOCAL_PARAM:
	case PROGRAM_ENV_PARAM:
	case PROGRAM_NAMED_PARAM:
	case PROGRAM_CONSTANT:
	case PROGRAM_STATE_VAR:
		return PVS_SRC_REG_CONSTANT;
		/*
		   case PROGRAM_OUTPUT:
		   case PROGRAM_WRITE_ONLY:
		   case PROGRAM_ADDRESS:
		 */
	default:
		fprintf(stderr, "problem in %s", __FUNCTION__);
		_mesa_exit(-1);
		return -1;
	}
}

static GLboolean t_src_conflict(struct prog_src_register a, struct prog_src_register b)
{
	unsigned long aclass = t_src_class(a.File);
	unsigned long bclass = t_src_class(b.File);

	if (aclass != bclass)
		return GL_FALSE;
	if (aclass == PVS_SRC_REG_TEMPORARY)
		return GL_FALSE;

	if (a.RelAddr || b.RelAddr)
		return GL_TRUE;
	if (a.Index != b.Index)
		return GL_TRUE;

	return GL_FALSE;
}

static INLINE unsigned long t_swizzle(GLubyte swizzle)
{
	/* this is in fact a NOP as the Mesa SWIZZLE_* are all identical to VSF_IN_COMPONENT_* */
	return swizzle;
}

static unsigned long t_src_index(struct r300_vertex_program_code *vp,
				 struct prog_src_register *src)
{
	if (src->File == PROGRAM_INPUT) {
		assert(vp->inputs[src->Index] != -1);
		return vp->inputs[src->Index];
	} else {
		if (src->Index < 0) {
			fprintf(stderr,
				"negative offsets for indirect addressing do not work.\n");
			return 0;
		}
		return src->Index;
	}
}

/* these two functions should probably be merged... */

static unsigned long t_src(struct r300_vertex_program_code *vp,
			   struct prog_src_register *src)
{
	/* src->Negate uses the NEGATE_ flags from program_instruction.h,
	 * which equal our VSF_FLAGS_ values, so it's safe to just pass it here.
	 */
	return PVS_SRC_OPERAND(t_src_index(vp, src),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 1)),
			       t_swizzle(GET_SWZ(src->Swizzle, 2)),
			       t_swizzle(GET_SWZ(src->Swizzle, 3)),
			       t_src_class(src->File),
			       src->Negate) | (src->RelAddr << 4);
}

static unsigned long t_src_scalar(struct r300_vertex_program_code *vp,
				  struct prog_src_register *src)
{
	/* src->Negate uses the NEGATE_ flags from program_instruction.h,
	 * which equal our VSF_FLAGS_ values, so it's safe to just pass it here.
	 */
	return PVS_SRC_OPERAND(t_src_index(vp, src),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_src_class(src->File),
			       src->Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (src->RelAddr << 4);
}

static GLboolean valid_dst(struct r300_vertex_program_code *vp,
			   struct prog_dst_register *dst)
{
	if (dst->File == PROGRAM_OUTPUT && vp->outputs[dst->Index] == -1) {
		return GL_FALSE;
	} else if (dst->File == PROGRAM_ADDRESS) {
		assert(dst->Index == 0);
	}

	return GL_TRUE;
}

static void ei_vector1(struct r300_vertex_program_code *vp,
				GLuint hw_opcode,
				struct prog_instruction *vpi,
				GLuint * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(hw_opcode,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &vpi->SrcReg[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);
}

static void ei_vector2(struct r300_vertex_program_code *vp,
				GLuint hw_opcode,
				struct prog_instruction *vpi,
				GLuint * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(hw_opcode,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &vpi->SrcReg[0]);
	inst[2] = t_src(vp, &vpi->SrcReg[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);
}

static void ei_math1(struct r300_vertex_program_code *vp,
				GLuint hw_opcode,
				struct prog_instruction *vpi,
				GLuint * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(hw_opcode,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &vpi->SrcReg[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);
}

static void ei_lit(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst)
{
	//LIT TMP 1.Y Z TMP 1{} {X W Z Y} TMP 1{} {Y W Z X} TMP 1{} {Y X Z W}

	inst[0] = PVS_OP_DST_OPERAND(ME_LIGHT_COEFF_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	/* NOTE: Users swizzling might not work. */
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &vpi->SrcReg[0]), t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 1)),	// Y
				  t_src_class(vpi->SrcReg[0].File),
				  vpi->SrcReg[0].Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (vpi->SrcReg[0].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &vpi->SrcReg[0]), t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 0)),	// X
				  t_src_class(vpi->SrcReg[0].File),
				  vpi->SrcReg[0].Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (vpi->SrcReg[0].RelAddr << 4);
	inst[3] = PVS_SRC_OPERAND(t_src_index(vp, &vpi->SrcReg[0]), t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 0)),	// X
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 3)),	// W
				  t_src_class(vpi->SrcReg[0].File),
				  vpi->SrcReg[0].Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (vpi->SrcReg[0].RelAddr << 4);
}

static void ei_mad(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(PVS_MACRO_OP_2CLK_MADD,
				     GL_FALSE,
				     GL_TRUE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &vpi->SrcReg[0]);
	inst[2] = t_src(vp, &vpi->SrcReg[1]);
	inst[3] = t_src(vp, &vpi->SrcReg[2]);
}

static void ei_pow(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(ME_POWER_FUNC_FF,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &vpi->SrcReg[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = t_src_scalar(vp, &vpi->SrcReg[1]);
}

static void t_inputs_outputs(struct r300_vertex_program_compiler * c)
{
	int i;
	int cur_reg;
	GLuint OutputsWritten, InputsRead;

	OutputsWritten = c->Base.Program.OutputsWritten;
	InputsRead = c->Base.Program.InputsRead;

	cur_reg = -1;
	for (i = 0; i < VERT_ATTRIB_MAX; i++) {
		if (InputsRead & (1 << i))
			c->code->inputs[i] = ++cur_reg;
		else
			c->code->inputs[i] = -1;
	}

	cur_reg = 0;
	for (i = 0; i < VERT_RESULT_MAX; i++)
		c->code->outputs[i] = -1;

	assert(OutputsWritten & (1 << VERT_RESULT_HPOS));

	if (OutputsWritten & (1 << VERT_RESULT_HPOS)) {
		c->code->outputs[VERT_RESULT_HPOS] = cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_PSIZ)) {
		c->code->outputs[VERT_RESULT_PSIZ] = cur_reg++;
	}

	/* If we're writing back facing colors we need to send
	 * four colors to make front/back face colors selection work.
	 * If the vertex program doesn't write all 4 colors, lets
	 * pretend it does by skipping output index reg so the colors
	 * get written into appropriate output vectors.
	 */
	if (OutputsWritten & (1 << VERT_RESULT_COL0)) {
		c->code->outputs[VERT_RESULT_COL0] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0) ||
		OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_COL1)) {
		c->code->outputs[VERT_RESULT_COL1] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0) ||
		OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_BFC0)) {
		c->code->outputs[VERT_RESULT_BFC0] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		c->code->outputs[VERT_RESULT_BFC1] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0)) {
		cur_reg++;
	}

	for (i = VERT_RESULT_TEX0; i <= VERT_RESULT_TEX7; i++) {
		if (OutputsWritten & (1 << i)) {
			c->code->outputs[i] = cur_reg++;
		}
	}

	if (OutputsWritten & (1 << VERT_RESULT_FOGC)) {
		c->code->outputs[VERT_RESULT_FOGC] = cur_reg++;
	}
}

static void translate_vertex_program(struct r300_vertex_program_compiler * compiler)
{
	struct rc_instruction *rci;

	compiler->code->pos_end = 0;	/* Not supported yet */
	compiler->code->length = 0;

	t_inputs_outputs(compiler);

	for(rci = compiler->Base.Program.Instructions.Next; rci != &compiler->Base.Program.Instructions; rci = rci->Next) {
		struct prog_instruction *vpi = &rci->I;
		GLuint *inst = compiler->code->body.d + compiler->code->length;

		/* Skip instructions writing to non-existing destination */
		if (!valid_dst(compiler->code, &vpi->DstReg))
			continue;

		if (compiler->code->length >= VSF_MAX_FRAGMENT_LENGTH) {
			rc_error(&compiler->Base, "Vertex program has too many instructions\n");
			return;
		}

		switch (vpi->Opcode) {
		case OPCODE_ADD: ei_vector2(compiler->code, VE_ADD, vpi, inst); break;
		case OPCODE_ARL: ei_vector1(compiler->code, VE_FLT2FIX_DX, vpi, inst); break;
		case OPCODE_DP4: ei_vector2(compiler->code, VE_DOT_PRODUCT, vpi, inst); break;
		case OPCODE_DST: ei_vector2(compiler->code, VE_DISTANCE_VECTOR, vpi, inst); break;
		case OPCODE_EX2: ei_math1(compiler->code, ME_EXP_BASE2_FULL_DX, vpi, inst); break;
		case OPCODE_EXP: ei_math1(compiler->code, ME_EXP_BASE2_DX, vpi, inst); break;
		case OPCODE_FRC: ei_vector1(compiler->code, VE_FRACTION, vpi, inst); break;
		case OPCODE_LG2: ei_math1(compiler->code, ME_LOG_BASE2_FULL_DX, vpi, inst); break;
		case OPCODE_LIT: ei_lit(compiler->code, vpi, inst); break;
		case OPCODE_LOG: ei_math1(compiler->code, ME_LOG_BASE2_DX, vpi, inst); break;
		case OPCODE_MAD: ei_mad(compiler->code, vpi, inst); break;
		case OPCODE_MAX: ei_vector2(compiler->code, VE_MAXIMUM, vpi, inst); break;
		case OPCODE_MIN: ei_vector2(compiler->code, VE_MINIMUM, vpi, inst); break;
		case OPCODE_MOV: ei_vector1(compiler->code, VE_ADD, vpi, inst); break;
		case OPCODE_MUL: ei_vector2(compiler->code, VE_MULTIPLY, vpi, inst); break;
		case OPCODE_POW: ei_pow(compiler->code, vpi, inst); break;
		case OPCODE_RCP: ei_math1(compiler->code, ME_RECIP_DX, vpi, inst); break;
		case OPCODE_RSQ: ei_math1(compiler->code, ME_RECIP_SQRT_DX, vpi, inst); break;
		case OPCODE_SGE: ei_vector2(compiler->code, VE_SET_GREATER_THAN_EQUAL, vpi, inst); break;
		case OPCODE_SLT: ei_vector2(compiler->code, VE_SET_LESS_THAN, vpi, inst); break;
		default:
			rc_error(&compiler->Base, "Unknown opcode %i\n", vpi->Opcode);
			return;
		}

		compiler->code->length += 4;

		if (compiler->Base.Error)
			return;
	}
}

struct temporary_allocation {
	GLuint Allocated:1;
	GLuint HwTemp:15;
	struct rc_instruction * LastRead;
};

static void allocate_temporary_registers(struct r300_vertex_program_compiler * compiler)
{
	struct rc_instruction *inst;
	GLuint num_orig_temps = 0;
	GLboolean hwtemps[VSF_MAX_FRAGMENT_TEMPS];
	struct temporary_allocation * ta;
	GLuint i, j;

	compiler->code->num_temporaries = 0;
	memset(hwtemps, 0, sizeof(hwtemps));

	/* Pass 1: Count original temporaries and allocate structures */
	for(inst = compiler->Base.Program.Instructions.Next; inst != &compiler->Base.Program.Instructions; inst = inst->Next) {
		GLuint numsrcs = _mesa_num_inst_src_regs(inst->I.Opcode);
		GLuint numdsts = _mesa_num_inst_dst_regs(inst->I.Opcode);

		for (i = 0; i < numsrcs; ++i) {
			if (inst->I.SrcReg[i].File == PROGRAM_TEMPORARY) {
				if (inst->I.SrcReg[i].Index >= num_orig_temps)
					num_orig_temps = inst->I.SrcReg[i].Index + 1;
			}
		}

		if (numdsts) {
			if (inst->I.DstReg.File == PROGRAM_TEMPORARY) {
				if (inst->I.DstReg.Index >= num_orig_temps)
					num_orig_temps = inst->I.DstReg.Index + 1;
			}
		}
	}

	ta = (struct temporary_allocation*)memory_pool_malloc(&compiler->Base.Pool,
			sizeof(struct temporary_allocation) * num_orig_temps);
	memset(ta, 0, sizeof(struct temporary_allocation) * num_orig_temps);

	/* Pass 2: Determine original temporary lifetimes */
	for(inst = compiler->Base.Program.Instructions.Next; inst != &compiler->Base.Program.Instructions; inst = inst->Next) {
		GLuint numsrcs = _mesa_num_inst_src_regs(inst->I.Opcode);

		for (i = 0; i < numsrcs; ++i) {
			if (inst->I.SrcReg[i].File == PROGRAM_TEMPORARY)
				ta[inst->I.SrcReg[i].Index].LastRead = inst;
		}
	}

	/* Pass 3: Register allocation */
	for(inst = compiler->Base.Program.Instructions.Next; inst != &compiler->Base.Program.Instructions; inst = inst->Next) {
		GLuint numsrcs = _mesa_num_inst_src_regs(inst->I.Opcode);
		GLuint numdsts = _mesa_num_inst_dst_regs(inst->I.Opcode);

		for (i = 0; i < numsrcs; ++i) {
			if (inst->I.SrcReg[i].File == PROGRAM_TEMPORARY) {
				GLuint orig = inst->I.SrcReg[i].Index;
				inst->I.SrcReg[i].Index = ta[orig].HwTemp;

				if (ta[orig].Allocated && inst == ta[orig].LastRead)
					hwtemps[ta[orig].HwTemp] = GL_FALSE;
			}
		}

		if (numdsts) {
			if (inst->I.DstReg.File == PROGRAM_TEMPORARY) {
				GLuint orig = inst->I.DstReg.Index;

				if (!ta[orig].Allocated) {
					for(j = 0; j < VSF_MAX_FRAGMENT_TEMPS; ++j) {
						if (!hwtemps[j])
							break;
					}
					if (j >= VSF_MAX_FRAGMENT_TEMPS) {
						fprintf(stderr, "Out of hw temporaries\n");
					} else {
						ta[orig].Allocated = GL_TRUE;
						ta[orig].HwTemp = j;
						hwtemps[j] = GL_TRUE;

						if (j >= compiler->code->num_temporaries)
							compiler->code->num_temporaries = j + 1;
					}
				}

				inst->I.DstReg.Index = ta[orig].HwTemp;
			}
		}
	}
}


/**
 * Vertex engine cannot read two inputs or two constants at the same time.
 * Introduce intermediate MOVs to temporary registers to account for this.
 */
static GLboolean transform_source_conflicts(
	struct radeon_transform_context *t,
	struct prog_instruction* orig_inst,
	void* unused)
{
	struct prog_instruction inst = *orig_inst;
	struct prog_instruction * dst;
	GLuint num_operands = _mesa_num_inst_src_regs(inst.Opcode);

	if (num_operands == 3) {
		if (t_src_conflict(inst.SrcReg[1], inst.SrcReg[2])
		    || t_src_conflict(inst.SrcReg[0], inst.SrcReg[2])) {
			int tmpreg = radeonFindFreeTemporary(t);
			struct prog_instruction * inst_mov = radeonAppendInstructions(t->Program, 1);
			inst_mov->Opcode = OPCODE_MOV;
			inst_mov->DstReg.File = PROGRAM_TEMPORARY;
			inst_mov->DstReg.Index = tmpreg;
			inst_mov->SrcReg[0] = inst.SrcReg[2];

			reset_srcreg(&inst.SrcReg[2]);
			inst.SrcReg[2].File = PROGRAM_TEMPORARY;
			inst.SrcReg[2].Index = tmpreg;
		}
	}

	if (num_operands >= 2) {
		if (t_src_conflict(inst.SrcReg[1], inst.SrcReg[0])) {
			int tmpreg = radeonFindFreeTemporary(t);
			struct prog_instruction * inst_mov = radeonAppendInstructions(t->Program, 1);
			inst_mov->Opcode = OPCODE_MOV;
			inst_mov->DstReg.File = PROGRAM_TEMPORARY;
			inst_mov->DstReg.Index = tmpreg;
			inst_mov->SrcReg[0] = inst.SrcReg[1];

			reset_srcreg(&inst.SrcReg[1]);
			inst.SrcReg[1].File = PROGRAM_TEMPORARY;
			inst.SrcReg[1].Index = tmpreg;
		}
	}

	dst = radeonAppendInstructions(t->Program, 1);
	*dst = inst;
	return GL_TRUE;
}

static void insert_wpos(struct gl_program *prog, GLuint temp_index, int tex_id)
{
	struct prog_instruction *vpi;

	_mesa_insert_instructions(prog, prog->NumInstructions - 1, 2);

	vpi = &prog->Instructions[prog->NumInstructions - 3];

	vpi->Opcode = OPCODE_MOV;

	vpi->DstReg.File = PROGRAM_OUTPUT;
	vpi->DstReg.Index = VERT_RESULT_HPOS;
	vpi->DstReg.WriteMask = WRITEMASK_XYZW;
	vpi->DstReg.CondMask = COND_TR;

	vpi->SrcReg[0].File = PROGRAM_TEMPORARY;
	vpi->SrcReg[0].Index = temp_index;
	vpi->SrcReg[0].Swizzle = SWIZZLE_XYZW;

	++vpi;

	vpi->Opcode = OPCODE_MOV;

	vpi->DstReg.File = PROGRAM_OUTPUT;
	vpi->DstReg.Index = VERT_RESULT_TEX0 + tex_id;
	vpi->DstReg.WriteMask = WRITEMASK_XYZW;
	vpi->DstReg.CondMask = COND_TR;

	vpi->SrcReg[0].File = PROGRAM_TEMPORARY;
	vpi->SrcReg[0].Index = temp_index;
	vpi->SrcReg[0].Swizzle = SWIZZLE_XYZW;

	++vpi;

	vpi->Opcode = OPCODE_END;
}

static void pos_as_texcoord(struct gl_program *prog, int tex_id)
{
	struct prog_instruction *vpi;
	GLuint tempregi = prog->NumTemporaries;

	prog->NumTemporaries++;

	for (vpi = prog->Instructions; vpi->Opcode != OPCODE_END; vpi++) {
		if (vpi->DstReg.File == PROGRAM_OUTPUT && vpi->DstReg.Index == VERT_RESULT_HPOS) {
			vpi->DstReg.File = PROGRAM_TEMPORARY;
			vpi->DstReg.Index = tempregi;
		}
	}

	insert_wpos(prog, tempregi, tex_id);

	prog->OutputsWritten |= 1 << (VERT_RESULT_TEX0 + tex_id);
}

/**
 * The fogcoord attribute is special in that only the first component
 * is relevant, and the remaining components are always fixed (when read
 * from by the fragment program) to yield an X001 pattern.
 *
 * We need to enforce this either in the vertex program or in the fragment
 * program, and this code chooses not to enforce it in the vertex program.
 * This is slightly cheaper, as long as the fragment program does not use
 * weird swizzles.
 *
 * And it seems that usually, weird swizzles are not used, so...
 *
 * See also the counterpart rewriting for fragment programs.
 */
static void fog_as_texcoord(struct gl_program *prog, int tex_id)
{
	struct prog_instruction *vpi;

	vpi = prog->Instructions;
	while (vpi->Opcode != OPCODE_END) {
		if (vpi->DstReg.File == PROGRAM_OUTPUT && vpi->DstReg.Index == VERT_RESULT_FOGC) {
			vpi->DstReg.Index = VERT_RESULT_TEX0 + tex_id;
			vpi->DstReg.WriteMask = WRITEMASK_X;
		}

		++vpi;
	}

	prog->OutputsWritten &= ~(1 << VERT_RESULT_FOGC);
	prog->OutputsWritten |= 1 << (VERT_RESULT_TEX0 + tex_id);
}


#define ADD_OUTPUT(fp_attr, vp_result) \
	do { \
		if ((FpReads & (1 << (fp_attr))) && !(compiler->program->OutputsWritten & (1 << (vp_result)))) { \
			OutputsAdded |= 1 << (vp_result); \
			count++; \
		} \
	} while (0)

static void addArtificialOutputs(struct r300_vertex_program_compiler * compiler)
{
	GLuint OutputsAdded, FpReads;
	int i, count;

	OutputsAdded = 0;
	count = 0;
	FpReads = compiler->state.FpReads;

	ADD_OUTPUT(FRAG_ATTRIB_COL0, VERT_RESULT_COL0);
	ADD_OUTPUT(FRAG_ATTRIB_COL1, VERT_RESULT_COL1);

	for (i = 0; i < 7; ++i) {
		ADD_OUTPUT(FRAG_ATTRIB_TEX0 + i, VERT_RESULT_TEX0 + i);
	}

	/* Some outputs may be artificially added, to match the inputs of the fragment program.
	 * Issue 16 of vertex program spec says that all vertex attributes that are unwritten by
	 * vertex program are undefined, so just use MOV [vertex_result], CONST[0]
	 */
	if (count > 0) {
		struct prog_instruction *inst;

		_mesa_insert_instructions(compiler->program, compiler->program->NumInstructions - 1, count);
		inst = &compiler->program->Instructions[compiler->program->NumInstructions - 1 - count];

		for (i = 0; i < VERT_RESULT_MAX; ++i) {
			if (OutputsAdded & (1 << i)) {
				inst->Opcode = OPCODE_MOV;

				inst->DstReg.File = PROGRAM_OUTPUT;
				inst->DstReg.Index = i;
				inst->DstReg.WriteMask = WRITEMASK_XYZW;
				inst->DstReg.CondMask = COND_TR;

				inst->SrcReg[0].File = PROGRAM_CONSTANT;
				inst->SrcReg[0].Index = 0;
				inst->SrcReg[0].Swizzle = SWIZZLE_XYZW;

				++inst;
			}
		}

		compiler->program->OutputsWritten |= OutputsAdded;
	}
}

#undef ADD_OUTPUT

static void nqssadceInit(struct nqssadce_state* s)
{
	struct r300_vertex_program_compiler * compiler = s->UserData;
	GLuint fp_reads;

	fp_reads = compiler->state.FpReads;
	{
		if (fp_reads & FRAG_BIT_COL0) {
				s->Outputs[VERT_RESULT_COL0].Sourced = WRITEMASK_XYZW;
				s->Outputs[VERT_RESULT_BFC0].Sourced = WRITEMASK_XYZW;
		}

		if (fp_reads & FRAG_BIT_COL1) {
				s->Outputs[VERT_RESULT_COL1].Sourced = WRITEMASK_XYZW;
				s->Outputs[VERT_RESULT_BFC1].Sourced = WRITEMASK_XYZW;
		}
	}

	{
		int i;
		for (i = 0; i < 8; ++i) {
			if (fp_reads & FRAG_BIT_TEX(i)) {
				s->Outputs[VERT_RESULT_TEX0 + i].Sourced = WRITEMASK_XYZW;
			}
		}
	}

	s->Outputs[VERT_RESULT_HPOS].Sourced = WRITEMASK_XYZW;
	if (s->Compiler->Program.OutputsWritten & (1 << VERT_RESULT_PSIZ))
		s->Outputs[VERT_RESULT_PSIZ].Sourced = WRITEMASK_X;
}

static GLboolean swizzleIsNative(GLuint opcode, struct prog_src_register reg)
{
	(void) opcode;
	(void) reg;

	return GL_TRUE;
}



void r3xx_compile_vertex_program(struct r300_vertex_program_compiler* compiler)
{
	if (compiler->state.WPosAttr != FRAG_ATTRIB_MAX) {
		pos_as_texcoord(compiler->program, compiler->state.WPosAttr - FRAG_ATTRIB_TEX0);
	}

	if (compiler->state.FogAttr != FRAG_ATTRIB_MAX) {
		fog_as_texcoord(compiler->program, compiler->state.FogAttr - FRAG_ATTRIB_TEX0);
	}

	addArtificialOutputs(compiler);

	{
		struct radeon_program_transformation transformations[] = {
			{ &r300_transform_vertex_alu, 0 },
		};
		radeonLocalTransform(compiler->program, 1, transformations);
	}

	if (compiler->Base.Debug) {
		fprintf(stderr, "Vertex program after native rewrite:\n");
		_mesa_print_program(compiler->program);
		fflush(stdout);
	}

	{
		/* Note: This pass has to be done seperately from ALU rewrite,
		 * otherwise non-native ALU instructions with source conflits
		 * will not be treated properly.
		 */
		struct radeon_program_transformation transformations[] = {
			{ &transform_source_conflicts, 0 },
		};
		radeonLocalTransform(compiler->program, 1, transformations);
	}

	if (compiler->Base.Debug) {
		fprintf(stderr, "Vertex program after source conflict resolve:\n");
		_mesa_print_program(compiler->program);
		fflush(stdout);
	}

	rc_mesa_to_rc_program(&compiler->Base, compiler->program);

	{
		struct radeon_nqssadce_descr nqssadce = {
			.Init = &nqssadceInit,
			.IsNativeSwizzle = &swizzleIsNative,
			.BuildSwizzle = NULL
		};
		radeonNqssaDce(&compiler->Base, &nqssadce, compiler);

		/* We need this step for reusing temporary registers */
		allocate_temporary_registers(compiler);

		if (compiler->Base.Debug) {
			fprintf(stderr, "Vertex program after NQSSADCE:\n");
			rc_print_program(&compiler->Base.Program);
			fflush(stdout);
		}
	}

	translate_vertex_program(compiler);

	rc_constants_copy(&compiler->code->constants, &compiler->Base.Program.Constants);

	compiler->code->InputsRead = compiler->Base.Program.InputsRead;
	compiler->code->OutputsWritten = compiler->Base.Program.OutputsWritten;
}
