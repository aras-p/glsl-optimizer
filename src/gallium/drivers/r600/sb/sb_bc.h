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

#ifndef SB_BC_H_
#define SB_BC_H_

extern "C" {
#include <stdint.h>
#include "r600_isa.h"
}

#include <cstdio>
#include <string>
#include <vector>
#include <stack>

struct r600_bytecode;
struct r600_shader;

namespace r600_sb {

class hw_encoding_format;
class node;
class alu_node;
class cf_node;
class fetch_node;
class alu_group_node;
class region_node;
class shader;

class sb_ostream {
public:
	sb_ostream() {}

	virtual void write(const char *s) = 0;

	sb_ostream& operator <<(const char *s) {
		write(s);
		return *this;
	}

	sb_ostream& operator <<(const std::string& s) {
		return *this << s.c_str();
	}

	sb_ostream& operator <<(void *p) {
		char b[32];
		sprintf(b, "%p", p);
		return *this << b;
	}

	sb_ostream& operator <<(char c) {
		char b[2];
		sprintf(b, "%c", c);
		return *this << b;
	}

	sb_ostream& operator <<(int n) {
		char b[32];
		sprintf(b, "%d", n);
		return *this << b;
	}

	sb_ostream& operator <<(unsigned n) {
		char b[32];
		sprintf(b, "%u", n);
		return *this << b;
	}

	sb_ostream& operator <<(double d) {
		char b[32];
		snprintf(b, 32, "%g", d);
		return *this << b;
	}

	// print as field of specified width, right aligned
	void print_w(int n, int width) {
		char b[256],f[8];
		sprintf(f, "%%%dd", width);
		snprintf(b, 256, f, n);
		write(b);
	}

	// print as field of specified width, left aligned
	void print_wl(int n, int width) {
		char b[256],f[8];
		sprintf(f, "%%-%dd", width);
		snprintf(b, 256, f, n);
		write(b);
	}

	// print as field of specified width, left aligned
	void print_wl(const std::string &s, int width) {
		write(s.c_str());
		int l = s.length();
		while (l++ < width) {
			write(" ");
		}
	}

	// print int as field of specified width, right aligned, zero-padded
	void print_zw(int n, int width) {
		char b[256],f[8];
		sprintf(f, "%%0%dd", width);
		snprintf(b, 256, f, n);
		write(b);
	}

	// print int as field of specified width, right aligned, zero-padded, hex
	void print_zw_hex(int n, int width) {
		char b[256],f[8];
		sprintf(f, "%%0%dx", width);
		snprintf(b, 256, f, n);
		write(b);
	}
};

class sb_ostringstream : public sb_ostream {
	std::string data;
public:
	sb_ostringstream() : data() {}

	virtual void write(const char *s) {
		data += s;
	}

	void clear() { data.clear(); }

	const char* c_str() { return data.c_str(); }
	std::string& str() { return data; }
};

class sb_log : public sb_ostream {
	FILE *o;
public:
	sb_log() : o(stderr) {}

	virtual void write(const char *s) {
		fputs(s, o);
	}
};

extern sb_log sblog;

enum shader_target
{
	TARGET_UNKNOWN,
	TARGET_VS,
	TARGET_ES,
	TARGET_PS,
	TARGET_GS,
	TARGET_GS_COPY,
	TARGET_COMPUTE,
	TARGET_FETCH,

	TARGET_NUM
};

enum sb_hw_class_bits
{
	HB_R6	= (1<<0),
	HB_R7	= (1<<1),
	HB_EG	= (1<<2),
	HB_CM	= (1<<3),

	HB_R6R7 = (HB_R6 | HB_R7),
	HB_EGCM = (HB_EG | HB_CM),
	HB_R6R7EG = (HB_R6 | HB_R7 | HB_EG),
	HB_R7EGCM = (HB_R7 | HB_EG | HB_CM),

