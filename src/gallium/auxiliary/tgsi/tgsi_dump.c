/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include "tgsi_dump.h"
#include "tgsi_iterate.h"

struct dump_ctx
{
   struct tgsi_iterate_context iter;

   uint instno;
};

static void
dump_enum(
   uint e,
   const char **enums,
   uint enum_count )
{
   if (e >= enum_count)
      debug_printf( "%u", e );
   else
      debug_printf( "%s", enums[e] );
}

#define EOL()           debug_printf( "\n" )
#define TXT(S)          debug_printf( "%s", S )
#define CHR(C)          debug_printf( "%c", C )
#define UIX(I)          debug_printf( "0x%x", I )
#define UID(I)          debug_printf( "%u", I )
#define SID(I)          debug_printf( "%d", I )
#define FLT(F)          debug_printf( "%10.4f", F )
#define ENM(E,ENUMS)    dump_enum( E, ENUMS, sizeof( ENUMS ) / sizeof( *ENUMS ) )

static const char *processor_type_names[] =
{
   "FRAG",
   "VERT",
   "GEOM"
};

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

static const char *interpolate_names[] =
{
   "CONSTANT",
   "LINEAR",
   "PERSPECTIVE"
};

static const char *semantic_names[] =
{
   "POSITION",
   "COLOR",
   "BCOLOR",
   "FOG",
   "PSIZE",
   "GENERIC",
   "NORMAL"
};

static const char *immediate_type_names[] =
{
   "FLT32"
};

static const char *opcode_names[TGSI_OPCODE_LAST] =
{
   "ARL",
   "MOV",
   "LIT",
   "RCP",
   "RSQ",
   "EXP",
   "LOG",
   "MUL",
   "ADD",
   "DP3",
   "DP4",
   "DST",
   "MIN",
   "MAX",
   "SLT",
   "SGE",
   "MAD",
   "SUB",
   "LERP",
   "CND",
   "CND0",
   "DOT2ADD",
   "INDEX",
   "NEGATE",
   "FRAC",
   "CLAMP",
   "FLOOR",
   "ROUND",
   "EXPBASE2",
   "LOGBASE2",
   "POWER",
   "CROSSPRODUCT",
   "MULTIPLYMATRIX",
   "ABS",
   "RCC",
   "DPH",
   "COS",
   "DDX",
   "DDY",
   "KILP",
   "PK2H",
   "PK2US",
   "PK4B",
   "PK4UB",
   "RFL",
   "SEQ",
   "SFL",
   "SGT",
   "SIN",
   "SLE",
   "SNE",
   "STR",
   "TEX",
   "TXD",
   "TXP",
   "UP2H",
   "UP2US",
   "UP4B",
   "UP4UB",
   "X2D",
   "ARA",
   "ARR",
   "BRA",
   "CAL",
   "RET",
   "SSG",
   "CMP",
   "SCS",
   "TXB",
   "NRM",
   "DIV",
   "DP2",
   "TXL",
   "BRK",
   "IF",
   "LOOP",
   "REP",
   "ELSE",
   "ENDIF",
   "ENDLOOP",
   "ENDREP",
   "PUSHA",
   "POPA",
   "CEIL",
   "I2F",
   "NOT",
   "TRUNC",
   "SHL",
   "SHR",
   "AND",
   "OR",
   "MOD",
   "XOR",
   "SAD",
   "TXF",
   "TXQ",
   "CONT",
   "EMIT",
   "ENDPRIM",
   "BGNLOOP2",
   "BGNSUB",
   "ENDLOOP2",
   "ENDSUB",
   "NOISE1",
   "NOISE2",
   "NOISE3",
   "NOISE4",
   "NOP",
   "M4X3",
   "M3X4",
   "M3X3",
   "M3X2",
   "NRM4",
   "CALLNZ",
   "IFC",
   "BREAKC",
   "KIL",
   "END",
   "SWZ"
};

static const char *swizzle_names[] =
{
   "x",
   "y",
   "z",
   "w"
};

static const char *texture_names[] =
{
   "UNKNOWN",
   "1D",
   "2D",
   "3D",
   "CUBE",
   "RECT",
   "SHADOW1D",
   "SHADOW2D",
   "SHADOWRECT"
};

static const char *extswizzle_names[] =
{
   "x",
   "y",
   "z",
   "w",
   "0",
   "1"
};

