/*
 * Copyright (C) 2008 Nicolai Haehnle.
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
 * @file
 *
 * Shareable transformations that transform "special" ALU instructions
 * into ALU instructions that are supported by hardware.
 *
 */

#include "radeon_program_alu.h"

#include "radeon_compiler.h"


static struct rc_instruction *emit1(
	struct radeon_compiler * c, struct rc_instruction * after,
	gl_inst_opcode Opcode, GLuint Saturate, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg)
{
	struct rc_instruction *fpi = rc_insert_new_instruction(c, after);

	fpi->I.Opcode = Opcode;
	fpi->I.SaturateMode = Saturate;
	fpi->I.DstReg = DstReg;
	fpi->I.SrcReg[0] = SrcReg;
	return fpi;
}

static struct rc_instruction *emit2(
	struct radeon_compiler * c, struct rc_instruction * after,
	gl_inst_opcode Opcode, GLuint Saturate, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg0, struct prog_src_register SrcReg1)
{
	struct rc_instruction *fpi = rc_insert_new_instruction(c, after);

	fpi->I.Opcode = Opcode;
	fpi->I.SaturateMode = Saturate;
	fpi->I.DstReg = DstReg;
	fpi->I.SrcReg[0] = SrcReg0;
	fpi->I.SrcReg[1] = SrcReg1;
	return fpi;
}

static struct rc_instruction *emit3(
	struct radeon_compiler * c, struct rc_instruction * after,
	gl_inst_opcode Opcode, GLuint Saturate, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg0, struct prog_src_register SrcReg1,
	struct prog_src_register SrcReg2)
{
	struct rc_instruction *fpi = rc_insert_new_instruction(c, after);

	fpi->I.Opcode = Opcode;
	fpi->I.SaturateMode = Saturate;
	fpi->I.DstReg = DstReg;
	fpi->I.SrcReg[0] = SrcReg0;
	fpi->I.SrcReg[1] = SrcReg1;
	fpi->I.SrcReg[2] = SrcReg2;
	return fpi;
}

static struct prog_dst_register dstreg(int file, int index)
{
	struct prog_dst_register dst;
	dst.File = file;
	dst.Index = index;
	dst.WriteMask = WRITEMASK_XYZW;
	dst.CondMask = COND_TR;
	dst.RelAddr = 0;
	dst.CondSwizzle = SWIZZLE_NOOP;
	dst.CondSrc = 0;
	dst.pad = 0;
	return dst;
}

static struct prog_dst_register dstregtmpmask(int index, int mask)
{
	struct prog_dst_register dst = {0};
	dst.File = PROGRAM_TEMPORARY;
	dst.Index = index;
	dst.WriteMask = mask;
	dst.RelAddr = 0;
	dst.CondMask = COND_TR;
	dst.CondSwizzle = SWIZZLE_NOOP;
	dst.CondSrc = 0;
	dst.pad = 0;
	return dst;
}

static const struct prog_src_register builtin_zero = {
	.File = PROGRAM_BUILTIN,
	.Index = 0,
	.Swizzle = SWIZZLE_0000
};
static const struct prog_src_register builtin_one = {
	.File = PROGRAM_BUILTIN,
	.Index = 0,
	.Swizzle = SWIZZLE_1111
};
static const struct prog_src_register srcreg_undefined = {
	.File = PROGRAM_UNDEFINED,
	.Index = 0,
	.Swizzle = SWIZZLE_NOOP
};

static struct prog_src_register srcreg(int file, int index)
{
	struct prog_src_register src = srcreg_undefined;
	src.File = file;
	src.Index = index;
	return src;
}

static struct prog_src_register srcregswz(int file, int index, int swz)
{
	struct prog_src_register src = srcreg_undefined;
	src.File = file;
	src.Index = index;
	src.Swizzle = swz;
	return src;
}

static struct prog_src_register absolute(struct prog_src_register reg)
{
	struct prog_src_register newreg = reg;
	newreg.Abs = 1;
	newreg.Negate = NEGATE_NONE;
	return newreg;
}

static struct prog_src_register negate(struct prog_src_register reg)
{
	struct prog_src_register newreg = reg;
	newreg.Negate = newreg.Negate ^ NEGATE_XYZW;
	return newreg;
}

