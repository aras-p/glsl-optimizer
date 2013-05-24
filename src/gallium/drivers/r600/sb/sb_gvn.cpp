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

#define GVN_DEBUG 0

#if GVN_DEBUG
#define GVN_DUMP(q) do { q } while (0)
#else
#define GVN_DUMP(q)
#endif

#include "sb_shader.h"
#include "sb_pass.h"
#include "sb_sched.h"

namespace r600_sb {

bool gvn::visit(node& n, bool enter) {
	if (enter) {


		bool rewrite = true;

		if (n.dst[0]->is_agpr()) {
			rewrite = false;
		}


		process_op(n, rewrite);

		assert(n.parent);

		if (n.parent->subtype == NST_LOOP_PHI_CONTAINER) {
			// There is a problem - sometimes with nested loops
			// loop counter initialization for inner loop is incorrectly hoisted
			// out of the outer loop

			// FIXME not sure if this is enough to fix a problem completely,
			// possibly more complete fix is needed (anyway, the
			// problem was seen only in relatively complex
			// case involving nested loops and
			// indirect access to loop counters (without proper array info
			// loop counters may be considered as array elements too),
			// was not seen in any tests
			// or real apps when proper array information is available in TGSI).

			// For now just mark the instructions that initialize loop counters
			// with DONT_HOIST flag to prevent the insts like MOV r, 0
			// (initialization of inner loop's counter with const)
			// from being hoisted out of the outer loop

			assert(!n.src.empty());
			value *v = n.src[0];

			if (v->is_any_gpr() && v->def)
				v->def->flags |= NF_DONT_HOIST;
		}

	} else {
	}
	return true;
}

bool gvn::visit(cf_node& n, bool enter) {
	if (enter) {
		process_op(n);
	} else {
	}
	return true;
}

bool gvn::visit(alu_node& n, bool enter) {
	if (enter) {
		process_op(n);
	} else {
	}
	return true;
}

bool gvn::visit(alu_packed_node& n, bool enter) {
	if (enter) {
		process_op(n);
	} else {
	}
	return false;
}

bool gvn::visit(fetch_node& n, bool enter) {
	if (enter) {
		process_op(n);
	} else {
	}
	return true;
}

bool gvn::visit(region_node& n, bool enter) {
	if (enter) {
// FIXME: loop_phi sources are undefined yet (except theone from the preceding
// code), can we handle that somehow?
//		if (n.loop_phi)
//			run_on(*n.loop_phi);
	} else {
		if (n.loop_phi)
			run_on(*n.loop_phi);

		if (n.phi)
			run_on(*n.phi);
	}
	return true;
}

bool gvn::process_src(value* &v, bool rewrite) {
	if (!v->gvn_source)
		sh.vt.add_value(v);

	if (rewrite && !v->gvn_source->is_rel()) {
		v = v->gvn_source;
		return true;
	}
	return false;
}

// FIXME: maybe handle it in the scheduler?
void gvn::process_alu_src_constants(node &n, value* &v) {
	if (n.src.size() < 3) {
		process_src(v, true);
		return;
	}

	if (!v->gvn_source)
		sh.vt.add_value(v);

	rp_kcache_tracker kc(sh);

	if (v->gvn_source->is_kcache())
		kc.try_reserve(v->gvn_source->select);

	// don't propagate 3rd constant to the trans-only instruction
	if (!n.is_alu_packed()) {
		alu_node *a = static_cast<alu_node*>(&n);
		if (a->bc.op_ptr->src_count == 3 && !(a->bc.slot_flags & AF_V)) {
			unsigned const_count = 0;
			for (vvec::iterator I = n.src.begin(), E = n.src.end(); I != E;
					++I) {
				value *c = (*I);
				if (c && c->is_readonly() && ++const_count == 2) {
					process_src(v, false);
					return;
				}
			}
		}
	}

	for (vvec::iterator I = n.src.begin(), E = n.src.end(); I != E; ++I) {
		value *c = (*I);

		if (c->is_kcache() && !kc.try_reserve(c->select)) {
			process_src(v, false);
			return;
		}
	}
	process_src(v, true);
}

void gvn::process_op(node& n, bool rewrite) {

	for(vvec::iterator I = n.src.begin(), E = n.src.end(); I != E; ++I) {
		value* &v = *I;
		if (v) {
			if (v->rel) {
				process_src(v->rel, rewrite);
			}

			if (rewrite && v->gvn_source && v->gvn_source->is_readonly() &&
					n.is_any_alu()) {
				process_alu_src_constants(n, v);
			} else if (rewrite && v->gvn_source && v->gvn_source->is_const() &&
					(n.is_fetch_op(FETCH_OP_VFETCH) ||
							n.is_fetch_op(FETCH_OP_SEMFETCH)))
				process_src(v, false);
			else
				process_src(v, rewrite);
		}
	}
	if (n.pred)
		process_src(n.pred, false);

	if (n.type == NT_IF) {
		if_node &i = (if_node&)n;
		if (i.cond)
			process_src(i.cond, false);
	}

	for(vvec::iterator I = n.dst.begin(), E = n.dst.end(); I != E; ++I) {
		value *v = *I;
		if (v) {
			if (v->rel)
				process_src(v->rel, rewrite);
			sh.vt.add_value(v);
		}
	}
}

} // namespace r600_sb
