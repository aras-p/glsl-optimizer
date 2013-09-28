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

#include <cstring>

#include "sb_bc.h"
#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

class regbits {
	typedef uint32_t basetype;
	static const unsigned bt_bytes = sizeof(basetype);
	static const unsigned bt_index_shift = 5;
	static const unsigned bt_index_mask = (1u << bt_index_shift) - 1;
	static const unsigned bt_bits = bt_bytes << 3;
	static const unsigned size = MAX_GPR * 4 / bt_bits;

	basetype dta[size];

	unsigned num_temps;

public:

	regbits(unsigned num_temps) : dta(), num_temps(num_temps) {}
	regbits(unsigned num_temps, unsigned value)	: num_temps(num_temps)
	{ set_all(value); }

	regbits(shader &sh, val_set &vs) : num_temps(sh.get_ctx().alu_temp_gprs)
	{ set_all(1); from_val_set(sh, vs); }

	void set_all(unsigned val);
	void from_val_set(shader &sh, val_set &vs);

	void set(unsigned index);
	void clear(unsigned index);
	bool get(unsigned index);

	void set(unsigned index, unsigned val);

	sel_chan find_free_bit();
	sel_chan find_free_chans(unsigned mask);
	sel_chan find_free_chan_by_mask(unsigned mask);
	sel_chan find_free_array(unsigned size, unsigned mask);

	void dump();
};

// =======================================

void regbits::dump() {
	for (unsigned i = 0; i < size * bt_bits; ++i) {

		if (!(i & 31))
			sblog << "\n";

		if (!(i & 3)) {
			sblog.print_w(i / 4, 7);
			sblog << " ";
		}

		sblog << (get(i) ? 1 : 0);
	}
}


void regbits::set_all(unsigned v) {
	memset(&dta, v ? 0xFF : 0x00, size * bt_bytes);
}

void regbits::from_val_set(shader &sh, val_set& vs) {
	val_set &s = vs;
	unsigned g;
	for (val_set::iterator I = s.begin(sh), E = s.end(sh); I != E; ++I) {
		value *v = *I;
		if (v->is_any_gpr()) {
			g = v->get_final_gpr();
			if (!g)
				continue;
		} else
			continue;

		assert(g);
		--g;
		assert(g < 512);
		clear(g);
	}
}

void regbits::set(unsigned index) {
	unsigned ih = index >> bt_index_shift;
	unsigned il = index & bt_index_mask;
	dta[ih] |= ((basetype)1u << il);
}

void regbits::clear(unsigned index) {
	unsigned ih = index >> bt_index_shift;
	unsigned il = index & bt_index_mask;
	assert(ih < size);
	dta[ih] &= ~((basetype)1u << il);
}

bool regbits::get(unsigned index) {
	unsigned ih = index >> bt_index_shift;
	unsigned il = index & bt_index_mask;
	return dta[ih] & ((basetype)1u << il);
}

void regbits::set(unsigned index, unsigned val) {
	unsigned ih = index >> bt_index_shift;
	unsigned il = index & bt_index_mask;
	basetype bm = 1u << il;
	dta[ih] = (dta[ih] & ~bm) | (val << il);
}

// free register for ra means the bit is set
sel_chan regbits::find_free_bit() {
	unsigned elt = 0;
	unsigned bit = 0;

	while (elt < size && !dta[elt])
		++elt;

	if (elt >= size)
		return 0;

	bit = __builtin_ctz(dta[elt]) + (elt << bt_index_shift);

	assert(bit < ((MAX_GPR - num_temps) << 2));

	return bit + 1;
}

// find free gpr component to use as indirectly addressable array
sel_chan regbits::find_free_array(unsigned length, unsigned mask) {
	unsigned cc[4] = {};

	// FIXME optimize this. though hopefully we won't have a lot of arrays
	for (unsigned a = 0; a < MAX_GPR - num_temps; ++a) {
		for(unsigned c = 0; c < MAX_CHAN; ++c) {
			if (mask & (1 << c)) {
				if (get((a << 2) | c)) {
					if (++cc[c] == length)
						return sel_chan(a - length + 1, c);
				} else {
					cc[c] = 0;
				}
			}
		}
	}
	return 0;
}

