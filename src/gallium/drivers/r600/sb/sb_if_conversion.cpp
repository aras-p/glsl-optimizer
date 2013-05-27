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

#define IFC_DEBUG 0

#if IFC_DEBUG
#define IFC_DUMP(q) do { q } while (0)
#else
#define IFC_DUMP(q)
#endif

#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

int if_conversion::run() {

	regions_vec &rv = sh.get_regions();

	unsigned converted = 0;

	for (regions_vec::reverse_iterator N, I = rv.rbegin(), E = rv.rend();
			I != E; I = N) {
		N = I; ++N;

		region_node *r = *I;
		if (run_on(r)) {
			rv.erase(I.base() - 1);
			++converted;
		}
	}
	return 0;
}

void if_conversion::convert_kill_instructions(region_node *r,
                                              value *em, bool branch,
                                              container_node *c) {
	value *cnd = NULL;

	for (node_iterator I = c->begin(), E = c->end(), N; I != E; I = N) {
		N = I + 1;

		if (!I->is_alu_inst())
			continue;

		alu_node *a = static_cast<alu_node*>(*I);
		unsigned flags = a->bc.op_ptr->flags;

		if (!(flags & AF_KILL))
			continue;

		// ignore predicated or non-const kill instructions
		if (a->pred || !a->src[0]->is_const() || !a->src[1]->is_const())
			continue;

		literal l0 = a->src[0]->literal_value;
		literal l1 = a->src[1]->literal_value;

		expr_handler::apply_alu_src_mod(a->bc, 0, l0);
		expr_handler::apply_alu_src_mod(a->bc, 1, l1);

		if (expr_handler::evaluate_condition(flags, l0, l1)) {
			// kill with constant 'true' condition, we'll convert it to the
			// conditional kill outside of the if-then-else block

			a->remove();

			if (!cnd) {
				cnd = get_select_value_for_em(sh, em);
			} else {
				// more than one kill with the same condition, just remove it
				continue;
			}

			r->insert_before(a);
			a->bc.set_op(branch ? ALU_OP2_KILLE_INT : ALU_OP2_KILLNE_INT);

			a->src[0] = cnd;
			a->src[1] = sh.get_const_value(0);
			// clear modifiers
			memset(&a->bc.src[0], 0, sizeof(bc_alu_src));
			memset(&a->bc.src[1], 0, sizeof(bc_alu_src));
		} else {
			// kill with constant 'false' condition, this shouldn't happen
			// but remove it anyway
			a->remove();
		}
	}
}

bool if_conversion::check_and_convert(region_node *r) {

	depart_node *nd1 = static_cast<depart_node*>(r->first);
	if (!nd1->is_depart())
		return false;
	if_node *nif = static_cast<if_node*>(nd1->first);
	if (!nif->is_if())
		return false;
	depart_node *nd2 = static_cast<depart_node*>(nif->first);
	if (!nd2->is_depart())
		return false;

	value* &em = nif->cond;

	node_stats s;

	r->collect_stats(s);

	IFC_DUMP(
		sblog << "ifcvt: region " << r->region_id << " :\n";
		s.dump();
	);

	if (s.region_count || s.fetch_count || s.alu_kill_count ||
			s.if_count != 1 || s.repeat_count)
		return false;

	unsigned real_alu_count = s.alu_count - s.alu_copy_mov_count;

	// if_conversion allows to eliminate JUMP-ALU_POP_AFTER or
	// JUMP-ALU-ELSE-ALU_POP_AFTER, for now let's assume that 3 CF instructions
	// are eliminated. According to the docs, cost of CF instruction is
	// equal to ~40 ALU VLIW instructions (instruction groups),
	// so we have eliminated cost equal to ~120 groups in total.
	// Let's also assume that we have avg 3 ALU instructions per group,
	// This means that potential eliminated cost is about 360 single alu inst.
	// On the other hand, we are speculatively executing conditional code now,
	// so we are increasing the cost in some cases. In the worst case, we'll
	// have to execute real_alu_count additional alu instructions instead of
	// jumping over them. Let's assume for now that average added cost is
	//
	// (0.9 * real_alu_count)
	//
	// So we should perform if_conversion if
	//
	// (0.9 * real_alu_count) < 360, or
	//
	// real_alu_count < 400
	//
	// So if real_alu_count is more than 400, than we think that if_conversion
	// doesn't make sense.

	// FIXME: We can use more precise heuristic, taking into account sizes of
	// the branches and their probability instead of total size.
	// Another way to improve this is to consider the number of the groups
	// instead of the number of instructions (taking into account actual VLIW
	// packing).
	// (Currently we don't know anything about packing at this stage, but
	// probably we can make some more precise estimations anyway)

	if (real_alu_count > 400)
		return false;

	IFC_DUMP( sblog << "if_cvt: processing...\n"; );

	value *select = get_select_value_for_em(sh, em);

	if (!select)
		return false;

	for (node_iterator I = r->phi->begin(), E = r->phi->end(); I != E; ++I) {
		node *n = *I;

		alu_node *ns = convert_phi(select, n);

		if (ns)
			r->insert_after(ns);
	}

	nd2->expand();
	nif->expand();
	nd1->expand();
	r->expand();

	return true;
}

