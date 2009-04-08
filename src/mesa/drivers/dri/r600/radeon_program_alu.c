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

#include "shader/prog_parameter.h"


static struct prog_instruction *emit1(struct gl_program* p,
	gl_inst_opcode Opcode, GLuint Saturate, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg)
{
	struct prog_instruction *fpi = radeonAppendInstructions(p, 1);

	fpi->Opcode = Opcode;
	fpi->SaturateMode = Saturate;
	fpi->DstReg = DstReg;
	fpi->SrcReg[0] = SrcReg;
	return fpi;
}

static struct prog_instruction *emit2(struct gl_program* p,
	gl_inst_opcode Opcode, GLuint Saturate, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg0, struct prog_src_register SrcReg1)
{
	struct prog_instruction *fpi = radeonAppendInstructions(p, 1);

	fpi->Opcode = Opcode;
	fpi->SaturateMode = Saturate;
	fpi->DstReg = DstReg;
	fpi->SrcReg[0] = SrcReg0;
	fpi->SrcReg[1] = SrcReg1;
	return fpi;
}

static struct prog_instruction *emit3(struct gl_program* p,
	gl_inst_opcode Opcode, GLuint Saturate, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg0, struct prog_src_register SrcReg1,
	struct prog_src_register SrcReg2)
{
	struct prog_instruction *fpi = radeonAppendInstructions(p, 1);

	fpi->Opcode = Opcode;
	fpi->SaturateMode = Saturate;
	fpi->DstReg = DstReg;
	fpi->SrcReg[0] = SrcReg0;
	fpi->SrcReg[1] = SrcReg1;
	fpi->SrcReg[2] = SrcReg2;
	return fpi;
}

static void set_swizzle(struct prog_src_register *SrcReg, int coordinate, int swz)
{
	SrcReg->Swizzle &= ~(7 << (3*coordinate));
	SrcReg->Swizzle |= swz << (3*coordinate);
}

static void set_negate_base(struct prog_src_register *SrcReg, int coordinate, int negate)
{
	SrcReg->NegateBase &= ~(1 << coordinate);
	SrcReg->NegateBase |= (negate << coordinate);
}

static struct prog_dst_register dstreg(int file, int index)
{
	struct prog_dst_register dst;
	dst.File = file;
	dst.Index = index;
	dst.WriteMask = WRITEMASK_XYZW;
	dst.CondMask = COND_TR;
	dst.CondSwizzle = SWIZZLE_NOOP;
	dst.CondSrc = 0;
	dst.pad = 0;
	return dst;
}

static struct prog_dst_register dstregtmpmask(int index, int mask)
{
	struct prog_dst_register dst;
	dst.File = PROGRAM_TEMPORARY;
	dst.Index = index;
	dst.WriteMask = mask;
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
	newreg.NegateBase = 0;
	newreg.NegateAbs = 0;
	return newreg;
}

