/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/



/**
 * Generate SPU fragment program/shader code.
 *
 * Note that we generate SOA-style code here.  So each TGSI instruction
 * operates on four pixels (and is translated into four SPU instructions,
 * generally speaking).
 *
 * \author Brian Paul
 */


#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_dump.h"
#include "rtasm/rtasm_ppc_spe.h"
#include "util/u_memory.h"
#include "cell_context.h"
#include "cell_gen_fp.h"


#define MAX_TEMPS 16
#define MAX_IMMED  8

#define CHAN_X  0
#define CHAN_Y  1
#define CHAN_Z  2
#define CHAN_W  3

/**
 * Context needed during code generation.
 */
struct codegen
{
   int inputs_reg;      /**< 1st function parameter */
   int outputs_reg;     /**< 2nd function parameter */
   int constants_reg;   /**< 3rd function parameter */
   int temp_regs[MAX_TEMPS][4]; /**< maps TGSI temps to SPE registers */
   int imm_regs[MAX_IMMED][4];  /**< maps TGSI immediates to SPE registers */

   int num_imm;  /**< number of immediates */

   int one_reg;         /**< register containing {1.0, 1.0, 1.0, 1.0} */

   /** Per-instruction temps / intermediate temps */
   int num_itemps;
   int itemps[10];

   /** Current IF/ELSE/ENDIF nesting level */
   int if_nesting;
   /** Index of execution mask register */
   int exec_mask_reg;

   struct spe_function *f;
   boolean error;
};


/**
 * Allocate an intermediate temporary register.
 */
static int
get_itemp(struct codegen *gen)
{
   int t = spe_allocate_available_register(gen->f);
   assert(gen->num_itemps < Elements(gen->itemps));
   gen->itemps[gen->num_itemps++] = t;
   return t;
}

/**
 * Free all intermediate temporary registers.  To be called after each
 * instruction has been emitted.
 */
static void
free_itemps(struct codegen *gen)
{
   int i;
   for (i = 0; i < gen->num_itemps; i++) {
      spe_release_register(gen->f, gen->itemps[i]);
   }
   gen->num_itemps = 0;
}


/**
 * Return index of an SPE register containing {1.0, 1.0, 1.0, 1.0}.
 * The register is allocated and initialized upon the first call.
 */
static int
get_const_one_reg(struct codegen *gen)
{
   if (gen->one_reg <= 0) {
      gen->one_reg = spe_allocate_available_register(gen->f);

      spe_indent(gen->f, 4);
      spe_comment(gen->f, -4, "INIT CONSTANT 1.0:");

      /* one = {1.0, 1.0, 1.0, 1.0} */
      spe_load_float(gen->f, gen->one_reg, 1.0f);

      spe_indent(gen->f, -4);
   }

   return gen->one_reg;
}


/**
 * Return index of the pixel execution mask.
 * The register is allocated an initialized upon the first call.
 *
 * The pixel execution mask controls which pixels in a quad are
 * modified, according to surrounding conditionals, loops, etc.
 */
static int
get_exec_mask_reg(struct codegen *gen)
{
   if (gen->exec_mask_reg <= 0) {
      gen->exec_mask_reg = spe_allocate_available_register(gen->f);

      spe_indent(gen->f, 4);
      spe_comment(gen->f, -4, "INIT EXEC MASK = ~0:");

      /* exec_mask = {~0, ~0, ~0, ~0} */
      spe_load_int(gen->f, gen->exec_mask_reg, ~0);

      spe_indent(gen->f, -4);
   }

   return gen->exec_mask_reg;
}


/**
 * Return the index of the SPU temporary containing the named TGSI
 * source register.  If the TGSI register is a TGSI_FILE_TEMPORARY we
 * just return the corresponding SPE register.  If the TGIS register
 * is TGSI_FILE_INPUT/CONSTANT/IMMEDIATE we allocate a new SPE register
 * and emit an SPE load instruction.
 */
