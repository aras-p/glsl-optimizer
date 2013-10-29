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

#define VT_DEBUG 0

#if VT_DEBUG
#define VT_DUMP(q) do { q } while (0)
#else
#define VT_DUMP(q)
#endif

#include <cstring>

#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

static const char * chans = "xyzw01?_";

sb_ostream& operator << (sb_ostream &o, value &v) {

	bool dead = v.flags & VLF_DEAD;

	if (dead)
		o << "{";

	switch (v.kind) {
	case VLK_SPECIAL_REG: {
		switch (v.select.sel()) {
			case SV_AR_INDEX: o << "AR"; break;
			case SV_ALU_PRED: o << "PR"; break;
			case SV_EXEC_MASK: o << "EM"; break;
			case SV_VALID_MASK: o << "VM"; break;
			default: o << "???specialreg"; break;
		}
		break;
	}

	case VLK_REG:
		o << "R" << v.select.sel() << "."
			<< chans[v.select.chan()];

		break;
	case VLK_KCACHE: {
		o << "C" << v.select.sel() << "." << chans[v.select.chan()];
	}
		break;
	case VLK_CONST:
		o << v.literal_value.f << "|";
		o.print_zw_hex(v.literal_value.u, 8);
		break;
	case VLK_PARAM:
		o << "Param" << (v.select.sel() - ALU_SRC_PARAM_OFFSET)
			<< chans[v.select.chan()];
		break;
	case VLK_TEMP:
		o << "t" << v.select.sel() - shader::temp_regid_offset;
		break;
	case VLK_REL_REG:

		o << "A" << v.select;
		o << "[";
		o << *v.rel;
		o << "]";

		o << "_" << v.uid;

		break;
	case VLK_UNDEF:
		o << "undef";
		break;
	default:
		o << v.kind << "?????";
		break;
	}

	if (v.version)
		o << "." << v.version;

	if (dead)
		o << "}";

	if (v.is_global())
		o << "||";
	if (v.is_fixed())
		o << "F";
	if (v.is_prealloc())
		o << "P";

	sel_chan g;

	if (v.is_rel()) {
		g = v.array->gpr;
	} else {
		g = v.gpr;
	}

	if (g) {
		o << "@R" << g.sel() << "." << chans[g.chan()];
	}

	return o;
}

void value_table::add_value(value* v) {

	if (v->gvn_source) {
		return;
	}

	VT_DUMP(
		sblog << "gvn add_value ";
		dump::dump_val(v);
	);

	value_hash hash = v->hash();
	vt_item & vti = hashtable[hash & size_mask];
	vti.push_back(v);
	++cnt;

	if (v->def && ex.try_fold(v)) {
		VT_DUMP(
			sblog << " folded: ";
			dump::dump_val(v->gvn_source);
			sblog << "\n";
		);
		return;
	}

	int n = 0;
	for (vt_item::iterator I = vti.begin(), E = vti.end(); I != E; ++I, ++n) {
		value *c = *I;

		if (c == v)
			break;

		if (expr_equal(c, v)) {
			v->gvn_source = c->gvn_source;

			VT_DUMP(
				sblog << " found : equal to ";
				dump::dump_val(v->gvn_source);
				sblog << "\n";
			);
			return;
		}
	}

	v->gvn_source = v;
	VT_DUMP(
		sblog << " added new\n";
	);
}

value_hash value::hash() {
	if (ghash)
		return ghash;
	if (is_rel())
		ghash = rel_hash();
	else if (def)
		ghash = def->hash();
	else
		ghash = ((uintptr_t)this) | 1;

	return ghash;
}

value_hash value::rel_hash() {
	value_hash h = rel ? rel->hash() : 0;
	h |= select << 10;
	h |= array->hash();
	return h;
}

bool value_table::expr_equal(value* l, value* r) {
	return ex.equal(l, r);
}

void value_table::get_values(vvec& v) {
	v.resize(cnt);

	vvec::iterator T = v.begin();

	for(vt_table::iterator I = hashtable.begin(), E = hashtable.end();
			I != E; ++I) {
		T = std::copy(I->begin(), I->end(), T);
	}
}

