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

   /** Per-instruction temps / intermediate temps */
   int num_itemps;
   int itemps[12];

   /** Current IF/ELSE/ENDIF nesting level */
   int if_nesting;
   /** Index of execution mask register */
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


static boolean
is_register_src(struct codegen *gen, int channel,
                const struct tgsi_full_src_register *src)
{
   int swizzle = tgsi_util_get_full_src_register_extswizzle(src, channel);
   int sign_op = tgsi_util_get_full_src_register_sign_mode(src, channel);

   if (swizzle > TGSI_SWIZZLE_W || sign_op != TGSI_UTIL_SIGN_KEEP) {
      return FALSE;
   }
   if (src->SrcRegister.File == TGSI_FILE_TEMPORARY ||
       src->SrcRegister.File == TGSI_FILE_IMMEDIATE) {
      return TRUE;
   }
   return FALSE;
}

  
static boolean
is_memory_dst(struct codegen *gen, int channel,
              const struct tgsi_full_dst_register *dst)
{
   if (dst->DstRegister.File == TGSI_FILE_OUTPUT) {
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
   int swizzle = tgsi_util_get_full_src_register_extswizzle(src, channel);
   boolean reg_is_itemp = FALSE;
   uint sign_op;

   assert(swizzle >= TGSI_SWIZZLE_X);
   assert(swizzle <= TGSI_EXTSWIZZLE_ONE);

   if (swizzle == TGSI_EXTSWIZZLE_ONE) {
      /* Load const one float and early out */
      reg = get_const_one_reg(gen);
   }
   else if (swizzle == TGSI_EXTSWIZZLE_ZERO) {
      /* Load const zero float and early out */
      reg = get_itemp(gen);
      spe_xor(gen->f, reg, reg, reg);
   }
   else {
      assert(swizzle < 4);

      switch (src->SrcRegister.File) {
      case TGSI_FILE_TEMPORARY:
         reg = gen->temp_regs[src->SrcRegister.Index][swizzle];
         break;
      case TGSI_FILE_INPUT:
         {
            /* offset is measured in quadwords, not bytes */
            int offset = src->SrcRegister.Index * 4 + swizzle;
            reg = get_itemp(gen);
            reg_is_itemp = TRUE;
            /* Load:  reg = memory[(machine_reg) + offset] */
            spe_lqd(gen->f, reg, gen->inputs_reg, offset * 16);
         }
         break;
      case TGSI_FILE_IMMEDIATE:
         reg = gen->imm_regs[src->SrcRegister.Index][swizzle];
         break;
      case TGSI_FILE_CONSTANT:
         {
            /* offset is measured in quadwords, not bytes */
            int offset = src->SrcRegister.Index * 4 + swizzle;
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

   spe_comment(gen->f, -4, "Function prologue:");

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

   spe_comment(gen->f, -4, "Function epilogue:");

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


static boolean
emit_MOV(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, src_reg[4], dst_reg[4];

   spe_comment(gen->f, -4, "MOV:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         src_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         dst_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
      }
   }

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         if (is_register_src(gen, ch, &inst->FullSrcRegisters[0]) &&
             is_memory_dst(gen, ch, &inst->FullDstRegisters[0])) {
            /* special-case: register to memory store */
            store_dest_reg(gen, src_reg[ch], ch, &inst->FullDstRegisters[0]);
         }
         else {
            spe_move(gen->f, dst_reg[ch], src_reg[ch]);
            store_dest_reg(gen, dst_reg[ch], ch, &inst->FullDstRegisters[0]);
         }
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
   int ch, s1_reg[4], s2_reg[4], d_reg[4];

   spe_comment(gen->f, -4, "ADD:");
   /* Loop over Red/Green/Blue/Alpha channels, fetch src operands */
   for (ch = 0; ch < 4; ch++) {
      /* If the dest R, G, B or A writemask is enabled... */
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s1_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         s2_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         d_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
      }
   }
   /* Loop over Red/Green/Blue/Alpha channels, do the add, store results */
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         /* Emit actual SPE instruction: d = s1 + s2 */
         spe_fa(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         /* Store the result (a no-op for TGSI_FILE_TEMPORARY dests) */
         store_dest_reg(gen, d_reg[ch], ch, &inst->FullDstRegisters[0]);
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
   int ch, s1_reg[4], s2_reg[4], d_reg[4];
   spe_comment(gen->f, -4, "SUB:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s1_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         s2_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         d_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
      }
   }
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         /* d = s1 - s2 */
         spe_fs(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         store_dest_reg(gen, d_reg[ch], ch, &inst->FullDstRegisters[0]);
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
   int ch, s1_reg[4], s2_reg[4], s3_reg[4], d_reg[4];
   spe_comment(gen->f, -4, "MAD:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s1_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         s2_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         s3_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[2]);
         d_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
      }
   }
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         /* d = s1 * s2 + s3 */
         spe_fma(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch], s3_reg[ch]);
         store_dest_reg(gen, d_reg[ch], ch, &inst->FullDstRegisters[0]);
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
   int ch, s1_reg[4], s2_reg[4], s3_reg[4], d_reg[4], tmp_reg[4];
   spe_comment(gen->f, -4, "LERP:");
   /* setup/get src/dst/temp regs */
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s1_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         s2_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         s3_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[2]);
         d_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         tmp_reg[ch] = get_itemp(gen);
      }
   }

   /* d = s3 + s1(s2 - s3) */
   /* do all subtracts, then all fma, then all stores to better pipeline */
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         spe_fs(gen->f, tmp_reg[ch], s2_reg[ch], s3_reg[ch]);
      }
   }
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         spe_fma(gen->f, d_reg[ch], tmp_reg[ch], s1_reg[ch], s3_reg[ch]);
      }
   }
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         store_dest_reg(gen, d_reg[ch], ch, &inst->FullDstRegisters[0]);
      }
   }
   free_itemps(gen);
   return true;
}

