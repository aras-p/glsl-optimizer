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

#define GCM_DEBUG 0

#if GCM_DEBUG
#define GCM_DUMP(a) do { a } while(0);
#else
#define GCM_DUMP(a)
#endif

#include <map>

#include "sb_bc.h"
#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

int gcm::run() {

	GCM_DUMP( sblog << "==== GCM ==== \n"; sh.dump_ir(); );

	collect_instructions(sh.root, true);

	init_def_count(uses, pending);

	for (node_iterator N, I = pending.begin(), E = pending.end();
			I != E; I = N) {
		N = I;
		++N;
		node *o = *I;

		GCM_DUMP(
			sblog << "pending : ";
			dump::dump_op(o);
			sblog << "\n";
		);

		if (td_is_ready(o)) {

			GCM_DUMP(
				sblog << "  ready: ";
				dump::dump_op(o);
				sblog << "\n";
			);
			pending.remove_node(o);
			ready.push_back(o);
		} else {
		}
	}

	sched_early(sh.root);

	if (!pending.empty()) {
		sblog << "##### gcm_sched_early_pass: unscheduled ops:\n";
		dump::dump_op(pending.front());
	}

	assert(pending.empty());

	GCM_DUMP( sh.dump_ir(); );

	GCM_DUMP( sblog << "\n\n ############## gcm late\n\n"; );

	collect_instructions(sh.root, false);

	init_use_count(uses, pending);

	sched_late(sh.root);
	if (!pending.empty()) {
		sblog << "##### gcm_sched_late_pass: unscheduled ops:\n";
		dump::dump_op(pending.front());
	}

	assert(ucs_level == 0);
	assert(pending.empty());

	return 0;
}


void gcm::collect_instructions(container_node *c, bool early_pass) {
	if (c->is_bb()) {

		if (early_pass) {
			for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
				node *n = *I;
				if (n->flags & NF_DONT_MOVE) {
					op_info &o = op_map[n];
					o.top_bb = o.bottom_bb = static_cast<bb_node*>(c);
				}
			}
		}

		pending.append_from(c);
		return;
	}

	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		if (I->is_container()) {
			collect_instructions(static_cast<container_node*>(*I), early_pass);
		}
	}
}

void gcm::sched_early(container_node *n) {

	region_node *r =
			(n->type == NT_REGION) ? static_cast<region_node*>(n) : NULL;

	if (r && r->loop_phi) {
		sched_early(r->loop_phi);
	}

	for (node_iterator I = n->begin(), E = n->end(); I != E; ++I) {
		if (I->type == NT_OP) {
			node *op = *I;
			if (op->subtype == NST_PHI) {
				td_release_uses(op->dst);
			}
		} else if (I->is_container()) {
			if (I->subtype == NST_BB) {
				bb_node* bb = static_cast<bb_node*>(*I);
				td_sched_bb(bb);
			} else {
				sched_early(static_cast<container_node*>(*I));
			}
		}
	}

	if (r && r->phi) {
		sched_early(r->phi);
	}
}

void gcm::td_schedule(bb_node *bb, node *n) {
	GCM_DUMP(
		sblog << "scheduling : ";
		dump::dump_op(n);
		sblog << "\n";
	);
	td_release_uses(n->dst);

	bb->push_back(n);

	op_map[n].top_bb = bb;

}

void gcm::td_sched_bb(bb_node* bb) {
	GCM_DUMP(
	sblog << "td scheduling BB_" << bb->id << "\n";
	);

	while (!ready.empty()) {
		for (sq_iterator N, I = ready.begin(), E = ready.end(); I != E;
				I = N) {
			N = I; ++N;
			td_schedule(bb, *I);
			ready.erase(I);
		}
	}
}

bool gcm::td_is_ready(node* n) {
	return uses[n] == 0;
}

