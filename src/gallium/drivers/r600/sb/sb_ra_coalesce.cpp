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

#define RA_DEBUG 0

#if RA_DEBUG
#define RA_DUMP(q) do { q } while (0)
#else
#define RA_DUMP(q)
#endif

#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

int ra_coalesce::run() {
	return sh.coal.run();
}

void coalescer::add_edge(value* a, value* b, unsigned cost) {
	assert(a->is_sgpr() && b->is_sgpr());
	edges.insert(new ra_edge(a,b, cost));
}

void coalescer::create_chunk(value *v) {

	assert(v->is_sgpr());

	ra_chunk *c = new ra_chunk();

	c->values.push_back(v);

	if (v->is_chan_pinned())
		c->flags |= RCF_PIN_CHAN;
	if (v->is_reg_pinned()) {
		c->flags |= RCF_PIN_REG;
	}

	c->pin = v->pin_gpr;

	RA_DUMP(
		sblog << "create_chunk: ";
		dump_chunk(c);
	);

	all_chunks.push_back(c);
	v->chunk = c;

}

void coalescer::unify_chunks(ra_edge *e) {
	ra_chunk *c1 = e->a->chunk, *c2 = e->b->chunk;

	RA_DUMP(
		sblog << "unify_chunks: ";
		dump_chunk(c1);
		dump_chunk(c2);
	);

	if (c2->is_chan_pinned() && !c1->is_chan_pinned()) {
		c1->flags |= RCF_PIN_CHAN;
		c1->pin = sel_chan(c1->pin.sel(), c2->pin.chan());
	}

	if (c2->is_reg_pinned() && !c1->is_reg_pinned()) {
		c1->flags |= RCF_PIN_REG;
		c1->pin = sel_chan(c2->pin.sel(), c1->pin.chan());
	}

	c1->values.reserve(c1->values.size() + c2->values.size());

	for (vvec::iterator I = c2->values.begin(), E = c2->values.end(); I != E;
			++I) {
		(*I)->chunk = c1;
		c1->values.push_back(*I);
	}

	chunk_vec::iterator F = std::find(all_chunks.begin(), all_chunks.end(), c2);
	assert(F != all_chunks.end());

	all_chunks.erase(F);

	c1->cost += c2->cost + e->cost;
	delete c2;
}

bool coalescer::chunks_interference(ra_chunk *c1, ra_chunk *c2) {
	unsigned pin_flags = (c1->flags & c2->flags) &
			(RCF_PIN_CHAN | RCF_PIN_REG);

	if ((pin_flags & RCF_PIN_CHAN) &&
			c1->pin.chan() != c2->pin.chan())
		return true;

	if ((pin_flags & RCF_PIN_REG) &&
			c1->pin.sel() != c2->pin.sel())
		return true;

	for (vvec::iterator I = c1->values.begin(), E = c1->values.end(); I != E;
			++I) {
		value *v1 = *I;

		for (vvec::iterator I = c2->values.begin(), E = c2->values.end(); I != E;
				++I) {
			value *v2 = *I;

			if (!v1->v_equal(v2) && v1->interferences.contains(v2))
				return true;
		}
	}
	return false;
}

void coalescer::build_chunks() {

	for (edge_queue::iterator I = edges.begin(), E = edges.end();
			I != E; ++I) {

		ra_edge *e = *I;

		if (!e->a->chunk)
			create_chunk(e->a);

		if (!e->b->chunk)
			create_chunk(e->b);

		ra_chunk *c1 = e->a->chunk, *c2 = e->b->chunk;

		if (c1 == c2) {
			c1->cost += e->cost;
		} else if (!chunks_interference(c1, c2))
			unify_chunks(e);
	}
}

ra_constraint* coalescer::create_constraint(constraint_kind kind) {
	ra_constraint *c = new ra_constraint(kind);
	all_constraints.push_back(c);
	return c;
}

void coalescer::dump_edges() {
	sblog << "######## affinity edges\n";

	for (edge_queue::iterator I = edges.begin(), E = edges.end();
			I != E; ++I) {
		ra_edge* e = *I;
		sblog << "  ra_edge ";
		dump::dump_val(e->a);
		sblog << " <-> ";
		dump::dump_val(e->b);
		sblog << "   cost = " << e->cost << "\n";
	}
}

void coalescer::dump_chunks() {
	sblog << "######## chunks\n";

	for (chunk_vec::iterator I = all_chunks.begin(), E = all_chunks.end();
			I != E; ++I) {
		ra_chunk* c = *I;
		dump_chunk(c);
	}
}


void coalescer::dump_constraint_queue() {
	sblog << "######## constraints\n";

	for (constraint_queue::iterator I = constraints.begin(),
			E = constraints.end(); I != E; ++I) {
		ra_constraint* c = *I;
		dump_constraint(c);
	}
}

