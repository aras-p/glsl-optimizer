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

#ifndef SB_PASS_H_
#define SB_PASS_H_

#include <stack>

namespace r600_sb {

class pass {
protected:
	sb_context &ctx;
	shader &sh;

public:
	pass(shader &s);

	virtual int run();

	virtual ~pass() {}
};

class vpass : public pass {

public:

	vpass(shader &s) : pass(s) {}

	virtual int init();
	virtual int done();

	virtual int run();
	virtual void run_on(container_node &n);

	virtual bool visit(node &n, bool enter);
	virtual bool visit(container_node &n, bool enter);
	virtual bool visit(alu_group_node &n, bool enter);
	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(alu_packed_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);
	virtual bool visit(region_node &n, bool enter);
	virtual bool visit(repeat_node &n, bool enter);
	virtual bool visit(depart_node &n, bool enter);
	virtual bool visit(if_node &n, bool enter);
	virtual bool visit(bb_node &n, bool enter);

};

class rev_vpass : public vpass {

public:
	rev_vpass(shader &s) : vpass(s) {}

	virtual void run_on(container_node &n);
};


// =================== PASSES

class bytecode;

class bc_dump : public vpass {
	using vpass::visit;

	uint32_t *bc_data;
	unsigned ndw;

	unsigned id;

	unsigned new_group, group_index;

public:

	bc_dump(shader &s, bytecode *bc = NULL);

	bc_dump(shader &s, uint32_t *bc_ptr, unsigned ndw) :
		vpass(s), bc_data(bc_ptr), ndw(ndw), id(), new_group(), group_index() {}

	virtual int init();
	virtual int done();

	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);

	void dump_dw(unsigned dw_id, unsigned count = 2);

	void dump(cf_node& n);
	void dump(alu_node& n);
	void dump(fetch_node& n);
};


class dce_cleanup : public vpass {
	using vpass::visit;

	bool remove_unused;

public:

	dce_cleanup(shader &s) : vpass(s),
		remove_unused(s.dce_flags & DF_REMOVE_UNUSED) {}

	virtual bool visit(node &n, bool enter);
	virtual bool visit(alu_group_node &n, bool enter);
	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(alu_packed_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);
	virtual bool visit(region_node &n, bool enter);
	virtual bool visit(container_node &n, bool enter);

private:

	void cleanup_dst(node &n);
	bool cleanup_dst_vec(vvec &vv);

};


class def_use : public pass {

public:

	def_use(shader &sh) : pass(sh) {}

	virtual int run();
	void run_on(node *n, bool defs);

private:

	void process_uses(node *n);
	void process_defs(node *n, vvec &vv, bool arr_def);
	void process_phi(container_node *c, bool defs, bool uses);
};



class dump : public vpass {
	using vpass::visit;

	int level;

public:

	dump(shader &s) : vpass(s), level(0) {}

	virtual bool visit(node &n, bool enter);
	virtual bool visit(container_node &n, bool enter);
	virtual bool visit(alu_group_node &n, bool enter);
	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(alu_packed_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);
	virtual bool visit(region_node &n, bool enter);
	virtual bool visit(repeat_node &n, bool enter);
	virtual bool visit(depart_node &n, bool enter);
	virtual bool visit(if_node &n, bool enter);
	virtual bool visit(bb_node &n, bool enter);


	static void dump_op(node &n, const char *name);
	static void dump_vec(const vvec & vv);
	static void dump_set(shader &sh, val_set & v);

	static void dump_rels(vvec & vv);

	static void dump_val(value *v);
	static void dump_op(node *n);

	static void dump_op_list(container_node *c);
	static void dump_queue(sched_queue &q);

	static void dump_alu(alu_node *n);

private:

	void indent();

	void dump_common(node &n);
	void dump_flags(node &n);

	void dump_live_values(container_node &n, bool before);
};


// Global Code Motion

class gcm : public pass {

	sched_queue bu_ready[SQ_NUM];
	sched_queue bu_ready_next[SQ_NUM];
	sched_queue bu_ready_early[SQ_NUM];
	sched_queue ready;
	sched_queue ready_above;

