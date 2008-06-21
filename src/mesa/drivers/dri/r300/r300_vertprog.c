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
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "tnl/tnl.h"

#include "r300_context.h"

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
			vp->native = GL_FALSE; \
		} \
		u_temp_i=VSF_MAX_FRAGMENT_TEMPS-1; \
	} while (0)

int r300VertexProgUpdateParams(GLcontext * ctx,
			       struct r300_vertex_program_cont *vp, float *dst)
{
	int pi;
	struct gl_vertex_program *mesa_vp = &vp->mesa_program;
	float *dst_o = dst;
	struct gl_program_parameter_list *paramList;

	if (mesa_vp->IsNVProgram) {
		_mesa_load_tracked_matrices(ctx);

		for (pi = 0; pi < MAX_NV_VERTEX_PROGRAM_PARAMS; pi++) {
			*dst++ = ctx->VertexProgram.Parameters[pi][0];
			*dst++ = ctx->VertexProgram.Parameters[pi][1];
			*dst++ = ctx->VertexProgram.Parameters[pi][2];
			*dst++ = ctx->VertexProgram.Parameters[pi][3];
		}
		return dst - dst_o;
	}

	assert(mesa_vp->Base.Parameters);
	_mesa_load_state_parameters(ctx, mesa_vp->Base.Parameters);

	if (mesa_vp->Base.Parameters->NumParameters * 4 >
	    VSF_MAX_FRAGMENT_LENGTH) {
		fprintf(stderr, "%s:Params exhausted\n", __FUNCTION__);
		_mesa_exit(-1);
	}

	paramList = mesa_vp->Base.Parameters;
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

static unsigned long t_dst_class(enum register_file file)
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

static unsigned long t_src_class(enum register_file file)
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
	int i;
	int max_reg = -1;

	if (src->File == PROGRAM_INPUT) {
		if (vp->inputs[src->Index] != -1)
			return vp->inputs[src->Index];

		for (i = 0; i < VERT_ATTRIB_MAX; i++)
			if (vp->inputs[i] > max_reg)
				max_reg = vp->inputs[i];

		vp->inputs[src->Index] = max_reg + 1;

		//vp_dump_inputs(vp, __FUNCTION__);

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
	/* src->NegateBase uses the NEGATE_ flags from program_instruction.h,
	 * which equal our VSF_FLAGS_ values, so it's safe to just pass it here.
	 */
	return PVS_SRC_OPERAND(t_src_index(vp, src),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 1)),
			       t_swizzle(GET_SWZ(src->Swizzle, 2)),
			       t_swizzle(GET_SWZ(src->Swizzle, 3)),
			       t_src_class(src->File),
			       src->NegateBase) | (src->RelAddr << 4);
}

