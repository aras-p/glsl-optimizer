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

#ifndef R600_SB_IR_H_
#define R600_SB_IR_H_

#include <algorithm>
#include <stdint.h>
#include <vector>
#include <set>
#include <algorithm>

#include "sb_bc.h"

namespace r600_sb {

enum special_regs {
	SV_ALU_PRED = 128,
	SV_EXEC_MASK,
	SV_AR_INDEX,
	SV_VALID_MASK
};

class node;
class value;
class shader;

struct sel_chan
{
	unsigned id;

	sel_chan(unsigned id = 0) : id(id) {}
	sel_chan(unsigned sel, unsigned chan) : id(((sel << 2) | chan) + 1) {}

	unsigned sel() const { return sel(id); }
	unsigned chan() const {return chan(id); }
	operator unsigned() const {return id;}

	static unsigned sel(unsigned idx) { return (idx-1) >> 2; }
	static unsigned chan(unsigned idx) { return (idx-1) & 3; }
};

inline sb_ostream& operator <<(sb_ostream& o, sel_chan r) {
	static const char * ch = "xyzw";
	o << r.sel() << "." << ch[r.chan()];
	return o;
}

typedef std::vector<value*>  vvec;

class sb_pool {
protected:
	static const unsigned SB_POOL_ALIGN = 8;
	static const unsigned SB_POOL_DEFAULT_BLOCK_SIZE = (1 << 16);

	typedef std::vector<void*> block_vector;

	unsigned block_size;
	block_vector blocks;
	unsigned total_size;

public:
	sb_pool(unsigned block_size = SB_POOL_DEFAULT_BLOCK_SIZE)
		: block_size(block_size), blocks(), total_size() {}

	virtual ~sb_pool() { free_all(); }

	void* allocate(unsigned sz);

protected:
	void free_all();
};

template <typename V, typename Comp = std::less<V> >
class sb_set {
	typedef std::vector<V> data_vector;
	data_vector vec;
public:

	typedef typename data_vector::iterator iterator;
	typedef typename data_vector::const_iterator const_iterator;

	sb_set() : vec() {}
	~sb_set() {  }

	iterator begin() { return vec.begin(); }
	iterator end() { return vec.end(); }
	const_iterator begin() const { return vec.begin(); }
	const_iterator end() const { return vec.end(); }

	void add_set(const sb_set& s) {
		data_vector t;
		t.reserve(vec.size() + s.vec.size());
		std::set_union(vec.begin(), vec.end(), s.vec.begin(), s.vec.end(),
		          std::inserter(t, t.begin()), Comp());
		vec.swap(t);
	}

	iterator lower_bound(const V& v) {
		return std::lower_bound(vec.begin(), vec.end(), v, Comp());
	}

	std::pair<iterator, bool> insert(const V& v) {
		iterator P = lower_bound(v);
		if (P != vec.end() && is_equal(*P, v))
			return std::make_pair(P, false);
		return std::make_pair(vec.insert(P, v), true);
	}

	unsigned erase(const V&  v) {
		iterator P = lower_bound(v);
		if (P == vec.end() || !is_equal(*P, v))
			return 0;
		vec.erase(P);
		return 1;
	}

	void clear() { vec.clear(); }

	bool empty() { return vec.empty(); }

	bool is_equal(const V& v1, const V& v2) {
		return !Comp()(v1, v2) && !Comp()(v2, v1);
	}

	iterator find(const V& v) {
		iterator P = lower_bound(v);
		return (P != vec.end() && is_equal(*P, v)) ? P : vec.end();
	}

	unsigned size() { return vec.size(); }
	void erase(iterator I) { vec.erase(I); }
};

template <typename K, typename V, typename KComp = std::less<K> >
class sb_map {
	typedef std::pair<K, V> datatype;

	struct Comp {
		bool operator()(const datatype &v1, const datatype &v2) {
			return KComp()(v1.first, v2.first);
		}
	};

	typedef sb_set<datatype, Comp> dataset;

	dataset set;

public:

	sb_map() : set() {}

	typedef typename dataset::iterator iterator;

	iterator begin() { return set.begin(); }
	iterator end() { return set.end(); }

	void clear() { set.clear(); }

