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

#ifndef SB_SCHED_H_
#define SB_SCHED_H_

namespace r600_sb {

typedef sb_map<node*, unsigned> uc_map;

// resource trackers for scheduler

typedef sb_set<unsigned> kc_lines;

class rp_kcache_tracker {
	unsigned rp[4];
	unsigned uc[4];
	const unsigned sel_count;

	unsigned kc_sel(sel_chan r) {
		return sel_count == 4 ? (unsigned)r : ((r - 1) >> 1) + 1;
	}

public:
	rp_kcache_tracker(shader &sh);

	bool try_reserve(node *n);
	void unreserve(node *n);


	bool try_reserve(sel_chan r);
	void unreserve(sel_chan r);

	void reset();

	unsigned num_sels() { return !!rp[0] + !!rp[1] + !!rp[2] + !!rp[3]; }

	unsigned get_lines(kc_lines &lines);
};

class literal_tracker {
	literal lt[4];
	unsigned uc[4];
public:
	literal_tracker() : lt(), uc() {}

	bool try_reserve(alu_node *n);
	void unreserve(alu_node *n);

	bool try_reserve(literal l);
	void unreserve(literal l);

	void reset();

	unsigned count() { return !!uc[0] + !!uc[1] + !!uc[2] + !!uc[3]; }

	void init_group_literals(alu_group_node *g);

};

class rp_gpr_tracker {
	// rp[cycle][elem]
	unsigned rp[3][4];
	unsigned uc[3][4];

public:
	rp_gpr_tracker() : rp(), uc() {}

	bool try_reserve(alu_node *n);
	void unreserve(alu_node *n);

	bool try_reserve(unsigned cycle, unsigned sel, unsigned chan);
	void unreserve(unsigned cycle, unsigned sel, unsigned chan);

	void reset();

	void dump();
};

class alu_group_tracker {

	shader &sh;

	rp_kcache_tracker kc;
	rp_gpr_tracker gpr;
	literal_tracker lt;

	alu_node * slots[5];

	unsigned available_slots;

	unsigned max_slots;

	typedef std::map<value*, unsigned> value_index_map;

	value_index_map vmap;

	bool has_mova;
	bool uses_ar;
	bool has_predset;
	bool has_kill;
	bool updates_exec_mask;

	unsigned chan_count[4];

	// param index + 1 (0 means that group doesn't refer to Params)
	// we can't use more than one param index in a group
	unsigned interp_param;

	unsigned next_id;

	node_vec packed_ops;

	void assign_slot(unsigned slot, alu_node *n);

public:
	alu_group_tracker(shader &sh);

	// FIXME use fast bs correctness check (values for same chan <= 3) ??
	bool try_reserve(alu_node *n);
	bool try_reserve(alu_packed_node *p);

	void reinit();
	void reset(bool keep_packed = false);

	sel_chan get_value_id(value *v);
	void update_flags(alu_node *n);

	alu_node* slot(unsigned i) { return slots[i]; }

	unsigned used_slots() {
		return (~available_slots) & ((1 << max_slots) - 1);
	}

	unsigned inst_count() {
		return __builtin_popcount(used_slots());
	}

	unsigned literal_count() { return lt.count(); }
	unsigned literal_slot_count() { return (literal_count() + 1) >> 1; };
	unsigned slot_count() { return inst_count() + literal_slot_count(); }

	alu_group_node* emit();

	rp_kcache_tracker& kcache() { return kc; }

	bool has_update_exec_mask() { return updates_exec_mask; }
	unsigned avail_slots() { return available_slots; }

	void discard_all_slots(container_node &removed_nodes);
	void discard_slots(unsigned slot_mask, container_node &removed_nodes);

	bool has_ar_load() { return has_mova; }
};

class alu_kcache_tracker {
	bc_kcache kc[4];
	sb_set<unsigned> lines;
	unsigned max_kcs;

public:

	alu_kcache_tracker(sb_hw_class hc)
		: kc(), lines(), max_kcs(hc >= HW_CLASS_EVERGREEN ? 4 : 2) {}

	void reset();
	bool try_reserve(alu_group_tracker &gt);
	bool update_kc();
	void init_clause(bc_cf &bc) {
		memcpy(bc.kc, kc, sizeof(kc));
	}
};

class alu_clause_tracker {
	shader &sh;

	alu_kcache_tracker kt;
	unsigned slot_count;

	alu_group_tracker grp0;
	alu_group_tracker grp1;

	unsigned group;

	cf_node *clause;

	bool push_exec_mask;

public:
	container_node conflict_nodes;

	// current values of AR and PR registers that we have to preload
	// till the end of clause (in fact, beginning, because we're scheduling
	// bottom-up)
	value *current_ar;
	value *current_pr;

	alu_clause_tracker(shader &sh);

	void reset();

	// current group
	alu_group_tracker& grp() { return group ? grp1 : grp0; }
	// previous group
	alu_group_tracker& prev_grp() { return group ? grp0 : grp1; }

	void emit_group();
	void emit_clause(container_node *c);
	bool check_clause_limits();
	void new_group();
	bool is_empty();

	alu_node* create_ar_load();

	void discard_current_group();

	unsigned total_slots() { return slot_count; }
};

class post_scheduler : public pass {

	container_node ready, ready_copies; // alu only
	container_node pending, bb_pending;
	bb_node *cur_bb;
	val_set live; // values live at the end of the alu clause
	uc_map ucm;
	alu_clause_tracker alu;

	typedef std::map<sel_chan, value*> rv_map;
	rv_map regmap, prev_regmap;

	val_set cleared_interf;

public:

	post_scheduler(shader &sh) : pass(sh),
		ready(), ready_copies(), pending(), cur_bb(),
		live(), ucm(), alu(sh),	regmap(), cleared_interf() {}

	virtual int run();
	void run_on(container_node *n);
	void schedule_bb(bb_node *bb);

	void process_alu(container_node *c);
	void schedule_alu(container_node *c);
	bool prepare_alu_group();

	void release_op(node *n);

	void release_src_values(node *n);
	void release_src_vec(vvec &vv, bool src);
	void release_src_val(value *v);

	void init_uc_val(container_node *c, value *v);
	void init_uc_vec(container_node *c, vvec &vv, bool src);
	unsigned init_ucm(container_node *c, node *n);

	void init_regmap();

	bool check_interferences();

	unsigned try_add_instruction(node *n);

	bool check_copy(node *n);
	void dump_group(alu_group_tracker &rt);

	bool unmap_dst(alu_node *n);
	bool unmap_dst_val(value *d);

	bool map_src(alu_node *n);
	bool map_src_vec(vvec &vv, bool src);
	bool map_src_val(value *v);

	bool recolor_local(value *v);

	void update_local_interferences();
	void update_live_src_vec(vvec &vv, val_set *born, bool src);
	void update_live_dst_vec(vvec &vv);
	void update_live(node *n, val_set *born);
	void process_group();

	void set_color_local_val(value *v, sel_chan color);
	void set_color_local(value *v, sel_chan color);

	void add_interferences(value *v, sb_bitset &rb, val_set &vs);

	void init_globals(val_set &s, bool prealloc);

	void recolor_locals();

	void dump_regmap();

	void emit_load_ar();
	void emit_clause();

	void process_ready_copies();
};

} // namespace r600_sb

#endif /* SB_SCHED_H_ */
