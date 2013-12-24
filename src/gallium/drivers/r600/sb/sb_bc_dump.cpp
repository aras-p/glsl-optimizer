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

static const char* chans = "xyzw01?_";

static const char* vec_bs[] = {
		"VEC_012", "VEC_021", "VEC_120", "VEC_102", "VEC_201", "VEC_210"
};

static const char* scl_bs[] = {
		"SCL_210", "SCL_122", "SCL_212", "SCL_221"
};


bool bc_dump::visit(cf_node& n, bool enter) {
	if (enter) {

		id = n.bc.id << 1;

		if ((n.bc.op_ptr->flags & CF_ALU) && n.bc.is_alu_extended()) {
			dump_dw(id, 2);
			id += 2;
			sblog << "\n";
		}

		dump_dw(id, 2);
		dump(n);

		if (n.bc.op_ptr->flags & CF_CLAUSE) {
			id = n.bc.addr << 1;
			new_group = 1;
		}
	}
	return true;
}

bool bc_dump::visit(alu_node& n, bool enter) {
	if (enter) {
		sblog << " ";
		dump_dw(id, 2);

		if (new_group) {
			sblog.print_w(++group_index, 5);
			sblog << " ";
		} else
			sblog << "      ";

		dump(n);
		id += 2;

		new_group = n.bc.last;
	} else {
		if (n.bc.last) {
			alu_group_node *g =
					static_cast<alu_group_node*>(n.get_alu_group_node());
			assert(g);
			for (unsigned k = 0; k < g->literals.size(); ++k) {
				sblog << " ";
				dump_dw(id, 1);
				id += 1;
				sblog << "\n";
			}

			id = (id + 1) & ~1u;
		}
	}

	return false;
}

bool bc_dump::visit(fetch_node& n, bool enter) {
	if (enter) {
		sblog << " ";
		dump_dw(id, 3);
		dump(n);
		id += 4;
	}
	return false;
}

static void fill_to(sb_ostringstream &s, int pos) {
	int l = s.str().length();
	if (l < pos)
		s << std::string(pos-l, ' ');
}

void bc_dump::dump(cf_node& n) {
	sb_ostringstream s;
	s << n.bc.op_ptr->name;

	if (n.bc.op_ptr->flags & CF_EXP) {
		static const char *exp_type[] = {"PIXEL", "POS  ", "PARAM"};

		fill_to(s, 18);
		s << " " << exp_type[n.bc.type] << " ";

		if (n.bc.burst_count) {
			sb_ostringstream s2;
			s2 << n.bc.array_base << "-" << n.bc.array_base + n.bc.burst_count;
			s.print_wl(s2.str(), 5);
			s << " R" << n.bc.rw_gpr << "-" <<
					n.bc.rw_gpr + n.bc.burst_count << ".";
		} else {
			s.print_wl(n.bc.array_base, 5);
			s << " R" << n.bc.rw_gpr << ".";
		}

		for (int k = 0; k < 4; ++k)
			s << chans[n.bc.sel[k]];

	} else if (n.bc.op_ptr->flags & CF_MEM) {
		static const char *exp_type[] = {"WRITE", "WRITE_IND", "WRITE_ACK",
				"WRITE_IND_ACK"};
		fill_to(s, 18);
		s << " " << exp_type[n.bc.type] << " ";
		s.print_wl(n.bc.array_base, 5);
		s << " R" << n.bc.rw_gpr << ".";
		for (int k = 0; k < 4; ++k)
			s << ((n.bc.comp_mask & (1 << k)) ? chans[k] : '_');

		if ((n.bc.op_ptr->flags & CF_RAT) && (n.bc.type & 1)) {
			s << ", @R" << n.bc.index_gpr << ".xyz";
		}
		if ((n.bc.op_ptr->flags & CF_MEM) && (n.bc.type & 1)) {
			s << ", @R" << n.bc.index_gpr << ".x";
		}

		s << "  ES:" << n.bc.elem_size;

	} else {

		if (n.bc.op_ptr->flags & CF_CLAUSE) {
			s << " " << n.bc.count+1;
		}

		s << " @" << (n.bc.addr << 1);

		if (n.bc.op_ptr->flags & CF_ALU) {

			for (int k = 0; k < 4; ++k) {
				bc_kcache &kc = n.bc.kc[k];
				if (kc.mode) {
					s << " KC" << k << "[CB" << kc.bank << ":" <<
							(kc.addr << 4) << "-" <<
							(((kc.addr + kc.mode) << 4) - 1) << "]";
				}
			}
		}

		if (n.bc.cond)
			s << " CND:" << n.bc.cond;

		if (n.bc.pop_count)
			s << " POP:" << n.bc.pop_count;
	}

	if (!n.bc.barrier)
		s << "  NO_BARRIER";

	if (n.bc.valid_pixel_mode)
		s << "  VPM";

	if (n.bc.whole_quad_mode)
		s << "  WQM";

	if (n.bc.end_of_program)
		s << "  EOP";

	sblog << s.str() << "\n";
}