sel_chan regbits::find_free_chans(unsigned mask) {
	unsigned elt = 0;
	unsigned bit = 0;

	assert (!(mask & ~0xF));
	basetype cd = dta[elt];

	do {
		if (!cd) {
			if (++elt < size) {
				cd = dta[elt];
				bit = 0;
				continue;
			} else
				return 0;
		}

		unsigned p = __builtin_ctz(cd) & ~(basetype)3u;

		assert (p <= bt_bits - bit);
		bit += p;
		cd >>= p;

		if ((cd & mask) == mask) {
			return ((elt << bt_index_shift) | bit) + 1;
		}

		bit += 4;
		cd >>= 4;

	} while (1);

	return 0;
}

sel_chan regbits::find_free_chan_by_mask(unsigned mask) {
	unsigned elt = 0;
	unsigned bit = 0;

	assert (!(mask & ~0xF));
	basetype cd = dta[elt];

	do {
		if (!cd) {
			if (++elt < size) {
				cd = dta[elt];
				bit = 0;
				continue;
			} else
				return 0;
		}

		unsigned p = __builtin_ctz(cd) & ~(basetype)3u;

		assert (p <= bt_bits - bit);
		bit += p;
		cd >>= p;

		if (cd & mask) {
			unsigned nb = __builtin_ctz(cd & mask);
			unsigned ofs = ((elt << bt_index_shift) | bit);
			return nb + ofs + 1;
		}

		bit += 4;
		cd >>= 4;

	} while (1);

	return 0;
}

// ================================

void ra_init::alloc_arrays() {

	gpr_array_vec &ga = sh.arrays();

	for(gpr_array_vec::iterator I = ga.begin(), E = ga.end(); I != E; ++I) {
		gpr_array *a = *I;

		RA_DUMP(
			sblog << "array [" << a->array_size << "] at " << a->base_gpr << "\n";
			sblog << "\n";
		);

		// skip preallocated arrays (e.g. with preloaded inputs)
		if (a->gpr) {
			RA_DUMP( sblog << "   FIXED at " << a->gpr << "\n"; );
			continue;
		}

		bool dead = a->is_dead();

		if (dead) {
			RA_DUMP( sblog << "   DEAD\n"; );
			continue;
		}

		val_set &s = a->interferences;


		for (val_set::iterator I = s.begin(sh), E = s.end(sh); I != E; ++I) {
			value *v = *I;
			if (v->array == a)
				s.remove_val(v);
		}

		RA_DUMP(
			sblog << "  interf: ";
			dump::dump_set(sh, s);
			sblog << "\n";
		);

		regbits rb(sh, s);

		sel_chan base = rb.find_free_array(a->array_size,
		                                   (1 << a->base_gpr.chan()));

		RA_DUMP( sblog << "  found base: " << base << "\n"; );

		a->gpr = base;
	}
}


int ra_init::run() {

	alloc_arrays();

	ra_node(sh.root);
	return 0;
}

void ra_init::ra_node(container_node* c) {

	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *n = *I;
		if (n->type == NT_OP) {
			process_op(n);
		}
		if (n->is_container() && !n->is_alu_packed()) {
			ra_node(static_cast<container_node*>(n));
		}
	}
}

void ra_init::process_op(node* n) {

	bool copy = n->is_copy_mov();

	RA_DUMP(
		sblog << "ra_init: process_op : ";
		dump::dump_op(n);
		sblog << "\n";
	);

	if (n->is_alu_packed()) {
		for (vvec::iterator I = n->src.begin(), E = n->src.end(); I != E; ++I) {
			value *v = *I;
			if (v && v->is_sgpr() && v->constraint &&
					v->constraint->kind == CK_PACKED_BS) {
				color_bs_constraint(v->constraint);
				break;
			}
		}
	}

	if (n->is_fetch_inst() || n->is_cf_inst()) {
		for (vvec::iterator I = n->src.begin(), E = n->src.end(); I != E; ++I) {
			value *v = *I;
			if (v && v->is_sgpr())
				color(v);
		}
	}

	for (vvec::iterator I = n->dst.begin(), E = n->dst.end(); I != E; ++I) {
		value *v = *I;
		if (!v)
			continue;
		if (v->is_sgpr()) {
			if (!v->gpr) {
				if (copy && !v->constraint) {
					value *s = *(n->src.begin() + (I - n->dst.begin()));
					assert(s);
					if (s->is_sgpr()) {
						assign_color(v, s->gpr);
					}
				} else
					color(v);
			}
		}
	}
}

