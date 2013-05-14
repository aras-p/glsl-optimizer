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
#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

bc_builder::bc_builder(shader &s)
	: sh(s), ctx(s.get_ctx()), bb(ctx.hw_class_bit()), error(0) {}

int bc_builder::build() {

	container_node *root = sh.root;
	int cf_cnt = 0;

	// FIXME reserve total size to avoid reallocs

	for (node_iterator it = root->begin(), end = root->end();
			it != end; ++it) {

		cf_node *cf = static_cast<cf_node*>(*it);
		assert(cf->is_cf_inst() || cf->is_alu_clause() || cf->is_fetch_clause());

		cf_op_flags flags = (cf_op_flags)cf->bc.op_ptr->flags;

		cf->bc.id = cf_cnt++;

		if (flags & CF_ALU) {
			if (cf->bc.is_alu_extended())
				cf_cnt++;
		}
	}

	bb.set_size(cf_cnt << 1);
	bb.seek(cf_cnt << 1);

	unsigned cf_pos = 0;

	for (node_iterator I = root->begin(), end = root->end();
			I != end; ++I) {

		cf_node *cf = static_cast<cf_node*>(*I);
		cf_op_flags flags = (cf_op_flags)cf->bc.op_ptr->flags;

		if (flags & CF_ALU) {
			bb.seek(bb.ndw());
			cf->bc.addr = bb.ndw() >> 1;
			build_alu_clause(cf);
			cf->bc.count = (bb.ndw() >> 1) - cf->bc.addr - 1;
		} else if (flags & CF_FETCH) {
			bb.align(4);
			bb.seek(bb.ndw());
			cf->bc.addr = bb.ndw() >> 1;
			build_fetch_clause(cf);
			cf->bc.count = (((bb.ndw() >> 1) - cf->bc.addr) >> 1) - 1;
		} else if (cf->jump_target) {
			cf->bc.addr = cf->jump_target->bc.id;
			if (cf->jump_after_target)
				cf->bc.addr += 1;
		}

		bb.seek(cf_pos);
		build_cf(cf);
		cf_pos = bb.get_pos();
	}

	return 0;
}

int bc_builder::build_alu_clause(cf_node* n) {
	for (node_iterator I = n->begin(),	E = n->end();
			I != E; ++I) {

		alu_group_node *g = static_cast<alu_group_node*>(*I);
		assert(g->is_valid());

		build_alu_group(g);
	}
	return 0;
}

int bc_builder::build_alu_group(alu_group_node* n) {

	for (node_iterator I = n->begin(),	E = n->end();
			I != E; ++I) {

		alu_node *a = static_cast<alu_node*>(*I);
		assert(a->is_valid());
		build_alu(a);
	}

	for(int i = 0, ls = n->literals.size(); i < ls; ++i) {
		bb << n->literals.at(i).u;
	}

	bb.align(2);
	bb.seek(bb.ndw());

	return 0;
}

int bc_builder::build_fetch_clause(cf_node* n) {
	for (node_iterator I = n->begin(), E = n->end();
			I != E; ++I) {
		fetch_node *f = static_cast<fetch_node*>(*I);

		if (f->bc.op_ptr->flags & FF_VTX)
			build_fetch_vtx(f);
		else
			build_fetch_tex(f);
	}
	return 0;
}