static void print_sel(sb_ostream &s, int sel, int rel, int index_mode,
                      int need_brackets) {
	if (rel && index_mode >= 5 && sel < 128)
		s << "G";
	if (rel || need_brackets) {
		s << "[";
	}
	s << sel;
	if (rel) {
		if (index_mode == 0 || index_mode == 6)
			s << "+AR";
		else if (index_mode == 4)
			s << "+AL";
	}
	if (rel || need_brackets) {
		s << "]";
	}
}

static void print_dst(sb_ostream &s, bc_alu &alu)
{
	unsigned sel = alu.dst_gpr;
	char reg_char = 'R';
	if (sel >= 128 - 4) { // clause temporary gpr
		sel -= 128 - 4;
		reg_char = 'T';
	}

	if (alu.write_mask || alu.op_ptr->src_count == 3) {
		s << reg_char;
		print_sel(s, sel, alu.dst_rel, alu.index_mode, 0);
	} else {
		s << "__";
	}
	s << ".";
	s << chans[alu.dst_chan];
}

static void print_src(sb_ostream &s, bc_alu &alu, unsigned idx)
{
	bc_alu_src *src = &alu.src[idx];
	unsigned sel = src->sel, need_sel = 1, need_chan = 1, need_brackets = 0;

	if (src->neg)
		s <<"-";
	if (src->abs)
		s <<"|";

	if (sel < 128 - 4) {
		s << "R";
	} else if (sel < 128) {
		s << "T";
		sel -= 128 - 4;
	} else if (sel < 160) {
		s << "KC0";
		need_brackets = 1;
		sel -= 128;
	} else if (sel < 192) {
		s << "KC1";
		need_brackets = 1;
		sel -= 160;
	} else if (sel >= 448) {
		s << "Param";
		sel -= 448;
	} else if (sel >= 288) {
		s << "KC3";
		need_brackets = 1;
		sel -= 288;
	} else if (sel >= 256) {
		s << "KC2";
		need_brackets = 1;
		sel -= 256;
	} else {
		need_sel = 0;
		need_chan = 0;
		switch (sel) {
		case ALU_SRC_PS:
			s << "PS";
			break;
		case ALU_SRC_PV:
			s << "PV";
			need_chan = 1;
			break;
		case ALU_SRC_LITERAL:
			s << "[0x";
			s.print_zw_hex(src->value.u, 8);
			s << " " << src->value.f << "]";
			need_chan = 1;
			break;
		case ALU_SRC_0_5:
			s << "0.5";
			break;
		case ALU_SRC_M_1_INT:
			s << "-1";
			break;
		case ALU_SRC_1_INT:
			s << "1";
			break;
		case ALU_SRC_1:
			s << "1.0";
			break;
		case ALU_SRC_0:
			s << "0";
			break;
		default:
			s << "??IMM_" <<  sel;
			break;
		}
	}

	if (need_sel)
		print_sel(s, sel, src->rel, alu.index_mode, need_brackets);

	if (need_chan) {
		s << "." << chans[src->chan];
	}

	if (src->abs)
		s << "|";
}
void bc_dump::dump(alu_node& n) {
	sb_ostringstream s;
	static const char *omod_str[] = {"","*2","*4","/2"};
	static const char *slots = "xyzwt";

	s << (n.bc.update_exec_mask ? "M" : " ");
	s << (n.bc.update_pred ? "P" : " ");
	s << " ";
	s << (n.bc.pred_sel>=2 ? (n.bc.pred_sel == 2 ? "0" : "1") : " ");
	s << " ";

	s << slots[n.bc.slot] << ": ";

	s << n.bc.op_ptr->name << omod_str[n.bc.omod] << (n.bc.clamp ? "_sat" : "");
	fill_to(s, 26);
	s << " ";

	print_dst(s, n.bc);
	for (int k = 0; k < n.bc.op_ptr->src_count; ++k) {
		s << (k ? ", " : ",  ");
		print_src(s, n.bc, k);
	}

	if (n.bc.bank_swizzle) {
		fill_to(s, 55);
		if (n.bc.slot == SLOT_TRANS)
			s << "  " << scl_bs[n.bc.bank_swizzle];
		else
			s << "  " << vec_bs[n.bc.bank_swizzle];
	}

	sblog << s.str() << "\n";
}

