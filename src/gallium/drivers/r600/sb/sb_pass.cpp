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

pass::pass(shader &s) : ctx(s.get_ctx()), sh(s) {}

int pass::run() { return -1; }

int vpass::init() { return 0; }
int vpass::done() {	return 0; }

int vpass::run() {
	int r;
	if ((r = init()))
		return r;

	run_on(*sh.root);

	if ((r = done()))
		return r;

	return 0;
}

void vpass::run_on(container_node &n) {
	if (n.accept(*this, true)) {

		for (node_iterator N, I = n.begin(), E = n.end(); I != E; I = N) {
			N = I;
			++N;

			if (I->is_container()) {
				container_node *c = static_cast<container_node*>(*I);
				run_on(*c);
			} else {
				I->accept(*this, true);
				I->accept(*this, false);
			}
		}

	}
	n.accept(*this, false);
}

bool vpass::visit(node& n, bool enter) { return true; }
bool vpass::visit(container_node& n, bool enter) { return true; }
bool vpass::visit(alu_group_node& n, bool enter) { return true; }
bool vpass::visit(cf_node& n, bool enter) { return true; }
bool vpass::visit(alu_node& n, bool enter) { return true; }
bool vpass::visit(alu_packed_node& n, bool enter) { return true; }
bool vpass::visit(fetch_node& n, bool enter) { return true; }
bool vpass::visit(region_node& n, bool enter) { return true; }
bool vpass::visit(repeat_node& n, bool enter) { return true; }
bool vpass::visit(depart_node& n, bool enter) { return true; }
bool vpass::visit(if_node& n, bool enter) { return true; }
bool vpass::visit(bb_node& n, bool enter) { return true; }

void rev_vpass::run_on(container_node& n) {
	if (n.accept(*this, true)) {

		for (node_riterator N, I = n.rbegin(), E = n.rend(); I != E; I = N) {
			N = I;
			++N;

			if (I->is_container()) {
				container_node *c = static_cast<container_node*>(*I);
				run_on(*c);
			} else {
				I->accept(*this, true);
				I->accept(*this, false);
			}
		}

	}
	n.accept(*this, false);
}

} // namespace r600_sb
