/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc.  All rights reserved.
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

#include <math.h>
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
   struct cell_context *cell;
   int inputs_reg;      /**< 1st function parameter */
   int outputs_reg;     /**< 2nd function parameter */
   int constants_reg;   /**< 3rd function parameter */
   int temp_regs[MAX_TEMPS][4]; /**< maps TGSI temps to SPE registers */
   int imm_regs[MAX_IMMED][4];  /**< maps TGSI immediates to SPE registers */

   int num_imm;  /**< number of immediates */

   int one_reg;         /**< register containing {1.0, 1.0, 1.0, 1.0} */

   int addr_reg;        /**< address register, integer values */

   /** Per-instruction temps / intermediate temps */
   int num_itemps;
   int itemps[12];

   /** Current IF/ELSE/ENDIF nesting level */
   int if_nesting;
   /** Current BGNLOOP/ENDLOOP nesting level */
   int loop_nesting;
   /** Location of start of current loop */
   int loop_start;

   /** Index of if/conditional mask register */
   int cond_mask_reg;
   /** Index of loop mask register */
   int loop_mask_reg;

   /** Index of master execution mask register */
   int exec_mask_reg;

   /** KIL mask: indicates which fragments have been killed */
   int kill_mask_reg;

   int frame_size;  /**< Stack frame size, in words */

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
      spe_comment(gen->f, -4, "init constant reg = 1.0:");

      /* one = {1.0, 1.0, 1.0, 1.0} */
      spe_load_float(gen->f, gen->one_reg, 1.0f);

      spe_indent(gen->f, -4);
   }

   return gen->one_reg;
}


/**
 * Return index of the address register.
 * Used for indirect register loads/stores.
 */
static int
get_address_reg(struct codegen *gen)
{
   if (gen->addr_reg <= 0) {
      gen->addr_reg = spe_allocate_available_register(gen->f);

      spe_indent(gen->f, 4);
      spe_comment(gen->f, -4, "init address reg = 0:");

      /* init addr = {0, 0, 0, 0} */
      spe_zero(gen->f, gen->addr_reg);

      spe_indent(gen->f, -4);
   }

   return gen->addr_reg;
}


/**
 * Return index of the master execution mask.
 * The register is allocated an initialized upon the first call.
 *
 * The master execution mask controls which pixels in a quad are
 * modified, according to surrounding conditionals, loops, etc.
 */
static int
get_exec_mask_reg(struct codegen *gen)
{
   if (gen->exec_mask_reg <= 0) {
      gen->exec_mask_reg = spe_allocate_available_register(gen->f);

      /* XXX this may not be needed */
      spe_comment(gen->f, 0*-4, "initialize master execution mask = ~0");
      spe_load_int(gen->f, gen->exec_mask_reg, ~0);
   }

   return gen->exec_mask_reg;
}


/** Return index of the conditional (if/else) execution mask register */
static int
get_cond_mask_reg(struct codegen *gen)
{
   if (gen->cond_mask_reg <= 0) {
      gen->cond_mask_reg = spe_allocate_available_register(gen->f);
   }

   return gen->cond_mask_reg;
}


/** Return index of the loop execution mask register */
static int
get_loop_mask_reg(struct codegen *gen)
{
   if (gen->loop_mask_reg <= 0) {
      gen->loop_mask_reg = spe_allocate_available_register(gen->f);
   }

   return gen->loop_mask_reg;
}