int bc_builder::build_cf(cf_node* n) {
	const bc_cf &bc = n->bc;
	const cf_op_info *cfop = bc.op_ptr;

	if (cfop->flags & CF_ALU)
		return build_cf_alu(n);
	if (cfop->flags & (CF_EXP | CF_MEM))
		return build_cf_exp(n);

	if (ctx.is_egcm()) {
		bb << CF_WORD0_EGCM()
				.ADDR(bc.addr)
				.JUMPTABLE_SEL(bc.jumptable_sel);

		if (ctx.is_evergreen())

			bb << CF_WORD1_EG()
					.BARRIER(bc.barrier)
					.CF_CONST(bc.cf_const)
					.CF_INST(ctx.cf_opcode(bc.op))
					.COND(bc.cond)
					.COUNT(bc.count)
					.END_OF_PROGRAM(bc.end_of_program)
					.POP_COUNT(bc.pop_count)
					.VALID_PIXEL_MODE(bc.valid_pixel_mode)
					.WHOLE_QUAD_MODE(bc.whole_quad_mode);

		else //cayman

			bb << CF_WORD1_CM()
					.BARRIER(bc.barrier)
					.CF_CONST(bc.cf_const)
					.CF_INST(ctx.cf_opcode(bc.op))
					.COND(bc.cond)
					.COUNT(bc.count)
					.POP_COUNT(bc.pop_count)
					.VALID_PIXEL_MODE(bc.valid_pixel_mode);
	} else {
		bb << CF_WORD0_R6R7()
				.ADDR(bc.addr);

		assert(bc.count < ctx.max_fetch);

		bb << CF_WORD1_R6R7()
				.BARRIER(bc.barrier)
				.CALL_COUNT(bc.call_count)
				.CF_CONST(bc.cf_const)
				.CF_INST(ctx.cf_opcode(bc.op))
				.COND(bc.cond)
				.COUNT(bc.count & 7)
				.COUNT_3(bc.count >> 3)
				.END_OF_PROGRAM(bc.end_of_program)
				.POP_COUNT(bc.pop_count)
				.VALID_PIXEL_MODE(bc.valid_pixel_mode)
				.WHOLE_QUAD_MODE(bc.whole_quad_mode);
	}

	return 0;
}

int bc_builder::build_cf_alu(cf_node* n) {
	const bc_cf &bc = n->bc;

	assert(bc.count < 128);

	if (n->bc.is_alu_extended()) {
		assert(ctx.is_egcm());

		bb << CF_ALU_WORD0_EXT_EGCM()
				.KCACHE_BANK2(bc.kc[2].bank)
				.KCACHE_BANK3(bc.kc[3].bank)
				.KCACHE_BANK_INDEX_MODE0(bc.kc[0].index_mode)
				.KCACHE_BANK_INDEX_MODE1(bc.kc[1].index_mode)
				.KCACHE_BANK_INDEX_MODE2(bc.kc[2].index_mode)
				.KCACHE_BANK_INDEX_MODE3(bc.kc[3].index_mode)
				.KCACHE_MODE2(bc.kc[2].mode);

		bb << CF_ALU_WORD1_EXT_EGCM()
				.BARRIER(bc.barrier)
				.CF_INST(ctx.cf_opcode(CF_OP_ALU_EXT))
				.KCACHE_ADDR2(bc.kc[2].addr)
				.KCACHE_ADDR3(bc.kc[3].addr)
				.KCACHE_MODE3(bc.kc[3].mode);
	}

	bb << CF_ALU_WORD0_ALL()
			.ADDR(bc.addr)
			.KCACHE_BANK0(bc.kc[0].bank)
			.KCACHE_BANK1(bc.kc[1].bank)
			.KCACHE_MODE0(bc.kc[0].mode);

	assert(bc.count < 128);

	if (ctx.is_r600())
		bb << CF_ALU_WORD1_R6()
				.BARRIER(bc.barrier)
				.CF_INST(ctx.cf_opcode(bc.op))
				.COUNT(bc.count)
				.KCACHE_ADDR0(bc.kc[0].addr)
				.KCACHE_ADDR1(bc.kc[1].addr)
				.KCACHE_MODE1(bc.kc[1].mode)
				.USES_WATERFALL(bc.uses_waterfall)
				.WHOLE_QUAD_MODE(bc.whole_quad_mode);
	else
		bb << CF_ALU_WORD1_R7EGCM()
				.ALT_CONST(bc.alt_const)
				.BARRIER(bc.barrier)
				.CF_INST(ctx.cf_opcode(bc.op))
				.COUNT(bc.count)
				.KCACHE_ADDR0(bc.kc[0].addr)
				.KCACHE_ADDR1(bc.kc[1].addr)
				.KCACHE_MODE1(bc.kc[1].mode)
				.WHOLE_QUAD_MODE(bc.whole_quad_mode);

	return 0;
}

