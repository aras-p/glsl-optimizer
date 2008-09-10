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

#include "pipe/p_debug.h"
#include "tgsi_sanity.h"
#include "tgsi_info.h"
#include "tgsi_iterate.h"

#define MAX_REGISTERS 256

typedef uint reg_flag;

#define BITS_IN_REG_FLAG (sizeof( reg_flag ) * 8)

struct sanity_check_ctx
{
   struct tgsi_iterate_context iter;

   reg_flag regs_decl[TGSI_FILE_COUNT][MAX_REGISTERS / BITS_IN_REG_FLAG];
   reg_flag regs_used[TGSI_FILE_COUNT][MAX_REGISTERS / BITS_IN_REG_FLAG];
   boolean regs_ind_used[TGSI_FILE_COUNT];
   uint num_imms;
   uint num_instructions;
   uint index_of_END;

   uint errors;
   uint warnings;
};

static void
report_error(
   struct sanity_check_ctx *ctx,
   const char *format,
   ... )
{
   va_list args;

   debug_printf( "Error  : " );
   va_start( args, format );
   _debug_vprintf( format, args );
   va_end( args );
   debug_printf( "\n" );
   ctx->errors++;
}

static void
report_warning(
   struct sanity_check_ctx *ctx,
   const char *format,
   ... )
{
   va_list args;

   debug_printf( "Warning: " );
   va_start( args, format );
   _debug_vprintf( format, args );
   va_end( args );
   debug_printf( "\n" );
   ctx->warnings++;
}

static boolean
check_file_name(
   struct sanity_check_ctx *ctx,
   uint file )
{
   if (file <= TGSI_FILE_NULL || file >= TGSI_FILE_COUNT) {
      report_error( ctx, "Invalid register file name" );
      return FALSE;
   }
   return TRUE;
}

static boolean
is_register_declared(
   struct sanity_check_ctx *ctx,
   uint file,
   int index )
{
   assert( index >= 0 && index < MAX_REGISTERS );

   return (ctx->regs_decl[file][index / BITS_IN_REG_FLAG] & (1 << (index % BITS_IN_REG_FLAG))) ? TRUE : FALSE;
}

static boolean
is_any_register_declared(
   struct sanity_check_ctx *ctx,
   uint file )
{
   uint i;

   for (i = 0; i < MAX_REGISTERS / BITS_IN_REG_FLAG; i++)
      if (ctx->regs_decl[file][i])
         return TRUE;
   return FALSE;
}

static boolean
is_register_used(
   struct sanity_check_ctx *ctx,
   uint file,
   int index )
{
   assert( index < MAX_REGISTERS );

   return (ctx->regs_used[file][index / BITS_IN_REG_FLAG] & (1 << (index % BITS_IN_REG_FLAG))) ? TRUE : FALSE;
}

static const char *file_names[] =
{
   "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMP",
   "ADDR",
   "IMM"
};

static boolean
check_register_usage(
   struct sanity_check_ctx *ctx,
   uint file,
   int index,
   const char *name,
   boolean indirect_access )
{
   if (!check_file_name( ctx, file ))
      return FALSE;
   if (indirect_access) {
      if (!is_any_register_declared( ctx, file ))
         report_error( ctx, "%s: Undeclared %s register", file_names[file], name );
      ctx->regs_ind_used[file] = TRUE;
   }
   else {
      if (!is_register_declared( ctx, file, index ))
         report_error( ctx, "%s[%d]: Undeclared %s register", file_names[file], index, name );
      ctx->regs_used[file][index / BITS_IN_REG_FLAG] |= (1 << (index % BITS_IN_REG_FLAG));
   }
   return TRUE;
}