static const char *modulate_names[TGSI_MODULATE_COUNT] =
{
   "",
   "_2X",
   "_4X",
   "_8X",
   "_D2",
   "_D4",
   "_D8"
};

static void
_dump_register(
   uint file,
   int first,
   int last )
{
   ENM( file, file_names );
   CHR( '[' );
   SID( first );
   if (first != last) {
      TXT( ".." );
      SID( last );
   }
   CHR( ']' );
}

static void
_dump_register_ind(
   uint file,
   int index,
   uint ind_file,
   int ind_index )
{
   ENM( file, file_names );
   CHR( '[' );
   ENM( ind_file, file_names );
   CHR( '[' );
   SID( ind_index );
   CHR( ']' );
   if (index != 0) {
      if (index > 0)
         CHR( '+' );
      SID( index );
   }
   CHR( ']' );
}

static void
_dump_writemask(
   uint writemask )
{
   if (writemask != TGSI_WRITEMASK_XYZW) {
      CHR( '.' );
      if (writemask & TGSI_WRITEMASK_X)
         CHR( 'x' );
      if (writemask & TGSI_WRITEMASK_Y)
         CHR( 'y' );
      if (writemask & TGSI_WRITEMASK_Z)
         CHR( 'z' );
      if (writemask & TGSI_WRITEMASK_W)
         CHR( 'w' );
   }
}

void
tgsi_dump_declaration(
   const struct tgsi_full_declaration *decl )
{
   TXT( "DCL " );

   _dump_register(
      decl->Declaration.File,
      decl->DeclarationRange.First,
      decl->DeclarationRange.Last );
   _dump_writemask(
      decl->Declaration.UsageMask );

   if (decl->Declaration.Semantic) {
      TXT( ", " );
      ENM( decl->Semantic.SemanticName, semantic_names );
      if (decl->Semantic.SemanticIndex != 0 ||
          decl->Semantic.SemanticName == TGSI_SEMANTIC_GENERIC) {
         CHR( '[' );
         UID( decl->Semantic.SemanticIndex );
         CHR( ']' );
      }
   }

   TXT( ", " );
   ENM( decl->Declaration.Interpolate, interpolate_names );

   EOL();
}

static boolean
iter_declaration(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_declaration *decl )
{
   tgsi_dump_declaration( decl );
   return TRUE;
}

void
tgsi_dump_immediate(
   const struct tgsi_full_immediate *imm )
{
   uint i;

   TXT( "IMM " );
   ENM( imm->Immediate.DataType, immediate_type_names );

   TXT( " { " );
   for (i = 0; i < imm->Immediate.Size - 1; i++) {
      switch (imm->Immediate.DataType) {
      case TGSI_IMM_FLOAT32:
         FLT( imm->u.ImmediateFloat32[i].Float );
         break;
      default:
         assert( 0 );
      }

      if (i < imm->Immediate.Size - 2)
         TXT( ", " );
   }
   TXT( " }" );

   EOL();
}

static boolean
iter_immediate(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_immediate *imm )
{
   tgsi_dump_immediate( imm );
   return TRUE;
}

