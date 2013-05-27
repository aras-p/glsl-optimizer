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

	// TODO

	return false;
}

bool expr_handler::fold(alu_node& n) {

	if (n.bc.op_ptr->flags & (AF_PRED | AF_KILL)) {
		fold_setcc(n);
		return false;
	}

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

	value* v0 = n.src[0];

	assert(v0 && n.dst[0]);

	if (!v0->is_const()) {
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
//	case ALU_OP1_RECIP_UINT:
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

bool expr_handler::fold_alu_op2(alu_node& n) {

	if (n.src.size() < 2)
		return false;

	value* v0 = n.src[0];
	value* v1 = n.src[1];

	assert(v0 && v1 && n.dst[0]);

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
		switch (n.bc.op) {
		case ALU_OP2_ADD: dv = cv0.f + cv1.f; break;
		case ALU_OP2_ADDC_UINT:
			dv = (uint32_t)(((uint64_t)cv0.u + cv1.u)>>32); break;
		case ALU_OP2_ADD_INT: dv = cv0.i + cv1.i; break;
		case ALU_OP2_AND_INT: dv = cv0.i & cv1.i; break;
		case ALU_OP2_ASHR_INT: dv = cv0.i >> (cv1.i & 0x1F); break;
		case ALU_OP2_BFM_INT:
			dv = (((1 << (cv0.i & 0x1F)) - 1) << (cv1.i & 0x1F)); break;
		case ALU_OP2_LSHL_INT: dv = cv0.i << cv1.i; break;
		case ALU_OP2_LSHR_INT: dv = cv0.u >> cv1.u; break;
		case ALU_OP2_MAX:
		case ALU_OP2_MAX_DX10: dv = cv0.f > cv1.f ? cv0.f : cv1.f; break;
		case ALU_OP2_MAX_INT: dv = cv0.i > cv1.i ? cv0.i : cv1.i; break;
		case ALU_OP2_MAX_UINT: dv = cv0.u > cv1.u ? cv0.u : cv1.u; break;
		case ALU_OP2_MIN:
		case ALU_OP2_MIN_DX10: dv = cv0.f < cv1.f ? cv0.f : cv1.f; break;
		case ALU_OP2_MIN_INT: dv = cv0.i < cv1.i ? cv0.i : cv1.i; break;
		case ALU_OP2_MIN_UINT: dv = cv0.u < cv1.u ? cv0.u : cv1.u; break;
		case ALU_OP2_MUL:
		case ALU_OP2_MUL_IEEE: dv = cv0.f * cv1.f; break;
		case ALU_OP2_MULHI_INT:
			dv = (int32_t)(((int64_t)cv0.u * cv1.u)>>32); break;
		case ALU_OP2_MULHI_UINT:
			dv = (uint32_t)(((uint64_t)cv0.u * cv1.u)>>32); break;
		case ALU_OP2_MULLO_INT:
			dv = (int32_t)(((int64_t)cv0.u * cv1.u) & 0xFFFFFFFF); break;
		case ALU_OP2_MULLO_UINT:
			dv = (uint32_t)(((uint64_t)cv0.u * cv1.u) & 0xFFFFFFFF); break;
		case ALU_OP2_OR_INT: dv = cv0.i | cv1.i; break;
		case ALU_OP2_SUB_INT: dv = cv0.i - cv1.i; break;
		case ALU_OP2_XOR_INT: dv = cv0.i ^ cv1.i; break;

		case ALU_OP2_SETE: dv = cv0.f == cv1.f ? 1.0f : 0.0f; break;

		default:
			return false;
		}

	} else { // one source is const

		// TODO handle 1 * anything, 0 * anything, 0 + anything, etc

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

	value* v0 = n.src[0];
	value* v1 = n.src[1];
	value* v2 = n.src[2];

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

	if (n.bc.op_ptr->flags & AF_CMOV) {
		int src = 0;

		if (v1->gvalue() == v2->gvalue() &&
				n.bc.src[1].neg == n.bc.src[2].neg) {
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
			n.bc.src[0] = n.bc.src[src];
			n.src[0] = n.src[src];
			n.src.resize(1);
			n.bc.set_op(ALU_OP1_MOV);
			return fold_alu_op1(n);
		}
	}

	if (!isc0 && !isc1 && !isc2)
		return false;

	if (isc0 && isc1 && isc2) {
		switch (n.bc.op) {
		case ALU_OP3_MULADD: dv = cv0.f * cv1.f + cv2.f; break;

		// TODO

		default:
			return false;
		}

	} else {

		// TODO

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
