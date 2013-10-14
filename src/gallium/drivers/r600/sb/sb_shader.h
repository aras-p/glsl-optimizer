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

#ifndef SB_SHADER_H_
#define SB_SHADER_H_

#include <list>
#include <string>
#include <map>

#include "sb_ir.h"
#include "sb_expr.h"

namespace r600_sb {

struct shader_input {
	unsigned comp_mask;
	unsigned preloaded;
};

struct error_info {
	node *n;
	unsigned arg_index;
	std::string message;
};

typedef std::multimap<node*, error_info> error_map;

class sb_context;

typedef std::vector<shader_input> inputs_vec;
typedef std::vector<gpr_array*> gpr_array_vec;

struct ra_edge {
	value *a, *b;
	unsigned cost;

	ra_edge(value *a, value *b, unsigned cost) : a(a), b(b), cost(cost) {}
};

enum chunk_flags {
	RCF_GLOBAL = (1 << 0),
	RCF_PIN_CHAN = (1 << 1),
	RCF_PIN_REG = (1 << 2),

	RCF_FIXED = (1 << 3),

	RCF_PREALLOC = (1 << 4)
};

enum dce_flags {
	DF_REMOVE_DEAD  = (1 << 0),
	DF_REMOVE_UNUSED = (1 << 1),
	DF_EXPAND = (1 << 2),
};

inline dce_flags operator |(dce_flags l, dce_flags r) {
	return (dce_flags)((unsigned)l|(unsigned)r);
}

inline chunk_flags operator |(chunk_flags l, chunk_flags r) {
	return (chunk_flags)((unsigned)l|(unsigned)r);
}
inline chunk_flags& operator |=(chunk_flags &l, chunk_flags r) {
	l = l | r;
	return l;
}

inline chunk_flags& operator &=(chunk_flags &l, chunk_flags r) {
	l = (chunk_flags)((unsigned)l & (unsigned)r);
	return l;
}

inline chunk_flags operator ~(chunk_flags r) {
	return (chunk_flags)~(unsigned)r;
}

struct ra_chunk {
	vvec values;
	chunk_flags flags;
	unsigned cost;
	sel_chan pin;

	ra_chunk() : values(), flags(), cost(), pin() {}

	bool is_fixed() { return flags & RCF_FIXED; }
	void fix() { flags |= RCF_FIXED; }

	bool is_global() { return flags & RCF_GLOBAL; }
	void set_global() {	flags |= RCF_GLOBAL; }

	bool is_reg_pinned() { return flags & RCF_PIN_REG; }
	bool is_chan_pinned() { return flags & RCF_PIN_CHAN; }

	bool is_prealloc() { return flags & RCF_PREALLOC; }
	void set_prealloc() { flags |= RCF_PREALLOC; }
};

typedef std::vector<ra_chunk*> chunk_vector;

class ra_constraint {
public:
	ra_constraint(constraint_kind kind) : kind(kind), cost(0) {}

	constraint_kind kind;
	vvec values;
	unsigned cost;

	void update_values();
	bool check();
};

typedef std::vector<ra_constraint*> constraint_vec;
typedef std::vector<ra_chunk*> chunk_vec;

// priority queue
// FIXME use something more suitale or custom class ?

template <class T>
struct cost_compare {
	bool operator ()(const T& t1, const T& t2) {
		return t1->cost > t2->cost;
	}
};

template <class T, class Comp>
class queue {
	typedef std::vector<T> container;
	container cont;

public:
	queue() : cont() {}

	typedef typename container::iterator iterator;

	iterator begin() { return cont.begin(); }
	iterator end() { return cont.end(); }

	iterator insert(const T& t) {
		iterator I = std::upper_bound(begin(), end(), t, Comp());
		if (I == end())
			cont.push_back(t);
		else
			cont.insert(I, t);

		return I;
	}

	void erase(const T& t) {
		std::pair<iterator, iterator> R =
				std::equal_range(begin(), end(), t, Comp());
		iterator F = std::find(R.first, R.second, t);
		if (F != R.second)
			cont.erase(F);
	}
};

typedef queue<ra_chunk*, cost_compare<ra_chunk*> > chunk_queue;
typedef queue<ra_edge*, cost_compare<ra_edge*> > edge_queue;
typedef queue<ra_constraint*, cost_compare<ra_constraint*> > constraint_queue;

typedef std::set<ra_chunk*> chunk_set;

class shader;

class coalescer {

	shader &sh;

	edge_queue edges;
	chunk_queue chunks;
	constraint_queue constraints;

	constraint_vec all_constraints;
	chunk_vec all_chunks;

public:

	coalescer(shader &sh) : sh(sh), edges(), chunks(), constraints() {}
	~coalescer();

	int run();

	void add_edge(value *a, value *b, unsigned cost);
	void build_chunks();
	void build_constraint_queue();
	void build_chunk_queue();
	int color_constraints();
	void color_chunks();

	ra_constraint* create_constraint(constraint_kind kind);

	enum ac_cost {
		phi_cost = 10000,
		copy_cost = 1,
	};

	void dump_edges();
	void dump_chunks();
	void dump_constraint_queue();