	V& operator[](const K& key) {
		datatype P = std::make_pair(key, V());
		iterator F = set.find(P);
		if (F == set.end()) {
			return (*(set.insert(P).first)).second;
		} else {
			return (*F).second;
		}
	}

	std::pair<iterator, bool> insert(const datatype& d) {
		return set.insert(d);
	}

	iterator find(const K& key) {
		return set.find(std::make_pair(key, V()));
	}

	unsigned erase(const K& key) {
		return set.erase(std::make_pair(key, V()));
	}

	void erase(iterator I) {
		set.erase(I);
	}
};

class sb_bitset {
	typedef uint32_t basetype;
	static const unsigned bt_bits = sizeof(basetype) << 3;
	std::vector<basetype> data;
	unsigned bit_size;

public:

	sb_bitset() : data(), bit_size() {}

	bool get(unsigned id);
	void set(unsigned id, bool bit = true);
	bool set_chk(unsigned id, bool bit = true);

	void clear();
	void resize(unsigned size);

	unsigned size() { return bit_size; }

	unsigned find_bit(unsigned start = 0);

	void swap(sb_bitset & bs2);

	bool operator==(const sb_bitset &bs2);
	bool operator!=(const sb_bitset &bs2) { return !(*this == bs2); }

	sb_bitset& operator|=(const sb_bitset &bs2) {
		if (bit_size < bs2.bit_size) {
			resize(bs2.bit_size);
		}

		for (unsigned i = 0, c = std::min(data.size(), bs2.data.size()); i < c;
				++i) {
			data[i] |= bs2.data[i];
		}
		return *this;
	}

	sb_bitset& operator&=(const sb_bitset &bs2);
	sb_bitset& mask(const sb_bitset &bs2);

	friend sb_bitset operator|(const sb_bitset &b1, const sb_bitset &b2) {
			sb_bitset nbs(b1);
			nbs |= b2;
			return nbs;
	}
};

class value;

enum value_kind {
	VLK_REG,
	VLK_REL_REG,
	VLK_SPECIAL_REG,
	VLK_TEMP,

	VLK_CONST,
	VLK_KCACHE,
	VLK_PARAM,
	VLK_SPECIAL_CONST,

	VLK_UNDEF
};



class sb_value_pool : protected sb_pool {
	unsigned aligned_elt_size;

public:
	sb_value_pool(unsigned elt_size, unsigned block_elts = 256)
		: sb_pool(block_elts * (aligned_elt_size = ((elt_size +
				SB_POOL_ALIGN - 1) & ~(SB_POOL_ALIGN - 1)))) {}

	virtual ~sb_value_pool() { delete_all(); }

	value* create(value_kind k, sel_chan regid, unsigned ver);

	value* operator[](unsigned id) {
		unsigned offset = id * aligned_elt_size;
		unsigned block_id;
		if (offset < block_size) {
			block_id = 0;
		} else {
			block_id = offset / block_size;
			offset = offset % block_size;
		}
		return (value*)((char*)blocks[block_id] + offset);
	}

	unsigned size() { return total_size / aligned_elt_size; }

protected:
	void delete_all();
};





class sb_value_set {

	sb_bitset bs;

public:
	sb_value_set() : bs() {}

	class iterator {
		sb_value_pool &vp;
		sb_value_set *s;
		unsigned nb;
	public:
		iterator(shader &sh, sb_value_set *s, unsigned nb = 0);


		iterator& operator++() {
			if (nb + 1 < s->bs.size())
				nb = s->bs.find_bit(nb + 1);
			else
				nb = s->bs.size();
			return *this;
		}
		bool operator !=(const iterator &i) {
			return s != i.s || nb != i.nb;
		}
		bool operator ==(const iterator &i) { return !(*this != i); }
		value* operator *() {
			 return vp[nb];
		}


	};

	iterator begin(shader &sh) {
		return iterator(sh, this, bs.size() ? bs.find_bit(0) : 0);
	}
	iterator end(shader &sh) { return iterator(sh, this, bs.size()); }

	bool add_set_checked(sb_value_set & s2);

	void add_set(sb_value_set & s2)  {
		if (bs.size() < s2.bs.size())
			bs.resize(s2.bs.size());
		bs |= s2.bs;
	}

	void remove_set(sb_value_set & s2);

	bool add_vec(vvec &vv);