static int
get_src_reg(struct codegen *gen,
            int channel,
            const struct tgsi_full_src_register *src)
{
   int reg = -1;
   int swizzle = tgsi_util_get_full_src_register_extswizzle(src, channel);
   boolean reg_is_itemp = FALSE;
   uint sign_op;

   assert(swizzle >= 0);
   assert(swizzle <= 3);

   channel = swizzle;

   switch (src->SrcRegister.File) {
   case TGSI_FILE_TEMPORARY:
      reg = gen->temp_regs[src->SrcRegister.Index][channel];
      break;
   case TGSI_FILE_INPUT:
      {
         /* offset is measured in quadwords, not bytes */
         int offset = src->SrcRegister.Index * 4 + channel;
         reg = get_itemp(gen);
         reg_is_itemp = TRUE;
         /* Load:  reg = memory[(machine_reg) + offset] */
         spe_lqd(gen->f, reg, gen->inputs_reg, offset);
      }
      break;
   case TGSI_FILE_IMMEDIATE:
      reg = gen->imm_regs[src->SrcRegister.Index][channel];
      break;
   case TGSI_FILE_CONSTANT:
      /* xxx fall-through for now / fix */
   default:
      assert(0);
   }

   /*
    * Handle absolute value, negate or set-negative of src register.
    */
   sign_op = tgsi_util_get_full_src_register_sign_mode(src, channel);
   if (sign_op != TGSI_UTIL_SIGN_KEEP) {
      /*
       * All sign ops are done by manipulating bit 31, the IEEE float sign bit.
       */
      const int bit31mask_reg = get_itemp(gen);
      int result_reg;

      if (reg_is_itemp) {
         /* re-use 'reg' for the result */
         result_reg = reg;
      }
      else {
         /* alloc a new reg for the result */
         result_reg = get_itemp(gen);
      }

      /* mask with bit 31 set, the rest cleared */
      spe_load_int(gen->f, bit31mask_reg, (1 << 31));

      if (sign_op == TGSI_UTIL_SIGN_CLEAR) {
         spe_andc(gen->f, result_reg, reg, bit31mask_reg);
      }
      else if (sign_op == TGSI_UTIL_SIGN_SET) {
         spe_and(gen->f, result_reg, reg, bit31mask_reg);
      }
      else {
         assert(sign_op == TGSI_UTIL_SIGN_TOGGLE);
         spe_xor(gen->f, result_reg, reg, bit31mask_reg);
      }

      reg = result_reg;
   }

   return reg;
}


/**
 * Return the index of an SPE register to use for the given TGSI register.
 * If the TGSI register is TGSI_FILE_TEMPORARAY, the index of the
 * corresponding SPE register is returned.  If the TGSI register is
 * TGSI_FILE_OUTPUT we allocate an intermediate temporary register.
 * See store_dest_reg() below...
 */
static int
get_dst_reg(struct codegen *gen,
            int channel,
            const struct tgsi_full_dst_register *dest)
{
   int reg = -1;

   switch (dest->DstRegister.File) {
   case TGSI_FILE_TEMPORARY:
      if (gen->if_nesting > 0)
         reg = get_itemp(gen);
      else
         reg = gen->temp_regs[dest->DstRegister.Index][channel];
      break;
   case TGSI_FILE_OUTPUT:
      reg = get_itemp(gen);
      break;
   default:
      assert(0);
   }

   return reg;
}


/**
 * When a TGSI instruction is writing to an output register, this
 * function emits the SPE store instruction to store the value_reg.
 * \param value_reg  the SPE register containing the value to store.
 *                   This would have been returned by get_dst_reg().
 */
static void
store_dest_reg(struct codegen *gen,
               int value_reg, int channel,
               const struct tgsi_full_dst_register *dest)
{
   switch (dest->DstRegister.File) {
   case TGSI_FILE_TEMPORARY:
      if (gen->if_nesting > 0) {
         int d_reg = gen->temp_regs[dest->DstRegister.Index][channel];
         int exec_reg = get_exec_mask_reg(gen);
         /* Mix d with new value according to exec mask:
          * d[i] = mask_reg[i] ? value_reg : d_reg
          */
         spe_selb(gen->f, d_reg, d_reg, value_reg, exec_reg);
      }
      else {
         /* we're not inside a condition or loop: do nothing special */
      }
      break;
   case TGSI_FILE_OUTPUT:
      {
         /* offset is measured in quadwords, not bytes */
         int offset = dest->DstRegister.Index * 4 + channel;
         if (gen->if_nesting > 0) {
            int exec_reg = get_exec_mask_reg(gen);
            int curval_reg = get_itemp(gen);
            /* First read the current value from memory:
             * Load:  curval = memory[(machine_reg) + offset]
             */
            spe_lqd(gen->f, curval_reg, gen->outputs_reg, offset);
            /* Mix curval with newvalue according to exec mask:
             * d[i] = mask_reg[i] ? value_reg : d_reg
             */
            spe_selb(gen->f, curval_reg, curval_reg, value_reg, exec_reg);
            /* Store: memory[(machine_reg) + offset] = curval */
            spe_stqd(gen->f, curval_reg, gen->outputs_reg, offset);
         }
         else {
            /* Store: memory[(machine_reg) + offset] = reg */
            spe_stqd(gen->f, value_reg, gen->outputs_reg, offset);
         }
      }
      break;
   default:
      assert(0);
   }
}