static boolean
is_register_src(struct codegen *gen, int channel,
                const struct tgsi_full_src_register *src)
{
   int swizzle = tgsi_util_get_full_src_register_swizzle(src, channel);
   int sign_op = tgsi_util_get_full_src_register_sign_mode(src, channel);

   if (swizzle > TGSI_SWIZZLE_W || sign_op != TGSI_UTIL_SIGN_KEEP) {
      return FALSE;
   }
   if (src->Register.File == TGSI_FILE_TEMPORARY ||
       src->Register.File == TGSI_FILE_IMMEDIATE) {
      return TRUE;
   }
   return FALSE;
}

  
static boolean
is_memory_dst(struct codegen *gen, int channel,
              const struct tgsi_full_dst_register *dst)
{
   if (dst->Register.File == TGSI_FILE_OUTPUT) {
      return TRUE;
   }
   else {
      return FALSE;
   }
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
   int swizzle = tgsi_util_get_full_src_register_swizzle(src, channel);
   boolean reg_is_itemp = FALSE;
   uint sign_op;

   assert(swizzle >= TGSI_SWIZZLE_X);
   assert(swizzle <= TGSI_SWIZZLE_W);

   {
      int index = src->Register.Index;

      assert(swizzle < 4);

      if (src->Register.Indirect) {
         /* XXX unfinished */
      }

      switch (src->Register.File) {
      case TGSI_FILE_TEMPORARY:
         reg = gen->temp_regs[index][swizzle];
         break;
      case TGSI_FILE_INPUT:
         {
            /* offset is measured in quadwords, not bytes */
            int offset = index * 4 + swizzle;
            reg = get_itemp(gen);
            reg_is_itemp = TRUE;
            /* Load:  reg = memory[(machine_reg) + offset] */
            spe_lqd(gen->f, reg, gen->inputs_reg, offset * 16);
         }
         break;
      case TGSI_FILE_IMMEDIATE:
         reg = gen->imm_regs[index][swizzle];
         break;
      case TGSI_FILE_CONSTANT:
         {
            /* offset is measured in quadwords, not bytes */
            int offset = index * 4 + swizzle;
            reg = get_itemp(gen);
            reg_is_itemp = TRUE;
            /* Load:  reg = memory[(machine_reg) + offset] */
            spe_lqd(gen->f, reg, gen->constants_reg, offset * 16);
         }
         break;
      default:
         assert(0);
      }
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
      spe_load_uint(gen->f, bit31mask_reg, (1 << 31));

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

   switch (dest->Register.File) {
   case TGSI_FILE_TEMPORARY:
      if (gen->if_nesting > 0 || gen->loop_nesting > 0)
         reg = get_itemp(gen);
      else
         reg = gen->temp_regs[dest->Register.Index][channel];
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
   /*
    * XXX need to implement dst reg clamping/saturation
    */
#if 0
   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      break;
   default:
      assert( 0 );
   }
#endif

   switch (dest->Register.File) {
   case TGSI_FILE_TEMPORARY:
      if (gen->if_nesting > 0 || gen->loop_nesting > 0) {
         int d_reg = gen->temp_regs[dest->Register.Index][channel];
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
         int offset = dest->Register.Index * 4 + channel;
         if (gen->if_nesting > 0 || gen->loop_nesting > 0) {
            int exec_reg = get_exec_mask_reg(gen);
            int curval_reg = get_itemp(gen);
            /* First read the current value from memory:
             * Load:  curval = memory[(machine_reg) + offset]
             */
            spe_lqd(gen->f, curval_reg, gen->outputs_reg, offset * 16);
            /* Mix curval with newvalue according to exec mask:
             * d[i] = mask_reg[i] ? value_reg : d_reg
             */
            spe_selb(gen->f, curval_reg, curval_reg, value_reg, exec_reg);
            /* Store: memory[(machine_reg) + offset] = curval */
            spe_stqd(gen->f, curval_reg, gen->outputs_reg, offset * 16);
         }
         else {
            /* Store: memory[(machine_reg) + offset] = reg */
            spe_stqd(gen->f, value_reg, gen->outputs_reg, offset * 16);
         }
      }
      break;
   default:
      assert(0);
   }
}



static void
emit_prologue(struct codegen *gen)
{
   gen->frame_size = 1024; /* XXX temporary, should be dynamic */

   spe_comment(gen->f, 0, "Function prologue:");

   /* save $lr on stack     # stqd $lr,16($sp) */
   spe_stqd(gen->f, SPE_REG_RA, SPE_REG_SP, 16);

   if (gen->frame_size >= 512) {
      /* offset is too large for ai instruction */
      int offset_reg = spe_allocate_available_register(gen->f);
      int sp_reg = spe_allocate_available_register(gen->f);
      /* offset = -framesize */
      spe_load_int(gen->f, offset_reg, -gen->frame_size);
      /* sp = $sp */
      spe_move(gen->f, sp_reg, SPE_REG_SP);
      /* $sp = $sp + offset_reg */
      spe_a(gen->f, SPE_REG_SP, SPE_REG_SP, offset_reg);
      /* save $sp in stack frame */
      spe_stqd(gen->f, sp_reg, SPE_REG_SP, 0);
      /* clean up */
      spe_release_register(gen->f, offset_reg);
      spe_release_register(gen->f, sp_reg);
   }
   else {
      /* save stack pointer    # stqd $sp,-frameSize($sp) */
      spe_stqd(gen->f, SPE_REG_SP, SPE_REG_SP, -gen->frame_size);

      /* adjust stack pointer  # ai $sp,$sp,-frameSize */
      spe_ai(gen->f, SPE_REG_SP, SPE_REG_SP, -gen->frame_size);
   }
}


static void
emit_epilogue(struct codegen *gen)
{
   const int return_reg = 3;

   spe_comment(gen->f, 0, "Function epilogue:");

   spe_comment(gen->f, 0, "return the killed mask");
   if (gen->kill_mask_reg > 0) {
      /* shader called KIL, return the "alive" mask */
      spe_move(gen->f, return_reg, gen->kill_mask_reg);
   }
   else {
      /* return {0,0,0,0} */
      spe_load_uint(gen->f, return_reg, 0);
   }

   spe_comment(gen->f, 0, "restore stack and return");
   if (gen->frame_size >= 512) {
      /* offset is too large for ai instruction */
      int offset_reg = spe_allocate_available_register(gen->f);
      /* offset = framesize */
      spe_load_int(gen->f, offset_reg, gen->frame_size);
      /* $sp = $sp + offset */
      spe_a(gen->f, SPE_REG_SP, SPE_REG_SP, offset_reg);
      /* clean up */
      spe_release_register(gen->f, offset_reg);
   }
   else {
      /* restore stack pointer    # ai $sp,$sp,frameSize */
      spe_ai(gen->f, SPE_REG_SP, SPE_REG_SP, gen->frame_size);
   }

   /* restore $lr              # lqd $lr,16($sp) */
   spe_lqd(gen->f, SPE_REG_RA, SPE_REG_SP, 16);

   /* return from function call */
   spe_bi(gen->f, SPE_REG_RA, 0, 0);
}


#define FOR_EACH_ENABLED_CHANNEL(inst, ch) \
   for (ch = 0; ch < 4; ch++) \
      if (inst->Dst[0].Register.WriteMask & (1 << ch))


static boolean
emit_ARL(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch = 0, src_reg, addr_reg;

   src_reg = get_src_reg(gen, ch, &inst->Src[0]);
   addr_reg = get_address_reg(gen);

   /* convert float to int */
   spe_cflts(gen->f, addr_reg, src_reg, 0);

   free_itemps(gen);

   return TRUE;
}


static boolean
emit_MOV(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, src_reg[4], dst_reg[4];

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      src_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      dst_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      if (is_register_src(gen, ch, &inst->Src[0]) &&
          is_memory_dst(gen, ch, &inst->Dst[0])) {
         /* special-case: register to memory store */
         store_dest_reg(gen, src_reg[ch], ch, &inst->Dst[0]);
      }
      else {
         spe_move(gen->f, dst_reg[ch], src_reg[ch]);
         store_dest_reg(gen, dst_reg[ch], ch, &inst->Dst[0]);
      }
   }

   free_itemps(gen);

   return TRUE;
}

/**
 * Emit binary operation
 */
