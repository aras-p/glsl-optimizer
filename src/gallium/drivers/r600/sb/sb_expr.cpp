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

#include <cmath>

#include "sb_shader.h"

namespace r600_sb {

value* get_select_value_for_em(shader& sh, value* em) {
	if (!em->def)
		return NULL;

	node *predset = em->def;
	if (!predset->is_pred_set())
		return NULL;

	alu_node *s = sh.clone(static_cast<alu_node*>(predset));
	convert_predset_to_set(sh, s);

	predset->insert_after(s);

	value* &d0 = s->dst[0];
	d0 = sh.create_temp_value();
	d0->def = s;
	return d0;
}

void convert_to_mov(alu_node &n, value *src, bool neg, bool abs) {
	n.src.resize(1);
	n.src[0] = src;
	n.bc.src[0].abs = abs;
	n.bc.src[0].neg = neg;
	n.bc.set_op(ALU_OP1_MOV);
}

expr_handler::expr_handler(shader& sh) : sh(sh), vt(sh.vt) {}

value * expr_handler::get_const(const literal &l) {
	value *v = sh.get_const_value(l);
	if (!v->gvn_source)
		vt.add_value(v);
	return v;
}

void expr_handler::assign_source(value *dst, value *src) {
	dst->gvn_source = src->gvn_source;
}

bool expr_handler::equal(value *l, value *r) {

	assert(l != r);

	if (l->gvalue() == r->gvalue())
		return true;

	if (l->def && r->def)
		return defs_equal(l, r);

	if (l->is_rel() && r->is_rel())
		return ivars_equal(l, r);

	return false;
}

bool expr_handler::ivars_equal(value* l, value* r) {
	if (l->rel->gvalue() == r->rel->gvalue()
			&& l->select == r->select) {

		vvec &lv = l->mdef.empty() ? l->muse : l->mdef;
		vvec &rv = r->mdef.empty() ? r->muse : r->mdef;

		// FIXME: replace this with more precise aliasing test
		return lv == rv;
	}
	return false;
}

bool expr_handler::defs_equal(value* l, value* r) {

	node *d1 = l->def;
	node *d2 = r->def;

	if (d1->type != d2->type || d1->subtype != d2->subtype)
		return false;

	if (d1->is_pred_set() || d2->is_pred_set())
		return false;

	if (d1->type == NT_OP) {
		switch (d1->subtype) {
		case NST_ALU_INST:
			return ops_equal(
					static_cast<alu_node*>(d1),
					static_cast<alu_node*>(d2));
//		case NST_FETCH_INST: return ops_equal(static_cast<fetch_node*>(d1),
//			static_cast<fetch_node*>(d2);
//		case NST_CF_INST: return ops_equal(static_cast<cf_node*>(d1),
//			static_cast<cf_node*>(d2);
		default:
			break;
		}
	}
	return false;
}

bool expr_handler::try_fold(value* v) {
	assert(!v->gvn_source);

	if (v->def)
		try_fold(v->def);

	if (v->gvn_source)
		return true;

	return false;
}

bool expr_handler::try_fold(node* n) {
	return n->fold_dispatch(this);
}

bool expr_handler::fold(node& n) {
	if (n.subtype == NST_PHI) {

		value *s = n.src[0];

		// FIXME disabling phi folding for registers for now, otherwise we lose
		// control flow information in some cases
		// (GCM fails on tests/shaders/glsl-fs-if-nested-loop.shader_test)
		// probably control flow transformation is required to enable it
		if (s->is_sgpr())
			return false;

		for(vvec::iterator I = n.src.begin() + 1, E = n.src.end(); I != E; ++I) {
			value *v = *I;
			if (!s->v_equal(v))
				return false;
		}

		assign_source(n.dst[0], s);
	} else {
		assert(n.subtype == NST_PSI);
		assert(n.src.size() >= 6);

		value *s = n.src[2];
		assert(s->gvn_source);

		for(vvec::iterator I = n.src.begin() + 3, E = n.src.end(); I != E; I += 3) {
			value *v = *(I+2);
			if (!s->v_equal(v))
				return false;
		}
		assign_source(n.dst[0], s);
	}
	return true;
}

bool expr_handler::fold(container_node& n) {
	return false;
}

bool expr_handler::fold_setcc(alu_node &n) {

	value* v0 = n.src[0]->gvalue();
	value* v1 = n.src[1]->gvalue();

	assert(v0 && v1 && n.dst[0]);

	unsigned flags = n.bc.op_ptr->flags;
	unsigned cc = flags & AF_CC_MASK;
	unsigned cmp_type = flags & AF_CMP_TYPE_MASK;
	unsigned dst_type = flags & AF_DST_TYPE_MASK;

	bool cond_result;
	bool have_result = false;

	bool isc0 = v0->is_const();
	bool isc1 = v1->is_const();

	literal dv, cv0, cv1;

	if (isc0) {
		cv0 = v0->get_const_value();
		apply_alu_src_mod(n.bc, 0, cv0);
	}

	if (isc1) {
		cv1 = v1->get_const_value();
		apply_alu_src_mod(n.bc, 1, cv1);
	}

	if (isc0 && isc1) {
		cond_result = evaluate_condition(flags, cv0, cv1);
		have_result = true;
	} else if (isc1) {
		if (cmp_type == AF_FLOAT_CMP) {
			if (n.bc.src[0].abs && !n.bc.src[0].neg) {
				if (cv1.f < 0.0f && (cc == AF_CC_GT || cc == AF_CC_NE)) {
					cond_result = true;
					have_result = true;
				} else if (cv1.f <= 0.0f && cc == AF_CC_GE) {
					cond_result = true;
					have_result = true;
				}
			} else if (n.bc.src[0].abs && n.bc.src[0].neg) {
				if (cv1.f > 0.0f && (cc == AF_CC_GE || cc == AF_CC_E)) {
					cond_result = false;
					have_result = true;
				} else if (cv1.f >= 0.0f && cc == AF_CC_GT) {
					cond_result = false;
					have_result = true;
				}
			}
		} else if (cmp_type == AF_UINT_CMP && cv1.u == 0 && cc == AF_CC_GE) {
			cond_result = true;
			have_result = true;
		}
	} else if (isc0) {
		if (cmp_type == AF_FLOAT_CMP) {
			if (n.bc.src[1].abs && !n.bc.src[1].neg) {
				if (cv0.f <= 0.0f && cc == AF_CC_GT) {
					cond_result = false;
					have_result = true;
				} else if (cv0.f < 0.0f && (cc == AF_CC_GE || cc == AF_CC_E)) {
					cond_result = false;
					have_result = true;
				}
			} else if (n.bc.src[1].abs && n.bc.src[1].neg) {
				if (cv0.f >= 0.0f && cc == AF_CC_GE) {
					cond_result = true;
					have_result = true;
				} else if (cv0.f > 0.0f && (cc == AF_CC_GT || cc == AF_CC_NE)) {
					cond_result = true;
					have_result = true;
				}
			}
		} else if (cmp_type == AF_UINT_CMP && cv0.u == 0 && cc == AF_CC_GT) {
			cond_result = false;
			have_result = true;
		}
	} else if (v0 == v1) {
		bc_alu_src &s0 = n.bc.src[0], &s1 = n.bc.src[1];
		if (s0.abs == s1.abs && s0.neg == s1.neg && cmp_type != AF_FLOAT_CMP) {
			// NOTE can't handle float comparisons here because of NaNs
			cond_result = (cc == AF_CC_E || cc == AF_CC_GE);
			have_result = true;
		}
	}

	if (have_result) {
		literal result;

		if (cond_result)
			result = dst_type != AF_FLOAT_DST ?
					literal(0xFFFFFFFFu) : literal(1.0f);
		else
			result = literal(0);

		convert_to_mov(n, sh.get_const_value(result));
		return fold_alu_op1(n);
	}

	return false;
}

bool expr_handler::fold(alu_node& n) {

	switch (n.bc.op_ptr->src_count) {
	case 1: return fold_alu_op1(n);
	case 2: return fold_alu_op2(n);
	case 3: return fold_alu_op3(n);
	default:
		assert(0);
	}
	return false;
}

bool expr_handler::fold(fetch_node& n) {

	unsigned chan = 0;
	for (vvec::iterator I = n.dst.begin(), E = n.dst.end(); I != E; ++I) {
		value* &v = *I;
		if (v) {
			if (n.bc.dst_sel[chan] == SEL_0)
				assign_source(*I, get_const(0.0f));
			else if (n.bc.dst_sel[chan] == SEL_1)
				assign_source(*I, get_const(1.0f));
		}
		++chan;
	}
	return false;
}

bool expr_handler::fold(cf_node& n) {
	return false;
}

void expr_handler::apply_alu_src_mod(const bc_alu &bc, unsigned src,
                                     literal &v) {
	const bc_alu_src &s = bc.src[src];

	if (s.abs)
		v = fabs(v.f);
	if (s.neg)
		v = -v.f;
}

void expr_handler::apply_alu_dst_mod(const bc_alu &bc, literal &v) {
	float omod_coeff[] = {2.0f, 4.0, 0.5f};

	if (bc.omod)
		v = v.f * omod_coeff[bc.omod - 1];
	if (bc.clamp)
		v = float_clamp(v.f);
}

bool expr_handler::args_equal(const vvec &l, const vvec &r) {

	assert(l.size() == r.size());

	int s = l.size();

	for (int k = 0; k < s; ++k) {
		if (!l[k]->v_equal(r[k]))
			return false;
	}

	return true;
}

bool expr_handler::ops_equal(const alu_node *l, const alu_node* r) {
	const bc_alu &b0 = l->bc;
	const bc_alu &b1 = r->bc;

	if (b0.op != b1.op)
		return false;

	unsigned src_count = b0.op_ptr->src_count;

	if (b0.index_mode != b1.index_mode)
		return false;

	if (b0.clamp != b1.clamp || b0.omod != b1.omod)
			return false;

	for (unsigned s = 0; s < src_count; ++s) {
		const bc_alu_src &s0 = b0.src[s];
		const bc_alu_src &s1 = b1.src[s];

		if (s0.abs != s1.abs || s0.neg != s1.neg)
			return false;
	}
	return args_equal(l->src, r->src);
}

bool expr_handler::fold_alu_op1(alu_node& n) {

	assert(!n.src.empty());
	if (n.src.empty())
		return false;

	value* v0 = n.src[0]->gvalue();

	assert(v0 && n.dst[0]);

	if (!v0->is_const()) {
		// handle (MOV -(MOV -x)) => (MOV x)
		if (n.bc.op == ALU_OP1_MOV && n.bc.src[0].neg && !n.bc.src[1].abs
				&& v0->def && v0->def->is_alu_op(ALU_OP1_MOV)) {
			alu_node *sd = static_cast<alu_node*>(v0->def);
			if (!sd->bc.clamp && !sd->bc.omod && !sd->bc.src[0].abs &&
					sd->bc.src[0].neg) {
				n.src[0] = sd->src[0];
				n.bc.src[0].neg = 0;
				v0 = n.src[0]->gvalue();
			}
		}

		if ((n.bc.op == ALU_OP1_MOV || n.bc.op == ALU_OP1_MOVA_INT ||
				n.bc.op == ALU_OP1_MOVA_GPR_INT)
				&& n.bc.clamp == 0 && n.bc.omod == 0
				&& n.bc.src[0].abs == 0 && n.bc.src[0].neg == 0) {
			assign_source(n.dst[0], v0);
			return true;
		}
		return false;
	}

	literal dv, cv = v0->get_const_value();
	apply_alu_src_mod(n.bc, 0, cv);

	switch (n.bc.op) {
	case ALU_OP1_CEIL: dv = ceil(cv.f); break;
	case ALU_OP1_COS: dv = cos(cv.f * 2.0f * M_PI); break;
	case ALU_OP1_EXP_IEEE: dv = exp2(cv.f); break;
	case ALU_OP1_FLOOR: dv = floor(cv.f); break;
	case ALU_OP1_FLT_TO_INT: dv = (int)cv.f; break; // FIXME: round modes ????
	case ALU_OP1_FLT_TO_INT_FLOOR: dv = (int32_t)floor(cv.f); break;
	case ALU_OP1_FLT_TO_INT_RPI: dv = (int32_t)floor(cv.f + 0.5f); break;
	case ALU_OP1_FLT_TO_INT_TRUNC: dv = (int32_t)trunc(cv.f); break;
	case ALU_OP1_FLT_TO_UINT: dv = (uint32_t)cv.f; break;
	case ALU_OP1_FRACT: dv = cv.f - floor(cv.f); break;
	case ALU_OP1_INT_TO_FLT: dv = (float)cv.i; break;
	case ALU_OP1_LOG_CLAMPED:
	case ALU_OP1_LOG_IEEE:
		if (cv.f != 0.0f)
			dv = log2(cv.f);
		else
			// don't fold to NAN, let the GPU handle it for now
			// (prevents degenerate LIT tests from failing)
			return false;
		break;
	case ALU_OP1_MOV: dv = cv; break;
	case ALU_OP1_MOVA_INT: dv = cv; break; // FIXME ???
//	case ALU_OP1_MOVA_FLOOR: dv = (int32_t)floor(cv.f); break;
//	case ALU_OP1_MOVA_GPR_INT:
	case ALU_OP1_NOT_INT: dv = ~cv.i; break;
	case ALU_OP1_PRED_SET_INV:
		dv = cv.f == 0.0f ? 1.0f : (cv.f == 1.0f ? 0.0f : cv.f); break;
	case ALU_OP1_PRED_SET_RESTORE: dv = cv; break;
	case ALU_OP1_RECIPSQRT_CLAMPED:
	case ALU_OP1_RECIPSQRT_FF:
	case ALU_OP1_RECIPSQRT_IEEE: dv = 1.0f / sqrt(cv.f); break;
	case ALU_OP1_RECIP_CLAMPED:
	case ALU_OP1_RECIP_FF:
	case ALU_OP1_RECIP_IEEE: dv = 1.0f / cv.f; break;
//	case ALU_OP1_RECIP_INT:
	case ALU_OP1_RECIP_UINT: dv.u = (1ull << 32) / cv.u; break;
//	case ALU_OP1_RNDNE: dv = floor(cv.f + 0.5f); break;
	case ALU_OP1_SIN: dv = sin(cv.f * 2.0f * M_PI); break;
	case ALU_OP1_SQRT_IEEE: dv = sqrt(cv.f); break;
	case ALU_OP1_TRUNC: dv = trunc(cv.f); break;

	default:
		return false;
	}

	apply_alu_dst_mod(n.bc, dv);
	assign_source(n.dst[0], get_const(dv));
	return true;
}

bool expr_handler::fold_mul_add(alu_node *n) {

	bool ieee;
	value* v0 = n->src[0]->gvalue();

	alu_node *d0 = (v0->def && v0->def->is_alu_inst()) ?
			static_cast<alu_node*>(v0->def) : NULL;

	if (d0) {
		if (d0->is_alu_op(ALU_OP2_MUL_IEEE))
			ieee = true;
		else if (d0->is_alu_op(ALU_OP2_MUL))
			ieee = false;
		else
			return false;

		if (!d0->bc.src[0].abs && !d0->bc.src[1].abs &&
				!n->bc.src[1].abs && !n->bc.src[0].abs && !d0->bc.omod &&
				!d0->bc.clamp && !n->bc.omod &&
				(!d0->src[0]->is_kcache() || !d0->src[1]->is_kcache() ||
						!n->src[1]->is_kcache())) {

			bool mul_neg = n->bc.src[0].neg;

			n->src.resize(3);
			n->bc.set_op(ieee ? ALU_OP3_MULADD_IEEE : ALU_OP3_MULADD);
			n->src[2] = n->src[1];
			n->bc.src[2] = n->bc.src[1];
			n->src[0] = d0->src[0];
			n->bc.src[0] = d0->bc.src[0];
			n->src[1] = d0->src[1];
			n->bc.src[1] = d0->bc.src[1];

			n->bc.src[0].neg ^= mul_neg;

			fold_alu_op3(*n);
			return true;
		}
	}

	value* v1 = n->src[1]->gvalue();

	alu_node *d1 = (v1->def && v1->def->is_alu_inst()) ?
			static_cast<alu_node*>(v1->def) : NULL;

	if (d1) {
		if (d1->is_alu_op(ALU_OP2_MUL_IEEE))
			ieee = true;
		else if (d1->is_alu_op(ALU_OP2_MUL))
			ieee = false;
		else
			return false;

		if (!d1->bc.src[1].abs && !d1->bc.src[0].abs &&
				!n->bc.src[0].abs && !n->bc.src[1].abs && !d1->bc.omod &&
				!d1->bc.clamp && !n->bc.omod &&
				(!d1->src[0]->is_kcache() || !d1->src[1]->is_kcache() ||
						!n->src[0]->is_kcache())) {

			bool mul_neg = n->bc.src[1].neg;

			n->src.resize(3);
			n->bc.set_op(ieee ? ALU_OP3_MULADD_IEEE : ALU_OP3_MULADD);
			n->src[2] = n->src[0];
			n->bc.src[2] = n->bc.src[0];
			n->src[1] = d1->src[1];
			n->bc.src[1] = d1->bc.src[1];
			n->src[0] = d1->src[0];
			n->bc.src[0] = d1->bc.src[0];

			n->bc.src[1].neg ^= mul_neg;

			fold_alu_op3(*n);
			return true;
		}
	}

	return false;
}

bool expr_handler::eval_const_op(unsigned op, literal &r,
                                 literal cv0, literal cv1) {

	switch (op) {
	case ALU_OP2_ADD: r = cv0.f + cv1.f; break;
	case ALU_OP2_ADDC_UINT:
		r = (uint32_t)(((uint64_t)cv0.u + cv1.u)>>32); break;
	case ALU_OP2_ADD_INT: r = cv0.i + cv1.i; break;
	case ALU_OP2_AND_INT: r = cv0.i & cv1.i; break;
	case ALU_OP2_ASHR_INT: r = cv0.i >> (cv1.i & 0x1F); break;
	case ALU_OP2_BFM_INT:
		r = (((1 << (cv0.i & 0x1F)) - 1) << (cv1.i & 0x1F)); break;
	case ALU_OP2_LSHL_INT: r = cv0.i << cv1.i; break;
	case ALU_OP2_LSHR_INT: r = cv0.u >> cv1.u; break;
	case ALU_OP2_MAX:
	case ALU_OP2_MAX_DX10: r = cv0.f > cv1.f ? cv0.f : cv1.f; break;
	case ALU_OP2_MAX_INT: r = cv0.i > cv1.i ? cv0.i : cv1.i; break;
	case ALU_OP2_MAX_UINT: r = cv0.u > cv1.u ? cv0.u : cv1.u; break;
	case ALU_OP2_MIN:
	case ALU_OP2_MIN_DX10: r = cv0.f < cv1.f ? cv0.f : cv1.f; break;
	case ALU_OP2_MIN_INT: r = cv0.i < cv1.i ? cv0.i : cv1.i; break;
	case ALU_OP2_MIN_UINT: r = cv0.u < cv1.u ? cv0.u : cv1.u; break;
	case ALU_OP2_MUL:
	case ALU_OP2_MUL_IEEE: r = cv0.f * cv1.f; break;
	case ALU_OP2_MULHI_INT:
		r = (int32_t)(((int64_t)cv0.u * cv1.u)>>32); break;
	case ALU_OP2_MULHI_UINT:
		r = (uint32_t)(((uint64_t)cv0.u * cv1.u)>>32); break;
	case ALU_OP2_MULLO_INT:
		r = (int32_t)(((int64_t)cv0.u * cv1.u) & 0xFFFFFFFF); break;
	case ALU_OP2_MULLO_UINT:
		r = (uint32_t)(((uint64_t)cv0.u * cv1.u) & 0xFFFFFFFF); break;
	case ALU_OP2_OR_INT: r = cv0.i | cv1.i; break;
	case ALU_OP2_SUB_INT: r = cv0.i - cv1.i; break;
	case ALU_OP2_XOR_INT: r = cv0.i ^ cv1.i; break;

	default:
		return false;
	}

	return true;
}

// fold the chain of associative ops, e.g. (ADD 2, (ADD x, 3)) => (ADD x, 5)
bool expr_handler::fold_assoc(alu_node *n) {

	alu_node *a = n;
	literal cr;

	int last_arg = -3;

	unsigned op = n->bc.op;
	bool allow_neg = false, cur_neg = false;

	switch(op) {
	case ALU_OP2_ADD:
	case ALU_OP2_MUL:
	case ALU_OP2_MUL_IEEE:
		allow_neg = true;
		break;
	case ALU_OP3_MULADD:
		allow_neg = true;
		op = ALU_OP2_MUL;
		break;
	case ALU_OP3_MULADD_IEEE:
		allow_neg = true;
		op = ALU_OP2_MUL_IEEE;
		break;
	default:
		if (n->bc.op_ptr->src_count != 2)
			return false;
	}

	// check if we can evaluate the op
	if (!eval_const_op(op, cr, literal(0), literal(0)))
		return false;

	while (true) {

		value *v0 = a->src[0]->gvalue();
		value *v1 = a->src[1]->gvalue();

		last_arg = -2;

		if (v1->is_const()) {
			literal arg = v1->get_const_value();
			apply_alu_src_mod(a->bc, 1, arg);
			if (cur_neg)
				arg.f = -arg.f;

			if (a == n)
				cr = arg;
			else
				eval_const_op(op, cr, cr, arg);

			if (v0->def) {
				alu_node *d0 = static_cast<alu_node*>(v0->def);
				if ((d0->is_alu_op(op) ||
						(op == ALU_OP2_MUL_IEEE &&
								d0->is_alu_op(ALU_OP2_MUL))) &&
						!d0->bc.omod && !d0->bc.clamp &&
						(!a->bc.src[0].neg || allow_neg)) {
					cur_neg ^= a->bc.src[0].neg;
					a = d0;
					continue;
				}
			}
			last_arg = 0;

		}

		if (v0->is_const()) {
			literal arg = v0->get_const_value();
			apply_alu_src_mod(a->bc, 0, arg);
			if (cur_neg)
				arg.f = -arg.f;

			if (last_arg == 0) {
				eval_const_op(op, cr, cr, arg);
				last_arg = -1;
				break;
			}

			if (a == n)
				cr = arg;
			else
				eval_const_op(op, cr, cr, arg);

			if (v1->def) {
				alu_node *d1 = static_cast<alu_node*>(v1->def);
				if ((d1->is_alu_op(op) ||
						(op == ALU_OP2_MUL_IEEE &&
								d1->is_alu_op(ALU_OP2_MUL))) &&
						!d1->bc.omod && !d1->bc.clamp &&
						(!a->bc.src[1].neg || allow_neg)) {
					cur_neg ^= a->bc.src[1].neg;
					a = d1;
					continue;
				}
			}

			last_arg = 1;
		}

		break;
	};

	if (last_arg == -1) {
		// result is const
		apply_alu_dst_mod(n->bc, cr);

		if (n->bc.op == op) {
			convert_to_mov(*n, sh.get_const_value(cr));
			fold_alu_op1(*n);
			return true;
		} else { // MULADD => ADD
			n->src[0] = n->src[2];
			n->bc.src[0] = n->bc.src[2];
			n->src[1] = sh.get_const_value(cr);
			memset(&n->bc.src[1], 0, sizeof(bc_alu_src));

			n->src.resize(2);
			n->bc.set_op(ALU_OP2_ADD);
		}
	} else if (last_arg >= 0) {
		n->src[0] = a->src[last_arg];
		n->bc.src[0] = a->bc.src[last_arg];
		n->bc.src[0].neg ^= cur_neg;
		n->src[1] = sh.get_const_value(cr);
		memset(&n->bc.src[1], 0, sizeof(bc_alu_src));
	}

	return false;
}

bool expr_handler::fold_alu_op2(alu_node& n) {

	if (n.src.size() < 2)
		return false;

	unsigned flags = n.bc.op_ptr->flags;

	if (flags & AF_SET) {
		return fold_setcc(n);
	}

	if (!sh.safe_math && (flags & AF_M_ASSOC)) {
		if (fold_assoc(&n))
			return true;
	}

	value* v0 = n.src[0]->gvalue();
	value* v1 = n.src[1]->gvalue();

	assert(v0 && v1);

	// handle some operations with equal args, e.g. x + x => x * 2
	if (v0 == v1) {
		if (n.bc.src[0].neg == n.bc.src[1].neg &&
				n.bc.src[0].abs == n.bc.src[1].abs) {
			switch (n.bc.op) {
			case ALU_OP2_MIN: // (MIN x, x) => (MOV x)
			case ALU_OP2_MAX:
				convert_to_mov(n, v0, n.bc.src[0].neg, n.bc.src[0].abs);
				return fold_alu_op1(n);
			case ALU_OP2_ADD:  // (ADD x, x) => (MUL x, 2)
				if (!sh.safe_math) {
					n.src[1] = sh.get_const_value(2.0f);
					memset(&n.bc.src[1], 0, sizeof(bc_alu_src));
					n.bc.set_op(ALU_OP2_MUL);
					return fold_alu_op2(n);
				}
				break;
			}
		}
		if (n.bc.src[0].neg != n.bc.src[1].neg &&
				n.bc.src[0].abs == n.bc.src[1].abs) {
			switch (n.bc.op) {
			case ALU_OP2_ADD:  // (ADD x, -x) => (MOV 0)
				if (!sh.safe_math) {
					convert_to_mov(n, sh.get_const_value(literal(0)));
					return fold_alu_op1(n);
				}
				break;
			}
		}
	}

	if (n.bc.op == ALU_OP2_ADD) {
		if (fold_mul_add(&n))
			return true;
	}

	bool isc0 = v0->is_const();
	bool isc1 = v1->is_const();

	if (!isc0 && !isc1)
		return false;

	literal dv, cv0, cv1;

	if (isc0) {
		cv0 = v0->get_const_value();
		apply_alu_src_mod(n.bc, 0, cv0);
	}

	if (isc1) {
		cv1 = v1->get_const_value();
		apply_alu_src_mod(n.bc, 1, cv1);
	}

	if (isc0 && isc1) {

		if (!eval_const_op(n.bc.op, dv, cv0, cv1))
			return false;

	} else { // one source is const

		if (isc0 && cv0 == literal(0)) {
			switch (n.bc.op) {
			case ALU_OP2_ADD:
			case ALU_OP2_ADD_INT:
			case ALU_OP2_MAX_UINT:
			case ALU_OP2_OR_INT:
			case ALU_OP2_XOR_INT:
				convert_to_mov(n, n.src[1], n.bc.src[1].neg,  n.bc.src[1].abs);
				return fold_alu_op1(n);
			case ALU_OP2_AND_INT:
			case ALU_OP2_ASHR_INT:
			case ALU_OP2_LSHL_INT:
			case ALU_OP2_LSHR_INT:
			case ALU_OP2_MIN_UINT:
			case ALU_OP2_MUL:
			case ALU_OP2_MULHI_UINT:
			case ALU_OP2_MULLO_UINT:
				convert_to_mov(n, sh.get_const_value(literal(0)));
				return fold_alu_op1(n);
			}
		} else if (isc1 && cv1 == literal(0)) {
			switch (n.bc.op) {
			case ALU_OP2_ADD:
			case ALU_OP2_ADD_INT:
			case ALU_OP2_ASHR_INT:
			case ALU_OP2_LSHL_INT:
			case ALU_OP2_LSHR_INT:
			case ALU_OP2_MAX_UINT:
			case ALU_OP2_OR_INT:
			case ALU_OP2_SUB_INT:
			case ALU_OP2_XOR_INT:
				convert_to_mov(n, n.src[0], n.bc.src[0].neg,  n.bc.src[0].abs);
				return fold_alu_op1(n);
			case ALU_OP2_AND_INT:
			case ALU_OP2_MIN_UINT:
			case ALU_OP2_MUL:
			case ALU_OP2_MULHI_UINT:
			case ALU_OP2_MULLO_UINT:
				convert_to_mov(n, sh.get_const_value(literal(0)));
				return fold_alu_op1(n);
			}
		} else if (isc0 && cv0 == literal(1.0f)) {
			switch (n.bc.op) {
			case ALU_OP2_MUL:
			case ALU_OP2_MUL_IEEE:
				convert_to_mov(n, n.src[1], n.bc.src[1].neg,  n.bc.src[1].abs);
				return fold_alu_op1(n);
			}
		} else if (isc1 && cv1 == literal(1.0f)) {
			switch (n.bc.op) {
			case ALU_OP2_MUL:
			case ALU_OP2_MUL_IEEE:
				convert_to_mov(n, n.src[0], n.bc.src[0].neg,  n.bc.src[0].abs);
				return fold_alu_op1(n);
			}
		}

		return false;
	}

	apply_alu_dst_mod(n.bc, dv);
	assign_source(n.dst[0], get_const(dv));
	return true;
}

bool expr_handler::evaluate_condition(unsigned alu_cnd_flags,
                                      literal s1, literal s2) {

	unsigned cmp_type = alu_cnd_flags & AF_CMP_TYPE_MASK;
	unsigned cc = alu_cnd_flags & AF_CC_MASK;

	switch (cmp_type) {
	case AF_FLOAT_CMP: {
		switch (cc) {
		case AF_CC_E : return s1.f == s2.f;
		case AF_CC_GT: return s1.f >  s2.f;
		case AF_CC_GE: return s1.f >= s2.f;
		case AF_CC_NE: return s1.f != s2.f;
		case AF_CC_LT: return s1.f <  s2.f;
		case AF_CC_LE: return s1.f <= s2.f;
		default:
			assert(!"invalid condition code");
			return false;
		}
	}
	case AF_INT_CMP: {
		switch (cc) {
		case AF_CC_E : return s1.i == s2.i;
		case AF_CC_GT: return s1.i >  s2.i;
		case AF_CC_GE: return s1.i >= s2.i;
		case AF_CC_NE: return s1.i != s2.i;
		case AF_CC_LT: return s1.i <  s2.i;
		case AF_CC_LE: return s1.i <= s2.i;
		default:
			assert(!"invalid condition code");
			return false;
		}
	}
	case AF_UINT_CMP: {
		switch (cc) {
		case AF_CC_E : return s1.u == s2.u;
		case AF_CC_GT: return s1.u >  s2.u;
		case AF_CC_GE: return s1.u >= s2.u;
		case AF_CC_NE: return s1.u != s2.u;
		case AF_CC_LT: return s1.u <  s2.u;
		case AF_CC_LE: return s1.u <= s2.u;
		default:
			assert(!"invalid condition code");
			return false;
		}
	}
	default:
		assert(!"invalid cmp_type");
		return false;
	}
}

bool expr_handler::fold_alu_op3(alu_node& n) {

	if (n.src.size() < 3)
		return false;

	if (!sh.safe_math && (n.bc.op_ptr->flags & AF_M_ASSOC)) {
		if (fold_assoc(&n))
			return true;
	}

	value* v0 = n.src[0]->gvalue();
	value* v1 = n.src[1]->gvalue();
	value* v2 = n.src[2]->gvalue();

	assert(v0 && v1 && v2 && n.dst[0]);

	bool isc0 = v0->is_const();
	bool isc1 = v1->is_const();
	bool isc2 = v2->is_const();

	literal dv, cv0, cv1, cv2;

	if (isc0) {
		cv0 = v0->get_const_value();
		apply_alu_src_mod(n.bc, 0, cv0);
	}

	if (isc1) {
		cv1 = v1->get_const_value();
		apply_alu_src_mod(n.bc, 1, cv1);
	}

	if (isc2) {
		cv2 = v2->get_const_value();
		apply_alu_src_mod(n.bc, 2, cv2);
	}

	unsigned flags = n.bc.op_ptr->flags;

	if (flags & AF_CMOV) {
		int src = 0;

		if (v1 == v2 && n.bc.src[1].neg == n.bc.src[2].neg) {
			// result doesn't depend on condition, convert to MOV
			src = 1;
		} else if (isc0) {
			// src0 is const, condition can be evaluated, convert to MOV
			bool cond = evaluate_condition(n.bc.op_ptr->flags & (AF_CC_MASK |
					AF_CMP_TYPE_MASK), cv0, literal(0));
			src = cond ? 1 : 2;
		}

		if (src) {
			// if src is selected, convert to MOV
			convert_to_mov(n, n.src[src], n.bc.src[src].neg);
			return fold_alu_op1(n);
		}
	}

	// handle (MULADD a, x, MUL (x, b)) => (MUL x, ADD (a, b))
	if (!sh.safe_math && (n.bc.op == ALU_OP3_MULADD ||
			n.bc.op == ALU_OP3_MULADD_IEEE)) {

		unsigned op = n.bc.op == ALU_OP3_MULADD_IEEE ?
				ALU_OP2_MUL_IEEE : ALU_OP2_MUL;

		if (!isc2 && v2->def && v2->def->is_alu_op(op)) {

			alu_node *md = static_cast<alu_node*>(v2->def);
			value *mv0 = md->src[0]->gvalue();
			value *mv1 = md->src[1]->gvalue();

			int es0 = -1, es1;

			if (v0 == mv0) {
				es0 = 0;
				es1 = 0;
			} else if (v0 == mv1) {
				es0 = 0;
				es1 = 1;
			} else if (v1 == mv0) {
				es0 = 1;
				es1 = 0;
			} else if (v1 == mv1) {
				es0 = 1;
				es1 = 1;
			}

			if (es0 != -1) {
				value *va0 = es0 == 0 ? v1 : v0;
				value *va1 = es1 == 0 ? mv1 : mv0;

				alu_node *add = sh.create_alu();
				add->bc.set_op(ALU_OP2_ADD);

				add->dst.resize(1);
				add->src.resize(2);

				value *t = sh.create_temp_value();
				t->def = add;
				add->dst[0] = t;
				add->src[0] = va0;
				add->src[1] = va1;
				add->bc.src[0] = n.bc.src[!es0];
				add->bc.src[1] = md->bc.src[!es1];

				add->bc.src[1].neg ^= n.bc.src[2].neg ^
						(n.bc.src[es0].neg != md->bc.src[es1].neg);

				n.insert_before(add);
				vt.add_value(t);

				t = t->gvalue();

				if (es0 == 1) {
					n.src[0] = n.src[1];
					n.bc.src[0] = n.bc.src[1];
				}

				n.src[1] = t;
				memset(&n.bc.src[1], 0, sizeof(bc_alu_src));

				n.src.resize(2);

				n.bc.set_op(op);
				return fold_alu_op2(n);
			}
		}
	}

	if (!isc0 && !isc1 && !isc2)
		return false;

	if (isc0 && isc1 && isc2) {
		switch (n.bc.op) {
		case ALU_OP3_MULADD_IEEE:
		case ALU_OP3_MULADD: dv = cv0.f * cv1.f + cv2.f; break;

		// TODO

		default:
			return false;
		}
	} else {
		if (isc0 && isc1) {
			switch (n.bc.op) {
			case ALU_OP3_MULADD:
			case ALU_OP3_MULADD_IEEE:
				dv = cv0.f * cv1.f;
				n.bc.set_op(ALU_OP2_ADD);
				n.src[0] = sh.get_const_value(dv);
				memset(&n.bc.src[0], 0, sizeof(bc_alu_src));
				n.src[1] = n.src[2];
				n.bc.src[1] = n.bc.src[2];
				n.src.resize(2);
				return fold_alu_op2(n);
			}
		}

		if (n.bc.op == ALU_OP3_MULADD) {
			if ((isc0 && cv0 == literal(0)) || (isc1 && cv1 == literal(0))) {
				convert_to_mov(n, n.src[2], n.bc.src[2].neg,  n.bc.src[2].abs);
				return fold_alu_op1(n);
			}
		}

		if (n.bc.op == ALU_OP3_MULADD || n.bc.op == ALU_OP3_MULADD_IEEE) {
			unsigned op = n.bc.op == ALU_OP3_MULADD_IEEE ?
					ALU_OP2_MUL_IEEE : ALU_OP2_MUL;

			if (isc1 && v0 == v2) {
				cv1.f += (n.bc.src[2].neg != n.bc.src[0].neg ? -1.0f : 1.0f);
				n.src[1] = sh.get_const_value(cv1);
				n.bc.src[1].neg = 0;
				n.bc.src[1].abs = 0;
				n.bc.set_op(op);
				n.src.resize(2);
				return fold_alu_op2(n);
			} else if (isc0 && v1 == v2) {
				cv0.f += (n.bc.src[2].neg != n.bc.src[1].neg ? -1.0f : 1.0f);
				n.src[0] = sh.get_const_value(cv0);
				n.bc.src[0].neg = 0;
				n.bc.src[0].abs = 0;
				n.bc.set_op(op);
				n.src.resize(2);
				return fold_alu_op2(n);
			}
		}

		return false;
	}

	apply_alu_dst_mod(n.bc, dv);
	assign_source(n.dst[0], get_const(dv));
	return true;
}

unsigned invert_setcc_condition(unsigned cc, bool &swap_args) {
	unsigned ncc = 0;

	switch (cc) {
	case AF_CC_E: ncc = AF_CC_NE; break;
	case AF_CC_NE: ncc = AF_CC_E; break;
	case AF_CC_GE: ncc = AF_CC_GT; swap_args = true; break;
	case AF_CC_GT: ncc = AF_CC_GE; swap_args = true; break;
	default:
		assert(!"unexpected condition code");
		break;
	}
	return ncc;
}

unsigned get_setcc_op(unsigned cc, unsigned cmp_type, bool int_dst) {

	if (int_dst && cmp_type == AF_FLOAT_CMP) {
		switch (cc) {
		case AF_CC_E: return ALU_OP2_SETE_DX10;
		case AF_CC_NE: return ALU_OP2_SETNE_DX10;
		case AF_CC_GT: return ALU_OP2_SETGT_DX10;
		case AF_CC_GE: return ALU_OP2_SETGE_DX10;
		}
	} else {

		switch(cmp_type) {
		case AF_FLOAT_CMP: {
			switch (cc) {
			case AF_CC_E: return ALU_OP2_SETE;
			case AF_CC_NE: return ALU_OP2_SETNE;
			case AF_CC_GT: return ALU_OP2_SETGT;
			case AF_CC_GE: return ALU_OP2_SETGE;
			}
			break;
		}
		case AF_INT_CMP: {
			switch (cc) {
			case AF_CC_E: return ALU_OP2_SETE_INT;
			case AF_CC_NE: return ALU_OP2_SETNE_INT;
			case AF_CC_GT: return ALU_OP2_SETGT_INT;
			case AF_CC_GE: return ALU_OP2_SETGE_INT;
			}
			break;
		}
		case AF_UINT_CMP: {
			switch (cc) {
			case AF_CC_E: return ALU_OP2_SETE_INT;
			case AF_CC_NE: return ALU_OP2_SETNE_INT;
			case AF_CC_GT: return ALU_OP2_SETGT_UINT;
			case AF_CC_GE: return ALU_OP2_SETGE_UINT;
			}
			break;
		}
		}
	}

	assert(!"unexpected cc&cmp_type combination");
	return ~0u;
}

unsigned get_predsetcc_op(unsigned cc, unsigned cmp_type) {

	switch(cmp_type) {
	case AF_FLOAT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP2_PRED_SETE;
		case AF_CC_NE: return ALU_OP2_PRED_SETNE;
		case AF_CC_GT: return ALU_OP2_PRED_SETGT;
		case AF_CC_GE: return ALU_OP2_PRED_SETGE;
		}
		break;
	}
	case AF_INT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP2_PRED_SETE_INT;
		case AF_CC_NE: return ALU_OP2_PRED_SETNE_INT;
		case AF_CC_GT: return ALU_OP2_PRED_SETGT_INT;
		case AF_CC_GE: return ALU_OP2_PRED_SETGE_INT;
		}
		break;
	}
	case AF_UINT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP2_PRED_SETE_INT;
		case AF_CC_NE: return ALU_OP2_PRED_SETNE_INT;
		case AF_CC_GT: return ALU_OP2_PRED_SETGT_UINT;
		case AF_CC_GE: return ALU_OP2_PRED_SETGE_UINT;
		}
		break;
	}
	}

	assert(!"unexpected cc&cmp_type combination");
	return ~0u;
}