void ra_init::color_bs_constraint(ra_constraint* c) {
	vvec &vv = c->values;
	assert(vv.size() <= 8);

	RA_DUMP(
		sblog << "color_bs_constraint: ";
		dump::dump_vec(vv);
		sblog << "\n";
	);

	regbits rb(ctx.alu_temp_gprs);

	unsigned chan_count[4] = {};
	unsigned allowed_chans = 0x0F;

	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;

		if (!v || v->is_dead())
			continue;

		sel_chan gpr = v->get_final_gpr();

		val_set interf;

		if (v->chunk)
			sh.coal.get_chunk_interferences(v->chunk, interf);
		else
			interf = v->interferences;

		RA_DUMP(
			sblog << "   processing " << *v << "  interferences : ";
			dump::dump_set(sh, interf);
			sblog << "\n";
		);

		if (gpr) {
			unsigned chan = gpr.chan();
			if (chan_count[chan] < 3) {
				++chan_count[chan];
				continue;
			} else {
				v->flags &= ~VLF_FIXED;
				allowed_chans &= ~(1 << chan);
				assert(allowed_chans);
			}
		}

		v->gpr = 0;

		gpr = 1;
		rb.set_all(1);


		rb.from_val_set(sh, interf);

		RA_DUMP(
			sblog << "   regbits : ";
			rb.dump();
			sblog << "\n";
		);

		while (allowed_chans && gpr.sel() < sh.num_nontemp_gpr()) {

			while (rb.get(gpr - 1) == 0)
				gpr = gpr + 1;

			RA_DUMP(
				sblog << "    trying " << gpr << "\n";
			);

			unsigned chan = gpr.chan();
			if (chan_count[chan] < 3) {
				++chan_count[chan];

				if (v->chunk) {
					vvec::iterator F = std::find(v->chunk->values.begin(),
					                             v->chunk->values.end(),
					                             v);
					v->chunk->values.erase(F);
					v->chunk = NULL;
				}

				assign_color(v, gpr);
				break;
			} else {
				allowed_chans &= ~(1 << chan);
			}
			gpr = gpr + 1;
		}

		if (!gpr) {
			sblog << "color_bs_constraint: failed...\n";
			assert(!"coloring failed");
		}
	}
}

void ra_init::color(value* v) {

	if (v->constraint && v->constraint->kind == CK_PACKED_BS) {
		color_bs_constraint(v->constraint);
		return;
	}

	if (v->chunk && v->chunk->is_fixed())
		return;

	RA_DUMP(
		sblog << "coloring ";
		dump::dump_val(v);
		sblog << "   interferences ";
		dump::dump_set(sh, v->interferences);
		sblog << "\n";
	);

	if (v->is_reg_pinned()) {
		assert(v->is_chan_pinned());
		assign_color(v, v->pin_gpr);
		return;
	}

	regbits rb(sh, v->interferences);
	sel_chan c;

	if (v->is_chan_pinned()) {
		RA_DUMP( sblog << "chan_pinned = " << v->pin_gpr.chan() << "  ";	);
		unsigned mask = 1 << v->pin_gpr.chan();
		c = rb.find_free_chans(mask) + v->pin_gpr.chan();
	} else {
		unsigned cm = get_preferable_chan_mask();
		RA_DUMP( sblog << "pref chan mask: " << cm << "\n"; );
		c = rb.find_free_chan_by_mask(cm);
	}

	assert(c && c.sel() < 128 - ctx.alu_temp_gprs && "color failed");
	assign_color(v, c);
}

void ra_init::assign_color(value* v, sel_chan c) {
	add_prev_chan(c.chan());
	v->gpr = c;
	RA_DUMP(
		sblog << "colored ";
		dump::dump_val(v);
		sblog << " to " << c << "\n";
	);
}

// ===================================================

int ra_split::run() {
	split(sh.root);
	return 0;
}

