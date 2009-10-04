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

#include "../r300_reg.h"

#include "radeon_dataflow.h"
#include "radeon_program_alu.h"
#include "radeon_swizzle.h"


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
			   RC_MASK_NONE) | (vpi->SrcReg[x].RelAddr << 4))


static unsigned long t_dst_mask(unsigned int mask)
{
	/* RC_MASK_* is equivalent to VSF_FLAG_* */
	return mask & RC_MASK_XYZW;
}

static unsigned long t_dst_class(rc_register_file file)
{
	switch (file) {
	default:
		fprintf(stderr, "%s: Bad register file %i\n", __FUNCTION__, file);
		/* fall-through */
	case RC_FILE_TEMPORARY:
		return PVS_DST_REG_TEMPORARY;
	case RC_FILE_OUTPUT:
		return PVS_DST_REG_OUT;
	case RC_FILE_ADDRESS:
		return PVS_DST_REG_A0;
	}
}

static unsigned long t_dst_index(struct r300_vertex_program_code *vp,
				 struct rc_dst_register *dst)
{
	if (dst->File == RC_FILE_OUTPUT)
		return vp->outputs[dst->Index];

	return dst->Index;
}

static unsigned long t_src_class(rc_register_file file)
{
	switch (file) {
	default:
		fprintf(stderr, "%s: Bad register file %i\n", __FUNCTION__, file);
		/* fall-through */
	case RC_FILE_NONE:
	case RC_FILE_TEMPORARY:
		return PVS_SRC_REG_TEMPORARY;
	case RC_FILE_INPUT:
		return PVS_SRC_REG_INPUT;
	case RC_FILE_CONSTANT:
		return PVS_SRC_REG_CONSTANT;
	}
}

static int t_src_conflict(struct rc_src_register a, struct rc_src_register b)
{
	unsigned long aclass = t_src_class(a.File);
	unsigned long bclass = t_src_class(b.File);

	if (aclass != bclass)
		return 0;
	if (aclass == PVS_SRC_REG_TEMPORARY)
		return 0;

	if (a.RelAddr || b.RelAddr)
		return 1;
	if (a.Index != b.Index)
		return 1;

	return 0;
}

static inline unsigned long t_swizzle(unsigned int swizzle)
{
	/* this is in fact a NOP as the Mesa RC_SWIZZLE_* are all identical to VSF_IN_COMPONENT_* */
	return swizzle;
}

static unsigned long t_src_index(struct r300_vertex_program_code *vp,
				 struct rc_src_register *src)
{
	if (src->File == RC_FILE_INPUT) {
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
			   struct rc_src_register *src)
{
	/* src->Negate uses the RC_MASK_ flags from program_instruction.h,
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
				  struct rc_src_register *src)
{
	/* src->Negate uses the RC_MASK_ flags from program_instruction.h,
	 * which equal our VSF_FLAGS_ values, so it's safe to just pass it here.
	 */
	return PVS_SRC_OPERAND(t_src_index(vp, src),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_src_class(src->File),
			       src->Negate ? RC_MASK_XYZW : RC_MASK_NONE) |
	    (src->RelAddr << 4);
}

static int valid_dst(struct r300_vertex_program_code *vp,
			   struct rc_dst_register *dst)
{
	if (dst->File == RC_FILE_OUTPUT && vp->outputs[dst->Index] == -1) {
		return 0;
	} else if (dst->File == RC_FILE_ADDRESS) {
		assert(dst->Index == 0);
	}