	container_node pending;

	struct op_info {
		bb_node* top_bb;
		bb_node* bottom_bb;
		op_info() : top_bb(), bottom_bb() {}
	};

	typedef std::map<node*, op_info> op_info_map;

	typedef std::map<node*, unsigned> nuc_map;

	op_info_map op_map;
	nuc_map uses;

	typedef std::vector<nuc_map> nuc_stack;

	nuc_stack nuc_stk;
	unsigned ucs_level;

	bb_node * bu_bb;

	vvec pending_defs;

	node_list pending_nodes;

	unsigned cur_sq;

	// for register pressure tracking in bottom-up pass
	val_set live;
	int live_count;

	static const int rp_threshold = 100;

	bool pending_exec_mask_update;

public:

	gcm(shader &sh) : pass(sh),
		bu_ready(), bu_ready_next(), bu_ready_early(),
		ready(), op_map(), uses(), nuc_stk(1), ucs_level(),
		bu_bb(), pending_defs(), pending_nodes(), cur_sq(),
		live(), live_count(), pending_exec_mask_update() {}

	virtual int run();

private:

	void collect_instructions(container_node *c, bool early_pass);

	void sched_early(container_node *n);
	void td_sched_bb(bb_node *bb);
	bool td_is_ready(node *n);
	void td_release_uses(vvec &v);
	void td_release_val(value *v);
	void td_schedule(bb_node *bb, node *n);

	void sched_late(container_node *n);
	void bu_sched_bb(bb_node *bb);
	void bu_release_defs(vvec &v, bool src);
	void bu_release_phi_defs(container_node *p, unsigned op);
	bool bu_is_ready(node *n);
	void bu_release_val(value *v);
	void bu_release_op(node * n);
	void bu_find_best_bb(node *n, op_info &oi);
	void bu_schedule(container_node *bb, node *n);

	void push_uc_stack();
	void pop_uc_stack();

	void init_def_count(nuc_map &m, container_node &s);
	void init_use_count(nuc_map &m, container_node &s);
	unsigned get_uc_vec(vvec &vv);
	unsigned get_dc_vec(vvec &vv, bool src);

	void add_ready(node *n);

	void dump_uc_stack();

	unsigned real_alu_count(sched_queue &q, unsigned max);

	// check if we have not less than threshold ready alu instructions
	bool check_alu_ready_count(unsigned threshold);
};


class gvn : public vpass {
	using vpass::visit;

public:

	gvn(shader &sh) : vpass(sh) {}

	virtual bool visit(node &n, bool enter);
	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(alu_packed_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);
	virtual bool visit(region_node &n, bool enter);

private:

	void process_op(node &n, bool rewrite = true);

	// returns true if the value was rewritten
	bool process_src(value* &v, bool rewrite);


	void process_alu_src_constants(node &n, value* &v);
};


class if_conversion : public pass {

public:

	if_conversion(shader &sh) : pass(sh) {}

	virtual int run();

	bool run_on(region_node *r);

	void convert_kill_instructions(region_node *r, value *em, bool branch,
	                               container_node *c);

	bool check_and_convert(region_node *r);

	alu_node* convert_phi(value *select, node *phi);

};


class liveness : public rev_vpass {
	using vpass::visit;

	val_set live;
	bool live_changed;

public:

	liveness(shader &s) : rev_vpass(s), live_changed(false) {}

	virtual int init();

	virtual bool visit(node &n, bool enter);
	virtual bool visit(bb_node &n, bool enter);
	virtual bool visit(container_node &n, bool enter);
	virtual bool visit(alu_group_node &n, bool enter);
	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(alu_packed_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);
	virtual bool visit(region_node &n, bool enter);
	virtual bool visit(repeat_node &n, bool enter);
	virtual bool visit(depart_node &n, bool enter);
	virtual bool visit(if_node &n, bool enter);

private:

	void update_interferences();
	void process_op(node &n);

	bool remove_val(value *v);
	bool remove_vec(vvec &v);
	bool process_outs(node& n);
	void process_ins(node& n);

