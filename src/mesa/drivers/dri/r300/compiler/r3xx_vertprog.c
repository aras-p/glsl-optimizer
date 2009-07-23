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

#include "shader/prog_optimize.h"
#include "shader/prog_print.h"


/* TODO: Get rid of t_src_class call */
#define CMP_SRCS(a, b) ((a.RelAddr != b.RelAddr) || (a.Index != b.Index && \
		       ((t_src_class(a.File) == PVS_SRC_REG_CONSTANT && \
			 t_src_class(b.File) == PVS_SRC_REG_CONSTANT) || \
			(t_src_class(a.File) == PVS_SRC_REG_INPUT && \
			 t_src_class(b.File) == PVS_SRC_REG_INPUT)))) \

/*
 * Take an already-setup and valid source then swizzle it appropriately to
 * obtain a constant ZERO or ONE source.
 */
#define __CONST(x, y)	\
	(PVS_SRC_OPERAND(t_src_index(vp, &src[x]),	\
			   t_swizzle(y),	\
			   t_swizzle(y),	\
			   t_swizzle(y),	\
			   t_swizzle(y),	\
			   t_src_class(src[x].File), \
			   NEGATE_NONE) | (src[x].RelAddr << 4))




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

static GLuint *r300TranslateOpcodeADD(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_ADD,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeARL(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_FLT2FIX_DX,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeDP4(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_DOT_PRODUCT,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeDST(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_DISTANCE_VECTOR,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeEX2(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(ME_EXP_BASE2_FULL_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeEXP(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(ME_EXP_BASE2_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeFRC(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_FRACTION,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeLG2(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	// LG2 RESULT 1.X Y Z W PARAM 0{} {X X X X}

	inst[0] = PVS_OP_DST_OPERAND(ME_LOG_BASE2_FULL_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
				  t_src_class(src[0].File),
				  src[0].Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeLIT(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	//LIT TMP 1.Y Z TMP 1{} {X W Z Y} TMP 1{} {Y W Z X} TMP 1{} {Y X Z W}

	inst[0] = PVS_OP_DST_OPERAND(ME_LIGHT_COEFF_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	/* NOTE: Users swizzling might not work. */
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_src_class(src[0].File),
				  src[0].Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  t_src_class(src[0].File),
				  src[0].Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (src[0].RelAddr << 4);
	inst[3] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  t_src_class(src[0].File),
				  src[0].Negate ? NEGATE_XYZW : NEGATE_NONE) |
	    (src[0].RelAddr << 4);

	return inst;
}

static GLuint *r300TranslateOpcodeLOG(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(ME_LOG_BASE2_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeMAD(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(PVS_MACRO_OP_2CLK_MADD,
				     GL_FALSE,
				     GL_TRUE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = t_src(vp, &src[2]);

	return inst;
}

static GLuint *r300TranslateOpcodeMAX(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_MAXIMUM,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeMIN(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_MINIMUM,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeMOV(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	//ADD RESULT 1.X Y Z W PARAM 0{} {X Y Z W} PARAM 0{} {ZERO ZERO ZERO ZERO}

	inst[0] = PVS_OP_DST_OPERAND(VE_ADD,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeMUL(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_MULTIPLY,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodePOW(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(ME_POWER_FUNC_FF,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = t_src_scalar(vp, &src[1]);

	return inst;
}

static GLuint *r300TranslateOpcodeRCP(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(ME_RECIP_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeRSQ(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(ME_RECIP_SQRT_DX,
				     GL_TRUE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src_scalar(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeSGE(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_SET_GREATER_THAN_EQUAL,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeSLT(struct r300_vertex_program_code *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	inst[0] = PVS_OP_DST_OPERAND(VE_SET_LESS_THAN,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static void t_inputs_outputs(struct r300_vertex_program_code *vp, struct gl_program * glvp)
{
	int i;
	int cur_reg;
	GLuint OutputsWritten, InputsRead;

	OutputsWritten = glvp->OutputsWritten;
	InputsRead = glvp->InputsRead;

	cur_reg = -1;
	for (i = 0; i < VERT_ATTRIB_MAX; i++) {
		if (InputsRead & (1 << i))
			vp->inputs[i] = ++cur_reg;
		else
			vp->inputs[i] = -1;
	}

	cur_reg = 0;
	for (i = 0; i < VERT_RESULT_MAX; i++)
		vp->outputs[i] = -1;

	assert(OutputsWritten & (1 << VERT_RESULT_HPOS));

	if (OutputsWritten & (1 << VERT_RESULT_HPOS)) {
		vp->outputs[VERT_RESULT_HPOS] = cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_PSIZ)) {
		vp->outputs[VERT_RESULT_PSIZ] = cur_reg++;
	}

	/* If we're writing back facing colors we need to send
	 * four colors to make front/back face colors selection work.
	 * If the vertex program doesn't write all 4 colors, lets
	 * pretend it does by skipping output index reg so the colors
	 * get written into appropriate output vectors.
	 */
	if (OutputsWritten & (1 << VERT_RESULT_COL0)) {
		vp->outputs[VERT_RESULT_COL0] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0) ||
		OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_COL1)) {
		vp->outputs[VERT_RESULT_COL1] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0) ||
		OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_BFC0)) {
		vp->outputs[VERT_RESULT_BFC0] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		cur_reg++;
	}

	if (OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		vp->outputs[VERT_RESULT_BFC1] = cur_reg++;
	} else if (OutputsWritten & (1 << VERT_RESULT_BFC0)) {
		cur_reg++;
	}

	for (i = VERT_RESULT_TEX0; i <= VERT_RESULT_TEX7; i++) {
		if (OutputsWritten & (1 << i)) {
			vp->outputs[i] = cur_reg++;
		}
	}

	if (OutputsWritten & (1 << VERT_RESULT_FOGC)) {
		vp->outputs[VERT_RESULT_FOGC] = cur_reg++;
	}
}

static GLboolean translate_vertex_program(struct r300_vertex_program_compiler * compiler)
{
	struct prog_instruction *vpi = compiler->program->Instructions;
	int i;
	GLuint *inst;
	unsigned long num_operands;
	/* Initial value should be last tmp reg that hw supports.
	   Strangely enough r300 doesnt mind even though these would be out of range.
	   Smart enough to realize that it doesnt need it? */
	int u_temp_i = VSF_MAX_FRAGMENT_TEMPS - 1;
	struct prog_src_register src[3];
	struct r300_vertex_program_code * vp = compiler->code;

	compiler->code->pos_end = 0;	/* Not supported yet */
	compiler->code->length = 0;

	t_inputs_outputs(compiler->code, compiler->program);

	for (inst = compiler->code->body.d; vpi->Opcode != OPCODE_END;
	     vpi++, inst += 4) {

		{
			int u_temp_used = (VSF_MAX_FRAGMENT_TEMPS - 1) - u_temp_i;
			if((compiler->code->num_temporaries + u_temp_used) > VSF_MAX_FRAGMENT_TEMPS) {
				fprintf(stderr, "Ran out of temps, num temps %d, us %d\n", compiler->code->num_temporaries, u_temp_used);
				return GL_FALSE;
			}
			u_temp_i=VSF_MAX_FRAGMENT_TEMPS-1;
		}

		if (!valid_dst(compiler->code, &vpi->DstReg)) {
			/* redirect result to unused temp */
			vpi->DstReg.File = PROGRAM_TEMPORARY;
			vpi->DstReg.Index = u_temp_i;
		}

		num_operands = _mesa_num_inst_src_regs(vpi->Opcode);

		/* copy the sources (src) from mesa into a local variable... is this needed? */
		for (i = 0; i < num_operands; i++) {
			src[i] = vpi->SrcReg[i];
		}

		if (num_operands == 3) {	/* TODO: scalars */
			if (CMP_SRCS(src[1], src[2])
			    || CMP_SRCS(src[0], src[2])) {
				inst[0] = PVS_OP_DST_OPERAND(VE_ADD,
							     GL_FALSE,
							     GL_FALSE,
							     u_temp_i,
							     WRITEMASK_XYZW,
							     PVS_DST_REG_TEMPORARY);
				inst[1] =
				    PVS_SRC_OPERAND(t_src_index(compiler->code, &src[2]),
						    SWIZZLE_X,
						    SWIZZLE_Y,
						    SWIZZLE_Z,
						    SWIZZLE_W,
						    t_src_class(src[2].File),
						    NEGATE_NONE) | (src[2].
								      RelAddr <<
								      4);
				inst[2] = __CONST(2, SWIZZLE_ZERO);
				inst[3] = __CONST(2, SWIZZLE_ZERO);
				inst += 4;

				src[2].File = PROGRAM_TEMPORARY;
				src[2].Index = u_temp_i;
				src[2].RelAddr = 0;
				u_temp_i--;
			}
		}

		if (num_operands >= 2) {
			if (CMP_SRCS(src[1], src[0])) {
				inst[0] = PVS_OP_DST_OPERAND(VE_ADD,
							     GL_FALSE,
							     GL_FALSE,
							     u_temp_i,
							     WRITEMASK_XYZW,
							     PVS_DST_REG_TEMPORARY);
				inst[1] =
				    PVS_SRC_OPERAND(t_src_index(compiler->code, &src[0]),
						    SWIZZLE_X,
						    SWIZZLE_Y,
						    SWIZZLE_Z,
						    SWIZZLE_W,
						    t_src_class(src[0].File),
						    NEGATE_NONE) | (src[0].
								      RelAddr <<
								      4);
				inst[2] = __CONST(0, SWIZZLE_ZERO);
				inst[3] = __CONST(0, SWIZZLE_ZERO);
				inst += 4;

				src[0].File = PROGRAM_TEMPORARY;
				src[0].Index = u_temp_i;
				src[0].RelAddr = 0;
				u_temp_i--;
			}
		}

		switch (vpi->Opcode) {
		case OPCODE_ADD:
			inst = r300TranslateOpcodeADD(compiler->code, vpi, inst, src);
			break;
		case OPCODE_ARL:
			inst = r300TranslateOpcodeARL(compiler->code, vpi, inst, src);
			break;
		case OPCODE_DP4:
			inst = r300TranslateOpcodeDP4(compiler->code, vpi, inst, src);
			break;
		case OPCODE_DST:
			inst = r300TranslateOpcodeDST(compiler->code, vpi, inst, src);
			break;
		case OPCODE_EX2:
			inst = r300TranslateOpcodeEX2(compiler->code, vpi, inst, src);
			break;
		case OPCODE_EXP:
			inst = r300TranslateOpcodeEXP(compiler->code, vpi, inst, src);
			break;
		case OPCODE_FRC:
			inst = r300TranslateOpcodeFRC(compiler->code, vpi, inst, src);
			break;
		case OPCODE_LG2:
			inst = r300TranslateOpcodeLG2(compiler->code, vpi, inst, src);
			break;
		case OPCODE_LIT:
			inst = r300TranslateOpcodeLIT(compiler->code, vpi, inst, src);
			break;
		case OPCODE_LOG:
			inst = r300TranslateOpcodeLOG(compiler->code, vpi, inst, src);
			break;
		case OPCODE_MAD:
			inst = r300TranslateOpcodeMAD(compiler->code, vpi, inst, src);
			break;
		case OPCODE_MAX:
			inst = r300TranslateOpcodeMAX(compiler->code, vpi, inst, src);
			break;
		case OPCODE_MIN:
			inst = r300TranslateOpcodeMIN(compiler->code, vpi, inst, src);
			break;
		case OPCODE_MOV:
			inst = r300TranslateOpcodeMOV(compiler->code, vpi, inst, src);
			break;
		case OPCODE_MUL:
			inst = r300TranslateOpcodeMUL(compiler->code, vpi, inst, src);
			break;
		case OPCODE_POW:
			inst = r300TranslateOpcodePOW(compiler->code, vpi, inst, src);
			break;
		case OPCODE_RCP:
			inst = r300TranslateOpcodeRCP(compiler->code, vpi, inst, src);
			break;
		case OPCODE_RSQ:
			inst = r300TranslateOpcodeRSQ(compiler->code, vpi, inst, src);
			break;
		case OPCODE_SGE:
			inst = r300TranslateOpcodeSGE(compiler->code, vpi, inst, src);
			break;
		case OPCODE_SLT:
			inst = r300TranslateOpcodeSLT(compiler->code, vpi, inst, src);
			break;
		default:
			return GL_FALSE;
		}
	}

	compiler->code->length = (inst - compiler->code->body.d);
	if (compiler->code->length >= VSF_MAX_FRAGMENT_LENGTH) {
		return GL_FALSE;
	}

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
	if (s->Program->OutputsWritten & (1 << VERT_RESULT_PSIZ))
		s->Outputs[VERT_RESULT_PSIZ].Sourced = WRITEMASK_X;
}

static GLboolean swizzleIsNative(GLuint opcode, struct prog_src_register reg)
{
	(void) opcode;
	(void) reg;

	return GL_TRUE;
}



GLboolean r3xx_compile_vertex_program(struct r300_vertex_program_compiler* compiler, GLcontext * ctx)
{
	GLboolean success;

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
		struct radeon_nqssadce_descr nqssadce = {
			.Init = &nqssadceInit,
			.IsNativeSwizzle = &swizzleIsNative,
			.BuildSwizzle = NULL
		};
		radeonNqssaDce(compiler->program, &nqssadce, compiler);

		/* We need this step for reusing temporary registers */
		_mesa_optimize_program(ctx, compiler->program);

		if (compiler->Base.Debug) {
			fprintf(stderr, "Vertex program after NQSSADCE:\n");
			_mesa_print_program(compiler->program);
			fflush(stdout);
		}
	}

	assert(compiler->program->NumInstructions);
	{
		struct prog_instruction *inst;
		int max, i, tmp;

		inst = compiler->program->Instructions;
		max = -1;
		while (inst->Opcode != OPCODE_END) {
			tmp = _mesa_num_inst_src_regs(inst->Opcode);
			for (i = 0; i < tmp; ++i) {
				if (inst->SrcReg[i].File == PROGRAM_TEMPORARY) {
					if ((int) inst->SrcReg[i].Index > max) {
						max = inst->SrcReg[i].Index;
					}
				}
			}

			if (_mesa_num_inst_dst_regs(inst->Opcode)) {
				if (inst->DstReg.File == PROGRAM_TEMPORARY) {
					if ((int) inst->DstReg.Index > max) {
						max = inst->DstReg.Index;
					}
				}
			}
			++inst;
		}

		/* We actually want highest index of used temporary register,
		 * not the number of temporaries used.
		 * These values aren't always the same.
		 */
		compiler->code->num_temporaries = max + 1;
	}

	success = translate_vertex_program(compiler);

	compiler->code->InputsRead = compiler->program->InputsRead;
	compiler->code->OutputsWritten = compiler->program->OutputsWritten;

	return success;
}
