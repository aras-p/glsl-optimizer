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

#define FBC_DEBUG 0

#if FBC_DEBUG
#define FBC_DUMP(q) do { q } while (0)
#else
#define FBC_DUMP(q)
#endif

#include "sb_bc.h"
#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

int bc_finalizer::run() {

	run_on(sh.root);

	regions_vec &rv = sh.get_regions();
	for (regions_vec::reverse_iterator I = rv.rbegin(), E = rv.rend(); I != E;
			++I) {
		region_node *r = *I;

		assert(r);

		bool loop = r->is_loop();

		if (loop)
			finalize_loop(r);
		else
			finalize_if(r);

		r->expand();
	}

	cf_peephole();

	// workaround for some problems on r6xx/7xx
	// add ALU NOP to each vertex shader
	if (!ctx.is_egcm() && (sh.target == TARGET_VS || sh.target == TARGET_ES)) {
		cf_node *c = sh.create_clause(NST_ALU_CLAUSE);

		alu_group_node *g = sh.create_alu_group();

		alu_node *a = sh.create_alu();
		a->bc.set_op(ALU_OP0_NOP);
		a->bc.last = 1;

		g->push_back(a);
		c->push_back(g);

		sh.root->push_back(c);

		c = sh.create_cf(CF_OP_NOP);
		sh.root->push_back(c);

		last_cf = c;
	}

	if (last_cf->bc.op_ptr->flags & CF_ALU) {
		last_cf = sh.create_cf(CF_OP_NOP);
		sh.root->push_back(last_cf);
	}

	if (ctx.is_cayman())
		last_cf->insert_after(sh.create_cf(CF_OP_CF_END));
	else
		last_cf->bc.end_of_program = 1;

	for (unsigned t = EXP_PIXEL; t < EXP_TYPE_COUNT; ++t) {
		cf_node *le = last_export[t];
		if (le)
			le->bc.set_op(CF_OP_EXPORT_DONE);
	}

	sh.ngpr = ngpr;
	sh.nstack = nstack;
	return 0;
}

void bc_finalizer::finalize_loop(region_node* r) {

	cf_node *loop_start = sh.create_cf(CF_OP_LOOP_START_DX10);
	cf_node *loop_end = sh.create_cf(CF_OP_LOOP_END);

	loop_start->jump_after(loop_end);
	loop_end->jump_after(loop_start);

	for (depart_vec::iterator I = r->departs.begin(), E = r->departs.end();
			I != E; ++I) {
		depart_node *dep = *I;
		cf_node *loop_break = sh.create_cf(CF_OP_LOOP_BREAK);
		loop_break->jump(loop_end);
		dep->push_back(loop_break);
		dep->expand();
	}

	// FIXME produces unnecessary LOOP_CONTINUE
	for (repeat_vec::iterator I = r->repeats.begin(), E = r->repeats.end();
			I != E; ++I) {
		repeat_node *rep = *I;
		if (!(rep->parent == r && rep->prev == NULL)) {
			cf_node *loop_cont = sh.create_cf(CF_OP_LOOP_CONTINUE);
			loop_cont->jump(loop_end);
			rep->push_back(loop_cont);
		}
		rep->expand();
	}

	r->push_front(loop_start);
	r->push_back(loop_end);
}