unsigned get_killcc_op(unsigned cc, unsigned cmp_type) {

	switch(cmp_type) {
	case AF_FLOAT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP2_KILLE;
		case AF_CC_NE: return ALU_OP2_KILLNE;
		case AF_CC_GT: return ALU_OP2_KILLGT;
		case AF_CC_GE: return ALU_OP2_KILLGE;
		}
		break;
	}
	case AF_INT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP2_KILLE_INT;
		case AF_CC_NE: return ALU_OP2_KILLNE_INT;
		case AF_CC_GT: return ALU_OP2_KILLGT_INT;
		case AF_CC_GE: return ALU_OP2_KILLGE_INT;
		}
		break;
	}
	case AF_UINT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP2_KILLE_INT;
		case AF_CC_NE: return ALU_OP2_KILLNE_INT;
		case AF_CC_GT: return ALU_OP2_KILLGT_UINT;
		case AF_CC_GE: return ALU_OP2_KILLGE_UINT;
		}
		break;
	}
	}

	assert(!"unexpected cc&cmp_type combination");
	return ~0u;
}

unsigned get_cndcc_op(unsigned cc, unsigned cmp_type) {

	switch(cmp_type) {
	case AF_FLOAT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP3_CNDE;
		case AF_CC_GT: return ALU_OP3_CNDGT;
		case AF_CC_GE: return ALU_OP3_CNDGE;
		}
		break;
	}
	case AF_INT_CMP: {
		switch (cc) {
		case AF_CC_E: return ALU_OP3_CNDE_INT;
		case AF_CC_GT: return ALU_OP3_CNDGT_INT;
		case AF_CC_GE: return ALU_OP3_CNDGE_INT;
		}
		break;
	}
	}

	assert(!"unexpected cc&cmp_type combination");
	return ~0u;
}


void convert_predset_to_set(shader& sh, alu_node* a) {

	unsigned flags = a->bc.op_ptr->flags;
	unsigned cc = flags & AF_CC_MASK;
	unsigned cmp_type = flags & AF_CMP_TYPE_MASK;

	bool swap_args = false;

	cc = invert_setcc_condition(cc, swap_args);

	unsigned newop = get_setcc_op(cc, cmp_type, true);

	a->dst.resize(1);
	a->bc.set_op(newop);

	if (swap_args) {
		std::swap(a->src[0], a->src[1]);
		std::swap(a->bc.src[0], a->bc.src[1]);
	}

	a->bc.update_exec_mask = 0;
	a->bc.update_pred = 0;
}

} // namespace r600_sb