static unsigned long t_src_scalar(struct r300_vertex_program *vp,
				  struct prog_src_register *src)
{
	/* src->NegateBase uses the NEGATE_ flags from program_instruction.h,
	 * which equal our VSF_FLAGS_ values, so it's safe to just pass it here.
	 */
	return PVS_SRC_OPERAND(t_src_index(vp, src),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_swizzle(GET_SWZ(src->Swizzle, 0)),
			       t_src_class(src->File),
			       src->
			       NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
				   NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
				  src[0].
				  NegateBase ? VSF_FLAG_XYZ : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] =
	    PVS_SRC_OPERAND(t_src_index(vp, &src[1]),
			    t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
			    t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
			    t_swizzle(GET_SWZ(src[1].Swizzle, 2)), SWIZZLE_ZERO,
			    t_src_class(src[1].File),
			    src[1].
			    NegateBase ? VSF_FLAG_XYZ : VSF_FLAG_NONE) |
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
				  src[0].
				  NegateBase ? VSF_FLAG_XYZ : VSF_FLAG_NONE) |
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
				   NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE
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
				  src[0].
				  NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
				  src[0].
				  NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  t_src_class(src[0].File),
				  src[0].
				  NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[3] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  PVS_SRC_SELECT_FORCE_0,	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  t_src_class(src[0].File),
				  src[0].
				  NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
				   NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
				   NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
				  src[0].
				  NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[0].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[1]), t_swizzle(GET_SWZ(src[1].Swizzle, 2)),	// Z
				  t_swizzle(GET_SWZ(src[1].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(src[1].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[1].Swizzle, 3)),	// W
				  t_src_class(src[1].File),
				  src[1].
				  NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
				   NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
	    (src[1].RelAddr << 4);
	inst[2] = PVS_SRC_OPERAND(t_src_index(vp, &src[0]), t_swizzle(GET_SWZ(src[0].Swizzle, 2)),	// Z
				  t_swizzle(GET_SWZ(src[0].Swizzle, 0)),	// X
				  t_swizzle(GET_SWZ(src[0].Swizzle, 1)),	// Y
				  t_swizzle(GET_SWZ(src[0].Swizzle, 3)),	// W
				  t_src_class(src[0].File),
				  src[0].
				  NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) |
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
	int cur_reg = 0;

	for (i = 0; i < VERT_ATTRIB_MAX; i++)
		vp->inputs[i] = -1;

	for (i = 0; i < VERT_RESULT_MAX; i++)
		vp->outputs[i] = -1;

	assert(vp->key.OutputsWritten & (1 << VERT_RESULT_HPOS));

	if (vp->key.OutputsWritten & (1 << VERT_RESULT_HPOS)) {
		vp->outputs[VERT_RESULT_HPOS] = cur_reg++;
	}

	if (vp->key.OutputsWritten & (1 << VERT_RESULT_PSIZ)) {
		vp->outputs[VERT_RESULT_PSIZ] = cur_reg++;
	}

	if (vp->key.OutputsWritten & (1 << VERT_RESULT_COL0)) {
		vp->outputs[VERT_RESULT_COL0] = cur_reg++;
	}

	if (vp->key.OutputsWritten & (1 << VERT_RESULT_COL1)) {
		vp->outputs[VERT_RESULT_COL1] =
		    vp->outputs[VERT_RESULT_COL0] + 1;
		cur_reg = vp->outputs[VERT_RESULT_COL1] + 1;
	}

	if (vp->key.OutputsWritten & (1 << VERT_RESULT_BFC0)) {
		vp->outputs[VERT_RESULT_BFC0] =
		    vp->outputs[VERT_RESULT_COL0] + 2;
		cur_reg = vp->outputs[VERT_RESULT_BFC0] + 2;
	}

	if (vp->key.OutputsWritten & (1 << VERT_RESULT_BFC1)) {
		vp->outputs[VERT_RESULT_BFC1] =
		    vp->outputs[VERT_RESULT_COL0] + 3;
		cur_reg = vp->outputs[VERT_RESULT_BFC1] + 1;
	}
#if 0
	if (vp->key.OutputsWritten & (1 << VERT_RESULT_FOGC)) {
		vp->outputs[VERT_RESULT_FOGC] = cur_reg++;
	}
#endif

	for (i = VERT_RESULT_TEX0; i <= VERT_RESULT_TEX7; i++) {
		if (vp->key.OutputsWritten & (1 << i)) {
			vp->outputs[i] = cur_reg++;
		}
	}
}

static void r300TranslateVertexShader(struct r300_vertex_program *vp,
				      struct prog_instruction *vpi)
{
	int i;
	GLuint *inst;
	unsigned long num_operands;
	/* Initial value should be last tmp reg that hw supports.
	   Strangely enough r300 doesnt mind even though these would be out of range.
	   Smart enough to realize that it doesnt need it? */
	int u_temp_i = VSF_MAX_FRAGMENT_TEMPS - 1;
	struct prog_src_register src[3];

	vp->pos_end = 0;	/* Not supported yet */
	vp->program.length = 0;
	/*vp->num_temporaries=mesa_vp->Base.NumTemporaries; */
	vp->translated = GL_TRUE;
	vp->native = GL_TRUE;

	t_inputs_outputs(vp);

	for (inst = vp->program.body.i; vpi->Opcode != OPCODE_END;
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
			assert(0);
			break;
		}
	}

	/* Some outputs may be artificially added, to match the inputs
	   of the fragment program. Blank the outputs here. */
	for (i = 0; i < VERT_RESULT_MAX; i++) {
		if (vp->key.OutputsAdded & (1 << i)) {
			inst[0] = PVS_OP_DST_OPERAND(VE_ADD,
						     GL_FALSE,
						     GL_FALSE,
						     vp->outputs[i],
						     VSF_FLAG_ALL,
						     PVS_DST_REG_OUT);
			inst[1] = __CONST(0, SWIZZLE_ZERO);
			inst[2] = __CONST(0, SWIZZLE_ZERO);
			inst[3] = __CONST(0, SWIZZLE_ZERO);
			inst += 4;
		}
	}

	vp->program.length = (inst - vp->program.body.i);
	if (vp->program.length >= VSF_MAX_FRAGMENT_LENGTH) {
		vp->program.length = 0;
		vp->native = GL_FALSE;
	}
#if 0
	fprintf(stderr, "hw program:\n");
	for (i = 0; i < vp->program.length; i++)
		fprintf(stderr, "%08x\n", vp->program.body.d[i]);
#endif
}

/* DP4 version seems to trigger some hw peculiarity */
//#define PREFER_DP4

