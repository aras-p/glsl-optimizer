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

#include <stack>
#include <map>

#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

container_node* ssa_prepare::create_phi_nodes(int count) {
	container_node *p = sh.create_container();
	val_set &vars = cur_set();
	node *nn;

	for (val_set::iterator I = vars.begin(sh), E = vars.end(sh); I != E; ++I) {
		nn = sh.create_node(NT_OP, NST_PHI);
		nn->dst.assign(1, *I);
		nn->src.assign(count, *I);
		p->push_back(nn);
	}
	return p;
}

void ssa_prepare::add_defs(node &n) {
	val_set &s = cur_set();
	for (vvec::iterator I = n.dst.begin(), E = n.dst.end(); I != E; ++I) {
		value *v = *I;
		if (!v)
			continue;

		if (v->is_rel()) {
			s.add_vec(v->mdef);
		} else
			s.add_val(v);
	}
}

bool ssa_prepare::visit(cf_node& n, bool enter) {
	if (enter) {
		push_stk();
	} else {
		add_defs(n);
		pop_stk();
	}
	return true;
}

bool ssa_prepare::visit(alu_node& n, bool enter) {
	if (enter) {
	} else {
		add_defs(n);
	}
	return true;
}

bool ssa_prepare::visit(fetch_node& n, bool enter) {
	if (enter) {
	} else {
		add_defs(n);
	}
	return true;
}

bool ssa_prepare::visit(region_node& n, bool enter) {
	if (enter) {

		push_stk();
	} else {
		cur_set().add_set(n.vars_defined);
		if (n.dep_count() > 0)
			n.phi = create_phi_nodes(n.dep_count());
		if (n.rep_count() > 1) {
			n.loop_phi = create_phi_nodes(n.rep_count());
			n.loop_phi->subtype = NST_LOOP_PHI_CONTAINER;
		}
		n.vars_defined.clear();
		pop_stk();
	}
	return true;
}

bool ssa_prepare::visit(repeat_node& n, bool enter) {
	if (enter) {
		push_stk();
	} else {
		assert(n.target);
		n.target->vars_defined.add_set(cur_set());
		cur_set().clear();
		pop_stk();
	}
	return true;
}

bool ssa_prepare::visit(depart_node& n, bool enter) {
	if (enter) {
		push_stk();
	} else {
		assert(n.target);
		n.target->vars_defined.add_set(cur_set());
		cur_set().clear();
		pop_stk();
	}
	return true;
}

// ===============================

int ssa_rename::init() {
	rename_stack.push(def_map());
	return 0;
}

bool ssa_rename::visit(alu_group_node& n, bool enter) {
	// taking into account parallel execution of the alu group
	if (enter) {
		for (node_iterator I = n.begin(), E = n.end(); I != E; ++I) {
			I->accept(*this, true);
		}
	} else {
		for (node_iterator I = n.begin(), E = n.end(); I != E; ++I) {
			I->accept(*this, false);
		}
	}
	return false;
}

bool ssa_rename::visit(cf_node& n, bool enter) {
	if (enter) {
		rename_src(&n);
	} else {
		rename_dst(&n);
	}
	return true;
}

bool ssa_rename::visit(alu_node& n, bool enter) {
	if (enter) {
		rename_src(&n);
	} else {

		node *psi = NULL;

		if (n.pred && n.dst[0]) {

			value *d = n.dst[0];
			unsigned index = get_index(rename_stack.top(), d);
			value *p = sh.get_value_version(d, index);

			psi = sh.create_node(NT_OP, NST_PSI);

			container_node *parent;
			if (n.parent->subtype == NST_ALU_GROUP)
				parent = n.parent;
			else {
				assert (n.parent->parent->subtype == NST_ALU_GROUP);
				parent = n.parent->parent;
			}
			parent->insert_after(psi);

			assert(n.bc.pred_sel);

			psi->src.resize(6);
			psi->src[2] = p;
			psi->src[3] = n.pred;
			psi->src[4] = sh.get_pred_sel(n.bc.pred_sel - PRED_SEL_0);
			psi->src[5] = d;
			psi->dst.push_back(d);
		}

		rename_dst(&n);

		if (psi) {
			rename_src(psi);
			rename_dst(psi);
		}

		if (!n.dst.empty() && n.dst[0]) {
			// FIXME probably use separate pass for such things
			if ((n.bc.op_ptr->flags & AF_INTERP) || n.bc.op == ALU_OP2_CUBE)
				n.dst[0]->flags |= VLF_PIN_CHAN;
		}
	}
	return true;
}

bool ssa_rename::visit(alu_packed_node& n, bool enter) {
	if (enter) {
		for (node_iterator I = n.begin(), E = n.end(); I != E; ++I) {
			I->accept(*this, true);
		}
	} else {
		for (node_iterator I = n.begin(), E = n.end(); I != E; ++I) {
			I->accept(*this, false);
		}

		bool repl = (n.op_ptr()->flags & AF_REPL) ||
				(ctx.is_cayman() && (n.first->alu_op_slot_flags() & AF_S));

		n.init_args(repl);
	}
	return false;
}

