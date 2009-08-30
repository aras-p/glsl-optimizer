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

#include "radeon_mesa_to_rc.h"

#include "main/mtypes.h"
#include "shader/prog_parameter.h"

#include "compiler/radeon_compiler.h"
#include "compiler/radeon_program.h"


void radeon_mesa_to_rc_program(struct radeon_compiler * c, struct gl_program * program)
{
	struct prog_instruction *source;
	unsigned int i;

	for(source = program->Instructions; source->Opcode != OPCODE_END; ++source) {
		struct rc_instruction * dest = rc_insert_new_instruction(c, c->Program.Instructions.Prev);
		dest->I = *source;
	}

	c->Program.ShadowSamplers = program->ShadowSamplers;
	c->Program.InputsRead = program->InputsRead;
	c->Program.OutputsWritten = program->OutputsWritten;

	for(i = 0; i < program->Parameters->NumParameters; ++i) {
		struct rc_constant constant;

		constant.Type = RC_CONSTANT_EXTERNAL;
		constant.Size = 4;
		constant.u.External = i;

		rc_constants_add(&c->Program.Constants, &constant);
	}
}
