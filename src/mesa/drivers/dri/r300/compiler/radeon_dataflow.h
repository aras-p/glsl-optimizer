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


/**
 * Help analyze the register accesses of instructions.
 */
/*@{*/
typedef void (*rc_read_write_fn)(void * userdata, struct rc_instruction * inst,
			rc_register_file file, unsigned int index, unsigned int chan);
void rc_for_all_reads(struct rc_instruction * inst, rc_read_write_fn cb, void * userdata);
void rc_for_all_writes(struct rc_instruction * inst, rc_read_write_fn cb, void * userdata);
/*@}*/


/**
 * Compiler passes based on dataflow analysis.
 */
/*@{*/
typedef void (*rc_dataflow_mark_outputs_fn)(void * userdata, void * data,
			void (*mark_fn)(void * data, unsigned int index, unsigned int mask));
void rc_dataflow_deadcode(struct radeon_compiler * c, rc_dataflow_mark_outputs_fn dce, void * userdata);
void rc_dataflow_swizzles(struct radeon_compiler * c);
/*@}*/

#endif /* RADEON_DATAFLOW_H */
