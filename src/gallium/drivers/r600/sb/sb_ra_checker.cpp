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

int ra_checker::run() {

	rm_stack.clear();
	rm_stack.resize(1);
	rm_stk_level = 0;

	process_op_dst(sh.root);

	run_on(sh.root);

	assert(rm_stk_level == 0);

	dump_all_errors();

	assert(sh.errors.empty());

	return 0;
}

void ra_checker::dump_error(const error_info &e) {

	sblog << "error at : ";
	dump::dump_op(e.n);

	sblog << "\n";
	sblog << "  : " << e.message << "\n";
}

void ra_checker::dump_all_errors() {
	for (error_map::iterator I = sh.errors.begin(), E = sh.errors.end();
			I != E; ++I) {
		dump_error(I->second);
	}
}


void ra_checker::error(node *n, unsigned id, std::string msg) {
	error_info e;
	e.n = n;
	e.arg_index = id;
	e.message = msg;
	sh.errors.insert(std::make_pair(n, e));
}

void ra_checker::push_stack() {
	++rm_stk_level;
	if (rm_stack.size() == rm_stk_level)
		rm_stack.push_back(rm_stack.back());
	else
		rm_stack[rm_stk_level] = rm_stack[rm_stk_level - 1];
}

void ra_checker::pop_stack() {
	--rm_stk_level;
}

void ra_checker::kill_alu_only_regs() {
	// TODO
}

void ra_checker::check_value_gpr(node *n, unsigned id, value *v) {
	sel_chan gpr = v->gpr;
	if (!gpr) {
		sb_ostringstream o;
		o << "operand value " << *v << " is not allocated";
		error(n, id, o.str());
		return;
	}
	reg_value_map::iterator F = rmap().find(v->gpr);
	if (F == rmap().end()) {
		sb_ostringstream o;
		o << "operand value " << *v << " was not previously written to its gpr";
		error(n, id, o.str());
		return;
	}
	if (!F->second->v_equal(v)) {
		sb_ostringstream o;
		o << "expected operand value " << *v
				<< ", gpr contains " << *(F->second);
		error(n, id, o.str());
		return;
	}


}

void ra_checker::check_src_vec(node *n, unsigned id, vvec &vv, bool src) {

	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;
		if (!v || !v->is_sgpr())
			continue;

		if (v->is_rel()) {
			if (!v->rel) {
				sb_ostringstream o;
				o << "expected relative offset in " << *v;
				error(n, id, o.str());
				return;
			}
		} else if (src) {
			check_value_gpr(n, id, v);
		}
	}
}

void ra_checker::check_op_src(node *n) {
	check_src_vec(n, 0, n->dst, false);
	check_src_vec(n, 100, n->src, true);
}

void ra_checker::process_op_dst(node *n) {

	unsigned id = 0;

	for (vvec::iterator I = n->dst.begin(), E = n->dst.end(); I != E; ++I) {
		value *v = *I;

		++id;

		if (!v)
			continue;

		if (v->is_sgpr()) {

			if (!v->gpr) {
				sb_ostringstream o;
				o << "destination operand " << *v << " is not allocated";
				error(n, id, o.str());
				return;
			}

			rmap()[v->gpr] = v;
		} else if (v->is_rel()) {
			if (v->rel->is_const()) {
				rmap()[v->get_final_gpr()] = v;
			} else {
				unsigned sz = v->array->array_size;
				unsigned start = v->array->gpr;
				for (unsigned i = 0; i < sz; ++i) {
					rmap()[start + (i << 2)] = v;
				}
			}
		}
	}
}

void ra_checker::check_phi_src(container_node *p, unsigned id) {
	for (node_iterator I = p->begin(), E = p->end(); I != E; ++I) {
		node *n = *I;
		value *s = n->src[id];
		if (s->is_sgpr())
			check_value_gpr(n, id, s);
	}
}

void ra_checker::process_phi_dst(container_node *p) {
	for (node_iterator I = p->begin(), E = p->end(); I != E; ++I) {
		node *n = *I;
		process_op_dst(n);
	}
}

void ra_checker::check_alu_group(alu_group_node *g) {

	for (node_iterator I = g->begin(), E = g->end(); I != E; ++I) {
		node *a = *I;
		if (!a->is_alu_inst()) {
			sb_ostringstream o;
			o << "non-alu node inside alu group";
			error(a, 0, o.str());
			return;
		}

		check_op_src(a);
	}

	std::fill(prev_dst, prev_dst + 5, (value*)NULL);

	for (node_iterator I = g->begin(), E = g->end(); I != E; ++I) {
		alu_node *a = static_cast<alu_node*>(*I);

		process_op_dst(a);

		unsigned slot = a->bc.slot;
		prev_dst[slot] = a->dst[0];
	}
}

void ra_checker::run_on(container_node* c) {

	if (c->is_region()) {
		region_node *r = static_cast<region_node*>(c);
		if (r->loop_phi) {
			check_phi_src(r->loop_phi, 0);
			process_phi_dst(r->loop_phi);
		}
	} else if (c->is_depart()) {

		push_stack();

	} else if (c->is_repeat()) {

		push_stack();

	}

	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *n = *I;

		if(n->is_cf_inst() || n->is_fetch_inst()) {
			check_op_src(n);
			process_op_dst(n);
		}

		if (n->is_container()) {
			if (n->is_alu_group()) {
				check_alu_group(static_cast<alu_group_node*>(n));
			} else {
				container_node *nc = static_cast<container_node*>(n);
				run_on(nc);
			}
		}
	}

	if (c->is_depart()) {
		depart_node *r = static_cast<depart_node*>(c);
		check_phi_src(r->target->phi, r->dep_id);
		pop_stack();
	} else if (c->is_repeat()) {
		repeat_node *r = static_cast<repeat_node*>(c);
		assert (r->target->loop_phi);

		pop_stack();
	} else if (c->is_region()) {
		region_node *r = static_cast<region_node*>(c);
		if (r->phi)
			process_phi_dst(r->phi);
	}
}

} // namespace r600_sb