/**
 * Emit multiply.  See emit_ADD for comments.
 */
static boolean
emit_MUL(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s1_reg[4], s2_reg[4], d_reg[4];
   spe_comment(gen->f, -4, "MUL:");
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s1_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         s2_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         d_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
      }
   }
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         /* d = s1 * s2 */
         spe_fm(gen->f, d_reg[ch], s1_reg[ch], s2_reg[ch]);
         store_dest_reg(gen, d_reg[ch], ch, &inst->FullDstRegisters[0]);
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
         spe_load_uint(gen->f, bit31mask_reg, (1 << 31));

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
   int s1x_reg, s1y_reg, s1z_reg;
   int s2x_reg, s2y_reg, s2z_reg;
   int t0_reg = get_itemp(gen), t1_reg = get_itemp(gen);

   spe_comment(gen->f, -4, "DP3:");

   s1x_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   s2x_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[1]);
   s1y_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   s2y_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[1]);
   s1z_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);
   s2z_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[1]);

   /* t0 = x0 * x1 */
   spe_fm(gen->f, t0_reg, s1x_reg, s2x_reg);

   /* t1 = y0 * y1 */
   spe_fm(gen->f, t1_reg, s1y_reg, s2y_reg);

   /* t0 = z0 * z1 + t0 */
   spe_fma(gen->f, t0_reg, s1z_reg, s2z_reg, t0_reg);

   /* t0 = t0 + t1 */
   spe_fa(gen->f, t0_reg, t0_reg, t1_reg);

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         spe_move(gen->f, d_reg, t0_reg);
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
   int s0x_reg, s0y_reg, s0z_reg, s0w_reg;
   int s1x_reg, s1y_reg, s1z_reg, s1w_reg;
   int t0_reg = get_itemp(gen), t1_reg = get_itemp(gen);

   spe_comment(gen->f, -4, "DP4:");

   s0x_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   s1x_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[1]);
   s0y_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   s1y_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[1]);
   s0z_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);
   s1z_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[1]);
   s0w_reg = get_src_reg(gen, CHAN_W, &inst->FullSrcRegisters[0]);
   s1w_reg = get_src_reg(gen, CHAN_W, &inst->FullSrcRegisters[1]);

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

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         spe_move(gen->f, d_reg, t0_reg);
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
      }
   }

   free_itemps(gen);
   return true;
}

/**
 * Emit homogeneous dot product.  See emit_ADD for comments.
 */
static boolean
emit_DPH(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   /* XXX rewrite this function to look more like DP3/DP4 */
   int ch;
   spe_comment(gen->f, -4, "DPH:");

   int s1_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   int s2_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[1]);
   int tmp_reg = get_itemp(gen);

   /* t = x0 * x1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[1]);
   /* t = y0 * y1 + t */
   spe_fma(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   s1_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[1]);
   /* t = z0 * z1 + t */
   spe_fma(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   s2_reg = get_src_reg(gen, CHAN_W, &inst->FullSrcRegisters[1]);
   /* t = w1 + t */
   spe_fa(gen->f, tmp_reg, s2_reg, tmp_reg);

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         spe_move(gen->f, d_reg, tmp_reg);
         store_dest_reg(gen, tmp_reg, ch, &inst->FullDstRegisters[0]);
      }
   }

   free_itemps(gen);
   return true;
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

   spe_comment(gen->f, -4, "NRM3:");

   src_reg[0] = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   src_reg[1] = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   src_reg[2] = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);

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

   for (ch = 0; ch < 3; ch++) {  /* NOTE: omit W channel */
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* dst = src[ch] * t1 */
         spe_fm(gen->f, d_reg, src_reg[ch], t1_reg);
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
      }
   }

   free_itemps(gen);
   return true;
}