int bc_builder::build_cf_exp(cf_node* n) {
	const bc_cf &bc = n->bc;
	const cf_op_info *cfop = bc.op_ptr;

	if (cfop->flags & CF_RAT) {
		assert(ctx.is_egcm());

		bb << CF_ALLOC_EXPORT_WORD0_RAT_EGCM()
				.ELEM_SIZE(bc.elem_size)
				.INDEX_GPR(bc.index_gpr)
				.RAT_ID(bc.rat_id)
				.RAT_INDEX_MODE(bc.rat_index_mode)
				.RAT_INST(bc.rat_inst)
				.RW_GPR(bc.rw_gpr)
				.RW_REL(bc.rw_rel)
				.TYPE(bc.type);
	} else {

		bb << CF_ALLOC_EXPORT_WORD0_ALL()
					.ARRAY_BASE(bc.array_base)
					.ELEM_SIZE(bc.elem_size)
					.INDEX_GPR(bc.index_gpr)
					.RW_GPR(bc.rw_gpr)
					.RW_REL(bc.rw_rel)
					.TYPE(bc.type);
	}

	if (cfop->flags & CF_EXP) {

		if (!ctx.is_egcm())
			bb << CF_ALLOC_EXPORT_WORD1_SWIZ_R6R7()
					.BARRIER(bc.barrier)
					.BURST_COUNT(bc.burst_count)
					.CF_INST(ctx.cf_opcode(bc.op))
					.END_OF_PROGRAM(bc.end_of_program)
					.SEL_X(bc.sel[0])
					.SEL_Y(bc.sel[1])
					.SEL_Z(bc.sel[2])
					.SEL_W(bc.sel[3])
					.VALID_PIXEL_MODE(bc.valid_pixel_mode)
					.WHOLE_QUAD_MODE(bc.whole_quad_mode);

		else if (ctx.is_evergreen())
			bb << CF_ALLOC_EXPORT_WORD1_SWIZ_EG()
					.BARRIER(bc.barrier)
					.BURST_COUNT(bc.burst_count)
					.CF_INST(ctx.cf_opcode(bc.op))
					.END_OF_PROGRAM(bc.end_of_program)
					.MARK(bc.mark)
					.SEL_X(bc.sel[0])
					.SEL_Y(bc.sel[1])
					.SEL_Z(bc.sel[2])
					.SEL_W(bc.sel[3])
					.VALID_PIXEL_MODE(bc.valid_pixel_mode);

		else // cayman
			bb << CF_ALLOC_EXPORT_WORD1_SWIZ_CM()
					.BARRIER(bc.barrier)
					.BURST_COUNT(bc.burst_count)
					.CF_INST(ctx.cf_opcode(bc.op))
					.MARK(bc.mark)
					.SEL_X(bc.sel[0])
					.SEL_Y(bc.sel[1])
					.SEL_Z(bc.sel[2])
					.SEL_W(bc.sel[3])
					.VALID_PIXEL_MODE(bc.valid_pixel_mode);

	} else if (cfop->flags & CF_MEM) {
		return build_cf_mem(n);
	}

	return 0;
}

int bc_builder::build_cf_mem(cf_node* n) {
	const bc_cf &bc = n->bc;

	if (!ctx.is_egcm())
		bb << CF_ALLOC_EXPORT_WORD1_BUF_R6R7()
				.ARRAY_SIZE(bc.array_size)
				.BARRIER(bc.barrier)
				.BURST_COUNT(bc.burst_count)
				.CF_INST(ctx.cf_opcode(bc.op))
				.COMP_MASK(bc.comp_mask)
				.END_OF_PROGRAM(bc.end_of_program)
				.VALID_PIXEL_MODE(bc.valid_pixel_mode)
				.WHOLE_QUAD_MODE(bc.whole_quad_mode);

	else if (ctx.is_evergreen())
		bb << CF_ALLOC_EXPORT_WORD1_BUF_EG()
				.ARRAY_SIZE(bc.array_size)
				.BARRIER(bc.barrier)
				.BURST_COUNT(bc.burst_count)
				.CF_INST(ctx.cf_opcode(bc.op))
				.COMP_MASK(bc.comp_mask)
				.END_OF_PROGRAM(bc.end_of_program)
				.MARK(bc.mark)
				.VALID_PIXEL_MODE(bc.valid_pixel_mode);

	else // cayman
		bb << CF_ALLOC_EXPORT_WORD1_BUF_CM()
		.ARRAY_SIZE(bc.array_size)
		.BARRIER(bc.barrier)
		.BURST_COUNT(bc.burst_count)
		.CF_INST(ctx.cf_opcode(bc.op))
		.COMP_MASK(bc.comp_mask)
		.MARK(bc.mark)
		.VALID_PIXEL_MODE(bc.valid_pixel_mode);

	return 0;
}