void value::add_use(node* n, use_kind kind, int arg) {
	if (0) {
	sblog << "add_use ";
	dump::dump_val(this);
	sblog << "   =>  ";
	dump::dump_op(n);
	sblog << "     kind " << kind << "    arg " << arg << "\n";
	}
	uses = new use_info(n, kind, arg, uses);
}

unsigned value::use_count() {
	use_info *u = uses;
	unsigned c = 0;
	while (u) {
		++c;
		u = u->next;
	}
	return c;
}

bool value::is_global() {
	if (chunk)
		return chunk->is_global();
	return flags & VLF_GLOBAL;
}

void value::set_global() {
	assert(is_sgpr());
	flags |= VLF_GLOBAL;
	if (chunk)
		chunk->set_global();
}

void value::set_prealloc() {
	assert(is_sgpr());
	flags |= VLF_PREALLOC;
	if (chunk)
		chunk->set_prealloc();
}

bool value::is_fixed() {
	if (array && array->gpr)
		return true;
	if (chunk && chunk->is_fixed())
		return true;
	return flags & VLF_FIXED;
}

void value::fix() {
	if (chunk)
		chunk->fix();
	flags |= VLF_FIXED;
}

bool value::is_prealloc() {
	if (chunk)
		return chunk->is_prealloc();
	return flags & VLF_PREALLOC;
}

void value::delete_uses() {
	use_info *u, *c = uses;
	while (c) {
		u = c->next;
		delete c;
		c = u;
	}
	uses = NULL;
}

void ra_constraint::update_values() {
	for (vvec::iterator I = values.begin(), E = values.end(); I != E; ++I) {
		assert(!(*I)->constraint);
		(*I)->constraint = this;
	}
}

void* sb_pool::allocate(unsigned sz) {
	sz = (sz + SB_POOL_ALIGN - 1) & ~(SB_POOL_ALIGN - 1);
	assert (sz < (block_size >> 6) && "too big allocation size for sb_pool");

	unsigned offset = total_size % block_size;
	unsigned capacity = block_size * blocks.size();

	if (total_size + sz > capacity) {
		total_size = capacity;
		void * nb = malloc(block_size);
		blocks.push_back(nb);
		offset = 0;
	}

	total_size += sz;
	return ((char*)blocks.back() + offset);
}

void sb_pool::free_all() {
	for (block_vector::iterator I = blocks.begin(), E = blocks.end(); I != E;
			++I) {
		free(*I);
	}
}

value* sb_value_pool::create(value_kind k, sel_chan regid,
                             unsigned ver) {
	void* np = allocate(aligned_elt_size);
	value *v = new (np) value(size(), k, regid, ver);
	return v;
}

void sb_value_pool::delete_all() {
	unsigned bcnt = blocks.size();
	unsigned toffset = 0;
	for (unsigned b = 0; b < bcnt; ++b) {
		char *bstart = (char*)blocks[b];
		for (unsigned offset = 0; offset < block_size;
				offset += aligned_elt_size) {
			((value*)(bstart + offset))->~value();
			toffset += aligned_elt_size;
			if (toffset >= total_size)
				return;
		}
	}
}

bool sb_bitset::get(unsigned id) {
	assert(id < bit_size);
	unsigned w = id / bt_bits;
	unsigned b = id % bt_bits;
	return (data[w] >> b) & 1;
}

void sb_bitset::set(unsigned id, bool bit) {
	assert(id < bit_size);
	unsigned w = id / bt_bits;
	unsigned b = id % bt_bits;
	if (w >= data.size())
		data.resize(w + 1);

	if (bit)
		data[w] |= (1 << b);
	else
		data[w] &= ~(1 << b);
}

inline bool sb_bitset::set_chk(unsigned id, bool bit) {
	assert(id < bit_size);
	unsigned w = id / bt_bits;
	unsigned b = id % bt_bits;
	basetype d = data[w];
	basetype dn = (d & ~(1 << b)) | (bit << b);
	bool r = (d != dn);
	data[w] = r ? dn : data[w];
	return r;
}

void sb_bitset::clear() {
	std::fill(data.begin(), data.end(), 0);
}