	return 1;
}

static void ei_vector1(struct r300_vertex_program_code *vp,
				unsigned int hw_opcode,
				struct rc_sub_instruction *vpi,
				unsigned int * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(hw_opcode,
				     0,
				     0,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &vpi->SrcReg[0]);
	inst[2] = __CONST(0, RC_SWIZZLE_ZERO);
	inst[3] = __CONST(0, RC_SWIZZLE_ZERO);
}

static void ei_vector2(struct r300_vertex_program_code *vp,
				unsigned int hw_opcode,
				struct rc_sub_instruction *vpi,
				unsigned int * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(hw_opcode,
				     0,
				     0,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &vpi->SrcReg[0]);
	inst[2] = t_src(vp, &vpi->SrcReg[1]);
	inst[3] = __CONST(1, RC_SWIZZLE_ZERO);
}

static void ei_math1(struct r300_vertex_program_code *vp,
				unsigned int hw_opcode,
				struct rc_sub_instruction *vpi,
				unsigned int * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(hw_opcode,
				     1,
				     0,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &vpi->SrcReg[0]);
	inst[2] = __CONST(0, RC_SWIZZLE_ZERO);
	inst[3] = __CONST(0, RC_SWIZZLE_ZERO);
}

static void ei_lit(struct r300_vertex_program_code *vp,
				      struct rc_sub_instruction *vpi,
				      unsigned int * inst)
{
	//LIT TMP 1.Y Z TMP 1{} {X W Z Y} TMP 1{} {Y W Z X} TMP 1{} {Y X Z W}

	inst[0] = PVS_OP_DST_OPERAND(ME_LIGHT_COEFF_DX,
				     1,
				     0,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	/* NOTE: Users swizzling might not work. */
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &vpi->SrcReg[0]), t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 1)),	// Y
				  t_src_class(vpi->SrcReg[0].File),
				  vpi->SrcReg[0].Negate ? RC_MASK_XYZW : RC_MASK_NONE) |
	    (vpi->SrcReg[0].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &vpi->SrcReg[0]), t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 0)),	// X
				  t_src_class(vpi->SrcReg[0].File),
				  vpi->SrcReg[0].Negate ? RC_MASK_XYZW : RC_MASK_NONE) |
	    (vpi->SrcReg[0].RelAddr << 4);
	inst[3] = PVS_SRC_OPERAND(t_src_index(vp, &vpi->SrcReg[0]), t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 0)),	// X
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(vpi->SrcReg[0].Swizzle, 3)),	// W
				  t_src_class(vpi->SrcReg[0].File),
				  vpi->SrcReg[0].Negate ? RC_MASK_XYZW : RC_MASK_NONE) |
	    (vpi->SrcReg[0].RelAddr << 4);
}

static void ei_mad(struct r300_vertex_program_code *vp,
				      struct rc_sub_instruction *vpi,
				      unsigned int * inst)
{
	/* Remarks about hardware limitations of MAD
	 * (please preserve this comment, as this information is _NOT_
	 * in the documentation provided by AMD).
	 *
	 * As described in the documentation, MAD with three unique temporary
	 * source registers requires the use of the macro version.
	 *
	 * However (and this is not mentioned in the documentation), apparently
	 * the macro version is _NOT_ a full superset of the normal version.
	 * In particular, the macro version does not always work when relative
	 * addressing is used in the source operands.
	 *
	 * This limitation caused incorrect rendering in Sauerbraten's OpenGL
	 * assembly shader path when using medium quality animations
	 * (i.e. animations with matrix blending instead of quaternion blending).
	 *
	 * Unfortunately, I (nha) have been unable to extract a Piglit regression
	 * test for this issue - for some reason, it is possible to have vertex
	 * programs whose prefix is *exactly* the same as the prefix of the
	 * offending program in Sauerbraten up to the offending instruction
	 * without causing any trouble.
	 *
	 * Bottom line: Only use the macro version only when really necessary;
	 * according to AMD docs, this should improve performance by one clock
	 * as a nice side bonus.
	 */
	if (vpi->SrcReg[0].File == RC_FILE_TEMPORARY &&
	    vpi->SrcReg[1].File == RC_FILE_TEMPORARY &&
	    vpi->SrcReg[2].File == RC_FILE_TEMPORARY &&
	    vpi->SrcReg[0].Index != vpi->SrcReg[1].Index &&
	    vpi->SrcReg[0].Index != vpi->SrcReg[2].Index &&
	    vpi->SrcReg[1].Index != vpi->SrcReg[2].Index) {
		inst[0] = PVS_OP_DST_OPERAND(PVS_MACRO_OP_2CLK_MADD,
				0,
				1,
				t_dst_index(vp, &vpi->DstReg),
				t_dst_mask(vpi->DstReg.WriteMask),
				t_dst_class(vpi->DstReg.File));
	} else {
		inst[0] = PVS_OP_DST_OPERAND(VE_MULTIPLY_ADD,
				0,
				0,
				t_dst_index(vp, &vpi->DstReg),
				t_dst_mask(vpi->DstReg.WriteMask),
				t_dst_class(vpi->DstReg.File));
	}
	inst[1] = t_src(vp, &vpi->SrcReg[0]);
	inst[2] = t_src(vp, &vpi->SrcReg[1]);
	inst[3] = t_src(vp, &vpi->SrcReg[2]);
}

