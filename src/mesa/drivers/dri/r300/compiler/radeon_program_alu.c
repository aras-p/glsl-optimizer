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
	rc_opcode Opcode, rc_saturate_mode Saturate, struct rc_dst_register DstReg,
	struct rc_src_register SrcReg)
{
	struct rc_instruction *fpi = rc_insert_new_instruction(c, after);

	fpi->U.I.Opcode = Opcode;
	fpi->U.I.SaturateMode = Saturate;
	fpi->U.I.DstReg = DstReg;
	fpi->U.I.SrcReg[0] = SrcReg;
	return fpi;
}

static struct rc_instruction *emit2(
	struct radeon_compiler * c, struct rc_instruction * after,
	rc_opcode Opcode, rc_saturate_mode Saturate, struct rc_dst_register DstReg,
	struct rc_src_register SrcReg0, struct rc_src_register SrcReg1)
{
	struct rc_instruction *fpi = rc_insert_new_instruction(c, after);

	fpi->U.I.Opcode = Opcode;
	fpi->U.I.SaturateMode = Saturate;
	fpi->U.I.DstReg = DstReg;
	fpi->U.I.SrcReg[0] = SrcReg0;
	fpi->U.I.SrcReg[1] = SrcReg1;
	return fpi;
}

static struct rc_instruction *emit3(
	struct radeon_compiler * c, struct rc_instruction * after,
	rc_opcode Opcode, rc_saturate_mode Saturate, struct rc_dst_register DstReg,
	struct rc_src_register SrcReg0, struct rc_src_register SrcReg1,
	struct rc_src_register SrcReg2)
{
	struct rc_instruction *fpi = rc_insert_new_instruction(c, after);

	fpi->U.I.Opcode = Opcode;
	fpi->U.I.SaturateMode = Saturate;
	fpi->U.I.DstReg = DstReg;
	fpi->U.I.SrcReg[0] = SrcReg0;
	fpi->U.I.SrcReg[1] = SrcReg1;
	fpi->U.I.SrcReg[2] = SrcReg2;
	return fpi;
}

static struct rc_dst_register dstreg(int file, int index)
{
	struct rc_dst_register dst;
	dst.File = file;
	dst.Index = index;
	dst.WriteMask = RC_MASK_XYZW;
	dst.RelAddr = 0;
	return dst;
}

static struct rc_dst_register dstregtmpmask(int index, int mask)
{
	struct rc_dst_register dst = {0};
	dst.File = RC_FILE_TEMPORARY;
	dst.Index = index;
	dst.WriteMask = mask;
	dst.RelAddr = 0;
	return dst;
}

static const struct rc_src_register builtin_zero = {
	.File = RC_FILE_NONE,
	.Index = 0,
	.Swizzle = RC_SWIZZLE_0000
};
static const struct rc_src_register builtin_one = {
	.File = RC_FILE_NONE,
	.Index = 0,
	.Swizzle = RC_SWIZZLE_1111
};
static const struct rc_src_register srcreg_undefined = {
	.File = RC_FILE_NONE,
	.Index = 0,
	.Swizzle = RC_SWIZZLE_XYZW
};

static struct rc_src_register srcreg(int file, int index)
{
	struct rc_src_register src = srcreg_undefined;
	src.File = file;
	src.Index = index;
	return src;
}

static struct rc_src_register srcregswz(int file, int index, int swz)
{
	struct rc_src_register src = srcreg_undefined;
	src.File = file;
	src.Index = index;
	src.Swizzle = swz;
	return src;
}

static struct rc_src_register absolute(struct rc_src_register reg)
{
	struct rc_src_register newreg = reg;
	newreg.Abs = 1;
	newreg.Negate = RC_MASK_NONE;
	return newreg;
}

static struct rc_src_register negate(struct rc_src_register reg)
{
	struct rc_src_register newreg = reg;
	newreg.Negate = newreg.Negate ^ RC_MASK_XYZW;
	return newreg;
}