void
tgsi_dump_instruction(
   const struct tgsi_full_instruction *inst,
   uint instno )
{
   uint i;
   boolean first_reg = TRUE;

   UID( instno );
   CHR( ':' );
   ENM( inst->Instruction.Opcode, opcode_names );

   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      TXT( "_SAT" );
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      TXT( "_SATNV" );
      break;
   default:
      assert( 0 );
   }

   for (i = 0; i < inst->Instruction.NumDstRegs; i++) {
      const struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];

      if (!first_reg)
         CHR( ',' );
      CHR( ' ' );

      _dump_register(
         dst->DstRegister.File,
         dst->DstRegister.Index,
         dst->DstRegister.Index );
      ENM( dst->DstRegisterExtModulate.Modulate, modulate_names );
      _dump_writemask( dst->DstRegister.WriteMask );

      first_reg = FALSE;
   }

   for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
      const struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];

      if (!first_reg)
         CHR( ',' );
      CHR( ' ' );

      if (src->SrcRegisterExtMod.Negate)
         TXT( "-(" );
      if (src->SrcRegisterExtMod.Absolute)
         CHR( '|' );
      if (src->SrcRegisterExtMod.Scale2X)
         TXT( "2*(" );
      if (src->SrcRegisterExtMod.Bias)
         CHR( '(' );
      if (src->SrcRegisterExtMod.Complement)
         TXT( "1-(" );
      if (src->SrcRegister.Negate)
         CHR( '-' );

      if (src->SrcRegister.Indirect) {
         _dump_register_ind(
            src->SrcRegister.File,
            src->SrcRegister.Index,
            src->SrcRegisterInd.File,
            src->SrcRegisterInd.Index );
      }
      else {
         _dump_register(
            src->SrcRegister.File,
            src->SrcRegister.Index,
            src->SrcRegister.Index );
      }

      if (src->SrcRegister.SwizzleX != TGSI_SWIZZLE_X ||
          src->SrcRegister.SwizzleY != TGSI_SWIZZLE_Y ||
          src->SrcRegister.SwizzleZ != TGSI_SWIZZLE_Z ||
          src->SrcRegister.SwizzleW != TGSI_SWIZZLE_W) {
         CHR( '.' );
         ENM( src->SrcRegister.SwizzleX, swizzle_names );
         ENM( src->SrcRegister.SwizzleY, swizzle_names );
         ENM( src->SrcRegister.SwizzleZ, swizzle_names );
         ENM( src->SrcRegister.SwizzleW, swizzle_names );
      }
      if (src->SrcRegisterExtSwz.ExtSwizzleX != TGSI_EXTSWIZZLE_X ||
          src->SrcRegisterExtSwz.ExtSwizzleY != TGSI_EXTSWIZZLE_Y ||
          src->SrcRegisterExtSwz.ExtSwizzleZ != TGSI_EXTSWIZZLE_Z ||
          src->SrcRegisterExtSwz.ExtSwizzleW != TGSI_EXTSWIZZLE_W) {
         CHR( '.' );
         if (src->SrcRegisterExtSwz.NegateX)
            TXT("-");
         ENM( src->SrcRegisterExtSwz.ExtSwizzleX, extswizzle_names );
         if (src->SrcRegisterExtSwz.NegateY)
            TXT("-");
         ENM( src->SrcRegisterExtSwz.ExtSwizzleY, extswizzle_names );
         if (src->SrcRegisterExtSwz.NegateZ)
            TXT("-");
         ENM( src->SrcRegisterExtSwz.ExtSwizzleZ, extswizzle_names );
         if (src->SrcRegisterExtSwz.NegateW)
            TXT("-");
         ENM( src->SrcRegisterExtSwz.ExtSwizzleW, extswizzle_names );
      }

      if (src->SrcRegisterExtMod.Complement)
         CHR( ')' );
      if (src->SrcRegisterExtMod.Bias)
         TXT( ")-.5" );
      if (src->SrcRegisterExtMod.Scale2X)
         CHR( ')' );
      if (src->SrcRegisterExtMod.Absolute)
         CHR( '|' );
      if (src->SrcRegisterExtMod.Negate)
         CHR( ')' );

      first_reg = FALSE;
   }

   if (inst->InstructionExtTexture.Texture != TGSI_TEXTURE_UNKNOWN) {
      TXT( ", " );
      ENM( inst->InstructionExtTexture.Texture, texture_names );
   }

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_ELSE:
   case TGSI_OPCODE_BGNLOOP2:
   case TGSI_OPCODE_ENDLOOP2:
   case TGSI_OPCODE_CAL:
      TXT( " :" );
      UID( inst->InstructionExtLabel.Label );
      break;
   }

   EOL();
}

static boolean
iter_instruction(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_instruction *inst )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;

   tgsi_dump_instruction( inst, ctx->instno++ );
   return TRUE;
}

static boolean
prolog(
   struct tgsi_iterate_context *ctx )
{
   ENM( ctx->processor.Processor, processor_type_names );
   UID( ctx->version.MajorVersion );
   CHR( '.' );
   UID( ctx->version.MinorVersion );
   EOL();
   return TRUE;
}

void
tgsi_dump(
   const struct tgsi_token *tokens,
   uint flags )
{
   struct dump_ctx ctx;

   /* sanity checks */
   assert( strcmp( opcode_names[TGSI_OPCODE_CONT], "CONT" ) == 0 );
   assert( strcmp( opcode_names[TGSI_OPCODE_END], "END" ) == 0 );

   ctx.iter.prolog = prolog;
   ctx.iter.iterate_instruction = iter_instruction;
   ctx.iter.iterate_declaration = iter_declaration;
   ctx.iter.iterate_immediate = iter_immediate;
   ctx.iter.epilog = NULL;

   ctx.instno = 0;

   tgsi_iterate_shader( tokens, &ctx.iter );
}