bool ssa_rename::visit(fetch_node& n, bool enter) {
	if (enter) {
		rename_src(&n);
		rename_dst(&n);
	} else {
	}
	return true;
}

bool ssa_rename::visit(region_node& n, bool enter) {
	if (enter) {
		if (n.loop_phi)
			rename_phi_args(n.loop_phi, 0, true);
	} else {
		if (n.phi)
			rename_phi_args(n.phi, ~0u, true);
	}
	return true;
}

bool ssa_rename::visit(repeat_node& n, bool enter) {
	if (enter) {
		push(n.target->loop_phi);
	} else {
		if (n.target->loop_phi)
			rename_phi_args(n.target->loop_phi, n.rep_id, false);
		pop();
	}
	return true;
}

bool ssa_rename::visit(depart_node& n, bool enter) {
	if (enter) {
		push(n.target->phi);
	} else {
		if (n.target->phi)
			rename_phi_args(n.target->phi, n.dep_id, false);
		pop();
	}
	return true;
}

bool ssa_rename::visit(if_node& n, bool enter) {
	if (enter) {
	} else {
		n.cond = rename_use(&n, n.cond);
	}
	return true;
}

void ssa_rename::push(node* phi) {
	rename_stack.push(rename_stack.top());
}

void ssa_rename::pop() {
	rename_stack.pop();
}

value* ssa_rename::rename_use(node *n, value* v) {
	if (v->version)
		return v;

	unsigned index = get_index(rename_stack.top(), v);
	v = sh.get_value_version(v, index);

	// if (alu) instruction is predicated and source arg comes from psi node
	// (that is, from another predicated instruction through its psi node),
	// we can try to select the corresponding source value directly
	if (n->pred && v->def && v->def->subtype == NST_PSI) {
		assert(n->subtype == NST_ALU_INST);
		alu_node *an = static_cast<alu_node*>(n);
		node *pn = v->def;
		// FIXME make it more generic ???
		if (pn->src.size() == 6) {
			if (pn->src[3] == n->pred) {
				value* ps = sh.get_pred_sel(an->bc.pred_sel - PRED_SEL_0);
				if (pn->src[4] == ps)
					return pn->src[5];
				else
					return pn->src[2];
			}
		}
	}
	return v;
}

value* ssa_rename::rename_def(node *n, value* v) {
	unsigned index = new_index(def_count, v);
	set_index(rename_stack.top(), v, index);
	value *r = sh.get_value_version(v, index);
	return r;
}

void ssa_rename::rename_src_vec(node *n, vvec &vv, bool src) {
	for(vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value* &v = *I;
		if (!v || v->is_readonly())
			continue;

		if (v->is_rel()) {
			if (!v->rel->is_readonly())
				v->rel = rename_use(n, v->rel);
			rename_src_vec(n, v->muse, true);
		} else if (src)
			v = rename_use(n, v);
	}
}

void ssa_rename::rename_src(node* n) {
	if (n->pred)
		n->pred = rename_use(n, n->pred);

	rename_src_vec(n, n->src, true);
	rename_src_vec(n, n->dst, false);

}

void ssa_rename::rename_dst_vec(node *n, vvec &vv, bool set_def) {

	for(vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value* &v = *I;
		if (!v)
			continue;

		if (v->is_rel()) {
			rename_dst_vec(n, v->mdef, false);
		} else {
			v = rename_def(n, v);
			if (set_def)
				v->def = n;
		}
	}
}

void ssa_rename::rename_dst(node* n) {
	rename_dst_vec(n, n->dst, true);
}

unsigned ssa_rename::get_index(def_map& m, value* v) {
	def_map::iterator I = m.find(v);
	if (I != m.end())
		return I->second;
	return 0;
}

void ssa_rename::set_index(def_map& m, value* v, unsigned index) {
	std::pair<def_map::iterator,bool>  r = m.insert(std::make_pair(v, index));
	if (!r.second)
		r.first->second = index;
}

unsigned ssa_rename::new_index(def_map& m, value* v) {
	unsigned index = 1;
	def_map::iterator I = m.find(v);
	if (I != m.end())
		index = ++I->second;
	else
		m.insert(std::make_pair(v, index));
	return index;
}

bool ssa_rename::visit(node& n, bool enter) {
	if (enter) {
		assert(n.subtype == NST_PSI);
		rename_src(&n);
		rename_dst(&n);
	}
	return false;
}

bool ssa_rename::visit(container_node& n, bool enter) {
	if (enter) {
	} else {
		// should be root container node
		assert(n.parent == NULL);
		rename_src_vec(&n, n.src, true);
	}
	return true;
}

void ssa_rename::rename_phi_args(container_node* phi, unsigned op, bool def) {
	for (node_iterator I = phi->begin(), E = phi->end(); I != E; ++I) {
		node *o = *I;
		if (op != ~0u)
			o->src[op] = rename_use(o, o->src[op]);
		if (def) {
			o->dst[0] = rename_def(o, o->dst[0]);
			o->dst[0]->def = o;
		}
	}
}

} // namespace r600_sb