static struct rc_src_register swizzle(struct rc_src_register reg,
		rc_swizzle x, rc_swizzle y, rc_swizzle z, rc_swizzle w)
{
	struct rc_src_register swizzled = reg;
	swizzled.Swizzle = combine_swizzles4(reg.Swizzle, x, y, z, w);
	return swizzled;
}

static struct rc_src_register scalar(struct rc_src_register reg)
{
	return swizzle(reg, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X);
}

static void transform_ABS(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	struct rc_src_register src = inst->U.I.SrcReg[0];
	src.Abs = 1;
	src.Negate = RC_MASK_NONE;
	emit1(c, inst->Prev, RC_OPCODE_MOV, inst->U.I.SaturateMode, inst->U.I.DstReg, src);
	rc_remove_instruction(inst);
}

static void transform_DP3(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	struct rc_src_register src0 = inst->U.I.SrcReg[0];
	struct rc_src_register src1 = inst->U.I.SrcReg[1];
	src0.Negate &= ~RC_MASK_W;
	src0.Swizzle &= ~(7 << (3 * 3));
	src0.Swizzle |= RC_SWIZZLE_ZERO << (3 * 3);
	src1.Negate &= ~RC_MASK_W;
	src1.Swizzle &= ~(7 << (3 * 3));
	src1.Swizzle |= RC_SWIZZLE_ZERO << (3 * 3);
	emit2(c, inst->Prev, RC_OPCODE_DP4, inst->U.I.SaturateMode, inst->U.I.DstReg, src0, src1);
	rc_remove_instruction(inst);
}

static void transform_DPH(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	struct rc_src_register src0 = inst->U.I.SrcReg[0];
	src0.Negate &= ~RC_MASK_W;
	src0.Swizzle &= ~(7 << (3 * 3));
	src0.Swizzle |= RC_SWIZZLE_ONE << (3 * 3);
	emit2(c, inst->Prev, RC_OPCODE_DP4, inst->U.I.SaturateMode, inst->U.I.DstReg, src0, inst->U.I.SrcReg[1]);
	rc_remove_instruction(inst);
}

/**
 * [1, src0.y*src1.y, src0.z, src1.w]
 * So basically MUL with lotsa swizzling.
 */
static void transform_DST(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	emit2(c, inst->Prev, RC_OPCODE_MUL, inst->U.I.SaturateMode, inst->U.I.DstReg,
		swizzle(inst->U.I.SrcReg[0], RC_SWIZZLE_ONE, RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_ONE),
		swizzle(inst->U.I.SrcReg[1], RC_SWIZZLE_ONE, RC_SWIZZLE_Y, RC_SWIZZLE_ONE, RC_SWIZZLE_W));
	rc_remove_instruction(inst);
}