	bool add_val(value *v);
	bool contains(value *v);

	bool remove_val(value *v);

	bool remove_vec(vvec &vv);

	void clear();

	bool empty();
};

typedef sb_value_set val_set;

struct gpr_array {
	sel_chan base_gpr; // original gpr
	sel_chan gpr; // assigned by regalloc
	unsigned array_size;

	gpr_array(sel_chan base_gpr, unsigned array_size) : base_gpr(base_gpr),
			array_size(array_size) {}

	unsigned hash() { return (base_gpr << 10) * array_size; }

	val_set interferences;
	vvec refs;

	bool is_dead();

};

typedef std::vector<gpr_array*> regarray_vec;

enum value_flags {
	VLF_UNDEF = (1 << 0),
	VLF_READONLY = (1 << 1),
	VLF_DEAD = (1 << 2),

	VLF_PIN_REG = (1 << 3),
	VLF_PIN_CHAN = (1 << 4),

	// opposite to alu clause local value - goes through alu clause boundary
	// (can't use temp gpr, can't recolor in the alu scheduler, etc)
	VLF_GLOBAL = (1 << 5),
	VLF_FIXED = (1 << 6),
	VLF_PVPS = (1 << 7),

	VLF_PREALLOC = (1 << 8)
};

inline value_flags operator |(value_flags l, value_flags r) {
	return (value_flags)((unsigned)l|(unsigned)r);
}
inline value_flags operator &(value_flags l, value_flags r) {
	return (value_flags)((unsigned)l&(unsigned)r);
}
inline value_flags operator ~(value_flags l) {
	return (value_flags)(~(unsigned)l);
}
inline value_flags& operator |=(value_flags &l, value_flags r) {
	l = l | r;
	return l;
}
inline value_flags& operator &=(value_flags &l, value_flags r) {
	l = l & r;
	return l;
}

struct value;

sb_ostream& operator << (sb_ostream &o, value &v);

typedef uint32_t value_hash;

enum use_kind {
	UK_SRC,
	UK_SRC_REL,
	UK_DST_REL,
	UK_MAYDEF,
	UK_MAYUSE,
	UK_PRED,
	UK_COND
};

struct use_info {
	use_info *next;
	node *op;
	use_kind kind;
	int arg;

	use_info(node *n, use_kind kind, int arg, use_info* next)
		: next(next), op(n), kind(kind), arg(arg) {}
};

enum constraint_kind {
	CK_SAME_REG,
	CK_PACKED_BS,
	CK_PHI
};

class shader;
class sb_value_pool;
class ra_chunk;
class ra_constraint;

class value {
protected:
	value(unsigned sh_id, value_kind k, sel_chan select, unsigned ver = 0)
		: kind(k), flags(),
			rel(), array(),
			version(ver), select(select), pin_gpr(select), gpr(),
			gvn_source(), ghash(),
			def(), adef(), uses(), constraint(), chunk(),
			literal_value(), uid(sh_id) {}

	~value() { delete_uses(); }

	friend class sb_value_pool;
public:
	value_kind kind;
	value_flags flags;

	vvec mdef;
	vvec muse;
	value *rel;
	gpr_array *array;

	unsigned version;

	sel_chan select;
	sel_chan pin_gpr;
	sel_chan gpr;

	value *gvn_source;
	value_hash ghash;

	node *def, *adef;
	use_info *uses;

	ra_constraint *constraint;
	ra_chunk *chunk;

	literal literal_value;

	bool is_const() { return kind == VLK_CONST || kind == VLK_UNDEF; }

	bool is_AR() {
		return is_special_reg() && select == sel_chan(SV_AR_INDEX, 0);
	}

	node* any_def() {
		assert(!(def && adef));
		return def ? def : adef;
	}

	value* gvalue() {
		value *v = this;
		while (v->gvn_source && v != v->gvn_source)
			// FIXME we really shouldn't have such chains
			v = v->gvn_source;
		return v;
	}

	bool is_float_0_or_1() {
		value *v = gvalue();
		return v->is_const() && (v->literal_value == literal(0)
						|| v->literal_value == literal(1.0f));
	}

	bool is_undef() { return gvalue()->kind == VLK_UNDEF; }

	bool is_any_gpr() {
		return (kind == VLK_REG || kind == VLK_TEMP);
	}

