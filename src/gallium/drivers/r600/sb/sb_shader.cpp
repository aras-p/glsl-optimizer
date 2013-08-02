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

#include "sb_bc.h"
#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

shader::shader(sb_context &sctx, shader_target t, unsigned id)
: ctx(sctx), next_temp_value_index(temp_regid_offset),
  prep_regs_count(), pred_sels(),
  regions(), inputs(), undef(), val_pool(sizeof(value)),
  pool(), all_nodes(), src_stats(), opt_stats(), errors(),
  optimized(), id(id),
  coal(*this), bbs(),
  target(t), vt(ex), ex(*this), root(),
  compute_interferences(),
  has_alu_predication(),
  uses_gradients(), safe_math(), ngpr(), nstack(), dce_flags() {}

bool shader::assign_slot(alu_node* n, alu_node *slots[5]) {

	unsigned slot_flags = ctx.alu_slots(n->bc.op);
	unsigned slot = n->bc.dst_chan;

	if (!ctx.is_cayman() && (!(slot_flags & AF_V) || slots[slot]) &&
			(slot_flags & AF_S))
		slot = SLOT_TRANS;

	if (slots[slot])
		return false;

	n->bc.slot = slot;
	slots[slot] = n;
	return true;
}

void shader::add_pinned_gpr_values(vvec& vec, unsigned gpr, unsigned comp_mask,
                            bool src) {
	unsigned chan = 0;
	while (comp_mask) {
		if (comp_mask & 1) {
			value *v = get_gpr_value(src, gpr, chan, false);
			v->flags |= (VLF_PIN_REG | VLF_PIN_CHAN);
			if (!v->is_rel()) {
				v->gpr = v->pin_gpr = v->select;
				v->fix();
			}
			if (v->array && !v->array->gpr) {
				// if pinned value can be accessed with indirect addressing
				// pin the entire array to its original location
				v->array->gpr = v->array->base_gpr;
			}
			vec.push_back(v);
		}
		comp_mask >>= 1;
		++chan;
	}
}

cf_node* shader::create_clause(node_subtype nst) {
	cf_node *n = create_cf();

	n->subtype = nst;

	switch (nst) {
	case NST_ALU_CLAUSE: n->bc.set_op(CF_OP_ALU); break;
	case NST_TEX_CLAUSE: n->bc.set_op(CF_OP_TEX); break;
	case NST_VTX_CLAUSE: n->bc.set_op(CF_OP_VTX); break;
	default: assert(!"invalid clause type"); break;
	}

	n->bc.barrier = 1;
	return n;
}

void shader::create_bbs() {
	create_bbs(root, bbs);
}

void shader::expand_bbs() {
	expand_bbs(bbs);
}

alu_node* shader::create_mov(value* dst, value* src) {
	alu_node *n = create_alu();
	n->bc.set_op(ALU_OP1_MOV);
	n->dst.push_back(dst);
	n->src.push_back(src);
	dst->def = n;

	return n;
}

alu_node* shader::create_copy_mov(value* dst, value* src, unsigned affcost) {
	alu_node *n = create_mov(dst, src);

	dst->assign_source(src);
	n->flags |= NF_COPY_MOV | NF_DONT_HOIST;

	if (affcost && dst->is_sgpr() && src->is_sgpr())
		coal.add_edge(src, dst, affcost);

	return n;
}

value* shader::get_value(value_kind kind, sel_chan id,
                         unsigned version) {
	if (version == 0 && kind == VLK_REG && id.sel() < prep_regs_count)
		return val_pool[id - 1];


	unsigned key = (kind << 28) | (version << 16) | id;
	value_map::iterator i = reg_values.find(key);
	if (i != reg_values.end()) {
		return i->second;
	}
	value *v = create_value(kind, id, version);
	reg_values.insert(std::make_pair(key, v));
	return v;
}

value* shader::get_special_value(unsigned sv_id, unsigned version) {
	sel_chan id(sv_id, 0);
	return get_value(VLK_SPECIAL_REG, id, version);
}