void bc_finalizer::finalize_if(region_node* r) {

	update_nstack(r);

	// expecting the following control flow structure here:
	//   - region
	//     {
	//       - depart/repeat 1 (it may be depart/repeat for some outer region)
	//         {
	//           - if
	//             {
	//               - depart/repeat 2 (possibly for outer region)
	//                 {
	//                   - some optional code
	//                 }
	//             }
	//           - optional <else> code> ...
	//         }
	//     }

	container_node *repdep1 = static_cast<container_node*>(r->first);
	assert(repdep1->is_depart() || repdep1->is_repeat());

	if_node *n_if = static_cast<if_node*>(repdep1->first);

	if (n_if) {


		assert(n_if->is_if());

		container_node *repdep2 = static_cast<container_node*>(n_if->first);
		assert(repdep2->is_depart() || repdep2->is_repeat());

		cf_node *if_jump = sh.create_cf(CF_OP_JUMP);
		cf_node *if_pop = sh.create_cf(CF_OP_POP);

		if_pop->bc.pop_count = 1;
		if_pop->jump_after(if_pop);

		r->push_front(if_jump);
		r->push_back(if_pop);

		bool has_else = n_if->next;

		if (has_else) {
			cf_node *nelse = sh.create_cf(CF_OP_ELSE);
			n_if->insert_after(nelse);
			if_jump->jump(nelse);
			nelse->jump_after(if_pop);
			nelse->bc.pop_count = 1;

		} else {
			if_jump->jump_after(if_pop);
			if_jump->bc.pop_count = 1;
		}

		n_if->expand();
	}

	for (depart_vec::iterator I = r->departs.begin(), E = r->departs.end();
			I != E; ++I) {
		(*I)->expand();
	}
	r->departs.clear();
	assert(r->repeats.empty());
}

void bc_finalizer::run_on(container_node* c) {

	for (node_iterator I = c->begin(), E = c->end(); I != E; ++I) {
		node *n = *I;

		if (n->is_alu_group()) {
			finalize_alu_group(static_cast<alu_group_node*>(n));
		} else {
			if (n->is_alu_clause()) {
				cf_node *c = static_cast<cf_node*>(n);

				if (c->bc.op == CF_OP_ALU_PUSH_BEFORE && ctx.is_egcm()) {
					if (ctx.stack_workaround_8xx) {
						region_node *r = c->get_parent_region();
						if (r) {
							unsigned ifs, loops;
							unsigned elems = get_stack_depth(r, loops, ifs);
							unsigned dmod1 = elems % ctx.stack_entry_size;
							unsigned dmod2 = (elems + 1) % ctx.stack_entry_size;

							if (elems && (!dmod1 || !dmod2))
								c->flags |= NF_ALU_STACK_WORKAROUND;
						}
					} else if (ctx.stack_workaround_9xx) {
						region_node *r = c->get_parent_region();
						if (r) {
							unsigned ifs, loops;
							get_stack_depth(r, loops, ifs);
							if (loops >= 2)
								c->flags |= NF_ALU_STACK_WORKAROUND;
						}
					}
				}
			} else if (n->is_fetch_inst()) {
				finalize_fetch(static_cast<fetch_node*>(n));
			} else if (n->is_cf_inst()) {
				finalize_cf(static_cast<cf_node*>(n));
			}
			if (n->is_container())
				run_on(static_cast<container_node*>(n));
		}
	}
}

void bc_finalizer::finalize_alu_group(alu_group_node* g) {

	alu_node *last = NULL;

	for (node_iterator I = g->begin(), E = g->end(); I != E; ++I) {
		alu_node *n = static_cast<alu_node*>(*I);
		unsigned slot = n->bc.slot;

		value *d = n->dst.empty() ? NULL : n->dst[0];

		if (d && d->is_special_reg()) {
			assert(n->bc.op_ptr->flags & AF_MOVA);
			d = NULL;
		}

		sel_chan fdst = d ? d->get_final_gpr() : sel_chan(0, 0);

		if (d) {
			assert(fdst.chan() == slot || slot == SLOT_TRANS);
		}

		n->bc.dst_gpr = fdst.sel();
		n->bc.dst_chan = d ? fdst.chan() : slot < SLOT_TRANS ? slot : 0;


		if (d && d->is_rel() && d->rel && !d->rel->is_const()) {
			n->bc.dst_rel = 1;
			update_ngpr(d->array->gpr.sel() + d->array->array_size -1);
		} else {
			n->bc.dst_rel = 0;
		}

		n->bc.write_mask = d != NULL;
		n->bc.last = 0;

		if (n->bc.op_ptr->flags & AF_PRED) {
			n->bc.update_pred = (n->dst[1] != NULL);
			n->bc.update_exec_mask = (n->dst[2] != NULL);
		}

		// FIXME handle predication here
		n->bc.pred_sel = PRED_SEL_OFF;

		update_ngpr(n->bc.dst_gpr);

		finalize_alu_src(g, n);

		last = n;
	}

	last->bc.last = 1;
}