void ra_split::split_phi_src(container_node *loc, container_node *c,
                             unsigned id, bool loop) {
	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *p = *I;
		value* &v = p->src[id], *d = p->dst[0];
		assert(v);

		if (!d->is_sgpr() || v->is_undef())
			continue;

		value *t = sh.create_temp_value();
		if (loop && id == 0)
			loc->insert_before(sh.create_copy_mov(t, v));
		else
			loc->push_back(sh.create_copy_mov(t, v));
		v = t;

		sh.coal.add_edge(v, d, coalescer::phi_cost);
	}
}

void ra_split::split_phi_dst(node* loc, container_node *c, bool loop) {
	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *p = *I;
		value* &v = p->dst[0];
		assert(v);

		if (!v->is_sgpr())
			continue;

		value *t = sh.create_temp_value();
		node *cp = sh.create_copy_mov(v, t);
		if (loop)
			static_cast<container_node*>(loc)->push_front(cp);
		else
			loc->insert_after(cp);
		v = t;
	}
}


void ra_split::init_phi_constraints(container_node *c) {
	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *p = *I;
		ra_constraint *cc = sh.coal.create_constraint(CK_PHI);
		cc->values.push_back(p->dst[0]);

		for (vvec::iterator I = p->src.begin(), E = p->src.end(); I != E; ++I) {
			value *v = *I;
			if (v->is_sgpr())
				cc->values.push_back(v);
		}

		cc->update_values();
	}
}

void ra_split::split(container_node* n) {

	if (n->type == NT_DEPART) {
		depart_node *d = static_cast<depart_node*>(n);
		if (d->target->phi)
			split_phi_src(d, d->target->phi, d->dep_id, false);
	} else if (n->type == NT_REPEAT) {
		repeat_node *r = static_cast<repeat_node*>(n);
		if (r->target->loop_phi)
			split_phi_src(r, r->target->loop_phi, r->rep_id, true);
	} else if (n->type == NT_REGION) {
		region_node *r = static_cast<region_node*>(n);
		if (r->phi) {
			split_phi_dst(r, r->phi, false);
		}
		if (r->loop_phi) {
			split_phi_dst(r->get_entry_code_location(), r->loop_phi,
					true);
			split_phi_src(r, r->loop_phi, 0, true);
		}
	}

	for (node_riterator N, I = n->rbegin(), E = n->rend(); I != E; I = N) {
		N = I;
		++N;
		node *o = *I;
		if (o->type == NT_OP) {
			split_op(o);
		} else if (o->is_container()) {
			split(static_cast<container_node*>(o));
		}
	}

	if (n->type == NT_REGION) {
		region_node *r = static_cast<region_node*>(n);
		if (r->phi)
			init_phi_constraints(r->phi);
		if (r->loop_phi)
			init_phi_constraints(r->loop_phi);
	}
}

void ra_split::split_op(node* n) {
	switch(n->subtype) {
		case NST_ALU_PACKED_INST:
			split_alu_packed(static_cast<alu_packed_node*>(n));
			break;
		case NST_FETCH_INST:
		case NST_CF_INST:
			split_vector_inst(n);
		default:
			break;
	}
}

void ra_split::split_packed_ins(alu_packed_node *n) {
	vvec vv = n->src;
	vvec sv, dv;

	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {

		value *&v = *I;

		if (v && v->is_any_gpr() && !v->is_undef()) {

			vvec::iterator F = std::find(sv.begin(), sv.end(), v);
			value *t;

			if (F != sv.end()) {
				t = *(dv.begin() + (F - sv.begin()));
			} else {
				t = sh.create_temp_value();
				sv.push_back(v);
				dv.push_back(t);
			}
			v = t;
		}
	}

	unsigned cnt = sv.size();

	if (cnt > 0) {
		n->src = vv;
		for (vvec::iterator SI = sv.begin(), DI = dv.begin(), SE = sv.end();
				SI != SE; ++SI, ++DI) {
			n->insert_before(sh.create_copy_mov(*DI, *SI));
		}

		ra_constraint *c = sh.coal.create_constraint(CK_PACKED_BS);
		c->values = dv;
		c->update_values();
	}
}

// TODO handle other packed ops for cayman
void ra_split::split_alu_packed(alu_packed_node* n) {
	switch (n->op()) {
		case ALU_OP2_DOT4:
		case ALU_OP2_CUBE:
			split_packed_ins(n);
			break;
		default:
			break;
	}
}

