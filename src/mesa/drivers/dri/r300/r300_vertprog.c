/**************************************************************************

Copyright (C) 2005  Aapo Tahkola <aet@rasterburn.org>
Copyright (C) 2008  Oliver McFadden <z3ro.geek@gmail.com>

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* Radeon R5xx Acceleration, Revision 1.2 */

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "shader/program.h"
#include "shader/programopt.h"
#include "shader/prog_instruction.h"
#include "shader/prog_optimize.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "tnl/tnl.h"

#include "radeon_nqssadce.h"
#include "r300_context.h"
#include "r300_state.h"

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
			   VSF_FLAG_NONE) | (src[x].RelAddr << 4))

#define FREE_TEMPS() \
	do { \
		int u_temp_used = (VSF_MAX_FRAGMENT_TEMPS - 1) - u_temp_i; \
		if((vp->num_temporaries + u_temp_used) > VSF_MAX_FRAGMENT_TEMPS) { \
			WARN_ONCE("Ran out of temps, num temps %d, us %d\n", vp->num_temporaries, u_temp_used); \
			vp->error = GL_TRUE; \
		} \
		u_temp_i=VSF_MAX_FRAGMENT_TEMPS-1; \
	} while (0)

static GLuint combineSwizzles(GLuint src, GLuint swz_x, GLuint swz_y, GLuint swz_z, GLuint swz_w)
{
	GLuint ret = 0;

	if (swz_x == SWIZZLE_ZERO || swz_x == SWIZZLE_ONE)
		ret |= swz_x;
	else
		ret |= GET_SWZ(src, swz_x);

	if (swz_y == SWIZZLE_ZERO || swz_y == SWIZZLE_ONE)
		ret |= swz_y << 3;
	else
		ret |= GET_SWZ(src, swz_y) << 3;

	if (swz_z == SWIZZLE_ZERO || swz_z == SWIZZLE_ONE)
		ret |= swz_z << 6;
	else
		ret |= GET_SWZ(src, swz_z) << 6;

	if (swz_w == SWIZZLE_ZERO || swz_w == SWIZZLE_ONE)
		ret |= swz_w << 9;
	else
		ret |= GET_SWZ(src, swz_w) << 9;

	return ret;
}

static int r300VertexProgUpdateParams(GLcontext * ctx, struct gl_vertex_program *vp, float *dst)
{
	int pi;
	float *dst_o = dst;
	struct gl_program_parameter_list *paramList;

	if (vp->IsNVProgram) {
		_mesa_load_tracked_matrices(ctx);

		for (pi = 0; pi < MAX_NV_VERTEX_PROGRAM_PARAMS; pi++) {
			*dst++ = ctx->VertexProgram.Parameters[pi][0];
			*dst++ = ctx->VertexProgram.Parameters[pi][1];
			*dst++ = ctx->VertexProgram.Parameters[pi][2];
			*dst++ = ctx->VertexProgram.Parameters[pi][3];
		}
		return dst - dst_o;
	}

	if (!vp->Base.Parameters)
		return 0;

	_mesa_load_state_parameters(ctx, vp->Base.Parameters);

	if (vp->Base.Parameters->NumParameters * 4 >
	    VSF_MAX_FRAGMENT_LENGTH) {
		fprintf(stderr, "%s:Params exhausted\n", __FUNCTION__);
		_mesa_exit(-1);
	}

	paramList = vp->Base.Parameters;
	for (pi = 0; pi < paramList->NumParameters; pi++) {
		switch (paramList->Parameters[pi].Type) {
		case PROGRAM_STATE_VAR:
		case PROGRAM_NAMED_PARAM:
			//fprintf(stderr, "%s", vp->Parameters->Parameters[pi].Name);
		case PROGRAM_CONSTANT:
			*dst++ = paramList->ParameterValues[pi][0];
			*dst++ = paramList->ParameterValues[pi][1];
			*dst++ = paramList->ParameterValues[pi][2];
			*dst++ = paramList->ParameterValues[pi][3];
			break;
		default:
			_mesa_problem(NULL, "Bad param type in %s",
				      __FUNCTION__);
		}

	}

	return dst - dst_o;
}

