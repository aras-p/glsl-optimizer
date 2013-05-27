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

	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *n = *I;

		if (n->is_container())
			run_on(static_cast<container_node*>(n));
		else {

			if (n->is_alu_inst()) {
				alu_node *a = static_cast<alu_node*>(n);

				if (a->bc.op_ptr->flags &
						(AF_PRED | AF_SET | AF_CMOV | AF_KILL)) {
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

	if (aflags & (AF_PRED | AF_SET | AF_KILL)) {
		optimize_cc_op2(a);
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

void peephole::optimize_cc_op2(alu_node* a) {

	unsigned flags = a->bc.op_ptr->flags;
	unsigned cc = flags & AF_CC_MASK;

	if ((cc != AF_CC_E && cc != AF_CC_NE) || a->pred)
		return;

	unsigned cmp_type = flags & AF_CMP_TYPE_MASK;
	unsigned dst_type = flags & AF_DST_TYPE_MASK;

	int op_kind = (flags & AF_PRED) ? 1 :
			(flags & AF_SET) ? 2 :
			(flags & AF_KILL) ? 3 : 0;

	bool swapped = false;

	if (a->src[0]->is_const() && a->src[0]->literal_value == literal(0)) {
		std::swap(a->src[0],a->src[1]);
		swapped = true;
		// clear modifiers
		memset(&a->bc.src[0], 0, sizeof(bc_alu_src));
		memset(&a->bc.src[1], 0, sizeof(bc_alu_src));
	}

	if (swapped || (a->src[1]->is_const() &&
			a->src[1]->literal_value == literal(0))) {

		value *s = a->src[0];

		bool_op_info bop = {};

		PPH_DUMP(
			sblog << "cc_op2: ";
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

		unsigned newop;

		switch(op_kind) {
		case 1:
			newop = get_predsetcc_op(cc, cmp_type);
			break;
		case 2:
			newop = get_setcc_op(cc, cmp_type, dst_type != AF_FLOAT_DST);
			break;
		case 3:
			newop = get_killcc_op(cc, cmp_type);
			break;
		default:
			newop = ALU_OP0_NOP;
			assert(!"invalid op kind");
			break;
		}

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
	unsigned flags = a->bc.op_ptr->flags;
	unsigned cc = flags & AF_CC_MASK;
	unsigned cmp_type = flags & AF_CMP_TYPE_MASK;
	bool swap = false;

	if (cc == AF_CC_E) {
		swap = !swap;
		cc = AF_CC_NE;
	} else if (cc != AF_CC_NE)
		return;

	value *s = a->src[0];

	bool_op_info bop = {};

	PPH_DUMP(
		sblog << "cndcc: ";
		dump::dump_op(a);
		sblog << "\n";
	);

	if (!get_bool_op_info(s, bop))
		return;

	alu_node *d = bop.n;

	if (d->bc.omod)
		return;

	PPH_DUMP(
		sblog << "cndcc def: ";
		dump::dump_op(d);
		sblog << "\n";
	);


	unsigned dflags = d->bc.op_ptr->flags;
	unsigned dcc = dflags & AF_CC_MASK;
	unsigned dcmp_type = dflags & AF_CMP_TYPE_MASK;
	unsigned ddst_type = dflags & AF_DST_TYPE_MASK;
	int nds;

	// TODO we can handle some of these cases,
	// though probably this shouldn't happen
	if (cmp_type != AF_FLOAT_CMP && ddst_type == AF_FLOAT_DST)
		return;

	if (d->src[0]->is_const() && d->src[0]->literal_value == literal(0))
		nds = 1;
	else if ((d->src[1]->is_const() &&
			d->src[1]->literal_value == literal(0)))
		nds = 0;
	else
		return;

	// can't propagate ABS modifier to CNDcc because it's OP3
	if (d->bc.src[nds].abs)
		return;

	// TODO we can handle some cases for uint comparison
	if (dcmp_type == AF_UINT_CMP)
		return;

	if (dcc == AF_CC_NE) {
		dcc = AF_CC_E;
		swap = !swap;
	}

	if (nds == 1) {
		switch (dcc) {
		case AF_CC_GT: dcc = AF_CC_GE; swap = !swap; break;
		case AF_CC_GE: dcc = AF_CC_GT; swap = !swap; break;
		default: break;
		}
	}

	a->src[0] = d->src[nds];
	a->bc.src[0] = d->bc.src[nds];

	if (swap) {
		std::swap(a->src[1], a->src[2]);
		std::swap(a->bc.src[1], a->bc.src[2]);
	}

	a->bc.set_op(get_cndcc_op(dcc, dcmp_type));

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
