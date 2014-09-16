/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#ifndef BRW_EU_H
#define BRW_EU_H

#include <stdbool.h>
#include "brw_inst.h"
#include "brw_structs.h"
#include "brw_defines.h"
#include "brw_reg.h"
#include "intel_asm_annotation.h"
#include "program/prog_instruction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BRW_EU_MAX_INSN_STACK 5

/* A helper for accessing the last instruction emitted.  This makes it easy
 * to set various bits on an instruction without having to create temporary
 * variable and assign the emitted instruction to those.
 */
#define brw_last_inst (&p->store[p->nr_insn - 1])

struct brw_compile {
   brw_inst *store;
   int store_size;
   unsigned nr_insn;
   unsigned int next_insn_offset;

   void *mem_ctx;

   /* Allow clients to push/pop instruction state:
    */
   brw_inst stack[BRW_EU_MAX_INSN_STACK];
   bool compressed_stack[BRW_EU_MAX_INSN_STACK];
   brw_inst *current;

   bool single_program_flow;
   bool compressed;
   struct brw_context *brw;

   /* Control flow stacks:
    * - if_stack contains IF and ELSE instructions which must be patched
    *   (and popped) once the matching ENDIF instruction is encountered.
    *
    *   Just store the instruction pointer(an index).
    */
   int *if_stack;
   int if_stack_depth;
   int if_stack_array_size;

   /**
    * loop_stack contains the instruction pointers of the starts of loops which
    * must be patched (and popped) once the matching WHILE instruction is
    * encountered.
    */
   int *loop_stack;
   /**
    * pre-gen6, the BREAK and CONT instructions had to tell how many IF/ENDIF
    * blocks they were popping out of, to fix up the mask stack.  This tracks
    * the IF/ENDIF nesting in each current nested loop level.
    */
   int *if_depth_in_loop;
   int loop_stack_depth;
   int loop_stack_array_size;
};

void brw_pop_insn_state( struct brw_compile *p );
void brw_push_insn_state( struct brw_compile *p );
void brw_set_default_mask_control( struct brw_compile *p, unsigned value );
void brw_set_default_saturate( struct brw_compile *p, bool enable );
void brw_set_default_access_mode( struct brw_compile *p, unsigned access_mode );
void brw_set_default_compression_control(struct brw_compile *p, enum brw_compression c);
void brw_set_default_predicate_control( struct brw_compile *p, unsigned pc );
void brw_set_default_predicate_inverse(struct brw_compile *p, bool predicate_inverse);
void brw_set_default_flag_reg(struct brw_compile *p, int reg, int subreg);
void brw_set_default_acc_write_control(struct brw_compile *p, unsigned value);

void brw_init_compile(struct brw_context *, struct brw_compile *p,
		      void *mem_ctx);
void brw_disassemble(struct brw_context *brw, void *assembly,
                     int start, int end, FILE *out);
const unsigned *brw_get_program( struct brw_compile *p, unsigned *sz );

brw_inst *brw_next_insn(struct brw_compile *p, unsigned opcode);
void brw_set_dest(struct brw_compile *p, brw_inst *insn, struct brw_reg dest);
void brw_set_src0(struct brw_compile *p, brw_inst *insn, struct brw_reg reg);

void gen6_resolve_implied_move(struct brw_compile *p,
			       struct brw_reg *src,
			       unsigned msg_reg_nr);

/* Helpers for regular instructions:
 */
#define ALU1(OP)				\
brw_inst *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,		\
	      struct brw_reg src0);

#define ALU2(OP)				\
brw_inst *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,		\
	      struct brw_reg src0,		\
	      struct brw_reg src1);

#define ALU3(OP)				\
brw_inst *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,		\
	      struct brw_reg src0,		\
	      struct brw_reg src1,		\
	      struct brw_reg src2);

#define ROUND(OP) \
void brw_##OP(struct brw_compile *p, struct brw_reg dest, struct brw_reg src0);

ALU1(MOV)
ALU2(SEL)
ALU1(NOT)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(SHR)
ALU2(SHL)
ALU2(ASR)
ALU1(F32TO16)
ALU1(F16TO32)
ALU2(ADD)
ALU2(AVG)
ALU2(MUL)
ALU1(FRC)
ALU1(RNDD)
ALU2(MAC)
ALU2(MACH)
ALU1(LZD)
ALU2(DP4)
ALU2(DPH)
ALU2(DP3)
ALU2(DP2)
ALU2(LINE)
ALU2(PLN)
ALU3(MAD)
ALU3(LRP)
ALU1(BFREV)
ALU3(BFE)
ALU2(BFI1)
ALU3(BFI2)
ALU1(FBH)
ALU1(FBL)
ALU1(CBIT)
ALU2(ADDC)
ALU2(SUBB)
ALU2(MAC)

