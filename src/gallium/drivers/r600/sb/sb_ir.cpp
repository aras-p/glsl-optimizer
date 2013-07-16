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

bool node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool container_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool alu_group_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool alu_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool cf_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool fetch_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool region_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }

bool repeat_node::accept(vpass& p, bool enter) {
	return p.visit(*this, enter);
}

bool depart_node::accept(vpass& p, bool enter) {
	return p.visit(*this, enter);
}
bool if_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool bb_node::accept(vpass& p, bool enter) { return p.visit(*this, enter); }
bool alu_packed_node::accept(vpass& p, bool enter) {
	return p.visit(*this, enter);
}

void alu_packed_node::init_args(bool repl) {
	alu_node *p = static_cast<alu_node*>(first);
	assert(p->is_valid());
	while (p) {
		dst.insert(dst.end(), p->dst.begin(), p->dst.end());
		src.insert(src.end(), p->src.begin(), p->src.end());
		p = static_cast<alu_node*>(p->next);
	}

	value *replicated_value = NULL;

	for (vvec::iterator I = dst.begin(), E = dst.end(); I != E; ++I) {
		value *v = *I;
		if (v) {
			if (repl) {
				if (replicated_value)
					v->assign_source(replicated_value);
				else
					replicated_value = v;
			}

			v->def = this;
		}
	}
}

void container_node::insert_node_before(node* s, node* n) {
	if (s->prev) {
		node *sp = s->prev;
		sp->next = n;
		n->prev = sp;
		n->next = s;
		s->prev = n;
	} else {
		n->next = s;
		s->prev = n;
		first = n;
	}
	n->parent = this;
}

void container_node::insert_node_after(node* s, node* n) {
	if (s->next) {
		node *sn = s->next;
		sn->prev = n;
		n->next = sn;
		n->prev = s;
		s->next = n;
	} else {
		n->prev = s;
		s->next = n;
		last = n;
	}
	n->parent = this;
}

void container_node::move(iterator b, iterator e) {
	assert(b != e);

	container_node *source_container = b->parent;
	node *l = source_container->cut(b, e);

	first = last = l;
	first->parent = this;

	while (last->next) {
		last = last->next;
		last->parent = this;
	}
}

node* container_node::cut(iterator b, iterator e) {
	assert(!*b || b->parent == this);
	assert(!*e || e->parent == this);
	assert(b != e);

	if (b->prev) {
		b->prev->next = *e;
	} else {
		first = *e;
	}

	if (*e) {
		e->prev->next = NULL;
		e->prev = b->prev;
	} else {
		last->next = NULL;
		last = b->prev;
	}

	b->prev = NULL;

	return *b;
}

unsigned container_node::count() {
	unsigned c = 0;
	node *t = first;
	while (t) {
		t = t->next;
		c++;
	}
	return c;
}

void container_node::remove_node(node *n) {
	if (n->prev)
		n->prev->next = n->next;
	else
		first = n->next;
	if (n->next)
		n->next->prev = n->prev;
	else
		last = n->prev;
	n->parent = NULL;
}

void container_node::expand(container_node *n) {
	if (!n->empty()) {
		node *e0 = n->first;
		node *e1 = n->last;

		e0->prev = n->prev;
		if (e0->prev) {
			e0->prev->next = e0;
		} else {
			first = e0;
		}

		e1->next = n->next;
		if (e1->next)
			e1->next->prev = e1;
		else
			last = e1;

		do {
			e0->parent = this;
			e0 = e0->next;
		} while (e0 != e1->next);
	} else
		remove_node(n);
}

void container_node::push_back(node *n) {
	if (last) {
		last->next = n;
		n->next = NULL;
		n->prev = last;
		last = n;
	} else {
		assert(!first);
		first = last = n;
		n->prev = n->next = NULL;
	}
	n->parent = this;
}
void container_node::push_front(node *n) {
	if (first) {
		first->prev = n;
		n->prev = NULL;
		n->next = first;
		first = n;
	} else {
		assert(!last);
		first = last = n;
		n->prev = n->next = NULL;
	}
	n->parent = this;
}

void node::insert_before(node* n) {
	 parent->insert_node_before(this, n);
}

void node::insert_after(node* n) {
	 parent->insert_node_after(this, n);
}