static void transform_FLR(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);
	emit1(c, inst->Prev, RC_OPCODE_FRC, 0, dstreg(RC_FILE_TEMPORARY, tempreg), inst->U.I.SrcReg[0]);
	emit2(c, inst->Prev, RC_OPCODE_ADD, inst->U.I.SaturateMode, inst->U.I.DstReg,
		inst->U.I.SrcReg[0], negate(srcreg(RC_FILE_TEMPORARY, tempreg)));
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
	unsigned int constant;
	unsigned int constant_swizzle;
	unsigned int temp;
	struct rc_src_register srctemp;

	constant = rc_constants_add_immediate_scalar(&c->Program.Constants, -127.999999, &constant_swizzle);

	if (inst->U.I.DstReg.WriteMask != RC_MASK_XYZW || inst->U.I.DstReg.File != RC_FILE_TEMPORARY) {
		struct rc_instruction * inst_mov;

		inst_mov = emit1(c, inst,
			RC_OPCODE_MOV, 0, inst->U.I.DstReg,
			srcreg(RC_FILE_TEMPORARY, rc_find_free_temporary(c)));

		inst->U.I.DstReg.File = RC_FILE_TEMPORARY;
		inst->U.I.DstReg.Index = inst_mov->U.I.SrcReg[0].Index;
		inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;
	}

	temp = inst->U.I.DstReg.Index;
	srctemp = srcreg(RC_FILE_TEMPORARY, temp);

	/* tmp.x = max(0.0, Src.x); */
	/* tmp.y = max(0.0, Src.y); */
	/* tmp.w = clamp(Src.z, -128+eps, 128-eps); */
	emit2(c, inst->Prev, RC_OPCODE_MAX, 0,
		dstregtmpmask(temp, RC_MASK_XYW),
		inst->U.I.SrcReg[0],
		swizzle(srcreg(RC_FILE_CONSTANT, constant),
			RC_SWIZZLE_ZERO, RC_SWIZZLE_ZERO, RC_SWIZZLE_ZERO, constant_swizzle&3));
	emit2(c, inst->Prev, RC_OPCODE_MIN, 0,
		dstregtmpmask(temp, RC_MASK_Z),
		swizzle(srctemp, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
		negate(srcregswz(RC_FILE_CONSTANT, constant, constant_swizzle)));

	/* tmp.w = Pow(tmp.y, tmp.w) */
	emit1(c, inst->Prev, RC_OPCODE_LG2, 0,
		dstregtmpmask(temp, RC_MASK_W),
		swizzle(srctemp, RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y));
	emit2(c, inst->Prev, RC_OPCODE_MUL, 0,
		dstregtmpmask(temp, RC_MASK_W),
		swizzle(srctemp, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
		swizzle(srctemp, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z));
	emit1(c, inst->Prev, RC_OPCODE_EX2, 0,
		dstregtmpmask(temp, RC_MASK_W),
		swizzle(srctemp, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W));

	/* tmp.z = (tmp.x > 0) ? tmp.w : 0.0 */
	emit3(c, inst->Prev, RC_OPCODE_CMP, inst->U.I.SaturateMode,
		dstregtmpmask(temp, RC_MASK_Z),
		negate(swizzle(srctemp, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X)),
		swizzle(srctemp, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
		builtin_zero);

	/* tmp.x, tmp.y, tmp.w = 1.0, tmp.x, 1.0 */
	emit1(c, inst->Prev, RC_OPCODE_MOV, inst->U.I.SaturateMode,
		dstregtmpmask(temp, RC_MASK_XYW),
		swizzle(srctemp, RC_SWIZZLE_ONE, RC_SWIZZLE_X, RC_SWIZZLE_ONE, RC_SWIZZLE_ONE));

	rc_remove_instruction(inst);
}

static void transform_LRP(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_ADD, 0,
		dstreg(RC_FILE_TEMPORARY, tempreg),
		inst->U.I.SrcReg[1], negate(inst->U.I.SrcReg[2]));
	emit3(c, inst->Prev, RC_OPCODE_MAD, inst->U.I.SaturateMode,
		inst->U.I.DstReg,
		inst->U.I.SrcReg[0], srcreg(RC_FILE_TEMPORARY, tempreg), inst->U.I.SrcReg[2]);

	rc_remove_instruction(inst);
}

static void transform_POW(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);
	struct rc_dst_register tempdst = dstreg(RC_FILE_TEMPORARY, tempreg);
	struct rc_src_register tempsrc = srcreg(RC_FILE_TEMPORARY, tempreg);
	tempdst.WriteMask = RC_MASK_W;
	tempsrc.Swizzle = RC_SWIZZLE_WWWW;

	emit1(c, inst->Prev, RC_OPCODE_LG2, 0, tempdst, scalar(inst->U.I.SrcReg[0]));
	emit2(c, inst->Prev, RC_OPCODE_MUL, 0, tempdst, tempsrc, scalar(inst->U.I.SrcReg[1]));
	emit1(c, inst->Prev, RC_OPCODE_EX2, inst->U.I.SaturateMode, inst->U.I.DstReg, tempsrc);

	rc_remove_instruction(inst);
}

static void transform_RSQ(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	inst->U.I.SrcReg[0] = absolute(inst->U.I.SrcReg[0]);
}