ROUND(RNDZ)
ROUND(RNDE)

#undef ALU1
#undef ALU2
#undef ALU3
#undef ROUND


/* Helpers for SEND instruction:
 */
void brw_set_sampler_message(struct brw_compile *p,
                             brw_inst *insn,
                             unsigned binding_table_index,
                             unsigned sampler,
                             unsigned msg_type,
                             unsigned response_length,
                             unsigned msg_length,
                             unsigned header_present,
                             unsigned simd_mode,
                             unsigned return_format);

void brw_set_indirect_send_descriptor(struct brw_compile *p,
                                      brw_inst *insn,
                                      unsigned sfid,
                                      struct brw_reg descriptor);

void brw_set_dp_read_message(struct brw_compile *p,
			     brw_inst *insn,
			     unsigned binding_table_index,
			     unsigned msg_control,
			     unsigned msg_type,
			     unsigned target_cache,
			     unsigned msg_length,
                             bool header_present,
			     unsigned response_length);

void brw_set_dp_write_message(struct brw_compile *p,
			      brw_inst *insn,
			      unsigned binding_table_index,
			      unsigned msg_control,
			      unsigned msg_type,
			      unsigned msg_length,
			      bool header_present,
			      unsigned last_render_target,
			      unsigned response_length,
			      unsigned end_of_thread,
			      unsigned send_commit_msg);

void brw_urb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   unsigned msg_reg_nr,
		   struct brw_reg src0,
                   enum brw_urb_write_flags flags,
		   unsigned msg_length,
		   unsigned response_length,
		   unsigned offset,
		   unsigned swizzle);

void brw_ff_sync(struct brw_compile *p,
		   struct brw_reg dest,
		   unsigned msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   unsigned response_length,
		   bool eot);

void brw_svb_write(struct brw_compile *p,
                   struct brw_reg dest,
                   unsigned msg_reg_nr,
                   struct brw_reg src0,
                   unsigned binding_table_index,
                   bool   send_commit_msg);

void brw_fb_WRITE(struct brw_compile *p,
		  int dispatch_width,
		   struct brw_reg payload,
		   struct brw_reg implied_header,
		   unsigned msg_control,
		   unsigned binding_table_index,
		   unsigned msg_length,
		   unsigned response_length,
		   bool eot,
		   bool header_present);

void brw_SAMPLE(struct brw_compile *p,
		struct brw_reg dest,
		unsigned msg_reg_nr,
		struct brw_reg src0,
		unsigned binding_table_index,
		unsigned sampler,
		unsigned msg_type,
		unsigned response_length,
		unsigned msg_length,
		unsigned header_present,
		unsigned simd_mode,
		unsigned return_format);

void brw_adjust_sampler_state_pointer(struct brw_compile *p,
                                      struct brw_reg header,
                                      struct brw_reg sampler_index,
                                      struct brw_reg scratch);

void gen4_math(struct brw_compile *p,
	       struct brw_reg dest,
	       unsigned function,
	       unsigned msg_reg_nr,
	       struct brw_reg src,
	       unsigned precision );

void gen6_math(struct brw_compile *p,
	       struct brw_reg dest,
	       unsigned function,
	       struct brw_reg src0,
	       struct brw_reg src1);

void brw_oword_block_read(struct brw_compile *p,
			  struct brw_reg dest,
			  struct brw_reg mrf,
			  uint32_t offset,
			  uint32_t bind_table_index);

void brw_oword_block_read_scratch(struct brw_compile *p,
				  struct brw_reg dest,
				  struct brw_reg mrf,
				  int num_regs,
				  unsigned offset);

void brw_oword_block_write_scratch(struct brw_compile *p,
				   struct brw_reg mrf,
				   int num_regs,
				   unsigned offset);

void gen7_block_read_scratch(struct brw_compile *p,
                             struct brw_reg dest,
                             int num_regs,
                             unsigned offset);

void brw_shader_time_add(struct brw_compile *p,
                         struct brw_reg payload,
                         uint32_t surf_index);

/**
 * Return the generation-specific jump distance scaling factor.
 *
 * Given the number of instructions to jump, we need to scale by
 * some number to obtain the actual jump distance to program in an
 * instruction.
 */