	HB_ALL = (HB_R6 | HB_R7 | HB_EG | HB_CM)
};

enum sb_hw_chip
{
	HW_CHIP_UNKNOWN,
	HW_CHIP_R600,
	HW_CHIP_RV610,
	HW_CHIP_RV630,
	HW_CHIP_RV670,
	HW_CHIP_RV620,
	HW_CHIP_RV635,
	HW_CHIP_RS780,
	HW_CHIP_RS880,
	HW_CHIP_RV770,
	HW_CHIP_RV730,
	HW_CHIP_RV710,
	HW_CHIP_RV740,
	HW_CHIP_CEDAR,
	HW_CHIP_REDWOOD,
	HW_CHIP_JUNIPER,
	HW_CHIP_CYPRESS,
	HW_CHIP_HEMLOCK,
	HW_CHIP_PALM,
	HW_CHIP_SUMO,
	HW_CHIP_SUMO2,
	HW_CHIP_BARTS,
	HW_CHIP_TURKS,
	HW_CHIP_CAICOS,
	HW_CHIP_CAYMAN,
	HW_CHIP_ARUBA
};

enum sb_hw_class
{
	HW_CLASS_UNKNOWN,
	HW_CLASS_R600,
	HW_CLASS_R700,
	HW_CLASS_EVERGREEN,
	HW_CLASS_CAYMAN
};

enum alu_slots {
	SLOT_X = 0,
	SLOT_Y = 1,
	SLOT_Z = 2,
	SLOT_W = 3,
	SLOT_TRANS = 4
};

enum misc_consts {
	MAX_ALU_LITERALS = 4,
	MAX_ALU_SLOTS = 128,
	MAX_GPR = 128,
	MAX_CHAN = 4

};

enum alu_src_sel {

	ALU_SRC_LDS_OQ_A = 219,
	ALU_SRC_LDS_OQ_B = 220,
	ALU_SRC_LDS_OQ_A_POP = 221,
	ALU_SRC_LDS_OQ_B_POP = 222,
	ALU_SRC_LDS_DIRECT_A = 223,
	ALU_SRC_LDS_DIRECT_B = 224,
	ALU_SRC_TIME_HI = 227,
	ALU_SRC_TIME_LO = 228,
	ALU_SRC_MASK_HI = 229,
	ALU_SRC_MASK_LO = 230,
	ALU_SRC_HW_WAVE_ID = 231,
	ALU_SRC_SIMD_ID = 232,
	ALU_SRC_SE_ID = 233,
	ALU_SRC_HW_THREADGRP_ID = 234,
	ALU_SRC_WAVE_ID_IN_GRP = 235,
	ALU_SRC_NUM_THREADGRP_WAVES = 236,
	ALU_SRC_HW_ALU_ODD = 237,
	ALU_SRC_LOOP_IDX = 238,
	ALU_SRC_PARAM_BASE_ADDR = 240,
	ALU_SRC_NEW_PRIM_MASK = 241,
	ALU_SRC_PRIM_MASK_HI = 242,
	ALU_SRC_PRIM_MASK_LO = 243,
	ALU_SRC_1_DBL_L = 244,
	ALU_SRC_1_DBL_M = 245,
	ALU_SRC_0_5_DBL_L = 246,
	ALU_SRC_0_5_DBL_M = 247,
	ALU_SRC_0 = 248,
	ALU_SRC_1 = 249,
	ALU_SRC_1_INT = 250,
	ALU_SRC_M_1_INT = 251,
	ALU_SRC_0_5 = 252,
	ALU_SRC_LITERAL = 253,
	ALU_SRC_PV = 254,
	ALU_SRC_PS = 255,

	ALU_SRC_PARAM_OFFSET = 448
};

enum alu_predicate_select
{
	PRED_SEL_OFF	= 0,
//	RESERVED		= 1,
	PRED_SEL_0		= 2,
	PRED_SEL_1		= 3
};


enum alu_omod {
	OMOD_OFF  = 0,
	OMOD_M2   = 1,
	OMOD_M4   = 2,
	OMOD_D2   = 3
};

enum alu_index_mode {
	INDEX_AR_X        = 0,
	INDEX_AR_Y_R600   = 1,
	INDEX_AR_Z_R600   = 2,
	INDEX_AR_W_R600   = 3,