void shader::fill_array_values(gpr_array *a, vvec &vv) {
	unsigned sz = a->array_size;
	vv.resize(sz);
	for (unsigned i = 0; i < a->array_size; ++i) {
		vv[i] = get_gpr_value(true, a->base_gpr.sel() + i, a->base_gpr.chan(),
		                      false);
	}
}

value* shader::get_gpr_value(bool src, unsigned reg, unsigned chan, bool rel,
                             unsigned version) {
	sel_chan id(reg, chan);
	value *v;
	gpr_array *a = get_gpr_array(reg, chan);
	if (rel) {
		assert(a);
		v = create_value(VLK_REL_REG, id, 0);
		v->rel = get_special_value(SV_AR_INDEX);
		fill_array_values(a, v->muse);
		if (!src)
			fill_array_values(a, v->mdef);
	} else {
		if (version == 0 && reg < prep_regs_count)
			return (val_pool[id - 1]);

		v = get_value(VLK_REG, id, version);
	}

	v->array = a;
	v->pin_gpr = v->select;

	return v;
}

value* shader::create_temp_value() {
	sel_chan id(++next_temp_value_index, 0);
	return get_value(VLK_TEMP, id, 0);
}

value* shader::get_kcache_value(unsigned bank, unsigned index, unsigned chan) {
	return get_ro_value(kcache_values, VLK_KCACHE,
			sel_chan((bank << 12) | index, chan));
}

void shader::add_input(unsigned gpr, bool preloaded, unsigned comp_mask) {
	if (inputs.size() <= gpr)
		inputs.resize(gpr+1);

	shader_input &i = inputs[gpr];
	i.preloaded = preloaded;
	i.comp_mask = comp_mask;

	if (preloaded) {
		add_pinned_gpr_values(root->dst, gpr, comp_mask, true);
	}

}

void shader::init() {
	assert(!root);
	root = create_container();
}

void shader::init_call_fs(cf_node* cf) {
	unsigned gpr = 0;

	assert(target == TARGET_VS || target == TARGET_ES);

	for(inputs_vec::const_iterator I = inputs.begin(),
			E = inputs.end(); I != E; ++I, ++gpr) {
		if (!I->preloaded)
			add_pinned_gpr_values(cf->dst, gpr, I->comp_mask, false);
		else
			add_pinned_gpr_values(cf->src, gpr, I->comp_mask, true);
	}
}

void shader::set_undef(val_set& s) {
	value *undefined = get_undef_value();
	if (!undefined->gvn_source)
		vt.add_value(undefined);

	val_set &vs = s;

	for (val_set::iterator I = vs.begin(*this), E = vs.end(*this); I != E; ++I) {
		value *v = *I;

		assert(!v->is_readonly() && !v->is_rel());

		v->gvn_source = undefined->gvn_source;
	}
}

value* shader::create_value(value_kind k, sel_chan regid, unsigned ver) {
	value *v = val_pool.create(k, regid, ver);
	return v;
}

value* shader::get_undef_value() {
	if (!undef)
		undef = create_value(VLK_UNDEF, 0, 0);
	return undef;
}

node* shader::create_node(node_type nt, node_subtype nst, node_flags flags) {
	node *n = new (pool.allocate(sizeof(node))) node(nt, nst, flags);
	all_nodes.push_back(n);
	return n;
}

alu_node* shader::create_alu() {
	alu_node* n = new (pool.allocate(sizeof(alu_node))) alu_node();
	all_nodes.push_back(n);
	return n;
}

alu_group_node* shader::create_alu_group() {
	alu_group_node* n =
			new (pool.allocate(sizeof(alu_group_node))) alu_group_node();
	all_nodes.push_back(n);
	return n;
}

alu_packed_node* shader::create_alu_packed() {
	alu_packed_node* n =
			new (pool.allocate(sizeof(alu_packed_node))) alu_packed_node();
	all_nodes.push_back(n);
	return n;
}

