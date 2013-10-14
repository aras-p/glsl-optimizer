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

bool dce_cleanup::visit(node& n, bool enter) {
	if (enter) {
	} else {
		if (n.flags & NF_DEAD)
			n.remove();
		else
			cleanup_dst(n);
	}
	return true;
}

bool dce_cleanup::visit(alu_group_node& n, bool enter) {
	if (enter) {
	} else {
		n.expand();
	}
	return true;
}

bool dce_cleanup::visit(cf_node& n, bool enter) {
	if (enter) {
		if (n.flags & NF_DEAD)
			n.remove();
		else
			cleanup_dst(n);
	} else {
		if ((sh.dce_flags & DF_EXPAND) &&
				(n.bc.op_ptr->flags & (CF_CLAUSE | CF_BRANCH | CF_LOOP)))
			n.expand();
	}
	return true;
}

bool dce_cleanup::visit(alu_node& n, bool enter) {
	if (enter) {
	} else {
		if (n.flags & NF_DEAD)
			n.remove();
		else
			cleanup_dst(n);
	}
	return true;
}

bool dce_cleanup::visit(alu_packed_node& n, bool enter) {
	if (enter) {
	} else {
		if (n.flags & NF_DEAD)
			n.remove();
		else
			cleanup_dst(n);
	}
	return false;
}

bool dce_cleanup::visit(fetch_node& n, bool enter) {
	if (enter) {
	} else {
		if (n.flags & NF_DEAD)
			n.remove();
		else
			cleanup_dst(n);
	}
	return true;
}

bool dce_cleanup::visit(region_node& n, bool enter) {
	if (enter) {
		if (n.loop_phi)
			run_on(*n.loop_phi);
	} else {
		if (n.phi)
			run_on(*n.phi);
	}
	return true;
}

void dce_cleanup::cleanup_dst(node& n) {
	if (!cleanup_dst_vec(n.dst) && remove_unused &&
			!n.dst.empty() && !(n.flags & NF_DONT_KILL) && n.parent)
		n.remove();
}

bool dce_cleanup::visit(container_node& n, bool enter) {
	if (enter)
		cleanup_dst(n);
	return true;
}

bool dce_cleanup::cleanup_dst_vec(vvec& vv) {
	bool alive = false;

	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value* &v = *I;
		if (!v)
			continue;

		if (v->gvn_source && v->gvn_source->is_dead())
			v->gvn_source = NULL;

		if (v->is_dead() || (remove_unused && !v->is_rel() && !v->uses))
			v = NULL;
		else
			alive = true;
	}

	return alive;
}

} // namespace r600_sb