	INDEX_LOOP        = 4,
	INDEX_GLOBAL      = 5,
	INDEX_GLOBAL_AR_X = 6
};

enum alu_cayman_mova_dst {
	CM_MOVADST_AR_X,
	CM_MOVADST_PC,
	CM_MOVADST_IDX0,
	CM_MOVADST_IDX1,
	CM_MOVADST_CG0,		// clause-global byte 0
	CM_MOVADST_CG1,
	CM_MOVADST_CG2,
	CM_MOVADST_CG3
};

enum alu_cayman_exec_mask_op {
	CM_EMO_DEACTIVATE,
	CM_EMO_BREAK,
	CM_EMO_CONTINUE,
	CM_EMO_KILL
};


enum cf_exp_type {
	EXP_PIXEL,
	EXP_POS,
	EXP_PARAM,

	EXP_TYPE_COUNT
};

enum cf_mem_type {
	MEM_WRITE,
	MEM_WRITE_IND,
	MEM_WRITE_ACK,
	MEM_WRITE_IND_ACK
};


enum alu_kcache_mode {
	KC_LOCK_NONE,
	KC_LOCK_1,
	KC_LOCK_2,
	KC_LOCK_LOOP
};

enum alu_kcache_index_mode {
	KC_INDEX_NONE,
	KC_INDEX_0,
	KC_INDEX_1,
	KC_INDEX_INVALID
};

enum chan_select {
	SEL_X	= 0,
	SEL_Y	= 1,
	SEL_Z	= 2,
	SEL_W	= 3,
	SEL_0	= 4,
	SEL_1	= 5,
//	RESERVED = 6,
	SEL_MASK = 7
};

enum bank_swizzle {
	VEC_012 = 0,
	VEC_021 = 1,
	VEC_120 = 2,
	VEC_102 = 3,
	VEC_201 = 4,
	VEC_210 = 5,

	VEC_NUM = 6,

	SCL_210 = 0,
	SCL_122 = 1,
	SCL_212 = 2,
	SCL_221 = 3,

	SCL_NUM = 4

};

enum sched_queue_id {
	SQ_CF,
	SQ_ALU,
	SQ_TEX,
	SQ_VTX,

	SQ_NUM
};

struct literal {
	union {
		int32_t i;
		uint32_t u;
		float f;
	};

	literal(int32_t i = 0) : i(i) {}
	literal(uint32_t u) : u(u) {}
	literal(float f) : f(f) {}
	literal(double f) : f(f) {}
	operator uint32_t() const { return u; }
	bool operator ==(literal l) { return u == l.u; }
	bool operator ==(int v_int) { return i == v_int; }
	bool operator ==(unsigned v_uns) { return u == v_uns; }
};

struct bc_kcache {
	unsigned mode;
	unsigned bank;
	unsigned addr;
	unsigned index_mode;
} ;

// TODO optimize bc structures

struct bc_cf {

	bc_kcache kc[4];

	unsigned id;


	const cf_op_info * op_ptr;
	unsigned op;

	unsigned addr:32;

	unsigned alt_const:1;
	unsigned uses_waterfall:1;

	unsigned barrier:1;
	unsigned count:7;
	unsigned pop_count:3;
	unsigned call_count:6;
	unsigned whole_quad_mode:1;
	unsigned valid_pixel_mode:1;

	unsigned jumptable_sel:3;
	unsigned cf_const:5;
	unsigned cond:2;
	unsigned end_of_program:1;

	unsigned array_base:13;
	unsigned elem_size:2;
	unsigned index_gpr:7;
	unsigned rw_gpr:7;
	unsigned rw_rel:1;
	unsigned type:2;

	unsigned burst_count:4;
	unsigned mark:1;
	unsigned sel[4];

	unsigned array_size:12;
	unsigned comp_mask:4;

	unsigned rat_id:4;
	unsigned rat_inst:6;
	unsigned rat_index_mode:2;

	void set_op(unsigned op) { this->op = op; op_ptr = r600_isa_cf(op); }

	bool is_alu_extended() {
		assert(op_ptr->flags & CF_ALU);
		return kc[2].mode != KC_LOCK_NONE || kc[3].mode != KC_LOCK_NONE;
	}

};

struct bc_alu_src {
	unsigned sel:9;
	unsigned chan:2;
	unsigned neg:1;
	unsigned abs:1;
	unsigned rel:1;
	literal value;
};

struct bc_alu {
	const alu_op_info * op_ptr;
	unsigned op;

	bc_alu_src src[3];