	bool is_agpr() {
		return array && is_any_gpr();
	}

	// scalar gpr, as opposed to element of gpr array
	bool is_sgpr() {
		return !array && is_any_gpr();
	}

	bool is_special_reg() {	return kind == VLK_SPECIAL_REG;	}
	bool is_any_reg() { return is_any_gpr() || is_special_reg(); }
	bool is_kcache() { return kind == VLK_KCACHE; }
	bool is_rel() {	return kind == VLK_REL_REG; }
	bool is_readonly() { return flags & VLF_READONLY; }

	bool is_chan_pinned() { return flags & VLF_PIN_CHAN; }
	bool is_reg_pinned() { return flags & VLF_PIN_REG; }

	bool is_global();
	void set_global();
	void set_prealloc();

	bool is_prealloc();

	bool is_fixed();
	void fix();

	bool is_dead() { return flags & VLF_DEAD; }

	literal & get_const_value() {
		value *v = gvalue();
		assert(v->is_const());
		return v->literal_value;
	}

	// true if needs to be encoded as literal in alu
	bool is_literal() {
		return is_const()
				&& literal_value != literal(0)
				&& literal_value != literal(1)
				&& literal_value != literal(-1)
				&& literal_value != literal(0.5)
				&& literal_value != literal(1.0);
	}

	void add_use(node *n, use_kind kind, int arg);

	value_hash hash();
	value_hash rel_hash();

	void assign_source(value *v) {
		assert(!gvn_source || gvn_source == this);
		gvn_source = v->gvalue();
	}

	bool v_equal(value *v) { return gvalue() == v->gvalue(); }

	unsigned use_count();
	void delete_uses();

	sel_chan get_final_gpr() {
		if (array && array->gpr) {
			int reg_offset = select.sel() - array->base_gpr.sel();
			if (rel && rel->is_const())
				reg_offset += rel->get_const_value().i;
			return array->gpr + (reg_offset << 2);
		} else {
			return gpr;
		}
	}

	unsigned get_final_chan() {
		if (array) {
			assert(array->gpr);
			return array->gpr.chan();
		} else {
			assert(gpr);
			return gpr.chan();
		}
	}

	val_set interferences;
	unsigned uid;
};

class expr_handler;

class value_table {
	typedef std::vector<value*> vt_item;
	typedef std::vector<vt_item> vt_table;

	expr_handler &ex;

	unsigned size_bits;
	unsigned size;
	unsigned size_mask;

	vt_table hashtable;

	unsigned cnt;

public:

	value_table(expr_handler &ex, unsigned size_bits = 10)
		: ex(ex), size_bits(size_bits), size(1u << size_bits),
		  size_mask(size - 1), hashtable(size), cnt() {}

	~value_table() {}

	void add_value(value* v);

	bool expr_equal(value* l, value* r);

	unsigned count() { return cnt; }

	void get_values(vvec & v);
};

class sb_context;

enum node_type {
	NT_UNKNOWN,
	NT_LIST,
	NT_OP,
	NT_REGION,
	NT_REPEAT,
	NT_DEPART,
	NT_IF,
};

enum node_subtype {
	NST_UNKNOWN,
	NST_LIST,
	NST_ALU_GROUP,
	NST_ALU_CLAUSE,
	NST_ALU_INST,
	NST_ALU_PACKED_INST,
	NST_CF_INST,
	NST_FETCH_INST,
	NST_TEX_CLAUSE,
	NST_VTX_CLAUSE,

	NST_BB,

	NST_PHI,
	NST_PSI,
	NST_COPY,

	NST_LOOP_PHI_CONTAINER,
	NST_LOOP_CONTINUE,
	NST_LOOP_BREAK
};

enum node_flags {
	NF_EMPTY = 0,
	NF_DEAD = (1 << 0),
	NF_REG_CONSTRAINT = (1 << 1),
	NF_CHAN_CONSTRAINT = (1 << 2),
	NF_ALU_4SLOT = (1 << 3),
	NF_CONTAINER = (1 << 4),

	NF_COPY_MOV = (1 << 5),

	NF_DONT_KILL = (1 << 6),
	NF_DONT_HOIST = (1 << 7),
	NF_DONT_MOVE = (1 << 8),