static struct prog_src_register swizzle(struct prog_src_register reg, GLuint x, GLuint y, GLuint z, GLuint w)
{
	struct prog_src_register swizzled = reg;
	swizzled.Swizzle = MAKE_SWIZZLE4(
		x >= 4 ? x : GET_SWZ(reg.Swizzle, x),
		y >= 4 ? y : GET_SWZ(reg.Swizzle, y),
		z >= 4 ? z : GET_SWZ(reg.Swizzle, z),
		w >= 4 ? w : GET_SWZ(reg.Swizzle, w));
	return swizzled;
}

static struct prog_src_register scalar(struct prog_src_register reg)
{
	return swizzle(reg, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X);
}

static void transform_ABS(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	struct prog_src_register src = inst->I.SrcReg[0];
	src.Abs = 1;
	src.Negate = NEGATE_NONE;
	emit1(c, inst->Prev, OPCODE_MOV, inst->I.SaturateMode, inst->I.DstReg, src);
	rc_remove_instruction(inst);
}

static void transform_DP3(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	struct prog_src_register src0 = inst->I.SrcReg[0];
	struct prog_src_register src1 = inst->I.SrcReg[1];
	src0.Negate &= ~NEGATE_W;
	src0.Swizzle &= ~(7 << (3 * 3));
	src0.Swizzle |= SWIZZLE_ZERO << (3 * 3);
	src1.Negate &= ~NEGATE_W;
	src1.Swizzle &= ~(7 << (3 * 3));
	src1.Swizzle |= SWIZZLE_ZERO << (3 * 3);
	emit2(c, inst->Prev, OPCODE_DP4, inst->I.SaturateMode, inst->I.DstReg, src0, src1);
	rc_remove_instruction(inst);
}

static void transform_DPH(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	struct prog_src_register src0 = inst->I.SrcReg[0];
	src0.Negate &= ~NEGATE_W;
	src0.Swizzle &= ~(7 << (3 * 3));
	src0.Swizzle |= SWIZZLE_ONE << (3 * 3);
	emit2(c, inst->Prev, OPCODE_DP4, inst->I.SaturateMode, inst->I.DstReg, src0, inst->I.SrcReg[1]);
	rc_remove_instruction(inst);
}

/**
 * [1, src0.y*src1.y, src0.z, src1.w]
 * So basically MUL with lotsa swizzling.
 */
static void transform_DST(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	emit2(c, inst->Prev, OPCODE_MUL, inst->I.SaturateMode, inst->I.DstReg,
		swizzle(inst->I.SrcReg[0], SWIZZLE_ONE, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE),
		swizzle(inst->I.SrcReg[1], SWIZZLE_ONE, SWIZZLE_Y, SWIZZLE_ONE, SWIZZLE_W));
	rc_remove_instruction(inst);
}

static void transform_FLR(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);
	emit1(c, inst->Prev, OPCODE_FRC, 0, dstreg(PROGRAM_TEMPORARY, tempreg), inst->I.SrcReg[0]);
	emit2(c, inst->Prev, OPCODE_ADD, inst->I.SaturateMode, inst->I.DstReg,
		inst->I.SrcReg[0], negate(srcreg(PROGRAM_TEMPORARY, tempreg)));
	rc_remove_instruction(inst);
}

/**
 * Definition of LIT (from ARB_fragment_program):
 *
 *  tmp = VectorLoad(op0);
 *  if (tmp.x < 0) tmp.x = 0;
 *  if (tmp.y < 0) tmp.y = 0;
 *  if (tmp.w < -(128.0-epsilon)) tmp.w = -(128.0-epsilon);
 *  else if (tmp.w > 128-epsilon) tmp.w = 128-epsilon;
 *  result.x = 1.0;
 *  result.y = tmp.x;
 *  result.z = (tmp.x > 0) ? RoughApproxPower(tmp.y, tmp.w) : 0.0;
 *  result.w = 1.0;
 *
 * The longest path of computation is the one leading to result.z,
 * consisting of 5 operations. This implementation of LIT takes
 * 5 slots, if the subsequent optimization passes are clever enough
 * to pair instructions correctly.
 */