static boolean
emit_MOV(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "MOV:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int src_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int dst_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* XXX we don't always need to actually emit a mov instruction here */
         spe_move(gen->f, dst_reg, src_reg);
         store_dest_reg(gen, dst_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
   return true;
}



/**
 * Emit addition instructions.  Recall that a single TGSI_OPCODE_ADD
 * becomes (up to) four SPU "fa" instructions because we're doing SOA
 * processing.
 */
static boolean
emit_ADD(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "ADD:");
   /* Loop over Red/Green/Blue/Alpha channels */
   for (ch = 0; ch < 4; ch++) {
      /* If the dest R, G, B or A writemask is enabled... */
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         /* get indexes of the two src, one dest SPE registers */
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* Emit actual SPE instruction: d = s1 + s2 */
         spe_fa(gen->f, d_reg, s1_reg, s2_reg);

         /* Store the result (a no-op for TGSI_FILE_TEMPORARY dests) */
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         /* Free any intermediate temps we allocated */
         free_itemps(gen);
      }
   }
   return true;
}

/**
 * Emit subtract.  See emit_ADD for comments.
 */
static boolean
emit_SUB(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "SUB:");
   /* Loop over Red/Green/Blue/Alpha channels */
   for (ch = 0; ch < 4; ch++) {
      /* If the dest R, G, B or A writemask is enabled... */
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         /* get indexes of the two src, one dest SPE registers */
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* Emit actual SPE instruction: d = s1 - s2 */
         spe_fs(gen->f, d_reg, s1_reg, s2_reg);

         /* Store the result (a no-op for TGSI_FILE_TEMPORARY dests) */
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         /* Free any intermediate temps we allocated */
         free_itemps(gen);
      }
   }
   return true;
}

/**
 * Emit multiply add.  See emit_ADD for comments.
 */
static boolean
emit_MAD(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "MAD:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int s3_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[2]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* d = s1 * s2 + s3 */
         spe_fma(gen->f, d_reg, s1_reg, s2_reg, s3_reg);
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
   return true;
}


/**
 * Emit linear interpolate.  See emit_ADD for comments.
 */
static boolean
emit_LERP(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "LERP:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int s3_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[2]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* d = s3 + s1(s2 - s3) */
         spe_fs(gen->f, d_reg, s2_reg, s3_reg);
         spe_fma(gen->f, d_reg, d_reg, s1_reg, s3_reg);
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
   return true;
}

/**
 * Emit multiply.  See emit_ADD for comments.
 */
static boolean
emit_MUL(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "MUL:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* d = s1 * s2 */
         spe_fm(gen->f, d_reg, s1_reg, s2_reg);
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
   return true;
}

/**
 * Emit reciprocal.  See emit_ADD for comments.
 */
static boolean
emit_RCP(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "RCP:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* d = 1/s1 */
         spe_frest(gen->f, d_reg, s1_reg);
         spe_fi(gen->f, d_reg, s1_reg, d_reg);
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
   return true;
}

/**
 * Emit reciprocal sqrt.  See emit_ADD for comments.
 */
static boolean
emit_RSQ(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "RSQ:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* d = 1/s1 */
         spe_frsqest(gen->f, d_reg, s1_reg);
         spe_fi(gen->f, d_reg, s1_reg, d_reg);
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
   return true;
}

/**
 * Emit absolute value.  See emit_ADD for comments.
 */