static boolean
emit_binop(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], s2_reg[4], d_reg[4];

   /* Loop over Red/Green/Blue/Alpha channels, fetch src operands */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      s2_reg[ch] = get_src_reg(gen, ch, &inst->Src[1]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
   }

   /* Loop over Red/Green/Blue/Alpha channels, do the op, store results */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      /* Emit actual SPE instruction: d = s1 + s2 */
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_ADD:
         spe_fa(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         break;
      case TGSI_OPCODE_SUB:
         spe_fs(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         break;
      case TGSI_OPCODE_MUL:
         spe_fm(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         break;
      default:
         ;
      }
   }

   /* Store the result (a no-op for TGSI_FILE_TEMPORARY dests) */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   /* Free any intermediate temps we allocated */
   free_itemps(gen);

   return TRUE;
}


/**
 * Emit multiply add.  See emit_ADD for comments.
 */
static boolean
emit_MAD(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], s2_reg[4], s3_reg[4], d_reg[4];

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      s2_reg[ch] = get_src_reg(gen, ch, &inst->Src[1]);
      s3_reg[ch] = get_src_reg(gen, ch, &inst->Src[2]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fma(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch], s3_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }
   free_itemps(gen);
   return TRUE;
}


/**
 * Emit linear interpolate.  See emit_ADD for comments.
 */
static boolean
emit_LRP(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], s2_reg[4], s3_reg[4], d_reg[4], tmp_reg[4];

   /* setup/get src/dst/temp regs */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      s2_reg[ch] = get_src_reg(gen, ch, &inst->Src[1]);
      s3_reg[ch] = get_src_reg(gen, ch, &inst->Src[2]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
      tmp_reg[ch] = get_itemp(gen);
   }

   /* d = s3 + s1(s2 - s3) */
   /* do all subtracts, then all fma, then all stores to better pipeline */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fs(gen->f, tmp_reg[ch], s2_reg[ch], s3_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fma(gen->f, d_reg[ch], tmp_reg[ch], s1_reg[ch], s3_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }
   free_itemps(gen);
   return TRUE;
}



/**
 * Emit reciprocal or recip sqrt.
 */
static boolean
emit_RCP_RSQ(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], d_reg[4], tmp_reg[4];

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
      tmp_reg[ch] = get_itemp(gen);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      if (inst->Instruction.Opcode == TGSI_OPCODE_RCP) {
         /* tmp = 1/s1 */
         spe_frest(gen->f, tmp_reg[ch], s1_reg[ch]);
      }
      else {
         /* tmp = 1/sqrt(s1) */
         spe_frsqest(gen->f, tmp_reg[ch], s1_reg[ch]);
      }
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      /* d = float_interp(s1, tmp) */
      spe_fi(gen->f, d_reg[ch], s1_reg[ch], tmp_reg[ch]);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}


/**
 * Emit absolute value.  See emit_ADD for comments.
 */
static boolean
emit_ABS(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], d_reg[4];
   const int bit31mask_reg = get_itemp(gen);

   /* mask with bit 31 set, the rest cleared */  
   spe_load_uint(gen->f, bit31mask_reg, (1 << 31));

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
   }

   /* d = sign bit cleared in s1 */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_andc(gen->f, d_reg[ch], s1_reg[ch], bit31mask_reg);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}

/**
 * Emit 3 component dot product.  See emit_ADD for comments.
 */