static void transform_LIT(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	GLuint constant;
	GLuint constant_swizzle;
	GLuint temp;
	struct prog_src_register srctemp;

	constant = rc_constants_add_immediate_scalar(&c->Program.Constants, -127.999999, &constant_swizzle);

	if (inst->I.DstReg.WriteMask != WRITEMASK_XYZW || inst->I.DstReg.File != PROGRAM_TEMPORARY) {
		struct rc_instruction * inst_mov;

		inst_mov = emit1(c, inst,
			OPCODE_MOV, 0, inst->I.DstReg,
			srcreg(PROGRAM_TEMPORARY, rc_find_free_temporary(c)));

		inst->I.DstReg.File = PROGRAM_TEMPORARY;
		inst->I.DstReg.Index = inst_mov->I.SrcReg[0].Index;
		inst->I.DstReg.WriteMask = WRITEMASK_XYZW;
	}

	temp = inst->I.DstReg.Index;
	srctemp = srcreg(PROGRAM_TEMPORARY, temp);

	// tmp.x = max(0.0, Src.x);
	// tmp.y = max(0.0, Src.y);
	// tmp.w = clamp(Src.z, -128+eps, 128-eps);
	emit2(c, inst->Prev, OPCODE_MAX, 0,
		dstregtmpmask(temp, WRITEMASK_XYW),
		inst->I.SrcReg[0],
		swizzle(srcreg(PROGRAM_CONSTANT, constant),
			SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO, constant_swizzle&3));
	emit2(c, inst->Prev, OPCODE_MIN, 0,
		dstregtmpmask(temp, WRITEMASK_Z),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		negate(srcregswz(PROGRAM_CONSTANT, constant, constant_swizzle)));

	// tmp.w = Pow(tmp.y, tmp.w)
	emit1(c, inst->Prev, OPCODE_LG2, 0,
		dstregtmpmask(temp, WRITEMASK_W),
		swizzle(srctemp, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y));
	emit2(c, inst->Prev, OPCODE_MUL, 0,
		dstregtmpmask(temp, WRITEMASK_W),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		swizzle(srctemp, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z));
	emit1(c, inst->Prev, OPCODE_EX2, 0,
		dstregtmpmask(temp, WRITEMASK_W),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W));

	// tmp.z = (tmp.x > 0) ? tmp.w : 0.0
	emit3(c, inst->Prev, OPCODE_CMP, inst->I.SaturateMode,
		dstregtmpmask(temp, WRITEMASK_Z),
		negate(swizzle(srctemp, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		builtin_zero);

	// tmp.x, tmp.y, tmp.w = 1.0, tmp.x, 1.0
	emit1(c, inst->Prev, OPCODE_MOV, inst->I.SaturateMode,
		dstregtmpmask(temp, WRITEMASK_XYW),
		swizzle(srctemp, SWIZZLE_ONE, SWIZZLE_X, SWIZZLE_ONE, SWIZZLE_ONE));

	rc_remove_instruction(inst);
}

static void transform_LRP(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, OPCODE_ADD, 0,
		dstreg(PROGRAM_TEMPORARY, tempreg),
		inst->I.SrcReg[1], negate(inst->I.SrcReg[2]));
	emit3(c, inst->Prev, OPCODE_MAD, inst->I.SaturateMode,
		inst->I.DstReg,
		inst->I.SrcReg[0], srcreg(PROGRAM_TEMPORARY, tempreg), inst->I.SrcReg[2]);

	rc_remove_instruction(inst);
}

static void transform_POW(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);
	struct prog_dst_register tempdst = dstreg(PROGRAM_TEMPORARY, tempreg);
	struct prog_src_register tempsrc = srcreg(PROGRAM_TEMPORARY, tempreg);
	tempdst.WriteMask = WRITEMASK_W;
	tempsrc.Swizzle = SWIZZLE_WWWW;

	emit1(c, inst->Prev, OPCODE_LG2, 0, tempdst, scalar(inst->I.SrcReg[0]));
	emit2(c, inst->Prev, OPCODE_MUL, 0, tempdst, tempsrc, scalar(inst->I.SrcReg[1]));
	emit1(c, inst->Prev, OPCODE_EX2, inst->I.SaturateMode, inst->I.DstReg, tempsrc);

	rc_remove_instruction(inst);
}

