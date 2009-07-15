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

#ifndef __RADEON_PROGRAM_ALU_H_
#define __RADEON_PROGRAM_ALU_H_

#include "radeon_program.h"

GLboolean radeonTransformALU(
	struct radeon_transform_context *t,
	struct prog_instruction*,
	void*);

GLboolean radeonTransformTrigSimple(
	struct radeon_transform_context *t,
	struct prog_instruction*,
	void*);

GLboolean radeonTransformTrigScale(
	struct radeon_transform_context *t,
	struct prog_instruction*,
	void*);

GLboolean radeonTransformDeriv(
	struct radeon_transform_context *t,
	struct prog_instruction*,
	void*);

#endif /* __RADEON_PROGRAM_ALU_H_ */