static inline unsigned
brw_jump_scale(const struct brw_context *brw)
{
   /* Broadwell measures jump targets in bytes. */
   if (brw->gen >= 8)
      return 16;

   /* Ironlake and later measure jump targets in 64-bit data chunks (in order
    * (to support compaction), so each 128-bit instruction requires 2 chunks.
    */
   if (brw->gen >= 5)
      return 2;

   /* Gen4 simply uses the number of 128-bit instructions. */
   return 1;
}

/* If/else/endif.  Works by manipulating the execution flags on each
 * channel.
 */
brw_inst *brw_IF(struct brw_compile *p, unsigned execute_size);
brw_inst *gen6_IF(struct brw_compile *p, enum brw_conditional_mod conditional,
                  struct brw_reg src0, struct brw_reg src1);

void brw_ELSE(struct brw_compile *p);
void brw_ENDIF(struct brw_compile *p);

/* DO/WHILE loops:
 */
brw_inst *brw_DO(struct brw_compile *p, unsigned execute_size);

brw_inst *brw_WHILE(struct brw_compile *p);

brw_inst *brw_BREAK(struct brw_compile *p);
brw_inst *brw_CONT(struct brw_compile *p);
brw_inst *gen6_HALT(struct brw_compile *p);

/* Forward jumps:
 */
void brw_land_fwd_jump(struct brw_compile *p, int jmp_insn_idx);

brw_inst *brw_JMPI(struct brw_compile *p, struct brw_reg index,
                   unsigned predicate_control);

void brw_NOP(struct brw_compile *p);

/* Special case: there is never a destination, execution size will be
 * taken from src0:
 */
void brw_CMP(struct brw_compile *p,
	     struct brw_reg dest,
	     unsigned conditional,
	     struct brw_reg src0,
	     struct brw_reg src1);

void
brw_untyped_atomic(struct brw_compile *p,
                   struct brw_reg dest,
                   struct brw_reg payload,
                   unsigned atomic_op,
                   unsigned bind_table_index,
                   unsigned msg_length,
                   unsigned response_length);

void
brw_untyped_surface_read(struct brw_compile *p,
                         struct brw_reg dest,
                         struct brw_reg mrf,
                         unsigned bind_table_index,
                         unsigned msg_length,
                         unsigned response_length);

void
brw_pixel_interpolator_query(struct brw_compile *p,
                             struct brw_reg dest,
                             struct brw_reg mrf,
                             bool noperspective,
                             unsigned mode,
                             unsigned data,
                             unsigned msg_length,
                             unsigned response_length);

/***********************************************************************
 * brw_eu_util.c:
 */

void brw_copy_indirect_to_indirect(struct brw_compile *p,
				   struct brw_indirect dst_ptr,
				   struct brw_indirect src_ptr,
				   unsigned count);

void brw_copy_from_indirect(struct brw_compile *p,
			    struct brw_reg dst,
			    struct brw_indirect ptr,
			    unsigned count);

void brw_copy4(struct brw_compile *p,
	       struct brw_reg dst,
	       struct brw_reg src,
	       unsigned count);

void brw_copy8(struct brw_compile *p,
	       struct brw_reg dst,
	       struct brw_reg src,
	       unsigned count);

void brw_math_invert( struct brw_compile *p,
		      struct brw_reg dst,
		      struct brw_reg src);

void brw_set_src1(struct brw_compile *p, brw_inst *insn, struct brw_reg reg);

void brw_set_uip_jip(struct brw_compile *p);

enum brw_conditional_mod brw_swap_cmod(uint32_t cmod);

/* brw_eu_compact.c */
void brw_init_compaction_tables(struct brw_context *brw);
void brw_compact_instructions(struct brw_compile *p, int start_offset,
                              int num_annotations, struct annotation *annotation);
void brw_uncompact_instruction(struct brw_context *brw, brw_inst *dst,
                               brw_compact_inst *src);
bool brw_try_compact_instruction(struct brw_context *brw, brw_compact_inst *dst,
                                 brw_inst *src);

void brw_debug_compact_uncompact(struct brw_context *brw, brw_inst *orig,
                                 brw_inst *uncompacted);

static inline int
next_offset(const struct brw_context *brw, void *store, int offset)
{
   brw_inst *insn = (brw_inst *)((char *)store + offset);

   if (brw_inst_cmpt_control(brw, insn))
      return offset + 8;
   else
      return offset + 16;
}

#ifdef __cplusplus
}
#endif

#endif