static void transform_RSQ(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	inst->I.SrcReg[0] = absolute(inst->I.SrcReg[0]);
}

static void transform_SGE(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, OPCODE_ADD, 0, dstreg(PROGRAM_TEMPORARY, tempreg), inst->I.SrcReg[0], negate(inst->I.SrcReg[1]));
	emit3(c, inst->Prev, OPCODE_CMP, inst->I.SaturateMode, inst->I.DstReg,
		srcreg(PROGRAM_TEMPORARY, tempreg), builtin_zero, builtin_one);

	rc_remove_instruction(inst);
}

static void transform_SLT(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, OPCODE_ADD, 0, dstreg(PROGRAM_TEMPORARY, tempreg), inst->I.SrcReg[0], negate(inst->I.SrcReg[1]));
	emit3(c, inst->Prev, OPCODE_CMP, inst->I.SaturateMode, inst->I.DstReg,
		srcreg(PROGRAM_TEMPORARY, tempreg), builtin_one, builtin_zero);

	rc_remove_instruction(inst);
}

static void transform_SUB(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	inst->I.Opcode = OPCODE_ADD;
	inst->I.SrcReg[1] = negate(inst->I.SrcReg[1]);
}

static void transform_SWZ(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	inst->I.Opcode = OPCODE_MOV;
}

static void transform_XPD(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, OPCODE_MUL, 0, dstreg(PROGRAM_TEMPORARY, tempreg),
		swizzle(inst->I.SrcReg[0], SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W),
		swizzle(inst->I.SrcReg[1], SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W));
	emit3(c, inst->Prev, OPCODE_MAD, inst->I.SaturateMode, inst->I.DstReg,
		swizzle(inst->I.SrcReg[0], SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W),
		swizzle(inst->I.SrcReg[1], SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W),
		negate(srcreg(PROGRAM_TEMPORARY, tempreg)));

	rc_remove_instruction(inst);
}


/**
 * Can be used as a transformation for @ref radeonClauseLocalTransform,
 * no userData necessary.
 *
 * Eliminates the following ALU instructions:
 *  ABS, DPH, DST, FLR, LIT, LRP, POW, SGE, SLT, SUB, SWZ, XPD
 * using:
 *  MOV, ADD, MUL, MAD, FRC, DP3, LG2, EX2, CMP
 *
 * Transforms RSQ to Radeon's native RSQ by explicitly setting
 * absolute value.
 *
 * @note should be applicable to R300 and R500 fragment programs.
 */
GLboolean radeonTransformALU(
	struct radeon_compiler * c,
	struct rc_instruction* inst,
	void* unused)
{
	switch(inst->I.Opcode) {
	case OPCODE_ABS: transform_ABS(c, inst); return GL_TRUE;
	case OPCODE_DPH: transform_DPH(c, inst); return GL_TRUE;
	case OPCODE_DST: transform_DST(c, inst); return GL_TRUE;
	case OPCODE_FLR: transform_FLR(c, inst); return GL_TRUE;
	case OPCODE_LIT: transform_LIT(c, inst); return GL_TRUE;
	case OPCODE_LRP: transform_LRP(c, inst); return GL_TRUE;
	case OPCODE_POW: transform_POW(c, inst); return GL_TRUE;
	case OPCODE_RSQ: transform_RSQ(c, inst); return GL_TRUE;
	case OPCODE_SGE: transform_SGE(c, inst); return GL_TRUE;
	case OPCODE_SLT: transform_SLT(c, inst); return GL_TRUE;
	case OPCODE_SUB: transform_SUB(c, inst); return GL_TRUE;
	case OPCODE_SWZ: transform_SWZ(c, inst); return GL_TRUE;
	case OPCODE_XPD: transform_XPD(c, inst); return GL_TRUE;
	default:
		return GL_FALSE;
	}
}


static void transform_r300_vertex_ABS(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	/* Note: r500 can take absolute values, but r300 cannot. */
	inst->I.Opcode = OPCODE_MAX;
	inst->I.SrcReg[1] = inst->I.SrcReg[0];
	inst->I.SrcReg[1].Negate ^= NEGATE_XYZW;
}