void bc_finalizer::finalize_alu_src(alu_group_node* g, alu_node* a) {
	vvec &sv = a->src;

	FBC_DUMP(
		sblog << "finalize_alu_src: ";
		dump::dump_op(a);
		sblog << "\n";
	);

	unsigned si = 0;

	for (vvec::iterator I = sv.begin(), E = sv.end(); I != E; ++I, ++si) {
		value *v = *I;
		assert(v);

		bc_alu_src &src = a->bc.src[si];
		sel_chan sc;
		src.rel = 0;

		sel_chan gpr;

		switch (v->kind) {
		case VLK_REL_REG:
			sc = v->get_final_gpr();
			src.sel = sc.sel();
			src.chan = sc.chan();
			if (!v->rel->is_const()) {
				src.rel = 1;
				update_ngpr(v->array->gpr.sel() + v->array->array_size -1);
			} else
				src.rel = 0;

			break;
		case VLK_REG:
			gpr = v->get_final_gpr();
			src.sel = gpr.sel();
			src.chan = gpr.chan();
			update_ngpr(src.sel);
			break;
		case VLK_TEMP:
			src.sel = v->gpr.sel();
			src.chan = v->gpr.chan();
			update_ngpr(src.sel);
			break;
		case VLK_UNDEF:
		case VLK_CONST: {
			literal lv = v->literal_value;
			src.chan = 0;

			if (lv == literal(0))
				src.sel = ALU_SRC_0;
			else if (lv == literal(0.5f))
				src.sel = ALU_SRC_0_5;
			else if (lv == literal(1.0f))
				src.sel = ALU_SRC_1;
			else if (lv == literal(1))
				src.sel = ALU_SRC_1_INT;
			else if (lv == literal(-1))
				src.sel = ALU_SRC_M_1_INT;
			else {
				src.sel = ALU_SRC_LITERAL;
				src.chan = g->literal_chan(lv);
				src.value = lv;
			}
			break;
		}
		case VLK_KCACHE: {
			cf_node *clause = static_cast<cf_node*>(g->parent);
			assert(clause->is_alu_clause());
			sel_chan k = translate_kcache(clause, v);

			assert(k && "kcache translation failed");

			src.sel = k.sel();
			src.chan = k.chan();
			break;
		}
		case VLK_PARAM:
		case VLK_SPECIAL_CONST:
			src.sel = v->select.sel();
			src.chan = v->select.chan();
			break;
		default:
			assert(!"unknown value kind");
			break;
		}
	}

	while (si < 3) {
		a->bc.src[si++].sel = 0;
	}
}

void bc_finalizer::emit_set_grad(fetch_node* f) {

	assert(f->src.size() == 12);
	unsigned ops[2] = { FETCH_OP_SET_GRADIENTS_V, FETCH_OP_SET_GRADIENTS_H };

	unsigned arg_start = 0;

	for (unsigned op = 0; op < 2; ++op) {
		fetch_node *n = sh.create_fetch();
		n->bc.set_op(ops[op]);

		// FIXME extract this loop into a separate method and reuse it

		int reg = -1;

		arg_start += 4;

		for (unsigned chan = 0; chan < 4; ++chan) {

			n->bc.dst_sel[chan] = SEL_MASK;

			unsigned sel = SEL_MASK;

			value *v = f->src[arg_start + chan];

			if (!v || v->is_undef()) {
				sel = SEL_MASK;
			} else if (v->is_const()) {
				literal l = v->literal_value;
				if (l == literal(0))
					sel = SEL_0;
				else if (l == literal(1.0f))
					sel = SEL_1;
				else {
					sblog << "invalid fetch constant operand  " << chan << " ";
					dump::dump_op(f);
					sblog << "\n";
					abort();
				}

			} else if (v->is_any_gpr()) {
				unsigned vreg = v->gpr.sel();
				unsigned vchan = v->gpr.chan();

				if (reg == -1)
					reg = vreg;
				else if ((unsigned)reg != vreg) {
					sblog << "invalid fetch source operand  " << chan << " ";
					dump::dump_op(f);
					sblog << "\n";
					abort();
				}

				sel = vchan;

			} else {
				sblog << "invalid fetch source operand  " << chan << " ";
				dump::dump_op(f);
				sblog << "\n";
				abort();
			}

			n->bc.src_sel[chan] = sel;
		}

		if (reg >= 0)
			update_ngpr(reg);

		n->bc.src_gpr = reg >= 0 ? reg : 0;

		f->insert_before(n);
	}

}

