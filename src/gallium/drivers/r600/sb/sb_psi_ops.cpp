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

bool r600_sb::psi_ops::visit(alu_node& n, bool enter) {
	if (enter) {
	}
	return false;
}

bool psi_ops::visit(node& n, bool enter) {
	if (enter) {
		assert(n.subtype == NST_PSI);

		try_inline(n);

		// TODO eliminate predication until there is full support in all passes
		// unpredicate instructions and replace psi-nodes with conditional moves
		eliminate(n);
	}
	return false;
}

value* get_pred_val(node &n) {
	value *pred_val = NULL;

	for (vvec::iterator I = n.src.begin(), E = n.src.end(); I != E; I += 3) {
		value* &pred = *I;
		if (pred) {
			if (!pred_val)
				pred_val = pred;
			else {
				assert(pred == pred_val);
			}
		}
	}
	return pred_val;
}

// for now we'll never inline psi's with different predicate values,
// so psi node may only contain the refs to one predicate value.
bool psi_ops::try_inline(node& n) {
	assert(n.subtype == NST_PSI);

	vvec &ns = n.src;

	int sz = ns.size();
	assert(sz && (sz % 3 == 0));

	value *pred_val = get_pred_val(n);

	int ps_mask = 0;

	bool r = false;

	for (int i = sz - 1; i >= 0; i -= 3) {

		if (ps_mask == 3) {
			ns.erase(ns.begin(), ns.begin() + i + 1);
			return r;
		}

		value* val = ns[i];
		value* predsel = ns[i-1];
		int ps = !predsel ? 3 : predsel == sh.get_pred_sel(0) ? 1 : 2;

		assert(val->def);

		if (val->def->subtype == NST_PSI && ps == 3) {
			if (get_pred_val(*val->def) != pred_val)
				continue;

			vvec &ds = val->def->src;

			ns.insert(ns.begin() + i + 1, ds.begin(), ds.end());
			ns.erase(ns.begin() + i - 2, ns.begin() + i + 1);
			i += ds.size();
			r = true;

		} else {
			if ((ps_mask & ps) == ps) {
				// this predicate select is subsumed by already handled ops
				ns.erase(ns.begin() + i - 2, ns.begin() + i + 1);
			} else {
				ps_mask |= ps;
			}
		}
	}
	return r;
}

bool psi_ops::try_reduce(node& n) {
	assert(n.subtype == NST_PSI);
	assert(n.src.size() % 3 == 0);

	// TODO

	return false;
}

void psi_ops::unpredicate(node *n) {

	if (!n->is_alu_inst())
		return;

	alu_node *a = static_cast<alu_node*>(n);
	a->pred = NULL;
}

bool psi_ops::eliminate(node& n) {
	assert(n.subtype == NST_PSI);
	assert(n.src.size() == 6);

	value *d = n.dst[0];

	value *s1 = n.src[2];
	value *s2 = n.src[5];

	value *pred = n.src[3];

	bool psel = n.src[4] == sh.get_pred_sel(0);

	value *sel = get_select_value_for_em(sh, pred);

	if (s1->is_undef()) {
		if (s2->is_undef()) {

		} else {
			n.insert_after(sh.create_mov(d, s2));
		}
	} else if (s2->is_undef()) {
		n.insert_after(sh.create_mov(d, s1));
	} else {
		alu_node *a = sh.create_alu();
		a->bc.set_op(ALU_OP3_CNDE_INT);

		a->dst.push_back(d);
		a->src.push_back(sel);

		if (psel) {
			a->src.push_back(s1);
			a->src.push_back(s2);
		} else {
			a->src.push_back(s2);
			a->src.push_back(s1);
		}

		n.insert_after(a);
	}

	n.remove();

	if (s1->is_any_gpr() && !s1->is_undef() && s1->def)
		unpredicate(s1->def);
	if (s2->is_any_gpr() && !s2->is_undef() && s2->def)
		unpredicate(s2->def);

	return false;
}

} // namespace r600_sb
