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

unsigned if_conversion::try_convert_kills(region_node* r) {

	// handling the simplest (and probably most frequent) case only -
	// if - 4 kills - endif

	// TODO handle more complex cases

	depart_node *d1 = static_cast<depart_node*>(r->front());
	if (!d1->is_depart())
		return 0;

	if_node *f = static_cast<if_node*>(d1->front());
	if (!f->is_if())
		return 0;

	depart_node *d2 = static_cast<depart_node*>(f->front());
	if (!d2->is_depart())
		return 0;

	unsigned cnt = 0;

	for (node_iterator I = d2->begin(), E = d2->end(); I != E; ++I) {
		alu_node *n = static_cast<alu_node*>(*I);
		if (!n->is_alu_inst())
			return 0;

		if (!(n->bc.op_ptr->flags & AF_KILL))
			return 0;

		if (n->bc.op_ptr->src_count != 2 || n->src.size() != 2)
			return 0;

		value *s1 = n->src[0], *s2 = n->src[1];

		// assuming that the KILL with constant operands is "always kill"

		if (!s1 || !s2 || !s1->is_const() || !s2->is_const())
			return 0;

		++cnt;
	}

	if (cnt > 4)
		return 0;

	value *cond = f->cond;
	value *pred = get_select_value_for_em(sh, cond);

	if (!pred)
		return 0;

	for (node_iterator N, I = d2->begin(), E = d2->end(); I != E; I = N) {
		N = I; ++N;

		alu_node *n = static_cast<alu_node*>(*I);

		IFC_DUMP(
			sblog << "converting ";
			dump::dump_op(n);
			sblog << "   " << n << "\n";
		);

		n->remove();

		n->bc.set_op(ALU_OP2_KILLE_INT);
		n->src[0] = pred;
		n->src[1] = sh.get_const_value(0);
		// reset src modifiers
		memset(&n->bc.src[0], 0, sizeof(bc_alu_src));
		memset(&n->bc.src[1], 0, sizeof(bc_alu_src));

		r->insert_before(n);
	}

	return cnt;
}



bool if_conversion::run_on(region_node* r) {

	if (r->dep_count() != 2 || r->rep_count() != 1)
		return false;

	node_stats s;

	r->collect_stats(s);

	IFC_DUMP(
		sblog << "ifcvt: region " << r->region_id << " :\n";
		s.dump();
	);

	if (s.region_count || s.fetch_count ||
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

	if (s.alu_kill_count) {
		unsigned kcnt = try_convert_kills(r);
		if (kcnt < s.alu_kill_count)
			return false;
	}

	IFC_DUMP( sblog << "if_cvt: processing...\n"; );

	depart_node *nd1 = static_cast<depart_node*>(r->first);
	if (!nd1->is_depart())
		return false;
	if_node *nif = static_cast<if_node*>(nd1->first);
	if (!nif->is_if())
		return false;
	depart_node *nd2 = static_cast<depart_node*>(nif->first);
	if (!nd2->is_depart())
		return false;

	value *em = nif->cond;
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