static void transform_SEQ(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_ADD, 0, dstreg(RC_FILE_TEMPORARY, tempreg), inst->U.I.SrcReg[0], negate(inst->U.I.SrcReg[1]));
	emit3(c, inst->Prev, RC_OPCODE_CMP, inst->U.I.SaturateMode, inst->U.I.DstReg,
		negate(absolute(srcreg(RC_FILE_TEMPORARY, tempreg))), builtin_zero, builtin_one);

	rc_remove_instruction(inst);
}

static void transform_SFL(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	emit1(c, inst->Prev, RC_OPCODE_MOV, inst->U.I.SaturateMode, inst->U.I.DstReg, builtin_zero);
	rc_remove_instruction(inst);
}

static void transform_SGE(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_ADD, 0, dstreg(RC_FILE_TEMPORARY, tempreg), inst->U.I.SrcReg[0], negate(inst->U.I.SrcReg[1]));
	emit3(c, inst->Prev, RC_OPCODE_CMP, inst->U.I.SaturateMode, inst->U.I.DstReg,
		srcreg(RC_FILE_TEMPORARY, tempreg), builtin_zero, builtin_one);

	rc_remove_instruction(inst);
}

static void transform_SGT(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_ADD, 0, dstreg(RC_FILE_TEMPORARY, tempreg), negate(inst->U.I.SrcReg[0]), inst->U.I.SrcReg[1]);
	emit3(c, inst->Prev, RC_OPCODE_CMP, inst->U.I.SaturateMode, inst->U.I.DstReg,
		srcreg(RC_FILE_TEMPORARY, tempreg), builtin_one, builtin_zero);

	rc_remove_instruction(inst);
}

static void transform_SLE(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_ADD, 0, dstreg(RC_FILE_TEMPORARY, tempreg), negate(inst->U.I.SrcReg[0]), inst->U.I.SrcReg[1]);
	emit3(c, inst->Prev, RC_OPCODE_CMP, inst->U.I.SaturateMode, inst->U.I.DstReg,
		srcreg(RC_FILE_TEMPORARY, tempreg), builtin_zero, builtin_one);

	rc_remove_instruction(inst);
}

static void transform_SLT(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_ADD, 0, dstreg(RC_FILE_TEMPORARY, tempreg), inst->U.I.SrcReg[0], negate(inst->U.I.SrcReg[1]));
	emit3(c, inst->Prev, RC_OPCODE_CMP, inst->U.I.SaturateMode, inst->U.I.DstReg,
		srcreg(RC_FILE_TEMPORARY, tempreg), builtin_one, builtin_zero);

	rc_remove_instruction(inst);
}

static void transform_SNE(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_ADD, 0, dstreg(RC_FILE_TEMPORARY, tempreg), inst->U.I.SrcReg[0], negate(inst->U.I.SrcReg[1]));
	emit3(c, inst->Prev, RC_OPCODE_CMP, inst->U.I.SaturateMode, inst->U.I.DstReg,
		negate(absolute(srcreg(RC_FILE_TEMPORARY, tempreg))), builtin_one, builtin_zero);

	rc_remove_instruction(inst);
}

static void transform_SUB(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	inst->U.I.Opcode = RC_OPCODE_ADD;
	inst->U.I.SrcReg[1] = negate(inst->U.I.SrcReg[1]);
}

static void transform_SWZ(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	inst->U.I.Opcode = RC_OPCODE_MOV;
}

static void transform_XPD(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_MUL, 0, dstreg(RC_FILE_TEMPORARY, tempreg),
		swizzle(inst->U.I.SrcReg[0], RC_SWIZZLE_Z, RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_W),
		swizzle(inst->U.I.SrcReg[1], RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_X, RC_SWIZZLE_W));
	emit3(c, inst->Prev, RC_OPCODE_MAD, inst->U.I.SaturateMode, inst->U.I.DstReg,
		swizzle(inst->U.I.SrcReg[0], RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_X, RC_SWIZZLE_W),
		swizzle(inst->U.I.SrcReg[1], RC_SWIZZLE_Z, RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_W),
		negate(srcreg(RC_FILE_TEMPORARY, tempreg)));

	rc_remove_instruction(inst);
}


