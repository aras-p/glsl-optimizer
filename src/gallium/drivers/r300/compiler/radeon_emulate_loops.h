/*
 * Copyright 2010 Tom Stellard <tstellar@gmail.com>
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

#ifndef RADEON_EMULATE_LOOPS_H
#define RADEON_EMULATE_LOOPS_H

#define MAX_ITERATIONS 8

struct radeon_compiler;

struct loop_info {
	struct rc_instruction * BeginLoop;
	struct rc_instruction * Cond;
	struct rc_instruction * If;
	struct rc_instruction * Brk;
	struct rc_instruction * EndIf;
	struct rc_instruction * EndLoop;
};

struct emulate_loop_state {
	struct radeon_compiler * C;
	struct loop_info * Loops;
	unsigned int LoopCount;
	unsigned int LoopReserved;
};

void rc_transform_loops(struct radeon_compiler *c, void *user);

void rc_unroll_loops(struct radeon_compiler * c, void *user);

void rc_emulate_loops(struct radeon_compiler * c, void *user);

#endif /* RADEON_EMULATE_LOOPS_H */