/**
 * For use with radeonLocalTransform, this transforms non-native ALU
 * instructions of the r300 up to r500 vertex engine.
 */
GLboolean r300_transform_vertex_alu(
	struct radeon_compiler * c,
	struct rc_instruction* inst,
	void* unused)
{
	switch(inst->I.Opcode) {
	case OPCODE_ABS: transform_r300_vertex_ABS(c, inst); return GL_TRUE;
	case OPCODE_DP3: transform_DP3(c, inst); return GL_TRUE;
	case OPCODE_DPH: transform_DPH(c, inst); return GL_TRUE;
	case OPCODE_FLR: transform_FLR(c, inst); return GL_TRUE;
	case OPCODE_LRP: transform_LRP(c, inst); return GL_TRUE;
	case OPCODE_SUB: transform_SUB(c, inst); return GL_TRUE;
	case OPCODE_SWZ: transform_SWZ(c, inst); return GL_TRUE;
	case OPCODE_XPD: transform_XPD(c, inst); return GL_TRUE;
	default:
		return GL_FALSE;
	}
}

static void sincos_constants(struct radeon_compiler* c, GLuint *constants)
{
	static const GLfloat SinCosConsts[2][4] = {
		{
			1.273239545,		// 4/PI
			-0.405284735,		// -4/(PI*PI)
			3.141592654,		// PI
			0.2225			// weight
		},
		{
			0.75,
			0.5,
			0.159154943,		// 1/(2*PI)
			6.283185307		// 2*PI
		}
	};
	int i;

	for(i = 0; i < 2; ++i)
		constants[i] = rc_constants_add_immediate_vec4(&c->Program.Constants, SinCosConsts[i]);
}

/**
 * Approximate sin(x), where x is clamped to (-pi/2, pi/2).
 *
 * MUL tmp.xy, src, { 4/PI, -4/(PI^2) }
 * MAD tmp.x, tmp.y, |src|, tmp.x
 * MAD tmp.y, tmp.x, |tmp.x|, -tmp.x
 * MAD dest, tmp.y, weight, tmp.x
 */
static void sin_approx(
	struct radeon_compiler* c, struct rc_instruction * before,
	struct prog_dst_register dst, struct prog_src_register src, const GLuint* constants)
{
	GLuint tempreg = rc_find_free_temporary(c);

	emit2(c, before->Prev, OPCODE_MUL, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
		swizzle(src, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
		srcreg(PROGRAM_CONSTANT, constants[0]));
	emit3(c, before->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_X),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
		absolute(swizzle(src, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X));
	emit3(c, before->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_Y),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
		absolute(swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)),
		negate(swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)));
	emit3(c, before->Prev, OPCODE_MAD, 0, dst,
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
		swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X));
}

/**
 * Translate the trigonometric functions COS, SIN, and SCS
 * using only the basic instructions
 *  MOV, ADD, MUL, MAD, FRC
 */
GLboolean radeonTransformTrigSimple(struct radeon_compiler* c,
	struct rc_instruction* inst,
	void* unused)
{
	if (inst->I.Opcode != OPCODE_COS &&
	    inst->I.Opcode != OPCODE_SIN &&
	    inst->I.Opcode != OPCODE_SCS)
		return GL_FALSE;

	GLuint constants[2];
	GLuint tempreg = rc_find_free_temporary(c);

	sincos_constants(c, constants);

	if (inst->I.Opcode == OPCODE_COS) {
		// MAD tmp.x, src, 1/(2*PI), 0.75
		// FRC tmp.x, tmp.x
		// MAD tmp.z, tmp.x, 2*PI, -PI
		emit3(c, inst->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(inst->I.SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X));
		emit1(c, inst->Prev, OPCODE_FRC, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W));
		emit3(c, inst->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			negate(swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z)));

		sin_approx(c, inst, inst->I.DstReg,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			constants);
	} else if (inst->I.Opcode == OPCODE_SIN) {
		emit3(c, inst->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(inst->I.SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y));
		emit1(c, inst->Prev, OPCODE_FRC, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W));
		emit3(c, inst->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			negate(swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z)));

		sin_approx(c, inst, inst->I.DstReg,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			constants);
	} else {
		emit3(c, inst->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
			swizzle(inst->I.SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W));
		emit1(c, inst->Prev, OPCODE_FRC, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
			srcreg(PROGRAM_TEMPORARY, tempreg));
		emit3(c, inst->Prev, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
			srcreg(PROGRAM_TEMPORARY, tempreg),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			negate(swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z)));

		struct prog_dst_register dst = inst->I.DstReg;

		dst.WriteMask = inst->I.DstReg.WriteMask & WRITEMASK_X;
		sin_approx(c, inst, dst,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			constants);

		dst.WriteMask = inst->I.DstReg.WriteMask & WRITEMASK_Y;
		sin_approx(c, inst, dst,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
			constants);
	}

	rc_remove_instruction(inst);

	return GL_TRUE;
}