/**
 * Can be used as a transformation for @ref radeonClauseLocalTransform,
 * no userData necessary.
 *
 * Eliminates the following ALU instructions:
 *  ABS, DPH, DST, FLR, LIT, LRP, POW, SEQ, SFL, SGE, SGT, SLE, SLT, SNE, SUB, SWZ, XPD
 * using:
 *  MOV, ADD, MUL, MAD, FRC, DP3, LG2, EX2, CMP
 *
 * Transforms RSQ to Radeon's native RSQ by explicitly setting
 * absolute value.
 *
 * @note should be applicable to R300 and R500 fragment programs.
 */
int radeonTransformALU(
	struct radeon_compiler * c,
	struct rc_instruction* inst,
	void* unused)
{
	switch(inst->U.I.Opcode) {
	case RC_OPCODE_ABS: transform_ABS(c, inst); return 1;
	case RC_OPCODE_DPH: transform_DPH(c, inst); return 1;
	case RC_OPCODE_DST: transform_DST(c, inst); return 1;
	case RC_OPCODE_FLR: transform_FLR(c, inst); return 1;
	case RC_OPCODE_LIT: transform_LIT(c, inst); return 1;
	case RC_OPCODE_LRP: transform_LRP(c, inst); return 1;
	case RC_OPCODE_POW: transform_POW(c, inst); return 1;
	case RC_OPCODE_RSQ: transform_RSQ(c, inst); return 1;
	case RC_OPCODE_SEQ: transform_SEQ(c, inst); return 1;
	case RC_OPCODE_SFL: transform_SFL(c, inst); return 1;
	case RC_OPCODE_SGE: transform_SGE(c, inst); return 1;
	case RC_OPCODE_SGT: transform_SGT(c, inst); return 1;
	case RC_OPCODE_SLE: transform_SLE(c, inst); return 1;
	case RC_OPCODE_SLT: transform_SLT(c, inst); return 1;
	case RC_OPCODE_SNE: transform_SNE(c, inst); return 1;
	case RC_OPCODE_SUB: transform_SUB(c, inst); return 1;
	case RC_OPCODE_SWZ: transform_SWZ(c, inst); return 1;
	case RC_OPCODE_XPD: transform_XPD(c, inst); return 1;
	default:
		return 0;
	}
}


static void transform_r300_vertex_ABS(struct radeon_compiler* c,
	struct rc_instruction* inst)
{
	/* Note: r500 can take absolute values, but r300 cannot. */
	inst->U.I.Opcode = RC_OPCODE_MAX;
	inst->U.I.SrcReg[1] = inst->U.I.SrcReg[0];
	inst->U.I.SrcReg[1].Negate ^= RC_MASK_XYZW;
}

/**
 * For use with radeonLocalTransform, this transforms non-native ALU
 * instructions of the r300 up to r500 vertex engine.
 */
int r300_transform_vertex_alu(
	struct radeon_compiler * c,
	struct rc_instruction* inst,
	void* unused)
{
	switch(inst->U.I.Opcode) {
	case RC_OPCODE_ABS: transform_r300_vertex_ABS(c, inst); return 1;
	case RC_OPCODE_DP3: transform_DP3(c, inst); return 1;
	case RC_OPCODE_DPH: transform_DPH(c, inst); return 1;
	case RC_OPCODE_FLR: transform_FLR(c, inst); return 1;
	case RC_OPCODE_LRP: transform_LRP(c, inst); return 1;
	case RC_OPCODE_SUB: transform_SUB(c, inst); return 1;
	case RC_OPCODE_SWZ: transform_SWZ(c, inst); return 1;
	case RC_OPCODE_XPD: transform_XPD(c, inst); return 1;
	default:
		return 0;
	}
}

