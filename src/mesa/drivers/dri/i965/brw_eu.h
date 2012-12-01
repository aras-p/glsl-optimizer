/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
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
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
   

#ifndef BRW_EU_H
#define BRW_EU_H

#include <stdbool.h>
#include "brw_structs.h"
#include "brw_defines.h"
#include "brw_reg.h"
#include "program/prog_instruction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BRW_EU_MAX_INSN_STACK 5

struct brw_compile {
   struct brw_instruction *store;
   int store_size;
   GLuint nr_insn;
   unsigned int next_insn_offset;

   void *mem_ctx;

   /* Allow clients to push/pop instruction state:
    */
   struct brw_instruction stack[BRW_EU_MAX_INSN_STACK];
   bool compressed_stack[BRW_EU_MAX_INSN_STACK];
   struct brw_instruction *current;

   GLuint flag_value;
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

static INLINE struct brw_instruction *current_insn( struct brw_compile *p)
{
   return &p->store[p->nr_insn];
}

void brw_pop_insn_state( struct brw_compile *p );
void brw_push_insn_state( struct brw_compile *p );
void brw_set_mask_control( struct brw_compile *p, GLuint value );
void brw_set_saturate( struct brw_compile *p, bool enable );
void brw_set_access_mode( struct brw_compile *p, GLuint access_mode );
void brw_set_compression_control(struct brw_compile *p, enum brw_compression c);
void brw_set_predicate_control_flag_value( struct brw_compile *p, GLuint value );
void brw_set_predicate_control( struct brw_compile *p, GLuint pc );
void brw_set_predicate_inverse(struct brw_compile *p, bool predicate_inverse);
void brw_set_conditionalmod( struct brw_compile *p, GLuint conditional );
void brw_set_flag_reg(struct brw_compile *p, int reg, int subreg);
void brw_set_acc_write_control(struct brw_compile *p, GLuint value);

void brw_init_compile(struct brw_context *, struct brw_compile *p,
		      void *mem_ctx);
void brw_dump_compile(struct brw_compile *p, FILE *out, int start, int end);
const GLuint *brw_get_program( struct brw_compile *p, GLuint *sz );

struct brw_instruction *brw_next_insn(struct brw_compile *p, GLuint opcode);
void brw_set_dest(struct brw_compile *p, struct brw_instruction *insn,
		  struct brw_reg dest);
void brw_set_src0(struct brw_compile *p, struct brw_instruction *insn,
		  struct brw_reg reg);

void gen6_resolve_implied_move(struct brw_compile *p,
			       struct brw_reg *src,
			       GLuint msg_reg_nr);

/* Helpers for regular instructions:
 */
#define ALU1(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0);

#define ALU2(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1);

#define ALU3(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1,			\
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
ALU2(RSR)
ALU2(RSL)
ALU2(ASR)
ALU1(F32TO16)
ALU1(F16TO32)
ALU2(JMPI)
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

ROUND(RNDZ)
ROUND(RNDE)

#undef ALU1
#undef ALU2
#undef ALU3
#undef ROUND


/* Helpers for SEND instruction:
 */
void brw_set_sampler_message(struct brw_compile *p,
                             struct brw_instruction *insn,
                             GLuint binding_table_index,
                             GLuint sampler,
                             GLuint msg_type,
                             GLuint response_length,
                             GLuint msg_length,
                             GLuint header_present,
                             GLuint simd_mode,
                             GLuint return_format);

void brw_set_dp_read_message(struct brw_compile *p,
			     struct brw_instruction *insn,
			     GLuint binding_table_index,
			     GLuint msg_control,
			     GLuint msg_type,
			     GLuint target_cache,
			     GLuint msg_length,
                             bool header_present,
			     GLuint response_length);

void brw_set_dp_write_message(struct brw_compile *p,
			      struct brw_instruction *insn,
			      GLuint binding_table_index,
			      GLuint msg_control,
			      GLuint msg_type,
			      GLuint msg_length,
			      bool header_present,
			      GLuint last_render_target,
			      GLuint response_length,
			      GLuint end_of_thread,
			      GLuint send_commit_msg);

void brw_urb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   bool used,
		   GLuint msg_length,
		   GLuint response_length,
		   bool eot,
		   bool writes_complete,
		   GLuint offset,
		   GLuint swizzle);

