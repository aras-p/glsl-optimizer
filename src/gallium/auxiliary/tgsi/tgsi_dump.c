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
#include "util/u_string.h"
#include "tgsi_dump.h"
#include "tgsi_info.h"
#include "tgsi_iterate.h"

struct dump_ctx
{
   struct tgsi_iterate_context iter;

   uint instno;
   
   struct util_strbuf *sbuf;
};

static void
dump_enum(
   struct util_strbuf *sbuf,
   uint e,
   const char **enums,
   uint enum_count )
{
   if (e >= enum_count)
      util_strbuf_printf( sbuf, "%u", e );
   else
      util_strbuf_printf( sbuf, "%s", enums[e] );
}

#define EOL()           util_strbuf_printf( sbuf, "\n" )
#define TXT(S)          util_strbuf_printf( sbuf, "%s", S )
#define CHR(C)          util_strbuf_printf( sbuf, "%c", C )
#define UIX(I)          util_strbuf_printf( sbuf, "0x%x", I )
#define UID(I)          util_strbuf_printf( sbuf, "%u", I )
#define SID(I)          util_strbuf_printf( sbuf, "%d", I )
#define FLT(F)          util_strbuf_printf( sbuf, "%10.4f", F )
#define ENM(E,ENUMS)    dump_enum( sbuf, E, ENUMS, sizeof( ENUMS ) / sizeof( *ENUMS ) )

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
   struct util_strbuf *sbuf,
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
   struct util_strbuf *sbuf,
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
   struct util_strbuf *sbuf,
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

static boolean
iter_declaration(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_declaration *decl )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;
   struct util_strbuf *sbuf = ctx->sbuf;

   TXT( "DCL " );

   _dump_register(
      sbuf,
      decl->Declaration.File,
      decl->DeclarationRange.First,
      decl->DeclarationRange.Last );
   _dump_writemask(
      sbuf,
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

   return TRUE;
}

void
tgsi_dump_declaration(
   const struct tgsi_full_declaration *decl )
{
   static char str[1024];
   struct util_strbuf sbuf;
   struct dump_ctx ctx;

   util_strbuf_init(&sbuf, str, sizeof(str));
   
   ctx.sbuf = &sbuf;

   iter_declaration( &ctx.iter, (struct tgsi_full_declaration *)decl );
   
   debug_printf("%s", str);
}

static boolean
iter_immediate(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_immediate *imm )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;
   struct util_strbuf *sbuf = ctx->sbuf;

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

   return TRUE;
}

void
tgsi_dump_immediate(
   const struct tgsi_full_immediate *imm )
{
   static char str[1024];
   struct util_strbuf sbuf;
   struct dump_ctx ctx;

   util_strbuf_init(&sbuf, str, sizeof(str));
   
   ctx.sbuf = &sbuf;

   iter_immediate( &ctx.iter, (struct tgsi_full_immediate *)imm );
   
   debug_printf("%s", str);
}

static boolean
iter_instruction(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_instruction *inst )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;
   struct util_strbuf *sbuf = ctx->sbuf;
   uint instno = ctx->instno++;
   
   uint i;
   boolean first_reg = TRUE;

   UID( instno );
   CHR( ':' );
   TXT( tgsi_get_opcode_info( inst->Instruction.Opcode )->mnemonic );

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
         sbuf,
         dst->DstRegister.File,
         dst->DstRegister.Index,
         dst->DstRegister.Index );
      ENM( dst->DstRegisterExtModulate.Modulate, modulate_names );
      _dump_writemask( sbuf, dst->DstRegister.WriteMask );

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
            sbuf,
            src->SrcRegister.File,
            src->SrcRegister.Index,
            src->SrcRegisterInd.File,
            src->SrcRegisterInd.Index );
      }
      else {
         _dump_register(
            sbuf,
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

   return TRUE;
}

void
tgsi_dump_instruction(
   const struct tgsi_full_instruction *inst,
   uint instno )
{
   static char str[1024];
   struct util_strbuf sbuf;
   struct dump_ctx ctx;

   util_strbuf_init(&sbuf, str, sizeof(str));
   
   ctx.instno = instno;
   ctx.sbuf = &sbuf;

   iter_instruction( &ctx.iter, (struct tgsi_full_instruction *)inst );
   
   debug_printf("%s", str);
}

static boolean
prolog(
   struct tgsi_iterate_context *iter )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;
   struct util_strbuf *sbuf = ctx->sbuf;
   ENM( iter->processor.Processor, processor_type_names );
   UID( iter->version.MajorVersion );
   CHR( '.' );
   UID( iter->version.MinorVersion );
   EOL();
   return TRUE;
}

void
tgsi_dump_str(
   const struct tgsi_token *tokens,
   uint flags,
   char *str,
   size_t size)
{
   struct util_strbuf sbuf;
   struct dump_ctx ctx;

   util_strbuf_init(&sbuf, str, size);
   
   ctx.iter.prolog = prolog;
   ctx.iter.iterate_instruction = iter_instruction;
   ctx.iter.iterate_declaration = iter_declaration;
   ctx.iter.iterate_immediate = iter_immediate;
   ctx.iter.epilog = NULL;

   ctx.instno = 0;
   ctx.sbuf = &sbuf;

   tgsi_iterate_shader( tokens, &ctx.iter );
}

void
tgsi_dump(
   const struct tgsi_token *tokens,
   uint flags )
{
   static char str[4096];
   uint len;
   char *p = str;

   tgsi_dump_str(tokens, flags, str, sizeof(str));

   /* Workaround output buffer size limitations.
    */
   len = strlen( str );
   while (len > 256) {
      char piggy_bank;

      piggy_bank = p[256];
      p[256] = '\0';
      debug_printf( "%s", p );
      p[256] = piggy_bank;
      p += 256;
      len -= 256;
   }
   debug_printf( "%s", p );
}
