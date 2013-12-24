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

#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

bool dump::visit(node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);

		switch (n.subtype) {
			case NST_PHI:
				dump_op(n, "* phi");
				break;
			case NST_PSI:
				dump_op(n, "* psi");
				break;
			case NST_COPY:
				dump_op(n, "* copy");
				break;
			default:
				assert(!"invalid node subtype");
				break;
		}
		sblog << "\n";
	}
	return false;
}

bool dump::visit(container_node& n, bool enter) {
	if (enter) {
		if (!n.empty()) {
			indent();
			dump_flags(n);
			sblog << "{  ";
			if (!n.dst.empty()) {
				sblog << " preloaded inputs [";
				dump_vec(n.dst);
				sblog << "]  ";
			}
			dump_live_values(n, true);
		}
		++level;
	} else {
		--level;
		if (!n.empty()) {
			indent();
			sblog << "}  ";
			if (!n.src.empty()) {
				sblog << " results [";
				dump_vec(n.src);
				sblog << "]  ";
			}
			dump_live_values(n, false);
		}
	}
	return true;
}

bool dump::visit(bb_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		sblog << "{ BB_" << n.id << "    loop_level = " << n.loop_level << "  ";
		dump_live_values(n, true);
		++level;
	} else {
		--level;
		indent();
		sblog << "} end BB_" << n.id << "  ";
		dump_live_values(n, false);
	}
	return true;
}

bool dump::visit(alu_group_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		sblog << "[  ";
		dump_live_values(n, true);

		++level;
	} else {
		--level;

		indent();
		sblog << "]  ";
		dump_live_values(n, false);
	}
	return true;
}

bool dump::visit(cf_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		dump_op(n, n.bc.op_ptr->name);

		if (n.bc.op_ptr->flags & CF_BRANCH) {
			sblog << " @" << (n.bc.addr << 1);
		}

		dump_common(n);
		sblog << "\n";

		if (!n.empty()) {
			indent();
			sblog << "<  ";
			dump_live_values(n, true);
		}

		++level;
	} else {
		--level;
		if (!n.empty()) {
			indent();
			sblog << ">  ";
			dump_live_values(n, false);
		}
	}
	return true;
}

bool dump::visit(alu_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		dump_alu(&n);
		dump_common(n);
		sblog << "\n";

		++level;
	} else {
		--level;

	}
	return true;
}

bool dump::visit(alu_packed_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		dump_op(n, n.op_ptr()->name);
		sblog << "  ";
		dump_live_values(n, true);

		++level;
	} else {
		--level;
		if (!n.live_after.empty()) {
			indent();
			dump_live_values(n, false);
		}

	}
	// proccess children only if their src/dst aren't moved to this node yet
	return n.src.empty();
}

bool dump::visit(fetch_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		dump_op(n, n.bc.op_ptr->name);
		sblog << "\n";

		++level;
	} else {
		--level;
	}
	return true;
}

bool dump::visit(region_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		sblog << "region #" << n.region_id << "   ";
		dump_common(n);

		if (!n.vars_defined.empty()) {
			sblog << "vars_defined: ";
			dump_set(sh, n.vars_defined);
		}

		dump_live_values(n, true);

		++level;

		if (n.loop_phi)
			run_on(*n.loop_phi);
	} else {
		--level;

		if (n.phi)
			run_on(*n.phi);

		indent();
		dump_live_values(n, false);
	}
	return true;
}

bool dump::visit(repeat_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		sblog << "repeat region #" << n.target->region_id;
		sblog << (n.empty() ? "   " : " after {  ");
		dump_common(n);
		sblog << "   ";
		dump_live_values(n, true);

		++level;
	} else {
		--level;

		if (!n.empty()) {
			indent();
			sblog << "} end_repeat   ";
			dump_live_values(n, false);
		}
	}
	return true;
}

bool dump::visit(depart_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		sblog << "depart region #" << n.target->region_id;
		sblog << (n.empty() ? "   " : " after {  ");
		dump_common(n);
		sblog << "  ";
		dump_live_values(n, true);

		++level;
	} else {
		--level;
		if (!n.empty()) {
			indent();
			sblog << "} end_depart   ";
			dump_live_values(n, false);
		}
	}
	return true;
}

bool dump::visit(if_node& n, bool enter) {
	if (enter) {
		indent();
		dump_flags(n);
		sblog << "if " << *n.cond << "    ";
		dump_common(n);
		sblog << "   ";
		dump_live_values(n, true);

		indent();
		sblog <<"{\n";

		++level;
	} else {
		--level;
		indent();
		sblog << "} endif   ";
		dump_live_values(n, false);
	}
	return true;
}

void dump::indent() {
	sblog.print_wl("", level * 4);
}

void dump::dump_vec(const vvec & vv) {
	bool first = true;
	for(vvec::const_iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;
		if (!first)
			sblog << ", ";
		else
			first = false;

		if (v) {
			sblog << *v;
		} else {
			sblog << "__";
		}
	}
}