static boolean
emit_DP3(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   int s1x_reg, s1y_reg, s1z_reg;
   int s2x_reg, s2y_reg, s2z_reg;
   int t0_reg = get_itemp(gen), t1_reg = get_itemp(gen);

   s1x_reg = get_src_reg(gen, CHAN_X, &inst->Src[0]);
   s2x_reg = get_src_reg(gen, CHAN_X, &inst->Src[1]);
   s1y_reg = get_src_reg(gen, CHAN_Y, &inst->Src[0]);
   s2y_reg = get_src_reg(gen, CHAN_Y, &inst->Src[1]);
   s1z_reg = get_src_reg(gen, CHAN_Z, &inst->Src[0]);
   s2z_reg = get_src_reg(gen, CHAN_Z, &inst->Src[1]);

   /* t0 = x0 * x1 */
   spe_fm(gen->f, t0_reg, s1x_reg, s2x_reg);

   /* t1 = y0 * y1 */
   spe_fm(gen->f, t1_reg, s1y_reg, s2y_reg);

   /* t0 = z0 * z1 + t0 */
   spe_fma(gen->f, t0_reg, s1z_reg, s2z_reg, t0_reg);

   /* t0 = t0 + t1 */
   spe_fa(gen->f, t0_reg, t0_reg, t1_reg);

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      int d_reg = get_dst_reg(gen, ch, &inst->Dst[0]);
      spe_move(gen->f, d_reg, t0_reg);
      store_dest_reg(gen, d_reg, ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}

/**
 * Emit 4 component dot product.  See emit_ADD for comments.
 */
static boolean
emit_DP4(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   int s0x_reg, s0y_reg, s0z_reg, s0w_reg;
   int s1x_reg, s1y_reg, s1z_reg, s1w_reg;
   int t0_reg = get_itemp(gen), t1_reg = get_itemp(gen);

   s0x_reg = get_src_reg(gen, CHAN_X, &inst->Src[0]);
   s1x_reg = get_src_reg(gen, CHAN_X, &inst->Src[1]);
   s0y_reg = get_src_reg(gen, CHAN_Y, &inst->Src[0]);
   s1y_reg = get_src_reg(gen, CHAN_Y, &inst->Src[1]);
   s0z_reg = get_src_reg(gen, CHAN_Z, &inst->Src[0]);
   s1z_reg = get_src_reg(gen, CHAN_Z, &inst->Src[1]);
   s0w_reg = get_src_reg(gen, CHAN_W, &inst->Src[0]);
   s1w_reg = get_src_reg(gen, CHAN_W, &inst->Src[1]);

   /* t0 = x0 * x1 */
   spe_fm(gen->f, t0_reg, s0x_reg, s1x_reg);

   /* t1 = y0 * y1 */
   spe_fm(gen->f, t1_reg, s0y_reg, s1y_reg);

   /* t0 = z0 * z1 + t0 */
   spe_fma(gen->f, t0_reg, s0z_reg, s1z_reg, t0_reg);

   /* t1 = w0 * w1 + t1 */
   spe_fma(gen->f, t1_reg, s0w_reg, s1w_reg, t1_reg);

   /* t0 = t0 + t1 */
   spe_fa(gen->f, t0_reg, t0_reg, t1_reg);

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      int d_reg = get_dst_reg(gen, ch, &inst->Dst[0]);
      spe_move(gen->f, d_reg, t0_reg);
      store_dest_reg(gen, d_reg, ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}

/**
 * Emit homogeneous dot product.  See emit_ADD for comments.
 */
static boolean
emit_DPH(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   /* XXX rewrite this function to look more like DP3/DP4 */
   int ch;
   int s1_reg = get_src_reg(gen, CHAN_X, &inst->Src[0]);
   int s2_reg = get_src_reg(gen, CHAN_X, &inst->Src[1]);
   int tmp_reg = get_itemp(gen);

   /* t = x0 * x1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->Src[0]);
   s2_reg = get_src_reg(gen, CHAN_Y, &inst->Src[1]);
   /* t = y0 * y1 + t */
   spe_fma(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   s1_reg = get_src_reg(gen, CHAN_Z, &inst->Src[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->Src[1]);
   /* t = z0 * z1 + t */
   spe_fma(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   s2_reg = get_src_reg(gen, CHAN_W, &inst->Src[1]);
   /* t = w1 + t */
   spe_fa(gen->f, tmp_reg, s2_reg, tmp_reg);

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      int d_reg = get_dst_reg(gen, ch, &inst->Dst[0]);
      spe_move(gen->f, d_reg, tmp_reg);
      store_dest_reg(gen, tmp_reg, ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}

/**
 * Emit 3-component vector normalize.
 */
static boolean
emit_NRM3(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   int src_reg[3];
   int t0_reg = get_itemp(gen), t1_reg = get_itemp(gen);

   src_reg[0] = get_src_reg(gen, CHAN_X, &inst->Src[0]);
   src_reg[1] = get_src_reg(gen, CHAN_Y, &inst->Src[0]);
   src_reg[2] = get_src_reg(gen, CHAN_Z, &inst->Src[0]);

   /* t0 = x * x */
   spe_fm(gen->f, t0_reg, src_reg[0], src_reg[0]);

   /* t1 = y * y */
   spe_fm(gen->f, t1_reg, src_reg[1], src_reg[1]);

   /* t0 = z * z + t0 */
   spe_fma(gen->f, t0_reg, src_reg[2], src_reg[2], t0_reg);

   /* t0 = t0 + t1 */
   spe_fa(gen->f, t0_reg, t0_reg, t1_reg);

   /* t1 = 1.0 / sqrt(t0) */
   spe_frsqest(gen->f, t1_reg, t0_reg);
   spe_fi(gen->f, t1_reg, t0_reg, t1_reg);

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      int d_reg = get_dst_reg(gen, ch, &inst->Dst[0]);
      /* dst = src[ch] * t1 */
      spe_fm(gen->f, d_reg, src_reg[ch], t1_reg);
      store_dest_reg(gen, d_reg, ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}


/**
 * Emit cross product.  See emit_ADD for comments.
 */
static boolean
emit_XPD(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int s1_reg = get_src_reg(gen, CHAN_Z, &inst->Src[0]);
   int s2_reg = get_src_reg(gen, CHAN_Y, &inst->Src[1]);
   int tmp_reg = get_itemp(gen);

   /* t = z0 * y1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->Src[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->Src[1]);
   /* t = y0 * z1 - t */
   spe_fms(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   if (inst->Dst[0].Register.WriteMask & (1 << CHAN_X)) {
      store_dest_reg(gen, tmp_reg, CHAN_X, &inst->Dst[0]);
   }

   s1_reg = get_src_reg(gen, CHAN_X, &inst->Src[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->Src[1]);
   /* t = x0 * z1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Z, &inst->Src[0]);
   s2_reg = get_src_reg(gen, CHAN_X, &inst->Src[1]);
   /* t = z0 * x1 - t */
   spe_fms(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   if (inst->Dst[0].Register.WriteMask & (1 << CHAN_Y)) {
      store_dest_reg(gen, tmp_reg, CHAN_Y, &inst->Dst[0]);
   }

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->Src[0]);
   s2_reg = get_src_reg(gen, CHAN_X, &inst->Src[1]);
   /* t = y0 * x1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_X, &inst->Src[0]);
   s2_reg = get_src_reg(gen, CHAN_Y, &inst->Src[1]);
   /* t = x0 * y1 - t */
   spe_fms(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   if (inst->Dst[0].Register.WriteMask & (1 << CHAN_Z)) {
      store_dest_reg(gen, tmp_reg, CHAN_Z, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}


/**
 * Emit inequality instruction.
 * Note that the SPE fcgt instruction produces 0x0 and 0xffffffff as
 * the result but OpenGL/TGSI needs 0.0 and 1.0 results.
 * We can easily convert 0x0/0xffffffff to 0.0/1.0 with a bitwise AND.
 */
static boolean
emit_inequality(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], s2_reg[4], d_reg[4], one_reg;
   boolean complement = FALSE;

   one_reg = get_const_one_reg(gen);

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      s2_reg[ch] = get_src_reg(gen, ch, &inst->Src[1]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_SGT:
         spe_fcgt(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         break;
      case TGSI_OPCODE_SLT:
         spe_fcgt(gen->f, d_reg[ch], s2_reg[ch], s1_reg[ch]);
         break;
      case TGSI_OPCODE_SGE:
         spe_fcgt(gen->f, d_reg[ch], s2_reg[ch], s1_reg[ch]);
         complement = TRUE;
         break;
      case TGSI_OPCODE_SLE:
         spe_fcgt(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         complement = TRUE;
         break;
      case TGSI_OPCODE_SEQ:
         spe_fceq(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         break;
      case TGSI_OPCODE_SNE:
         spe_fceq(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         complement = TRUE;
         break;
      default:
         assert(0);
      }
   }

   /* convert d from 0x0/0xffffffff to 0.0/1.0 */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      /* d = d & one_reg */
      if (complement)
         spe_andc(gen->f, d_reg[ch], one_reg, d_reg[ch]);
      else
         spe_and(gen->f, d_reg[ch], one_reg, d_reg[ch]);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}


/**
 * Emit compare.
 */
static boolean
emit_CMP(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      int s1_reg = get_src_reg(gen, ch, &inst->Src[0]);
      int s2_reg = get_src_reg(gen, ch, &inst->Src[1]);
      int s3_reg = get_src_reg(gen, ch, &inst->Src[2]);
      int d_reg = get_dst_reg(gen, ch, &inst->Dst[0]);
      int zero_reg = get_itemp(gen);
   
      spe_zero(gen->f, zero_reg);

      /* d = (s1 < 0) ? s2 : s3 */
      spe_fcgt(gen->f, d_reg, zero_reg, s1_reg);
      spe_selb(gen->f, d_reg, s3_reg, s2_reg, d_reg);

      store_dest_reg(gen, d_reg, ch, &inst->Dst[0]);
      free_itemps(gen);
   }

   return TRUE;
}

/**
 * Emit trunc.  
 * Convert float to signed int
 * Convert signed int to float
 */
static boolean
emit_TRUNC(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], d_reg[4];

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
   }

   /* Convert float to int */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_cflts(gen->f, d_reg[ch], s1_reg[ch], 0);
   }

   /* Convert int to float */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_csflt(gen->f, d_reg[ch], d_reg[ch], 0);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
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
   int ch, s1_reg[4], d_reg[4], tmp_reg[4], zero_reg, one_reg;

   zero_reg = get_itemp(gen);
   spe_zero(gen->f, zero_reg);
   one_reg = get_const_one_reg(gen);
   
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
      tmp_reg[ch] = get_itemp(gen);
   }

   /* If negative, subtract 1.0 */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fcgt(gen->f, tmp_reg[ch], zero_reg, s1_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_selb(gen->f, tmp_reg[ch], zero_reg, one_reg, tmp_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fs(gen->f, tmp_reg[ch], s1_reg[ch], tmp_reg[ch]);
   }

   /* Convert float to int */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_cflts(gen->f, tmp_reg[ch], tmp_reg[ch], 0);
   }

   /* Convert int to float */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_csflt(gen->f, d_reg[ch], tmp_reg[ch], 0);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}


/**
 * Compute frac = Input - FLR(Input)
 */
static boolean
emit_FRC(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], d_reg[4], tmp_reg[4], zero_reg, one_reg;

   zero_reg = get_itemp(gen);
   spe_zero(gen->f, zero_reg);
   one_reg = get_const_one_reg(gen);

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
      tmp_reg[ch] = get_itemp(gen);
   }

   /* If negative, subtract 1.0 */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fcgt(gen->f, tmp_reg[ch], zero_reg, s1_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_selb(gen->f, tmp_reg[ch], zero_reg, one_reg, tmp_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fs(gen->f, tmp_reg[ch], s1_reg[ch], tmp_reg[ch]);
   }

   /* Convert float to int */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_cflts(gen->f, tmp_reg[ch], tmp_reg[ch], 0);
   }

   /* Convert int to float */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_csflt(gen->f, tmp_reg[ch], tmp_reg[ch], 0);
   }

   /* d = s1 - FLR(s1) */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_fs(gen->f, d_reg[ch], s1_reg[ch], tmp_reg[ch]);
   }

   /* store result */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}


#if 0
static void
print_functions(struct cell_context *cell)
{
   struct cell_spu_function_info *funcs = &cell->spu_functions;
   uint i;
   for (i = 0; i < funcs->num; i++) {
      printf("SPU func %u: %s at %u\n",
             i, funcs->names[i], funcs->addrs[i]);
   }
}
#endif


static uint
lookup_function(struct cell_context *cell, const char *funcname)
{
   const struct cell_spu_function_info *funcs = &cell->spu_functions;
   uint i, addr = 0;
   for (i = 0; i < funcs->num; i++) {
      if (strcmp(funcs->names[i], funcname) == 0) {
         addr = funcs->addrs[i];
      }
   }
   assert(addr && "spu function not found");
   return addr / 4;  /* discard 2 least significant bits */
}


/**
 * Emit code to call a SPU function.
 * Used to implement instructions like SIN/COS/POW/TEX/etc.
 * If scalar, only the X components of the src regs are used, and the
 * result is replicated across the dest register's XYZW components.
 */
static boolean
emit_function_call(struct codegen *gen,
                   const struct tgsi_full_instruction *inst,
                   char *funcname, uint num_args, boolean scalar)
{
   const uint addr = lookup_function(gen->cell, funcname);
   char comment[100];
   int s_regs[3];
   int func_called = FALSE;
   uint a, ch;
   int retval_reg = -1;

   assert(num_args <= 3);

   snprintf(comment, sizeof(comment), "CALL %s:", funcname);
   spe_comment(gen->f, -4, comment);

   if (scalar) {
      for (a = 0; a < num_args; a++) {
         s_regs[a] = get_src_reg(gen, CHAN_X, &inst->Src[a]);
      }
      /* we'll call the function, put the return value in this register,
       * then replicate it across all write-enabled components in d_reg.
       */
      retval_reg = spe_allocate_available_register(gen->f);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      int d_reg;
      ubyte usedRegs[SPE_NUM_REGS];
      uint i, numUsed;

      if (!scalar) {
         for (a = 0; a < num_args; a++) {
            s_regs[a] = get_src_reg(gen, ch, &inst->Src[a]);
         }
      }

      d_reg = get_dst_reg(gen, ch, &inst->Dst[0]);

      if (!scalar || !func_called) {
         /* for a scalar function, we'll really only call the function once */

         numUsed = spe_get_registers_used(gen->f, usedRegs);
         assert(numUsed < gen->frame_size / 16 - 2);

         /* save registers to stack */
         for (i = 0; i < numUsed; i++) {
            uint reg = usedRegs[i];
            int offset = 2 + i;
            spe_stqd(gen->f, reg, SPE_REG_SP, 16 * offset);
         }

         /* setup function arguments */
         for (a = 0; a < num_args; a++) {
            spe_move(gen->f, 3 + a, s_regs[a]);
         }

         /* branch to function, save return addr */
         spe_brasl(gen->f, SPE_REG_RA, addr);

         /* save function's return value */
         if (scalar)
            spe_move(gen->f, retval_reg, 3);
         else
            spe_move(gen->f, d_reg, 3);

         /* restore registers from stack */
         for (i = 0; i < numUsed; i++) {
            uint reg = usedRegs[i];
            if (reg != d_reg && reg != retval_reg) {
               int offset = 2 + i;
               spe_lqd(gen->f, reg, SPE_REG_SP, 16 * offset);
            }
         }

         func_called = TRUE;
      }

      if (scalar) {
         spe_move(gen->f, d_reg, retval_reg);
      }

      store_dest_reg(gen, d_reg, ch, &inst->Dst[0]);
      free_itemps(gen);
   }

   if (scalar) {
      spe_release_register(gen->f, retval_reg);
   }

   return TRUE;
}


static boolean
emit_TEX(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const uint target = inst->Texture.Texture;
   const uint unit = inst->Src[1].Register.Index;
   uint addr;
   int ch;
   int coord_regs[4], d_regs[4];

   switch (target) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_2D:
      addr = lookup_function(gen->cell, "spu_tex_2d");
      break;
   case TGSI_TEXTURE_3D:
      addr = lookup_function(gen->cell, "spu_tex_3d");
      break;
   case TGSI_TEXTURE_CUBE:
      addr = lookup_function(gen->cell, "spu_tex_cube");
      break;
   default:
      ASSERT(0 && "unsupported texture target");
      return FALSE;
   }

   assert(inst->Src[1].Register.File == TGSI_FILE_SAMPLER);

   spe_comment(gen->f, -4, "CALL tex:");

   /* get src/dst reg info */
   for (ch = 0; ch < 4; ch++) {
      coord_regs[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      d_regs[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
   }

   {
      ubyte usedRegs[SPE_NUM_REGS];
      uint i, numUsed;

      numUsed = spe_get_registers_used(gen->f, usedRegs);
      assert(numUsed < gen->frame_size / 16 - 2);

      /* save registers to stack */
      for (i = 0; i < numUsed; i++) {
         uint reg = usedRegs[i];
         int offset = 2 + i;
         spe_stqd(gen->f, reg, SPE_REG_SP, 16 * offset);
      }

      /* setup function arguments (XXX depends on target) */
      for (i = 0; i < 4; i++) {
         spe_move(gen->f, 3 + i, coord_regs[i]);
      }
      spe_load_uint(gen->f, 7, unit); /* sampler unit */

      /* branch to function, save return addr */
      spe_brasl(gen->f, SPE_REG_RA, addr);

      /* save function's return values (four pixel's colors) */
      for (i = 0; i < 4; i++) {
         spe_move(gen->f, d_regs[i], 3 + i);
      }

      /* restore registers from stack */
      for (i = 0; i < numUsed; i++) {
         uint reg = usedRegs[i];
         if (reg != d_regs[0] &&
             reg != d_regs[1] &&
             reg != d_regs[2] &&
             reg != d_regs[3]) {
            int offset = 2 + i;
            spe_lqd(gen->f, reg, SPE_REG_SP, 16 * offset);
         }
      }
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_regs[ch], ch, &inst->Dst[0]);
      free_itemps(gen);
   }

   return TRUE;
}


/**
 * KILL if any of src reg values are less than zero.
 */
static boolean
emit_KIL(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;
   int s_regs[4], kil_reg = -1, cmp_reg, zero_reg;

   spe_comment(gen->f, -4, "CALL kil:");

   /* zero = {0,0,0,0} */
   zero_reg = get_itemp(gen);
   spe_zero(gen->f, zero_reg);

   cmp_reg = get_itemp(gen);

   /* get src regs */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s_regs[ch] = get_src_reg(gen, ch, &inst->Src[0]);
   }

   /* test if any src regs are < 0 */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      if (kil_reg >= 0) {
         /* cmp = 0 > src ? : ~0 : 0 */
         spe_fcgt(gen->f, cmp_reg, zero_reg, s_regs[ch]);
         /* kil = kil | cmp */
         spe_or(gen->f, kil_reg, kil_reg, cmp_reg);
      }
      else {
         kil_reg = get_itemp(gen);
         /* kil = 0 > src ? : ~0 : 0 */
         spe_fcgt(gen->f, kil_reg, zero_reg, s_regs[ch]);
      }
   }

   if (gen->if_nesting || gen->loop_nesting) {
      /* may have been a conditional kil */
      spe_and(gen->f, kil_reg, kil_reg, gen->exec_mask_reg);
   }

   /* allocate the kill mask reg if needed */
   if (gen->kill_mask_reg <= 0) {
      gen->kill_mask_reg = spe_allocate_available_register(gen->f);
      spe_move(gen->f, gen->kill_mask_reg, kil_reg);
   }
   else {
      spe_or(gen->f, gen->kill_mask_reg, gen->kill_mask_reg, kil_reg);
   }

   free_itemps(gen);

   return TRUE;
}



/**
 * Emit min or max.
 */
static boolean
emit_MIN_MAX(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s0_reg[4], s1_reg[4], d_reg[4], tmp_reg[4];

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      s0_reg[ch] = get_src_reg(gen, ch, &inst->Src[0]);
      s1_reg[ch] = get_src_reg(gen, ch, &inst->Src[1]);
      d_reg[ch] = get_dst_reg(gen, ch, &inst->Dst[0]);
      tmp_reg[ch] = get_itemp(gen);         
   }

   /* d = (s0 > s1) ? s0 : s1 */
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      if (inst->Instruction.Opcode == TGSI_OPCODE_MAX)
         spe_fcgt(gen->f, tmp_reg[ch], s0_reg[ch], s1_reg[ch]);
      else
         spe_fcgt(gen->f, tmp_reg[ch], s1_reg[ch], s0_reg[ch]);
   }
   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      spe_selb(gen->f, d_reg[ch], s1_reg[ch], s0_reg[ch], tmp_reg[ch]);
   }

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      store_dest_reg(gen, d_reg[ch], ch, &inst->Dst[0]);
   }

   free_itemps(gen);
   return TRUE;
}