void sb_bitset::resize(unsigned size) {
	unsigned cur_data_size = data.size();
	unsigned new_data_size = (size + bt_bits - 1) / bt_bits;


	if (new_data_size != cur_data_size)
		data.resize(new_data_size);

	// make sure that new bits in the existing word are cleared
	if (cur_data_size && size > bit_size && bit_size % bt_bits) {
		basetype clear_mask = (~(basetype)0u) << (bit_size % bt_bits);
		data[cur_data_size - 1] &= ~clear_mask;
	}

	bit_size = size;
}

unsigned sb_bitset::find_bit(unsigned start) {
	assert(start < bit_size);
	unsigned w = start / bt_bits;
	unsigned b = start % bt_bits;
	unsigned sz = data.size();

	while (w < sz) {
		basetype d = data[w] >> b;
		if (d != 0) {
			unsigned pos = __builtin_ctz(d) + b + w * bt_bits;
			return pos;
		}

		b = 0;
		++w;
	}

	return bit_size;
}

sb_value_set::iterator::iterator(shader& sh, sb_value_set* s, unsigned nb)
	: vp(sh.get_value_pool()), s(s), nb(nb) {}

bool sb_value_set::add_set_checked(sb_value_set& s2) {
	if (bs.size() < s2.bs.size())
		bs.resize(s2.bs.size());
	sb_bitset nbs = bs | s2.bs;
	if (bs != nbs) {
		bs.swap(nbs);
		return true;
	}
	return false;
}

void r600_sb::sb_value_set::remove_set(sb_value_set& s2) {
	bs.mask(s2.bs);
}

bool sb_value_set::add_val(value* v) {
	assert(v);
	if (bs.size() < v->uid)
		bs.resize(v->uid + 32);

	return bs.set_chk(v->uid - 1, 1);
}

bool sb_value_set::remove_vec(vvec& vv) {
	bool modified = false;
	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		if (*I)
			modified |= remove_val(*I);
	}
	return modified;
}

void sb_value_set::clear() {
	bs.clear();
}

bool sb_value_set::remove_val(value* v) {
	assert(v);
	if (bs.size() < v->uid)
		return false;
	return bs.set_chk(v->uid - 1, 0);
}

bool r600_sb::sb_value_set::add_vec(vvec& vv) {
	bool modified = false;
	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;
		if (v)
			modified |= add_val(v);
	}
	return modified;
}

bool r600_sb::sb_value_set::contains(value* v) {
	unsigned b = v->uid - 1;
	if (b < bs.size())
		return bs.get(v->uid - 1);
	else
		return false;
}

bool sb_value_set::empty() {
	return bs.size() == 0 || bs.find_bit(0) == bs.size();
}

void sb_bitset::swap(sb_bitset& bs2) {
	std::swap(data, bs2.data);
	std::swap(bit_size, bs2.bit_size);
}

bool sb_bitset::operator ==(const sb_bitset& bs2) {
	if (bit_size != bs2.bit_size)
		return false;

	for (unsigned i = 0, c = data.size(); i < c; ++i) {
		if (data[i] != bs2.data[i])
			return false;
	}
	return true;
}

sb_bitset& sb_bitset::operator &=(const sb_bitset& bs2) {
	if (bit_size > bs2.bit_size) {
		resize(bs2.bit_size);
	}

	for (unsigned i = 0, c = std::min(data.size(), bs2.data.size()); i < c;
			++i) {
		data[i] &= bs2.data[i];
	}
	return *this;
}

sb_bitset& sb_bitset::mask(const sb_bitset& bs2) {
	if (bit_size < bs2.bit_size) {
		resize(bs2.bit_size);
	}

	for (unsigned i = 0, c = data.size(); i < c;
			++i) {
		data[i] &= ~bs2.data[i];
	}
	return *this;
}

bool ra_constraint::check() {
	assert(kind == CK_SAME_REG);

	unsigned reg = 0;

	for (vvec::iterator I = values.begin(), E = values.end(); I != E; ++I) {
		value *v = *I;
		if (!v)
			continue;

		if (!v->gpr)
			return false;

		if (reg == 0)
			reg = v->gpr.sel() + 1;
		else if (reg != v->gpr.sel() + 1)
			return false;

		if (v->is_chan_pinned()) {
			if (v->pin_gpr.chan() != v->gpr.chan())
				return false;
		}
	}
	return true;
}

bool gpr_array::is_dead() {
	return false;
}

} // namespace r600_sb
