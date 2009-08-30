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

#ifndef __RADEON_PROGRAM_H_
#define __RADEON_PROGRAM_H_

#include <stdint.h>
#include <string.h>

#include "radeon_opcodes.h"
#include "radeon_code.h"

struct radeon_compiler;

typedef enum {
	RC_SATURATE_NONE = 0,
	RC_SATURATE_ZERO_ONE,
	RC_SATURATE_MINUS_PLUS_ONE
} rc_saturate_mode;

typedef enum {
	RC_TEXTURE_2D_ARRAY,
	RC_TEXTURE_1D_ARRAY,
	RC_TEXTURE_CUBE,
	RC_TEXTURE_3D,
	RC_TEXTURE_RECT,
	RC_TEXTURE_2D,
	RC_TEXTURE_1D
} rc_texture_target;

typedef enum {
	/**
	 * Used to indicate unused register descriptions and
	 * source register that use a constant swizzle.
	 */
	RC_FILE_NONE = 0,
	RC_FILE_TEMPORARY,

	/**
	 * Input register.
	 *
	 * \note The compiler attaches no implicit semantics to input registers.
	 * Fragment/vertex program specific semantics must be defined explicitly
	 * using the appropriate compiler interfaces.
	 */
	RC_FILE_INPUT,

	/**
	 * Output register.
	 *
	 * \note The compiler attaches no implicit semantics to input registers.
	 * Fragment/vertex program specific semantics must be defined explicitly
	 * using the appropriate compiler interfaces.
	 */
	RC_FILE_OUTPUT,
	RC_FILE_ADDRESS,

	/**
	 * Indicates a constant from the \ref rc_constant_list .
	 */
	RC_FILE_CONSTANT
} rc_register_file;

#define RC_REGISTER_INDEX_BITS 10
#define RC_REGISTER_MAX_INDEX (1 << RC_REGISTER_INDEX_BITS)

typedef enum {
	RC_SWIZZLE_X = 0,
	RC_SWIZZLE_Y,
	RC_SWIZZLE_Z,
	RC_SWIZZLE_W,
	RC_SWIZZLE_ZERO,
	RC_SWIZZLE_ONE,
	RC_SWIZZLE_HALF,
	RC_SWIZZLE_UNUSED
} rc_swizzle;

#define RC_MAKE_SWIZZLE(a,b,c,d) (((a)<<0) | ((b)<<3) | ((c)<<6) | ((d)<<9))
#define RC_MAKE_SWIZZLE_SMEAR(a) RC_MAKE_SWIZZLE((a),(a),(a),(a))
#define GET_SWZ(swz, idx)      (((swz) >> ((idx)*3)) & 0x7)
#define GET_BIT(msk, idx)      (((msk) >> (idx)) & 0x1)

#define RC_SWIZZLE_XYZW RC_MAKE_SWIZZLE(RC_SWIZZLE_X, RC_SWIZZLE_Y, RC_SWIZZLE_Z, RC_SWIZZLE_W)
#define RC_SWIZZLE_XXXX RC_MAKE_SWIZZLE_SMEAR(RC_SWIZZLE_X)
#define RC_SWIZZLE_YYYY RC_MAKE_SWIZZLE_SMEAR(RC_SWIZZLE_Y)
#define RC_SWIZZLE_ZZZZ RC_MAKE_SWIZZLE_SMEAR(RC_SWIZZLE_Z)
#define RC_SWIZZLE_WWWW RC_MAKE_SWIZZLE_SMEAR(RC_SWIZZLE_W)
#define RC_SWIZZLE_0000 RC_MAKE_SWIZZLE_SMEAR(RC_SWIZZLE_ZERO)
#define RC_SWIZZLE_1111 RC_MAKE_SWIZZLE_SMEAR(RC_SWIZZLE_ONE)

/**
 * \name Bitmasks for components of vectors.
 *
 * Used for write masks, negation masks, etc.
 */
/*@{*/
#define RC_MASK_NONE 0
#define RC_MASK_X 1
#define RC_MASK_Y 2
#define RC_MASK_Z 4
#define RC_MASK_W 8
#define RC_MASK_XY (RC_MASK_X|RC_MASK_Y)
#define RC_MASK_XYZ (RC_MASK_X|RC_MASK_Y|RC_MASK_Z)
#define RC_MASK_XYW (RC_MASK_X|RC_MASK_Y|RC_MASK_W)
#define RC_MASK_XYZW (RC_MASK_X|RC_MASK_Y|RC_MASK_Z|RC_MASK_W)
/*@}*/

struct rc_src_register {
	rc_register_file File:3;