	void process_phi_outs(container_node *phi);
	void process_phi_branch(container_node *phi, unsigned id);

	bool process_maydef(value *v);

	bool add_vec(vvec &vv, bool src);

	void update_src_vec(vvec &vv, bool src);
};


struct bool_op_info {
	bool invert;
	unsigned int_cvt;

	alu_node *n;
};

class peephole : public pass {

public:

	peephole(shader &sh) : pass(sh) {}

	virtual int run();

	void run_on(container_node *c);

	void optimize_cc_op(alu_node *a);

	void optimize_cc_op2(alu_node *a);
	void optimize_CNDcc_op(alu_node *a);

	bool get_bool_op_info(value *b, bool_op_info& bop);
	bool get_bool_flt_to_int_source(alu_node* &a);
	void convert_float_setcc(alu_node *f2i, alu_node *s);
};


class psi_ops : public rev_vpass {
	using rev_vpass::visit;

public:

	psi_ops(shader &s) : rev_vpass(s) {}

	virtual bool visit(node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);

	bool try_inline(node &n);
	bool try_reduce(node &n);
	bool eliminate(node &n);

	void unpredicate(node *n);
};


// check correctness of the generated code, e.g.:
// - expected source operand value is the last value written to its gpr,
// - all arguments of phi node should be allocated to the same gpr,
// TODO other tests
class ra_checker : public pass {

	typedef std::map<sel_chan, value *> reg_value_map;

	typedef std::vector<reg_value_map> regmap_stack;

	regmap_stack rm_stack;
	unsigned rm_stk_level;

	value* prev_dst[5];

public:

	ra_checker(shader &sh) : pass(sh), rm_stk_level(0), prev_dst() {}

	virtual int run();

	void run_on(container_node *c);

	void dump_error(const error_info &e);
	void dump_all_errors();

private:

	reg_value_map& rmap() { return rm_stack[rm_stk_level]; }

	void push_stack();
	void pop_stack();

	// when going out of the alu clause, values in the clause temporary gprs,
	// AR, predicate values, PS/PV are destroyed
	void kill_alu_only_regs();
	void error(node *n, unsigned id, std::string msg);

	void check_phi_src(container_node *p, unsigned id);
	void process_phi_dst(container_node *p);
	void check_alu_group(alu_group_node *g);
	void process_op_dst(node *n);
	void check_op_src(node *n);
	void check_src_vec(node *n, unsigned id, vvec &vv, bool src);
	void check_value_gpr(node *n, unsigned id, value *v);
};

// =======================================


class ra_coalesce : public pass {

public:

	ra_coalesce(shader &sh) : pass(sh) {}

	virtual int run();
};



// =======================================

class ra_init : public pass {

public:

	ra_init(shader &sh) : pass(sh), prev_chans() {

		// The parameter below affects register channels distribution.
		// For cayman (VLIW-4) we're trying to distribute the channels
		// uniformly, this means significantly better alu slots utilization
		// at the expense of higher gpr usage. Hopefully this will improve
		// performance, though it has to be proven with real benchmarks yet.
		// For VLIW-5 this method could also slightly improve slots
		// utilization, but increased register pressure seems more significant
		// and overall performance effect is negative according to some
		// benchmarks, so it's not used currently. Basically, VLIW-5 doesn't
		// really need it because trans slot (unrestricted by register write
		// channel) allows to consume most deviations from uniform channel
		// distribution.
		// Value 3 means that for new allocation we'll use channel that differs
		// from 3 last used channels. 0 for VLIW-5 effectively turns this off.

		ra_tune = sh.get_ctx().is_cayman() ? 3 : 0;
	}

	virtual int run();

private:

	unsigned prev_chans;
	unsigned ra_tune;

	void add_prev_chan(unsigned chan);
	unsigned get_preferable_chan_mask();

	void ra_node(container_node *c);
	void process_op(node *n);

	void color(value *v);

	void color_bs_constraint(ra_constraint *c);

	void assign_color(value *v, sel_chan c);
	void alloc_arrays();
};

// =======================================

class ra_split : public pass {

public:

	ra_split(shader &sh) : pass(sh) {}