/**
 * Emit code to update the execution mask.
 * This needs to be done whenever the execution status of a conditional
 * or loop is changed.
 */
static void
emit_update_exec_mask(struct codegen *gen)
{
   const int exec_reg = get_exec_mask_reg(gen);
   const int cond_reg = gen->cond_mask_reg;
   const int loop_reg = gen->loop_mask_reg;

   spe_comment(gen->f, 0, "Update master execution mask");

   if (gen->if_nesting > 0 && gen->loop_nesting > 0) {
      /* exec_mask = cond_mask & loop_mask */
      assert(cond_reg > 0);
      assert(loop_reg > 0);
      spe_and(gen->f, exec_reg, cond_reg, loop_reg);
   }
   else if (gen->if_nesting > 0) {
      assert(cond_reg > 0);
      spe_move(gen->f, exec_reg, cond_reg);
   }
   else if (gen->loop_nesting > 0) {
      assert(loop_reg > 0);
      spe_move(gen->f, exec_reg, loop_reg);
   }
   else {
      spe_load_int(gen->f, exec_reg, ~0x0);
   }
}


static boolean
emit_IF(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const int channel = 0;
   int cond_reg;

   cond_reg = get_cond_mask_reg(gen);

   /* XXX push cond exec mask */

   spe_comment(gen->f,  0, "init conditional exec mask = ~0:");
   spe_load_int(gen->f, cond_reg, ~0);

   /* update conditional execution mask with the predicate register */
   int tmp_reg = get_itemp(gen);
   int s1_reg = get_src_reg(gen, channel, &inst->Src[0]);

   /* tmp = (s1_reg == 0) */
   spe_ceqi(gen->f, tmp_reg, s1_reg, 0);
   /* tmp = !tmp */
   spe_complement(gen->f, tmp_reg, tmp_reg);
   /* cond_mask = cond_mask & tmp */
   spe_and(gen->f, cond_reg, cond_reg, tmp_reg);

   gen->if_nesting++;

   /* update the master execution mask */
   emit_update_exec_mask(gen);

   free_itemps(gen);

   return TRUE;
}