cf_node* shader::create_cf() {
	cf_node* n = new (pool.allocate(sizeof(cf_node))) cf_node();
	n->bc.barrier = 1;
	all_nodes.push_back(n);
	return n;
}

fetch_node* shader::create_fetch() {
	fetch_node* n = new (pool.allocate(sizeof(fetch_node))) fetch_node();
	all_nodes.push_back(n);
	return n;
}

region_node* shader::create_region() {
	region_node *n = new (pool.allocate(sizeof(region_node)))
			region_node(regions.size());
	regions.push_back(n);
	all_nodes.push_back(n);
	return n;
}

depart_node* shader::create_depart(region_node* target) {
	depart_node* n = new (pool.allocate(sizeof(depart_node)))
			depart_node(target, target->departs.size());
	target->departs.push_back(n);
	all_nodes.push_back(n);
	return n;
}

repeat_node* shader::create_repeat(region_node* target) {
	repeat_node* n = new (pool.allocate(sizeof(repeat_node)))
			repeat_node(target, target->repeats.size() + 1);
	target->repeats.push_back(n);
	all_nodes.push_back(n);
	return n;
}

container_node* shader::create_container(node_type nt, node_subtype nst,
		                                 node_flags flags) {
	container_node *n = new (pool.allocate(sizeof(container_node)))
			container_node(nt, nst, flags);
	all_nodes.push_back(n);
	return n;
}

if_node* shader::create_if() {
	if_node* n = new (pool.allocate(sizeof(if_node))) if_node();
	all_nodes.push_back(n);
	return n;
}

bb_node* shader::create_bb(unsigned id, unsigned loop_level) {
	bb_node* n = new (pool.allocate(sizeof(bb_node))) bb_node(id, loop_level);
	all_nodes.push_back(n);
	return n;
}

value* shader::get_special_ro_value(unsigned sel) {
	return get_ro_value(special_ro_values, VLK_PARAM, sel);
}

value* shader::get_const_value(const literal &v) {
	value *val = get_ro_value(const_values, VLK_CONST, v);
	val->literal_value = v;
	return val;
}

shader::~shader() {
	for (node_vec::iterator I = all_nodes.begin(), E = all_nodes.end();
			I != E; ++I)
		(*I)->~node();

	for (gpr_array_vec::iterator I = gpr_arrays.begin(), E = gpr_arrays.end();
			I != E; ++I) {
		delete *I;
	}
}

void shader::dump_ir() {
	if (ctx.dump_pass)
		dump(*this).run();
}

value* shader::get_value_version(value* v, unsigned ver) {
	assert(!v->is_readonly() && !v->is_rel());
	value *vv = get_value(v->kind, v->select, ver);
	assert(vv);

	if (v->array) {
		vv->array = v->array;
	}

	return vv;
}

gpr_array* shader::get_gpr_array(unsigned reg, unsigned chan) {

	for (regarray_vec::iterator I = gpr_arrays.begin(),
			E = gpr_arrays.end(); I != E; ++I) {
		gpr_array* a = *I;
		unsigned achan = a->base_gpr.chan();
		unsigned areg = a->base_gpr.sel();
		if (achan == chan && (reg >= areg && reg < areg+a->array_size))
			return a;
	}
	return NULL;
}

void shader::add_gpr_array(unsigned gpr_start, unsigned gpr_count,
					   unsigned comp_mask) {
	unsigned chan = 0;
	while (comp_mask) {
		if (comp_mask & 1) {
			gpr_array *a = new gpr_array(
					sel_chan(gpr_start, chan), gpr_count);

			SB_DUMP_PASS( sblog << "add_gpr_array: @" << a->base_gpr
			         << " [" << a->array_size << "]\n";
			);

			gpr_arrays.push_back(a);
		}
		comp_mask >>= 1;
		++chan;
	}
}

value* shader::get_pred_sel(int sel) {
	assert(sel == 0 || sel == 1);
	if (!pred_sels[sel])
		pred_sels[sel] = get_const_value(sel);

	return pred_sels[sel];
}