void ra_split::split_vec(vvec &vv, vvec &v1, vvec &v2, bool allow_swz) {
	unsigned ch = 0;
	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I, ++ch) {

		value* &o = *I;

		if (o) {

			assert(!o->is_dead());

			if (o->is_undef())
				continue;

			if (allow_swz && o->is_float_0_or_1())
				continue;

			value *t;
			vvec::iterator F =
					allow_swz ? std::find(v2.begin(), v2.end(), o) : v2.end();

			if (F != v2.end()) {
				t = *(v1.begin() + (F - v2.begin()));
			} else {
				t = sh.create_temp_value();

				if (!allow_swz) {
					t->flags |= VLF_PIN_CHAN;
					t->pin_gpr = sel_chan(0, ch);
				}

				v2.push_back(o);
				v1.push_back(t);
			}
			o = t;
		}
	}
}

void ra_split::split_vector_inst(node* n) {
	ra_constraint *c;

	bool call_fs = n->is_cf_op(CF_OP_CALL_FS);
	bool no_src_swizzle = n->is_cf_inst() && (n->cf_op_flags() & CF_MEM);

	no_src_swizzle |= n->is_fetch_op(FETCH_OP_VFETCH) ||
			n->is_fetch_op(FETCH_OP_SEMFETCH);

	if (!n->src.empty() && !call_fs) {

		// we may have more than one source vector -
		// fetch instructions with FF_USEGRAD have gradient values in
		// src vectors 1 (src[4-7] and 2 (src[8-11])

		unsigned nvec = n->src.size() >> 2;
		assert(nvec << 2 == n->src.size());

		for (unsigned nv = 0; nv < nvec; ++nv) {
			vvec sv, tv, nsrc(4);
			unsigned arg_start = nv << 2;

			std::copy(n->src.begin() + arg_start,
			          n->src.begin() + arg_start + 4,
			          nsrc.begin());

			split_vec(nsrc, tv, sv, !no_src_swizzle);

			unsigned cnt = sv.size();

			if (no_src_swizzle || cnt) {

				std::copy(nsrc.begin(), nsrc.end(), n->src.begin() + arg_start);

				for(unsigned i = 0, s = tv.size(); i < s; ++i) {
					n->insert_before(sh.create_copy_mov(tv[i], sv[i]));
				}

				c = sh.coal.create_constraint(CK_SAME_REG);
				c->values = tv;
				c->update_values();
			}
		}
	}

	if (!n->dst.empty()) {
		vvec sv, tv, ndst = n->dst;

		split_vec(ndst, tv, sv, true);

		if (sv.size()) {
			n->dst = ndst;

			node *lp = n;
			for(unsigned i = 0, s = tv.size(); i < s; ++i) {
				lp->insert_after(sh.create_copy_mov(sv[i], tv[i]));
				lp = lp->next;
			}

			if (call_fs) {
				for (unsigned i = 0, cnt = tv.size(); i < cnt; ++i) {
					value *v = tv[i];
					value *s = sv[i];
					if (!v)
						continue;

					v->flags |= VLF_PIN_REG | VLF_PIN_CHAN;
					s->flags &= ~(VLF_PIN_REG | VLF_PIN_CHAN);
					sel_chan sel;

					if (s->is_rel()) {
						assert(s->rel->is_const());
						sel = sel_chan(s->select.sel() +
										 s->rel->get_const_value().u,
						             s->select.chan());
					} else
						sel = s->select;

					v->gpr = v->pin_gpr = sel;
					v->fix();
				}
			} else {
				c = sh.coal.create_constraint(CK_SAME_REG);
				c->values = tv;
				c->update_values();
			}
		}
	}
}

void ra_init::add_prev_chan(unsigned chan) {
	prev_chans = (prev_chans << 4) | (1 << chan);
}

unsigned ra_init::get_preferable_chan_mask() {
	unsigned i, used_chans = 0;
	unsigned chans = prev_chans;

	for (i = 0; i < ra_tune; ++i) {
		used_chans |= chans;
		chans >>= 4;
	}

	return (~used_chans) & 0xF;
}

} // namespace r600_sb