void bc_finalizer::finalize_fetch(fetch_node* f) {

	int reg = -1;

	// src

	unsigned src_count = 4;

	unsigned flags = f->bc.op_ptr->flags;

	if (flags & FF_VTX) {
		src_count = 1;
	} else if (flags & FF_USEGRAD) {
		emit_set_grad(f);
	}

	for (unsigned chan = 0; chan < src_count; ++chan) {

		unsigned sel = f->bc.src_sel[chan];

		if (sel > SEL_W)
			continue;

		value *v = f->src[chan];

		if (v->is_undef()) {
			sel = SEL_MASK;
		} else if (v->is_const()) {
			literal l = v->literal_value;
			if (l == literal(0))
				sel = SEL_0;
			else if (l == literal(1.0f))
				sel = SEL_1;
			else {
				sblog << "invalid fetch constant operand  " << chan << " ";
				dump::dump_op(f);
				sblog << "\n";
				abort();
			}

		} else if (v->is_any_gpr()) {
			unsigned vreg = v->gpr.sel();
			unsigned vchan = v->gpr.chan();

			if (reg == -1)
				reg = vreg;
			else if ((unsigned)reg != vreg) {
				sblog << "invalid fetch source operand  " << chan << " ";
				dump::dump_op(f);
				sblog << "\n";
				abort();
			}

			sel = vchan;

		} else {
			sblog << "invalid fetch source operand  " << chan << " ";
			dump::dump_op(f);
			sblog << "\n";
			abort();
		}

		f->bc.src_sel[chan] = sel;
	}

	if (reg >= 0)
		update_ngpr(reg);

	f->bc.src_gpr = reg >= 0 ? reg : 0;

	// dst

	reg = -1;

	unsigned dst_swz[4] = {SEL_MASK, SEL_MASK, SEL_MASK, SEL_MASK};

	for (unsigned chan = 0; chan < 4; ++chan) {

		unsigned sel = f->bc.dst_sel[chan];

		if (sel == SEL_MASK)
			continue;

		value *v = f->dst[chan];
		if (!v)
			continue;

		if (v->is_any_gpr()) {
			unsigned vreg = v->gpr.sel();
			unsigned vchan = v->gpr.chan();

			if (reg == -1)
				reg = vreg;
			else if ((unsigned)reg != vreg) {
				sblog << "invalid fetch dst operand  " << chan << " ";
				dump::dump_op(f);
				sblog << "\n";
				abort();
			}

			dst_swz[vchan] = sel;

		} else {
			sblog << "invalid fetch dst operand  " << chan << " ";
			dump::dump_op(f);
			sblog << "\n";
			abort();
		}

	}

	for (unsigned i = 0; i < 4; ++i)
		f->bc.dst_sel[i] = dst_swz[i];

	assert(reg >= 0);

	if (reg >= 0)
		update_ngpr(reg);

	f->bc.dst_gpr = reg >= 0 ? reg : 0;
}