int bc_builder::build_alu(alu_node* n) {
	const bc_alu &bc = n->bc;
	const alu_op_info *aop = bc.op_ptr;

	bb << ALU_WORD0_ALL()
			.INDEX_MODE(bc.index_mode)
			.LAST(bc.last)
			.PRED_SEL(bc.pred_sel)
			.SRC0_SEL(bc.src[0].sel)
			.SRC0_CHAN(bc.src[0].chan)
			.SRC0_NEG(bc.src[0].neg)
			.SRC0_REL(bc.src[0].rel)
			.SRC1_SEL(bc.src[1].sel)
			.SRC1_CHAN(bc.src[1].chan)
			.SRC1_NEG(bc.src[1].neg)
			.SRC1_REL(bc.src[1].rel);

	if (aop->src_count<3) {
		if (ctx.is_r600())
			bb << ALU_WORD1_OP2_R6()
					.ALU_INST(ctx.alu_opcode(bc.op))
					.BANK_SWIZZLE(bc.bank_swizzle)
					.CLAMP(bc.clamp)
					.DST_GPR(bc.dst_gpr)
					.DST_CHAN(bc.dst_chan)
					.DST_REL(bc.dst_rel)
					.FOG_MERGE(bc.fog_merge)
					.OMOD(bc.omod)
					.SRC0_ABS(bc.src[0].abs)
					.SRC1_ABS(bc.src[1].abs)
					.UPDATE_EXEC_MASK(bc.update_exec_mask)
					.UPDATE_PRED(bc.update_pred)
					.WRITE_MASK(bc.write_mask);
		else {

			if (ctx.is_cayman() && (aop->flags & AF_MOVA)) {

				bb << ALU_WORD1_OP2_MOVA_CM()
						.ALU_INST(ctx.alu_opcode(bc.op))
						.BANK_SWIZZLE(bc.bank_swizzle)
						.CLAMP(bc.clamp)
						.MOVA_DST(bc.dst_gpr)
						.DST_CHAN(bc.dst_chan)
						.DST_REL(bc.dst_rel)
						.OMOD(bc.omod)
						.UPDATE_EXEC_MASK(bc.update_exec_mask)
						.UPDATE_PRED(bc.update_pred)
						.WRITE_MASK(bc.write_mask)
						.SRC0_ABS(bc.src[0].abs)
						.SRC1_ABS(bc.src[1].abs);

			} else if (ctx.is_cayman() && (aop->flags & (AF_PRED|AF_KILL))) {
				bb << ALU_WORD1_OP2_EXEC_MASK_CM()
						.ALU_INST(ctx.alu_opcode(bc.op))
						.BANK_SWIZZLE(bc.bank_swizzle)
						.CLAMP(bc.clamp)
						.DST_CHAN(bc.dst_chan)
						.DST_REL(bc.dst_rel)
						.EXECUTE_MASK_OP(bc.omod)
						.UPDATE_EXEC_MASK(bc.update_exec_mask)
						.UPDATE_PRED(bc.update_pred)
						.WRITE_MASK(bc.write_mask)
						.SRC0_ABS(bc.src[0].abs)
						.SRC1_ABS(bc.src[1].abs);

			} else
				bb << ALU_WORD1_OP2_R7EGCM()
						.ALU_INST(ctx.alu_opcode(bc.op))
						.BANK_SWIZZLE(bc.bank_swizzle)
						.CLAMP(bc.clamp)
						.DST_GPR(bc.dst_gpr)
						.DST_CHAN(bc.dst_chan)
						.DST_REL(bc.dst_rel)
						.OMOD(bc.omod)
						.UPDATE_EXEC_MASK(bc.update_exec_mask)
						.UPDATE_PRED(bc.update_pred)
						.WRITE_MASK(bc.write_mask)
						.SRC0_ABS(bc.src[0].abs)
						.SRC1_ABS(bc.src[1].abs);

		}
	} else
		bb << ALU_WORD1_OP3_ALL()
				.ALU_INST(ctx.alu_opcode(bc.op))
				.BANK_SWIZZLE(bc.bank_swizzle)
				.CLAMP(bc.clamp)
				.DST_GPR(bc.dst_gpr)
				.DST_CHAN(bc.dst_chan)
				.DST_REL(bc.dst_rel)
				.SRC2_SEL(bc.src[2].sel)
				.SRC2_CHAN(bc.src[2].chan)
				.SRC2_NEG(bc.src[2].neg)
				.SRC2_REL(bc.src[2].rel);
	return 0;
}

