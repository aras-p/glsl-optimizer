/*
 * Copyright 2012 Vadim Girlin <vadimgirlin@gmail.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Vadim Girlin
 */

#include "r600_pipe.h"
#include "r600_isa.h"

int r600_isa_init(struct r600_context *ctx, struct r600_isa *isa) {
	unsigned i;

	assert(ctx->b.chip_class >= R600 && ctx->b.chip_class <= CAYMAN);
	isa->hw_class = ctx->b.chip_class - R600;

	/* reverse lookup maps are required for bytecode parsing */

	isa->alu_op2_map = calloc(256, sizeof(unsigned));
	if (!isa->alu_op2_map)
		return -1;
	isa->alu_op3_map = calloc(256, sizeof(unsigned));
	if (!isa->alu_op3_map)
		return -1;
	isa->fetch_map = calloc(256, sizeof(unsigned));
	if (!isa->fetch_map)
		return -1;
	isa->cf_map = calloc(256, sizeof(unsigned));
	if (!isa->cf_map)
		return -1;

	for (i = 0; i < TABLE_SIZE(alu_op_table); ++i) {
		const struct alu_op_info *op = &alu_op_table[i];
		unsigned opc;
		if (op->flags & AF_LDS || op->slots[isa->hw_class] == 0)
			continue;
		opc = op->opcode[isa->hw_class >> 1];
		assert(opc != -1);
		if (op->src_count == 3)
			isa->alu_op3_map[opc] = i + 1;
		else
			isa->alu_op2_map[opc] = i + 1;
	}

	for (i = 0; i < TABLE_SIZE(fetch_op_table); ++i) {
		const struct fetch_op_info *op = &fetch_op_table[i];
		unsigned opc = op->opcode[isa->hw_class];
		if ((op->flags & FF_GDS) || ((opc & 0xFF) != opc))
			continue; /* ignore GDS ops and INST_MOD versions for now */
		isa->fetch_map[opc] = i + 1;
	}

	for (i = 0; i < TABLE_SIZE(cf_op_table); ++i) {
		const struct cf_op_info *op = &cf_op_table[i];
		unsigned opc = op->opcode[isa->hw_class];
		if (opc == -1)
			continue;
		/* using offset for CF_ALU_xxx opcodes because they overlap with other
		 * CF opcodes (they use different encoding in hw) */
		if (op->flags & CF_ALU)
			opc += 0x80;
		isa->cf_map[opc] = i + 1;
	}

	return 0;
}

int r600_isa_destroy(struct r600_isa *isa) {

	if (!isa)
		return 0;

	if (isa->alu_op2_map)
		free(isa->alu_op2_map);
	if (isa->alu_op3_map)
		free(isa->alu_op3_map);
	if (isa->fetch_map)
		free(isa->fetch_map);
	if (isa->cf_map)
		free(isa->cf_map);

	free(isa);
	return 0;
}