/**
 * Emit cross product.  See emit_ADD for comments.
 */
static boolean
emit_XPD(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   spe_comment(gen->f, -4, "XPD:");

   int s1_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);
   int s2_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[1]);
   int tmp_reg = get_itemp(gen);

   /* t = z0 * y1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[1]);
   /* t = y0 * z1 - t */
   spe_fms(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << CHAN_X)) {
      store_dest_reg(gen, tmp_reg, CHAN_X, &inst->FullDstRegisters[0]);
   }

   s1_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[1]);
   /* t = x0 * z1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_Z, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[1]);
   /* t = z0 * x1 - t */
   spe_fms(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << CHAN_Y)) {
      store_dest_reg(gen, tmp_reg, CHAN_Y, &inst->FullDstRegisters[0]);
   }

   s1_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[1]);
   /* t = y0 * x1 */
   spe_fm(gen->f, tmp_reg, s1_reg, s2_reg);

   s1_reg = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[0]);
   s2_reg = get_src_reg(gen, CHAN_Y, &inst->FullSrcRegisters[1]);
   /* t = x0 * y1 - t */
   spe_fms(gen->f, tmp_reg, s1_reg, s2_reg, tmp_reg);

   if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << CHAN_Z)) {
      store_dest_reg(gen, tmp_reg, CHAN_Z, &inst->FullDstRegisters[0]);
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
 * Emit trunc.  
 * Convert float to signed int
 * Convert signed int to float
 */
static boolean
emit_TRUNC(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "TRUNC:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* Convert float to int */
         spe_cflts(gen->f, d_reg, s1_reg, 0);

         /* Convert int to float */
         spe_csflt(gen->f, d_reg, d_reg, 0);

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

   int zero_reg = get_itemp(gen);
   spe_xor(gen->f, zero_reg, zero_reg, zero_reg);
   
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         int tmp_reg = get_itemp(gen);

         /* If negative, subtract 1.0 */
         spe_fcgt(gen->f, tmp_reg, zero_reg, s1_reg);
         spe_selb(gen->f, tmp_reg, zero_reg, get_const_one_reg(gen), tmp_reg);
         spe_fs(gen->f, tmp_reg, s1_reg, tmp_reg);

         /* Convert float to int */
         spe_cflts(gen->f, tmp_reg, tmp_reg, 0);

         /* Convert int to float */
         spe_csflt(gen->f, d_reg, tmp_reg, 0);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
}

/**
 * Compute frac = Input - FLR(Input)
 */
static boolean
emit_FRC(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch;

   spe_comment(gen->f, -4, "FRC:");

   int zero_reg = get_itemp(gen);
   spe_xor(gen->f, zero_reg, zero_reg, zero_reg);

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         int tmp_reg = get_itemp(gen);

         /* If negative, subtract 1.0 */
         spe_fcgt(gen->f, tmp_reg, zero_reg, s1_reg);
         spe_selb(gen->f, tmp_reg, zero_reg, get_const_one_reg(gen), tmp_reg);
         spe_fs(gen->f, tmp_reg, s1_reg, tmp_reg);

         /* Convert float to int */
         spe_cflts(gen->f, tmp_reg, tmp_reg, 0);

         /* Convert int to float */
         spe_csflt(gen->f, tmp_reg, tmp_reg, 0);

         /* d = s1 - FLR(s1) */
         spe_fs(gen->f, d_reg, s1_reg, tmp_reg);

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   return true;
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
         s_regs[a] = get_src_reg(gen, CHAN_X, &inst->FullSrcRegisters[a]);
      }
      /* we'll call the function, put the return value in this register,
       * then replicate it across all write-enabled components in d_reg.
       */
      retval_reg = spe_allocate_available_register(gen->f);
   }

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int d_reg;
         ubyte usedRegs[SPE_NUM_REGS];
         uint i, numUsed;

         if (!scalar) {
            for (a = 0; a < num_args; a++) {
               s_regs[a] = get_src_reg(gen, ch, &inst->FullSrcRegisters[a]);
            }
         }

         d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

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

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }

   if (scalar) {
      spe_release_register(gen->f, retval_reg);
   }

   return true;
}


