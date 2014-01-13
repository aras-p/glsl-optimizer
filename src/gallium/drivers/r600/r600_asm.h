/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 */
#ifndef R600_ASM_H
#define R600_ASM_H

#include "r600_pipe.h"
#include "r600_isa.h"

struct r600_bytecode_alu_src {
	unsigned			sel;
	unsigned			chan;
	unsigned			neg;
	unsigned			abs;
	unsigned			rel;
	unsigned			kc_bank;
	uint32_t			value;
};

struct r600_bytecode_alu_dst {
	unsigned			sel;
	unsigned			chan;
	unsigned			clamp;
	unsigned			write;
	unsigned			rel;
};

struct r600_bytecode_alu {
	struct list_head		list;
	struct r600_bytecode_alu_src		src[3];
	struct r600_bytecode_alu_dst		dst;
	unsigned			op;
	unsigned			last;
	unsigned			is_op3;
	unsigned			execute_mask;
	unsigned			update_pred;
	unsigned			pred_sel;
	unsigned			bank_swizzle;
	unsigned			bank_swizzle_force;
	unsigned			omod;
	unsigned                        index_mode;
};

struct r600_bytecode_tex {
	struct list_head		list;
	unsigned			op;
	unsigned			inst_mod;
	unsigned			resource_id;
	unsigned			src_gpr;
	unsigned			src_rel;
	unsigned			dst_gpr;
	unsigned			dst_rel;
	unsigned			dst_sel_x;
	unsigned			dst_sel_y;
	unsigned			dst_sel_z;
	unsigned			dst_sel_w;
	unsigned			lod_bias;
	unsigned			coord_type_x;
	unsigned			coord_type_y;
	unsigned			coord_type_z;
	unsigned			coord_type_w;
	int				offset_x;
	int				offset_y;
	int				offset_z;
	unsigned			sampler_id;
	unsigned			src_sel_x;
	unsigned			src_sel_y;
	unsigned			src_sel_z;
	unsigned			src_sel_w;
};

struct r600_bytecode_vtx {
	struct list_head		list;
	unsigned			op;
	unsigned			fetch_type;
	unsigned			buffer_id;
	unsigned			src_gpr;
	unsigned			src_sel_x;
	unsigned			mega_fetch_count;
	unsigned			dst_gpr;
	unsigned			dst_sel_x;
	unsigned			dst_sel_y;
	unsigned			dst_sel_z;
	unsigned			dst_sel_w;
	unsigned			use_const_fields;
	unsigned			data_format;
	unsigned			num_format_all;
	unsigned			format_comp_all;
	unsigned			srf_mode_all;
	unsigned			offset;
	unsigned			endian;
};

struct r600_bytecode_output {
	unsigned			array_base;
	unsigned			array_size;
	unsigned			comp_mask;
	unsigned			type;

	unsigned			op;

	unsigned			elem_size;
	unsigned			gpr;
	unsigned			swizzle_x;
	unsigned			swizzle_y;
	unsigned			swizzle_z;
	unsigned			swizzle_w;
	unsigned			burst_count;
	unsigned			index_gpr;
};

struct r600_bytecode_kcache {
	unsigned			bank;
	unsigned			mode;
	unsigned			addr;
};

struct r600_bytecode_cf {
	struct list_head		list;

	unsigned			op;
	unsigned			addr;
	unsigned			ndw;
	unsigned			id;
	unsigned			cond;
	unsigned			pop_count;
	unsigned			cf_addr; /* control flow addr */
	struct r600_bytecode_kcache		kcache[4];
	unsigned			r6xx_uses_waterfall;
	unsigned			eg_alu_extended;
	unsigned			barrier;
	unsigned			end_of_program;
	struct list_head		alu;
	struct list_head		tex;
	struct list_head		vtx;
	struct r600_bytecode_output		output;
	struct r600_bytecode_alu		*curr_bs_head;
	struct r600_bytecode_alu		*prev_bs_head;
	struct r600_bytecode_alu		*prev2_bs_head;
	unsigned isa[2];
};

#define FC_NONE				0
#define FC_IF				1
#define FC_LOOP				2
#define FC_REP				3
#define FC_PUSH_VPM			4
#define FC_PUSH_WQM			5