static void sincos_constants(struct radeon_compiler* c, unsigned int *constants)
{
	static const float SinCosConsts[2][4] = {
		{
			1.273239545,		/* 4/PI */
			-0.405284735,		/* -4/(PI*PI) */
			3.141592654,		/* PI */
			0.2225			/* weight */
		},
		{
			0.75,
			0.5,
			0.159154943,		/* 1/(2*PI) */
			6.283185307		/* 2*PI */
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
	struct radeon_compiler* c, struct rc_instruction * inst,
	struct rc_dst_register dst, struct rc_src_register src, const unsigned int* constants)
{
	unsigned int tempreg = rc_find_free_temporary(c);

	emit2(c, inst->Prev, RC_OPCODE_MUL, 0, dstregtmpmask(tempreg, RC_MASK_XY),
		swizzle(src, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X),
		srcreg(RC_FILE_CONSTANT, constants[0]));
	emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_X),
		swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y),
		absolute(swizzle(src, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X)),
		swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X));
	emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_Y),
		swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X),
		absolute(swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X)),
		negate(swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X)));
	emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dst,
		swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y),
		swizzle(srcreg(RC_FILE_CONSTANT, constants[0]), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
		swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X));
}

/**
 * Translate the trigonometric functions COS, SIN, and SCS
 * using only the basic instructions
 *  MOV, ADD, MUL, MAD, FRC
 */
int radeonTransformTrigSimple(struct radeon_compiler* c,
	struct rc_instruction* inst,
	void* unused)
{
	if (inst->U.I.Opcode != RC_OPCODE_COS &&
	    inst->U.I.Opcode != RC_OPCODE_SIN &&
	    inst->U.I.Opcode != RC_OPCODE_SCS)
		return 0;

	unsigned int constants[2];
	unsigned int tempreg = rc_find_free_temporary(c);

	sincos_constants(c, constants);

	if (inst->U.I.Opcode == RC_OPCODE_COS) {
		/* MAD tmp.x, src, 1/(2*PI), 0.75 */
		/* FRC tmp.x, tmp.x */
		/* MAD tmp.z, tmp.x, 2*PI, -PI */
		emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_W),
			swizzle(inst->U.I.SrcReg[0], RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X));
		emit1(c, inst->Prev, RC_OPCODE_FRC, 0, dstregtmpmask(tempreg, RC_MASK_W),
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W));
		emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_W),
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
			negate(swizzle(srcreg(RC_FILE_CONSTANT, constants[0]), RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z)));

		sin_approx(c, inst, inst->U.I.DstReg,
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
			constants);
	} else if (inst->U.I.Opcode == RC_OPCODE_SIN) {
		emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_W),
			swizzle(inst->U.I.SrcReg[0], RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y));
		emit1(c, inst->Prev, RC_OPCODE_FRC, 0, dstregtmpmask(tempreg, RC_MASK_W),
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W));
		emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_W),
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
			negate(swizzle(srcreg(RC_FILE_CONSTANT, constants[0]), RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z)));

		sin_approx(c, inst, inst->U.I.DstReg,
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
			constants);
	} else {
		emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_XY),
			swizzle(inst->U.I.SrcReg[0], RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_W));
		emit1(c, inst->Prev, RC_OPCODE_FRC, 0, dstregtmpmask(tempreg, RC_MASK_XY),
			srcreg(RC_FILE_TEMPORARY, tempreg));
		emit3(c, inst->Prev, RC_OPCODE_MAD, 0, dstregtmpmask(tempreg, RC_MASK_XY),
			srcreg(RC_FILE_TEMPORARY, tempreg),
			swizzle(srcreg(RC_FILE_CONSTANT, constants[1]), RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W, RC_SWIZZLE_W),
			negate(swizzle(srcreg(RC_FILE_CONSTANT, constants[0]), RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z, RC_SWIZZLE_Z)));

		struct rc_dst_register dst = inst->U.I.DstReg;

		dst.WriteMask = inst->U.I.DstReg.WriteMask & RC_MASK_X;
		sin_approx(c, inst, dst,
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X),
			constants);

		dst.WriteMask = inst->U.I.DstReg.WriteMask & RC_MASK_Y;
		sin_approx(c, inst, dst,
			swizzle(srcreg(RC_FILE_TEMPORARY, tempreg), RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y, RC_SWIZZLE_Y),
			constants);
	}

	rc_remove_instruction(inst);

	return 1;
}