void dump::dump_rels(vvec & vv) {
	for(vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;

		if (!v || !v->is_rel())
			continue;

		sblog << "\n\t\t\t\t\t";
		sblog << "    rels: " << *v << " : ";
		dump_vec(v->mdef);
		sblog << " <= ";
		dump_vec(v->muse);
	}
}

void dump::dump_op(node &n, const char *name) {

	if (n.pred) {
		alu_node &a = static_cast<alu_node&>(n);
		sblog << (a.bc.pred_sel-2) << " [" << *a.pred << "] ";
	}

	sblog << name;

	bool has_dst = !n.dst.empty();

	if (n.subtype == NST_CF_INST) {
		cf_node *c = static_cast<cf_node*>(&n);
		if (c->bc.op_ptr->flags & CF_EXP) {
			static const char *exp_type[] = {"PIXEL", "POS  ", "PARAM"};
			sblog << "  " << exp_type[c->bc.type] << " " << c->bc.array_base;
			has_dst = false;
		} else if (c->bc.op_ptr->flags & (CF_MEM)) {
			static const char *exp_type[] = {"WRITE", "WRITE_IND", "WRITE_ACK",
					"WRITE_IND_ACK"};
			sblog << "  " << exp_type[c->bc.type] << " " << c->bc.array_base
					<< "   ES:" << c->bc.elem_size;
			has_dst = false;
		}
	}

	sblog << "     ";

	if (has_dst) {
		dump_vec(n.dst);
		sblog << ",       ";
	}

	dump_vec(n.src);
}

void dump::dump_set(shader &sh, val_set& v) {
	sblog << "[";
	for(val_set::iterator I = v.begin(sh), E = v.end(sh); I != E; ++I) {
		value *val = *I;
		sblog << *val << " ";
	}
	sblog << "]";
}

void dump::dump_common(node& n) {
}

void dump::dump_flags(node &n) {
	if (n.flags & NF_DEAD)
		sblog << "### DEAD  ";
	if (n.flags & NF_REG_CONSTRAINT)
		sblog << "R_CONS  ";
	if (n.flags & NF_CHAN_CONSTRAINT)
		sblog << "CH_CONS  ";
	if (n.flags & NF_ALU_4SLOT)
		sblog << "4S  ";
}

void dump::dump_val(value* v) {
	sblog << *v;
}

void dump::dump_alu(alu_node *n) {

	if (n->is_copy_mov())
		sblog << "(copy) ";

	if (n->pred) {
		sblog << (n->bc.pred_sel-2) << " [" << *n->pred << "] ";
	}

	sblog << n->bc.op_ptr->name;

	if (n->bc.omod) {
		static const char *omod_str[] = {"", "*2", "*4", "/2"};
		sblog << omod_str[n->bc.omod];
	}

	if (n->bc.clamp) {
		sblog << "_sat";
	}

	bool has_dst = !n->dst.empty();

	sblog << "     ";

	if (has_dst) {
		dump_vec(n->dst);
		sblog << ",    ";
	}

	unsigned s = 0;
	for (vvec::iterator I = n->src.begin(), E = n->src.end(); I != E;
			++I, ++s) {

		bc_alu_src &src = n->bc.src[s];

		if (src.neg)
			sblog << "-";

		if (src.abs)
			sblog << "|";

		dump_val(*I);

		if (src.abs)
			sblog << "|";

		if (I + 1 != E)
			sblog << ", ";
	}

	dump_rels(n->dst);
	dump_rels(n->src);

}

void dump::dump_op(node* n) {
	if (n->type == NT_IF) {
		dump_op(*n, "IF ");
		return;
	}

	switch(n->subtype) {
	case NST_ALU_INST:
		dump_alu(static_cast<alu_node*>(n));
		break;
	case NST_FETCH_INST:
		dump_op(*n, static_cast<fetch_node*>(n)->bc.op_ptr->name);
		break;
	case NST_CF_INST:
	case NST_ALU_CLAUSE:
	case NST_TEX_CLAUSE:
	case NST_VTX_CLAUSE:
		dump_op(*n, static_cast<cf_node*>(n)->bc.op_ptr->name);
		break;
	case NST_ALU_PACKED_INST:
		dump_op(*n, static_cast<alu_packed_node*>(n)->op_ptr()->name);
		break;
	case NST_PHI:
		dump_op(*n, "PHI");
		break;
	case NST_PSI:
		dump_op(*n, "PSI");
		break;
	case NST_COPY:
		dump_op(*n, "COPY");
		break;
	default:
		dump_op(*n, "??unknown_op");
	}
}

void dump::dump_op_list(container_node* c) {
	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		dump_op(*I);
		sblog << "\n";
	}
}

void dump::dump_queue(sched_queue& q) {
	for (sched_queue::iterator I = q.begin(), E = q.end(); I != E; ++I) {
		dump_op(*I);
		sblog << "\n";
	}
}

void dump::dump_live_values(container_node &n, bool before) {
	if (before) {
		if (!n.live_before.empty()) {
			sblog << "live_before: ";
			dump_set(sh, n.live_before);
		}
	} else {
		if (!n.live_after.empty()) {
			sblog << "live_after: ";
			dump_set(sh, n.live_after);
		}
	}
	sblog << "\n";
}

} // namespace r600_sb