static void ei_pow(struct r300_vertex_program_code *vp,
				      struct rc_sub_instruction *vpi,
				      unsigned int * inst)
{
	inst[0] = PVS_OP_DST_OPERAND(ME_POWER_FUNC_FF,
				     1,
				     0,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &vpi->SrcReg[0]);
	inst[2] = __CONST(0, RC_SWIZZLE_ZERO);
	inst[3] = t_src_scalar(vp, &vpi->SrcReg[1]);
}


static void translate_vertex_program(struct r300_vertex_program_compiler * compiler)
{
	struct rc_instruction *rci;

	compiler->code->pos_end = 0;	/* Not supported yet */
	compiler->code->length = 0;

	compiler->SetHwInputOutput(compiler);

	for(rci = compiler->Base.Program.Instructions.Next; rci != &compiler->Base.Program.Instructions; rci = rci->Next) {
		struct rc_sub_instruction *vpi = &rci->U.I;
		unsigned int *inst = compiler->code->body.d + compiler->code->length;

		/* Skip instructions writing to non-existing destination */
		if (!valid_dst(compiler->code, &vpi->DstReg))
			continue;

		if (compiler->code->length >= VSF_MAX_FRAGMENT_LENGTH) {
			rc_error(&compiler->Base, "Vertex program has too many instructions\n");
			return;
		}

		switch (vpi->Opcode) {
		case RC_OPCODE_ADD: ei_vector2(compiler->code, VE_ADD, vpi, inst); break;
		case RC_OPCODE_ARL: ei_vector1(compiler->code, VE_FLT2FIX_DX, vpi, inst); break;
		case RC_OPCODE_DP4: ei_vector2(compiler->code, VE_DOT_PRODUCT, vpi, inst); break;
		case RC_OPCODE_DST: ei_vector2(compiler->code, VE_DISTANCE_VECTOR, vpi, inst); break;
		case RC_OPCODE_EX2: ei_math1(compiler->code, ME_EXP_BASE2_FULL_DX, vpi, inst); break;
		case RC_OPCODE_EXP: ei_math1(compiler->code, ME_EXP_BASE2_DX, vpi, inst); break;
		case RC_OPCODE_FRC: ei_vector1(compiler->code, VE_FRACTION, vpi, inst); break;
		case RC_OPCODE_LG2: ei_math1(compiler->code, ME_LOG_BASE2_FULL_DX, vpi, inst); break;
		case RC_OPCODE_LIT: ei_lit(compiler->code, vpi, inst); break;
		case RC_OPCODE_LOG: ei_math1(compiler->code, ME_LOG_BASE2_DX, vpi, inst); break;
		case RC_OPCODE_MAD: ei_mad(compiler->code, vpi, inst); break;
		case RC_OPCODE_MAX: ei_vector2(compiler->code, VE_MAXIMUM, vpi, inst); break;
		case RC_OPCODE_MIN: ei_vector2(compiler->code, VE_MINIMUM, vpi, inst); break;
		case RC_OPCODE_MOV: ei_vector1(compiler->code, VE_ADD, vpi, inst); break;
		case RC_OPCODE_MUL: ei_vector2(compiler->code, VE_MULTIPLY, vpi, inst); break;
		case RC_OPCODE_POW: ei_pow(compiler->code, vpi, inst); break;
		case RC_OPCODE_RCP: ei_math1(compiler->code, ME_RECIP_DX, vpi, inst); break;
		case RC_OPCODE_RSQ: ei_math1(compiler->code, ME_RECIP_SQRT_DX, vpi, inst); break;
		case RC_OPCODE_SGE: ei_vector2(compiler->code, VE_SET_GREATER_THAN_EQUAL, vpi, inst); break;
		case RC_OPCODE_SLT: ei_vector2(compiler->code, VE_SET_LESS_THAN, vpi, inst); break;
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
	unsigned int Allocated:1;
	unsigned int HwTemp:15;
	struct rc_instruction * LastRead;
};

static void allocate_temporary_registers(struct r300_vertex_program_compiler * compiler)
{
	struct rc_instruction *inst;
	unsigned int num_orig_temps = 0;
	char hwtemps[VSF_MAX_FRAGMENT_TEMPS];
	struct temporary_allocation * ta;
	unsigned int i, j;

	compiler->code->num_temporaries = 0;
	memset(hwtemps, 0, sizeof(hwtemps));

	/* Pass 1: Count original temporaries and allocate structures */
	for(inst = compiler->Base.Program.Instructions.Next; inst != &compiler->Base.Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		for (i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_TEMPORARY) {
				if (inst->U.I.SrcReg[i].Index >= num_orig_temps)
					num_orig_temps = inst->U.I.SrcReg[i].Index + 1;
			}
		}

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_TEMPORARY) {
				if (inst->U.I.DstReg.Index >= num_orig_temps)
					num_orig_temps = inst->U.I.DstReg.Index + 1;
			}
		}
	}

	ta = (struct temporary_allocation*)memory_pool_malloc(&compiler->Base.Pool,
			sizeof(struct temporary_allocation) * num_orig_temps);
	memset(ta, 0, sizeof(struct temporary_allocation) * num_orig_temps);

	/* Pass 2: Determine original temporary lifetimes */
	for(inst = compiler->Base.Program.Instructions.Next; inst != &compiler->Base.Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		for (i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_TEMPORARY)
				ta[inst->U.I.SrcReg[i].Index].LastRead = inst;
		}
	}

	/* Pass 3: Register allocation */
	for(inst = compiler->Base.Program.Instructions.Next; inst != &compiler->Base.Program.Instructions; inst = inst->Next) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

		for (i = 0; i < opcode->NumSrcRegs; ++i) {
			if (inst->U.I.SrcReg[i].File == RC_FILE_TEMPORARY) {
				unsigned int orig = inst->U.I.SrcReg[i].Index;
				inst->U.I.SrcReg[i].Index = ta[orig].HwTemp;

				if (ta[orig].Allocated && inst == ta[orig].LastRead)
					hwtemps[ta[orig].HwTemp] = 0;
			}
		}

		if (opcode->HasDstReg) {
			if (inst->U.I.DstReg.File == RC_FILE_TEMPORARY) {
				unsigned int orig = inst->U.I.DstReg.Index;

				if (!ta[orig].Allocated) {
					for(j = 0; j < VSF_MAX_FRAGMENT_TEMPS; ++j) {
						if (!hwtemps[j])
							break;
					}
					if (j >= VSF_MAX_FRAGMENT_TEMPS) {
						fprintf(stderr, "Out of hw temporaries\n");
					} else {
						ta[orig].Allocated = 1;
						ta[orig].HwTemp = j;
						hwtemps[j] = 1;

						if (j >= compiler->code->num_temporaries)
							compiler->code->num_temporaries = j + 1;
					}
				}

				inst->U.I.DstReg.Index = ta[orig].HwTemp;
			}
		}
	}
}


