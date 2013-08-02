/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 */
#include "r600_pipe.h"
#include "r600_opcodes.h"
#include "r600_shader.h"

#include "util/u_memory.h"
#include "eg_sq.h"
#include <errno.h>

int eg_bytecode_cf_build(struct r600_bytecode *bc, struct r600_bytecode_cf *cf)
{
	unsigned id = cf->id;

	if (cf->op == CF_NATIVE) {
		bc->bytecode[id++] = cf->isa[0];
		bc->bytecode[id++] = cf->isa[1];
	} else {
		const struct cf_op_info *cfop = r600_isa_cf(cf->op);
		unsigned opcode = r600_isa_cf_opcode(bc->isa->hw_class, cf->op);
		if (cfop->flags & CF_ALU) { /* ALU clauses */

			/* prepend ALU_EXTENDED if we need more than 2 kcache sets */
			if (cf->eg_alu_extended) {
				bc->bytecode[id++] =
						S_SQ_CF_ALU_WORD0_EXT_KCACHE_BANK_INDEX_MODE0(V_SQ_CF_INDEX_NONE) |
						S_SQ_CF_ALU_WORD0_EXT_KCACHE_BANK_INDEX_MODE1(V_SQ_CF_INDEX_NONE) |
						S_SQ_CF_ALU_WORD0_EXT_KCACHE_BANK_INDEX_MODE2(V_SQ_CF_INDEX_NONE) |
						S_SQ_CF_ALU_WORD0_EXT_KCACHE_BANK_INDEX_MODE3(V_SQ_CF_INDEX_NONE) |
						S_SQ_CF_ALU_WORD0_EXT_KCACHE_BANK2(cf->kcache[2].bank) |
						S_SQ_CF_ALU_WORD0_EXT_KCACHE_BANK3(cf->kcache[3].bank) |
						S_SQ_CF_ALU_WORD0_EXT_KCACHE_MODE2(cf->kcache[2].mode);
				bc->bytecode[id++] =
						S_SQ_CF_ALU_WORD1_EXT_CF_INST(
							r600_isa_cf_opcode(bc->isa->hw_class, CF_OP_ALU_EXT)) |
						S_SQ_CF_ALU_WORD1_EXT_KCACHE_MODE3(cf->kcache[3].mode) |
						S_SQ_CF_ALU_WORD1_EXT_KCACHE_ADDR2(cf->kcache[2].addr) |
						S_SQ_CF_ALU_WORD1_EXT_KCACHE_ADDR3(cf->kcache[3].addr) |
						S_SQ_CF_ALU_WORD1_EXT_BARRIER(1);
			}
			bc->bytecode[id++] = S_SQ_CF_ALU_WORD0_ADDR(cf->addr >> 1) |
					S_SQ_CF_ALU_WORD0_KCACHE_MODE0(cf->kcache[0].mode) |
					S_SQ_CF_ALU_WORD0_KCACHE_BANK0(cf->kcache[0].bank) |
					S_SQ_CF_ALU_WORD0_KCACHE_BANK1(cf->kcache[1].bank);
			bc->bytecode[id++] = S_SQ_CF_ALU_WORD1_CF_INST(opcode) |
					S_SQ_CF_ALU_WORD1_KCACHE_MODE1(cf->kcache[1].mode) |
					S_SQ_CF_ALU_WORD1_KCACHE_ADDR0(cf->kcache[0].addr) |
					S_SQ_CF_ALU_WORD1_KCACHE_ADDR1(cf->kcache[1].addr) |
					S_SQ_CF_ALU_WORD1_BARRIER(1) |
					S_SQ_CF_ALU_WORD1_COUNT((cf->ndw / 2) - 1);
		} else if (cfop->flags & CF_CLAUSE) {
			/* CF_TEX/VTX (CF_ALU already handled above) */
			bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->addr >> 1);
			bc->bytecode[id++] = S_SQ_CF_WORD1_CF_INST(opcode) |
					S_SQ_CF_WORD1_BARRIER(1) |
					S_SQ_CF_WORD1_COUNT((cf->ndw / 4) - 1);
		} else if (cfop->flags & CF_EXP) {
			/* EXPORT instructions */
			bc->bytecode[id++] = S_SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR(cf->output.gpr) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE(cf->output.elem_size) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(cf->output.array_base) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(cf->output.type) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR(cf->output.index_gpr);
			bc->bytecode[id] =
					S_SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT(cf->output.burst_count - 1) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X(cf->output.swizzle_x) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y(cf->output.swizzle_y) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z(cf->output.swizzle_z) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W(cf->output.swizzle_w) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_BARRIER(cf->barrier) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_CF_INST(opcode);

			if (bc->chip_class == EVERGREEN) /* no EOP on cayman */
				bc->bytecode[id] |= S_SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM(cf->end_of_program);
			id++;
		} else if (cfop->flags & CF_MEM) {
			/* MEM_STREAM, MEM_RING instructions */
			bc->bytecode[id++] = S_SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR(cf->output.gpr) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE(cf->output.elem_size) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(cf->output.array_base) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(cf->output.type) |
					S_SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR(cf->output.index_gpr);
			bc->bytecode[id] = S_SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT(cf->output.burst_count - 1) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_BARRIER(cf->barrier) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_CF_INST(opcode) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK(cf->output.comp_mask) |
					S_SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE(cf->output.array_size);
			if (bc->chip_class == EVERGREEN) /* no EOP on cayman */
				bc->bytecode[id] |= S_SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM(cf->end_of_program);
			id++;
		} else {
			/* other instructions */
			bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->cf_addr >> 1);
			bc->bytecode[id++] =  S_SQ_CF_WORD1_CF_INST(opcode)|
					S_SQ_CF_WORD1_BARRIER(1) |
					S_SQ_CF_WORD1_COND(cf->cond) |
					S_SQ_CF_WORD1_POP_COUNT(cf->pop_count) |
					S_SQ_CF_WORD1_END_OF_PROGRAM(cf->end_of_program);
		}
	}
	return 0;
}

#if 0
void eg_bytecode_export_read(struct r600_bytecode *bc,
		struct r600_bytecode_output *output, uint32_t word0, uint32_t word1)
{
	output->array_base = G_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(word0);
	output->type = G_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(word0);
	output->gpr = G_SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR(word0);
	output->elem_size = G_SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE(word0);

	output->swizzle_x = G_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X(word1);
	output->swizzle_y = G_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y(word1);
	output->swizzle_z = G_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z(word1);
	output->swizzle_w = G_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W(word1);
	output->burst_count = G_SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT(word1);
	output->end_of_program = G_SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM(word1);
	output->op = r600_isa_cf_by_opcode(bc->isa,
			G_SQ_CF_ALLOC_EXPORT_WORD1_CF_INST(word1), /* is_cf_alu = */ 0 );
	output->barrier = G_SQ_CF_ALLOC_EXPORT_WORD1_BARRIER(word1);
	output->array_size = G_SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE(word1);
	output->comp_mask = G_SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK(word1);
}
#endif
