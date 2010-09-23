/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef TPF_H_
#define TPF_H_

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include "le32.h"

#include "tpf_defs.h"

extern const char* tpf_opcode_names[];
extern const char* tpf_file_names[];
extern const char* tpf_file_ms_names[];
extern const char* tpf_target_names[];
extern const char* tpf_interpolation_names[];
extern const char* tpf_sv_names[];

struct tpf_token_version
{
	unsigned minor : 4;
	unsigned major : 4;
	unsigned format : 8;
	unsigned type : 16;
};

struct tpf_token_instruction
{
	// we don't make it an union directly because unions can't be inherited from
	union
	{
		// length and extended are always present, but they are only here to reduce duplication
		struct
		{
			unsigned opcode : 11;
			unsigned _11_23 : 13;
			unsigned length : 7;
			unsigned extended : 1;
		};
		struct
		{
			unsigned opcode : 11;
			unsigned resinfo_return_type : 2;
			unsigned sat : 1;
			unsigned _14_17 : 4;
			unsigned test_nz : 1; // bit 18
			unsigned precise_mask : 4;
			unsigned _23 : 1;
			unsigned length : 7;
			unsigned extended : 1;
		} insn;
		struct
		{
			unsigned opcode : 11;
			unsigned threads_in_group : 1;
			unsigned shared_memory : 1;
			unsigned uav_group : 1;
			unsigned uav_global : 1;
			unsigned _15_17 : 3;
		} sync;
		struct
		{
			unsigned opcode : 11;
			unsigned allow_refactoring : 1;
			unsigned fp64 : 1;
			unsigned early_depth_stencil : 1;
			unsigned enable_raw_and_structured_in_non_cs : 1;
		} dcl_global_flags;
		struct
		{
			unsigned opcode : 11;
			unsigned target : 5;
			unsigned nr_samples : 7;
		} dcl_resource;
		struct
		{
			unsigned opcode : 11;
			unsigned shadow : 1;
			unsigned mono : 1;
		} dcl_sampler;
		struct
		{
			unsigned opcode : 11;
			unsigned interpolation : 5;
		} dcl_input_ps;
		struct
		{
			unsigned opcode : 11;
			unsigned dynamic : 1;
		} dcl_constant_buffer;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive : 6;
		} dcl_gs_input_primitive;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive_topology : 7;
		} dcl_gs_output_primitive_topology;
		struct
		{
			unsigned opcode : 11;
			unsigned control_points : 6;
		} dcl_input_control_point_count;
		struct
		{
			unsigned opcode : 11;
			unsigned control_points : 6;
		} dcl_output_control_point_count;
		struct
		{
			unsigned opcode : 11;
			unsigned domain : 3; /* D3D_TESSELLATOR_DOMAIN */
		} dcl_tess_domain;
		struct
		{
			unsigned opcode : 11;
			unsigned partitioning : 3; /* D3D_TESSELLATOR_PARTITIONING */
		} dcl_tess_partitioning;
		struct
		{
			unsigned opcode : 11;
			unsigned primitive : 3; /* D3D_TESSELLATOR_OUTPUT_PRIMITIVE */
		} dcl_tess_output_primitive;
	};
};

union  tpf_token_instruction_extended
{
	struct
	{
		unsigned type : 6;
		unsigned _6_30 : 25;
		unsigned extended :1;
	};
	struct
	{
		unsigned type : 6;
		unsigned _6_8 : 3;
		int offset_u : 4;
		int offset_v : 4;
		int offset_w : 4;
	} sample_controls;
	struct
	{
		unsigned type : 6;
		unsigned target : 5;
	} resource_target;
	struct
	{
		unsigned type : 6;
		unsigned x : 4;
		unsigned y : 4;
		unsigned z : 4;
		unsigned w : 4;
	} resource_return_type;
};

struct tpf_token_resource_return_type
{
	unsigned x : 4;
	unsigned y : 4;
	unsigned z : 4;
	unsigned w : 4;
};

struct tpf_token_operand
{
	unsigned comps_enum : 2; /* tpf_operands_comps */
	unsigned mode : 2; /* tpf_operand_mode */
	unsigned sel : 8;
	unsigned file : 8; /* tpf_file */
	unsigned num_indices : 2;
	unsigned index0_repr : 3; /* tpf_operand_index_repr */
	unsigned index1_repr : 3; /* tpf_operand_index_repr */
	unsigned index2_repr : 3; /* tpf_operand_index_repr */
	unsigned extended : 1;
};

#define TPF_OPERAND_SEL_MASK(sel) ((sel) & 0xf)
#define TPF_OPERAND_SEL_SWZ(sel, i) (((sel) >> ((i) * 2)) & 3)
#define TPF_OPERAND_SEL_SCALAR(sel) ((sel) & 3)