bool if_conversion::run_on(region_node* r) {

	if (r->dep_count() != 2 || r->rep_count() != 1)
		return false;

	depart_node *nd1 = static_cast<depart_node*>(r->first);
	if (!nd1->is_depart())
		return false;
	if_node *nif = static_cast<if_node*>(nd1->first);
	if (!nif->is_if())
		return false;
	depart_node *nd2 = static_cast<depart_node*>(nif->first);
	if (!nd2->is_depart())
		return false;

	value* &em = nif->cond;

	convert_kill_instructions(r, em, true, nd2);
	convert_kill_instructions(r, em, false, nd1);

	if (check_and_convert(r))
		return true;

	if (nd2->empty() && nif->next) {
		// empty true branch, non-empty false branch
		// we'll invert it to get rid of 'else'

		assert(em && em->def);

		alu_node *predset = static_cast<alu_node*>(em->def);

		// create clone of PREDSET instruction with inverted condition.
		// PREDSET has 3 dst operands in our IR (value written to gpr,
		// predicate value and exec mask value), we'll split it such that
		// new PREDSET will define exec mask value only, and two others will
		// be defined in the old PREDSET (if they are not used then DCE will
		// simply remove old PREDSET).

		alu_node *newpredset = sh.clone(predset);
		predset->insert_after(newpredset);

		predset->dst[2] = NULL;

		newpredset->dst[0] = NULL;
		newpredset->dst[1] = NULL;

		em->def = newpredset;

		unsigned cc = newpredset->bc.op_ptr->flags & AF_CC_MASK;
		unsigned cmptype = newpredset->bc.op_ptr->flags & AF_CMP_TYPE_MASK;
		bool swapargs = false;

		cc = invert_setcc_condition(cc, swapargs);

		if (swapargs) {
			std::swap(newpredset->src[0], newpredset->src[1]);
			std::swap(newpredset->bc.src[0], newpredset->bc.src[1]);
		}

		unsigned newopcode = get_predsetcc_op(cc, cmptype);
		newpredset->bc.set_op(newopcode);

		// move the code from the 'false' branch ('else') to the 'true' branch
		nd2->move(nif->next, NULL);

		// swap phi operands
		for (node_iterator I = r->phi->begin(), E = r->phi->end(); I != E;
				++I) {
			node *p = *I;
			assert(p->src.size() == 2);
			std::swap(p->src[0], p->src[1]);
		}
	}

	return false;
}

alu_node* if_conversion::convert_phi(value* select, node* phi) {
	assert(phi->dst.size() == 1 || phi->src.size() == 2);

	value *d = phi->dst[0];
	value *v1 = phi->src[0];
	value *v2 = phi->src[1];

	assert(d);

	if (!d->is_any_gpr())
		return NULL;

	if (v1->is_undef()) {
		if (v2->is_undef()) {
			return NULL;
		} else {
			return sh.create_mov(d, v2);
		}
	} else if (v2->is_undef())
		return sh.create_mov(d, v1);

	alu_node* n = sh.create_alu();

	n->bc.set_op(ALU_OP3_CNDE_INT);
	n->dst.push_back(d);
	n->src.push_back(select);
	n->src.push_back(v1);
	n->src.push_back(v2);

	return n;
}

} // namespace r600_sb