void brw_ff_sync(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   GLuint response_length,
		   bool eot);

void brw_svb_write(struct brw_compile *p,
                   struct brw_reg dest,
                   GLuint msg_reg_nr,
                   struct brw_reg src0,
                   GLuint binding_table_index,
                   bool   send_commit_msg);

void brw_fb_WRITE(struct brw_compile *p,
		  int dispatch_width,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   GLuint msg_control,
		   GLuint binding_table_index,
		   GLuint msg_length,
		   GLuint response_length,
		   bool eot,
		   bool header_present);

void brw_SAMPLE(struct brw_compile *p,
		struct brw_reg dest,
		GLuint msg_reg_nr,
		struct brw_reg src0,
		GLuint binding_table_index,
		GLuint sampler,
		GLuint msg_type,
		GLuint response_length,
		GLuint msg_length,
		GLuint header_present,
		GLuint simd_mode,
		GLuint return_format);

void brw_math( struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       GLuint msg_reg_nr,
	       struct brw_reg src,
	       GLuint data_type,
	       GLuint precision );

void brw_math2(struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
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
				  GLuint offset);

void brw_oword_block_write_scratch(struct brw_compile *p,
				   struct brw_reg mrf,
				   int num_regs,
				   GLuint offset);

void brw_shader_time_add(struct brw_compile *p,
                         struct brw_reg payload,
                         uint32_t surf_index);

/* If/else/endif.  Works by manipulating the execution flags on each
 * channel.
 */
struct brw_instruction *brw_IF(struct brw_compile *p, 
			       GLuint execute_size);
struct brw_instruction *gen6_IF(struct brw_compile *p, uint32_t conditional,
				struct brw_reg src0, struct brw_reg src1);

void brw_ELSE(struct brw_compile *p);
void brw_ENDIF(struct brw_compile *p);

/* DO/WHILE loops:
 */
struct brw_instruction *brw_DO(struct brw_compile *p,
			       GLuint execute_size);

struct brw_instruction *brw_WHILE(struct brw_compile *p);

struct brw_instruction *brw_BREAK(struct brw_compile *p);
struct brw_instruction *brw_CONT(struct brw_compile *p);
struct brw_instruction *gen6_CONT(struct brw_compile *p);
struct brw_instruction *gen6_HALT(struct brw_compile *p);
/* Forward jumps:
 */
void brw_land_fwd_jump(struct brw_compile *p, int jmp_insn_idx);



void brw_NOP(struct brw_compile *p);

void brw_WAIT(struct brw_compile *p);

/* Special case: there is never a destination, execution size will be
 * taken from src0:
 */
void brw_CMP(struct brw_compile *p,
	     struct brw_reg dest,
	     GLuint conditional,
	     struct brw_reg src0,
	     struct brw_reg src1);

/*********************************************************************** 
 * brw_eu_util.c:
 */

void brw_copy_indirect_to_indirect(struct brw_compile *p,
				   struct brw_indirect dst_ptr,
				   struct brw_indirect src_ptr,
				   GLuint count);

void brw_copy_from_indirect(struct brw_compile *p,
			    struct brw_reg dst,
			    struct brw_indirect ptr,
			    GLuint count);

void brw_copy4(struct brw_compile *p,
	       struct brw_reg dst,
	       struct brw_reg src,
	       GLuint count);

void brw_copy8(struct brw_compile *p,
	       struct brw_reg dst,
	       struct brw_reg src,
	       GLuint count);

void brw_math_invert( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg src);

void brw_set_src1(struct brw_compile *p,
		  struct brw_instruction *insn,
		  struct brw_reg reg);

void brw_set_uip_jip(struct brw_compile *p);

uint32_t brw_swap_cmod(uint32_t cmod);

/* brw_eu_compact.c */
void brw_init_compaction_tables(struct intel_context *intel);
void brw_compact_instructions(struct brw_compile *p);
void brw_uncompact_instruction(struct intel_context *intel,
			       struct brw_instruction *dst,
			       struct brw_compact_instruction *src);
bool brw_try_compact_instruction(struct brw_compile *p,
                                 struct brw_compact_instruction *dst,
                                 struct brw_instruction *src);

void brw_debug_compact_uncompact(struct intel_context *intel,
				 struct brw_instruction *orig,
				 struct brw_instruction *uncompacted);

#ifdef __cplusplus
}
#endif

#endif