	// for KILLxx - we want to schedule them as early as possible
	NF_SCHEDULE_EARLY = (1 << 9),

	// for ALU_PUSH_BEFORE - when set, replace with PUSH + ALU
	NF_ALU_STACK_WORKAROUND = (1 << 10)
};

inline node_flags operator |(node_flags l, node_flags r) {
	return (node_flags)((unsigned)l|(unsigned)r);
}
inline node_flags& operator |=(node_flags &l, node_flags r) {
	l = l | r;
	return l;
}

inline node_flags& operator &=(node_flags &l, node_flags r) {
	l = (node_flags)((unsigned)l & (unsigned)r);
	return l;
}

inline node_flags operator ~(node_flags r) {
	return (node_flags)~(unsigned)r;
}

struct node_stats {
	unsigned alu_count;
	unsigned alu_kill_count;
	unsigned alu_copy_mov_count;
	unsigned cf_count;
	unsigned fetch_count;
	unsigned region_count;
	unsigned loop_count;
	unsigned phi_count;
	unsigned loop_phi_count;
	unsigned depart_count;
	unsigned repeat_count;
	unsigned if_count;

	node_stats() : alu_count(), alu_kill_count(), alu_copy_mov_count(),
			cf_count(), fetch_count(), region_count(),
			loop_count(), phi_count(), loop_phi_count(), depart_count(),
			repeat_count(), if_count() {}

	void dump();
};

class shader;

class vpass;

class container_node;
class region_node;

class node {

protected:
	node(node_type nt, node_subtype nst, node_flags flags = NF_EMPTY)
	: prev(), next(), parent(),
	  type(nt), subtype(nst), flags(flags),
	  pred(), dst(), src() {}

	virtual ~node() {};

public:
	node *prev, *next;
	container_node *parent;

	node_type type;
	node_subtype subtype;
	node_flags flags;

	value *pred;

	vvec dst;
	vvec src;

	virtual bool is_valid() { return true; }
	virtual bool accept(vpass &p, bool enter);

	void insert_before(node *n);
	void insert_after(node *n);
	void replace_with(node *n);
	void remove();

	virtual value_hash hash();
	value_hash hash_src();

	virtual bool fold_dispatch(expr_handler *ex);

	bool is_container() { return flags & NF_CONTAINER; }

	bool is_alu_packed() { return subtype == NST_ALU_PACKED_INST; }
	bool is_alu_inst() { return subtype == NST_ALU_INST; }
	bool is_alu_group() { return subtype == NST_ALU_GROUP; }
	bool is_alu_clause() { return subtype == NST_ALU_CLAUSE; }

	bool is_fetch_clause() {
		return subtype == NST_TEX_CLAUSE || subtype == NST_VTX_CLAUSE;
	}

	bool is_copy() { return subtype == NST_COPY; }
	bool is_copy_mov() { return flags & NF_COPY_MOV; }
	bool is_any_alu() { return is_alu_inst() || is_alu_packed() || is_copy(); }

	bool is_fetch_inst() { return subtype == NST_FETCH_INST; }
	bool is_cf_inst() { return subtype == NST_CF_INST; }

	bool is_region() { return type == NT_REGION; }
	bool is_depart() { return type == NT_DEPART; }
	bool is_repeat() { return type == NT_REPEAT; }
	bool is_if() { return type == NT_IF; }
	bool is_bb() { return subtype == NST_BB; }

	bool is_phi() { return subtype == NST_PHI; }

	bool is_dead() { return flags & NF_DEAD; }

	bool is_cf_op(unsigned op);
	bool is_alu_op(unsigned op);
	bool is_fetch_op(unsigned op);

	unsigned cf_op_flags();
	unsigned alu_op_flags();
	unsigned alu_op_slot_flags();
	unsigned fetch_op_flags();

	bool is_mova();
	bool is_pred_set();

	bool vec_uses_ar(vvec &vv) {
		for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
			value *v = *I;
			if (v && v->rel && !v->rel->is_const())
				return true;
		}
		return false;
	}

	bool uses_ar() {
		return vec_uses_ar(dst) || vec_uses_ar(src);
	}


	region_node* get_parent_region();

	friend class shader;
};

class container_node : public node {
public:

	container_node(node_type nt = NT_LIST, node_subtype nst = NST_LIST,
	               node_flags flags = NF_EMPTY)
	: node(nt, nst, flags | NF_CONTAINER), first(), last(),
	  live_after(), live_before() {}

	// child items list
	node *first, *last;

	val_set live_after;
	val_set live_before;

	class iterator {
		node *p;
	public:
		iterator(node *pp = NULL) : p(pp) {}
		iterator & operator ++() { p = p->next; return *this;}
		iterator & operator --() { p = p->prev; return *this;}
		node* operator *() { return p; }
		node* operator ->() { return p; }
		const iterator advance(int n) {
			if (!n) return *this;
			iterator I(p);
			if (n > 0) while (n--) ++I;
			else while (n++) --I;
			return I;
		}
		const iterator operator +(int n) { return advance(n); }
		const iterator operator -(int n) { return advance(-n); }
		bool operator !=(const iterator &i) { return p != i.p; }
		bool operator ==(const iterator &i) { return p == i.p; }
	};

	class riterator {
		iterator i;
	public:
		riterator(node *p = NULL) : i(p) {}
		riterator & operator ++() { --i; return *this;}
		riterator & operator --() { ++i; return *this;}
		node* operator *() { return *i; }
		node* operator ->() { return *i; }
		bool operator !=(const riterator &r) { return i != r.i; }
		bool operator ==(const riterator &r) { return i == r.i; }
	};

	iterator begin() { return first; }
	iterator end() { return NULL; }
	riterator rbegin() { return last; }
	riterator rend() { return NULL; }

	bool empty() { assert(first != NULL || first == last); return !first; }
	unsigned count();

	// used with node containers that represent shceduling queues
	// ignores copies and takes into account alu_packed_node items
	unsigned real_alu_count();

	void push_back(node *n);
	void push_front(node *n);

	void insert_node_before(node *s, node *n);
	void insert_node_after(node *s, node *n);

	void append_from(container_node *c);

	// remove range [b..e) from some container and assign to this container
	void move(iterator b, iterator e);

	void expand();
	void expand(container_node *n);
	void remove_node(node *n);

	node *cut(iterator b, iterator e);

	void clear() { first = last = NULL; }

	virtual bool is_valid() { return true; }
	virtual bool accept(vpass &p, bool enter);
	virtual bool fold_dispatch(expr_handler *ex);

	node* front() { return first; }
	node* back() { return last; }

	void collect_stats(node_stats &s);

	friend class shader;


};

typedef container_node::iterator node_iterator;
typedef container_node::riterator node_riterator;

class alu_group_node : public container_node {
protected:
	alu_group_node() : container_node(NT_LIST, NST_ALU_GROUP), literals() {}
public:

	std::vector<literal> literals;

	virtual bool is_valid() { return subtype == NST_ALU_GROUP; }
	virtual bool accept(vpass &p, bool enter);


	unsigned literal_chan(literal l) {
		std::vector<literal>::iterator F =
				std::find(literals.begin(), literals.end(), l);
		assert(F != literals.end());
		return F - literals.begin();
	}

	friend class shader;
};

class cf_node : public container_node {
protected:
	cf_node() : container_node(NT_OP, NST_CF_INST), jump_target(),
		jump_after_target() { memset(&bc, 0, sizeof(bc_cf)); };
public:
	bc_cf bc;

	cf_node *jump_target;
	bool jump_after_target;

	virtual bool is_valid() { return subtype == NST_CF_INST; }
	virtual bool accept(vpass &p, bool enter);
	virtual bool fold_dispatch(expr_handler *ex);

	void jump(cf_node *c) { jump_target = c; jump_after_target = false; }
	void jump_after(cf_node *c) { jump_target = c; jump_after_target = true; }

	friend class shader;
};

class alu_node : public node {
protected:
	alu_node() : node(NT_OP, NST_ALU_INST) { memset(&bc, 0, sizeof(bc_alu)); };
public:
	bc_alu bc;

	virtual bool is_valid() { return subtype == NST_ALU_INST; }
	virtual bool accept(vpass &p, bool enter);
	virtual bool fold_dispatch(expr_handler *ex);

	unsigned forced_bank_swizzle() {
		return ((bc.op_ptr->flags & AF_INTERP) && (bc.slot_flags == AF_4V)) ?
				VEC_210 : 0;
	}