cf_node* shader::create_cf(unsigned op) {
	cf_node *c = create_cf();
	c->bc.set_op(op);
	c->bc.barrier = 1;
	return c;
}

std::string shader::get_full_target_name() {
	std::string s = get_shader_target_name();
	s += "/";
	s += ctx.get_hw_chip_name();
	s += "/";
	s += ctx.get_hw_class_name();
	return s;
}

const char* shader::get_shader_target_name() {
	switch (target) {
		case TARGET_VS: return "VS";
		case TARGET_ES: return "ES";
		case TARGET_PS: return "PS";
		case TARGET_GS: return "GS";
		case TARGET_COMPUTE: return "COMPUTE";
		case TARGET_FETCH: return "FETCH";
		default:
			return "INVALID_TARGET";
	}
}

void shader::simplify_dep_rep(node* dr) {
	container_node *p = dr->parent;
	if (p->is_repeat()) {
		repeat_node *r = static_cast<repeat_node*>(p);
		r->target->expand_repeat(r);
	} else if (p->is_depart()) {
		depart_node *d = static_cast<depart_node*>(p);
		d->target->expand_depart(d);
	}
	if (dr->next)
		dr->parent->cut(dr->next, NULL);
}


// FIXME this is used in some places as the max non-temp gpr,
// (MAX_GPR - 2 * ctx.alu_temp_gprs) should be used for that instead.
unsigned shader::first_temp_gpr() {
	return MAX_GPR - ctx.alu_temp_gprs;
}

unsigned shader::num_nontemp_gpr() {
	return MAX_GPR - 2 * ctx.alu_temp_gprs;
}

void shader::set_uses_kill() {
	if (root->src.empty())
		root->src.resize(1);

	if (!root->src[0])
		root->src[0] = get_special_value(SV_VALID_MASK);
}

alu_node* shader::clone(alu_node* n) {
	alu_node *c = create_alu();

	// FIXME: this may be wrong with indirect operands
	c->src = n->src;
	c->dst = n->dst;

	c->bc = n->bc;
	c->pred = n->pred;

	return c;
}

void shader::collect_stats(bool opt) {
	if (!sb_context::dump_stat)
		return;

	shader_stats &s = opt ? opt_stats : src_stats;

	s.shaders = 1;
	s.ngpr = ngpr;
	s.nstack = nstack;
	s.collect(root);

	if (opt)
		ctx.opt_stats.accumulate(s);
	else
		ctx.src_stats.accumulate(s);
}

value* shader::get_ro_value(value_map& vm, value_kind vk, unsigned key) {
	value_map::iterator I = vm.find(key);
	if (I != vm.end())
		return I->second;
	value *v = create_value(vk, key, 0);
	v->flags = VLF_READONLY;
	vm.insert(std::make_pair(key, v));
	return v;
}

void shader::create_bbs(container_node* n, bbs_vec &bbs, int loop_level) {

	bool inside_bb = false;
	bool last_inside_bb = true;
	node_iterator bb_start(n->begin()), I(bb_start), E(n->end());

	for (; I != E; ++I) {
		node *k = *I;
		inside_bb = k->type == NT_OP;

		if (inside_bb && !last_inside_bb)
			bb_start = I;
		else if (!inside_bb) {
			if (last_inside_bb
					&& I->type != NT_REPEAT
					&& I->type != NT_DEPART
					&& I->type != NT_IF) {
				bb_node *bb = create_bb(bbs.size(), loop_level);
				bbs.push_back(bb);
				n->insert_node_before(*bb_start, bb);
				if (bb_start != I)
					bb->move(bb_start, I);
			}

			if (k->is_container()) {

				bool loop = false;
				if (k->type == NT_REGION) {
					loop = static_cast<region_node*>(k)->is_loop();
				}

				create_bbs(static_cast<container_node*>(k), bbs,
				           loop_level + loop);
			}
		}

		if (k->type == NT_DEPART)
			return;

		last_inside_bb = inside_bb;
	}

	if (last_inside_bb) {
		bb_node *bb = create_bb(bbs.size(), loop_level);
		bbs.push_back(bb);
		if (n->empty())
				n->push_back(bb);
		else {
			n->insert_node_before(*bb_start, bb);
			if (bb_start != n->end())
				bb->move(bb_start, n->end());
		}
	} else {
		if (n->last && n->last->type == NT_IF) {
			bb_node *bb = create_bb(bbs.size(), loop_level);
			bbs.push_back(bb);
			n->push_back(bb);
		}
	}
}