/**
 * Vertex engine cannot read two inputs or two constants at the same time.
 * Introduce intermediate MOVs to temporary registers to account for this.
 */
static int transform_source_conflicts(
	struct radeon_compiler *c,
	struct rc_instruction* inst,
	void* unused)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

	if (opcode->NumSrcRegs == 3) {
		if (t_src_conflict(inst->U.I.SrcReg[1], inst->U.I.SrcReg[2])
		    || t_src_conflict(inst->U.I.SrcReg[0], inst->U.I.SrcReg[2])) {
			int tmpreg = rc_find_free_temporary(c);
			struct rc_instruction * inst_mov = rc_insert_new_instruction(c, inst->Prev);
			inst_mov->U.I.Opcode = RC_OPCODE_MOV;
			inst_mov->U.I.DstReg.File = RC_FILE_TEMPORARY;
			inst_mov->U.I.DstReg.Index = tmpreg;
			inst_mov->U.I.SrcReg[0] = inst->U.I.SrcReg[2];

			reset_srcreg(&inst->U.I.SrcReg[2]);
			inst->U.I.SrcReg[2].File = RC_FILE_TEMPORARY;
			inst->U.I.SrcReg[2].Index = tmpreg;
		}
	}

	if (opcode->NumSrcRegs >= 2) {
		if (t_src_conflict(inst->U.I.SrcReg[1], inst->U.I.SrcReg[0])) {
			int tmpreg = rc_find_free_temporary(c);
			struct rc_instruction * inst_mov = rc_insert_new_instruction(c, inst->Prev);
			inst_mov->U.I.Opcode = RC_OPCODE_MOV;
			inst_mov->U.I.DstReg.File = RC_FILE_TEMPORARY;
			inst_mov->U.I.DstReg.Index = tmpreg;
			inst_mov->U.I.SrcReg[0] = inst->U.I.SrcReg[1];

			reset_srcreg(&inst->U.I.SrcReg[1]);
			inst->U.I.SrcReg[1].File = RC_FILE_TEMPORARY;
			inst->U.I.SrcReg[1].Index = tmpreg;
		}
	}

	return 1;
}

