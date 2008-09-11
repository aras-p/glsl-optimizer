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


/** Set to 1 to enable debug/disassembly printfs */
#define DISASSEM 01


/**
 * Context needed during code generation.
 */
struct codegen
{
   int inputs_reg;      /**< 1st function parameter */
   int outputs_reg;     /**< 2nd function parameter */
   int constants_reg;   /**< 3rd function parameter */
   int temp_regs[8][4]; /**< maps TGSI temps to SPE registers */

   int one_reg;         /**< register containing {1.0, 1.0, 1.0, 1.0} */

   /** Per-instruction temps / intermediate temps */
   int num_itemps;
   int itemps[3];

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
   }

   /* one = {1.0, 1.0, 1.0, 1.0} */
   spe_load_float(gen->f, gen->one_reg, 1.0f);
#if DISASSEM
   printf("il\tr%d, 1.0f\n", gen->one_reg);
#endif

   return gen->one_reg;
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
   int reg;

   /* XXX need to examine src swizzle info here.
    * That will involve changing the channel var...
    */


   switch (src->SrcRegister.File) {
   case TGSI_FILE_TEMPORARY:
      reg = gen->temp_regs[src->SrcRegister.Index][channel];
      break;
   case TGSI_FILE_INPUT:
      {
         /* offset is measured in quadwords, not bytes */
         int offset = src->SrcRegister.Index * 4 + channel;
         reg = get_itemp(gen);
         /* Load:  reg = memory[(machine_reg) + offset] */
         spe_lqd(gen->f, reg, gen->inputs_reg, offset);
#if DISASSEM
         printf("lqd\tr%d, r%d + %d\n", reg, gen->inputs_reg, offset);
#endif
      }
      break;
   case TGSI_FILE_IMMEDIATE:
      /* xxx fall-through for now / fix */
   case TGSI_FILE_CONSTANT:
      /* xxx fall-through for now / fix */
   default:
      assert(0);
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
   int reg;

   switch (dest->DstRegister.File) {
   case TGSI_FILE_TEMPORARY:
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
      /* no-op */
      break;
   case TGSI_FILE_OUTPUT:
      {
         /* offset is measured in quadwords, not bytes */
         int offset = dest->DstRegister.Index * 4 + channel;
         /* Store: memory[(machine_reg) + offset] = reg */
         spe_stqd(gen->f, value_reg, gen->outputs_reg, offset);
#if DISASSEM
         printf("stqd\tr%d, r%d + %d\n", value_reg, gen->outputs_reg, offset);
#endif
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
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int src_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int dst_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* XXX we don't always need to actually emit a mov instruction here */
         spe_move(gen->f, dst_reg, src_reg);
#if DISASSEM
         printf("mov\tr%d, r%d\n", dst_reg, src_reg);
#endif
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
#if DISASSEM
         printf("fa\tr%d, r%d, r%d\n", d_reg, s1_reg, s2_reg);
#endif

         /* Store the result (a no-op for TGSI_FILE_TEMPORARY dests) */
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         /* Free any intermediate temps we allocated */
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
   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);
         /* d = s1 * s2 */
         spe_fm(gen->f, d_reg, s1_reg, s2_reg);
#if DISASSEM
         printf("fm\tr%d, r%d, r%d\n", d_reg, s1_reg, s2_reg);
#endif
         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
         free_itemps(gen);
      }
   }
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

   for (ch = 0; ch < 4; ch++) {
      if (inst->FullDstRegisters[0].DstRegister.WriteMask & (1 << ch)) {
         int s1_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[0]);
         int s2_reg = get_src_reg(gen, ch, &inst->FullSrcRegisters[1]);
         int d_reg = get_dst_reg(gen, ch, &inst->FullDstRegisters[0]);

         /* d = (s1 > s2) */
         spe_fcgt(gen->f, d_reg, s1_reg, s2_reg);
#if DISASSEM
         printf("fcgt\tr%d, r%d, r%d\n", d_reg, s1_reg, s2_reg);
#endif

         /* convert d from 0x0/0xffffffff to 0.0/1.0 */
         /* d = d & one_reg */
         spe_and(gen->f, d_reg, d_reg, get_const_one_reg(gen));
#if DISASSEM
         printf("and\tr%d, r%d, r%d\n", d_reg, d_reg, get_const_one_reg(gen));
#endif

         store_dest_reg(gen, d_reg, ch, &inst->FullDstRegisters[0]);
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
   /* return from function call */
   spe_bi(gen->f, SPE_REG_RA, 0, 0);
#if DISASSEM
   printf("bi\trRA\n");
#endif
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
   case TGSI_OPCODE_SGT:
      return emit_SGT(gen, inst);
   case TGSI_OPCODE_END:
      return emit_END(gen);

   /* XXX lots more cases to do... */

   default:
      return false;
   }

   return true;
}



/**
 * Emit "code" for a TGSI declaration.
 * We only care about TGSI TEMPORARY register declarations at this time.
 * For each TGSI TEMPORARY we allocate four SPE registers.
 */
static void
emit_declaration(struct codegen *gen, const struct tgsi_full_declaration *decl)
{
   int i, ch;

   switch (decl->Declaration.File) {
   case TGSI_FILE_TEMPORARY:
#if DISASSEM
      printf("Declare temp reg %d .. %d\n",
             decl->DeclarationRange.First,
             decl->DeclarationRange.Last);
#endif
      for (i = decl->DeclarationRange.First;
           i <= decl->DeclarationRange.Last;
           i++) {
         for (ch = 0; ch < 4; ch++) {
            gen->temp_regs[i][ch] = spe_allocate_available_register(gen->f);
         }

         /* XXX if we run out of SPE registers, we need to spill
          * to SPU memory.  someday...
          */

#if DISASSEM
         printf("  SPE regs: %d %d %d %d\n",
                gen->temp_regs[i][0],
                gen->temp_regs[i][1],
                gen->temp_regs[i][2],
                gen->temp_regs[i][3]);
#endif
      }
      break;
   default:
      ; /* ignore */
   }
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

#if DISASSEM
   printf("Begin %s\n", __FUNCTION__);
   tgsi_dump(tokens, 0);
#endif

   tgsi_parse_init(&parse, tokens);

   while (!tgsi_parse_end_of_tokens(&parse) && !gen.error) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_IMMEDIATE:
#if 0
         if (!note_immediate(&gen, &parse.FullToken.FullImmediate ))
            goto fail;
#endif
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         emit_declaration(&gen, &parse.FullToken.FullDeclaration);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (!emit_instruction(&gen, &parse.FullToken.FullInstruction )) {
            gen.error = true;
         }
         break;

      default:
         assert(0);

      }
   }


   if (gen.error) {
      /* terminate the SPE code */
      return emit_END(&gen);
   }

#if DISASSEM
   printf("cell_gen_fragment_program nr instructions: %d\n", f->num_inst);
   printf("End %s\n", __FUNCTION__);
#endif

   tgsi_parse_free( &parse );

   return !gen.error;
}
