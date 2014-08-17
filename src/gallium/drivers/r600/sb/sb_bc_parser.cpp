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

#define BCP_DEBUG 0

#if BCP_DEBUG
#define BCP_DUMP(q) do { q } while (0)
#else
#define BCP_DUMP(q)
#endif

extern "C" {
#include "r600_pipe.h"
#include "r600_shader.h"
}

#include <stack>

#include "sb_bc.h"
#include "sb_shader.h"
#include "sb_pass.h"

namespace r600_sb {

int bc_parser::decode() {

	dw = bc->bytecode;
	bc_ndw = bc->ndw;
	max_cf = 0;

	dec = new bc_decoder(ctx, dw, bc_ndw);

	shader_target t = TARGET_UNKNOWN;

	if (pshader) {
		switch (bc->type) {
		case TGSI_PROCESSOR_FRAGMENT: t = TARGET_PS; break;
		case TGSI_PROCESSOR_VERTEX:
			t = pshader->vs_as_es ? TARGET_ES : TARGET_VS;
			break;
		case TGSI_PROCESSOR_GEOMETRY: t = TARGET_GS; break;
		case TGSI_PROCESSOR_COMPUTE: t = TARGET_COMPUTE; break;
		default: assert(!"unknown shader target"); return -1; break;
		}
	} else {
		if (bc->type == TGSI_PROCESSOR_COMPUTE)
			t = TARGET_COMPUTE;
		else
			t = TARGET_FETCH;
	}

	sh = new shader(ctx, t, bc->debug_id);
	sh->safe_math = sb_context::safe_math || (t == TARGET_COMPUTE);

	int r = decode_shader();

	delete dec;

	sh->ngpr = bc->ngpr;
	sh->nstack = bc->nstack;

	return r;
}

int bc_parser::decode_shader() {
	int r = 0;
	unsigned i = 0;
	bool eop = false;

	sh->init();

	do {
		eop = false;
		if ((r = decode_cf(i, eop)))
			return r;

	} while (!eop || (i >> 1) <= max_cf);

	return 0;
}

int bc_parser::prepare() {
	int r = 0;
	if ((r = parse_decls()))
		return r;
	if ((r = prepare_ir()))
		return r;
	return 0;
}

int bc_parser::parse_decls() {

	if (!pshader) {
		if (gpr_reladdr)
			sh->add_gpr_array(0, bc->ngpr, 0x0F);

		// compute shaders have some values preloaded in R0, R1
		sh->add_input(0 /* GPR */, true /* preloaded */, 0x0F /* mask */);
		sh->add_input(1 /* GPR */, true /* preloaded */, 0x0F /* mask */);
		return 0;
	}

	if (pshader->indirect_files & ~(1 << TGSI_FILE_CONSTANT)) {

		assert(pshader->num_arrays);

		if (pshader->num_arrays) {
			for (unsigned i = 0; i < pshader->num_arrays; ++i) {
				r600_shader_array &a = pshader->arrays[i];
				sh->add_gpr_array(a.gpr_start, a.gpr_count, a.comp_mask);
			}
		} else {
			sh->add_gpr_array(0, pshader->bc.ngpr, 0x0F);
		}
	}

	if (sh->target == TARGET_VS || sh->target == TARGET_ES)
		sh->add_input(0, 1, 0x0F);
	else if (sh->target == TARGET_GS) {
		sh->add_input(0, 1, 0x0F);
		sh->add_input(1, 1, 0x0F);
	}

	bool ps_interp = ctx.hw_class >= HW_CLASS_EVERGREEN
			&& sh->target == TARGET_PS;

	unsigned linear = 0, persp = 0, centroid = 1;

	for (unsigned i = 0; i < pshader->ninput; ++i) {
		r600_shader_io & in = pshader->input[i];
		bool preloaded = sh->target == TARGET_PS && !(ps_interp && in.spi_sid);
		sh->add_input(in.gpr, preloaded, /*in.write_mask*/ 0x0F);
		if (ps_interp && in.spi_sid) {
			if (in.interpolate == TGSI_INTERPOLATE_LINEAR ||
					in.interpolate == TGSI_INTERPOLATE_COLOR)
				linear = 1;
			else if (in.interpolate == TGSI_INTERPOLATE_PERSPECTIVE)
				persp = 1;
			if (in.centroid)
				centroid = 2;
		}
	}

	if (ps_interp) {
		unsigned mask = (1 << (2 * (linear + persp) * centroid)) - 1;
		unsigned gpr = 0;

		while (mask) {
			sh->add_input(gpr, true, mask & 0x0F);
			++gpr;
			mask >>= 4;
		}
	}

	return 0;
}

int bc_parser::decode_cf(unsigned &i, bool &eop) {

	int r;

	cf_node *cf = sh->create_cf();
	sh->root->push_back(cf);

	unsigned id = i >> 1;

	cf->bc.id = id;

	if (cf_map.size() < id + 1)
		cf_map.resize(id + 1);

	cf_map[id] = cf;

	if ((r = dec->decode_cf(i, cf->bc)))
		return r;

	cf_op_flags flags = (cf_op_flags)cf->bc.op_ptr->flags;

	if (flags & CF_ALU) {
		if ((r = decode_alu_clause(cf)))
			return r;
	} else if (flags & CF_FETCH) {
		if ((r = decode_fetch_clause(cf)))
			return r;;
	} else if (flags & CF_EXP) {
		if (cf->bc.rw_rel)
			gpr_reladdr = true;
		assert(!cf->bc.rw_rel);
	} else if (flags & CF_MEM) {
		if (cf->bc.rw_rel)
			gpr_reladdr = true;
		assert(!cf->bc.rw_rel);
	} else if (flags & CF_BRANCH) {
		if (cf->bc.addr > max_cf)
			max_cf = cf->bc.addr;
	}

	eop = cf->bc.end_of_program || cf->bc.op == CF_OP_CF_END ||
			cf->bc.op == CF_OP_RET;
	return 0;
}

int bc_parser::decode_alu_clause(cf_node* cf) {
	unsigned i = cf->bc.addr << 1, cnt = cf->bc.count + 1, gcnt;

	cf->subtype = NST_ALU_CLAUSE;

	cgroup = 0;
	memset(slots[0], 0, 5*sizeof(slots[0][0]));

	unsigned ng = 0;

	do {
		decode_alu_group(cf, i, gcnt);
		assert(gcnt <= cnt);
		cnt -= gcnt;
		ng++;
	} while (cnt);

	return 0;
}

int bc_parser::decode_alu_group(cf_node* cf, unsigned &i, unsigned &gcnt) {
	int r;
	alu_node *n;
	alu_group_node *g = sh->create_alu_group();

	cgroup = !cgroup;
	memset(slots[cgroup], 0, 5*sizeof(slots[0][0]));
	gcnt = 0;

	unsigned literal_mask = 0;

	do {
		n = sh->create_alu();
		g->push_back(n);

		if ((r = dec->decode_alu(i, n->bc)))
			return r;

		if (!sh->assign_slot(n, slots[cgroup])) {
			assert(!"alu slot assignment failed");
			return -1;
		}

		gcnt++;

	} while (gcnt <= 5 && !n->bc.last);

	assert(n->bc.last);

	for (node_iterator I = g->begin(), E = g->end(); I != E; ++I) {
		n = static_cast<alu_node*>(*I);

		if (n->bc.dst_rel)
			gpr_reladdr = true;

		for (int k = 0; k < n->bc.op_ptr->src_count; ++k) {
			bc_alu_src &src = n->bc.src[k];
			if (src.rel)
				gpr_reladdr = true;
			if (src.sel == ALU_SRC_LITERAL) {
				literal_mask |= (1 << src.chan);
				src.value.u = dw[i + src.chan];
			}
		}
	}

	unsigned literal_ndw = 0;
	while (literal_mask) {
		g->literals.push_back(dw[i + literal_ndw]);
		literal_ndw += 1;
		literal_mask >>= 1;
	}

	literal_ndw = (literal_ndw + 1) & ~1u;

	i += literal_ndw;
	gcnt += literal_ndw >> 1;

	cf->push_back(g);
	return 0;
}

int bc_parser::prepare_alu_clause(cf_node* cf) {

	// loop over alu groups
	for (node_iterator I = cf->begin(), E = cf->end(); I != E; ++I) {
		assert(I->subtype == NST_ALU_GROUP);
		alu_group_node *g = static_cast<alu_group_node*>(*I);
		prepare_alu_group(cf, g);
	}

	return 0;
}

int bc_parser::prepare_alu_group(cf_node* cf, alu_group_node *g) {

	alu_node *n;

	cgroup = !cgroup;
	memset(slots[cgroup], 0, 5*sizeof(slots[0][0]));

	for (node_iterator I = g->begin(), E = g->end();
			I != E; ++I) {
		n = static_cast<alu_node*>(*I);

		if (!sh->assign_slot(n, slots[cgroup])) {
			assert(!"alu slot assignment failed");
			return -1;
		}

		unsigned src_count = n->bc.op_ptr->src_count;

		if (ctx.alu_slots(n->bc.op) & AF_4SLOT)
			n->flags |= NF_ALU_4SLOT;

		n->src.resize(src_count);

		unsigned flags = n->bc.op_ptr->flags;

		if (flags & AF_PRED) {
			n->dst.resize(3);
			if (n->bc.update_pred)
				n->dst[1] = sh->get_special_value(SV_ALU_PRED);
			if (n->bc.update_exec_mask)
				n->dst[2] = sh->get_special_value(SV_EXEC_MASK);

			n->flags |= NF_DONT_HOIST;

		} else if (flags & AF_KILL) {

			n->dst.resize(2);
			n->dst[1] = sh->get_special_value(SV_VALID_MASK);
			sh->set_uses_kill();

			n->flags |= NF_DONT_HOIST | NF_DONT_MOVE |
					NF_DONT_KILL | NF_SCHEDULE_EARLY;

		} else {
			n->dst.resize(1);
		}

		if (flags & AF_MOVA) {

			n->dst[0] = sh->get_special_value(SV_AR_INDEX);

			n->flags |= NF_DONT_HOIST;

		} else if (n->bc.op_ptr->src_count == 3 || n->bc.write_mask) {
			assert(!n->bc.dst_rel || n->bc.index_mode == INDEX_AR_X);

			value *v = sh->get_gpr_value(false, n->bc.dst_gpr, n->bc.dst_chan,
					n->bc.dst_rel);

			n->dst[0] = v;
		}

		if (n->bc.pred_sel) {
			sh->has_alu_predication = true;
			n->pred = sh->get_special_value(SV_ALU_PRED);
		}

		for (unsigned s = 0; s < src_count; ++s) {
			bc_alu_src &src = n->bc.src[s];

			if (src.sel == ALU_SRC_LITERAL) {
				n->src[s] = sh->get_const_value(src.value);
			} else if (src.sel == ALU_SRC_PS || src.sel == ALU_SRC_PV) {
				unsigned pgroup = !cgroup, prev_slot = src.sel == ALU_SRC_PS ?
						SLOT_TRANS : src.chan;

				// XXX shouldn't happen but llvm backend uses PS on cayman
				if (prev_slot == SLOT_TRANS && ctx.is_cayman())
					prev_slot = SLOT_X;

				alu_node *prev_alu = slots[pgroup][prev_slot];

				assert(prev_alu);

				if (!prev_alu->dst[0]) {
					value * t = sh->create_temp_value();
					prev_alu->dst[0] = t;
				}

				value *d = prev_alu->dst[0];

				if (d->is_rel()) {
					d = sh->get_gpr_value(true, prev_alu->bc.dst_gpr,
					                      prev_alu->bc.dst_chan,
					                      prev_alu->bc.dst_rel);
				}

				n->src[s] = d;
			} else if (ctx.is_kcache_sel(src.sel)) {
				unsigned sel = src.sel, kc_addr;
				unsigned kc_set = ((sel >> 7) & 2) + ((sel >> 5) & 1);

				bc_kcache &kc = cf->bc.kc[kc_set];
				kc_addr = (kc.addr << 4) + (sel & 0x1F);
				n->src[s] = sh->get_kcache_value(kc.bank, kc_addr, src.chan);
			} else if (src.sel < MAX_GPR) {
				value *v = sh->get_gpr_value(true, src.sel, src.chan, src.rel);

				n->src[s] = v;

			} else if (src.sel >= ALU_SRC_PARAM_OFFSET) {
				// using slot for value channel because in fact the slot
				// determines the channel that is loaded by INTERP_LOAD_P0
				// (and maybe some others).
				// otherwise GVN will consider INTERP_LOAD_P0s with the same
				// param index as equal instructions and leave only one of them
				n->src[s] = sh->get_special_ro_value(sel_chan(src.sel,
				                                              n->bc.slot));
			} else {
				switch (src.sel) {
				case ALU_SRC_0:
					n->src[s] = sh->get_const_value(0);
					break;
				case ALU_SRC_0_5:
					n->src[s] = sh->get_const_value(0.5f);
					break;
				case ALU_SRC_1:
					n->src[s] = sh->get_const_value(1.0f);
					break;
				case ALU_SRC_1_INT:
					n->src[s] = sh->get_const_value(1);
					break;
				case ALU_SRC_M_1_INT:
					n->src[s] = sh->get_const_value(-1);
					break;
				default:
					n->src[s] = sh->get_special_ro_value(src.sel);
					break;
				}
			}
		}
	}

	// pack multislot instructions into alu_packed_node

	alu_packed_node *p = NULL;
	for (node_iterator N, I = g->begin(), E = g->end(); I != E; I = N) {
		N = I + 1;
		alu_node *a = static_cast<alu_node*>(*I);
		unsigned sflags = a->bc.slot_flags;

		if (sflags == AF_4V || (ctx.is_cayman() && sflags == AF_S)) {
			if (!p)
				p = sh->create_alu_packed();

			a->remove();
			p->push_back(a);
		}
	}

	if (p) {
		g->push_front(p);

		if (p->count() == 3 && ctx.is_cayman()) {
			// cayman's scalar instruction that can use 3 or 4 slots

			// FIXME for simplicity we'll always add 4th slot,
			// but probably we might want to always remove 4th slot and make
			// sure that regalloc won't choose 'w' component for dst

			alu_node *f = static_cast<alu_node*>(p->first);
			alu_node *a = sh->create_alu();
			a->src = f->src;
			a->dst.resize(f->dst.size());
			a->bc = f->bc;
			a->bc.slot = SLOT_W;
			p->push_back(a);
		}
	}

	return 0;
}

int bc_parser::decode_fetch_clause(cf_node* cf) {
	int r;
	unsigned i = cf->bc.addr << 1, cnt = cf->bc.count + 1;

	cf->subtype = NST_TEX_CLAUSE;

	while (cnt--) {
		fetch_node *n = sh->create_fetch();
		cf->push_back(n);
		if ((r = dec->decode_fetch(i, n->bc)))
			return r;
		if (n->bc.src_rel || n->bc.dst_rel)
			gpr_reladdr = true;

	}
	return 0;
}

int bc_parser::prepare_fetch_clause(cf_node *cf) {

	vvec grad_v, grad_h, texture_offsets;

	for (node_iterator I = cf->begin(), E = cf->end(); I != E; ++I) {

		fetch_node *n = static_cast<fetch_node*>(*I);
		assert(n->is_valid());

		unsigned flags = n->bc.op_ptr->flags;

		unsigned vtx = flags & FF_VTX;
		unsigned num_src = vtx ? ctx.vtx_src_num : 4;

		n->dst.resize(4);

		if (flags & (FF_SETGRAD | FF_USEGRAD | FF_GETGRAD)) {
			sh->uses_gradients = true;
		}

		if (flags & (FF_SETGRAD | FF_SET_TEXTURE_OFFSETS)) {

			vvec *grad = NULL;

			switch (n->bc.op) {
				case FETCH_OP_SET_GRADIENTS_V:
					grad = &grad_v;
					break;
				case FETCH_OP_SET_GRADIENTS_H:
					grad = &grad_h;
					break;
				case FETCH_OP_SET_TEXTURE_OFFSETS:
					grad = &texture_offsets;
					break;
				default:
					assert(!"unexpected SET_GRAD instruction");
					return -1;
			}

			if (grad->empty())
				grad->resize(4);

			for(unsigned s = 0; s < 4; ++s) {
				unsigned sw = n->bc.src_sel[s];
				if (sw <= SEL_W)
					(*grad)[s] = sh->get_gpr_value(true, n->bc.src_gpr,
					                               sw, false);
				else if (sw == SEL_0)
					(*grad)[s] = sh->get_const_value(0.0f);
				else if (sw == SEL_1)
					(*grad)[s] = sh->get_const_value(1.0f);
			}
		} else {
			// Fold source values for instructions with hidden target values in to the instructions
			// using them. The set instructions are later re-emitted by bc_finalizer
			if (flags & FF_USEGRAD) {
				n->src.resize(12);
				std::copy(grad_v.begin(), grad_v.end(), n->src.begin() + 4);
				std::copy(grad_h.begin(), grad_h.end(), n->src.begin() + 8);
			} else if (flags & FF_USE_TEXTURE_OFFSETS) {
				n->src.resize(8);
				std::copy(texture_offsets.begin(), texture_offsets.end(), n->src.begin() + 4);
			} else {
				n->src.resize(4);
			}

			for(int s = 0; s < 4; ++s) {
				if (n->bc.dst_sel[s] != SEL_MASK)
					n->dst[s] = sh->get_gpr_value(false, n->bc.dst_gpr, s, false);
				// NOTE: it doesn't matter here which components of the result we
				// are using, but original n->bc.dst_sel should be taken into
				// account when building the bytecode
			}
			for(unsigned s = 0; s < num_src; ++s) {
				if (n->bc.src_sel[s] <= SEL_W)
					n->src[s] = sh->get_gpr_value(true, n->bc.src_gpr,
					                              n->bc.src_sel[s], false);
			}

		}
	}

	return 0;
}

int bc_parser::prepare_ir() {

	for(id_cf_map::iterator I = cf_map.begin(), E = cf_map.end(); I != E; ++I) {
		cf_node *c = *I;

		if (!c)
			continue;

		unsigned flags = c->bc.op_ptr->flags;

		if (flags & CF_ALU) {
			prepare_alu_clause(c);
		} else if (flags & CF_FETCH) {
			prepare_fetch_clause(c);
		} else if (c->bc.op == CF_OP_CALL_FS) {
			sh->init_call_fs(c);
			c->flags |= NF_SCHEDULE_EARLY | NF_DONT_MOVE;
		} else if (flags & CF_LOOP_START) {
			prepare_loop(c);
		} else if (c->bc.op == CF_OP_JUMP) {
			prepare_if(c);
		} else if (c->bc.op == CF_OP_LOOP_END) {
			loop_stack.pop();
		} else if (c->bc.op == CF_OP_LOOP_CONTINUE) {
			assert(!loop_stack.empty());
			repeat_node *rep = sh->create_repeat(loop_stack.top());
			if (c->parent->first != c)
				rep->move(c->parent->first, c);
			c->replace_with(rep);
			sh->simplify_dep_rep(rep);
		} else if (c->bc.op == CF_OP_LOOP_BREAK) {
			assert(!loop_stack.empty());
			depart_node *dep = sh->create_depart(loop_stack.top());
			if (c->parent->first != c)
				dep->move(c->parent->first, c);
			c->replace_with(dep);
			sh->simplify_dep_rep(dep);
		} else if (flags & CF_EXP) {

			// unroll burst exports

			assert(c->bc.op == CF_OP_EXPORT || c->bc.op == CF_OP_EXPORT_DONE);

			c->bc.set_op(CF_OP_EXPORT);

			unsigned burst_count = c->bc.burst_count;
			unsigned eop = c->bc.end_of_program;

			c->bc.end_of_program = 0;
			c->bc.burst_count = 0;

			do {
				c->src.resize(4);

				for(int s = 0; s < 4; ++s) {
					switch (c->bc.sel[s]) {
					case SEL_0:
						c->src[s] = sh->get_const_value(0.0f);
						break;
					case SEL_1:
						c->src[s] = sh->get_const_value(1.0f);
						break;
					case SEL_MASK:
						break;
					default:
						if (c->bc.sel[s] <= SEL_W)
							c->src[s] = sh->get_gpr_value(true, c->bc.rw_gpr,
									c->bc.sel[s], false);
						else
							assert(!"invalid src_sel for export");
					}
				}

				if (!burst_count--)
					break;

				cf_node *cf_next = sh->create_cf();
				cf_next->bc = c->bc;
				++cf_next->bc.rw_gpr;
				++cf_next->bc.array_base;

				c->insert_after(cf_next);
				c = cf_next;

			} while (1);

			c->bc.end_of_program = eop;
		} else if (flags & CF_MEM) {

			unsigned burst_count = c->bc.burst_count;
			unsigned eop = c->bc.end_of_program;

			c->bc.end_of_program = 0;
			c->bc.burst_count = 0;

			do {

				c->src.resize(4);

				for(int s = 0; s < 4; ++s) {
					if (c->bc.comp_mask & (1 << s))
						c->src[s] =
								sh->get_gpr_value(true, c->bc.rw_gpr, s, false);
				}

				if (((flags & CF_RAT) || (!(flags & CF_STRM))) && (c->bc.type & 1)) { // indexed write
					c->src.resize(8);
					for(int s = 0; s < 3; ++s) {
						c->src[4 + s] =
							sh->get_gpr_value(true, c->bc.index_gpr, s, false);
					}

					// FIXME probably we can relax it a bit
					c->flags |= NF_DONT_HOIST | NF_DONT_MOVE;
				}

				if (!burst_count--)
					break;

				cf_node *cf_next = sh->create_cf();
				cf_next->bc = c->bc;
				++cf_next->bc.rw_gpr;

				// FIXME is it correct?
				cf_next->bc.array_base += cf_next->bc.elem_size + 1;

				c->insert_after(cf_next);
				c = cf_next;
			} while (1);

			c->bc.end_of_program = eop;

		}
	}

	assert(loop_stack.empty());
	return 0;
}

int bc_parser::prepare_loop(cf_node* c) {

	cf_node *end = cf_map[c->bc.addr - 1];
	assert(end->bc.op == CF_OP_LOOP_END);
	assert(c->parent == end->parent);

	region_node *reg = sh->create_region();
	repeat_node *rep = sh->create_repeat(reg);

	reg->push_back(rep);
	c->insert_before(reg);
	rep->move(c, end->next);

	loop_stack.push(reg);
	return 0;
}

int bc_parser::prepare_if(cf_node* c) {
	cf_node *c_else = NULL, *end = cf_map[c->bc.addr];

	BCP_DUMP(
		sblog << "parsing JUMP @" << c->bc.id;
		sblog << "\n";
	);

	if (end->bc.op == CF_OP_ELSE) {
		BCP_DUMP(
			sblog << "  found ELSE : ";
			dump::dump_op(end);
			sblog << "\n";
		);

		c_else = end;
		end = cf_map[c_else->bc.addr];
	} else {
		BCP_DUMP(
			sblog << "  no else\n";
		);

		c_else = end;
	}

	if (c_else->parent != c->parent)
		c_else = NULL;

	if (end->parent != c->parent)
		end = NULL;

	region_node *reg = sh->create_region();

	depart_node *dep2 = sh->create_depart(reg);
	depart_node *dep = sh->create_depart(reg);
	if_node *n_if = sh->create_if();

	c->insert_before(reg);

	if (c_else != end)
		dep->move(c_else, end);
	dep2->move(c, end);

	reg->push_back(dep);
	dep->push_front(n_if);
	n_if->push_back(dep2);

	n_if->cond = sh->get_special_value(SV_EXEC_MASK);

	return 0;
}


} // namespace r600_sb
