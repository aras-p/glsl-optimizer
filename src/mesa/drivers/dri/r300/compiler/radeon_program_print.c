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

#include "radeon_program.h"

#include <stdio.h>

static void print_comment(FILE * f)
{
	fprintf(f, "               # ");
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

static void rc_print_ref(FILE * f, struct rc_dataflow_ref * ref)
{
	fprintf(f, "ref(%p", ref->Vector);

	if (ref->UseMask != RC_MASK_XYZW) {
		fprintf(f, ".");
		rc_print_mask(f, ref->UseMask);
	}

	fprintf(f, ")");
}

static void rc_print_instruction(FILE * f, unsigned int flags, struct rc_instruction * inst)
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

	if (flags & RC_PRINT_DATAFLOW) {
		print_comment(f);

		fprintf(f, "Dst = %p", inst->Dataflow.DstReg);
		if (inst->Dataflow.DstRegAliased)
			fprintf(f, " aliased");
		if (inst->Dataflow.DstRegPrev) {
			fprintf(f, " from ");
			rc_print_ref(f, inst->Dataflow.DstRegPrev);
		}

		for(reg = 0; reg < opcode->NumSrcRegs; ++reg) {
			fprintf(f, ", ");
			if (inst->Dataflow.SrcReg[reg])
				rc_print_ref(f, inst->Dataflow.SrcReg[reg]);
			else
				fprintf(f, "<no ref>");
		}

		fprintf(f, "\n");
	}
}

/**
 * Print program to stderr, default options.
 */
void rc_print_program(const struct rc_program *prog, unsigned int flags)
{
	unsigned int linenum = 0;
	struct rc_instruction *inst;

	fprintf(stderr, "# Radeon Compiler Program%s\n",
			flags & RC_PRINT_DATAFLOW ? " (with dataflow annotations)" : "");

	for(inst = prog->Instructions.Next; inst != &prog->Instructions; inst = inst->Next) {
		fprintf(stderr, "%3d: ", linenum);

		rc_print_instruction(stderr, flags, inst);

		linenum++;
	}
}