	unsigned dst_gpr:7;
	unsigned dst_chan:2;
	unsigned dst_rel:1;
	unsigned clamp:1;
	unsigned omod:2;
	unsigned bank_swizzle:3;

	unsigned index_mode:3;
	unsigned last:1;
	unsigned pred_sel:2;

	unsigned fog_merge:1;
	unsigned write_mask:1;
	unsigned update_exec_mask:1;
	unsigned update_pred:1;

	unsigned slot:3;

	alu_op_flags slot_flags;

	void set_op(unsigned op) {
		this->op = op;
		op_ptr = r600_isa_alu(op);
	}
};

struct bc_fetch {
	const fetch_op_info * op_ptr;
	unsigned op;

	unsigned bc_frac_mode:1;
	unsigned fetch_whole_quad:1;
	unsigned resource_id:8;

	unsigned src_gpr:7;
	unsigned src_rel:1;
	unsigned src_sel[4];

	unsigned dst_gpr:7;
	unsigned dst_rel:1;
	unsigned dst_sel[4];

	unsigned alt_const:1;

	unsigned inst_mod:2;
	unsigned resource_index_mode:2;
	unsigned sampler_index_mode:2;

	unsigned coord_type[4];
	unsigned lod_bias:7;

	unsigned offset[3];

	unsigned sampler_id:5;


	unsigned fetch_type:2;
	unsigned mega_fetch_count:6;
	unsigned coalesced_read:1;
	unsigned structured_read:2;
	unsigned lds_req:1;

	unsigned data_format:6;
	unsigned format_comp_all:1;
	unsigned num_format_all:2;
	unsigned semantic_id:8;
	unsigned srf_mode_all:1;
	unsigned use_const_fields:1;

	unsigned const_buf_no_stride:1;
	unsigned endian_swap:2;
	unsigned mega_fetch:1;

	void set_op(unsigned op) { this->op = op; op_ptr = r600_isa_fetch(op); }
};

struct shader_stats {
	unsigned	ndw;
	unsigned	ngpr;
	unsigned	nstack;

	unsigned	cf; // clause instructions not included
	unsigned	alu;
	unsigned	alu_clauses;
	unsigned	fetch_clauses;
	unsigned	fetch;
	unsigned	alu_groups;

	unsigned	shaders;		// number of shaders (for accumulated stats)

	shader_stats() : ndw(), ngpr(), nstack(), cf(), alu(), alu_clauses(),
			fetch_clauses(), fetch(), alu_groups(), shaders() {}

	void collect(node *n);
	void accumulate(shader_stats &s);
	void dump();
	void dump_diff(shader_stats &s);
};

class sb_context {

public:

	shader_stats src_stats, opt_stats;

	r600_isa *isa;

	sb_hw_chip hw_chip;
	sb_hw_class hw_class;

	unsigned alu_temp_gprs;
	unsigned max_fetch;
	bool has_trans;
	unsigned vtx_src_num;
	unsigned num_slots;
	bool uses_mova_gpr;

	bool stack_workaround_8xx;
	bool stack_workaround_9xx;

	unsigned wavefront_size;
	unsigned stack_entry_size;

	static unsigned dump_pass;
	static unsigned dump_stat;

	static unsigned dry_run;
	static unsigned no_fallback;
	static unsigned safe_math;

	static unsigned dskip_start;
	static unsigned dskip_end;
	static unsigned dskip_mode;

	sb_context() : src_stats(), opt_stats(), isa(0),
			hw_chip(HW_CHIP_UNKNOWN), hw_class(HW_CLASS_UNKNOWN) {}

	int init(r600_isa *isa, sb_hw_chip chip, sb_hw_class cclass);

	bool is_r600() {return hw_class == HW_CLASS_R600;}
	bool is_r700() {return hw_class == HW_CLASS_R700;}
	bool is_evergreen() {return hw_class == HW_CLASS_EVERGREEN;}
	bool is_cayman() {return hw_class == HW_CLASS_CAYMAN;}
	bool is_egcm() {return hw_class >= HW_CLASS_EVERGREEN;}

	bool needs_8xx_stack_workaround() {
		if (!is_evergreen())
			return false;

		switch (hw_chip) {
		case HW_CHIP_CYPRESS:
		case HW_CHIP_JUNIPER:
			return false;
		default:
			return true;
		}
	}