/**
 * Transform the trigonometric functions COS, SIN, and SCS
 * to include pre-scaling by 1/(2*PI) and taking the fractional
 * part, so that the input to COS and SIN is always in the range [0,1).
 * SCS is replaced by one COS and one SIN instruction.
 *
 * @warning This transformation implicitly changes the semantics of SIN and COS!
 */
GLboolean radeonTransformTrigScale(struct radeon_compiler* c,
	struct rc_instruction* inst,
	void* unused)
{
	if (inst->I.Opcode != OPCODE_COS &&
	    inst->I.Opcode != OPCODE_SIN &&
	    inst->I.Opcode != OPCODE_SCS)
		return GL_FALSE;

	static const GLfloat RCP_2PI = 0.15915494309189535;
	GLuint temp;
	GLuint constant;
	GLuint constant_swizzle;

	temp = rc_find_free_temporary(c);
	constant = rc_constants_add_immediate_scalar(&c->Program.Constants, RCP_2PI, &constant_swizzle);

	emit2(c, inst->Prev, OPCODE_MUL, 0, dstregtmpmask(temp, WRITEMASK_W),
		swizzle(inst->I.SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
		srcregswz(PROGRAM_CONSTANT, constant, constant_swizzle));
	emit1(c, inst->Prev, OPCODE_FRC, 0, dstregtmpmask(temp, WRITEMASK_W),
		srcreg(PROGRAM_TEMPORARY, temp));

	if (inst->I.Opcode == OPCODE_COS) {
		emit1(c, inst->Prev, OPCODE_COS, inst->I.SaturateMode, inst->I.DstReg,
			srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
	} else if (inst->I.Opcode == OPCODE_SIN) {
		emit1(c, inst->Prev, OPCODE_SIN, inst->I.SaturateMode,
			inst->I.DstReg, srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
	} else if (inst->I.Opcode == OPCODE_SCS) {
		struct prog_dst_register moddst = inst->I.DstReg;

		if (inst->I.DstReg.WriteMask & WRITEMASK_X) {
			moddst.WriteMask = WRITEMASK_X;
			emit1(c, inst->Prev, OPCODE_COS, inst->I.SaturateMode, moddst,
				srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
		}
		if (inst->I.DstReg.WriteMask & WRITEMASK_Y) {
			moddst.WriteMask = WRITEMASK_Y;
			emit1(c, inst->Prev, OPCODE_SIN, inst->I.SaturateMode, moddst,
				srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
		}
	}

	rc_remove_instruction(inst);

	return GL_TRUE;
}

/**
 * Rewrite DDX/DDY instructions to properly work with r5xx shaders.
 * The r5xx MDH/MDV instruction provides per-quad partial derivatives.
 * It takes the form A*B+C. A and C are set by setting src0. B should be -1.
 *
 * @warning This explicitly changes the form of DDX and DDY!
 */

GLboolean radeonTransformDeriv(struct radeon_compiler* c,
	struct rc_instruction* inst,
	void* unused)
{
	if (inst->I.Opcode != OPCODE_DDX && inst->I.Opcode != OPCODE_DDY)
		return GL_FALSE;

	inst->I.SrcReg[1].Swizzle = MAKE_SWIZZLE4(SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE);
	inst->I.SrcReg[1].Negate = NEGATE_XYZW;

	return GL_TRUE;
}
