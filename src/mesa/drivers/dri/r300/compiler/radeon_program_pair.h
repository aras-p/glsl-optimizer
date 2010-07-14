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

#ifndef __RADEON_PROGRAM_PAIR_H_
#define __RADEON_PROGRAM_PAIR_H_

#include "radeon_code.h"
#include "radeon_opcodes.h"
#include "radeon_program_constants.h"

struct radeon_compiler;


/**
 * \file
 * Represents a paired ALU instruction, as found in R300 and R500
 * fragment programs.
 *
 * Note that this representation is taking some liberties as far
 * as register files are concerned, to allow separate register
 * allocation.
 *
 * Also note that there are some subtleties in that the semantics
 * of certain opcodes are implicitly changed in this representation;
 * see \ref rc_pair_translate
 */

/* For rgb and alpha instructions when arg[n].Source = RC_PAIR_PRESUB_SRC, then
 * the presubtract value will be used, and
 * {RGB,Alpha}.Src[RC_PAIR_PRESUB_SRC].File will be set to RC_FILE_PRESUB.
 */
#define RC_PAIR_PRESUB_SRC 3

struct radeon_pair_instruction_source {
	unsigned int Used:1;
	unsigned int File:3;
	unsigned int Index:RC_REGISTER_INDEX_BITS;
};

struct radeon_pair_instruction_rgb {
	unsigned int Opcode:8;
	unsigned int DestIndex:RC_REGISTER_INDEX_BITS;
	unsigned int WriteMask:3;
    unsigned int Target:2;
	unsigned int OutputWriteMask:3;
	unsigned int Saturate:1;

	struct radeon_pair_instruction_source Src[4];

	struct {
		unsigned int Source:2;
		unsigned int Swizzle:9;
		unsigned int Abs:1;
		unsigned int Negate:1;
	} Arg[3];
};

struct radeon_pair_instruction_alpha {
	unsigned int Opcode:8;
	unsigned int DestIndex:RC_REGISTER_INDEX_BITS;
	unsigned int WriteMask:1;
    unsigned int Target:2;
	unsigned int OutputWriteMask:1;
	unsigned int DepthWriteMask:1;
	unsigned int Saturate:1;

	struct radeon_pair_instruction_source Src[4];

	struct {
		unsigned int Source:2;
		unsigned int Swizzle:3;
		unsigned int Abs:1;
		unsigned int Negate:1;
	} Arg[3];
};

struct rc_pair_instruction {
	struct radeon_pair_instruction_rgb RGB;
	struct radeon_pair_instruction_alpha Alpha;

	unsigned int WriteALUResult:2;
	unsigned int ALUResultCompare:3;
	unsigned int Nop:1;
};

typedef void (*rc_pair_foreach_src_fn)
			(void *, struct radeon_pair_instruction_source *);

typedef enum {
	RC_PAIR_SOURCE_NONE = 0,
	RC_PAIR_SOURCE_RGB,
	RC_PAIR_SOURCE_ALPHA
} rc_pair_source_type;

/**
 * General helper functions for dealing with the paired instruction format.
 */
/*@{*/
int rc_pair_alloc_source(struct rc_pair_instruction *pair,
	unsigned int rgb, unsigned int alpha,
	rc_register_file file, unsigned int index);

void rc_pair_foreach_source_that_alpha_reads(
	struct rc_pair_instruction * pair,
	void * data,
	rc_pair_foreach_src_fn cb);

void rc_pair_foreach_source_that_rgb_reads(
	struct rc_pair_instruction * pair,
	void * data,
	rc_pair_foreach_src_fn cb);

rc_pair_source_type rc_source_type_that_arg_reads(
	unsigned int source,
	unsigned int swizzle,
	unsigned int channels);
/*@}*/


/**
 * Compiler passes that operate with the paired format.
 */
/*@{*/
struct radeon_pair_handler;

void rc_pair_translate(struct radeon_compiler *cc, void *user);
void rc_pair_schedule(struct radeon_compiler *cc, void *user);
void rc_pair_regalloc(struct radeon_compiler *cc, void *user);
void rc_pair_regalloc_inputs_only(struct radeon_compiler *cc, void *user);
/*@}*/

#endif /* __RADEON_PROGRAM_PAIR_H_ */