void gcm::td_release_val(value *v) {

	GCM_DUMP(
		sblog << "td checking uses: ";
		dump::dump_val(v);
		sblog << "\n";
	);

	use_info *u = v->uses;
	while (u) {
		if (u->op->parent != &pending) {
			u = u->next;
			continue;
		}

		GCM_DUMP(
			sblog << "td    used in ";
			dump::dump_op(u->op);
			sblog << "\n";
		);

		if (--uses[u->op] == 0) {
			GCM_DUMP(
				sblog << "td        released : ";
				dump::dump_op(u->op);
				sblog << "\n";
			);

			pending.remove_node(u->op);
			ready.push_back(u->op);
		}
		u = u->next;
	}

}

void gcm::td_release_uses(vvec& v) {
	for (vvec::iterator I = v.begin(), E = v.end(); I != E; ++I) {
		value *v = *I;
		if (!v)
			continue;

		if (v->is_rel())
			td_release_uses(v->mdef);
		else
			td_release_val(v);
	}
}

void gcm::sched_late(container_node *n) {

	bool stack_pushed = false;

	if (n->is_depart()) {
		depart_node *d = static_cast<depart_node*>(n);
		push_uc_stack();
		stack_pushed = true;
		bu_release_phi_defs(d->target->phi, d->dep_id);
	} else if (n->is_repeat()) {
		repeat_node *r = static_cast<repeat_node*>(n);
		assert(r->target->loop_phi);
		push_uc_stack();
		stack_pushed = true;
		bu_release_phi_defs(r->target->loop_phi, r->rep_id);
	}

	for (node_riterator I = n->rbegin(), E = n->rend(); I != E; ++I) {
		if (I->is_container()) {
			if (I->subtype == NST_BB) {
				bb_node* bb = static_cast<bb_node*>(*I);
				bu_sched_bb(bb);
			} else {
				sched_late(static_cast<container_node*>(*I));
			}
		}
	}

	if (n->type == NT_IF) {
		if_node *f = static_cast<if_node*>(n);
		if (f->cond)
			pending_defs.push_back(f->cond);
	} else if (n->type == NT_REGION) {
		region_node *r = static_cast<region_node*>(n);
		if (r->loop_phi)
			bu_release_phi_defs(r->loop_phi, 0);
	}

	if (stack_pushed)
		pop_uc_stack();

}

