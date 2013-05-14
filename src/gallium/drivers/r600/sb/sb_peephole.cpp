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

#define PPH_DEBUG 0

#if PPH_DEBUG
#define PPH_DUMP(q) do { q } while (0)
#else
#define PPH_DUMP(q)
#endif

#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

int peephole::run() {

	run_on(sh.root);

	return 0;
}

void peephole::run_on(container_node* c) {

	for (node_riterator I = c->rbegin(), E = c->rend(); I != E; ++I) {
		node *n = *I;

		if (n->is_container())
			run_on(static_cast<container_node*>(n));
		else {

			if (n->is_alu_inst()) {
				alu_node *a = static_cast<alu_node*>(n);

				if (a->bc.op_ptr->flags & AF_CC_MASK) {
					optimize_cc_op(a);
				} else if (a->bc.op == ALU_OP1_FLT_TO_INT) {

					alu_node *s = a;
					if (get_bool_flt_to_int_source(s)) {
						convert_float_setcc(a, s);
					}
				}
			}
		}
	}
}

void peephole::optimize_cc_op(alu_node* a) {
	unsigned aflags = a->bc.op_ptr->flags;

	if (aflags & (AF_PRED | AF_SET)) {
		optimize_SETcc_op(a);
	} else if (aflags & AF_CMOV) {
		optimize_CNDcc_op(a);
	}
}

void peephole::convert_float_setcc(alu_node *f2i, alu_node *s) {
	alu_node *ns = sh.clone(s);

	ns->dst[0] = f2i->dst[0];
	ns->dst[0]->def = ns;
	ns->bc.set_op(ns->bc.op + (ALU_OP2_SETE_DX10 - ALU_OP2_SETE));
	f2i->insert_after(ns);
	f2i->remove();
}

void peephole::optimize_SETcc_op(alu_node* a) {

	unsigned flags = a->bc.op_ptr->flags;
	unsigned cc = flags & AF_CC_MASK;
	unsigned cmp_type = flags & AF_CMP_TYPE_MASK;
	unsigned dst_type = flags & AF_DST_TYPE_MASK;
	bool is_pred = flags & AF_PRED;

	// TODO handle other cases

	if (a->src[1]->is_const() && (cc == AF_CC_E || cc == AF_CC_NE) &&
			a->src[1]->literal_value == literal(0) &&
			a->bc.src[0].neg == 0 && a->bc.src[0].abs == 0) {

		value *s = a->src[0];

		bool_op_info bop = {};

		PPH_DUMP(
			sblog << "optSETcc ";
			dump::dump_op(a);
			sblog << "\n";
		);

		if (!get_bool_op_info(s, bop))
			return;

		if (cc == AF_CC_E)
			bop.invert = !bop.invert;

		bool swap_args = false;

		cc = bop.n->bc.op_ptr->flags & AF_CC_MASK;

		if (bop.invert)
			cc = invert_setcc_condition(cc, swap_args);

		if (bop.int_cvt) {
			assert(cmp_type != AF_FLOAT_CMP);
			cmp_type = AF_FLOAT_CMP;
		}

		PPH_DUMP(
			sblog << "boi node: ";
			dump::dump_op(bop.n);
			sblog << " invert: " << bop.invert << "  int_cvt: " << bop.int_cvt;
			sblog <<"\n";
		);

		unsigned newop = is_pred ? get_predsetcc_opcode(cc, cmp_type) :
				get_setcc_opcode(cc, cmp_type, dst_type != AF_FLOAT_DST);

		a->bc.set_op(newop);

		if (swap_args) {
			a->src[0] = bop.n->src[1];
			a->src[1] = bop.n->src[0];
			a->bc.src[0] = bop.n->bc.src[1];
			a->bc.src[1] = bop.n->bc.src[0];

		} else {
			a->src[0] = bop.n->src[0];
			a->src[1] = bop.n->src[1];
			a->bc.src[0] = bop.n->bc.src[0];
			a->bc.src[1] = bop.n->bc.src[1];
		}
	}
}

void peephole::optimize_CNDcc_op(alu_node* a) {

	//TODO
}

bool peephole::get_bool_flt_to_int_source(alu_node* &a) {

	if (a->bc.op == ALU_OP1_FLT_TO_INT) {

		if (a->bc.src[0].neg || a->bc.src[0].abs || a->bc.src[0].rel)
			return false;

		value *s = a->src[0];
		if (!s || !s->def || !s->def->is_alu_inst())
			return false;

		alu_node *dn = static_cast<alu_node*>(s->def);

		if (dn->is_alu_op(ALU_OP1_TRUNC)) {
			s = dn->src[0];
			if (!s || !s->def || !s->def->is_alu_inst())
				return false;

			if (dn->bc.src[0].neg != 1 || dn->bc.src[0].abs != 0 ||
					dn->bc.src[0].rel != 0) {
				return false;
			}

			dn = static_cast<alu_node*>(s->def);

		}

		if (dn->bc.op_ptr->flags & AF_SET) {
			a = dn;
			return true;
		}
	}
	return false;
}

bool peephole::get_bool_op_info(value* b, bool_op_info& bop) {

	node *d = b->def;

	if (!d || !d->is_alu_inst())
		return false;

	alu_node *dn = static_cast<alu_node*>(d);

	if (dn->bc.op_ptr->flags & AF_SET) {
		bop.n = dn;

		if (dn->bc.op_ptr->flags & AF_DX10)
			bop.int_cvt = true;

		return true;
	}

	if (get_bool_flt_to_int_source(dn)) {
		bop.n = dn;
		bop.int_cvt = true;
		return true;
	}

	return false;
}

} // namespace r600_sb