static void addArtificialOutputs(struct r300_vertex_program_compiler * compiler)
{
	int i;

	for(i = 0; i < 32; ++i) {
		if ((compiler->RequiredOutputs & (1 << i)) &&
		    !(compiler->Base.Program.OutputsWritten & (1 << i))) {
			struct rc_instruction * inst = rc_insert_new_instruction(&compiler->Base, compiler->Base.Program.Instructions.Prev);
			inst->U.I.Opcode = RC_OPCODE_MOV;

			inst->U.I.DstReg.File = RC_FILE_OUTPUT;
			inst->U.I.DstReg.Index = i;
			inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;

			inst->U.I.SrcReg[0].File = RC_FILE_CONSTANT;
			inst->U.I.SrcReg[0].Index = 0;
			inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

			compiler->Base.Program.OutputsWritten |= 1 << i;
		}
	}
}

static void dataflow_outputs_mark_used(void * userdata, void * data,
		void (*callback)(void *, unsigned int, unsigned int))
{
	struct r300_vertex_program_compiler * c = userdata;
	int i;

	for(i = 0; i < 32; ++i) {
		if (c->RequiredOutputs & (1 << i))
			callback(data, i, RC_MASK_XYZW);
	}
}

static int swizzle_is_native(rc_opcode opcode, struct rc_src_register reg)
{
	(void) opcode;
	(void) reg;

	return 1;
}


static struct rc_swizzle_caps r300_vertprog_swizzle_caps = {
	.IsNative = &swizzle_is_native,
	.Split = 0 /* should never be called */
};


void r3xx_compile_vertex_program(struct r300_vertex_program_compiler* compiler)
{
	compiler->Base.SwizzleCaps = &r300_vertprog_swizzle_caps;

	addArtificialOutputs(compiler);

	{
		struct radeon_program_transformation transformations[] = {
			{ &r300_transform_vertex_alu, 0 },
		};
		radeonLocalTransform(&compiler->Base, 1, transformations);
	}

	if (compiler->Base.Debug) {
		fprintf(stderr, "Vertex program after native rewrite:\n");
		rc_print_program(&compiler->Base.Program);
		fflush(stderr);
	}

	{
		/* Note: This pass has to be done seperately from ALU rewrite,
		 * otherwise non-native ALU instructions with source conflits
		 * will not be treated properly.
		 */
		struct radeon_program_transformation transformations[] = {
			{ &transform_source_conflicts, 0 },
		};
		radeonLocalTransform(&compiler->Base, 1, transformations);
	}

	if (compiler->Base.Debug) {
		fprintf(stderr, "Vertex program after source conflict resolve:\n");
		rc_print_program(&compiler->Base.Program);
		fflush(stderr);
	}

	rc_dataflow_deadcode(&compiler->Base, &dataflow_outputs_mark_used, compiler);

	if (compiler->Base.Debug) {
		fprintf(stderr, "Vertex program after deadcode:\n");
		rc_print_program(&compiler->Base.Program);
		fflush(stderr);
	}

	rc_dataflow_swizzles(&compiler->Base);

	allocate_temporary_registers(compiler);

	if (compiler->Base.Debug) {
		fprintf(stderr, "Vertex program after dataflow:\n");
		rc_print_program(&compiler->Base.Program);
		fflush(stderr);
	}

	translate_vertex_program(compiler);

	rc_constants_copy(&compiler->code->constants, &compiler->Base.Program.Constants);

	compiler->code->InputsRead = compiler->Base.Program.InputsRead;
	compiler->code->OutputsWritten = compiler->Base.Program.OutputsWritten;

	if (compiler->Base.Debug) {
		fprintf(stderr, "Final vertex program code:\n");
		r300_vertex_program_dump(compiler->code);
	}
}
