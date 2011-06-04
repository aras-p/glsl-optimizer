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
#include <stdio.h>
#include <errno.h>
#include "util/u_memory.h"
#include "r600_pipe.h"
#include "r600_asm.h"
#include "eg_sq.h"
#include "r600_opcodes.h"
#include "evergreend.h"

int eg_bc_cf_build(struct r600_bc *bc, struct r600_bc_cf *cf)
{
	unsigned id = cf->id;

	switch (cf->inst) {
	case (EG_V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU << 3):
	case (EG_V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP_AFTER << 3):
	case (EG_V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_POP2_AFTER << 3):
	case (EG_V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU_PUSH_BEFORE << 3):
		bc->bytecode[id++] = S_SQ_CF_ALU_WORD0_ADDR(cf->addr >> 1) |
			S_SQ_CF_ALU_WORD0_KCACHE_MODE0(cf->kcache[0].mode) |
			S_SQ_CF_ALU_WORD0_KCACHE_BANK0(cf->kcache[0].bank) |
			S_SQ_CF_ALU_WORD0_KCACHE_BANK1(cf->kcache[1].bank);
		bc->bytecode[id++] = S_SQ_CF_ALU_WORD1_CF_INST(cf->inst >> 3) |
			S_SQ_CF_ALU_WORD1_KCACHE_MODE1(cf->kcache[1].mode) |
			S_SQ_CF_ALU_WORD1_KCACHE_ADDR0(cf->kcache[0].addr) |
			S_SQ_CF_ALU_WORD1_KCACHE_ADDR1(cf->kcache[1].addr) |
					S_SQ_CF_ALU_WORD1_BARRIER(1) |
					S_SQ_CF_ALU_WORD1_COUNT((cf->ndw / 2) - 1);
		break;
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_TEX:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_VTX:
		bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->addr >> 1);
		bc->bytecode[id++] = S_SQ_CF_WORD1_CF_INST(cf->inst) |
					S_SQ_CF_WORD1_BARRIER(1) |
					S_SQ_CF_WORD1_COUNT((cf->ndw / 4) - 1);
		break;
	case EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT:
	case EG_V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE:
		bc->bytecode[id++] = S_SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR(cf->output.gpr) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE(cf->output.elem_size) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(cf->output.array_base) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(cf->output.type);
		bc->bytecode[id] = S_SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT(cf->output.burst_count - 1) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X(cf->output.swizzle_x) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y(cf->output.swizzle_y) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z(cf->output.swizzle_z) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W(cf->output.swizzle_w) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_BARRIER(cf->output.barrier) |
			S_SQ_CF_ALLOC_EXPORT_WORD1_CF_INST(cf->output.inst);
		if (bc->chiprev == CHIPREV_EVERGREEN) /* no EOP on cayman */
			bc->bytecode[id] |= S_SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM(cf->output.end_of_program);
		id++;

		break;
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_JUMP:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_ELSE:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_POP:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_START_NO_AL:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_END:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_CONTINUE:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_LOOP_BREAK:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_CALL_FS:
	case EG_V_SQ_CF_WORD1_SQ_CF_INST_RETURN:
	case CM_V_SQ_CF_WORD1_SQ_CF_INST_END:
		bc->bytecode[id++] = S_SQ_CF_WORD0_ADDR(cf->cf_addr >> 1);
		bc->bytecode[id++] = S_SQ_CF_WORD1_CF_INST(cf->inst) |
					S_SQ_CF_WORD1_BARRIER(1) |
					S_SQ_CF_WORD1_COND(cf->cond) |
					S_SQ_CF_WORD1_POP_COUNT(cf->pop_count);

		break;
	default:
		R600_ERR("unsupported CF instruction (0x%X)\n", cf->inst);
		return -EINVAL;
	}
	return 0;
}
