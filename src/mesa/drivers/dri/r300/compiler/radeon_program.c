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

#include "radeon_program.h"

#include <stdio.h>

#include "radeon_compiler.h"


/**
 * Transform the given clause in the following way:
 *  1. Replace it with an empty clause
 *  2. For every instruction in the original clause, try the given
 *     transformations in order.
 *  3. If one of the transformations returns GL_TRUE, assume that it
 *     has emitted the appropriate instruction(s) into the new clause;
 *     otherwise, copy the instruction verbatim.
 *
 * \note The transformation is currently not recursive; in other words,
 * instructions emitted by transformations are not transformed.
 *
 * \note The transform is called 'local' because it can only look at
 * one instruction at a time.
 */
void radeonLocalTransform(
	struct radeon_compiler * c,
	int num_transformations,
	struct radeon_program_transformation* transformations)
{
	struct rc_instruction * inst = c->Program.Instructions.Next;

	while(inst != &c->Program.Instructions) {
		struct rc_instruction * current = inst;
		int i;

		inst = inst->Next;

		for(i = 0; i < num_transformations; ++i) {
			struct radeon_program_transformation* t = transformations + i;

			if (t->function(c, current, t->userData))
				break;
		}
	}
}

/**
 * Left multiplication of a register with a swizzle
 */
struct rc_src_register lmul_swizzle(unsigned int swizzle, struct rc_src_register srcreg)
{
	struct rc_src_register tmp = srcreg;
	int i;
	tmp.Swizzle = 0;
	tmp.Negate = 0;
	for(i = 0; i < 4; ++i) {
		rc_swizzle swz = GET_SWZ(swizzle, i);
		if (swz < 4) {
			tmp.Swizzle |= GET_SWZ(srcreg.Swizzle, swz) << (i*3);
			tmp.Negate |= GET_BIT(srcreg.Negate, swz) << i;
		} else {
			tmp.Swizzle |= swz << (i*3);
		}
	}
	return tmp;
}

unsigned int rc_find_free_temporary(struct radeon_compiler * c)
{
	char used[RC_REGISTER_MAX_INDEX];
	unsigned int i;

	memset(used, 0, sizeof(used));

	for (struct rc_instruction * rcinst = c->Program.Instructions.Next; rcinst != &c->Program.Instructions; rcinst = rcinst->Next) {
		const struct rc_sub_instruction *inst = &rcinst->I;
		const struct rc_opcode_info *opcode = rc_get_opcode_info(inst->Opcode);
		unsigned int k;

		for (k = 0; k < opcode->NumSrcRegs; k++) {
			if (inst->SrcReg[k].File == RC_FILE_TEMPORARY)
				used[inst->SrcReg[k].Index] = 1;
		}

		if (opcode->HasDstReg) {
			if (inst->DstReg.File == RC_FILE_TEMPORARY)
				used[inst->DstReg.Index] = 1;
		}
	}

	for (i = 0; i < RC_REGISTER_MAX_INDEX; i++) {
		if (!used[i])
			return i;
	}

	rc_error(c, "Ran out of temporary registers\n");
	return 0;
}


struct rc_instruction *rc_alloc_instruction(struct radeon_compiler * c)
{
	struct rc_instruction * inst = memory_pool_malloc(&c->Pool, sizeof(struct rc_instruction));

	memset(inst, 0, sizeof(struct rc_instruction));

	inst->I.Opcode = RC_OPCODE_ILLEGAL_OPCODE;
	inst->I.DstReg.WriteMask = RC_MASK_XYZW;
	inst->I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;
	inst->I.SrcReg[1].Swizzle = RC_SWIZZLE_XYZW;
	inst->I.SrcReg[2].Swizzle = RC_SWIZZLE_XYZW;

	return inst;
}


struct rc_instruction *rc_insert_new_instruction(struct radeon_compiler * c, struct rc_instruction * after)
{
	struct rc_instruction * inst = rc_alloc_instruction(c);

	inst->Prev = after;
	inst->Next = after->Next;

	inst->Prev->Next = inst;
	inst->Next->Prev = inst;

	return inst;
}

void rc_remove_instruction(struct rc_instruction * inst)
{
	inst->Prev->Next = inst->Next;
	inst->Next->Prev = inst->Prev;
}

static const char * textarget_to_string(rc_texture_target target)
{
	switch(target) {
	case RC_TEXTURE_2D_ARRAY: return "2D_ARRAY";
	case RC_TEXTURE_1D_ARRAY: return "1D_ARRAY";
	case RC_TEXTURE_CUBE: return "CUBE";
	case RC_TEXTURE_3D: return "3D";
	case RC_TEXTURE_RECT: return "RECT";
	case RC_TEXTURE_2D: return "2D";
	case RC_TEXTURE_1D: return "1D";
	default: return "BAD_TEXTURE_TARGET";
	}
}