	// return param index + 1 if instruction references interpolation param,
	// otherwise 0
	unsigned interp_param();

	alu_group_node *get_alu_group_node();

	friend class shader;
};

// for multi-slot instrs - DOT/INTERP/... (maybe useful for 64bit pairs later)
class alu_packed_node : public container_node {
protected:
	alu_packed_node() : container_node(NT_OP, NST_ALU_PACKED_INST) {}
public:

	const alu_op_info* op_ptr() {
		return static_cast<alu_node*>(first)->bc.op_ptr;
	}
	unsigned op() { return static_cast<alu_node*>(first)->bc.op; }
	void init_args(bool repl);

	virtual bool is_valid() { return subtype == NST_ALU_PACKED_INST; }
	virtual bool accept(vpass &p, bool enter);
	virtual bool fold_dispatch(expr_handler *ex);

	unsigned get_slot_mask();
	void update_packed_items(sb_context &ctx);

	friend class shader;
};

class fetch_node : public node {
protected:
	fetch_node() : node(NT_OP, NST_FETCH_INST) { memset(&bc, 0, sizeof(bc_fetch)); };
public:
	bc_fetch bc;

	virtual bool is_valid() { return subtype == NST_FETCH_INST; }
	virtual bool accept(vpass &p, bool enter);
	virtual bool fold_dispatch(expr_handler *ex);

	bool uses_grad() { return bc.op_ptr->flags & FF_USEGRAD; }

	friend class shader;
};

class region_node;

class repeat_node : public container_node {
protected:
	repeat_node(region_node *target, unsigned id)
	: container_node(NT_REPEAT, NST_LIST), target(target), rep_id(id) {}
public:
	region_node *target;
	unsigned rep_id;

	virtual bool accept(vpass &p, bool enter);

	friend class shader;
};

class depart_node : public container_node {
protected:
	depart_node(region_node *target, unsigned id)
	: container_node(NT_DEPART, NST_LIST), target(target), dep_id(id) {}
public:
	region_node *target;
	unsigned dep_id;

	virtual bool accept(vpass &p, bool enter);

	friend class shader;
};

class if_node : public container_node {
protected:
	if_node() : container_node(NT_IF, NST_LIST), cond() {};
public:
	value *cond; // glued to pseudo output (dst[2]) of the PRED_SETxxx

	virtual bool accept(vpass &p, bool enter);

	friend class shader;
};

typedef std::vector<depart_node*> depart_vec;
typedef std::vector<repeat_node*> repeat_vec;

class region_node : public container_node {
protected:
	region_node(unsigned id) : container_node(NT_REGION, NST_LIST), region_id(id),
			loop_phi(), phi(), vars_defined(), departs(), repeats() {}
public:
	unsigned region_id;

	container_node *loop_phi;
	container_node *phi;

	val_set vars_defined;

	depart_vec departs;
	repeat_vec repeats;

	virtual bool accept(vpass &p, bool enter);

	unsigned dep_count() { return departs.size(); }
	unsigned rep_count() { return repeats.size() + 1; }

	bool is_loop() { return !repeats.empty(); }

	container_node* get_entry_code_location() {
		node *p = first;
		while (p && (p->is_depart() || p->is_repeat()))
			p = static_cast<container_node*>(p)->first;

		container_node *c = static_cast<container_node*>(p);
		if (c->is_bb())
			return c;
		else
			return c->parent;
	}

	void expand_depart(depart_node *d);
	void expand_repeat(repeat_node *r);

	friend class shader;
};

class bb_node : public container_node {
protected:
	bb_node(unsigned id, unsigned loop_level)
		: container_node(NT_LIST, NST_BB), id(id), loop_level(loop_level) {}
public:
	unsigned id;
	unsigned loop_level;

	virtual bool accept(vpass &p, bool enter);

	friend class shader;
};


typedef std::vector<region_node*> regions_vec;
typedef std::vector<bb_node*> bbs_vec;
typedef std::list<node*> sched_queue;
typedef sched_queue::iterator sq_iterator;
typedef std::vector<node*> node_vec;
typedef std::list<node*> node_list;
typedef std::set<node*> node_set;



} // namespace r600_sb

#endif /* R600_SB_IR_H_ */