void gcm::bu_sched_bb(bb_node* bb) {
	GCM_DUMP(
	sblog << "bu scheduling BB_" << bb->id << "\n";
	);

	bu_bb = bb;

	if (!pending_nodes.empty()) {
		GCM_DUMP(
				sblog << "pending nodes:\n";
		);

		// TODO consider sorting the exports by array_base,
		// possibly it can improve performance

		for (node_list::iterator I = pending_nodes.begin(),
				E = pending_nodes.end(); I != E; ++I) {
			bu_release_op(*I);
		}
		pending_nodes.clear();
		GCM_DUMP(
			sblog << "pending nodes processed...\n";
		);
	}


	if (!pending_defs.empty()) {
		for (vvec::iterator I = pending_defs.begin(), E = pending_defs.end();
				I != E; ++I) {
			bu_release_val(*I);
		}
		pending_defs.clear();
	}

	for (sched_queue::iterator N, I = ready_above.begin(), E = ready_above.end();
			I != E;	I = N) {
		N = I;
		++N;
		node *n = *I;
		if (op_map[n].bottom_bb == bb) {
			add_ready(*I);
			ready_above.erase(I);
		}
	}

	unsigned cnt_ready[SQ_NUM];

	container_node *clause = NULL;
	unsigned last_inst_type = ~0;
	unsigned last_count = 0;

	bool s = true;
	while (s) {
		node *n;

		s = false;

		unsigned ready_mask = 0;

		for (unsigned sq = SQ_CF; sq < SQ_NUM; ++sq) {
			if (!bu_ready[sq].empty() || !bu_ready_next[sq].empty())
				ready_mask |= (1 << sq);
		}

		if (!ready_mask) {
			for (unsigned sq = SQ_CF; sq < SQ_NUM; ++sq) {
				if (!bu_ready_early[sq].empty()) {
					node *n = bu_ready_early[sq].front();
					bu_ready_early[sq].pop_front();
					bu_ready[sq].push_back(n);
					break;
				}
			}
		}

		for (unsigned sq = SQ_CF; sq < SQ_NUM; ++sq) {

			if (sq == SQ_CF && pending_exec_mask_update) {
				pending_exec_mask_update = false;
				sq = SQ_ALU;
				--sq;
				continue;
			}

			if (!bu_ready_next[sq].empty())
				bu_ready[sq].splice(bu_ready[sq].end(), bu_ready_next[sq]);

			cnt_ready[sq] = bu_ready[sq].size();

			if ((sq == SQ_TEX || sq == SQ_VTX) && live_count <= rp_threshold &&
					cnt_ready[sq] < ctx.max_fetch/2	&&
					!bu_ready_next[SQ_ALU].empty()) {
				sq = SQ_ALU;
				--sq;
				continue;
			}

			while (!bu_ready[sq].empty()) {

				if (last_inst_type != sq) {
					clause = NULL;
					last_count = 0;
					last_inst_type = sq;
				}

				// simple heuristic to limit register pressure,
				if (sq == SQ_ALU && live_count > rp_threshold &&
						(!bu_ready[SQ_TEX].empty() ||
						 !bu_ready[SQ_VTX].empty() ||
						 !bu_ready_next[SQ_TEX].empty() ||
						 !bu_ready_next[SQ_VTX].empty())) {
					GCM_DUMP( sblog << "switching to fetch (regpressure)\n"; );
					break;
				}

				n = bu_ready[sq].front();

				// real count (e.g. SAMPLE_G will be expanded to 3 instructions,
				// 2 SET_GRAD_ + 1 SAMPLE_G
				unsigned ncnt = 1;
				if (n->is_fetch_inst() && n->src.size() == 12) {
					ncnt = 3;
				}

				if ((sq == SQ_TEX || sq == SQ_VTX) &&
						((last_count >= ctx.max_fetch/2 &&
						check_alu_ready_count(24)) ||
								last_count + ncnt > ctx.max_fetch))
					break;
				else if (sq == SQ_CF && last_count > 4 &&
						check_alu_ready_count(24))
					break;

				bu_ready[sq].pop_front();

				if (sq != SQ_CF) {
					if (!clause) {
						clause = sh.create_clause(sq == SQ_ALU ?
								NST_ALU_CLAUSE :
									sq == SQ_TEX ? NST_TEX_CLAUSE :
											NST_VTX_CLAUSE);
						bb->push_front(clause);
					}
				} else {
					clause = bb;
				}

				bu_schedule(clause, n);
				s = true;
				last_count += ncnt;
			}
		}
	}

	bu_bb = NULL;

	GCM_DUMP(
		sblog << "bu finished scheduling BB_" << bb->id << "\n";
	);
}

void gcm::bu_release_defs(vvec& v, bool src) {
	for (vvec::reverse_iterator I = v.rbegin(), E = v.rend(); I != E; ++I) {
		value *v = *I;
		if (!v || v->is_readonly())
			continue;

		if (v->is_rel()) {
			if (!v->rel->is_readonly())
				bu_release_val(v->rel);
			bu_release_defs(v->muse, true);
		} else if (src)
			bu_release_val(v);
		else {
			if (live.remove_val(v)) {
				--live_count;
			}
		}
	}
}

void gcm::push_uc_stack() {
	GCM_DUMP(
		sblog << "pushing use count stack prev_level " << ucs_level
			<< "   new level " << (ucs_level + 1) << "\n";
	);
	++ucs_level;
	if (ucs_level == nuc_stk.size()) {
		nuc_stk.resize(ucs_level + 1);
	}
	else {
		nuc_stk[ucs_level].clear();
	}
}

bool gcm::bu_is_ready(node* n) {
	nuc_map &cm = nuc_stk[ucs_level];
	nuc_map::iterator F = cm.find(n);
	unsigned uc = (F == cm.end() ? 0 : F->second);
	return uc == uses[n];
}