static struct prog_src_register negate(struct prog_src_register reg)
{
	struct prog_src_register newreg = reg;
	newreg.NegateAbs = !newreg.NegateAbs;
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

static void transform_ABS(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	struct prog_src_register src = inst->SrcReg[0];
	src.Abs = 1;
	src.NegateBase = 0;
	src.NegateAbs = 0;
	emit1(t->Program, OPCODE_MOV, inst->SaturateMode, inst->DstReg, src);
}

static void transform_DPH(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	struct prog_src_register src0 = inst->SrcReg[0];
	if (src0.NegateAbs) {
		if (src0.Abs) {
			int tempreg = radeonFindFreeTemporary(t);
			emit1(t->Program, OPCODE_MOV, 0, dstreg(PROGRAM_TEMPORARY, tempreg), src0);
			src0 = srcreg(src0.File, src0.Index);
		} else {
			src0.NegateAbs = 0;
			src0.NegateBase ^= NEGATE_XYZW;
		}
	}
	set_swizzle(&src0, 3, SWIZZLE_ONE);
	set_negate_base(&src0, 3, 0);
	emit2(t->Program, OPCODE_DP4, inst->SaturateMode, inst->DstReg, src0, inst->SrcReg[1]);
}

/**
 * [1, src0.y*src1.y, src0.z, src1.w]
 * So basically MUL with lotsa swizzling.
 */
static void transform_DST(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	emit2(t->Program, OPCODE_MUL, inst->SaturateMode, inst->DstReg,
		swizzle(inst->SrcReg[0], SWIZZLE_ONE, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE),
		swizzle(inst->SrcReg[1], SWIZZLE_ONE, SWIZZLE_Y, SWIZZLE_ONE, SWIZZLE_W));
}

static void transform_FLR(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	int tempreg = radeonFindFreeTemporary(t);
	emit1(t->Program, OPCODE_FRC, 0, dstreg(PROGRAM_TEMPORARY, tempreg), inst->SrcReg[0]);
	emit2(t->Program, OPCODE_ADD, inst->SaturateMode, inst->DstReg,
		inst->SrcReg[0], negate(srcreg(PROGRAM_TEMPORARY, tempreg)));
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
static void transform_LIT(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	static const GLfloat LitConst[4] = { -127.999999 };

	GLuint constant;
	GLuint constant_swizzle;
	GLuint temp;
	int needTemporary = 0;
	struct prog_src_register srctemp;

	constant = _mesa_add_unnamed_constant(t->Program->Parameters, LitConst, 1, &constant_swizzle);

	if (inst->DstReg.WriteMask != WRITEMASK_XYZW) {
		needTemporary = 1;
	} else if (inst->DstReg.File != PROGRAM_TEMPORARY) {
		// LIT is typically followed by DP3/DP4, so there's no point
		// in creating special code for this case
		needTemporary = 1;
	}

	if (needTemporary) {
		temp = radeonFindFreeTemporary(t);
	} else {
		temp = inst->DstReg.Index;
	}
	srctemp = srcreg(PROGRAM_TEMPORARY, temp);

	// tmp.x = max(0.0, Src.x);
	// tmp.y = max(0.0, Src.y);
	// tmp.w = clamp(Src.z, -128+eps, 128-eps);
	emit2(t->Program, OPCODE_MAX, 0,
		dstregtmpmask(temp, WRITEMASK_XYW),
		inst->SrcReg[0],
		swizzle(srcreg(PROGRAM_CONSTANT, constant),
			SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO, constant_swizzle&3));
	emit2(t->Program, OPCODE_MIN, 0,
		dstregtmpmask(temp, WRITEMASK_Z),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		negate(srcregswz(PROGRAM_CONSTANT, constant, constant_swizzle)));

	// tmp.w = Pow(tmp.y, tmp.w)
	emit1(t->Program, OPCODE_LG2, 0,
		dstregtmpmask(temp, WRITEMASK_W),
		swizzle(srctemp, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y));
	emit2(t->Program, OPCODE_MUL, 0,
		dstregtmpmask(temp, WRITEMASK_W),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		swizzle(srctemp, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z));
	emit1(t->Program, OPCODE_EX2, 0,
		dstregtmpmask(temp, WRITEMASK_W),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W));

	// tmp.z = (tmp.x > 0) ? tmp.w : 0.0
	emit3(t->Program, OPCODE_CMP, inst->SaturateMode,
		dstregtmpmask(temp, WRITEMASK_Z),
		negate(swizzle(srctemp, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)),
		swizzle(srctemp, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		builtin_zero);

	// tmp.x, tmp.y, tmp.w = 1.0, tmp.x, 1.0
	emit1(t->Program, OPCODE_MOV, inst->SaturateMode,
		dstregtmpmask(temp, WRITEMASK_XYW),
		swizzle(srctemp, SWIZZLE_ONE, SWIZZLE_X, SWIZZLE_ONE, SWIZZLE_ONE));

	if (needTemporary)
		emit1(t->Program, OPCODE_MOV, 0, inst->DstReg, srctemp);
}

static void transform_LRP(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	int tempreg = radeonFindFreeTemporary(t);

	emit2(t->Program, OPCODE_ADD, 0,
		dstreg(PROGRAM_TEMPORARY, tempreg),
		inst->SrcReg[1], negate(inst->SrcReg[2]));
	emit3(t->Program, OPCODE_MAD, inst->SaturateMode,
		inst->DstReg,
		inst->SrcReg[0], srcreg(PROGRAM_TEMPORARY, tempreg), inst->SrcReg[2]);
}

static void transform_POW(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	int tempreg = radeonFindFreeTemporary(t);
	struct prog_dst_register tempdst = dstreg(PROGRAM_TEMPORARY, tempreg);
	struct prog_src_register tempsrc = srcreg(PROGRAM_TEMPORARY, tempreg);
	tempdst.WriteMask = WRITEMASK_W;
	tempsrc.Swizzle = SWIZZLE_WWWW;

	emit1(t->Program, OPCODE_LG2, 0, tempdst, scalar(inst->SrcReg[0]));
	emit2(t->Program, OPCODE_MUL, 0, tempdst, tempsrc, scalar(inst->SrcReg[1]));
	emit1(t->Program, OPCODE_EX2, inst->SaturateMode, inst->DstReg, tempsrc);
}

static void transform_RSQ(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	emit1(t->Program, OPCODE_RSQ, inst->SaturateMode, inst->DstReg, absolute(inst->SrcReg[0]));
}

static void transform_SGE(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	int tempreg = radeonFindFreeTemporary(t);

	emit2(t->Program, OPCODE_ADD, 0, dstreg(PROGRAM_TEMPORARY, tempreg), inst->SrcReg[0], negate(inst->SrcReg[1]));
	emit3(t->Program, OPCODE_CMP, inst->SaturateMode, inst->DstReg,
		srcreg(PROGRAM_TEMPORARY, tempreg), builtin_zero, builtin_one);
}

static void transform_SLT(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	int tempreg = radeonFindFreeTemporary(t);

	emit2(t->Program, OPCODE_ADD, 0, dstreg(PROGRAM_TEMPORARY, tempreg), inst->SrcReg[0], negate(inst->SrcReg[1]));
	emit3(t->Program, OPCODE_CMP, inst->SaturateMode, inst->DstReg,
		srcreg(PROGRAM_TEMPORARY, tempreg), builtin_one, builtin_zero);
}

static void transform_SUB(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	emit2(t->Program, OPCODE_ADD, inst->SaturateMode, inst->DstReg, inst->SrcReg[0], negate(inst->SrcReg[1]));
}

static void transform_SWZ(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	emit1(t->Program, OPCODE_MOV, inst->SaturateMode, inst->DstReg, inst->SrcReg[0]);
}

static void transform_XPD(struct radeon_transform_context* t,
	struct prog_instruction* inst)
{
	int tempreg = radeonFindFreeTemporary(t);

	emit2(t->Program, OPCODE_MUL, 0, dstreg(PROGRAM_TEMPORARY, tempreg),
		swizzle(inst->SrcReg[0], SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W),
		swizzle(inst->SrcReg[1], SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W));
	emit3(t->Program, OPCODE_MAD, inst->SaturateMode, inst->DstReg,
		swizzle(inst->SrcReg[0], SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W),
		swizzle(inst->SrcReg[1], SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W),
		negate(srcreg(PROGRAM_TEMPORARY, tempreg)));
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
GLboolean radeonTransformALU(struct radeon_transform_context* t,
	struct prog_instruction* inst,
	void* unused)
{
	switch(inst->Opcode) {
	case OPCODE_ABS: transform_ABS(t, inst); return GL_TRUE;
	case OPCODE_DPH: transform_DPH(t, inst); return GL_TRUE;
	case OPCODE_DST: transform_DST(t, inst); return GL_TRUE;
	case OPCODE_FLR: transform_FLR(t, inst); return GL_TRUE;
	case OPCODE_LIT: transform_LIT(t, inst); return GL_TRUE;
	case OPCODE_LRP: transform_LRP(t, inst); return GL_TRUE;
	case OPCODE_POW: transform_POW(t, inst); return GL_TRUE;
	case OPCODE_RSQ: transform_RSQ(t, inst); return GL_TRUE;
	case OPCODE_SGE: transform_SGE(t, inst); return GL_TRUE;
	case OPCODE_SLT: transform_SLT(t, inst); return GL_TRUE;
	case OPCODE_SUB: transform_SUB(t, inst); return GL_TRUE;
	case OPCODE_SWZ: transform_SWZ(t, inst); return GL_TRUE;
	case OPCODE_XPD: transform_XPD(t, inst); return GL_TRUE;
	default:
		return GL_FALSE;
	}
}


static void sincos_constants(struct radeon_transform_context* t, GLuint *constants)
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

	for(i = 0; i < 2; ++i) {
		GLuint swz;
		constants[i] = _mesa_add_unnamed_constant(t->Program->Parameters, SinCosConsts[i], 4, &swz);
		ASSERT(swz == SWIZZLE_NOOP);
	}
}

/**
 * Approximate sin(x), where x is clamped to (-pi/2, pi/2).
 *
 * MUL tmp.xy, src, { 4/PI, -4/(PI^2) }
 * MAD tmp.x, tmp.y, |src|, tmp.x
 * MAD tmp.y, tmp.x, |tmp.x|, -tmp.x
 * MAD dest, tmp.y, weight, tmp.x
 */
static void sin_approx(struct radeon_transform_context* t,
	struct prog_dst_register dst, struct prog_src_register src, const GLuint* constants)
{
	GLuint tempreg = radeonFindFreeTemporary(t);

	emit2(t->Program, OPCODE_MUL, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
		swizzle(src, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
		srcreg(PROGRAM_CONSTANT, constants[0]));
	emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_X),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
		absolute(swizzle(src, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X));
	emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_Y),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
		absolute(swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)),
		negate(swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X)));
	emit3(t->Program, OPCODE_MAD, 0, dst,
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
		swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
		swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X));
}