int bc_builder::build_fetch_tex(fetch_node* n) {
	const bc_fetch &bc = n->bc;
	const fetch_op_info *fop = bc.op_ptr;

	assert(!(fop->flags & FF_VTX));

	if (ctx.is_r600())
		bb << TEX_WORD0_R6()
				.BC_FRAC_MODE(bc.bc_frac_mode)
				.FETCH_WHOLE_QUAD(bc.fetch_whole_quad)
				.RESOURCE_ID(bc.resource_id)
				.SRC_GPR(bc.src_gpr)
				.SRC_REL(bc.src_rel)
				.TEX_INST(ctx.fetch_opcode(bc.op));

	else if (ctx.is_r700())
		bb << TEX_WORD0_R7()
				.ALT_CONST(bc.alt_const)
				.BC_FRAC_MODE(bc.bc_frac_mode)
				.FETCH_WHOLE_QUAD(bc.fetch_whole_quad)
				.RESOURCE_ID(bc.resource_id)
				.SRC_GPR(bc.src_gpr)
				.SRC_REL(bc.src_rel)
				.TEX_INST(ctx.fetch_opcode(bc.op));

	else
		bb << TEX_WORD0_EGCM()
				.ALT_CONST(bc.alt_const)
				.FETCH_WHOLE_QUAD(bc.fetch_whole_quad)
				.INST_MOD(bc.inst_mod)
				.RESOURCE_ID(bc.resource_id)
				.RESOURCE_INDEX_MODE(bc.resource_index_mode)
				.SAMPLER_INDEX_MODE(bc.sampler_index_mode)
				.SRC_GPR(bc.src_gpr)
				.SRC_REL(bc.src_rel)
				.TEX_INST(ctx.fetch_opcode(bc.op));

	bb << TEX_WORD1_ALL()
			.COORD_TYPE_X(bc.coord_type[0])
			.COORD_TYPE_Y(bc.coord_type[1])
			.COORD_TYPE_Z(bc.coord_type[2])
			.COORD_TYPE_W(bc.coord_type[3])
			.DST_GPR(bc.dst_gpr)
			.DST_REL(bc.dst_rel)
			.DST_SEL_X(bc.dst_sel[0])
			.DST_SEL_Y(bc.dst_sel[1])
			.DST_SEL_Z(bc.dst_sel[2])
			.DST_SEL_W(bc.dst_sel[3])
			.LOD_BIAS(bc.lod_bias);

	bb << TEX_WORD2_ALL()
			.OFFSET_X(bc.offset[0])
			.OFFSET_Y(bc.offset[1])
			.OFFSET_Z(bc.offset[2])
			.SAMPLER_ID(bc.sampler_id)
			.SRC_SEL_X(bc.src_sel[0])
			.SRC_SEL_Y(bc.src_sel[1])
			.SRC_SEL_Z(bc.src_sel[2])
			.SRC_SEL_W(bc.src_sel[3]);

	bb << 0;
	return 0;
}