	/** Negative values may be used for relative addressing. */
	signed int Index:(RC_REGISTER_INDEX_BITS+1);
	unsigned int RelAddr:1;

	unsigned int Swizzle:12;

	/** Take the component-wise absolute value */
	unsigned int Abs:1;

	/** Post-Abs negation. */
	unsigned int Negate:4;
};

struct rc_dst_register {
	rc_register_file File:3;

	/** Negative values may be used for relative addressing. */
	signed int Index:(RC_REGISTER_INDEX_BITS+1);
	unsigned int RelAddr:1;

	unsigned int WriteMask:4;
};

/**
 * Instructions are maintained by the compiler in a doubly linked list
 * of these structures.
 *
 * This instruction format is intended to be expanded for hardware-specific
 * trickery. At different stages of compilation, a different set of
 * instruction types may be valid.
 */
struct rc_sub_instruction {
		struct rc_src_register SrcReg[3];
		struct rc_dst_register DstReg;

		/**
		 * Opcode of this instruction, according to \ref rc_opcode enums.
		 */
		rc_opcode Opcode:8;

		/**
		 * Saturate each value of the result to the range [0,1] or [-1,1],
		 * according to \ref rc_saturate_mode enums.
		 */
		rc_saturate_mode SaturateMode:2;

		/**
		 * \name Extra fields for TEX, TXB, TXD, TXL, TXP instructions.
		 */
		/*@{*/
		/** Source texture unit. */
		unsigned int TexSrcUnit:5;

		/** Source texture target, one of the \ref rc_texture_target enums */
		rc_texture_target TexSrcTarget:3;

		/** True if tex instruction should do shadow comparison */
		unsigned int TexShadow:1;
		/*@}*/
};

struct rc_instruction {
	struct rc_instruction * Prev;
	struct rc_instruction * Next;

	struct rc_sub_instruction I;
};

struct rc_program {
	/**
	 * Instructions.Next points to the first instruction,
	 * Instructions.Prev points to the last instruction.
	 */
	struct rc_instruction Instructions;

	/* Long term, we should probably remove InputsRead & OutputsWritten,
	 * since updating dependent state can be fragile, and they aren't
	 * actually used very often. */
	uint32_t InputsRead;
	uint32_t OutputsWritten;
	uint32_t ShadowSamplers; /**< Texture units used for shadow sampling. */

	struct rc_constant_list Constants;
};

enum {
	OPCODE_REPL_ALPHA = MAX_RC_OPCODE /**< used in paired instructions */
};


static inline rc_swizzle get_swz(unsigned int swz, rc_swizzle idx)
{
	if (idx & 0x4)
		return idx;
	return GET_SWZ(swz, idx);
}

static inline unsigned int combine_swizzles4(unsigned int src,
		rc_swizzle swz_x, rc_swizzle swz_y, rc_swizzle swz_z, rc_swizzle swz_w)
{
	unsigned int ret = 0;

	ret |= get_swz(src, swz_x);
	ret |= get_swz(src, swz_y) << 3;
	ret |= get_swz(src, swz_z) << 6;
	ret |= get_swz(src, swz_w) << 9;

	return ret;
}

static inline unsigned int combine_swizzles(unsigned int src, unsigned int swz)
{
	unsigned int ret = 0;

	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_X));
	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_Y)) << 3;
	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_Z)) << 6;
	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_W)) << 9;

	return ret;
}

struct rc_src_register lmul_swizzle(unsigned int swizzle, struct rc_src_register srcreg);

static inline void reset_srcreg(struct rc_src_register* reg)
{
	memset(reg, 0, sizeof(reg));
	reg->Swizzle = RC_SWIZZLE_XYZW;
}


/**
 * A transformation that can be passed to \ref radeonLocalTransform.
 *
 * The function will be called once for each instruction.
 * It has to either emit the appropriate transformed code for the instruction
 * and return true, or return false if it doesn't understand the
 * instruction.
 *
 * The function gets passed the userData as last parameter.
 */
struct radeon_program_transformation {
	int (*function)(
		struct radeon_compiler*,
		struct rc_instruction*,
		void*);
	void *userData;
};

void radeonLocalTransform(
	struct radeon_compiler *c,
	int num_transformations,
	struct radeon_program_transformation* transformations);

unsigned int rc_find_free_temporary(struct radeon_compiler * c);

struct rc_instruction *rc_alloc_instruction(struct radeon_compiler * c);
struct rc_instruction *rc_insert_new_instruction(struct radeon_compiler * c, struct rc_instruction * after);
void rc_remove_instruction(struct rc_instruction * inst);

void rc_print_program(const struct rc_program *prog);

#endif