	virtual int run();

	void split(container_node *n);
	void split_op(node *n);
	void split_alu_packed(alu_packed_node *n);
	void split_vector_inst(node *n);

	void split_packed_ins(alu_packed_node *n);

#if 0
	void split_pinned_outs(node *n);
#endif

	void split_vec(vvec &vv, vvec &v1, vvec &v2, bool allow_swz);

	void split_phi_src(container_node *loc, container_node *c, unsigned id,
	                   bool loop);
	void split_phi_dst(node *loc, container_node *c, bool loop);
	void init_phi_constraints(container_node *c);
};



class ssa_prepare : public vpass {
	using vpass::visit;

	typedef std::vector<val_set> vd_stk;
	vd_stk stk;

	unsigned level;

public:
	ssa_prepare(shader &s) : vpass(s), level(0) {}

	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);
	virtual bool visit(region_node &n, bool enter);
	virtual bool visit(repeat_node &n, bool enter);
	virtual bool visit(depart_node &n, bool enter);

private:

	void push_stk() {
		++level;
		if (level + 1 > stk.size())
			stk.resize(level+1);
		else
			stk[level].clear();
	}
	void pop_stk() {
		assert(level);
		--level;
		stk[level].add_set(stk[level + 1]);
	}

	void add_defs(node &n);

	val_set & cur_set() { return stk[level]; }

	container_node* create_phi_nodes(int count);
};

class ssa_rename : public vpass {
	using vpass::visit;

	typedef sb_map<value*, unsigned> def_map;

	def_map def_count;
	std::stack<def_map> rename_stack;

	typedef std::map<uint32_t, value*> val_map;
	val_map values;

public:

	ssa_rename(shader &s) : vpass(s) {}

	virtual int init();

	virtual bool visit(container_node &n, bool enter);
	virtual bool visit(node &n, bool enter);
	virtual bool visit(alu_group_node &n, bool enter);
	virtual bool visit(cf_node &n, bool enter);
	virtual bool visit(alu_node &n, bool enter);
	virtual bool visit(alu_packed_node &n, bool enter);
	virtual bool visit(fetch_node &n, bool enter);
	virtual bool visit(region_node &n, bool enter);
	virtual bool visit(repeat_node &n, bool enter);
	virtual bool visit(depart_node &n, bool enter);
	virtual bool visit(if_node &n, bool enter);

private:

	void push(node *phi);
	void pop();

	unsigned get_index(def_map& m, value* v);
	void set_index(def_map& m, value* v, unsigned index);
	unsigned new_index(def_map& m, value* v);

	value* rename_use(node *n, value* v);
	value* rename_def(node *def, value* v);

	void rename_src_vec(node *n, vvec &vv, bool src);
	void rename_dst_vec(node *def, vvec &vv, bool set_def);

	void rename_src(node *n);
	void rename_dst(node *n);

	void rename_phi_args(container_node *phi, unsigned op, bool def);

	void rename_virt(node *n);
	void rename_virt_val(node *n, value *v);
};

class bc_finalizer : public pass {

	cf_node *last_export[EXP_TYPE_COUNT];
	cf_node *last_cf;

	unsigned ngpr;
	unsigned nstack;

public:

	bc_finalizer(shader &sh) : pass(sh), last_export(), last_cf(), ngpr(),
		nstack() {}

	virtual int run();

	void finalize_loop(region_node *r);
	void finalize_if(region_node *r);

	void run_on(container_node *c);

	void finalize_alu_group(alu_group_node *g);
	void finalize_alu_src(alu_group_node *g, alu_node *a);

	void emit_set_grad(fetch_node* f);
	void finalize_fetch(fetch_node *f);

	void finalize_cf(cf_node *c);

	sel_chan translate_kcache(cf_node *alu, value *v);

	void update_ngpr(unsigned gpr);
	void update_nstack(region_node *r, unsigned add = 0);

	unsigned get_stack_depth(node *n, unsigned &loops, unsigned &ifs,
	                         unsigned add = 0);

	void cf_peephole();

};


} // namespace r600_sb

#endif /* SB_PASS_H_ */
