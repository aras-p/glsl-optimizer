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


static struct prog_instruction *emit1(struct radeon_program_transform_context* ctx,
	gl_inst_opcode Opcode, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg)
{
	struct prog_instruction *fpi =
		radeonClauseInsertInstructions(ctx->compiler, ctx->dest,
			ctx->dest->NumInstructions, 1);

	fpi->Opcode = Opcode;
	fpi->DstReg = DstReg;
	fpi->SrcReg[0] = SrcReg;
	return fpi;
}

static struct prog_instruction *emit2(struct radeon_program_transform_context* ctx,
	gl_inst_opcode Opcode, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg0, struct prog_src_register SrcReg1)
{
	struct prog_instruction *fpi =
		radeonClauseInsertInstructions(ctx->compiler, ctx->dest,
			ctx->dest->NumInstructions, 1);

	fpi->Opcode = Opcode;
	fpi->DstReg = DstReg;
	fpi->SrcReg[0] = SrcReg0;
	fpi->SrcReg[1] = SrcReg1;
	return fpi;
}

static struct prog_instruction *emit3(struct radeon_program_transform_context* ctx,
	gl_inst_opcode Opcode, struct prog_dst_register DstReg,
	struct prog_src_register SrcReg0, struct prog_src_register SrcReg1,
	struct prog_src_register SrcReg2)
{
	struct prog_instruction *fpi =
		radeonClauseInsertInstructions(ctx->compiler, ctx->dest,
			ctx->dest->NumInstructions, 1);

	fpi->Opcode = Opcode;
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
		GET_SWZ(reg.Swizzle, x),
		GET_SWZ(reg.Swizzle, y),
		GET_SWZ(reg.Swizzle, z),
		GET_SWZ(reg.Swizzle, w));
	return swizzled;
}

static struct prog_src_register scalar(struct prog_src_register reg)
{
	return swizzle(reg, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X);
}

static void transform_ABS(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	struct prog_src_register src = inst->SrcReg[0];
	src.Abs = 1;
	src.NegateBase = 0;
	src.NegateAbs = 0;
	emit1(ctx, OPCODE_MOV, inst->DstReg, src);
}

static void transform_DPH(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	struct prog_src_register src0 = inst->SrcReg[0];
	if (src0.NegateAbs) {
		if (src0.Abs) {
			int tempreg = radeonCompilerAllocateTemporary(ctx->compiler);
			emit1(ctx, OPCODE_MOV, dstreg(PROGRAM_TEMPORARY, tempreg), src0);
			src0 = srcreg(src0.File, src0.Index);
		} else {
			src0.NegateAbs = 0;
			src0.NegateBase ^= NEGATE_XYZW;
		}
	}
	set_swizzle(&src0, 3, SWIZZLE_ONE);
	set_negate_base(&src0, 3, 0);
	emit2(ctx, OPCODE_DP4, inst->DstReg, src0, inst->SrcReg[1]);
}

static void transform_FLR(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	int tempreg = radeonCompilerAllocateTemporary(ctx->compiler);
	emit1(ctx, OPCODE_FRC, dstreg(PROGRAM_TEMPORARY, tempreg), inst->SrcReg[0]);
	emit2(ctx, OPCODE_ADD, inst->DstReg, inst->SrcReg[0], negate(srcreg(PROGRAM_TEMPORARY, tempreg)));
}

static void transform_POW(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	int tempreg = radeonCompilerAllocateTemporary(ctx->compiler);
	struct prog_dst_register tempdst = dstreg(PROGRAM_TEMPORARY, tempreg);
	struct prog_src_register tempsrc = srcreg(PROGRAM_TEMPORARY, tempreg);
	tempdst.WriteMask = WRITEMASK_W;
	tempsrc.Swizzle = SWIZZLE_WWWW;

	emit1(ctx, OPCODE_LG2, tempdst, scalar(inst->SrcReg[0]));
	emit2(ctx, OPCODE_MUL, tempdst, tempsrc, scalar(inst->SrcReg[1]));
	emit1(ctx, OPCODE_EX2, inst->DstReg, tempsrc);
}

static void transform_SGE(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	int tempreg = radeonCompilerAllocateTemporary(ctx->compiler);

	emit2(ctx, OPCODE_ADD, dstreg(PROGRAM_TEMPORARY, tempreg), inst->SrcReg[0], negate(inst->SrcReg[1]));
	emit3(ctx, OPCODE_CMP, inst->DstReg, srcreg(PROGRAM_TEMPORARY, tempreg), builtin_zero, builtin_one);
}

static void transform_SLT(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	int tempreg = radeonCompilerAllocateTemporary(ctx->compiler);

	emit2(ctx, OPCODE_ADD, dstreg(PROGRAM_TEMPORARY, tempreg), inst->SrcReg[0], negate(inst->SrcReg[1]));
	emit3(ctx, OPCODE_CMP, inst->DstReg, srcreg(PROGRAM_TEMPORARY, tempreg), builtin_one, builtin_zero);
}

static void transform_SUB(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	emit2(ctx, OPCODE_ADD, inst->DstReg, inst->SrcReg[0], negate(inst->SrcReg[1]));
}

static void transform_SWZ(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	emit1(ctx, OPCODE_MOV, inst->DstReg, inst->SrcReg[0]);
}

static void transform_XPD(struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst)
{
	int tempreg = radeonCompilerAllocateTemporary(ctx->compiler);

	emit2(ctx, OPCODE_MUL, dstreg(PROGRAM_TEMPORARY, tempreg),
		swizzle(inst->SrcReg[0], SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W),
		swizzle(inst->SrcReg[1], SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W));
	emit3(ctx, OPCODE_MAD, inst->DstReg,
		swizzle(inst->SrcReg[0], SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_X, SWIZZLE_W),
		swizzle(inst->SrcReg[1], SWIZZLE_Z, SWIZZLE_X, SWIZZLE_Y, SWIZZLE_W),
		negate(srcreg(PROGRAM_TEMPORARY, tempreg)));
}


/**
 * Can be used as a transformation for @ref radeonClauseLocalTransform,
 * no userData necessary.
 *
 * Eliminates the following ALU instructions:
 *  ABS, DPH, FLR, POW, SGE, SLT, SUB, SWZ, XPD
 * using:
 *  MOV, ADD, MUL, MAD, FRC, DP3, LG2, EX2, CMP
 *
 * @note should be applicable to R300 and R500 fragment programs.
 *
 * @todo add LIT here as well?
 */
GLboolean radeonTransformALU(
	struct radeon_program_transform_context* ctx,
	struct prog_instruction* inst,
	void* unused)
{
	switch(inst->Opcode) {
	case OPCODE_ABS: transform_ABS(ctx, inst); return GL_TRUE;
	case OPCODE_DPH: transform_DPH(ctx, inst); return GL_TRUE;
	case OPCODE_FLR: transform_FLR(ctx, inst); return GL_TRUE;
	case OPCODE_POW: transform_POW(ctx, inst); return GL_TRUE;
	case OPCODE_SGE: transform_SGE(ctx, inst); return GL_TRUE;
	case OPCODE_SLT: transform_SLT(ctx, inst); return GL_TRUE;
	case OPCODE_SUB: transform_SUB(ctx, inst); return GL_TRUE;
	case OPCODE_SWZ: transform_SWZ(ctx, inst); return GL_TRUE;
	case OPCODE_XPD: transform_XPD(ctx, inst); return GL_TRUE;
	default:
		return GL_FALSE;
	}
}