	bool needs_9xx_stack_workaround() {
		return is_cayman();
	}

	sb_hw_class_bits hw_class_bit() {
		switch (hw_class) {
		case HW_CLASS_R600:return HB_R6;
		case HW_CLASS_R700:return HB_R7;
		case HW_CLASS_EVERGREEN:return HB_EG;
		case HW_CLASS_CAYMAN:return HB_CM;
		default: assert(!"unknown hw class"); return (sb_hw_class_bits)0;

		}
	}

	unsigned cf_opcode(unsigned op) {
		return r600_isa_cf_opcode(isa->hw_class, op);
	}

	unsigned alu_opcode(unsigned op) {
		return r600_isa_alu_opcode(isa->hw_class, op);
	}

	unsigned alu_slots(unsigned op) {
		return r600_isa_alu_slots(isa->hw_class, op);
	}

	unsigned alu_slots(const alu_op_info * op_ptr) {
		return op_ptr->slots[isa->hw_class];
	}

	unsigned alu_slots_mask(const alu_op_info * op_ptr) {
		unsigned mask = 0;
		unsigned slot_flags = alu_slots(op_ptr);
		if (slot_flags & AF_V)
			mask = 0x0F;
		if (!is_cayman() && (slot_flags & AF_S))
			mask |= 0x10;
		return mask;
	}

	unsigned fetch_opcode(unsigned op) {
		return r600_isa_fetch_opcode(isa->hw_class, op);
	}

	bool is_kcache_sel(unsigned sel) {
		return ((sel >= 128 && sel < 192) || (sel >= 256 && sel < 320));
	}

	const char * get_hw_class_name();
	const char * get_hw_chip_name();

};

#define SB_DUMP_STAT(a) do { if (sb_context::dump_stat) { a } } while (0)
#define SB_DUMP_PASS(a) do { if (sb_context::dump_pass) { a } } while (0)

class bc_decoder {

	sb_context &ctx;

	uint32_t* dw;
	unsigned ndw;

public:

	bc_decoder(sb_context &sctx, uint32_t *data, unsigned size)
		: ctx(sctx), dw(data), ndw(size) {}

	int decode_cf(unsigned &i, bc_cf &bc);
	int decode_alu(unsigned &i, bc_alu &bc);
	int decode_fetch(unsigned &i, bc_fetch &bc);

private:
	int decode_cf_alu(unsigned &i, bc_cf &bc);
	int decode_cf_exp(unsigned &i, bc_cf &bc);
	int decode_cf_mem(unsigned &i, bc_cf &bc);

	int decode_fetch_vtx(unsigned &i, bc_fetch &bc);
};

// bytecode format definition

class hw_encoding_format {
	const sb_hw_class_bits hw_target; //FIXME: debug - remove after testing
	hw_encoding_format();
protected:
	uint32_t value;
public:
	hw_encoding_format(sb_hw_class_bits hw)
		: hw_target(hw), value(0) {}
	hw_encoding_format(uint32_t v, sb_hw_class_bits hw)
		: hw_target(hw), value(v) {}
	uint32_t get_value(sb_hw_class_bits hw) const {
		assert((hw & hw_target) == hw);
		return value;
	}
};

#define BC_FORMAT_BEGIN_HW(fmt, hwset) \
class fmt##_##hwset : public hw_encoding_format {\
	typedef fmt##_##hwset thistype; \
public: \
	fmt##_##hwset() : hw_encoding_format(HB_##hwset) {}; \
	fmt##_##hwset(uint32_t v) : hw_encoding_format(v, HB_##hwset) {};

#define BC_FORMAT_BEGIN(fmt) BC_FORMAT_BEGIN_HW(fmt, ALL)

#define BC_FORMAT_END(fmt) };

// bytecode format field definition

#define BC_FIELD(fmt, name, shortname, last_bit, first_bit) \
	thistype & name(unsigned v) { \
		value |= ((v&((1ull<<((last_bit)-(first_bit)+1))-1))<<(first_bit)); \
		return *this; \
	} \
	unsigned get_##name() const { \
		return (value>>(first_bit))&((1ull<<((last_bit)-(first_bit)+1))-1); \
	} \