struct tpf_token_operand_extended
{
	unsigned type : 6;
	unsigned neg : 1;
	unsigned abs : 1;
};

union tpf_any
{
	double f64;
	float f32;
	int64_t i64;
	int32_t i32;
	uint64_t u64;
	int64_t u32;
};

struct tpf_op;
struct tpf_insn;
struct tpf_dcl;
struct tpf_program;
std::ostream& operator <<(std::ostream& out, const tpf_op& op);
std::ostream& operator <<(std::ostream& out, const tpf_insn& op);
std::ostream& operator <<(std::ostream& out, const tpf_dcl& op);
std::ostream& operator <<(std::ostream& out, const tpf_program& op);

struct tpf_op
{
	uint8_t mode;
	uint8_t comps;
	uint8_t mask;
	uint8_t num_indices;
	uint8_t swizzle[4];
	tpf_file file;
	tpf_any imm_values[4];
	bool neg;
	bool abs;
	struct
	{
		int64_t disp;
		std::auto_ptr<tpf_op> reg;
	} indices[3];

	bool is_index_simple(unsigned i) const
	{
		 return !indices[i].reg.get() && indices[i].disp >= 0 && (int64_t)(int32_t)indices[i].disp == indices[i].disp;
	}

	bool has_simple_index() const
	{
		return num_indices == 1 && is_index_simple(0);
	}

	tpf_op()
	{
		memset(this, 0, sizeof(*this));
	}

	void dump();

private:
	tpf_op(const tpf_op& op)
	{}
};

/* for sample_d */
#define TPF_MAX_OPS 6

struct tpf_insn : public tpf_token_instruction
{
	int8_t sample_offset[3];
	uint8_t resource_target;
	uint8_t resource_return_type[4];

	unsigned num;
	unsigned num_ops;
	std::auto_ptr<tpf_op> ops[TPF_MAX_OPS];

	tpf_insn()
	{
		memset(this, 0, sizeof(*this));
	}

	void dump();

private:
	tpf_insn(const tpf_insn& op)
	{}
};

struct tpf_dcl : public tpf_token_instruction
{
	std::auto_ptr<tpf_op> op;
	union
	{
		unsigned num;
		float f32;
		tpf_sv sv;
		struct
		{
			unsigned id;
			unsigned expected_function_table_length;
			unsigned table_length;
			unsigned array_length;
		} intf;
		unsigned thread_group_size[3];
		tpf_token_resource_return_type rrt;
		struct
		{
			unsigned num;
			unsigned comps;
		} indexable_temp;
		struct
		{
			unsigned stride;
			unsigned count;
		} structured;
	};

	void* data;

	tpf_dcl()
	{
		memset(this, 0, sizeof(*this));
	}

	~tpf_dcl()
	{
		free(data);
	}

	void dump();

private:
	tpf_dcl(const tpf_dcl& op)
	{}
};

struct tpf_program
{
	tpf_token_version version;
	std::vector<tpf_dcl*> dcls;
	std::vector<tpf_insn*> insns;

	/* for ifs, the insn number of the else or endif if there is no else
	 * for elses, the insn number of the endif
	 * for endifs, the insn number of the if
	 * for loops, the insn number of the endloop
	 * for endloops, the insn number of the loop
	 * for all others, -1
	 */
	std::vector<int> cf_insn_linked;

	/* NOTE: sampler 0 is the unnormalized nearest sampler for LD/LD_MS, while
	 * sampler 1 is user-specified sampler 0
	 */
	bool resource_sampler_slots_assigned;
	std::vector<int> slot_to_resource;
	std::vector<int> slot_to_sampler;
	std::map<std::pair<int, int>, int> resource_sampler_to_slot;
	std::map<int, int> resource_to_slot;

	bool labels_found;
	std::vector<int> label_to_insn_num;

	tpf_program()
	{
		memset(&version, 0, sizeof(version));
		labels_found = false;
		resource_sampler_slots_assigned = false;
	}

	~tpf_program()
	{
		for(std::vector<tpf_dcl*>::iterator i = dcls.begin(), e = dcls.end(); i != e; ++i)
			delete *i;
		for(std::vector<tpf_insn*>::iterator i = insns.begin(), e = insns.end(); i != e; ++i)
			delete *i;
	}

	void dump();

private:
	tpf_program(const tpf_dcl& op)
	{}
};

tpf_program* tpf_parse(void* tokens, int size);

bool tpf_link_cf_insns(tpf_program& program);
bool tpf_find_labels(tpf_program& program);
bool tpf_allocate_resource_sampler_pairs(tpf_program& program);

#endif /* TPF_H_ */