struct r600_cf_stack_entry {
	int				type;
	struct r600_bytecode_cf		*start;
	struct r600_bytecode_cf		**mid; /* used to store the else point */
	int				num_mid;
};

#define SQ_MAX_CALL_DEPTH 0x00000020

#define AR_HANDLE_NORMAL 0
#define AR_HANDLE_RV6XX 1 /* except RV670 */

struct r600_stack_info {
	/* current level of non-WQM PUSH operations
	 * (PUSH, PUSH_ELSE, ALU_PUSH_BEFORE) */
	int push;
	/* current level of WQM PUSH operations
	 * (PUSH, PUSH_ELSE, PUSH_WQM) */
	int push_wqm;
	/* current loop level */
	int loop;

	/* required depth */
	int max_entries;
	/* subentries per entry */
	int entry_size;
};

struct r600_bytecode {
	enum chip_class			chip_class;
	bool				has_compressed_msaa_texturing;
	int				type;
	struct list_head		cf;
	struct r600_bytecode_cf		*cf_last;
	unsigned			ndw;
	unsigned			ncf;
	unsigned			ngpr;
	unsigned			nstack;
	unsigned			nlds_dw;
	unsigned			nresource;
	unsigned			force_add_cf;
	uint32_t			*bytecode;
	uint32_t			fc_sp;
	struct r600_cf_stack_entry	fc_stack[32];
	struct r600_stack_info		stack;
	unsigned	ar_loaded;
	unsigned	ar_reg;
	unsigned	ar_chan;
	unsigned        ar_handling;
	unsigned        r6xx_nop_after_rel_dst;
	unsigned        debug_id;
	struct r600_isa* isa;
};

/* eg_asm.c */
int eg_bytecode_cf_build(struct r600_bytecode *bc, struct r600_bytecode_cf *cf);

/* r600_asm.c */
void r600_bytecode_init(struct r600_bytecode *bc,
			enum chip_class chip_class,
			enum radeon_family family,
			bool has_compressed_msaa_texturing);
void r600_bytecode_clear(struct r600_bytecode *bc);
int r600_bytecode_add_alu(struct r600_bytecode *bc,
		const struct r600_bytecode_alu *alu);
int r600_bytecode_add_vtx(struct r600_bytecode *bc,
		const struct r600_bytecode_vtx *vtx);
int r600_bytecode_add_tex(struct r600_bytecode *bc,
		const struct r600_bytecode_tex *tex);
int r600_bytecode_add_output(struct r600_bytecode *bc,
		const struct r600_bytecode_output *output);
int r600_bytecode_build(struct r600_bytecode *bc);
int r600_bytecode_add_cf(struct r600_bytecode *bc);
int r600_bytecode_add_cfinst(struct r600_bytecode *bc,
		unsigned op);
int r600_bytecode_add_alu_type(struct r600_bytecode *bc,
		const struct r600_bytecode_alu *alu, unsigned type);
void r600_bytecode_special_constants(uint32_t value,
		unsigned *sel, unsigned *neg);
void r600_bytecode_disasm(struct r600_bytecode *bc);
void r600_bytecode_alu_read(struct r600_bytecode *bc,
		struct r600_bytecode_alu *alu, uint32_t word0, uint32_t word1);

int cm_bytecode_add_cf_end(struct r600_bytecode *bc);

void *r600_create_vertex_fetch_shader(struct pipe_context *ctx,
				      unsigned count,
				      const struct pipe_vertex_element *elements);

/* r700_asm.c */
void r700_bytecode_cf_vtx_build(uint32_t *bytecode,
		const struct r600_bytecode_cf *cf);
int r700_bytecode_alu_build(struct r600_bytecode *bc,
		struct r600_bytecode_alu *alu, unsigned id);
void r700_bytecode_alu_read(struct r600_bytecode *bc,
		struct r600_bytecode_alu *alu, uint32_t word0, uint32_t word1);
void r600_bytecode_export_read(struct r600_bytecode *bc,
		struct r600_bytecode_output *output, uint32_t word0, uint32_t word1);
void eg_bytecode_export_read(struct r600_bytecode *bc,
		struct r600_bytecode_output *output, uint32_t word0, uint32_t word1);

void r600_vertex_data_type(enum pipe_format pformat, unsigned *format,
			   unsigned *num_format, unsigned *format_comp, unsigned *endian);
#endif