static void rc_print_register(FILE * f, rc_register_file file, int index, unsigned int reladdr)
{
	if (file == RC_FILE_NONE) {
		fprintf(f, "none");
	} else {
		const char * filename;
		switch(file) {
		case RC_FILE_TEMPORARY: filename = "temp"; break;
		case RC_FILE_INPUT: filename = "input"; break;
		case RC_FILE_OUTPUT: filename = "output"; break;
		case RC_FILE_ADDRESS: filename = "addr"; break;
		case RC_FILE_CONSTANT: filename = "const"; break;
		default: filename = "BAD FILE"; break;
		}
		fprintf(f, "%s[%i%s]", filename, index, reladdr ? " + addr[0]" : "");
	}
}

static void rc_print_mask(FILE * f, unsigned int mask)
{
	if (mask & RC_MASK_X) fprintf(f, "x");
	if (mask & RC_MASK_Y) fprintf(f, "y");
	if (mask & RC_MASK_Z) fprintf(f, "z");
	if (mask & RC_MASK_W) fprintf(f, "w");
}

static void rc_print_dst_register(FILE * f, struct rc_dst_register dst)
{
	rc_print_register(f, dst.File, dst.Index, dst.RelAddr);
	if (dst.WriteMask != RC_MASK_XYZW) {
		fprintf(f, ".");
		rc_print_mask(f, dst.WriteMask);
	}
}

static void rc_print_swizzle(FILE * f, unsigned int swizzle, unsigned int negate)
{
	unsigned int comp;
	for(comp = 0; comp < 4; ++comp) {
		rc_swizzle swz = GET_SWZ(swizzle, comp);
		if (GET_BIT(negate, comp))
			fprintf(f, "-");
		switch(swz) {
		case RC_SWIZZLE_X: fprintf(f, "x"); break;
		case RC_SWIZZLE_Y: fprintf(f, "y"); break;
		case RC_SWIZZLE_Z: fprintf(f, "z"); break;
		case RC_SWIZZLE_W: fprintf(f, "w"); break;
		case RC_SWIZZLE_ZERO: fprintf(f, "0"); break;
		case RC_SWIZZLE_ONE: fprintf(f, "1"); break;
		case RC_SWIZZLE_HALF: fprintf(f, "H"); break;
		case RC_SWIZZLE_UNUSED: fprintf(f, "_"); break;
		}
	}
}

static void rc_print_src_register(FILE * f, struct rc_src_register src)
{
	int trivial_negate = (src.Negate == RC_MASK_NONE || src.Negate == RC_MASK_XYZW);

	if (src.Negate == RC_MASK_XYZW)
		fprintf(f, "-");
	if (src.Abs)
		fprintf(f, "|");

	rc_print_register(f, src.File, src.Index, src.RelAddr);

	if (src.Abs && !trivial_negate)
		fprintf(f, "|");

	if (src.Swizzle != RC_SWIZZLE_XYZW || !trivial_negate) {
		fprintf(f, ".");
		rc_print_swizzle(f, src.Swizzle, trivial_negate ? 0 : src.Negate);
	}

	if (src.Abs && trivial_negate)
		fprintf(f, "|");
}

static void rc_print_instruction(FILE * f, struct rc_instruction * inst)
{
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->I.Opcode);
	unsigned int reg;

	fprintf(f, "%s", opcode->Name);

	switch(inst->I.SaturateMode) {
	case RC_SATURATE_NONE: break;
	case RC_SATURATE_ZERO_ONE: fprintf(f, "_SAT"); break;
	case RC_SATURATE_MINUS_PLUS_ONE: fprintf(f, "_SAT2"); break;
	default: fprintf(f, "_BAD_SAT"); break;
	}

	if (opcode->HasDstReg) {
		fprintf(f, " ");
		rc_print_dst_register(f, inst->I.DstReg);
		if (opcode->NumSrcRegs)
			fprintf(f, ",");
	}

	for(reg = 0; reg < opcode->NumSrcRegs; ++reg) {
		if (reg > 0)
			fprintf(f, ",");
		fprintf(f, " ");
		rc_print_src_register(f, inst->I.SrcReg[reg]);
	}

	if (opcode->HasTexture) {
		fprintf(f, ", %s%s[%u]",
			textarget_to_string(inst->I.TexSrcTarget),
			inst->I.TexShadow ? "SHADOW" : "",
			inst->I.TexSrcUnit);
	}

	fprintf(f, ";\n");
}

/**
 * Print program to stderr, default options.
 */
void rc_print_program(const struct rc_program *prog)
{
	unsigned int linenum = 0;
	struct rc_instruction *inst;

	fprintf(stderr, "# Radeon Compiler Program\n");

	for(inst = prog->Instructions.Next; inst != &prog->Instructions; inst = inst->Next) {
		fprintf(stderr, "%3d: ", linenum);

		rc_print_instruction(stderr, inst);

		linenum++;
	}
}
