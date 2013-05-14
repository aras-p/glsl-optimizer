/*
 * Copyright 2013 Vadim Girlin <vadimgirlin@gmail.com>
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

#include "sb_bc.h"

namespace r600_sb {

int bc_decoder::decode_cf(unsigned &i, bc_cf& bc) {
	int r = 0;
	uint32_t dw0 = dw[i];
	uint32_t dw1 = dw[i+1];

	if ((dw1 >> 29) & 1) { // CF_ALU
		return decode_cf_alu(i, bc);
	} else {
		// CF_INST field encoding on cayman is the same as on evergreen
		unsigned opcode = ctx.is_egcm() ?
				CF_WORD1_EG(dw1).get_CF_INST() :
				CF_WORD1_R6R7(dw1).get_CF_INST();

		bc.set_op(r600_isa_cf_by_opcode(ctx.isa, opcode, 0));

		if (bc.op_ptr->flags & CF_EXP) {
			return decode_cf_exp(i, bc);
		} else if (bc.op_ptr->flags & CF_MEM) {
			return decode_cf_mem(i, bc);
		}

		if (ctx.is_egcm()) {
			CF_WORD0_EGCM w0(dw0);
			bc.addr = w0.get_ADDR();
			bc.jumptable_sel = w0.get_JUMPTABLE_SEL();

			if (ctx.is_evergreen()) {
				CF_WORD1_EG w1(dw1);

				bc.barrier = w1.get_BARRIER();
				bc.cf_const = w1.get_CF_CONST();
				bc.cond = w1.get_COND();
				bc.count = w1.get_COUNT();
				bc.end_of_program = w1.get_END_OF_PROGRAM();
				bc.pop_count = w1.get_POP_COUNT();
				bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
				bc.whole_quad_mode = w1.get_WHOLE_QUAD_MODE();

			} else { // cayman
				CF_WORD1_CM w1(dw1);

				bc.barrier = w1.get_BARRIER();
				bc.cf_const = w1.get_CF_CONST();
				bc.cond = w1.get_COND();
				bc.count = w1.get_COUNT();
				bc.pop_count = w1.get_POP_COUNT();
				bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
			}


		} else {
			CF_WORD0_R6R7 w0(dw0);
			bc.addr = w0.get_ADDR();

			CF_WORD1_R6R7 w1(dw1);
			bc.barrier = w1.get_BARRIER();
			bc.cf_const = w1.get_CF_CONST();
			bc.cond = w1.get_COND();

			if (ctx.is_r600())
				bc.count = w1.get_COUNT();
			else
				bc.count = w1.get_COUNT() + (w1.get_COUNT_3() << 3);

			bc.end_of_program = w1.get_END_OF_PROGRAM();
			bc.pop_count = w1.get_POP_COUNT();
			bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
			bc.whole_quad_mode = w1.get_WHOLE_QUAD_MODE();
			bc.call_count = w1.get_CALL_COUNT();
		}
	}

	i += 2;

	return r;
}

int bc_decoder::decode_cf_alu(unsigned & i, bc_cf& bc) {
	int r = 0;
	uint32_t dw0 = dw[i++];
	uint32_t dw1 = dw[i++];

	assert(i <= ndw);

	CF_ALU_WORD0_ALL w0(dw0);

	bc.kc[0].bank = w0.get_KCACHE_BANK0();
	bc.kc[1].bank = w0.get_KCACHE_BANK1();
	bc.kc[0].mode = w0.get_KCACHE_MODE0();

	bc.addr = w0.get_ADDR();

	if (ctx.is_r600()) {
		CF_ALU_WORD1_R6 w1(dw1);

		bc.set_op(r600_isa_cf_by_opcode(ctx.isa, w1.get_CF_INST(), 1));

		bc.kc[0].addr = w1.get_KCACHE_ADDR0();
		bc.kc[1].mode = w1.get_KCACHE_MODE1();
		bc.kc[1].addr = w1.get_KCACHE_ADDR1();

		bc.barrier = w1.get_BARRIER();
		bc.count = w1.get_COUNT();
		bc.whole_quad_mode = w1.get_WHOLE_QUAD_MODE();

		bc.uses_waterfall = w1.get_USES_WATERFALL();
	} else {
		CF_ALU_WORD1_R7EGCM w1(dw1);

		bc.set_op(r600_isa_cf_by_opcode(ctx.isa, w1.get_CF_INST(), 1));

		if (bc.op == CF_OP_ALU_EXT) {
			CF_ALU_WORD0_EXT_EGCM w0(dw0);
			CF_ALU_WORD1_EXT_EGCM w1(dw1);

			bc.kc[0].index_mode = w0.get_KCACHE_BANK_INDEX_MODE0();
			bc.kc[1].index_mode = w0.get_KCACHE_BANK_INDEX_MODE1();
			bc.kc[2].index_mode = w0.get_KCACHE_BANK_INDEX_MODE2();
			bc.kc[3].index_mode = w0.get_KCACHE_BANK_INDEX_MODE3();
			bc.kc[2].bank = w0.get_KCACHE_BANK2();
			bc.kc[3].bank = w0.get_KCACHE_BANK3();
			bc.kc[2].mode = w0.get_KCACHE_MODE2();
			bc.kc[3].mode = w1.get_KCACHE_MODE3();
			bc.kc[2].addr = w1.get_KCACHE_ADDR2();
			bc.kc[3].addr = w1.get_KCACHE_ADDR3();

			r = decode_cf_alu(i, bc);

		} else {

			bc.kc[0].addr = w1.get_KCACHE_ADDR0();
			bc.kc[1].mode = w1.get_KCACHE_MODE1();
			bc.kc[1].addr = w1.get_KCACHE_ADDR1();
			bc.barrier = w1.get_BARRIER();
			bc.count = w1.get_COUNT();
			bc.whole_quad_mode = w1.get_WHOLE_QUAD_MODE();

			bc.alt_const = w1.get_ALT_CONST();
		}
	}
	return r;
}

int bc_decoder::decode_cf_exp(unsigned & i, bc_cf& bc) {
	int r = 0;
	uint32_t dw0 = dw[i++];
	uint32_t dw1 = dw[i++];
	assert(i <= ndw);

	CF_ALLOC_EXPORT_WORD0_ALL w0(dw0);
	bc.array_base = w0.get_ARRAY_BASE();
	bc.elem_size = w0.get_ELEM_SIZE();
	bc.index_gpr = w0.get_INDEX_GPR();
	bc.rw_gpr = w0.get_RW_GPR();
	bc.rw_rel = w0.get_RW_REL();
	bc.type = w0.get_TYPE();

	if (ctx.is_evergreen()) {
		CF_ALLOC_EXPORT_WORD1_SWIZ_EG w1(dw1);
		bc.barrier = w1.get_BARRIER();
		bc.burst_count = w1.get_BURST_COUNT();
		bc.end_of_program = w1.get_END_OF_PROGRAM();
		bc.sel[0] = w1.get_SEL_X();
		bc.sel[1] = w1.get_SEL_Y();
		bc.sel[2] = w1.get_SEL_Z();
		bc.sel[3] = w1.get_SEL_W();
		bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
		bc.mark = w1.get_MARK();

	} else if (ctx.is_cayman()) {
		CF_ALLOC_EXPORT_WORD1_SWIZ_CM w1(dw1);
		bc.barrier = w1.get_BARRIER();
		bc.burst_count = w1.get_BURST_COUNT();
		bc.mark = w1.get_MARK();
		bc.sel[0] = w1.get_SEL_X();
		bc.sel[1] = w1.get_SEL_Y();
		bc.sel[2] = w1.get_SEL_Z();
		bc.sel[3] = w1.get_SEL_W();
		bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();

	} else { // r67
		CF_ALLOC_EXPORT_WORD1_SWIZ_R6R7 w1(dw1);
		bc.barrier = w1.get_BARRIER();
		bc.burst_count = w1.get_BURST_COUNT();
		bc.end_of_program = w1.get_END_OF_PROGRAM();
		bc.sel[0] = w1.get_SEL_X();
		bc.sel[1] = w1.get_SEL_Y();
		bc.sel[2] = w1.get_SEL_Z();
		bc.sel[3] = w1.get_SEL_W();
		bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
		bc.whole_quad_mode = w1.get_WHOLE_QUAD_MODE();
	}

	return r;
}


int bc_decoder::decode_cf_mem(unsigned & i, bc_cf& bc) {
	int r = 0;
	uint32_t dw0 = dw[i++];
	uint32_t dw1 = dw[i++];
	assert(i <= ndw);

	if (!(bc.op_ptr->flags & CF_RAT)) {
		CF_ALLOC_EXPORT_WORD0_ALL w0(dw0);
		bc.array_base = w0.get_ARRAY_BASE();
		bc.elem_size = w0.get_ELEM_SIZE();
		bc.index_gpr = w0.get_INDEX_GPR();
		bc.rw_gpr = w0.get_RW_GPR();
		bc.rw_rel = w0.get_RW_REL();
		bc.type = w0.get_TYPE();
	} else {
		assert(ctx.is_egcm());
		CF_ALLOC_EXPORT_WORD0_RAT_EGCM w0(dw0);
		bc.elem_size = w0.get_ELEM_SIZE();
		bc.index_gpr = w0.get_INDEX_GPR();
		bc.rw_gpr = w0.get_RW_GPR();
		bc.rw_rel = w0.get_RW_REL();
		bc.type = w0.get_TYPE();
		bc.rat_id = w0.get_RAT_ID();
		bc.rat_inst = w0.get_RAT_INST();
		bc.rat_index_mode = w0.get_RAT_INDEX_MODE();
	}

	if (ctx.is_evergreen()) {
		CF_ALLOC_EXPORT_WORD1_BUF_EG w1(dw1);
		bc.barrier = w1.get_BARRIER();
		bc.burst_count = w1.get_BURST_COUNT();
		bc.end_of_program = w1.get_END_OF_PROGRAM();
		bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
		bc.mark = w1.get_MARK();
		bc.array_size = w1.get_ARRAY_SIZE();
		bc.comp_mask = w1.get_COMP_MASK();

	} else if (ctx.is_cayman()) {
		CF_ALLOC_EXPORT_WORD1_BUF_CM w1(dw1);
		bc.barrier = w1.get_BARRIER();
		bc.burst_count = w1.get_BURST_COUNT();
		bc.mark = w1.get_MARK();
		bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
		bc.array_size = w1.get_ARRAY_SIZE();
		bc.comp_mask = w1.get_COMP_MASK();

	} else { // r67
		CF_ALLOC_EXPORT_WORD1_BUF_R6R7 w1(dw1);
		bc.barrier = w1.get_BARRIER();
		bc.burst_count = w1.get_BURST_COUNT();
		bc.end_of_program = w1.get_END_OF_PROGRAM();
		bc.valid_pixel_mode = w1.get_VALID_PIXEL_MODE();
		bc.whole_quad_mode = w1.get_WHOLE_QUAD_MODE();
		bc.array_size = w1.get_ARRAY_SIZE();
		bc.comp_mask = w1.get_COMP_MASK();
		bc.whole_quad_mode = w1.get_WHOLE_QUAD_MODE();
	}

	return r;
}

int bc_decoder::decode_alu(unsigned & i, bc_alu& bc) {
	int r = 0;
	uint32_t dw0 = dw[i++];
	uint32_t dw1 = dw[i++];
	assert(i <= ndw);

	ALU_WORD0_ALL w0(dw0);
	bc.index_mode = w0.get_INDEX_MODE();
	bc.last = w0.get_LAST();
	bc.pred_sel = w0.get_PRED_SEL();
	bc.src[0].chan = w0.get_SRC0_CHAN();
	bc.src[0].sel = w0.get_SRC0_SEL();
	bc.src[0].neg = w0.get_SRC0_NEG();
	bc.src[0].rel = w0.get_SRC0_REL();
	bc.src[1].chan = w0.get_SRC1_CHAN();
	bc.src[1].sel = w0.get_SRC1_SEL();
	bc.src[1].neg = w0.get_SRC1_NEG();
	bc.src[1].rel = w0.get_SRC1_REL();

	if ((dw1 >> 15) & 7) { // op3
		ALU_WORD1_OP3_ALL w1(dw1);
		bc.set_op(r600_isa_alu_by_opcode(ctx.isa, w1.get_ALU_INST(), 1));

		bc.bank_swizzle = w1.get_BANK_SWIZZLE();
		bc.clamp = w1.get_CLAMP();
		bc.dst_chan = w1.get_DST_CHAN();
		bc.dst_gpr = w1.get_DST_GPR();
		bc.dst_rel = w1.get_DST_REL();

		bc.src[2].chan = w1.get_SRC2_CHAN();
		bc.src[2].sel = w1.get_SRC2_SEL();
		bc.src[2].neg = w1.get_SRC2_NEG();
		bc.src[2].rel = w1.get_SRC2_REL();

	} else { // op2
		if (ctx.is_r600()) {
			ALU_WORD1_OP2_R6 w1(dw1);
			bc.set_op(r600_isa_alu_by_opcode(ctx.isa, w1.get_ALU_INST(), 0));

			bc.bank_swizzle = w1.get_BANK_SWIZZLE();
			bc.clamp = w1.get_CLAMP();
			bc.dst_chan = w1.get_DST_CHAN();
			bc.dst_gpr = w1.get_DST_GPR();
			bc.dst_rel = w1.get_DST_REL();

			bc.omod = w1.get_OMOD();
			bc.src[0].abs = w1.get_SRC0_ABS();
			bc.src[1].abs = w1.get_SRC1_ABS();
			bc.write_mask = w1.get_WRITE_MASK();
			bc.update_exec_mask = w1.get_UPDATE_EXEC_MASK();
			bc.update_pred = w1.get_UPDATE_PRED();

			bc.fog_merge = w1.get_FOG_MERGE();

		} else {
			ALU_WORD1_OP2_R7EGCM w1(dw1);
			bc.set_op(r600_isa_alu_by_opcode(ctx.isa, w1.get_ALU_INST(), 0));

			bc.bank_swizzle = w1.get_BANK_SWIZZLE();
			bc.clamp = w1.get_CLAMP();
			bc.dst_chan = w1.get_DST_CHAN();
			bc.dst_gpr = w1.get_DST_GPR();
			bc.dst_rel = w1.get_DST_REL();

			bc.omod = w1.get_OMOD();
			bc.src[0].abs = w1.get_SRC0_ABS();
			bc.src[1].abs = w1.get_SRC1_ABS();
			bc.write_mask = w1.get_WRITE_MASK();
			bc.update_exec_mask = w1.get_UPDATE_EXEC_MASK();
			bc.update_pred = w1.get_UPDATE_PRED();
		}
	}

	bc.slot_flags = (alu_op_flags)bc.op_ptr->slots[ctx.isa->hw_class];
	return r;
}

int bc_decoder::decode_fetch(unsigned & i, bc_fetch& bc) {
	int r = 0;
	uint32_t dw0 = dw[i];
	uint32_t dw1 = dw[i+1];
	uint32_t dw2 = dw[i+2];
	assert(i + 4 <= ndw);

	unsigned fetch_opcode = dw0 & 0x1F;

	bc.set_op(r600_isa_fetch_by_opcode(ctx.isa, fetch_opcode));

	if (bc.op_ptr->flags & FF_VTX)
		return decode_fetch_vtx(i, bc);

	// tex

	if (ctx.is_r600()) {
		TEX_WORD0_R6 w0(dw0);

		bc.bc_frac_mode = w0.get_BC_FRAC_MODE();
		bc.fetch_whole_quad = w0.get_FETCH_WHOLE_QUAD();
		bc.resource_id = w0.get_RESOURCE_ID();
		bc.src_gpr = w0.get_SRC_GPR();
		bc.src_rel = w0.get_SRC_REL();

	} else if (ctx.is_r600()) {
		TEX_WORD0_R7 w0(dw0);

		bc.bc_frac_mode = w0.get_BC_FRAC_MODE();
		bc.fetch_whole_quad = w0.get_FETCH_WHOLE_QUAD();
		bc.resource_id = w0.get_RESOURCE_ID();
		bc.src_gpr = w0.get_SRC_GPR();
		bc.src_rel = w0.get_SRC_REL();
		bc.alt_const = w0.get_ALT_CONST();

	} else { // eg/cm
		TEX_WORD0_EGCM w0(dw0);

		bc.fetch_whole_quad = w0.get_FETCH_WHOLE_QUAD();
		bc.resource_id = w0.get_RESOURCE_ID();
		bc.src_gpr = w0.get_SRC_GPR();
		bc.src_rel = w0.get_SRC_REL();
		bc.alt_const = w0.get_ALT_CONST();
		bc.inst_mod = w0.get_INST_MOD();
		bc.resource_index_mode = w0.get_RESOURCE_INDEX_MODE();
		bc.sampler_index_mode = w0.get_SAMPLER_INDEX_MODE();
	}

	TEX_WORD1_ALL w1(dw1);
	bc.coord_type[0] = w1.get_COORD_TYPE_X();
	bc.coord_type[1] = w1.get_COORD_TYPE_Y();
	bc.coord_type[2] = w1.get_COORD_TYPE_Z();
	bc.coord_type[3] = w1.get_COORD_TYPE_W();
	bc.dst_gpr = w1.get_DST_GPR();
	bc.dst_rel = w1.get_DST_REL();
	bc.dst_sel[0] = w1.get_DST_SEL_X();
	bc.dst_sel[1] = w1.get_DST_SEL_Y();
	bc.dst_sel[2] = w1.get_DST_SEL_Z();
	bc.dst_sel[3] = w1.get_DST_SEL_W();
	bc.lod_bias = w1.get_LOD_BIAS();

	TEX_WORD2_ALL w2(dw2);
	bc.offset[0] = w2.get_OFFSET_X();
	bc.offset[1] = w2.get_OFFSET_Y();
	bc.offset[2] = w2.get_OFFSET_Z();
	bc.sampler_id = w2.get_SAMPLER_ID();
	bc.src_sel[0] = w2.get_SRC_SEL_X();
	bc.src_sel[1] = w2.get_SRC_SEL_Y();
	bc.src_sel[2] = w2.get_SRC_SEL_Z();
	bc.src_sel[3] = w2.get_SRC_SEL_W();

	i += 4;
	return r;
}

int bc_decoder::decode_fetch_vtx(unsigned & i, bc_fetch& bc) {
	int r = 0;
	uint32_t dw0 = dw[i];
	uint32_t dw1 = dw[i+1];
	uint32_t dw2 = dw[i+2];
	i+= 4;
	assert(i <= ndw);

	if (ctx.is_cayman()) {
		VTX_WORD0_CM w0(dw0);
		bc.resource_id = w0.get_BUFFER_ID();
		bc.fetch_type = w0.get_FETCH_TYPE();
		bc.fetch_whole_quad = w0.get_FETCH_WHOLE_QUAD();
		bc.src_gpr = w0.get_SRC_GPR();
		bc.src_rel = w0.get_SRC_REL();
		bc.src_sel[0] = w0.get_SRC_SEL_X();
		bc.coalesced_read = w0.get_COALESCED_READ();
		bc.lds_req = w0.get_LDS_REQ();
		bc.structured_read = w0.get_STRUCTURED_READ();

	} else {
		VTX_WORD0_R6R7EG w0(dw0);
		bc.resource_id = w0.get_BUFFER_ID();
		bc.fetch_type = w0.get_FETCH_TYPE();
		bc.fetch_whole_quad = w0.get_FETCH_WHOLE_QUAD();
		bc.mega_fetch_count = w0.get_MEGA_FETCH_COUNT();
		bc.src_gpr = w0.get_SRC_GPR();
		bc.src_rel = w0.get_SRC_REL();
		bc.src_sel[0] = w0.get_SRC_SEL_X();
	}

	if (bc.op == FETCH_OP_SEMFETCH) {
		VTX_WORD1_SEM_ALL w1(dw1);
		bc.data_format = w1.get_DATA_FORMAT();
		bc.dst_sel[0] = w1.get_DST_SEL_X();
		bc.dst_sel[1] = w1.get_DST_SEL_Y();
		bc.dst_sel[2] = w1.get_DST_SEL_Z();
		bc.dst_sel[3] = w1.get_DST_SEL_W();
		bc.format_comp_all = w1.get_FORMAT_COMP_ALL();
		bc.num_format_all = w1.get_NUM_FORMAT_ALL();
		bc.srf_mode_all = w1.get_SRF_MODE_ALL();
		bc.use_const_fields = w1.get_USE_CONST_FIELDS();

		bc.semantic_id = w1.get_SEMANTIC_ID();

	} else {
		VTX_WORD1_GPR_ALL w1(dw1);
		bc.data_format = w1.get_DATA_FORMAT();
		bc.dst_sel[0] = w1.get_DST_SEL_X();
		bc.dst_sel[1] = w1.get_DST_SEL_Y();
		bc.dst_sel[2] = w1.get_DST_SEL_Z();
		bc.dst_sel[3] = w1.get_DST_SEL_W();
		bc.format_comp_all = w1.get_FORMAT_COMP_ALL();
		bc.num_format_all = w1.get_NUM_FORMAT_ALL();
		bc.srf_mode_all = w1.get_SRF_MODE_ALL();
		bc.use_const_fields = w1.get_USE_CONST_FIELDS();

		bc.dst_gpr = w1.get_DST_GPR();
		bc.dst_rel = w1.get_DST_REL();
	}

	switch (ctx.hw_class) {
	case HW_CLASS_R600:
	{
		VTX_WORD2_R6 w2(dw2);
		bc.const_buf_no_stride = w2.get_CONST_BUF_NO_STRIDE();
		bc.endian_swap = w2.get_ENDIAN_SWAP();
		bc.mega_fetch = w2.get_MEGA_FETCH();
		bc.offset[0] = w2.get_OFFSET();
		break;
	}
	case HW_CLASS_R700:
	{
		VTX_WORD2_R7 w2(dw2);
		bc.const_buf_no_stride = w2.get_CONST_BUF_NO_STRIDE();
		bc.endian_swap = w2.get_ENDIAN_SWAP();
		bc.mega_fetch = w2.get_MEGA_FETCH();
		bc.offset[0] = w2.get_OFFSET();
		bc.alt_const = w2.get_ALT_CONST();
		break;
	}
	case HW_CLASS_EVERGREEN:
	{
		VTX_WORD2_EG w2(dw2);
		bc.const_buf_no_stride = w2.get_CONST_BUF_NO_STRIDE();
		bc.endian_swap = w2.get_ENDIAN_SWAP();
		bc.mega_fetch = w2.get_MEGA_FETCH();
		bc.offset[0] = w2.get_OFFSET();
		bc.alt_const = w2.get_ALT_CONST();
		bc.resource_index_mode = w2.get_BUFFER_INDEX_MODE();
		break;
	}
	case HW_CLASS_CAYMAN:
	{
		VTX_WORD2_CM w2(dw2);
		bc.const_buf_no_stride = w2.get_CONST_BUF_NO_STRIDE();
		bc.endian_swap = w2.get_ENDIAN_SWAP();
		bc.offset[0] = w2.get_OFFSET();
		bc.alt_const = w2.get_ALT_CONST();
		bc.resource_index_mode = w2.get_BUFFER_INDEX_MODE();
		break;
	}
	default:
		assert(!"unknown hw class");
		return -1;
	}

	return r;
}

}