#define BC_RSRVD(fmt, last_bit, first_bit)

// CLAMP macro defined elsewhere interferes with bytecode field name
#undef CLAMP

#include "sb_bc_fmt_def.inc"

#undef BC_FORMAT_BEGIN
#undef BC_FORMAT_END
#undef BC_FIELD
#undef BC_RSRVD

class bc_parser {
	sb_context & ctx;

	bc_decoder *dec;

	r600_bytecode *bc;
	r600_shader *pshader;

	uint32_t *dw;
	unsigned bc_ndw;

	unsigned max_cf;

	shader *sh;

	int error;

	alu_node *slots[2][5];
	unsigned cgroup;

	typedef std::vector<cf_node*> id_cf_map;
	id_cf_map cf_map;

	typedef std::stack<region_node*> region_stack;
	region_stack loop_stack;

	bool gpr_reladdr;

public:

	bc_parser(sb_context &sctx, r600_bytecode *bc, r600_shader* pshader) :
		ctx(sctx), dec(), bc(bc), pshader(pshader),
		dw(), bc_ndw(), max_cf(),
		sh(), error(), slots(), cgroup(),
		cf_map(), loop_stack(), gpr_reladdr() { }

	int decode();
	int prepare();

	shader* get_shader() { assert(!error); return sh; }

private:

	int decode_shader();

	int parse_decls();

	int decode_cf(unsigned &i, bool &eop);

	int decode_alu_clause(cf_node *cf);
	int decode_alu_group(cf_node* cf, unsigned &i, unsigned &gcnt);

	int decode_fetch_clause(cf_node *cf);

	int prepare_ir();
	int prepare_alu_clause(cf_node *cf);
	int prepare_alu_group(cf_node* cf, alu_group_node *g);
	int prepare_fetch_clause(cf_node *cf);

	int prepare_loop(cf_node *c);
	int prepare_if(cf_node *c);

};




class bytecode {
	typedef std::vector<uint32_t> bc_vector;
	sb_hw_class_bits hw_class_bit;

	bc_vector bc;

	unsigned pos;

public:

	bytecode(sb_hw_class_bits hw, unsigned rdw = 256)
		: hw_class_bit(hw), pos(0) { bc.reserve(rdw); }

	unsigned ndw() { return bc.size(); }

	void write_data(uint32_t* dst) {
		std::copy(bc.begin(), bc.end(), dst);
	}

	void align(unsigned a) {
		unsigned size = bc.size();
		size = (size + a - 1) & ~(a-1);
		bc.resize(size);
	}

	void set_size(unsigned sz) {
		assert(sz >= bc.size());
		bc.resize(sz);
	}

	void seek(unsigned p) {
		if (p != pos) {
			if (p > bc.size()) {
				bc.resize(p);
			}
			pos = p;
		}
	}

	unsigned get_pos() { return pos; }
	uint32_t *data() { return &bc[0]; }

	bytecode & operator <<(uint32_t v) {
		if (pos == ndw()) {
			bc.push_back(v);
		} else
			bc.at(pos) = v;
		++pos;
		return *this;
	}

	bytecode & operator <<(const hw_encoding_format &e) {
		*this << e.get_value(hw_class_bit);
		return *this;
	}

	bytecode & operator <<(const bytecode &b) {
		bc.insert(bc.end(), b.bc.begin(), b.bc.end());
		return *this;
	}

	uint32_t at(unsigned dw_id) { return bc.at(dw_id); }
};


class bc_builder {
	shader &sh;
	sb_context &ctx;
	bytecode bb;
	int error;

public:

	bc_builder(shader &s);
	int build();
	bytecode& get_bytecode() { assert(!error); return bb; }

private:

	int build_cf(cf_node *n);

	int build_cf_alu(cf_node *n);
	int build_cf_mem(cf_node *n);
	int build_cf_exp(cf_node *n);

	int build_alu_clause(cf_node *n);
	int build_alu_group(alu_group_node *n);
	int build_alu(alu_node *n);

	int build_fetch_clause(cf_node *n);
	int build_fetch_tex(fetch_node *n);
	int build_fetch_vtx(fetch_node *n);
};

} // namespace r600_sb

#endif /* SB_BC_H_ */
