/*
 * Copyright 2012 Advanced Micro Devices, Inc.
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
 *      Christian König <christian.koenig@amd.com>
 */

#include "sid.h"
#include "si_pipe.h"

void si_cmd_context_control(struct si_pm4_state *pm4)
{
	si_pm4_cmd_begin(pm4, PKT3_CONTEXT_CONTROL);
	si_pm4_cmd_add(pm4, 0x80000000);
	si_pm4_cmd_add(pm4, 0x80000000);
	si_pm4_cmd_end(pm4, false);
}

void si_cmd_draw_index_2(struct si_pm4_state *pm4, uint32_t max_size,
			 uint64_t index_base, uint32_t index_count,
			 uint32_t initiator, bool predicate)
{
	si_pm4_cmd_begin(pm4, PKT3_DRAW_INDEX_2);
	si_pm4_cmd_add(pm4, max_size);
	si_pm4_cmd_add(pm4, index_base);
	si_pm4_cmd_add(pm4, (index_base >> 32UL) & 0xFF);
	si_pm4_cmd_add(pm4, index_count);
	si_pm4_cmd_add(pm4, initiator);
	si_pm4_cmd_end(pm4, predicate);
}

void si_cmd_draw_index_auto(struct si_pm4_state *pm4, uint32_t count,
			    uint32_t initiator, bool predicate)
{
	si_pm4_cmd_begin(pm4, PKT3_DRAW_INDEX_AUTO);
	si_pm4_cmd_add(pm4, count);
	si_pm4_cmd_add(pm4, initiator);
	si_pm4_cmd_end(pm4, predicate);
}

void si_cmd_draw_indirect(struct si_pm4_state *pm4, uint64_t indirect_va,
			  uint32_t indirect_offset, uint32_t base_vtx_loc,
			  uint32_t start_inst_loc, bool predicate)
{
	assert(indirect_va % 8 == 0);
	assert(indirect_offset % 4 == 0);

	si_pm4_cmd_begin(pm4, PKT3_SET_BASE);
	si_pm4_cmd_add(pm4, 1);
	si_pm4_cmd_add(pm4, indirect_va);
	si_pm4_cmd_add(pm4, indirect_va >> 32);
	si_pm4_cmd_end(pm4, predicate);

	si_pm4_cmd_begin(pm4, PKT3_DRAW_INDIRECT);
	si_pm4_cmd_add(pm4, indirect_offset);
	si_pm4_cmd_add(pm4, (base_vtx_loc - SI_SH_REG_OFFSET) >> 2);
	si_pm4_cmd_add(pm4, (start_inst_loc - SI_SH_REG_OFFSET) >> 2);
	si_pm4_cmd_add(pm4, V_0287F0_DI_SRC_SEL_AUTO_INDEX);
	si_pm4_cmd_end(pm4, predicate);
}

void si_cmd_draw_index_indirect(struct si_pm4_state *pm4, uint64_t indirect_va,
				uint64_t index_va, uint32_t index_max_size,
				uint32_t indirect_offset, uint32_t base_vtx_loc,
				uint32_t start_inst_loc, bool predicate)
{
	assert(indirect_va % 8 == 0);
	assert(index_va % 2 == 0);
	assert(indirect_offset % 4 == 0);

	si_pm4_cmd_begin(pm4, PKT3_SET_BASE);
	si_pm4_cmd_add(pm4, 1);
	si_pm4_cmd_add(pm4, indirect_va);
	si_pm4_cmd_add(pm4, indirect_va >> 32);
	si_pm4_cmd_end(pm4, predicate);

	si_pm4_cmd_begin(pm4, PKT3_INDEX_BASE);
	si_pm4_cmd_add(pm4, index_va);
	si_pm4_cmd_add(pm4, index_va >> 32);
	si_pm4_cmd_end(pm4, predicate);

	si_pm4_cmd_begin(pm4, PKT3_INDEX_BUFFER_SIZE);
	si_pm4_cmd_add(pm4, index_max_size);
	si_pm4_cmd_end(pm4, predicate);

	si_pm4_cmd_begin(pm4, PKT3_DRAW_INDEX_INDIRECT);
	si_pm4_cmd_add(pm4, indirect_offset);
	si_pm4_cmd_add(pm4, (base_vtx_loc - SI_SH_REG_OFFSET) >> 2);
	si_pm4_cmd_add(pm4, (start_inst_loc - SI_SH_REG_OFFSET) >> 2);
	si_pm4_cmd_add(pm4, V_0287F0_DI_SRC_SEL_DMA);
	si_pm4_cmd_end(pm4, predicate);
}
