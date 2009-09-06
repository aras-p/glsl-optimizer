/*
 * Copyright (C) 2009 Nicolai Haehnle.
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

#ifndef RADEON_DATAFLOW_H
#define RADEON_DATAFLOW_H

#include "radeon_program_constants.h"

struct radeon_compiler;
struct rc_instruction;
struct rc_swizzle_caps;

struct rc_dataflow_vector;

struct rc_dataflow_ref {
	struct rc_dataflow_vector * Vector;

	/**
	 * Linked list of references to the above-mentioned vector.
	 * The linked list is \em not sorted.
	 */
	/*@{*/
	struct rc_dataflow_ref * Prev;
	struct rc_dataflow_ref * Next;
	/*@}*/

	unsigned int UseMask:4;
	struct rc_instruction * ReadInstruction;
};

struct rc_dataflow_vector {
	rc_register_file File:3;
	unsigned int Index:RC_REGISTER_INDEX_BITS;

	/** For private use in compiler passes. MUST BE RESET TO 0 by the end of each pass.
	 * The annotate pass uses this bit to track whether a vector is in the
	 * update stack.
	 */
	unsigned int PassBit:1;
	/** Which of the components have been written with useful values */
	unsigned int ValidMask:4;
	/** Which of the components are used downstream */
	unsigned int UseMask:4;
	/** The instruction that produced this vector */
	struct rc_instruction * WriteInstruction;

	/** Linked list of references to this vector */
	struct rc_dataflow_ref Refs;
};

struct rc_instruction_dataflow {
	struct rc_dataflow_ref * SrcReg[3];
	struct rc_dataflow_ref * SrcRegAddress[3];

	/** Reference the components of the destination register
	 * that are carried over without being overwritten */
	struct rc_dataflow_ref * DstRegPrev;
	/** Indicates whether the destination register was in use
	 * before this instruction */
	unsigned int DstRegAliased:1;
	struct rc_dataflow_vector * DstReg;
};

/**
 * General functions for manipulating the dataflow structures.
 */
/*@{*/
struct rc_dataflow_ref * rc_dataflow_create_ref(struct radeon_compiler * c,
		struct rc_dataflow_vector * vector, struct rc_instruction * inst);
struct rc_dataflow_vector * rc_dataflow_create_vector(struct radeon_compiler * c,
		rc_register_file file, unsigned int index, struct rc_instruction * inst);
void rc_dataflow_remove_ref(struct rc_dataflow_ref * ref);

void rc_dataflow_remove_instruction(struct rc_instruction * inst);
/*@}*/


/**
 * Compiler passes based on dataflow structures.
 */
/*@{*/
typedef void (*rc_dataflow_mark_outputs_fn)(void * userdata, void * data,
			void (*mark_fn)(void * data, unsigned int index, unsigned int mask));
void rc_dataflow_annotate(struct radeon_compiler * c, rc_dataflow_mark_outputs_fn dce, void * userdata);
void rc_dataflow_dealias(struct radeon_compiler * c);
void rc_dataflow_swizzles(struct radeon_compiler * c);
/*@}*/

#endif /* RADEON_DATAFLOW_H */
