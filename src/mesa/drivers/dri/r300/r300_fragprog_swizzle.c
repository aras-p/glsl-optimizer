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
 * Utilities to deal with the somewhat odd restriction on R300 fragment
 * program swizzles.
 */

#include "r300_fragprog_swizzle.h"

#include "r300_reg.h"
#include "radeon_nqssadce.h"

#define MAKE_SWZ3(x, y, z) (MAKE_SWIZZLE4(SWIZZLE_##x, SWIZZLE_##y, SWIZZLE_##z, SWIZZLE_ZERO))

struct swizzle_data {
	GLuint hash; /**< swizzle value this matches */
	GLuint base; /**< base value for hw swizzle */
	GLuint stride; /**< difference in base between arg0/1/2 */
};

static const struct swizzle_data native_swizzles[] = {
	{MAKE_SWZ3(X, Y, Z), R300_ALU_ARGC_SRC0C_XYZ, 4},
	{MAKE_SWZ3(X, X, X), R300_ALU_ARGC_SRC0C_XXX, 4},
	{MAKE_SWZ3(Y, Y, Y), R300_ALU_ARGC_SRC0C_YYY, 4},
	{MAKE_SWZ3(Z, Z, Z), R300_ALU_ARGC_SRC0C_ZZZ, 4},
	{MAKE_SWZ3(W, W, W), R300_ALU_ARGC_SRC0A, 1},
	{MAKE_SWZ3(Y, Z, X), R300_ALU_ARGC_SRC0C_YZX, 1},
	{MAKE_SWZ3(Z, X, Y), R300_ALU_ARGC_SRC0C_ZXY, 1},
	{MAKE_SWZ3(W, Z, Y), R300_ALU_ARGC_SRC0CA_WZY, 1},
	{MAKE_SWZ3(ONE, ONE, ONE), R300_ALU_ARGC_ONE, 0},
	{MAKE_SWZ3(ZERO, ZERO, ZERO), R300_ALU_ARGC_ZERO, 0}
};

static const int num_native_swizzles = sizeof(native_swizzles)/sizeof(native_swizzles[0]);


/**
 * Find a native RGB swizzle that matches the given swizzle.
 * Returns 0 if none found.
 */
static const struct swizzle_data* lookup_native_swizzle(GLuint swizzle)
{
	int i, comp;

	for(i = 0; i < num_native_swizzles; ++i) {
		const struct swizzle_data* sd = &native_swizzles[i];
		for(comp = 0; comp < 3; ++comp) {
			GLuint swz = GET_SWZ(swizzle, comp);
			if (swz == SWIZZLE_NIL)
				continue;
			if (swz != GET_SWZ(sd->hash, comp))
				break;
		}
		if (comp == 3)
			return sd;
	}

	return 0;
}


/**
 * Check whether the given instruction supports the swizzle and negate
 * combinations in the given source register.
 */
GLboolean r300FPIsNativeSwizzle(GLuint opcode, struct prog_src_register reg)
{
	if (reg.Abs)
		reg.NegateBase = 0;

	if (opcode == OPCODE_KIL ||
	    opcode == OPCODE_TEX ||
	    opcode == OPCODE_TXB ||
	    opcode == OPCODE_TXP) {
		int j;

		if (reg.Abs || reg.NegateBase != (15*reg.NegateAbs))
			return GL_FALSE;

		for(j = 0; j < 4; ++j) {
			GLuint swz = GET_SWZ(reg.Swizzle, j);
			if (swz == SWIZZLE_NIL)
				continue;
			if (swz != j)
				return GL_FALSE;
		}

		return GL_TRUE;
	}

	GLuint relevant = 0;
	int j;

	for(j = 0; j < 3; ++j)
		if (GET_SWZ(reg.Swizzle, j) != SWIZZLE_NIL)
			relevant |= 1 << j;

	if ((reg.NegateBase & relevant) && (reg.NegateBase & relevant) != relevant)
		return GL_FALSE;

	if (!lookup_native_swizzle(reg.Swizzle))
		return GL_FALSE;

	return GL_TRUE;
}


/**
 * Generate MOV dst, src using only native swizzles.
 */
void r300FPBuildSwizzle(struct nqssadce_state *s, struct prog_dst_register dst, struct prog_src_register src)
{
	if (src.Abs)
		src.NegateBase = 0;

	while(dst.WriteMask) {
		const struct swizzle_data *best_swizzle = 0;
		GLuint best_matchcount = 0;
		GLuint best_matchmask = 0;
		GLboolean rgbnegate;
		int i, comp;

		for(i = 0; i < num_native_swizzles; ++i) {
			const struct swizzle_data *sd = &native_swizzles[i];
			GLuint matchcount = 0;
			GLuint matchmask = 0;
			for(comp = 0; comp < 3; ++comp) {
				if (!GET_BIT(dst.WriteMask, comp))
					continue;
				GLuint swz = GET_SWZ(src.Swizzle, comp);
				if (swz == SWIZZLE_NIL)
					continue;
				if (swz == GET_SWZ(sd->hash, comp)) {
					matchcount++;
					matchmask |= 1 << comp;
				}
			}
			if (matchcount > best_matchcount) {
				best_swizzle = sd;
				best_matchcount = matchcount;
				best_matchmask = matchmask;
				if (matchmask == (dst.WriteMask & WRITEMASK_XYZ))
					break;
			}
		}

		if ((src.NegateBase & best_matchmask) != 0) {
			best_matchmask &= src.NegateBase;
			rgbnegate = !src.NegateAbs;
		} else {
			rgbnegate = src.NegateAbs;
		}

		struct prog_instruction *inst;

		_mesa_insert_instructions(s->Program, s->IP, 1);
		inst = s->Program->Instructions + s->IP++;
		inst->Opcode = OPCODE_MOV;
		inst->DstReg = dst;
		inst->DstReg.WriteMask &= (best_matchmask | WRITEMASK_W);
		inst->SrcReg[0] = src;
		/* Note: We rely on NqSSA/DCE to set unused swizzle components to NIL */

		dst.WriteMask &= ~inst->DstReg.WriteMask;
	}
}


/**
 * Translate an RGB (XYZ) swizzle into the hardware code for the given
 * instruction source.
 */
GLuint r300FPTranslateRGBSwizzle(GLuint src, GLuint swizzle)
{
	const struct swizzle_data* sd = lookup_native_swizzle(swizzle);

	if (!sd) {
		_mesa_printf("Not a native swizzle: %08x\n", swizzle);
		return 0;
	}

	return sd->base + src*sd->stride;
}


/**
 * Translate an Alpha (W) swizzle into the hardware code for the given
 * instruction source.
 */
GLuint r300FPTranslateAlphaSwizzle(GLuint src, GLuint swizzle)
{
	if (swizzle < 3)
		return swizzle + 3*src;

	switch(swizzle) {
	case SWIZZLE_W: return R300_ALU_ARGA_SRC0A + src;
	case SWIZZLE_ONE: return R300_ALU_ARGA_ONE;
	case SWIZZLE_ZERO: return R300_ALU_ARGA_ZERO;
	default: return R300_ALU_ARGA_ONE;
	}
}