static boolean
emit_TEX(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   const uint target = inst->InstructionExtTexture.Texture;
   const uint unit = inst->FullSrcRegisters[1].SrcRegister.Index;
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

   assert(inst->FullSrcRegisters[1].SrcRegister.File == TGSI_FILE_SAMPLER);

   spe_comment(gen->f, -4, "CALL tex:");

   /* get src/dst reg info */
   for (ch = 0; ch < 4; ch++) {
      coord_regs[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
      d_regs[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
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

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         store_dest_reg(gen, d_regs[ch], ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
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
   spe_load_uint(gen->f, zero_reg, 0);

   cmp_reg = get_itemp(gen);

   /* get src regs */
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s_regs[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
      }
   }

   /* test if any src regs are < 0 */
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
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
   }

   if (gen->if_nesting) {
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
 * Emit max.  See emit_SGT for comments.
 */
static boolean
emit_MAX(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s0_reg[4], s1_reg[4], d_reg[4], tmp_reg[4];

   spe_comment(gen->f, -4, "MAX:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s0_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         s1_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         d_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         tmp_reg[ch] = get_itemp(gen);         
      }
   }

   /* d = (s0 > s1) ? s0 : s1 */
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         spe_fcgt(gen->f, tmp_reg[ch], s0_reg[ch], s1_reg[ch]);
      }
   }
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         spe_selb(gen->f, d_reg[ch], s1_reg[ch], s0_reg[ch], tmp_reg[ch]);
      }
   }

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         store_dest_reg(gen, d_reg[ch], ch, &inst->FullDstRegisters[0]);
      }
   }

   free_itemps(gen);
   return true;
}

/**
 * Emit max.  See emit_SGT for comments.
 */
static boolean
emit_MIN(struct codegen *gen, const struct tgsi_full_instruction *inst)
{
   int ch, s0_reg[4], s1_reg[4], d_reg[4], tmp_reg[4];

   spe_comment(gen->f, -4, "MIN:");

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         s0_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         s1_reg[ch] = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         d_reg[ch] = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         tmp_reg[ch] = get_itemp(gen);         
      }
   }

   /* d = (s1 > s0) ? s0 : s1 */
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         spe_fcgt(gen->f, tmp_reg[ch], s1_reg[ch], s0_reg[ch]);
      }
   }
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         spe_selb(gen->f, d_reg[ch], s1_reg[ch], s0_reg[ch], tmp_reg[ch]);
      }
   }

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         store_dest_reg(gen, d_reg[ch], ch, &inst->FullDstRegisters[0]);
      }
   }

   free_itemps(gen);
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
   emit_epilogue(gen);
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
   case TGSI_OPCODE_SWZ:
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
   case TGSI_OPCODE_DPH:
      return emit_DPH(gen, inst);
   case TGSI_OPCODE_NRM:
      return emit_NRM3(gen, inst);
   case TGSI_OPCODE_XPD:
      return emit_XPD(gen, inst);
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
   case TGSI_OPCODE_EXPBASE2:
      return emit_function_call(gen, inst, "spu_exp2", 1, TRUE);
   case TGSI_OPCODE_LOGBASE2:
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

      if (ch > 0 && val == immed->u.ImmediateFloat32[ch - 1].Float) {
         /* re-use previous register */
         gen->imm_regs[gen->num_imm][ch] = gen->imm_regs[gen->num_imm][ch - 1];
      }
      else {
         int reg = spe_allocate_available_register(gen->f);

         if (reg < 0)
            return false;

         /* update immediate map */
         gen->imm_regs[gen->num_imm][ch] = reg;

         /* emit initializer instruction */
         spe_load_float(gen->f, reg, val);
      }
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

         {
            char buf[100];
            sprintf(buf, "TGSI temp[%d] maps to SPU regs [$%d $%d $%d $%d]", i,
                    gen->temp_regs[i][0], gen->temp_regs[i][1],
                    gen->temp_regs[i][2], gen->temp_regs[i][3]);
            spe_comment(gen->f, -4, buf);
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
      spe_print_code(f, true);
      spe_indent(f, 8);
      printf("Begin %s\n", __FUNCTION__);
      tgsi_dump(tokens, 0);
   }

   tgsi_parse_init(&parse, tokens);

   emit_prologue(&gen);

   while (!tgsi_parse_end_of_tokens(&parse) && !gen.error) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         if (!emit_immediate(&gen, &parse.FullToken.FullImmediate))
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