/**
 * Translate the trigonometric functions COS, SIN, and SCS
 * using only the basic instructions
 *  MOV, ADD, MUL, MAD, FRC
 */
GLboolean radeonTransformTrigSimple(struct radeon_transform_context* t,
	struct prog_instruction* inst,
	void* unused)
{
	if (inst->Opcode != OPCODE_COS &&
	    inst->Opcode != OPCODE_SIN &&
	    inst->Opcode != OPCODE_SCS)
		return GL_FALSE;

	GLuint constants[2];
	GLuint tempreg = radeonFindFreeTemporary(t);

	sincos_constants(t, constants);

	if (inst->Opcode == OPCODE_COS) {
		// MAD tmp.x, src, 1/(2*PI), 0.75
		// FRC tmp.x, tmp.x
		// MAD tmp.z, tmp.x, 2*PI, -PI
		emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(inst->SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X));
		emit1(t->Program, OPCODE_FRC, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W));
		emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			negate(swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z)));

		sin_approx(t, inst->DstReg,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			constants);
	} else if (inst->Opcode == OPCODE_SIN) {
		emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(inst->SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y));
		emit1(t->Program, OPCODE_FRC, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W));
		emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_W),
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			negate(swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z)));

		sin_approx(t, inst->DstReg,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			constants);
	} else {
		emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
			swizzle(inst->SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W));
		emit1(t->Program, OPCODE_FRC, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
			srcreg(PROGRAM_TEMPORARY, tempreg));
		emit3(t->Program, OPCODE_MAD, 0, dstregtmpmask(tempreg, WRITEMASK_XY),
			srcreg(PROGRAM_TEMPORARY, tempreg),
			swizzle(srcreg(PROGRAM_CONSTANT, constants[1]), SWIZZLE_W, SWIZZLE_W, SWIZZLE_W, SWIZZLE_W),
			negate(swizzle(srcreg(PROGRAM_CONSTANT, constants[0]), SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z, SWIZZLE_Z)));

		struct prog_dst_register dst = inst->DstReg;

		dst.WriteMask = inst->DstReg.WriteMask & WRITEMASK_X;
		sin_approx(t, dst,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
			constants);

		dst.WriteMask = inst->DstReg.WriteMask & WRITEMASK_Y;
		sin_approx(t, dst,
			swizzle(srcreg(PROGRAM_TEMPORARY, tempreg), SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
			constants);
	}

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
GLboolean radeonTransformTrigScale(struct radeon_transform_context* t,
	struct prog_instruction* inst,
	void* unused)
{
	if (inst->Opcode != OPCODE_COS &&
	    inst->Opcode != OPCODE_SIN &&
	    inst->Opcode != OPCODE_SCS)
		return GL_FALSE;

	static const GLfloat RCP_2PI[] = { 0.15915494309189535 };
	GLuint temp;
	GLuint constant;
	GLuint constant_swizzle;

	temp = radeonFindFreeTemporary(t);
	constant = _mesa_add_unnamed_constant(t->Program->Parameters, RCP_2PI, 1, &constant_swizzle);

	emit2(t->Program, OPCODE_MUL, 0, dstregtmpmask(temp, WRITEMASK_W),
		swizzle(inst->SrcReg[0], SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
		srcregswz(PROGRAM_CONSTANT, constant, constant_swizzle));
	emit1(t->Program, OPCODE_FRC, 0, dstregtmpmask(temp, WRITEMASK_W),
		srcreg(PROGRAM_TEMPORARY, temp));

	if (inst->Opcode == OPCODE_COS) {
		emit1(t->Program, OPCODE_COS, inst->SaturateMode, inst->DstReg,
			srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
	} else if (inst->Opcode == OPCODE_SIN) {
		emit1(t->Program, OPCODE_SIN, inst->SaturateMode,
			inst->DstReg, srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
	} else if (inst->Opcode == OPCODE_SCS) {
		struct prog_dst_register moddst = inst->DstReg;

		if (inst->DstReg.WriteMask & WRITEMASK_X) {
			moddst.WriteMask = WRITEMASK_X;
			emit1(t->Program, OPCODE_COS, inst->SaturateMode, moddst,
				srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
		}
		if (inst->DstReg.WriteMask & WRITEMASK_Y) {
			moddst.WriteMask = WRITEMASK_Y;
			emit1(t->Program, OPCODE_SIN, inst->SaturateMode, moddst,
				srcregswz(PROGRAM_TEMPORARY, temp, SWIZZLE_WWWW));
		}
	}

	return GL_TRUE;
}

/**
 * Rewrite DDX/DDY instructions to properly work with r5xx shaders.
 * The r5xx MDH/MDV instruction provides per-quad partial derivatives.
 * It takes the form A*B+C. A and C are set by setting src0. B should be -1.
 *
 * @warning This explicitly changes the form of DDX and DDY!
 */

GLboolean radeonTransformDeriv(struct radeon_transform_context* t,
	struct prog_instruction* inst,
	void* unused)
{
	if (inst->Opcode != OPCODE_DDX && inst->Opcode != OPCODE_DDY)
		return GL_FALSE;

	struct prog_src_register B = inst->SrcReg[1];

	B.Swizzle = MAKE_SWIZZLE4(SWIZZLE_ONE, SWIZZLE_ONE,
						SWIZZLE_ONE, SWIZZLE_ONE);
	B.NegateBase = NEGATE_XYZW;

	emit2(t->Program, inst->Opcode, inst->SaturateMode, inst->DstReg,
		inst->SrcReg[0], B);

	return GL_TRUE;
}