static boolean
emit_ABS(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "ABS:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         const int bit31mask_reg = get_itemp(gen);

         /* mask with bit 31 set, the rest cleared */  
         spe_load_int(gen->f, bit31mask_reg, (1 << 31));

         /* d = sign bit cleared in s1 */
         spe_andc(gen->f, d_reg, s1_reg, bit31mask_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
   return true;
}

/**
 * Emit 3 component dot product.  See emit_ADD for comments.
 */
static boolean
emit_DP3(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "DP3:");

   int s1_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   int s2_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[1]);
   int d_reg = get_dst_reg(gen, CHAN_X, &inst->FullDstRegisters[0]);
   /* d = x * x */
   spe_fm(gen->f, d_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[1]);
   /* d = y * y + d */
   spe_fma(gen->f, d_reg, s1_reg, s2_reg, d_reg);

   s1_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[1]);
   /* d = z * z + d */
   spe_fma(gen->f, d_reg, s1_reg, s2_reg, d_reg);

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
      }
   }

   free_itemps(gen);
   return true;
}

/**
 * Emit 4 component dot product.  See emit_ADD for comments.
 */
static boolean
emit_DP4(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   spe_comment(gen->f, -4, "DP3:");

   int s1_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   int s2_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[1]);
   int d_reg = get_dst_reg(gen, CHAN_X, &inst->FullDstRegisters[0]);
   /* d = x * x */
   spe_fm(gen->f, d_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[1]);
   /* d = y * y + d */
   spe_fma(gen->f, d_reg, s1_reg, s2_reg, d_reg);

   s1_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[1]);
   /* d = z * z + d */
   spe_fma(gen->f, d_reg, s1_reg, s2_reg, d_reg);

   s1_reg = get_src_reg(gen, CHAN_W, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_W, &inst->FullSrcRegisters[1]);
   /* d = w * w + d */
   spe_fma(gen->f, d_reg, s1_reg, s2_reg, d_reg);

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
      }
   }

   free_itemps(gen);
   return true;
}

/**
 * Emit set-if-greater-than.
 * Note that the SPE fcgt instruction produces 0x0 and 0xffffffff as
 * the result but OpenGL/TGSI needs 0.0 and 1.0 results.
 * We can easily convert 0x0/0xffffffff to 0.0/1.0 with a bitwise AND.
 */