static unsigned long t_dst_mask(GLuint mask)
{
	/* WRITEMASK_* is equivalent to VSF_FLAG_* */
	return mask & VSF_FLAG_ALL;
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

static unsigned long t_dst_index(struct r300_vertex_program *vp,
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

#if 0
static void vp_dump_inputs(struct r300_vertex_program *vp, char *caller)
{
	int i;

	if (vp == NULL) {
		fprintf(stderr, "vp null in call to %s from %s\n", __FUNCTION__,
			caller);
		return;
	}

	fprintf(stderr, "%s:<", caller);
	for (i = 0; i < VERT_ATTRIB_MAX; i++)
		fprintf(stderr, "%d ", vp->inputs[i]);
	fprintf(stderr, ">\n");

}
#endif

static unsigned long t_src_index(struct r300_vertex_program *vp,
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

static unsigned long t_src(struct r300_vertex_program *vp,
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

static unsigned long t_src_scalar(struct r300_vertex_program *vp,
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
			       src->Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src->RelAddr << 4);
}

static GLboolean valid_dst(struct r300_vertex_program *vp,
			   struct prog_dst_register *dst)
{
	if (dst->File == PROGRAM_OUTPUT && vp->outputs[dst->Index] == -1) {
		return GL_FALSE;
	} else if (dst->File == PROGRAM_ADDRESS) {
		assert(dst->Index == 0);
	}

	return GL_TRUE;
}

static GLuint *r300TranslateOpcodeABS(struct r300_vertex_program *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	//MAX RESULT 1.X Y Z W PARAM 0{} {X Y Z W} PARAM 0{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W

	inst[0] = PVS_OP_DST_OPERAND(VE_MAXIMUM,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),
				  t_src_class(src[0].File),
				  (!src[0].
				   Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[3] = 0;

	return inst;
}

static GLuint *r300TranslateOpcodeADD(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeARL(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeDP3(struct r300_vertex_program *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	//DOT RESULT 1.X Y Z W PARAM 0{} {X Y Z ZERO} PARAM 0{} {X Y Z ZERO}

	inst[0] = PVS_OP_DST_OPERAND(VE_DOT_PRODUCT,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
				  SWIZZLE_ZERO,
				  t_src_class(src[0].File),
				  src[0].Negate ? VSF_FLAG_XYZ : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] =
	    PVS_SRC_OPERAND(t_src_index(vp, &src[1]),
			    t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
			    t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
			    t_swizzle(GET_SWZ(src[1].Swizzle, 2)), SWIZZLE_ZERO,
			    t_src_class(src[1].File),
			    src[1].Negate ? VSF_FLAG_XYZ : VSF_FLAG_NONE) |
	    (src[1].RelAddr << 4);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeDP4(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeDPH(struct r300_vertex_program *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	//DOT RESULT 1.X Y Z W PARAM 0{} {X Y Z ONE} PARAM 0{} {X Y Z W}
	inst[0] = PVS_OP_DST_OPERAND(VE_DOT_PRODUCT,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
				  t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
				  PVS_SRC_SELECT_FORCE_1,
				  t_src_class(src[0].File),
				  src[0].Negate ? VSF_FLAG_XYZ : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = t_src(vp, &src[1]);
	inst[3] = __CONST(1, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeDST(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeEX2(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeEXP(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeFLR(struct r300_vertex_program *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3],
				      int *u_temp_i)
{
	/* FRC TMP 0.X Y Z W PARAM 0{} {X Y Z W}
	   ADD RESULT 1.X Y Z W PARAM 0{} {X Y Z W} TMP 0{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W */

	inst[0] = PVS_OP_DST_OPERAND(VE_FRACTION,
				     GL_FALSE,
				     GL_FALSE,
				     *u_temp_i,
				     t_dst_mask(vpi->DstReg.WriteMask),
				     PVS_DST_REG_TEMPORARY);
	inst[1] = t_src(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);
	inst += 4;

	inst[0] = PVS_OP_DST_OPERAND(VE_ADD,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = PVS_SRC_OPERAND(*u_temp_i,
				  PVS_SRC_SELECT_X,
				  PVS_SRC_SELECT_Y,
				  PVS_SRC_SELECT_Z,
				  PVS_SRC_SELECT_W, PVS_SRC_REG_TEMPORARY,
				  /* Not 100% sure about this */
				  (!src[0].
				   Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE
				  /*VSF_FLAG_ALL */ );
	inst[3] = __CONST(0, SWIZZLE_ZERO);
	(*u_temp_i)--;

	return inst;
}

static GLuint *r300TranslateOpcodeFRC(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeLG2(struct r300_vertex_program *vp,
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
				  src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = __CONST(0, SWIZZLE_ZERO);
	inst[3] = __CONST(0, SWIZZLE_ZERO);

	return inst;
}

static GLuint *r300TranslateOpcodeLIT(struct r300_vertex_program *vp,
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
				  src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  t_src_class(src[0].File),
				  src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[3] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  t_src_class(src[0].File),
				  src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);

	return inst;
}

static GLuint *r300TranslateOpcodeLOG(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeMAD(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeMAX(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeMIN(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeMOV(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeMUL(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodePOW(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeRCP(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeRSQ(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeSGE(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeSLT(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeSUB(struct r300_vertex_program *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3])
{
	//ADD RESULT 1.X Y Z W TMP 0{} {X Y Z W} PARAM 1{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W

#if 0
	inst[0] = PVS_OP_DST_OPERAND(VE_ADD,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[1]),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 2)),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 3)),
				  t_src_class(src[1].File),
				  (!src[1].
				   Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[1].RelAddr << 4);
	inst[3] = 0;
#else
	inst[0] =
	    PVS_OP_DST_OPERAND(VE_MULTIPLY_ADD,
			       GL_FALSE,
			       GL_FALSE,
			       t_dst_index(vp, &vpi->DstReg),
			       t_dst_mask(vpi->DstReg.WriteMask),
			       t_dst_class(vpi->DstReg.File));
	inst[1] = t_src(vp, &src[0]);
	inst[2] = __CONST(0, SWIZZLE_ONE);
	inst[3] = PVS_SRC_OPERAND(t_src_index(vp, &src[1]),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 2)),
				  t_swizzle(GET_SWZ(src[1].Swizzle, 3)),
				  t_src_class(src[1].File),
				  (!src[1].
				   Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[1].RelAddr << 4);
#endif

	return inst;
}

static GLuint *r300TranslateOpcodeSWZ(struct r300_vertex_program *vp,
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

static GLuint *r300TranslateOpcodeXPD(struct r300_vertex_program *vp,
				      struct prog_instruction *vpi,
				      GLuint * inst,
				      struct prog_src_register src[3],
				      int *u_temp_i)
{
	/* mul r0, r1.yzxw, r2.zxyw
	   mad r0, -r2.yzxw, r1.zxyw, r0
	 */

	inst[0] = PVS_OP_DST_OPERAND(VE_MULTIPLY_ADD,
				     GL_FALSE,
				     GL_FALSE,
				     *u_temp_i,
				     t_dst_mask(vpi->DstReg.WriteMask),
				     PVS_DST_REG_TEMPORARY);
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 2)),	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  t_src_class(src[0].File),
				  src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[1]), t_swizzle(GET_SWZ(src[1].Swizzle, 2)),	// Z
				  t_swizzle(GET_SWZ(src[1].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(src[1].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[1].Swizzle, 3)),	// W
				  t_src_class(src[1].File),
				  src[1].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[1].RelAddr << 4);
	inst[3] = __CONST(1, SWIZZLE_ZERO);
	inst += 4;

	inst[0] = PVS_OP_DST_OPERAND(VE_MULTIPLY_ADD,
				     GL_FALSE,
				     GL_FALSE,
				     t_dst_index(vp, &vpi->DstReg),
				     t_dst_mask(vpi->DstReg.WriteMask),
				     t_dst_class(vpi->DstReg.File));
	inst[1] = PVS_SRC_OPERAND(t_src_index(vp, &src[1]), t_swizzle(GET_SWZ(src[1].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[1].Swizzle, 2)),	// Z
				  t_swizzle(GET_SWZ(src[1].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(src[1].Swizzle, 3)),	// W
				  t_src_class(src[1].File),
				  (!src[1].
				   Negate) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[1].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 2)),	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  t_src_class(src[0].File),
				  src[0].Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[3] =
	    PVS_SRC_OPERAND(*u_temp_i, PVS_SRC_SELECT_X, PVS_SRC_SELECT_Y,
			    PVS_SRC_SELECT_Z, PVS_SRC_SELECT_W,
			    PVS_SRC_REG_TEMPORARY, VSF_FLAG_NONE);

	(*u_temp_i)--;

	return inst;
}

static void t_inputs_outputs(struct r300_vertex_program *vp)
{
	int i;
	int cur_reg;
	GLuint OutputsWritten, InputsRead;

	OutputsWritten = vp->Base->Base.OutputsWritten;
	InputsRead = vp->Base->Base.InputsRead;

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

void r300TranslateVertexShader(struct r300_vertex_program *vp)
{
	struct prog_instruction *vpi = vp->Base->Base.Instructions;
	int i;
	GLuint *inst;
	unsigned long num_operands;
	/* Initial value should be last tmp reg that hw supports.
	   Strangely enough r300 doesnt mind even though these would be out of range.
	   Smart enough to realize that it doesnt need it? */
	int u_temp_i = VSF_MAX_FRAGMENT_TEMPS - 1;
	struct prog_src_register src[3];

	vp->pos_end = 0;	/* Not supported yet */
	vp->hw_code.length = 0;
	vp->translated = GL_TRUE;
	vp->error = GL_FALSE;

	t_inputs_outputs(vp);

	for (inst = vp->hw_code.body.d; vpi->Opcode != OPCODE_END;
	     vpi++, inst += 4) {

		FREE_TEMPS();

		if (!valid_dst(vp, &vpi->DstReg)) {
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
							     VSF_FLAG_ALL,
							     PVS_DST_REG_TEMPORARY);
				inst[1] =
				    PVS_SRC_OPERAND(t_src_index(vp, &src[2]),
						    SWIZZLE_X,
						    SWIZZLE_Y,
						    SWIZZLE_Z,
						    SWIZZLE_W,
						    t_src_class(src[2].File),
						    VSF_FLAG_NONE) | (src[2].
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
							     VSF_FLAG_ALL,
							     PVS_DST_REG_TEMPORARY);
				inst[1] =
				    PVS_SRC_OPERAND(t_src_index(vp, &src[0]),
						    SWIZZLE_X,
						    SWIZZLE_Y,
						    SWIZZLE_Z,
						    SWIZZLE_W,
						    t_src_class(src[0].File),
						    VSF_FLAG_NONE) | (src[0].
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
		case OPCODE_ABS:
			inst = r300TranslateOpcodeABS(vp, vpi, inst, src);
			break;
		case OPCODE_ADD:
			inst = r300TranslateOpcodeADD(vp, vpi, inst, src);
			break;
		case OPCODE_ARL:
			inst = r300TranslateOpcodeARL(vp, vpi, inst, src);
			break;
		case OPCODE_DP3:
			inst = r300TranslateOpcodeDP3(vp, vpi, inst, src);
			break;
		case OPCODE_DP4:
			inst = r300TranslateOpcodeDP4(vp, vpi, inst, src);
			break;
		case OPCODE_DPH:
			inst = r300TranslateOpcodeDPH(vp, vpi, inst, src);
			break;
		case OPCODE_DST:
			inst = r300TranslateOpcodeDST(vp, vpi, inst, src);
			break;
		case OPCODE_EX2:
			inst = r300TranslateOpcodeEX2(vp, vpi, inst, src);
			break;
		case OPCODE_EXP:
			inst = r300TranslateOpcodeEXP(vp, vpi, inst, src);
			break;
		case OPCODE_FLR:
			inst = r300TranslateOpcodeFLR(vp, vpi, inst, src,	/* FIXME */
						      &u_temp_i);
			break;
		case OPCODE_FRC:
			inst = r300TranslateOpcodeFRC(vp, vpi, inst, src);
			break;
		case OPCODE_LG2:
			inst = r300TranslateOpcodeLG2(vp, vpi, inst, src);
			break;
		case OPCODE_LIT:
			inst = r300TranslateOpcodeLIT(vp, vpi, inst, src);
			break;
		case OPCODE_LOG:
			inst = r300TranslateOpcodeLOG(vp, vpi, inst, src);
			break;
		case OPCODE_MAD:
			inst = r300TranslateOpcodeMAD(vp, vpi, inst, src);
			break;
		case OPCODE_MAX:
			inst = r300TranslateOpcodeMAX(vp, vpi, inst, src);
			break;
		case OPCODE_MIN:
			inst = r300TranslateOpcodeMIN(vp, vpi, inst, src);
			break;
		case OPCODE_MOV:
			inst = r300TranslateOpcodeMOV(vp, vpi, inst, src);
			break;
		case OPCODE_MUL:
			inst = r300TranslateOpcodeMUL(vp, vpi, inst, src);
			break;
		case OPCODE_POW:
			inst = r300TranslateOpcodePOW(vp, vpi, inst, src);
			break;
		case OPCODE_RCP:
			inst = r300TranslateOpcodeRCP(vp, vpi, inst, src);
			break;
		case OPCODE_RSQ:
			inst = r300TranslateOpcodeRSQ(vp, vpi, inst, src);
			break;
		case OPCODE_SGE:
			inst = r300TranslateOpcodeSGE(vp, vpi, inst, src);
			break;
		case OPCODE_SLT:
			inst = r300TranslateOpcodeSLT(vp, vpi, inst, src);
			break;
		case OPCODE_SUB:
			inst = r300TranslateOpcodeSUB(vp, vpi, inst, src);
			break;
		case OPCODE_SWZ:
			inst = r300TranslateOpcodeSWZ(vp, vpi, inst, src);
			break;
		case OPCODE_XPD:
			inst = r300TranslateOpcodeXPD(vp, vpi, inst, src,	/* FIXME */
						      &u_temp_i);
			break;
		default:
			vp->error = GL_TRUE;
			break;
		}
	}

	vp->hw_code.length = (inst - vp->hw_code.body.d);
	if (vp->hw_code.length >= VSF_MAX_FRAGMENT_LENGTH) {
		vp->error = GL_TRUE;
	}
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
 @TODO
  We can put X001 swizzle only if input components are directly mapped from output components.
  For some insts we need to skip source swizzles and add: MOV OUTPUT[fog_attr].yzw, CONST[0].0001
 */
static void fog_as_texcoord(struct gl_program *prog, int tex_id)
{
	struct prog_instruction *vpi;
	int i;

	vpi = prog->Instructions;
	while (vpi->Opcode != OPCODE_END) {
		if (vpi->DstReg.File == PROGRAM_OUTPUT && vpi->DstReg.Index == VERT_RESULT_FOGC) {
			vpi->DstReg.Index = VERT_RESULT_TEX0 + tex_id;
			vpi->DstReg.WriteMask = WRITEMASK_XYZW;

			for (i = 0; i < _mesa_num_inst_src_regs(vpi->Opcode); ++i) {
				vpi->SrcReg[i].Swizzle = combineSwizzles(vpi->SrcReg[i].Swizzle, SWIZZLE_X, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ONE);
			}
		}

		++vpi;
	}

	prog->OutputsWritten &= ~(1 << VERT_RESULT_FOGC);
	prog->OutputsWritten |= 1 << (VERT_RESULT_TEX0 + tex_id);
}

static int translateABS(struct gl_program *prog, int pos)
{
	struct prog_instruction *inst;

	inst = &prog->Instructions[pos];

	inst->Opcode = OPCODE_MAX;
	inst->SrcReg[1] = inst->SrcReg[0];
	inst->SrcReg[1].Negate ^= NEGATE_XYZW;

	return 0;
}

static int translateDP3(struct gl_program *prog, int pos)
{
	struct prog_instruction *inst;

	inst = &prog->Instructions[pos];

	inst->Opcode = OPCODE_DP4;
	inst->SrcReg[0].Swizzle = combineSwizzles(inst->SrcReg[0].Swizzle, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ZERO);

	return 0;
}

static int translateDPH(struct gl_program *prog, int pos)
{
	struct prog_instruction *inst;

	inst = &prog->Instructions[pos];

	inst->Opcode = OPCODE_DP4;
	inst->SrcReg[0].Swizzle = combineSwizzles(inst->SrcReg[0].Swizzle, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);

	return 0;
}

static int translateFLR(struct gl_program *prog, int pos)
{
	struct prog_instruction *inst;
	struct prog_dst_register dst;
	int tmp_idx;

	tmp_idx = prog->NumTemporaries++;

	_mesa_insert_instructions(prog, pos + 1, 1);

	inst = &prog->Instructions[pos];
	dst = inst->DstReg;

	inst->Opcode = OPCODE_FRC;
	inst->DstReg.File = PROGRAM_TEMPORARY;
	inst->DstReg.Index = tmp_idx;
	++inst;

	inst->Opcode = OPCODE_ADD;
	inst->DstReg = dst;
	inst->SrcReg[0] = (inst-1)->SrcReg[0];
	inst->SrcReg[1].File = PROGRAM_TEMPORARY;
	inst->SrcReg[1].Index = tmp_idx;
	inst->SrcReg[1].Negate = NEGATE_XYZW;

	return 1;
}

static int translateSUB(struct gl_program *prog, int pos)
{
	struct prog_instruction *inst;

	inst = &prog->Instructions[pos];

	inst->Opcode = OPCODE_ADD;
	inst->SrcReg[1].Negate ^= NEGATE_XYZW;

	return 0;
}

static int translateSWZ(struct gl_program *prog, int pos)
{
	prog->Instructions[pos].Opcode = OPCODE_MOV;

	return 0;
}

static int translateXPD(struct gl_program *prog, int pos)
{
	struct prog_instruction *inst;
	int tmp_idx;

	tmp_idx = prog->NumTemporaries++;

	_mesa_insert_instructions(prog, pos + 1, 1);

	inst = &prog->Instructions[pos];

	*(inst+1) = *inst;

	inst->Opcode = OPCODE_MUL;
	inst->DstReg.File = PROGRAM_TEMPORARY;
	inst->DstReg.Index = tmp_idx;
	inst->SrcReg[0].Swizzle = combineSwizzles(inst->SrcReg[0].Swizzle, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W);
	inst->SrcReg[1].Swizzle = combineSwizzles(inst->SrcReg[1].Swizzle, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W);
	++inst;

	inst->Opcode = OPCODE_MAD;
	inst->SrcReg[0].Swizzle = combineSwizzles(inst->SrcReg[0].Swizzle, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W);
	inst->SrcReg[1].Swizzle = combineSwizzles(inst->SrcReg[1].Swizzle, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W);
	inst->SrcReg[1].Negate ^= NEGATE_XYZW;
	inst->SrcReg[2].File = PROGRAM_TEMPORARY;
	inst->SrcReg[2].Index = tmp_idx;

	return 1;
}

static void translateInsts(struct gl_program *prog)
{
	struct prog_instruction *inst;
	int i;

	for (i = 0; i < prog->NumInstructions; ++i) {
		inst = &prog->Instructions[i];

		switch (inst->Opcode) {
			case OPCODE_ABS:
				i += translateABS(prog, i);
				break;
			case OPCODE_DP3:
				i += translateDP3(prog, i);
				break;
			case OPCODE_DPH:
				i += translateDPH(prog, i);
				break;
			case OPCODE_FLR:
				i += translateFLR(prog, i);
				break;
			case OPCODE_SUB:
				i += translateSUB(prog, i);
				break;
			case OPCODE_SWZ:
				i += translateSWZ(prog, i);
				break;
			case OPCODE_XPD:
				i += translateXPD(prog, i);
				break;
			default:
				break;
		}
	}
}

#define ADD_OUTPUT(fp_attr, vp_result) \
	do { \
		if ((FpReads & (1 << (fp_attr))) && !(prog->OutputsWritten & (1 << (vp_result)))) { \
			OutputsAdded |= 1 << (vp_result); \
			count++; \
		} \
	} while (0)

static void addArtificialOutputs(GLcontext *ctx, struct gl_program *prog)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	GLuint OutputsAdded, FpReads;
	int i, count;

	OutputsAdded = 0;
	count = 0;
	FpReads = r300->selected_fp->Base->InputsRead;

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

		_mesa_insert_instructions(prog, prog->NumInstructions - 1, count);
		inst = &prog->Instructions[prog->NumInstructions - 1 - count];

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

		prog->OutputsWritten |= OutputsAdded;
	}
}

#undef ADD_OUTPUT

static GLuint getUsedComponents(const GLuint swizzle)
{
	GLuint ret;

	ret = 0;

	/* need to mask out ZERO, ONE and NIL swizzles */
	ret |= 1 << (GET_SWZ(swizzle, SWIZZLE_X) & 0x3);
	ret |= 1 << (GET_SWZ(swizzle, SWIZZLE_Y) & 0x3);
	ret |= 1 << (GET_SWZ(swizzle, SWIZZLE_Z) & 0x3);
	ret |= 1 << (GET_SWZ(swizzle, SWIZZLE_W) & 0x3);

	return ret;
}

static GLuint trackUsedComponents(const struct gl_program *prog, const GLuint attrib)
{
	struct prog_instruction *inst;
	int tmp, i;
	GLuint ret;

	inst = prog->Instructions;
	ret = 0;
	while (inst->Opcode != OPCODE_END) {
		tmp = _mesa_num_inst_src_regs(inst->Opcode);
		for (i = 0; i < tmp; ++i) {
			if (inst->SrcReg[i].File == PROGRAM_INPUT && inst->SrcReg[i].Index == attrib) {
				ret |= getUsedComponents(inst->SrcReg[i].Swizzle);
			}
		}
		++inst;
	}

	return ret;
}

static void nqssadceInit(struct nqssadce_state* s)
{
	r300ContextPtr r300 = R300_CONTEXT(s->Ctx);
	GLuint fp_reads;

	fp_reads = r300->selected_fp->Base->InputsRead;
	{
		GLuint tmp;

		if (fp_reads & FRAG_BIT_COL0) {
				tmp = trackUsedComponents(r300->selected_fp->Base, FRAG_ATTRIB_COL0);
				s->Outputs[VERT_RESULT_COL0].Sourced = tmp;
				s->Outputs[VERT_RESULT_BFC0].Sourced = tmp;
		}

		if (fp_reads & FRAG_BIT_COL1) {
				tmp = trackUsedComponents(r300->selected_fp->Base, FRAG_ATTRIB_COL1);
				s->Outputs[VERT_RESULT_COL1].Sourced = tmp;
				s->Outputs[VERT_RESULT_BFC1].Sourced = tmp;
		}
	}

	{
		int i;
		for (i = 0; i < 8; ++i) {
			if (fp_reads & FRAG_BIT_TEX(i)) {
				s->Outputs[VERT_RESULT_TEX0 + i].Sourced = trackUsedComponents(r300->selected_fp->Base, FRAG_ATTRIB_TEX0 + i);
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

static struct r300_vertex_program *build_program(GLcontext *ctx,
						 struct r300_vertex_program_key *wanted_key,
						 const struct gl_vertex_program *mesa_vp)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_program *vp;
	struct gl_program *prog;

	vp = _mesa_calloc(sizeof(*vp));
	vp->Base = (struct gl_vertex_program *) _mesa_clone_program(ctx, &mesa_vp->Base);
	_mesa_memcpy(&vp->key, wanted_key, sizeof(vp->key));

	prog = &vp->Base->Base;

	if (RADEON_DEBUG & DEBUG_VERTS) {
		fprintf(stderr, "Initial vertex program:\n");
		_mesa_print_program(prog);
		fflush(stdout);
	}

	if (vp->Base->IsPositionInvariant) {
		_mesa_insert_mvp_code(ctx, vp->Base);
	}

	if (r300->selected_fp->wpos_attr != FRAG_ATTRIB_MAX) {
		pos_as_texcoord(&vp->Base->Base, r300->selected_fp->wpos_attr - FRAG_ATTRIB_TEX0);
	}

	if (r300->selected_fp->fog_attr != FRAG_ATTRIB_MAX) {
		fog_as_texcoord(&vp->Base->Base, r300->selected_fp->fog_attr - FRAG_ATTRIB_TEX0);
	}

	addArtificialOutputs(ctx, prog);

	translateInsts(prog);

	if (RADEON_DEBUG & DEBUG_VERTS) {
		fprintf(stderr, "Vertex program after native rewrite:\n");
		_mesa_print_program(prog);
		fflush(stdout);
	}

	{
		struct radeon_nqssadce_descr nqssadce = {
			.Init = &nqssadceInit,
			.IsNativeSwizzle = &swizzleIsNative,
			.BuildSwizzle = NULL
		};
		radeonNqssaDce(ctx, prog, &nqssadce);

		/* We need this step for reusing temporary registers */
		_mesa_optimize_program(ctx, prog);

		if (RADEON_DEBUG & DEBUG_VERTS) {
			fprintf(stderr, "Vertex program after NQSSADCE:\n");
			_mesa_print_program(prog);
			fflush(stdout);
		}
	}

	assert(prog->NumInstructions);
	{
		struct prog_instruction *inst;
		int max, i, tmp;

		inst = prog->Instructions;
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
		vp->num_temporaries = max + 1;
	}

	return vp;
}

struct r300_vertex_program * r300SelectVertexShader(GLcontext *ctx)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	struct r300_vertex_program_key wanted_key = { 0 };
	struct r300_vertex_program_cont *vpc;
	struct r300_vertex_program *vp;

	vpc = (struct r300_vertex_program_cont *)ctx->VertexProgram._Current;
	wanted_key.FpReads = r300->selected_fp->Base->InputsRead;
	wanted_key.FogAttr = r300->selected_fp->fog_attr;
	wanted_key.WPosAttr = r300->selected_fp->wpos_attr;

	for (vp = vpc->progs; vp; vp = vp->next) {
		if (_mesa_memcmp(&vp->key, &wanted_key, sizeof(wanted_key))
		    == 0) {
			return r300->selected_vp = vp;
		}
	}

	vp = build_program(ctx, &wanted_key, &vpc->mesa_program);
	vp->next = vpc->progs;
	vpc->progs = vp;

	return r300->selected_vp = vp;
}

#define bump_vpu_count(ptr, new_count)   do { \
		drm_r300_cmd_header_t* _p=((drm_r300_cmd_header_t*)(ptr)); \
		int _nc=(new_count)/4; \
		assert(_nc < 256); \
		if(_nc>_p->vpu.count)_p->vpu.count=_nc; \
	} while(0)

static void r300EmitVertexProgram(r300ContextPtr r300, int dest, struct r300_vertex_shader_hw_code *code)
{
	int i;

	assert((code->length > 0) && (code->length % 4 == 0));

	switch ((dest >> 8) & 0xf) {
		case 0:
			R300_STATECHANGE(r300, vpi);
			for (i = 0; i < code->length; i++)
				r300->hw.vpi.cmd[R300_VPI_INSTR_0 + i + 4 * (dest & 0xff)] = (code->body.d[i]);
			bump_vpu_count(r300->hw.vpi.cmd, code->length + 4 * (dest & 0xff));
			break;
		case 2:
			R300_STATECHANGE(r300, vpp);
			for (i = 0; i < code->length; i++)
				r300->hw.vpp.cmd[R300_VPP_PARAM_0 + i + 4 * (dest & 0xff)] = (code->body.d[i]);
			bump_vpu_count(r300->hw.vpp.cmd, code->length + 4 * (dest & 0xff));
			break;
		case 4:
			R300_STATECHANGE(r300, vps);
			for (i = 0; i < code->length; i++)
				r300->hw.vps.cmd[1 + i + 4 * (dest & 0xff)] = (code->body.d[i]);
			bump_vpu_count(r300->hw.vps.cmd, code->length + 4 * (dest & 0xff));
			break;
		default:
			fprintf(stderr, "%s:%s don't know how to handle dest %04x\n", __FILE__, __FUNCTION__, dest);
			_mesa_exit(-1);
	}
}

void r300SetupVertexProgram(r300ContextPtr rmesa)
{
	GLcontext *ctx = rmesa->radeon.glCtx;
	struct r300_vertex_program *prog = rmesa->selected_vp;
	int inst_count = 0;
	int param_count = 0;
	
	/* Reset state, in case we don't use something */
	((drm_r300_cmd_header_t *) rmesa->hw.vpp.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vpi.cmd)->vpu.count = 0;
	((drm_r300_cmd_header_t *) rmesa->hw.vps.cmd)->vpu.count = 0;
	
	R300_STATECHANGE(rmesa, vpp);
	param_count = r300VertexProgUpdateParams(ctx, prog->Base, (float *)&rmesa->hw.vpp.cmd[R300_VPP_PARAM_0]);
	bump_vpu_count(rmesa->hw.vpp.cmd, param_count);
	param_count /= 4;

	r300EmitVertexProgram(rmesa, R300_PVS_CODE_START, &(prog->hw_code));
	inst_count = (prog->hw_code.length / 4) - 1;

	r300VapCntl(rmesa, _mesa_bitcount(prog->Base->Base.InputsRead),
				 _mesa_bitcount(prog->Base->Base.OutputsWritten), prog->num_temporaries);

	R300_STATECHANGE(rmesa, pvs);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_1] = (0 << R300_PVS_FIRST_INST_SHIFT) | (inst_count << R300_PVS_XYZW_VALID_INST_SHIFT) |
				(inst_count << R300_PVS_LAST_INST_SHIFT);

	rmesa->hw.pvs.cmd[R300_PVS_CNTL_2] = (0 << R300_PVS_CONST_BASE_OFFSET_SHIFT) | (param_count << R300_PVS_MAX_CONST_ADDR_SHIFT);
	rmesa->hw.pvs.cmd[R300_PVS_CNTL_3] = (inst_count << R300_PVS_LAST_VTX_SRC_INST_SHIFT);
}