void bc_finalizer::finalize_cf(cf_node* c) {

	unsigned flags = c->bc.op_ptr->flags;

	c->bc.end_of_program = 0;
	last_cf = c;

	if (flags & CF_EXP) {
		c->bc.set_op(CF_OP_EXPORT);
		last_export[c->bc.type] = c;

		int reg = -1;

		for (unsigned chan = 0; chan < 4; ++chan) {

			unsigned sel = c->bc.sel[chan];

			if (sel > SEL_W)
				continue;

			value *v = c->src[chan];

			if (v->is_undef()) {
				sel = SEL_MASK;
			} else if (v->is_const()) {
				literal l = v->literal_value;
				if (l == literal(0))
					sel = SEL_0;
				else if (l == literal(1.0f))
					sel = SEL_1;
				else {
					sblog << "invalid export constant operand  " << chan << " ";
					dump::dump_op(c);
					sblog << "\n";
					abort();
				}

			} else if (v->is_any_gpr()) {
				unsigned vreg = v->gpr.sel();
				unsigned vchan = v->gpr.chan();

				if (reg == -1)
					reg = vreg;
				else if ((unsigned)reg != vreg) {
					sblog << "invalid export source operand  " << chan << " ";
					dump::dump_op(c);
					sblog << "\n";
					abort();
				}

				sel = vchan;

			} else {
				sblog << "invalid export source operand  " << chan << " ";
				dump::dump_op(c);
				sblog << "\n";
				abort();
			}

			c->bc.sel[chan] = sel;
		}

		if (reg >= 0)
			update_ngpr(reg);

		c->bc.rw_gpr = reg >= 0 ? reg : 0;

	} else if (flags & CF_MEM) {

		int reg = -1;
		unsigned mask = 0;

		for (unsigned chan = 0; chan < 4; ++chan) {
			value *v = c->src[chan];
			if (!v || v->is_undef())
				continue;

			if (!v->is_any_gpr() || v->gpr.chan() != chan) {
				sblog << "invalid source operand  " << chan << " ";
				dump::dump_op(c);
				sblog << "\n";
				abort();
			}
			unsigned vreg = v->gpr.sel();
			if (reg == -1)
				reg = vreg;
			else if ((unsigned)reg != vreg) {
				sblog << "invalid source operand  " << chan << " ";
				dump::dump_op(c);
				sblog << "\n";
				abort();
			}

			mask |= (1 << chan);
		}

		assert(reg >= 0 && mask);

		if (reg >= 0)
			update_ngpr(reg);

		c->bc.rw_gpr = reg >= 0 ? reg : 0;
		c->bc.comp_mask = mask;

		if (((flags & CF_RAT) || (!(flags & CF_STRM))) && (c->bc.type & 1)) {

			reg = -1;

			for (unsigned chan = 0; chan < 4; ++chan) {
				value *v = c->src[4 + chan];
				if (!v || v->is_undef())
					continue;

				if (!v->is_any_gpr() || v->gpr.chan() != chan) {
					sblog << "invalid source operand  " << chan << " ";
					dump::dump_op(c);
					sblog << "\n";
					abort();
				}
				unsigned vreg = v->gpr.sel();
				if (reg == -1)
					reg = vreg;
				else if ((unsigned)reg != vreg) {
					sblog << "invalid source operand  " << chan << " ";
					dump::dump_op(c);
					sblog << "\n";
					abort();
				}
			}

			assert(reg >= 0);

			if (reg >= 0)
				update_ngpr(reg);

			c->bc.index_gpr = reg >= 0 ? reg : 0;
		}
	} else if (flags & CF_CALL) {
		update_nstack(c->get_parent_region(), ctx.wavefront_size == 16 ? 2 : 1);
	}
}

sel_chan bc_finalizer::translate_kcache(cf_node* alu, value* v) {
	unsigned sel = v->select.sel();
	unsigned bank = sel >> 12;
	unsigned chan = v->select.chan();
	static const unsigned kc_base[] = {128, 160, 256, 288};

	sel &= 4095;

	unsigned line = sel >> 4;

	for (unsigned k = 0; k < 4; ++k) {
		bc_kcache &kc = alu->bc.kc[k];

		if (kc.mode == KC_LOCK_NONE)
			break;

		if (kc.bank == bank && (kc.addr == line ||
				(kc.mode == KC_LOCK_2 && kc.addr + 1 == line))) {

			sel = kc_base[k] + (sel - (kc.addr << 4));

			return sel_chan(sel, chan);
		}
	}

	assert(!"kcache translation error");
	return 0;
}