void coalescer::dump_chunk(ra_chunk* c) {
	sblog << "  ra_chunk cost = " << c->cost << "  :  ";
	dump::dump_vec(c->values);

	if (c->flags & RCF_PIN_REG)
		sblog << "   REG = " << c->pin.sel();

	if (c->flags & RCF_PIN_CHAN)
		sblog << "   CHAN = " << c->pin.chan();

	sblog << (c->flags & RCF_GLOBAL ? "  GLOBAL" : "");

	sblog << "\n";
}

void coalescer::dump_constraint(ra_constraint* c) {
	sblog << "  ra_constraint: ";
	switch (c->kind) {
		case CK_PACKED_BS: sblog << "PACKED_BS"; break;
		case CK_PHI: sblog << "PHI"; break;
		case CK_SAME_REG: sblog << "SAME_REG"; break;
		default: sblog << "UNKNOWN_KIND"; assert(0); break;
	}

	sblog << "  cost = " << c->cost << "  : ";
	dump::dump_vec(c->values);

	sblog << "\n";
}

void coalescer::get_chunk_interferences(ra_chunk *c, val_set &s) {

	for (vvec::iterator I = c->values.begin(), E = c->values.end(); I != E;
			++I) {
		value *v = *I;
		s.add_set(v->interferences);
	}
	s.remove_vec(c->values);
}

void coalescer::build_chunk_queue() {
	for (chunk_vec::iterator I = all_chunks.begin(),
			E = all_chunks.end(); I != E; ++I) {
		ra_chunk *c = *I;

		if (!c->is_fixed())
			chunks.insert(c);
	}
}

void coalescer::build_constraint_queue() {
	for (constraint_vec::iterator I = all_constraints.begin(),
			E = all_constraints.end(); I != E; ++I) {
		ra_constraint *c = *I;
		unsigned cost = 0;

		if (c->values.empty() || !c->values.front()->is_sgpr())
			continue;

		if (c->kind != CK_SAME_REG)
			continue;

		for (vvec::iterator I = c->values.begin(), E = c->values.end();
				I != E; ++I) {
			value *v = *I;
			if (!v->chunk)
				create_chunk(v);
			else
				cost += v->chunk->cost;
		}
		c->cost = cost;
		constraints.insert(c);
	}
}

void coalescer::color_chunks() {

	for (chunk_queue::iterator I = chunks.begin(), E = chunks.end();
			I != E; ++I) {
		ra_chunk *c = *I;
		if (c->is_fixed() || c->values.size() == 1)
			continue;

		sb_bitset rb;
		val_set interf;

		get_chunk_interferences(c, interf);

		RA_DUMP(
			sblog << "color_chunks: ";
			dump_chunk(c);
			sblog << "\n interferences: ";
			dump::dump_set(sh,interf);
			sblog << "\n";
		);

		init_reg_bitset(rb, interf);

		unsigned pass = c->is_reg_pinned() ? 0 : 1;

		unsigned cs = c->is_chan_pinned() ? c->pin.chan() : 0;
		unsigned ce = c->is_chan_pinned() ? cs + 1 : 4;

		unsigned color = 0;

		while (pass < 2) {

			unsigned rs, re;

			if (pass == 0) {
				rs = c->pin.sel();
				re = rs + 1;
			} else {
				rs = 0;
				re = sh.num_nontemp_gpr();
			}

			for (unsigned reg = rs; reg < re; ++reg) {
				for (unsigned chan = cs; chan < ce; ++chan) {
					unsigned bit = sel_chan(reg, chan);
					if (bit >= rb.size() || !rb.get(bit)) {
						color = bit;
						break;
					}
				}
				if (color)
					break;
			}

			if (color)
				break;

			++pass;
		}

		assert(color);
		color_chunk(c, color);
	}
}

void coalescer::init_reg_bitset(sb_bitset &bs, val_set &vs) {

	for (val_set::iterator I = vs.begin(sh), E = vs.end(sh); I != E; ++I) {
		value *v = *I;

		if (!v->is_any_gpr())
			continue;

		unsigned gpr = v->get_final_gpr();
		if (!gpr)
			continue;

		if (gpr) {
			if (gpr >= bs.size())
				bs.resize(gpr + 64);
			bs.set(gpr, 1);
		}
	}
}

void coalescer::color_chunk(ra_chunk *c, sel_chan color) {

	vvec vv = c->values;

	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E;
			++I) {
		value *v = *I;

		if (v->is_reg_pinned() && v->pin_gpr.sel() != color.sel()) {
			detach_value(v);
			continue;
		}

		if (v->is_chan_pinned() && v->pin_gpr.chan() != color.chan()) {
			detach_value(v);
			continue;
		}

		v->gpr = color;

		if (v->constraint && v->constraint->kind == CK_PHI)
			v->fix();


		RA_DUMP(
			sblog << " assigned " << color << " to ";
			dump::dump_val(v);
			sblog << "\n";
		);
	}

	c->pin = color;

	if (c->is_reg_pinned()) {
		c->fix();
	}
}

coalescer::~coalescer() {

	// FIXME use pool allocator ??

	for (constraint_vec::iterator I = all_constraints.begin(),
			E = all_constraints.end(); I != E; ++I) {
		delete (*I);
	}

	for (chunk_vec::iterator I = all_chunks.begin(),
			E = all_chunks.end(); I != E; ++I) {
		delete (*I);
	}

	for (edge_queue::iterator I = edges.begin(), E = edges.end();
			I != E; ++I) {
		delete (*I);
	}
}