	static void dump_chunk(ra_chunk *c);
	static void dump_constraint(ra_constraint* c);

	void get_chunk_interferences(ra_chunk *c, val_set &s);

private:

	void create_chunk(value *v);
	void unify_chunks(ra_edge *e);
	bool chunks_interference(ra_chunk *c1, ra_chunk *c2);

	int color_reg_constraint(ra_constraint *c);
	void color_phi_constraint(ra_constraint *c);


	void init_reg_bitset(sb_bitset &bs, val_set &vs);

	void color_chunk(ra_chunk *c, sel_chan color);

	ra_chunk* detach_value(value *v);
};



class shader {

	sb_context &ctx;

	typedef sb_map<uint32_t, value*> value_map;
	value_map reg_values;

	// read-only values
	value_map const_values; // immediate constants key -const  value (uint32_t)
	value_map special_ro_values; //  key - hw alu_sel & chan
	value_map kcache_values;

	gpr_array_vec gpr_arrays;

	unsigned next_temp_value_index;

	unsigned prep_regs_count;

	value* pred_sels[2];

	regions_vec regions;
	inputs_vec inputs;

	value *undef;

	sb_value_pool val_pool;
	sb_pool pool;

	std::vector<node*> all_nodes;

public:
	shader_stats src_stats, opt_stats;

	error_map errors;

	bool optimized;

	unsigned id;

	coalescer coal;

	static const unsigned temp_regid_offset = 512;

	bbs_vec bbs;

	const shader_target target;

	value_table vt;
	expr_handler ex;

	container_node *root;

	bool compute_interferences;

	bool has_alu_predication;
	bool uses_gradients;

	bool safe_math;

	unsigned ngpr, nstack;

	unsigned dce_flags;

	shader(sb_context &sctx, shader_target t, unsigned id);

	~shader();

	sb_context &get_ctx() const { return ctx; }

	value* get_const_value(const literal & v);
	value* get_special_value(unsigned sv_id, unsigned version = 0);
	value* create_temp_value();
	value* get_gpr_value(bool src, unsigned reg, unsigned chan, bool rel,
                         unsigned version = 0);


	value* get_special_ro_value(unsigned sel);
	value* get_kcache_value(unsigned bank, unsigned index, unsigned chan);

	value* get_value_version(value* v, unsigned ver);

	void init();
	void add_pinned_gpr_values(vvec& vec, unsigned gpr, unsigned comp_mask, bool src);

	void dump_ir();

	void add_gpr_array(unsigned gpr_start, unsigned gpr_count,
	                   unsigned comp_mask);

	value* get_pred_sel(int sel);
	bool assign_slot(alu_node *n, alu_node *slots[5]);

	gpr_array* get_gpr_array(unsigned reg, unsigned chan);

	void add_input(unsigned gpr, bool preloaded = false,
	               unsigned comp_mask = 0xF);

	const inputs_vec & get_inputs() {return inputs; }

	regions_vec & get_regions() { return regions; }

	void init_call_fs(cf_node *cf);

	value *get_undef_value();
	void set_undef(val_set &s);

	node* create_node(node_type nt, node_subtype nst,
	                  node_flags flags = NF_EMPTY);
	alu_node* create_alu();
	alu_group_node* create_alu_group();
	alu_packed_node* create_alu_packed();
	cf_node* create_cf();
	cf_node* create_cf(unsigned op);
	fetch_node* create_fetch();
	region_node* create_region();
	depart_node* create_depart(region_node *target);
	repeat_node* create_repeat(region_node *target);
	container_node* create_container(node_type nt = NT_LIST,
	                                 node_subtype nst = NST_LIST,
	                                 node_flags flags = NF_EMPTY);
	if_node* create_if();
	bb_node* create_bb(unsigned id, unsigned loop_level);

	value* get_value_by_uid(unsigned id) { return val_pool[id - 1]; }

	cf_node* create_clause(node_subtype nst);

	void create_bbs();
	void expand_bbs();

	alu_node* create_mov(value* dst, value* src);
	alu_node* create_copy_mov(value *dst, value *src, unsigned affcost = 1);

	const char * get_shader_target_name();

	std::string get_full_target_name();

	void create_bbs(container_node* n, bbs_vec &bbs, int loop_level = 0);
	void expand_bbs(bbs_vec &bbs);

	sched_queue_id get_queue_id(node* n);

	void simplify_dep_rep(node *dr);

	unsigned first_temp_gpr();
	unsigned num_nontemp_gpr();

	gpr_array_vec& arrays() { return gpr_arrays; }

	void set_uses_kill();

	void fill_array_values(gpr_array *a, vvec &vv);

	alu_node* clone(alu_node *n);

	sb_value_pool& get_value_pool() { return val_pool; }

	void collect_stats(bool opt);

private:
	value* create_value(value_kind k, sel_chan regid, unsigned ver);
	value* get_value(value_kind kind, sel_chan id,
	                         unsigned version = 0);
	value* get_ro_value(value_map &vm, value_kind vk, unsigned key);
};

}

#endif /* SHADER_H_ */