static void position_invariant(struct gl_program *prog)
{
	struct prog_instruction *vpi;
	struct gl_program_parameter_list *paramList;
	int i;

	gl_state_index tokens[STATE_LENGTH] = { STATE_MVP_MATRIX, 0, 0, 0, 0 };

	/* tokens[4] = matrix modifier */
#ifdef PREFER_DP4
	tokens[4] = 0;		/* not transposed or inverted */
#else
	tokens[4] = STATE_MATRIX_TRANSPOSE;
#endif
	paramList = prog->Parameters;

	vpi = _mesa_alloc_instructions(prog->NumInstructions + 4);
	_mesa_init_instructions(vpi, prog->NumInstructions + 4);

	for (i = 0; i < 4; i++) {
		GLint idx;
		tokens[2] = tokens[3] = i;	/* matrix row[i]..row[i] */
		idx = _mesa_add_state_reference(paramList, tokens);
#ifdef PREFER_DP4
		vpi[i].Opcode = OPCODE_DP4;
		vpi[i].StringPos = 0;
		vpi[i].Data = 0;

		vpi[i].DstReg.File = PROGRAM_OUTPUT;
		vpi[i].DstReg.Index = VERT_RESULT_HPOS;
		vpi[i].DstReg.WriteMask = 1 << i;
		vpi[i].DstReg.CondMask = COND_TR;

		vpi[i].SrcReg[0].File = PROGRAM_STATE_VAR;
		vpi[i].SrcReg[0].Index = idx;
		vpi[i].SrcReg[0].Swizzle = SWIZZLE_XYZW;

		vpi[i].SrcReg[1].File = PROGRAM_INPUT;
		vpi[i].SrcReg[1].Index = VERT_ATTRIB_POS;
		vpi[i].SrcReg[1].Swizzle = SWIZZLE_XYZW;
#else
		if (i == 0)
			vpi[i].Opcode = OPCODE_MUL;
		else
			vpi[i].Opcode = OPCODE_MAD;

		vpi[i].StringPos = 0;
		vpi[i].Data = 0;

		if (i == 3)
			vpi[i].DstReg.File = PROGRAM_OUTPUT;
		else
			vpi[i].DstReg.File = PROGRAM_TEMPORARY;
		vpi[i].DstReg.Index = 0;
		vpi[i].DstReg.WriteMask = 0xf;
		vpi[i].DstReg.CondMask = COND_TR;

		vpi[i].SrcReg[0].File = PROGRAM_STATE_VAR;
		vpi[i].SrcReg[0].Index = idx;
		vpi[i].SrcReg[0].Swizzle = SWIZZLE_XYZW;

		vpi[i].SrcReg[1].File = PROGRAM_INPUT;
		vpi[i].SrcReg[1].Index = VERT_ATTRIB_POS;
		vpi[i].SrcReg[1].Swizzle = MAKE_SWIZZLE4(i, i, i, i);

		if (i > 0) {
			vpi[i].SrcReg[2].File = PROGRAM_TEMPORARY;
			vpi[i].SrcReg[2].Index = 0;
			vpi[i].SrcReg[2].Swizzle = SWIZZLE_XYZW;
		}
#endif
	}

	_mesa_copy_instructions(&vpi[i], prog->Instructions,
				prog->NumInstructions);

	free(prog->Instructions);

	prog->Instructions = vpi;

	prog->NumInstructions += 4;
	vpi = &prog->Instructions[prog->NumInstructions - 1];

	assert(vpi->Opcode == OPCODE_END);
}

static void insert_wpos(struct r300_vertex_program *vp, struct gl_program *prog,
			GLuint temp_index)
{
	struct prog_instruction *vpi;
	struct prog_instruction *vpi_insert;
	int i = 0;

	vpi = _mesa_alloc_instructions(prog->NumInstructions + 2);
	_mesa_init_instructions(vpi, prog->NumInstructions + 2);
	/* all but END */
	_mesa_copy_instructions(vpi, prog->Instructions,
				prog->NumInstructions - 1);
	/* END */
	_mesa_copy_instructions(&vpi[prog->NumInstructions + 1],
				&prog->Instructions[prog->NumInstructions - 1],
				1);
	vpi_insert = &vpi[prog->NumInstructions - 1];

	vpi_insert[i].Opcode = OPCODE_MOV;

	vpi_insert[i].DstReg.File = PROGRAM_OUTPUT;
	vpi_insert[i].DstReg.Index = VERT_RESULT_HPOS;
	vpi_insert[i].DstReg.WriteMask = WRITEMASK_XYZW;
	vpi_insert[i].DstReg.CondMask = COND_TR;

	vpi_insert[i].SrcReg[0].File = PROGRAM_TEMPORARY;
	vpi_insert[i].SrcReg[0].Index = temp_index;
	vpi_insert[i].SrcReg[0].Swizzle = SWIZZLE_XYZW;
	i++;

	vpi_insert[i].Opcode = OPCODE_MOV;

	vpi_insert[i].DstReg.File = PROGRAM_OUTPUT;
	vpi_insert[i].DstReg.Index = VERT_RESULT_TEX0 + vp->wpos_idx;
	vpi_insert[i].DstReg.WriteMask = WRITEMASK_XYZW;
	vpi_insert[i].DstReg.CondMask = COND_TR;

	vpi_insert[i].SrcReg[0].File = PROGRAM_TEMPORARY;
	vpi_insert[i].SrcReg[0].Index = temp_index;
	vpi_insert[i].SrcReg[0].Swizzle = SWIZZLE_XYZW;
	i++;

	free(prog->Instructions);

	prog->Instructions = vpi;

	prog->NumInstructions += i;
	vpi = &prog->Instructions[prog->NumInstructions - 1];

	assert(vpi->Opcode == OPCODE_END);
}