void shader::expand_bbs(bbs_vec &bbs) {

	for (bbs_vec::iterator I = bbs.begin(), E = bbs.end(); I != E; ++I) {
		bb_node *b = *I;
		b->expand();
	}
}

sched_queue_id shader::get_queue_id(node* n) {
	switch (n->subtype) {
		case NST_ALU_INST:
		case NST_ALU_PACKED_INST:
		case NST_COPY:
		case NST_PSI:
			return SQ_ALU;
		case NST_FETCH_INST: {
			fetch_node *f = static_cast<fetch_node*>(n);
			if (ctx.is_r600() && (f->bc.op_ptr->flags & FF_VTX))
				return SQ_VTX;
			return SQ_TEX;
		}
		case NST_CF_INST:
			return SQ_CF;
		default:
			assert(0);
			return SQ_NUM;
	}
}

void shader_stats::collect(node *n) {
	if (n->is_alu_inst())
		++alu;
	else if (n->is_fetch_inst())
		++fetch;
	else if (n->is_container()) {
		container_node *c = static_cast<container_node*>(n);

		if (n->is_alu_group())
			++alu_groups;
		else if (n->is_alu_clause())
			++alu_clauses;
		else if (n->is_fetch_clause())
			++fetch_clauses;
		else if (n->is_cf_inst())
			++cf;

		if (!c->empty()) {
			for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
				collect(*I);
			}
		}
	}
}

void shader_stats::accumulate(shader_stats& s) {
	++shaders;
	ndw += s.ndw;
	ngpr += s.ngpr;
	nstack += s.nstack;

	alu += s.alu;
	alu_groups += s.alu_groups;
	alu_clauses += s.alu_clauses;
	fetch += s.fetch;
	fetch_clauses += s.fetch_clauses;
	cf += s.cf;
}

void shader_stats::dump() {
	sblog << "dw:" << ndw << ", gpr:" << ngpr << ", stk:" << nstack
			<< ", alu groups:" << alu_groups << ", alu clauses: " << alu_clauses
			<< ", alu:" << alu << ", fetch:" << fetch
			<< ", fetch clauses:" << fetch_clauses
			<< ", cf:" << cf;

	if (shaders > 1)
		sblog << ", shaders:" << shaders;

	sblog << "\n";
}

static void print_diff(unsigned d1, unsigned d2) {
	if (d1)
		sblog << ((int)d2 - (int)d1) * 100 / (int)d1 << "%";
	else if (d2)
		sblog << "N/A";
	else
		sblog << "0%";
}

void shader_stats::dump_diff(shader_stats& s) {
	sblog << "dw:"; print_diff(ndw, s.ndw);
	sblog << ", gpr:" ; print_diff(ngpr, s.ngpr);
	sblog << ", stk:" ; print_diff(nstack, s.nstack);
	sblog << ", alu groups:" ; print_diff(alu_groups, s.alu_groups);
	sblog << ", alu clauses: " ; print_diff(alu_clauses, s.alu_clauses);
	sblog << ", alu:" ; print_diff(alu, s.alu);
	sblog << ", fetch:" ; print_diff(fetch, s.fetch);
	sblog << ", fetch clauses:" ; print_diff(fetch_clauses, s.fetch_clauses);
	sblog << ", cf:" ; print_diff(cf, s.cf);
	sblog << "\n";
}

} // namespace r600_sb
