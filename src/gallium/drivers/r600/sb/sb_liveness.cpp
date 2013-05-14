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

#define LIV_DEBUG 0

#if LIV_DEBUG
#define LIV_DUMP(a) do { a } while (0)
#else
#define LIV_DUMP(a)
#endif

namespace r600_sb {

bool liveness::visit(container_node& n, bool enter) {
	if (enter) {
		n.live_after = live;
		process_ins(n);
	} else {
		process_outs(n);
		n.live_before = live;
	}
	return true;
}

bool liveness::visit(bb_node& n, bool enter) {
	if (enter) {
		n.live_after = live;
	} else {
		n.live_before = live;
	}
	return true;
}

bool liveness::visit(alu_group_node& n, bool enter) {
	if (enter) {
	} else {
	}
	return true;
}

bool liveness::visit(cf_node& n, bool enter) {
	if (enter) {
		if (n.bc.op == CF_OP_CF_END) {
			n.flags |= NF_DEAD;
			return false;
		}
		n.live_after = live;
		update_interferences();
		process_op(n);
	} else {
		n.live_before = live;
	}
	return true;
}

bool liveness::visit(alu_node& n, bool enter) {
	if (enter) {
		update_interferences();
		process_op(n);
	} else {
	}
	return false;
}

bool liveness::visit(alu_packed_node& n, bool enter) {
	if (enter) {
		update_interferences();
		process_op(n);

	} else {
	}
	return false;
}

bool liveness::visit(fetch_node& n, bool enter) {
	if (enter) {
		update_interferences();
		process_op(n);
	} else {
	}
	return true;
}

bool liveness::visit(region_node& n, bool enter) {
	if (enter) {
		val_set s = live;

		update_interferences();

		if (n.phi)
			process_phi_outs(n.phi);

		n.live_after = live;

		live.clear();

		if (n.loop_phi) {
			n.live_before.clear();
		}

		assert(n.count() == 1);
		run_on(*static_cast<container_node*>(*n.begin()));

		// second pass for loops
		if (n.loop_phi) {
			process_phi_outs(n.loop_phi);
			n.live_before = live;

			run_on(*static_cast<container_node*>(*n.begin()));

			update_interferences(); // FIXME is it required

			process_phi_outs(n.loop_phi);
			process_phi_branch(n.loop_phi, 0);
		}

		update_interferences(); // FIXME is it required

		n.live_after = s;
		n.live_before = live;
	}
	return false;
}

bool liveness::visit(repeat_node& n, bool enter) {
	if (enter) {
		live = n.target->live_before;
		process_phi_branch(n.target->loop_phi, n.rep_id);
	} else {
	}
	return true;
}

bool liveness::visit(depart_node& n, bool enter) {
	if (enter) {
		live = n.target->live_after;
		if(n.target->phi)
			process_phi_branch(n.target->phi, n.dep_id);
	} else {
	}
	return true;
}

bool liveness::visit(if_node& n, bool enter) {
	if (enter) {
		assert(n.count() == 1);
		n.live_after = live;

		run_on(*static_cast<container_node*>(*n.begin()));

		process_op(n);
		live.add_set(n.live_after);
	}
	return false;
}

void liveness::update_interferences() {
	if (!sh.compute_interferences)
		return;

	if (!live_changed)
		return;

	LIV_DUMP(
		sblog << "interf ";
		dump::dump_set(sh, live);
		sblog << "\n";
	);

	val_set& s = live;
	for(val_set::iterator I = s.begin(sh), E = s.end(sh); I != E; ++I) {
		value *v = *I;
		assert(v);

		if (v->array) {
			v->array->interferences.add_set(s);
		}

		v->interferences.add_set(s);
		v->interferences.remove_val(v);

		LIV_DUMP(
			sblog << "interferences updated for ";
			dump::dump_val(v);
			sblog << " : ";
			dump::dump_set(sh, v->interferences);
			sblog << "\n";
		);
	}
	live_changed = false;
}

bool liveness::remove_val(value *v) {
	if (live.remove_val(v)) {
		v->flags &= ~VLF_DEAD;
		return true;
	}
	v->flags |= VLF_DEAD;
	return false;
}

bool liveness::process_maydef(value *v) {
	bool r = false;
	vvec::iterator S(v->muse.begin());

	for (vvec::iterator I = v->mdef.begin(), E = v->mdef.end(); I != E;
			++I, ++S) {
		value *&d = *I, *&u = *S;
		if (!d) {
			assert(!u);
			continue;
		}

		bool alive = remove_val(d);
		if (alive) {
			r = true;
		} else {
			d = NULL;
			u = NULL;
		}
	}
	return r;
}

bool liveness::remove_vec(vvec &vv) {
	bool r = false;
	for (vvec::reverse_iterator I = vv.rbegin(), E = vv.rend(); I != E; ++I) {
		value* &v = *I;
		if (!v)
			continue;

		if (v->is_rel()) {
			r |= process_maydef(v);
		} else
			r |= remove_val(v);
	}
	return r;
}

bool r600_sb::liveness::visit(node& n, bool enter) {
	if (enter) {
		update_interferences();
		process_op(n);
	}
	return false;
}

bool liveness::process_outs(node& n) {
	bool alive = remove_vec(n.dst);
	if (alive)
		live_changed = true;
	return alive;
}

bool liveness::add_vec(vvec &vv, bool src) {
	bool r = false;
	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;
		if (!v || v->is_readonly())
			continue;

		if (v->is_rel()) {
			r |= add_vec(v->muse, true);
			if (v->rel->is_any_reg())
				r |= live.add_val(v->rel);

		} else if (src) {
			r |= live.add_val(v);
		}
	}