static void pos_as_texcoord(struct r300_vertex_program *vp,
			    struct gl_program *prog)
{
	struct prog_instruction *vpi;
	GLuint tempregi = prog->NumTemporaries;
	/* should do something else if no temps left... */
	prog->NumTemporaries++;

	for (vpi = prog->Instructions; vpi->Opcode != OPCODE_END; vpi++) {
		if (vpi->DstReg.File == PROGRAM_OUTPUT
		    && vpi->DstReg.Index == VERT_RESULT_HPOS) {
			vpi->DstReg.File = PROGRAM_TEMPORARY;
			vpi->DstReg.Index = tempregi;
		}
	}
	insert_wpos(vp, prog, tempregi);
}

static struct r300_vertex_program *build_program(struct r300_vertex_program_key
						 *wanted_key, struct gl_vertex_program
						 *mesa_vp, GLint wpos_idx)
{
	struct r300_vertex_program *vp;

	vp = _mesa_calloc(sizeof(*vp));
	_mesa_memcpy(&vp->key, wanted_key, sizeof(vp->key));
	vp->wpos_idx = wpos_idx;

	if (mesa_vp->IsPositionInvariant) {
		position_invariant(&mesa_vp->Base);
	}

	if (wpos_idx > -1) {
		pos_as_texcoord(vp, &mesa_vp->Base);
	}

	assert(mesa_vp->Base.NumInstructions);
	vp->num_temporaries = mesa_vp->Base.NumTemporaries;
	r300TranslateVertexShader(vp, mesa_vp->Base.Instructions);

	return vp;
}

static void add_outputs(struct r300_vertex_program_key *key, GLint vert)
{
	if (key->OutputsWritten & (1 << vert))
		return;

	key->OutputsWritten |= 1 << vert;
	key->OutputsAdded |= 1 << vert;
}

void r300SelectVertexShader(r300ContextPtr r300)
{
	GLcontext *ctx = ctx = r300->radeon.glCtx;
	GLuint InputsRead;
	struct r300_vertex_program_key wanted_key = { 0 };
	GLint i;
	struct r300_vertex_program_cont *vpc;
	struct r300_vertex_program *vp;
	GLint wpos_idx;

	vpc = (struct r300_vertex_program_cont *)ctx->VertexProgram._Current;
	wanted_key.InputsRead = vpc->mesa_program.Base.InputsRead;
	wanted_key.OutputsWritten = vpc->mesa_program.Base.OutputsWritten;
	InputsRead = ctx->FragmentProgram._Current->Base.InputsRead;

	wpos_idx = -1;
	if (InputsRead & FRAG_BIT_WPOS) {
		for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
			if (!(InputsRead & (FRAG_BIT_TEX0 << i)))
				break;

		if (i == ctx->Const.MaxTextureUnits) {
			fprintf(stderr, "\tno free texcoord found\n");
			_mesa_exit(-1);
		}

		wanted_key.OutputsWritten |= 1 << (VERT_RESULT_TEX0 + i);
		wpos_idx = i;
	}

	add_outputs(&wanted_key, VERT_RESULT_HPOS);

	if (InputsRead & FRAG_BIT_COL0) {
		add_outputs(&wanted_key, VERT_RESULT_COL0);
	}

	if (InputsRead & FRAG_BIT_COL1) {
		add_outputs(&wanted_key, VERT_RESULT_COL1);
	}

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			add_outputs(&wanted_key, VERT_RESULT_TEX0 + i);
		}
	}

	if (vpc->mesa_program.IsPositionInvariant) {
		/* we wan't position don't we ? */
		wanted_key.InputsRead |= (1 << VERT_ATTRIB_POS);
	}

	for (vp = vpc->progs; vp; vp = vp->next)
		if (_mesa_memcmp(&vp->key, &wanted_key, sizeof(wanted_key))
		    == 0) {
			r300->selected_vp = vp;
			return;
		}
	//_mesa_print_program(&vpc->mesa_program.Base);

	vp = build_program(&wanted_key, &vpc->mesa_program, wpos_idx);
	vp->next = vpc->progs;
	vpc->progs = vp;
	r300->selected_vp = vp;
}