void node::replace_with(node* n) {
	n->prev = prev;
	n->next = next;
	n->parent = parent;
	if (prev)
		prev->next = n;
	if (next)
		next->prev = n;

	if (parent->first == this)
		parent->first = n;

	if (parent->last == this)
		parent->last = n;

	parent = NULL;
	next = prev = NULL;
}

void container_node::expand() {
	 parent->expand(this);
}

void node::remove() {parent->remove_node(this);
}

value_hash node::hash_src() {

	value_hash h = 12345;

	for (int k = 0, e = src.size(); k < e; ++k) {
		value *s = src[k];
		if (s)
			h ^=  (s->hash());
	}

	return h;
}


value_hash node::hash() {

	if (parent && parent->subtype == NST_LOOP_PHI_CONTAINER)
		return 47451;

	return hash_src() ^ (subtype << 13) ^ (type << 3);
}

void r600_sb::container_node::append_from(container_node* c) {
	if (!c->first)
		return;

	node *b = c->first;

	if (last) {
		last->next = c->first;
		last->next->prev = last;
	} else {
		first = c->first;
	}

	last = c->last;
	c->first = NULL;
	c->last = NULL;

	while (b) {
		b->parent = this;
		b = b->next;
	}
}

bool node::fold_dispatch(expr_handler* ex) { return ex->fold(*this); }
bool container_node::fold_dispatch(expr_handler* ex) { return ex->fold(*this); }
bool alu_node::fold_dispatch(expr_handler* ex) { return ex->fold(*this); }
bool alu_packed_node::fold_dispatch(expr_handler* ex) { return ex->fold(*this); }
bool fetch_node::fold_dispatch(expr_handler* ex) { return ex->fold(*this); }
bool cf_node::fold_dispatch(expr_handler* ex) { return ex->fold(*this); }

unsigned alu_packed_node::get_slot_mask() {
	unsigned mask = 0;
	for (node_iterator I = begin(), E = end(); I != E; ++I)
		mask |= 1 << static_cast<alu_node*>(*I)->bc.slot;
	return mask;
}

void alu_packed_node::update_packed_items(sb_context &ctx) {

	vvec::iterator SI(src.begin()), DI(dst.begin());

	assert(first);

	alu_node *c = static_cast<alu_node*>(first);
	unsigned flags = c->bc.op_ptr->flags;
	unsigned slot_flags = c->bc.slot_flags;

	// fixup dst for instructions that replicate output
	if (((flags & AF_REPL) && slot_flags == AF_4V) ||
			(ctx.is_cayman() && slot_flags == AF_S)) {

		value *swp[4] = {};

		unsigned chan;

		for (vvec::iterator I2 = dst.begin(), E2 = dst.end();
				I2 != E2; ++I2) {
			value *v = *I2;
			if (v) {
				chan = v->get_final_chan();
				assert(!swp[chan] || swp[chan] == v);
				swp[chan] = v;
			}
		}

		chan = 0;
		for (vvec::iterator I2 = dst.begin(), E2 = dst.end();
				I2 != E2; ++I2, ++chan) {
			*I2 = swp[chan];
		}
	}

	for (node_iterator I = begin(), E = end(); I != E; ++I) {
		alu_node *n = static_cast<alu_node*>(*I);
		assert(n);

		for (vvec::iterator I2 = n->src.begin(), E2 = n->src.end();
				I2 != E2; ++I2, ++SI) {
			*I2 = *SI;
		}
		for (vvec::iterator I2 = n->dst.begin(), E2 = n->dst.end();
				I2 != E2; ++I2, ++DI) {
			*I2 = *DI;
		}
	}
}

bool node::is_cf_op(unsigned op) {
	if (!is_cf_inst())
		return false;
	cf_node *c = static_cast<cf_node*>(this);
	return c->bc.op == op;
}

bool node::is_alu_op(unsigned op) {
	if (!is_alu_inst())
		return false;
	alu_node *c = static_cast<alu_node*>(this);
	return c->bc.op == op;
}

bool node::is_fetch_op(unsigned op) {
	if (!is_fetch_inst())
		return false;
	fetch_node *c = static_cast<fetch_node*>(this);
	return c->bc.op == op;
}



bool node::is_mova() {
	if (!is_alu_inst())
		return false;
	alu_node *a = static_cast<alu_node*>(this);
	return (a->bc.op_ptr->flags & AF_MOVA);
}

bool node::is_pred_set() {
	if (!is_alu_inst())
		return false;
	alu_node *a = static_cast<alu_node*>(this);
	return (a->bc.op_ptr->flags & AF_ANY_PRED);
}