static boolean
iter_instruction(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_instruction *inst )
{
   struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;
   const struct tgsi_opcode_info *info;
   uint i;

   /* There must be no other instructions after END.
    */
   if (ctx->index_of_END != ~0) {
      report_error( ctx, "Unexpected instruction after END" );
   }
   else if (inst->Instruction.Opcode == TGSI_OPCODE_END) {
      ctx->index_of_END = ctx->num_instructions;
   }

   info = tgsi_get_opcode_info( inst->Instruction.Opcode );
   if (info == NULL) {
      report_error( ctx, "Invalid instruction opcode" );
      return TRUE;
   }

   if (info->num_dst != inst->Instruction.NumDstRegs) {
      report_error( ctx, "Invalid number of destination operands" );
   }
   if (info->num_src != inst->Instruction.NumSrcRegs) {
      report_error( ctx, "Invalid number of source operands" );
   }

   /* Check destination and source registers' validity.
    * Mark the registers as used.
    */
   for (i = 0; i < inst->Instruction.NumDstRegs; i++) {
      check_register_usage(
         ctx,
         inst->FullDstRegisters[i].DstRegister.File,
         inst->FullDstRegisters[i].DstRegister.Index,
         "destination",
         FALSE );
   }
   for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
      check_register_usage(
         ctx,
         inst->FullSrcRegisters[i].SrcRegister.File,
         inst->FullSrcRegisters[i].SrcRegister.Index,
         "source",
         (boolean)inst->FullSrcRegisters[i].SrcRegister.Indirect );
      if (inst->FullSrcRegisters[i].SrcRegister.Indirect) {
         uint file;
         int index;

         file = inst->FullSrcRegisters[i].SrcRegisterInd.File;
         index = inst->FullSrcRegisters[i].SrcRegisterInd.Index;
         check_register_usage(
            ctx,
            file,
            index,
            "indirect",
            FALSE );
         if (file != TGSI_FILE_ADDRESS || index != 0)
            report_warning( ctx, "Indirect register not ADDR[0]" );
      }
   }

   ctx->num_instructions++;

   return TRUE;
}

static boolean
iter_declaration(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_declaration *decl )
{
   struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;
   uint file;
   uint i;

   /* No declarations allowed after the first instruction.
    */
   if (ctx->num_instructions > 0)
      report_error( ctx, "Instruction expected but declaration found" );

   /* Check registers' validity.
    * Mark the registers as declared.
    */
   file = decl->Declaration.File;
   if (!check_file_name( ctx, file ))
      return TRUE;
   for (i = decl->DeclarationRange.First; i <= decl->DeclarationRange.Last; i++) {
      if (is_register_declared( ctx, file, i ))
         report_error( ctx, "The same register declared twice" );
      ctx->regs_decl[file][i / BITS_IN_REG_FLAG] |= (1 << (i % BITS_IN_REG_FLAG));
   }

   return TRUE;
}

static boolean
iter_immediate(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_immediate *imm )
{
   struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;

   assert( ctx->num_imms < MAX_REGISTERS );

   /* No immediates allowed after the first instruction.
    */
   if (ctx->num_instructions > 0)
      report_error( ctx, "Instruction expected but immediate found" );

   /* Mark the register as declared.
    */
   ctx->regs_decl[TGSI_FILE_IMMEDIATE][ctx->num_imms / BITS_IN_REG_FLAG] |= (1 << (ctx->num_imms % BITS_IN_REG_FLAG));
   ctx->num_imms++;

   /* Check data type validity.
    */
   if (imm->Immediate.DataType != TGSI_IMM_FLOAT32) {
      report_error( ctx, "Invalid immediate data type" );
      return TRUE;
   }

   return TRUE;
}

static boolean
epilog(
   struct tgsi_iterate_context *iter )
{
   struct sanity_check_ctx *ctx = (struct sanity_check_ctx *) iter;
   uint file;

   /* There must be an END instruction at the end.
    */
   if (ctx->index_of_END == ~0 || ctx->index_of_END != ctx->num_instructions - 1) {
      report_error( ctx, "Expected END at end of instruction sequence" );
   }

   /* Check if all declared registers were used.
    */
   for (file = TGSI_FILE_NULL; file < TGSI_FILE_COUNT; file++) {
      uint i;

      for (i = 0; i < MAX_REGISTERS; i++) {
         if (is_register_declared( ctx, file, i ) && !is_register_used( ctx, file, i ) && !ctx->regs_ind_used[file]) {
            report_warning( ctx, "Register never used" );
         }
      }
   }

   /* Print totals, if any.
    */
   if (ctx->errors || ctx->warnings)
      debug_printf( "%u errors, %u warnings\n", ctx->errors, ctx->warnings );

   return TRUE;
}

boolean
tgsi_sanity_check(
   struct tgsi_token *tokens )
{
   struct sanity_check_ctx ctx;

   ctx.iter.prolog = NULL;
   ctx.iter.iterate_instruction = iter_instruction;
   ctx.iter.iterate_declaration = iter_declaration;
   ctx.iter.iterate_immediate = iter_immediate;
   ctx.iter.epilog = epilog;

   memset( ctx.regs_decl, 0, sizeof( ctx.regs_decl ) );
   memset( ctx.regs_used, 0, sizeof( ctx.regs_used ) );
   memset( ctx.regs_ind_used, 0, sizeof( ctx.regs_ind_used ) );
   ctx.num_imms = 0;
   ctx.num_instructions = 0;
   ctx.index_of_END = ~0;

   ctx.errors = 0;
   ctx.warnings = 0;

   if (!tgsi_iterate_shader( tokens, &ctx.iter ))
      return FALSE;

   return ctx.errors == 0;
}
