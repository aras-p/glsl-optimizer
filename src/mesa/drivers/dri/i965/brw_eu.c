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


#include "brw_context.h"
#include "brw_defines.h"
#include "brw_eu.h"

#include "util/ralloc.h"

/**
 * Converts a BRW_REGISTER_TYPE_* enum to a short string (F, UD, and so on).
 *
 * This is different than reg_encoding from brw_disasm.c in that it operates
 * on the abstract enum values, rather than the generation-specific encoding.
 */
const char *
brw_reg_type_letters(unsigned type)
{
   const char *names[] = {
      [BRW_REGISTER_TYPE_UD] = "UD",
      [BRW_REGISTER_TYPE_D]  = "D",
      [BRW_REGISTER_TYPE_UW] = "UW",
      [BRW_REGISTER_TYPE_W]  = "W",
      [BRW_REGISTER_TYPE_F]  = "F",
      [BRW_REGISTER_TYPE_UB] = "UB",
      [BRW_REGISTER_TYPE_B]  = "B",
      [BRW_REGISTER_TYPE_UV] = "UV",
      [BRW_REGISTER_TYPE_V]  = "V",
      [BRW_REGISTER_TYPE_VF] = "VF",
      [BRW_REGISTER_TYPE_DF] = "DF",
      [BRW_REGISTER_TYPE_HF] = "HF",
      [BRW_REGISTER_TYPE_UQ] = "UQ",
      [BRW_REGISTER_TYPE_Q]  = "Q",
   };
   assert(type <= BRW_REGISTER_TYPE_UQ);
   return names[type];
}

/* Returns the corresponding conditional mod for swapping src0 and
 * src1 in e.g. CMP.
 */
enum brw_conditional_mod
brw_swap_cmod(uint32_t cmod)
{
   switch (cmod) {
   case BRW_CONDITIONAL_Z:
   case BRW_CONDITIONAL_NZ:
      return cmod;
   case BRW_CONDITIONAL_G:
      return BRW_CONDITIONAL_L;
   case BRW_CONDITIONAL_GE:
      return BRW_CONDITIONAL_LE;
   case BRW_CONDITIONAL_L:
      return BRW_CONDITIONAL_G;
   case BRW_CONDITIONAL_LE:
      return BRW_CONDITIONAL_GE;
   default:
      return BRW_CONDITIONAL_NONE;
   }
}

void brw_set_default_predicate_control( struct brw_compile *p, unsigned pc )
{
   brw_inst_set_pred_control(p->brw, p->current, pc);
}

void brw_set_default_predicate_inverse(struct brw_compile *p, bool predicate_inverse)
{
   brw_inst_set_pred_inv(p->brw, p->current, predicate_inverse);
}

void brw_set_default_flag_reg(struct brw_compile *p, int reg, int subreg)
{
   if (p->brw->gen >= 7)
      brw_inst_set_flag_reg_nr(p->brw, p->current, reg);

   brw_inst_set_flag_subreg_nr(p->brw, p->current, subreg);
}

void brw_set_default_access_mode( struct brw_compile *p, unsigned access_mode )
{
   brw_inst_set_access_mode(p->brw, p->current, access_mode);
}

void
brw_set_default_compression_control(struct brw_compile *p,
			    enum brw_compression compression_control)
{
   struct brw_context *brw = p->brw;

   p->compressed = (compression_control == BRW_COMPRESSION_COMPRESSED);

   if (brw->gen >= 6) {
      /* Since we don't use the SIMD32 support in gen6, we translate
       * the pre-gen6 compression control here.
       */
      switch (compression_control) {
      case BRW_COMPRESSION_NONE:
	 /* This is the "use the first set of bits of dmask/vmask/arf
	  * according to execsize" option.
	  */
         brw_inst_set_qtr_control(brw, p->current, GEN6_COMPRESSION_1Q);
	 break;
      case BRW_COMPRESSION_2NDHALF:
	 /* For SIMD8, this is "use the second set of 8 bits." */
         brw_inst_set_qtr_control(brw, p->current, GEN6_COMPRESSION_2Q);
	 break;
      case BRW_COMPRESSION_COMPRESSED:
	 /* For SIMD16 instruction compression, use the first set of 16 bits
	  * since we don't do SIMD32 dispatch.
	  */
         brw_inst_set_qtr_control(brw, p->current, GEN6_COMPRESSION_1H);
	 break;
      default:
         unreachable("not reached");
      }
   } else {
      brw_inst_set_qtr_control(brw, p->current, compression_control);
   }
}

