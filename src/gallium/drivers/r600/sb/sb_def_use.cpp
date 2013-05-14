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

int def_use::run() {
	run_on(sh.root, true);
	run_on(sh.root, false);
	return 0;
}

void def_use::process_phi(container_node *c, bool defs, bool uses) {

	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *n = *I;
		if (uses)
			process_uses(n);
		if (defs)
			process_defs(n, n->dst, false);
	}
}

void def_use::run_on(node* n, bool defs) {

	bool is_region = (n->type == NT_REGION);
	bool is_op = (n->type == NT_OP || n->type == NT_IF);

	if (is_op) {

		if (0) {
			sblog << "def_use processing op ";
			dump::dump_op(n);
			sblog << "\n";
		}

		if (defs)
			process_defs(n, n->dst, false);
		else
			process_uses(n);
	} else if (is_region & defs) {
		region_node *r = static_cast<region_node*>(n);
		if (r->loop_phi)
			process_phi(r->loop_phi, true, false);
	}

	if (n->is_container() && n->subtype != NST_ALU_PACKED_INST) {
		container_node *c = static_cast<container_node*>(n);
		for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
			run_on(*I, defs);
		}
	}

	if (is_region) {
		region_node *r = static_cast<region_node*>(n);
		if (r->phi)
			process_phi(r->phi, defs, !defs);
		if (r->loop_phi && !defs)
			process_phi(r->loop_phi, false, true);
	}
}

void def_use::process_defs(node *n, vvec &vv, bool arr_def) {

	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;
		if (!v)
			continue;

		if (arr_def)
			v->adef = n;
		else
			v->def = n;

		v->delete_uses();

		if (v->is_rel()) {
			process_defs(n, v->mdef, true);
		}
	}
}

void def_use::process_uses(node* n) {
	unsigned k = 0;

	for (vvec::iterator I = n->src.begin(), E = n->src.end(); I != E;
			++I, ++k) {
		value *v = *I;
		if (!v || v->is_readonly())
			continue;

		if (v->is_rel()) {
			if (!v->rel->is_readonly())
				v->rel->add_use(n, UK_SRC_REL, k);

			unsigned k2 = 0;
			for (vvec::iterator I = v->muse.begin(), E = v->muse.end();
					I != E; ++I, ++k2) {
				value *v = *I;
				if (!v)
					continue;

				v->add_use(n, UK_MAYUSE, k2);
			}
		} else
			v->add_use(n, UK_SRC, k);
	}

	k = 0;
	for (vvec::iterator I = n->dst.begin(), E = n->dst.end(); I != E;
			++I, ++k) {
		value *v = *I;
		if (!v || !v->is_rel())
			continue;

		if (!v->rel->is_readonly())
			v->rel->add_use(n, UK_DST_REL, k);
		unsigned k2 = 0;
		for (vvec::iterator I = v->muse.begin(), E = v->muse.end();
				I != E; ++I, ++k2) {
			value *v = *I;
			if (!v)
				continue;

			v->add_use(n, UK_MAYDEF, k2);
		}
	}

	if (n->pred)
		n->pred->add_use(n, UK_PRED, 0);

	if (n->type == NT_IF) {
		if_node *i = static_cast<if_node*>(n);
		if (i->cond)
			i->cond->add_use(i, UK_COND, 0);
	}
}

} // namespace r600_sb