void bc_finalizer::update_ngpr(unsigned gpr) {
	if (gpr < MAX_GPR - ctx.alu_temp_gprs && gpr >= ngpr)
		ngpr = gpr + 1;
}

unsigned bc_finalizer::get_stack_depth(node *n, unsigned &loops,
                                           unsigned &ifs, unsigned add) {
	unsigned stack_elements = add;
	bool has_non_wqm_push = (add != 0);
	region_node *r = n->is_region() ?
			static_cast<region_node*>(n) : n->get_parent_region();

	loops = 0;
	ifs = 0;

	while (r) {
		if (r->is_loop()) {
			++loops;
		} else {
			++ifs;
			has_non_wqm_push = true;
		}
		r = r->get_parent_region();
	}
	stack_elements += (loops * ctx.stack_entry_size) + ifs;

	// reserve additional elements in some cases
	switch (ctx.hw_class) {
	case HW_CLASS_R600:
	case HW_CLASS_R700:
		// If any non-WQM push is invoked, 2 elements should be reserved.
		if (has_non_wqm_push)
			stack_elements += 2;
		break;
	case HW_CLASS_CAYMAN:
		// If any stack operation is invoked, 2 elements should be reserved
		if (stack_elements)
			stack_elements += 2;
		break;
	case HW_CLASS_EVERGREEN:
		// According to the docs we need to reserve 1 element for each of the
		// following cases:
		//   1) non-WQM push is used with WQM/LOOP frames on stack
		//   2) ALU_ELSE_AFTER is used at the point of max stack usage
		// NOTE:
		// It was found that the conditions above are not sufficient, there are
		// other cases where we also need to reserve stack space, that's why
		// we always reserve 1 stack element if we have non-WQM push on stack.
		// Condition 2 is ignored for now because we don't use this instruction.
		if (has_non_wqm_push)
			++stack_elements;
		break;
	case HW_CLASS_UNKNOWN:
		assert(0);
	}
	return stack_elements;
}

void bc_finalizer::update_nstack(region_node* r, unsigned add) {
	unsigned loops = 0;
	unsigned ifs = 0;
	unsigned elems = r ? get_stack_depth(r, loops, ifs, add) : add;

	// XXX all chips expect this value to be computed using 4 as entry size,
	// not the real entry size
	unsigned stack_entries = (elems + 3) >> 2;

	if (nstack < stack_entries)
		nstack = stack_entries;
}

void bc_finalizer::cf_peephole() {
	if (ctx.stack_workaround_8xx || ctx.stack_workaround_9xx) {
		for (node_iterator N, I = sh.root->begin(), E = sh.root->end(); I != E;
				I = N) {
			N = I; ++N;
			cf_node *c = static_cast<cf_node*>(*I);

			if (c->bc.op == CF_OP_ALU_PUSH_BEFORE &&
					(c->flags & NF_ALU_STACK_WORKAROUND)) {
				cf_node *push = sh.create_cf(CF_OP_PUSH);
				c->insert_before(push);
				push->jump(c);
				c->bc.set_op(CF_OP_ALU);
			}
		}
	}

	for (node_iterator N, I = sh.root->begin(), E = sh.root->end(); I != E;
			I = N) {
		N = I; ++N;

		cf_node *c = static_cast<cf_node*>(*I);

		if (c->jump_after_target) {
			c->jump_target = static_cast<cf_node*>(c->jump_target->next);
			c->jump_after_target = false;
		}

		if (c->is_cf_op(CF_OP_POP)) {
			node *p = c->prev;
			if (p->is_alu_clause()) {
				cf_node *a = static_cast<cf_node*>(p);

				if (a->bc.op == CF_OP_ALU) {
					a->bc.set_op(CF_OP_ALU_POP_AFTER);
					c->remove();
				}
			}
		} else if (c->is_cf_op(CF_OP_JUMP) && c->jump_target == c->next) {
			// if JUMP is immediately followed by its jump target,
			// then JUMP is useless and we can eliminate it
			c->remove();
		}
	}
}

} // namespace r600_sb