int coalescer::run() {
	int r;

	RA_DUMP( dump_edges(); );

	build_chunks();
	RA_DUMP( dump_chunks(); );

	build_constraint_queue();
	RA_DUMP( dump_constraint_queue(); );

	if ((r = color_constraints()))
		return r;

	build_chunk_queue();
	color_chunks();

	return 0;
}

void coalescer::color_phi_constraint(ra_constraint* c) {
}

ra_chunk* coalescer::detach_value(value *v) {

	vvec::iterator F = std::find(v->chunk->values.begin(),
	                             v->chunk->values.end(), v);

	assert(F != v->chunk->values.end());
	v->chunk->values.erase(F);
	create_chunk(v);

	if (v->is_reg_pinned()) {
		v->chunk->fix();
	}

	RA_DUMP(
		sblog << "           detached : ";
		dump_chunk(v->chunk);
	);

	return v->chunk;

}

int coalescer::color_reg_constraint(ra_constraint *c) {
	unsigned k, cnt = c->values.size();
	vvec & cv = c->values;

	ra_chunk *ch[4];
	unsigned swz[4] = {0, 1, 2, 3};
	val_set interf[4];
	sb_bitset rb[4];

	bool reg_pinned = false;
	unsigned pin_reg = ~0;

	unsigned chan_mask = 0;

	k = 0;
	for (vvec::iterator I = cv.begin(), E = cv.end(); I != E; ++I, ++k) {
		value *v = *I;

		if (!v->chunk)
			create_chunk(v);

		ch[k] = v->chunk;

		if (v->chunk->is_chan_pinned()) {
			unsigned chan = 1 << v->chunk->pin.chan();

			if (chan & chan_mask) { // channel already in use
				ch[k] = detach_value(v);
				assert(!ch[k]->is_chan_pinned());
			} else {
				chan_mask |= chan;
			}
		}

		if (v->chunk->is_reg_pinned()) {
			if (!reg_pinned) {
				reg_pinned = true;
				pin_reg = v->chunk->pin.sel();
			}
		}

		get_chunk_interferences(ch[k], interf[k]);
		init_reg_bitset(rb[k], interf[k]);
	}

	unsigned start_reg, end_reg;

	start_reg = 0;
	end_reg = sh.num_nontemp_gpr();

	unsigned min_reg = end_reg;
	unsigned min_swz[4];
	unsigned i, pass = reg_pinned ? 0 : 1;

	bool done = false;

	while (pass < 2) {

		unsigned rs, re;

		if (pass == 0) {
			re = pin_reg + 1;
			rs = pin_reg;
		} else {
			re = end_reg;
			rs = start_reg;
		}

		min_reg = re;

		// cycle on swizzle combinations
		do {
			for (i = 0; i < cnt; ++i) {
				if (ch[i]->flags & RCF_PIN_CHAN)
					if (ch[i]->pin.chan() != swz[i])
						break;
			}
			if (i != cnt)
				continue;

			// looking for minimal reg number such that the constrained chunks
			// may be colored with the current swizzle combination
			for (unsigned reg = rs; reg < min_reg; ++reg) {
				for (i = 0; i < cnt; ++i) {
					unsigned bit = sel_chan(reg, swz[i]);
					if (bit < rb[i].size() && rb[i].get(bit))
						break;
				}
				if (i == cnt) {
					done = true;
					min_reg = reg;
					std::copy(swz, swz + 4, min_swz);
					break;
				}
			}

			if (pass == 0 && done)
				break;

		} while (std::next_permutation(swz, swz + 4));

		if (!done && pass) {
			sblog << "sb: ra_coalesce - out of registers\n";
			return -1;
		}

		if (pass == 0 && done)
			break;

		++pass;
	};

	assert(done);

	RA_DUMP(
	sblog << "min reg = " << min_reg << "   min_swz = "
			<< min_swz[0] << min_swz[1] << min_swz[2] << min_swz[3] << "\n";
	);

	for (i = 0; i < cnt; ++i) {
		sel_chan color(min_reg, min_swz[i]);
		ra_chunk *cc = ch[i];

		if (cc->is_fixed()) {
			if (cc->pin != color)
				cc = detach_value(cv[i]);
			else
				continue;
		}

		color_chunk(cc, color);
		cc->fix();
		cc->set_prealloc();
	}

	return 0;
}

int coalescer::color_constraints() {
	int r;

	for (constraint_queue::iterator I = constraints.begin(),
			E = constraints.end(); I != E; ++I) {

		ra_constraint *c = *I;

		RA_DUMP(
			sblog << "color_constraints: ";
			dump_constraint(c);
		);

		if (c->kind == CK_SAME_REG) {
			if ((r = color_reg_constraint(c)))
				return r;
		} else if (c->kind == CK_PHI)
			color_phi_constraint(c);
	}
	return 0;
}

} // namespace r600_sb