int bc_builder::build_fetch_vtx(fetch_node* n) {
	const bc_fetch &bc = n->bc;
	const fetch_op_info *fop = bc.op_ptr;

	assert(fop->flags & FF_VTX);

	if (!ctx.is_cayman())
		bb << VTX_WORD0_R6R7EG()
				.BUFFER_ID(bc.resource_id)
				.FETCH_TYPE(bc.fetch_type)
				.FETCH_WHOLE_QUAD(bc.fetch_whole_quad)
				.MEGA_FETCH_COUNT(bc.mega_fetch_count)
				.SRC_GPR(bc.src_gpr)
				.SRC_REL(bc.src_rel)
				.SRC_SEL_X(bc.src_sel[0])
				.VC_INST(ctx.fetch_opcode(bc.op));

	else
		bb << VTX_WORD0_CM()
				.BUFFER_ID(bc.resource_id)
				.COALESCED_READ(bc.coalesced_read)
				.FETCH_TYPE(bc.fetch_type)
				.FETCH_WHOLE_QUAD(bc.fetch_whole_quad)
				.LDS_REQ(bc.lds_req)
				.SRC_GPR(bc.src_gpr)
				.SRC_REL(bc.src_rel)
				.SRC_SEL_X(bc.src_sel[0])
				.SRC_SEL_Y(bc.src_sel[1])
				.STRUCTURED_READ(bc.structured_read)
				.VC_INST(ctx.fetch_opcode(bc.op));

	if (bc.op == FETCH_OP_SEMFETCH)
		bb << VTX_WORD1_SEM_ALL()
				.DATA_FORMAT(bc.data_format)
				.DST_SEL_X(bc.dst_sel[0])
				.DST_SEL_Y(bc.dst_sel[1])
				.DST_SEL_Z(bc.dst_sel[2])
				.DST_SEL_W(bc.dst_sel[3])
				.FORMAT_COMP_ALL(bc.format_comp_all)
				.NUM_FORMAT_ALL(bc.num_format_all)
				.SEMANTIC_ID(bc.semantic_id)
				.SRF_MODE_ALL(bc.srf_mode_all)
				.USE_CONST_FIELDS(bc.use_const_fields);
	else
		bb << VTX_WORD1_GPR_ALL()
				.DATA_FORMAT(bc.data_format)
				.DST_GPR(bc.dst_gpr)
				.DST_REL(bc.dst_rel)
				.DST_SEL_X(bc.dst_sel[0])
				.DST_SEL_Y(bc.dst_sel[1])
				.DST_SEL_Z(bc.dst_sel[2])
				.DST_SEL_W(bc.dst_sel[3])
				.FORMAT_COMP_ALL(bc.format_comp_all)
				.NUM_FORMAT_ALL(bc.num_format_all)
				.SRF_MODE_ALL(bc.srf_mode_all)
				.USE_CONST_FIELDS(bc.use_const_fields);

	switch (ctx.hw_class) {
	case HW_CLASS_R600:
		bb << VTX_WORD2_R6()
				.CONST_BUF_NO_STRIDE(bc.const_buf_no_stride)
				.ENDIAN_SWAP(bc.endian_swap)
				.MEGA_FETCH(bc.mega_fetch)
				.OFFSET(bc.offset[0]);
		break;
	case HW_CLASS_R700:
		bb << VTX_WORD2_R7()
				.ALT_CONST(bc.alt_const)
				.CONST_BUF_NO_STRIDE(bc.const_buf_no_stride)
				.ENDIAN_SWAP(bc.endian_swap)
				.MEGA_FETCH(bc.mega_fetch)
				.OFFSET(bc.offset[0]);
		break;
	case HW_CLASS_EVERGREEN:
		bb << VTX_WORD2_EG()
				.ALT_CONST(bc.alt_const)
				.BUFFER_INDEX_MODE(bc.resource_index_mode)
				.CONST_BUF_NO_STRIDE(bc.const_buf_no_stride)
				.ENDIAN_SWAP(bc.endian_swap)
				.MEGA_FETCH(bc.mega_fetch)
				.OFFSET(bc.offset[0]);
		break;
	case HW_CLASS_CAYMAN:
		bb << VTX_WORD2_CM()
				.ALT_CONST(bc.alt_const)
				.BUFFER_INDEX_MODE(bc.resource_index_mode)
				.CONST_BUF_NO_STRIDE(bc.const_buf_no_stride)
				.ENDIAN_SWAP(bc.endian_swap)
				.OFFSET(bc.offset[0]);
		break;
	default:
		assert(!"unknown hw class");
		return -1;
	}

	bb << 0;
	return 0;
}

}