void brw_set_default_mask_control( struct brw_compile *p, unsigned value )
{
   brw_inst_set_mask_control(p->brw, p->current, value);
}

void brw_set_default_saturate( struct brw_compile *p, bool enable )
{
   brw_inst_set_saturate(p->brw, p->current, enable);
}

void brw_set_default_acc_write_control(struct brw_compile *p, unsigned value)
{
   struct brw_context *brw = p->brw;

   if (brw->gen >= 6)
      brw_inst_set_acc_wr_control(p->brw, p->current, value);
}

void brw_push_insn_state( struct brw_compile *p )
{
   assert(p->current != &p->stack[BRW_EU_MAX_INSN_STACK-1]);
   memcpy(p->current + 1, p->current, sizeof(brw_inst));
   p->compressed_stack[p->current - p->stack] = p->compressed;
   p->current++;
}

void brw_pop_insn_state( struct brw_compile *p )
{
   assert(p->current != p->stack);
   p->current--;
   p->compressed = p->compressed_stack[p->current - p->stack];
}


/***********************************************************************
 */
void
brw_init_compile(struct brw_context *brw, struct brw_compile *p, void *mem_ctx)
{
   memset(p, 0, sizeof(*p));

   p->brw = brw;
   /*
    * Set the initial instruction store array size to 1024, if found that
    * isn't enough, then it will double the store size at brw_next_insn()
    * until out of memory.
    */
   p->store_size = 1024;
   p->store = rzalloc_array(mem_ctx, brw_inst, p->store_size);
   p->nr_insn = 0;
   p->current = p->stack;
   p->compressed = false;
   memset(p->current, 0, sizeof(p->current[0]));

   p->mem_ctx = mem_ctx;

   /* Some defaults?
    */
   brw_set_default_mask_control(p, BRW_MASK_ENABLE); /* what does this do? */
   brw_set_default_saturate(p, 0);
   brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);

   /* Set up control flow stack */
   p->if_stack_depth = 0;
   p->if_stack_array_size = 16;
   p->if_stack = rzalloc_array(mem_ctx, int, p->if_stack_array_size);

   p->loop_stack_depth = 0;
   p->loop_stack_array_size = 16;
   p->loop_stack = rzalloc_array(mem_ctx, int, p->loop_stack_array_size);
   p->if_depth_in_loop = rzalloc_array(mem_ctx, int, p->loop_stack_array_size);

   brw_init_compaction_tables(brw);
}


const unsigned *brw_get_program( struct brw_compile *p,
			       unsigned *sz )
{
   *sz = p->next_insn_offset;
   return (const unsigned *)p->store;
}

void
brw_disassemble(struct brw_context *brw,
                void *assembly, int start, int end, FILE *out)
{
   bool dump_hex = false;

   for (int offset = start; offset < end;) {
      brw_inst *insn = assembly + offset;
      brw_inst uncompacted;
      bool compacted = brw_inst_cmpt_control(brw, insn);
      if (0)
         fprintf(out, "0x%08x: ", offset);

      if (compacted) {
         brw_compact_inst *compacted = (void *)insn;
	 if (dump_hex) {
	    fprintf(out, "0x%08x 0x%08x                       ",
		    ((uint32_t *)insn)[1],
		    ((uint32_t *)insn)[0]);
	 }

	 brw_uncompact_instruction(brw, &uncompacted, compacted);
	 insn = &uncompacted;
	 offset += 8;
      } else {
	 if (dump_hex) {
	    fprintf(out, "0x%08x 0x%08x 0x%08x 0x%08x ",
		    ((uint32_t *)insn)[3],
		    ((uint32_t *)insn)[2],
		    ((uint32_t *)insn)[1],
		    ((uint32_t *)insn)[0]);
	 }
	 offset += 16;
      }

      brw_disassemble_inst(out, brw, insn, compacted);
   }
}