/**
 * Transform the trigonometric functions COS, SIN, and SCS
 * to include pre-scaling by 1/(2*PI) and taking the fractional
 * part, so that the input to COS and SIN is always in the range [0,1).
 * SCS is replaced by one COS and one SIN instruction.
 *
 * @warning This transformation implicitly changes the semantics of SIN and COS!
 */
int radeonTransformTrigScale(struct radeon_compiler* c,
	struct rc_instruction* inst,
	void* unused)
{
	if (inst->U.I.Opcode != RC_OPCODE_COS &&
	    inst->U.I.Opcode != RC_OPCODE_SIN &&
	    inst->U.I.Opcode != RC_OPCODE_SCS)
		return 0;

	static const float RCP_2PI = 0.15915494309189535;
	unsigned int temp;
	unsigned int constant;
	unsigned int constant_swizzle;

	temp = rc_find_free_temporary(c);
	constant = rc_constants_add_immediate_scalar(&c->Program.Constants, RCP_2PI, &constant_swizzle);

	emit2(c, inst->Prev, RC_OPCODE_MUL, 0, dstregtmpmask(temp, RC_MASK_W),
		swizzle(inst->U.I.SrcReg[0], RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X, RC_SWIZZLE_X),
		srcregswz(RC_FILE_CONSTANT, constant, constant_swizzle));
	emit1(c, inst->Prev, RC_OPCODE_FRC, 0, dstregtmpmask(temp, RC_MASK_W),
		srcreg(RC_FILE_TEMPORARY, temp));

	if (inst->U.I.Opcode == RC_OPCODE_COS) {
		emit1(c, inst->Prev, RC_OPCODE_COS, inst->U.I.SaturateMode, inst->U.I.DstReg,
			srcregswz(RC_FILE_TEMPORARY, temp, RC_SWIZZLE_WWWW));
	} else if (inst->U.I.Opcode == RC_OPCODE_SIN) {
		emit1(c, inst->Prev, RC_OPCODE_SIN, inst->U.I.SaturateMode,
			inst->U.I.DstReg, srcregswz(RC_FILE_TEMPORARY, temp, RC_SWIZZLE_WWWW));
	} else if (inst->U.I.Opcode == RC_OPCODE_SCS) {
		struct rc_dst_register moddst = inst->U.I.DstReg;

		if (inst->U.I.DstReg.WriteMask & RC_MASK_X) {
			moddst.WriteMask = RC_MASK_X;
			emit1(c, inst->Prev, RC_OPCODE_COS, inst->U.I.SaturateMode, moddst,
				srcregswz(RC_FILE_TEMPORARY, temp, RC_SWIZZLE_WWWW));
		}
		if (inst->U.I.DstReg.WriteMask & RC_MASK_Y) {
			moddst.WriteMask = RC_MASK_Y;
			emit1(c, inst->Prev, RC_OPCODE_SIN, inst->U.I.SaturateMode, moddst,
				srcregswz(RC_FILE_TEMPORARY, temp, RC_SWIZZLE_WWWW));
		}
	}

	rc_remove_instruction(inst);

	return 1;
}

/**
 * Rewrite DDX/DDY instructions to properly work with r5xx shaders.
 * The r5xx MDH/MDV instruction provides per-quad partial derivatives.
 * It takes the form A*B+C. A and C are set by setting src0. B should be -1.
 *
 * @warning This explicitly changes the form of DDX and DDY!
 */

int radeonTransformDeriv(struct radeon_compiler* c,
	struct rc_instruction* inst,
	void* unused)
{
	if (inst->U.I.Opcode != RC_OPCODE_DDX && inst->U.I.Opcode != RC_OPCODE_DDY)
		return 0;

	inst->U.I.SrcReg[1].Swizzle = RC_MAKE_SWIZZLE(RC_SWIZZLE_ONE, RC_SWIZZLE_ONE, RC_SWIZZLE_ONE, RC_SWIZZLE_ONE);
	inst->U.I.SrcReg[1].Negate = RC_MASK_XYZW;

	return 1;
}
