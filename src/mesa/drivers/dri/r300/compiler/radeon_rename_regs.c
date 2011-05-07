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

/**
 * \file
 */

#include "radeon_rename_regs.h"

#include "radeon_compiler.h"
#include "radeon_dataflow.h"
#include "radeon_program.h"

/**
 * This function renames registers in an attempt to get the code close to
 * SSA form.  After this function has completed, most of the register are only
 * written to one time, with a few exceptions.
 *
 * This function assumes all the instructions are still of type
 * RC_INSTRUCTION_NORMAL.
 */
void rc_rename_regs(struct radeon_compiler *c, void *user)
{
	unsigned int i, used_length;
	int new_index;
	struct rc_instruction * inst;
	struct rc_reader_data reader_data;
	unsigned char * used;

	/* XXX Remove this once the register allocation works with flow control. */
	for(inst = c->Program.Instructions.Next;
					inst != &c->Program.Instructions;
					inst = inst->Next) {
		if (inst->U.I.Opcode == RC_OPCODE_BGNLOOP)
			return;
	}

	used_length = 2 * rc_recompute_ips(c);
	used = memory_pool_malloc(&c->Pool, sizeof(unsigned char) * used_length);
	memset(used, 0, sizeof(unsigned char) * used_length);

	rc_get_used_temporaries(c, used, used_length);
	for(inst = c->Program.Instructions.Next;
					inst != &c->Program.Instructions;
					inst = inst->Next) {

		if (inst->U.I.DstReg.File != RC_FILE_TEMPORARY)
			continue;

		reader_data.ExitOnAbort = 1;
		rc_get_readers(c, inst, &reader_data, NULL, NULL, NULL);

		if (reader_data.Abort || reader_data.ReaderCount == 0)
			continue;

		new_index = rc_find_free_temporary_list(c, used, used_length,
						RC_MASK_XYZW);
		if (new_index < 0) {
			rc_error(c, "Ran out of temporary registers\n");
			return;
		}

		reader_data.Writer->U.I.DstReg.Index = new_index;
		for(i = 0; i < reader_data.ReaderCount; i++) {
			reader_data.Readers[i].U.I.Src->Index = new_index;
		}
	}
}