static boolean
emit_ELSE(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const int cond_reg = get_cond_mask_reg(gen);

   spe_comment(gen->f, 0, "cond exec mask = !cond exec mask");
   spe_complement(gen->f, cond_reg, cond_reg);
   emit_update_exec_mask(gen);

   return TRUE;
}


static boolean
emit_ENDIF(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   /* XXX todo: pop cond exec mask */

   gen->if_nesting--;

   emit_update_exec_mask(gen);

   return TRUE;
}


static boolean
emit_BGNLOOP(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int exec_reg, loop_reg;

   exec_reg = get_exec_mask_reg(gen);
   loop_reg = get_loop_mask_reg(gen);

   /* XXX push loop_exec mask */

   spe_comment(gen->f,  0*-4, "initialize loop exec mask = ~0");
   spe_load_int(gen->f, loop_reg, ~0x0);

   gen->loop_nesting++;
   gen->loop_start = spe_code_size(gen->f);  /* in bytes */

   return TRUE;
}


static boolean
emit_ENDLOOP(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const int loop_reg = get_loop_mask_reg(gen);
   const int tmp_reg = get_itemp(gen);
   int offset;

   /* tmp_reg = exec[0] | exec[1] | exec[2] | exec[3] */
   spe_orx(gen->f, tmp_reg, loop_reg);

   offset = gen->loop_start - spe_code_size(gen->f); /* in bytes */

   /* branch back to top of loop if tmp_reg != 0 */
   spe_brnz(gen->f, tmp_reg, offset / 4);

   /* XXX pop loop_exec mask */

   gen->loop_nesting--;

   emit_update_exec_mask(gen);

   return TRUE;
}