unsigned node::cf_op_flags() {
	assert(is_cf_inst());
	cf_node *c = static_cast<cf_node*>(this);
	return c->bc.op_ptr->flags;
}

unsigned node::alu_op_flags() {
	assert(is_alu_inst());
	alu_node *c = static_cast<alu_node*>(this);
	return c->bc.op_ptr->flags;
}

unsigned node::fetch_op_flags() {
	assert(is_fetch_inst());
	fetch_node *c = static_cast<fetch_node*>(this);
	return c->bc.op_ptr->flags;
}

unsigned node::alu_op_slot_flags() {
	assert(is_alu_inst());
	alu_node *c = static_cast<alu_node*>(this);
	return c->bc.slot_flags;
}

region_node* node::get_parent_region() {
	node *p = this;
	while ((p = p->parent))
		if (p->is_region())
			return static_cast<region_node*>(p);
	return NULL;
}

unsigned container_node::real_alu_count() {
	unsigned c = 0;
	node *t = first;
	while (t) {
		if (t->is_alu_inst())
			++c;
		else if (t->is_alu_packed())
			c += static_cast<container_node*>(t)->count();
		t = t->next;
	}
	return c;
}

void container_node::collect_stats(node_stats& s) {

	for (node_iterator I = begin(), E = end(); I != E; ++I) {
		node *n = *I;
		if (n->is_container()) {
			static_cast<container_node*>(n)->collect_stats(s);
		}

		if (n->is_alu_inst()) {
			++s.alu_count;
			alu_node *a = static_cast<alu_node*>(n);
			if (a->bc.op_ptr->flags & AF_KILL)
				++s.alu_kill_count;
			else if (a->is_copy_mov())
				++s.alu_copy_mov_count;
		} else if (n->is_fetch_inst())
			++s.fetch_count;
		else if (n->is_cf_inst())
			++s.cf_count;
		else if (n->is_region()) {
			++s.region_count;
			region_node *r = static_cast<region_node*>(n);
			if(r->is_loop())
				++s.loop_count;

			if (r->phi)
				s.phi_count += r->phi->count();
			if (r->loop_phi)
				s.loop_phi_count += r->loop_phi->count();
		}
		else if (n->is_depart())
			++s.depart_count;
		else if (n->is_repeat())
			++s.repeat_count;
		else if (n->is_if())
			++s.if_count;
	}
}

void region_node::expand_depart(depart_node *d) {
	depart_vec::iterator I = departs.begin() + d->dep_id, E;
	I = departs.erase(I);
	E = departs.end();
	while (I != E) {
		--(*I)->dep_id;
		++I;
	}
	d->expand();
}

void region_node::expand_repeat(repeat_node *r) {
	repeat_vec::iterator I = repeats.begin() + r->rep_id - 1, E;
	I = repeats.erase(I);
	E = repeats.end();
	while (I != E) {
		--(*I)->rep_id;
		++I;
	}
	r->expand();
}

void node_stats::dump() {
	sblog << "  alu_count : " << alu_count << "\n";
	sblog << "  alu_kill_count : " << alu_kill_count << "\n";
	sblog << "  alu_copy_mov_count : " << alu_copy_mov_count << "\n";
	sblog << "  cf_count : " << cf_count << "\n";
	sblog << "  fetch_count : " << fetch_count << "\n";
	sblog << "  region_count : " << region_count << "\n";
	sblog << "  loop_count : " << loop_count << "\n";
	sblog << "  phi_count : " << phi_count << "\n";
	sblog << "  loop_phi_count : " << loop_phi_count << "\n";
	sblog << "  depart_count : " << depart_count << "\n";
	sblog << "  repeat_count : " << repeat_count << "\n";
	sblog << "  if_count : " << if_count << "\n";
}

unsigned alu_node::interp_param() {
	if (!(bc.op_ptr->flags & AF_INTERP))
		return 0;
	unsigned param;
	if (bc.op_ptr->src_count == 2) {
		param = src[1]->select.sel();
	} else {
		param = src[0]->select.sel();
	}
	return param + 1;
}

alu_group_node* alu_node::get_alu_group_node() {
	node *p = parent;
	if (p) {
		if (p->subtype == NST_ALU_PACKED_INST) {
			assert(p->parent && p->parent->subtype == NST_ALU_GROUP);
			p = p->parent;
		}
		return static_cast<alu_group_node*>(p);
	}
	return NULL;
}

} // namespace r600_sb