void gcm::bu_schedule(container_node* c, node* n) {
	GCM_DUMP(
		sblog << "bu scheduling : ";
		dump::dump_op(n);
		sblog << "\n";
	);

	assert(op_map[n].bottom_bb == bu_bb);

	bu_release_defs(n->src, true);
	bu_release_defs(n->dst, false);

	c->push_front(n);
}

void gcm::dump_uc_stack() {
	sblog << "##### uc_stk start ####\n";
	for (unsigned l = 0; l <= ucs_level; ++l) {
		nuc_map &m = nuc_stk[l];

		sblog << "nuc_stk[" << l << "] :   @" << &m << "\n";

		for (nuc_map::iterator I = m.begin(), E = m.end(); I != E; ++I) {
			sblog << "    uc " << I->second << " for ";
			dump::dump_op(I->first);
			sblog << "\n";
		}
	}
	sblog << "##### uc_stk end ####\n";
}

void gcm::pop_uc_stack() {
	nuc_map &pm = nuc_stk[ucs_level];
	--ucs_level;
	nuc_map &cm = nuc_stk[ucs_level];

	GCM_DUMP(
		sblog << "merging use stack from level " << (ucs_level+1)
			<< " to " << ucs_level << "\n";
	);

	for (nuc_map::iterator N, I = pm.begin(), E = pm.end(); I != E; ++I) {
		node *n = I->first;

		GCM_DUMP(
			sblog << "      " << cm[n] << " += " << I->second << "  for ";
			dump::dump_op(n);
			sblog << "\n";
		);

		unsigned uc = cm[n] += I->second;

		if (n->parent == &pending && uc == uses[n]) {
			cm.erase(n);
			pending_nodes.push_back(n);
			GCM_DUMP(
				sblog << "pushed pending_node due to stack pop ";
				dump::dump_op(n);
				sblog << "\n";
			);
		}
	}
}

void gcm::bu_find_best_bb(node *n, op_info &oi) {

	GCM_DUMP(
		sblog << "  find best bb : ";
		dump::dump_op(n);
		sblog << "\n";
	);

	if (oi.bottom_bb)
		return;

	// don't hoist generated copies
	if (n->flags & NF_DONT_HOIST) {
		oi.bottom_bb = bu_bb;
		return;
	}

	bb_node* best_bb = bu_bb;
	bb_node* top_bb = oi.top_bb;
	assert(oi.top_bb && !oi.bottom_bb);

	node *c = best_bb;

	// FIXME top_bb may be located inside the loop so we'll never enter it
	// in the loop below, and the instruction will be incorrectly placed at the
	// beginning of the shader.
	// For now just check if top_bb's loop_level is higher than of
	// current bb and abort the search for better bb in such case,
	// but this problem may require more complete (and more expensive) fix
	if (top_bb->loop_level <= best_bb->loop_level) {
		while (c && c != top_bb) {

			if (c->prev) {
				c = c->prev;
			} else {
				c = c->parent;
				if (!c)
					break;
				continue;
			}

			if (c->subtype == NST_BB) {
				bb_node *bb = static_cast<bb_node*>(c);
				if (bb->loop_level < best_bb->loop_level)
					best_bb = bb;
			}
		}
	}

	oi.bottom_bb = best_bb;
}

void gcm::add_ready(node *n) {
	sched_queue_id sq = sh.get_queue_id(n);
	if (n->flags & NF_SCHEDULE_EARLY)
		bu_ready_early[sq].push_back(n);
	else if (sq == SQ_ALU && n->is_copy_mov())
		bu_ready[sq].push_front(n);
	else if (n->is_alu_inst()) {
		alu_node *a = static_cast<alu_node*>(n);
		if (a->bc.op_ptr->flags & AF_PRED && a->dst[2]) {
			// PRED_SET instruction that updates exec mask
			pending_exec_mask_update = true;
		}
		bu_ready_next[sq].push_back(n);
	} else
		bu_ready_next[sq].push_back(n);
}