int bc_dump::init() {
	sb_ostringstream s;
	s << "===== SHADER #" << sh.id;

	if (sh.optimized)
		s << " OPT";

	s << " ";

	std::string target = std::string(" ") +
			sh.get_full_target_name() + " =====";

	while (s.str().length() + target.length() < 80)
		s << "=";

	s << target;

	sblog << "\n" << s.str() << "\n";

	s.clear();

	if (bc_data) {
		s << "===== " << ndw << " dw ===== " << sh.ngpr
				<< " gprs ===== " << sh.nstack << " stack ";
	}

	while (s.str().length() < 80)
		s << "=";

	sblog << s.str() << "\n";

	return 0;
}

int bc_dump::done() {
	sb_ostringstream s;
	s << "===== SHADER_END ";

	while (s.str().length() < 80)
		s << "=";

	sblog << s.str() << "\n\n";

	return 0;
}

bc_dump::bc_dump(shader& s, bytecode* bc)  :
	vpass(s), bc_data(), ndw(), id(),
	new_group(), group_index() {

	if (bc) {
		bc_data = bc->data();
		ndw = bc->ndw();
	}
}

void bc_dump::dump(fetch_node& n) {
	sb_ostringstream s;
	static const char * fetch_type[] = {"VERTEX", "INSTANCE", ""};

	s << n.bc.op_ptr->name;
	fill_to(s, 20);

	s << "R";
	print_sel(s, n.bc.dst_gpr, n.bc.dst_rel, INDEX_LOOP, 0);
	s << ".";
	for (int k = 0; k < 4; ++k)
		s << chans[n.bc.dst_sel[k]];
	s << ", ";

	s << "R";
	print_sel(s, n.bc.src_gpr, n.bc.src_rel, INDEX_LOOP, 0);
	s << ".";

	unsigned vtx = n.bc.op_ptr->flags & FF_VTX;
	unsigned num_src_comp = vtx ? ctx.is_cayman() ? 2 : 1 : 4;

	for (unsigned k = 0; k < num_src_comp; ++k)
		s << chans[n.bc.src_sel[k]];

	if (vtx && n.bc.offset[0]) {
		s << " + " << n.bc.offset[0] << "b ";
	}

	s << ",   RID:" << n.bc.resource_id;

	if (vtx) {
		s << "  " << fetch_type[n.bc.fetch_type];
		if (!ctx.is_cayman() && n.bc.mega_fetch_count)
			s << " MFC:" << n.bc.mega_fetch_count;
		if (n.bc.fetch_whole_quad)
			s << " FWQ";
		s << " UCF:" << n.bc.use_const_fields
				<< " FMT(DTA:" << n.bc.data_format
				<< " NUM:" << n.bc.num_format_all
				<< " COMP:" << n.bc.format_comp_all
				<< " MODE:" << n.bc.srf_mode_all << ")";
	} else {
		s << ", SID:" << n.bc.sampler_id;
		if (n.bc.lod_bias)
			s << " LB:" << n.bc.lod_bias;
		s << " CT:";
		for (unsigned k = 0; k < 4; ++k)
			s << (n.bc.coord_type[k] ? "N" : "U");
		for (unsigned k = 0; k < 3; ++k)
			if (n.bc.offset[k])
				s << " O" << chans[k] << ":" << n.bc.offset[k];
	}

	sblog << s.str() << "\n";
}

void bc_dump::dump_dw(unsigned dw_id, unsigned count) {
	if (!bc_data)
		return;

	assert(dw_id + count <= ndw);

	sblog.print_zw(dw_id, 4);
	sblog << "  ";
	while (count--) {
		sblog.print_zw_hex(bc_data[dw_id++], 8);
		sblog << " ";
	}
}

} // namespace r600_sb