	return r;
}

void liveness::process_ins(node& n) {
	if (!(n.flags & NF_DEAD)) {

		live_changed |= add_vec(n.src, true);
		live_changed |= add_vec(n.dst, false);

		if (n.type == NT_IF) {
			if_node &in = (if_node&)n;
			if (in.cond)
				live_changed |= live.add_val(in.cond);
		}
		if (n.pred)
			live_changed |= live.add_val(n.pred);
	}
}

void liveness::process_op(node& n) {

	LIV_DUMP(
		sblog << "process_op: ";
		dump::dump_op(&n);
		sblog << "\n";
		sblog << "process_op: live_after:";
		dump::dump_set(sh, live);
		sblog << "\n";
	);

	if(!n.dst.empty() || n.is_cf_op(CF_OP_CALL_FS)) {
		if (!process_outs(n)) {
			if (!(n.flags & NF_DONT_KILL))
				n.flags |= NF_DEAD;
		} else {
			n.flags &= ~NF_DEAD;
		}
	}
	process_ins(n);

	LIV_DUMP(
		sblog << "process_op: live_before:";
		dump::dump_set(sh, live);
		sblog << "\n";
	);
}

int liveness::init() {

	if (sh.compute_interferences) {
		gpr_array_vec &vv = sh.arrays();
		for (gpr_array_vec::iterator I = vv.begin(), E = vv.end(); I != E;
				++I) {
			gpr_array *a = *I;
			a->interferences.clear();
		}
	}

	return 0;
}

void liveness::update_src_vec(vvec &vv, bool src) {
	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;

		if (!v || !v->is_sgpr())
			continue;

		if (v->rel && v->rel->is_dead())
			v->rel->flags &= ~VLF_DEAD;

		if (src && v->is_dead()) {
			v->flags &= ~VLF_DEAD;
		}
	}
}

void liveness::process_phi_outs(container_node *phi) {
	for (node_iterator I = phi->begin(), E = phi->end(); I != E; ++I) {
		node *n = *I;
		if (!process_outs(*n)) {
			n->flags |= NF_DEAD;
		} else {
			n->flags &= ~NF_DEAD;
			update_src_vec(n->src, true);
			update_src_vec(n->dst, false);
		}
	}
}

void liveness::process_phi_branch(container_node* phi, unsigned id) {
	val_set &s = live;
	for (node_iterator I = phi->begin(), E = phi->end(); I != E; ++I) {
		node *n = *I;
		if (n->is_dead())
			continue;

		value *v = n->src[id];

		if (!v->is_readonly()) {
			live_changed |= s.add_val(v);
			v->flags &= ~VLF_DEAD;
		}
	}
}

} //namespace r600_sb