void gcm::bu_release_op(node * n) {
	op_info &oi = op_map[n];

	GCM_DUMP(
	sblog << "  bu release op  ";
	dump::dump_op(n);
	);

	nuc_stk[ucs_level].erase(n);
	pending.remove_node(n);

	bu_find_best_bb(n, oi);

	if (oi.bottom_bb == bu_bb) {
		GCM_DUMP( sblog << "   ready\n";);
		add_ready(n);
	} else {
		GCM_DUMP( sblog << "   ready_above\n";);
		ready_above.push_back(n);
	}
}

void gcm::bu_release_phi_defs(container_node* p, unsigned op)
{
	for (node_riterator I = p->rbegin(), E = p->rend(); I != E; ++I) {
		node *o = *I;
		value *v = o->src[op];
		if (v && !v->is_readonly())
			pending_defs.push_back(o->src[op]);

	}
}

unsigned gcm::get_uc_vec(vvec &vv) {
	unsigned c = 0;
	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;
		if (!v)
			continue;

		if (v->is_rel())
			c += get_uc_vec(v->mdef);
		else
			c += v->use_count();
	}
	return c;
}

void gcm::init_use_count(nuc_map& m, container_node &s) {
	m.clear();
	for (node_iterator I = s.begin(), E = s.end(); I != E; ++I) {
		node *n = *I;
		unsigned uc = get_uc_vec(n->dst);
		GCM_DUMP(
			sblog << "uc " << uc << "  ";
			dump::dump_op(n);
			sblog << "\n";
		);
		if (!uc) {
			pending_nodes.push_back(n);
			GCM_DUMP(
				sblog << "pushed pending_node in init ";
				dump::dump_op(n);
				sblog << "\n";
			);

		} else
			m[n] = uc;
	}
}

void gcm::bu_release_val(value* v) {
	node *n = v->any_def();

	if (n && n->parent == &pending) {
		nuc_map &m = nuc_stk[ucs_level];
		unsigned uc = ++m[n];
		unsigned uc2 = uses[n];

		if (live.add_val(v)) {
			++live_count;
			GCM_DUMP ( sblog << "live_count: " << live_count << "\n"; );
		}

		GCM_DUMP(
			sblog << "release val ";
			dump::dump_val(v);
			sblog << "  for node ";
			dump::dump_op(n);
			sblog << "    new uc=" << uc << ", total " << uc2 << "\n";
		);

		if (uc == uc2)
			bu_release_op(n);
	}

}

void gcm::init_def_count(nuc_map& m, container_node& s) {
	m.clear();
	for (node_iterator I = s.begin(), E = s.end(); I != E; ++I) {
		node *n = *I;
		unsigned dc = get_dc_vec(n->src, true) + get_dc_vec(n->dst, false);
		m[n] = dc;

		GCM_DUMP(
			sblog << "dc " << dc << "  ";
			dump::dump_op(n);
			sblog << "\n";
		);
	}
}

unsigned gcm::get_dc_vec(vvec& vv, bool src) {
	unsigned c = 0;
	for (vvec::iterator I = vv.begin(), E = vv.end(); I != E; ++I) {
		value *v = *I;
		if (!v || v->is_readonly())
			continue;

		if (v->is_rel()) {
			c += v->rel->def != NULL;
			c += get_dc_vec(v->muse, true);
		}
		else if (src) {
			c += v->def != NULL;
			c += v->adef != NULL;
		}
	}
	return c;
}

unsigned gcm::real_alu_count(sched_queue& q, unsigned max) {
	sq_iterator I(q.begin()), E(q.end());
	unsigned c = 0;

	while (I != E && c < max) {
		node *n = *I;
		if (n->is_alu_inst()) {
			if (!n->is_copy_mov() || !n->src[0]->is_any_gpr())
				++c;
		} else if (n->is_alu_packed()) {
			c += static_cast<container_node*>(n)->count();
		}
		++I;
	}

	return c;
}

bool gcm::check_alu_ready_count(unsigned threshold) {
	unsigned r = real_alu_count(bu_ready[SQ_ALU], threshold);
	if (r >= threshold)
		return true;
	r += real_alu_count(bu_ready_next[SQ_ALU], threshold - r);
	return r >= threshold;
}

} // namespace r600_sb