static boolean
emit_SGT(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "SGT:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 > s2) */
         spe_fcgt(gen->f, d_reg, s1_reg, s2_reg);

         /* convert d from 0x0/0xffffffff to 0.0/1.0 */
         /* d = d & one_reg */
         spe_and(gen->f, d_reg, d_reg, get_const_one_reg(gen));

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit set-if_less-then.  See emit_SGT for comments.
 */
static boolean
emit_SLT(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "SLT:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 < s2) */
         spe_fcgt(gen->f, d_reg, s2_reg, s1_reg);

         /* convert d from 0x0/0xffffffff to 0.0/1.0 */
         /* d = d & one_reg */
         spe_and(gen->f, d_reg, d_reg, get_const_one_reg(gen));

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit set-if_greater-then-or-equal.  See emit_SGT for comments.
 */
static boolean
emit_SGE(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "SGE:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 >= s2) */
         spe_fcgt(gen->f, d_reg, s2_reg, s1_reg);

         /* convert d from 0x0/0xffffffff to 0.0/1.0 */
         /* d = ~d & one_reg */
         spe_andc(gen->f, d_reg, get_const_one_reg(gen), d_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit set-if_less-then-or-equal.  See emit_SGT for comments.
 */
static boolean
emit_SLE(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "SLE:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 <= s2) */
         spe_fcgt(gen->f, d_reg, s1_reg, s2_reg);

         /* convert d from 0x0/0xffffffff to 0.0/1.0 */
         /* d = ~d & one_reg */
         spe_andc(gen->f, d_reg, get_const_one_reg(gen), d_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit set-if_equal.  See emit_SGT for comments.
 */
static boolean
emit_SEQ(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "SEQ:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 == s2) */
         spe_fceq(gen->f, d_reg, s1_reg, s2_reg);

         /* convert d from 0x0/0xffffffff to 0.0/1.0 */
         /* d = d & one_reg */
         spe_and(gen->f, d_reg, d_reg, get_const_one_reg(gen));

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit set-if_not_equal.  See emit_SGT for comments.
 */
static boolean
emit_SNE(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "SNE:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 != s2) */
         spe_fceq(gen->f, d_reg, s1_reg, s2_reg);
         spe_nor(gen->f, d_reg, d_reg, d_reg);

         /* convert d from 0x0/0xffffffff to 0.0/1.0 */
         /* d = d & one_reg */
         spe_and(gen->f, d_reg, d_reg, get_const_one_reg(gen));

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit compare.  See emit_SGT for comments.
 */
static boolean
emit_CMP(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "CMP:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int s3_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[2]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         int zero_reg = get_itemp(gen);
   
         spe_xor(gen->f, zero_reg, zero_reg, zero_reg);

         /* d = (s1 < 0) ? s2 : s3 */
         spe_fcgt(gen->f, d_reg, zero_reg, s1_reg);
         spe_selb(gen->f, d_reg, s3_reg, s2_reg, d_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit floor.  
 * If negative int subtract one
 * Convert float to signed int
 * Convert signed int to float
 */
static boolean
emit_FLR(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "FLR:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         int tmp_reg = get_itemp(gen);

         /* If negative, subtract 1.0 */
         spe_xor(gen->f, tmp_reg, tmp_reg, tmp_reg);
         spe_fcgt(gen->f, d_reg, tmp_reg, s1_reg);
         spe_selb(gen->f, tmp_reg, tmp_reg, get_const_one_reg(gen), d_reg);
         spe_fs(gen->f, d_reg, s1_reg, tmp_reg);

         /* Convert float to int */
         spe_cflts(gen->f, d_reg, d_reg, 0);

         /* Convert int to float */
         spe_csflt(gen->f, d_reg, d_reg, 0);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit frac.  
 * Input - FLR(Input)
 */
static boolean
emit_FRC(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "FLR:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         int tmp_reg = get_itemp(gen);

         /* If negative, subtract 1.0 */
         spe_xor(gen->f, tmp_reg, tmp_reg, tmp_reg);
         spe_fcgt(gen->f, d_reg, tmp_reg, s1_reg);
         spe_selb(gen->f, tmp_reg, tmp_reg, get_const_one_reg(gen), d_reg);
         spe_fs(gen->f, d_reg, s1_reg, tmp_reg);

         /* Convert float to int */
         spe_cflts(gen->f, d_reg, d_reg, 0);

         /* Convert int to float */
         spe_csflt(gen->f, d_reg, d_reg, 0);

         /* d = s1 - FLR(s1) */
         spe_fs(gen->f, d_reg, s1_reg, d_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}


/**
 * Emit max.  See emit_SGT for comments.
 */
static boolean
emit_MAX(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "MAX:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 > s2) ? s1 : s2 */
         spe_fcgt(gen->f, d_reg, s1_reg, s2_reg);
         spe_selb(gen->f, d_reg, s2_reg, s1_reg, d_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Emit max.  See emit_SGT for comments.
 */
static boolean
emit_MIN(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "MIN:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s2 > s1) ? s1 : s2 */
         spe_fcgt(gen->f, d_reg, s2_reg, s1_reg);
         spe_selb(gen->f, d_reg, s2_reg, s1_reg, d_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

static boolean
emit_IF(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const int channel = 0;
   const int exec_reg = get_exec_mask_reg(gen);

   spe_comment(gen->f, -4, "IF:");

   /* update execution mask with the predicate register */
   int tmp_reg = get_itemp(gen);
   int s1_reg = get_src_reg(gen, channel, &inst->FullSrcRegisters[0]);

   /* tmp = (s1_reg == 0) */
   spe_ceqi(gen->f, tmp_reg, s1_reg, 0);
   /* tmp = !tmp */
   spe_complement(gen->f, tmp_reg, tmp_reg);
   /* exec_mask = exec_mask & tmp */
   spe_and(gen->f, exec_reg, exec_reg, tmp_reg);

   gen->if_nesting++;

   free_itemps(gen);

   return true;
}


static boolean
emit_ELSE(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const int exec_reg = get_exec_mask_reg(gen);

   spe_comment(gen->f, -4, "ELSE:");

   /* exec_mask = !exec_mask */
   spe_complement(gen->f, exec_reg, exec_reg);

   return true;
}


static boolean
emit_ENDIF(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const int exec_reg = get_exec_mask_reg(gen);

   spe_comment(gen->f, -4, "ENDIF:");

   /* XXX todo: pop execution mask */

   spe_load_int(gen->f, exec_reg, ~0x0);

   gen->if_nesting--;
   return true;
}


static boolean
emit_DDX_DDY(struct codegen *gen, const struct tgsi_full_instruction *inst,
             boolean ddx)
{
   int ch;

   spe_comment(gen->f, -4, ddx ? "DDX:" : "DDY:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         int t1_reg = get_itemp(gen);
         int t2_reg = get_itemp(gen);

         spe_splat_word(gen->f, t1_reg, s_reg, 0); /* upper-left pixel */
         if (ddx) {
            spe_splat_word(gen->f, t2_reg, s_reg, 1); /* upper-right pixel */
         }
         else {
            spe_splat_word(gen->f, t2_reg, s_reg, 2); /* lower-left pixel */
         }
         spe_fs(gen->f, d_reg, t2_reg, t1_reg);

         free_itemps(gen);
      }
   }

   return true;
}




/**
 * Emit END instruction.
 * We just return from the shader function at this point.
 *
 * Note that there may be more code after this that would be
 * called by TGSI_OPCODE_CALL.
 */
static boolean
emit_END(struct codegen *gen)
{
   spe_comment(gen->f, -4, "END:");
   /* return from function call */
   spe_bi(gen->f, SPE_REG_RA, 0, 0);
   return true;
}


/**
 * Emit code for the given instruction.  Just a big switch stmt.
 */
static boolean
emit_instruction(struct codegen *gen,
                 const struct tgsi_full_instruction *inst)
{
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_MOV:
      return emit_MOV(gen, inst);
   case TGSI_OPCODE_MUL:
      return emit_MUL(gen, inst);
   case TGSI_OPCODE_ADD:
      return emit_ADD(gen, inst);
   case TGSI_OPCODE_SUB:
      return emit_SUB(gen, inst);
   case TGSI_OPCODE_MAD:
      return emit_MAD(gen, inst);
   case TGSI_OPCODE_LERP:
      return emit_LERP(gen, inst);
   case TGSI_OPCODE_DP3:
      return emit_DP3(gen, inst);
   case TGSI_OPCODE_DP4:
      return emit_DP4(gen, inst);
   case TGSI_OPCODE_RCP:
      return emit_RCP(gen, inst);
   case TGSI_OPCODE_RSQ:
      return emit_RSQ(gen, inst);
   case TGSI_OPCODE_ABS:
      return emit_ABS(gen, inst);
   case TGSI_OPCODE_SGT:
      return emit_SGT(gen, inst);
   case TGSI_OPCODE_SLT:
      return emit_SLT(gen, inst);
   case TGSI_OPCODE_SGE:
      return emit_SGE(gen, inst);
   case TGSI_OPCODE_SLE:
      return emit_SLE(gen, inst);
   case TGSI_OPCODE_SEQ:
      return emit_SEQ(gen, inst);
   case TGSI_OPCODE_SNE:
      return emit_SNE(gen, inst);
   case TGSI_OPCODE_CMP:
      return emit_CMP(gen, inst);
   case TGSI_OPCODE_MAX:
      return emit_MAX(gen, inst);
   case TGSI_OPCODE_MIN:
      return emit_MIN(gen, inst);
   case TGSI_OPCODE_FLR:
      return emit_FLR(gen, inst);
   case TGSI_OPCODE_FRC:
      return emit_FRC(gen, inst);
   case TGSI_OPCODE_END:
      return emit_END(gen);

   case TGSI_OPCODE_IF:
      return emit_IF(gen, inst);
   case TGSI_OPCODE_ELSE:
      return emit_ELSE(gen, inst);
   case TGSI_OPCODE_ENDIF:
      return emit_ENDIF(gen, inst);

   case TGSI_OPCODE_DDX:
      return emit_DDX_DDY(gen, inst, true);
   case TGSI_OPCODE_DDY:
      return emit_DDX_DDY(gen, inst, false);

   /* XXX lots more cases to do... */

   default:
      fprintf(stderr, "Cell: unimplemented TGSI instruction %d!\n",
              inst->Instruction.Opcode);
      return false;
   }

   return true;
}



/**
 * Emit code for a TGSI immediate value (vector of four floats).
 * This involves register allocation and initialization.
 * XXX the initialization should be done by a "prepare" stage, not
 * per quad execution!
 */
static boolean
emit_immediate(struct codegen *gen, const struct tgsi_full_immediate *immed)
{
   int ch;

   assert(gen->num_imm < MAX_TEMPS);

   spe_comment(gen->f, -4, "IMMEDIATE:");

   for (ch = 0; ch < 4; ch++) {
      float val = immed->u.ImmediateFloat32[ch].Float;
      int reg = spe_allocate_available_register(gen->f);

      if (reg < 0)
         return false;

      /* update immediate map */
      gen->imm_regs[gen->num_imm][ch] = reg;

      /* emit initializer instruction */
      spe_load_float(gen->f, reg, val);
   }

   gen->num_imm++;

   return true;
}



/**
 * Emit "code" for a TGSI declaration.
 * We only care about TGSI TEMPORARY register declarations at this time.
 * For each TGSI TEMPORARY we allocate four SPE registers.
 */
static boolean
emit_declaration(struct cell_context *cell,
                 struct codegen *gen, const struct tgsi_full_declaration *decl)
{
   int i, ch;

   switch (decl->Declaration.File) {
   case TGSI_FILE_TEMPORARY:
      if (cell->debug_flags & CELL_DEBUG_ASM) {
         printf("Declare temp reg %d .. %d\n",
                decl->DeclarationRange.First,
                decl->DeclarationRange.Last);
      }

      for (i = decl->DeclarationRange.First;
           i <= decl->DeclarationRange.Last;
           i++) {
         assert(i < MAX_TEMPS);
         for (ch = 0; ch < 4; ch++) {
            gen->temp_regs[i][ch] = spe_allocate_available_register(gen->f);
            if (gen->temp_regs[i][ch] < 0)
               return false; /* out of regs */
         }

         /* XXX if we run out of SPE registers, we need to spill
          * to SPU memory.  someday...
          */

         if (cell->debug_flags & CELL_DEBUG_ASM) {
            printf("  SPE regs: %d %d %d %d\n",
                   gen->temp_regs[i][0],
                   gen->temp_regs[i][1],
                   gen->temp_regs[i][2],
                   gen->temp_regs[i][3]);
         }
      }
      break;
   default:
      ; /* ignore */
   }

   return true;
}


/**
 * Translate TGSI shader code to SPE instructions.  This is done when
 * the state tracker gives us a new shader (via pipe->create_fs_state()).
 *
 * \param cell    the rendering context (in)
 * \param tokens  the TGSI shader (in)
 * \param f       the generated function (out)
 */
boolean
cell_gen_fragment_program(struct cell_context *cell,
                          const struct tgsi_token *tokens,
                          struct spe_function *f)
{
   struct tgsi_parse_context parse;
   struct codegen gen;

   memset(&gen, 0, sizeof(gen));
   gen.f = f;

   /* For SPE function calls: reg $3 = first param, $4 = second param, etc. */
   gen.inputs_reg = 3;     /* pointer to inputs array */
   gen.outputs_reg = 4;    /* pointer to outputs array */
   gen.constants_reg = 5;  /* pointer to constants array */

   spe_init_func(f, SPU_MAX_FRAGMENT_PROGRAM_INSTS * SPE_INST_SIZE);
   spe_allocate_register(f, gen.inputs_reg);
   spe_allocate_register(f, gen.outputs_reg);
   spe_allocate_register(f, gen.constants_reg);

   if (cell->debug_flags & CELL_DEBUG_ASM) {
      spe_print_code(f, true);
      spe_indent(f, 8);
      printf("Begin %s\n", __FUNCTION__);
      tgsi_dump(tokens, 0);
   }

   tgsi_parse_init(&parse, tokens);

   while (!tgsi_parse_end_of_tokens(&parse) && !gen.error) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         if (!emit_immediate(&gen,  &parse.FullToken.FullImmediate))
            gen.error = true;
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         if (!emit_declaration(cell, &gen, &parse.FullToken.FullDeclaration))
            gen.error = true;
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (!emit_instruction(&gen, &parse.FullToken.FullInstruction))
            gen.error = true;
         break;

      default:
         assert(0);
      }
   }


   if (gen.error) {
      /* terminate the SPE code */
      return emit_END(&gen);
   }

   if (cell->debug_flags & CELL_DEBUG_ASM) {
      printf("cell_gen_fragment_program nr instructions: %d\n", f->num_inst);
      printf("End %s\n", __FUNCTION__);
   }

   tgsi_parse_free( &parse );

   return !gen.error;
}