static boolean
emit_BRK(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const int exec_reg = get_exec_mask_reg(gen);
   const int loop_reg = get_loop_mask_reg(gen);

   assert(gen->loop_nesting > 0);

   spe_comment(gen->f, 0, "loop exec mask &= ~master exec mask");
   spe_andc(gen->f, loop_reg, loop_reg, exec_reg);

   emit_update_exec_mask(gen);

   return TRUE;
}


static boolean
emit_CONT(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   assert(gen->loop_nesting > 0);

   return TRUE;
}


static boolean
emit_DDX_DDY(struct codegen *gen, const struct tgsi_full_instruction *inst,
             boolean ddx)
{
   int ch;

   FOR_EACH_ENABLED_CHANNEL(inst, ch) {
      int s_reg = get_src_reg(gen, ch, &inst->Src[0]);
      int d_reg = get_dst_reg(gen, ch, &inst->Dst[0]);

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

   return TRUE;
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
   emit_epilogue(gen);
   return TRUE;
}


/**
 * Emit code for the given instruction.  Just a big switch stmt.
 */
static boolean
emit_instruction(struct codegen *gen,
                 const struct tgsi_full_instruction *inst)
{
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      return emit_ARL(gen, inst);
   case TGSI_OPCODE_MOV:
      return emit_MOV(gen, inst);
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_MUL:
      return emit_binop(gen, inst);
   case TGSI_OPCODE_MAD:
      return emit_MAD(gen, inst);
   case TGSI_OPCODE_LRP:
      return emit_LRP(gen, inst);
   case TGSI_OPCODE_DP3:
      return emit_DP3(gen, inst);
   case TGSI_OPCODE_DP4:
      return emit_DP4(gen, inst);
   case TGSI_OPCODE_DPH:
      return emit_DPH(gen, inst);
   case TGSI_OPCODE_NRM:
      return emit_NRM3(gen, inst);
   case TGSI_OPCODE_XPD:
      return emit_XPD(gen, inst);
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_RSQ:
      return emit_RCP_RSQ(gen, inst);
   case TGSI_OPCODE_ABS:
      return emit_ABS(gen, inst);
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGE:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SNE:
      return emit_inequality(gen, inst);
   case TGSI_OPCODE_CMP:
      return emit_CMP(gen, inst);
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MAX:
      return emit_MIN_MAX(gen, inst);
   case TGSI_OPCODE_TRUNC:
      return emit_TRUNC(gen, inst);
   case TGSI_OPCODE_FLR:
      return emit_FLR(gen, inst);
   case TGSI_OPCODE_FRC:
      return emit_FRC(gen, inst);
   case TGSI_OPCODE_END:
      return emit_END(gen);

   case TGSI_OPCODE_COS:
      return emit_function_call(gen, inst, "spu_cos", 1, TRUE);
   case TGSI_OPCODE_SIN:
      return emit_function_call(gen, inst, "spu_sin", 1, TRUE);
   case TGSI_OPCODE_POW:
      return emit_function_call(gen, inst, "spu_pow", 2, TRUE);
   case TGSI_OPCODE_EX2:
      return emit_function_call(gen, inst, "spu_exp2", 1, TRUE);
   case TGSI_OPCODE_LG2:
      return emit_function_call(gen, inst, "spu_log2", 1, TRUE);
   case TGSI_OPCODE_TEX:
      /* fall-through for now */
   case TGSI_OPCODE_TXD:
      /* fall-through for now */
   case TGSI_OPCODE_TXB:
      /* fall-through for now */
   case TGSI_OPCODE_TXL:
      /* fall-through for now */
   case TGSI_OPCODE_TXP:
      return emit_TEX(gen, inst);
   case TGSI_OPCODE_KIL:
      return emit_KIL(gen, inst);

   case TGSI_OPCODE_IF:
      return emit_IF(gen, inst);
   case TGSI_OPCODE_ELSE:
      return emit_ELSE(gen, inst);
   case TGSI_OPCODE_ENDIF:
      return emit_ENDIF(gen, inst);

   case TGSI_OPCODE_BGNLOOP:
      return emit_BGNLOOP(gen, inst);
   case TGSI_OPCODE_ENDLOOP:
      return emit_ENDLOOP(gen, inst);
   case TGSI_OPCODE_BRK:
      return emit_BRK(gen, inst);
   case TGSI_OPCODE_CONT:
      return emit_CONT(gen, inst);

   case TGSI_OPCODE_DDX:
      return emit_DDX_DDY(gen, inst, TRUE);
   case TGSI_OPCODE_DDY:
      return emit_DDX_DDY(gen, inst, FALSE);

   /* XXX lots more cases to do... */

   default:
      fprintf(stderr, "Cell: unimplemented TGSI instruction %d!\n",
              inst->Instruction.Opcode);
      return FALSE;
   }

   return TRUE;
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

   for (ch = 0; ch < 4; ch++) {
      float val = immed->u[ch].Float;

      if (ch > 0 && val == immed->u[ch - 1].Float) {
         /* re-use previous register */
         gen->imm_regs[gen->num_imm][ch] = gen->imm_regs[gen->num_imm][ch - 1];
      }
      else {
         char str[100];
         int reg = spe_allocate_available_register(gen->f);

         if (reg < 0)
            return FALSE;

         sprintf(str, "init $%d = %f", reg, val);
         spe_comment(gen->f, 0, str);

         /* update immediate map */
         gen->imm_regs[gen->num_imm][ch] = reg;

         /* emit initializer instruction */
         spe_load_float(gen->f, reg, val);
      }
   }

   gen->num_imm++;

   return TRUE;
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
      for (i = decl->Range.First;
           i <= decl->Range.Last;
           i++) {
         assert(i < MAX_TEMPS);
         for (ch = 0; ch < 4; ch++) {
            gen->temp_regs[i][ch] = spe_allocate_available_register(gen->f);
            if (gen->temp_regs[i][ch] < 0)
               return FALSE; /* out of regs */
         }

         /* XXX if we run out of SPE registers, we need to spill
          * to SPU memory.  someday...
          */

         {
            char buf[100];
            sprintf(buf, "TGSI temp[%d] maps to SPU regs [$%d $%d $%d $%d]", i,
                    gen->temp_regs[i][0], gen->temp_regs[i][1],
                    gen->temp_regs[i][2], gen->temp_regs[i][3]);
            spe_comment(gen->f, 0, buf);
         }
      }
      break;
   default:
      ; /* ignore */
   }

   return TRUE;
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
   uint ic = 0;

   memset(&gen, 0, sizeof(gen));
   gen.cell = cell;
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
      spe_print_code(f, TRUE);
      spe_indent(f, 2*8);
      printf("Begin %s\n", __FUNCTION__);
      tgsi_dump(tokens, 0);
   }

   tgsi_parse_init(&parse, tokens);

   emit_prologue(&gen);

   while (!tgsi_parse_end_of_tokens(&parse) && !gen.error) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         if (f->print) {
            _debug_printf("    # ");
            tgsi_dump_immediate(&parse.FullToken.FullImmediate);
         }
         if (!emit_immediate(&gen, &parse.FullToken.FullImmediate))
            gen.error = TRUE;
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         if (f->print) {
            _debug_printf("    # ");
            tgsi_dump_declaration(&parse.FullToken.FullDeclaration);
         }
         if (!emit_declaration(cell, &gen, &parse.FullToken.FullDeclaration))
            gen.error = TRUE;
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (f->print) {
            _debug_printf("    # ");
            ic++;
            tgsi_dump_instruction(&parse.FullToken.FullInstruction, ic);
         }
         if (!emit_instruction(&gen, &parse.FullToken.FullInstruction))
            gen.error = TRUE;
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
