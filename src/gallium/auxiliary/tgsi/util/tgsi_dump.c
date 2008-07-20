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
#include "pipe/p_util.h"
#include "util/u_string.h"
#include "tgsi_dump.h"
#include "tgsi_parse.h"
#include "tgsi_build.h"

static void
dump_enum(
   const unsigned    e,
   const char        **enums,
   const unsigned    enums_count )
{
   if (e >= enums_count) {
      debug_printf( "%u", e );
   }
   else {
      debug_printf( "%s", enums[e] );
   }
}

#define EOL()           debug_printf( "\n" )
#define TXT(S)          debug_printf( "%s", S )
#define CHR(C)          debug_printf( "%c", C )
#define UIX(I)          debug_printf( "0x%x", I )
#define UID(I)          debug_printf( "%u", I )
#define SID(I)          debug_printf( "%d", I )
#define FLT(F)          debug_printf( "%10.4f", F )
#define ENM(E,ENUMS)    dump_enum( E, ENUMS, sizeof( ENUMS ) / sizeof( *ENUMS ) )

static const char *TGSI_PROCESSOR_TYPES_SHORT[] =
{
   "FRAG",
   "VERT",
   "GEOM"
};

static const char *TGSI_FILES_SHORT[] =
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

static const char *TGSI_INTERPOLATES_SHORT[] =
{
   "CONSTANT",
   "LINEAR",
   "PERSPECTIVE"
};

static const char *TGSI_SEMANTICS_SHORT[] =
{
   "POSITION",
   "COLOR",
   "BCOLOR",
   "FOG",
   "PSIZE",
   "GENERIC",
   "NORMAL"
};

static const char *TGSI_IMMS_SHORT[] =
{
   "FLT32"
};

static const char *TGSI_OPCODES_SHORT[TGSI_OPCODE_LAST] =
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

static const char *TGSI_SWIZZLES_SHORT[] =
{
   "x",
   "y",
   "z",
   "w"
};

static const char *TGSI_TEXTURES_SHORT[] =
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

static const char *TGSI_EXTSWIZZLES_SHORT[] =
{
   "x",
   "y",
   "z",
   "w",
   "0",
   "1"
};

static const char *TGSI_MODULATES_SHORT[TGSI_MODULATE_COUNT] =
{
   "",
   "_2X",
   "_4X",
   "_8X",
   "_D2",
   "_D4",
   "_D8"
};

void
tgsi_dump_declaration(
   const struct tgsi_full_declaration *decl )
{
   TXT( "\nDCL " );
   ENM( decl->Declaration.File, TGSI_FILES_SHORT );

   CHR( '[' );
   UID( decl->DeclarationRange.First );
   if (decl->DeclarationRange.First != decl->DeclarationRange.Last) {
      TXT( ".." );
      UID( decl->DeclarationRange.Last );
   }
   CHR( ']' );

   if( decl->Declaration.UsageMask != TGSI_WRITEMASK_XYZW ) {
      CHR( '.' );
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_X ) {
         CHR( 'x' );
      }
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_Y ) {
         CHR( 'y' );
      }
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_Z ) {
         CHR( 'z' );
      }
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_W ) {
         CHR( 'w' );
      }
   }

   if (decl->Declaration.Semantic) {
      TXT( ", " );
      ENM( decl->Semantic.SemanticName, TGSI_SEMANTICS_SHORT );
      if (decl->Semantic.SemanticIndex != 0 ||
          decl->Semantic.SemanticName == TGSI_SEMANTIC_GENERIC) {
         CHR( '[' );
         UID( decl->Semantic.SemanticIndex );
         CHR( ']' );
      }
   }

   TXT( ", " );
   ENM( decl->Declaration.Interpolate, TGSI_INTERPOLATES_SHORT );
}

void
tgsi_dump_immediate(
   const struct tgsi_full_immediate *imm )
{
   unsigned i;

   TXT( "\nIMM " );
   ENM( imm->Immediate.DataType, TGSI_IMMS_SHORT );

   TXT( " { " );
   for( i = 0; i < imm->Immediate.Size - 1; i++ ) {
      switch( imm->Immediate.DataType ) {
      case TGSI_IMM_FLOAT32:
         FLT( imm->u.ImmediateFloat32[i].Float );
         break;

      default:
         assert( 0 );
      }

      if( i < imm->Immediate.Size - 2 ) {
         TXT( ", " );
      }
   }
   TXT( " }" );
}

void
tgsi_dump_instruction(
   const struct tgsi_full_instruction *inst,
   uint instno )
{
   unsigned i;
   boolean  first_reg = TRUE;

   EOL();
   UID( instno );
   CHR( ':' );
   ENM( inst->Instruction.Opcode, TGSI_OPCODES_SHORT );

   switch( inst->Instruction.Saturate ) {
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

   for( i = 0; i < inst->Instruction.NumDstRegs; i++ ) {
      const struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];

      if( !first_reg ) {
         CHR( ',' );
      }
      CHR( ' ' );

      ENM( dst->DstRegister.File, TGSI_FILES_SHORT );

      CHR( '[' );
      SID( dst->DstRegister.Index );
      CHR( ']' );

      ENM( dst->DstRegisterExtModulate.Modulate, TGSI_MODULATES_SHORT );

      if( dst->DstRegister.WriteMask != TGSI_WRITEMASK_XYZW ) {
         CHR( '.' );
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_X ) {
            CHR( 'x' );
         }
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_Y ) {
            CHR( 'y' );
         }
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_Z ) {
            CHR( 'z' );
         }
         if( dst->DstRegister.WriteMask & TGSI_WRITEMASK_W ) {
            CHR( 'w' );
         }
      }

      first_reg = FALSE;
   }

   for( i = 0; i < inst->Instruction.NumSrcRegs; i++ ) {
      const struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];

      if( !first_reg ) {
         CHR( ',' );
      }
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

      ENM( src->SrcRegister.File, TGSI_FILES_SHORT );

      CHR( '[' );
      if (src->SrcRegister.Indirect) {
         TXT( "ADDR[0]" );
         if (src->SrcRegister.Index != 0) {
            if (src->SrcRegister.Index > 0)
               CHR( '+' );
            SID( src->SrcRegister.Index );
         }
      }
      else
         SID( src->SrcRegister.Index );
      CHR( ']' );

      if (src->SrcRegister.SwizzleX != TGSI_SWIZZLE_X ||
          src->SrcRegister.SwizzleY != TGSI_SWIZZLE_Y ||
          src->SrcRegister.SwizzleZ != TGSI_SWIZZLE_Z ||
          src->SrcRegister.SwizzleW != TGSI_SWIZZLE_W) {
         CHR( '.' );
         ENM( src->SrcRegister.SwizzleX, TGSI_SWIZZLES_SHORT );
         ENM( src->SrcRegister.SwizzleY, TGSI_SWIZZLES_SHORT );
         ENM( src->SrcRegister.SwizzleZ, TGSI_SWIZZLES_SHORT );
         ENM( src->SrcRegister.SwizzleW, TGSI_SWIZZLES_SHORT );
      }
      if (src->SrcRegisterExtSwz.ExtSwizzleX != TGSI_EXTSWIZZLE_X ||
          src->SrcRegisterExtSwz.ExtSwizzleY != TGSI_EXTSWIZZLE_Y ||
          src->SrcRegisterExtSwz.ExtSwizzleZ != TGSI_EXTSWIZZLE_Z ||
          src->SrcRegisterExtSwz.ExtSwizzleW != TGSI_EXTSWIZZLE_W) {
         CHR( '.' );
         ENM( src->SrcRegisterExtSwz.ExtSwizzleX, TGSI_EXTSWIZZLES_SHORT );
         ENM( src->SrcRegisterExtSwz.ExtSwizzleY, TGSI_EXTSWIZZLES_SHORT );
         ENM( src->SrcRegisterExtSwz.ExtSwizzleZ, TGSI_EXTSWIZZLES_SHORT );
         ENM( src->SrcRegisterExtSwz.ExtSwizzleW, TGSI_EXTSWIZZLES_SHORT );
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
      ENM( inst->InstructionExtTexture.Texture, TGSI_TEXTURES_SHORT );
   }

   switch( inst->Instruction.Opcode ) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_ELSE:
   case TGSI_OPCODE_BGNLOOP2:
   case TGSI_OPCODE_ENDLOOP2:
   case TGSI_OPCODE_CAL:
      TXT( " :" );
      UID( inst->InstructionExtLabel.Label );
      break;
   }
}

void
tgsi_dump(
   const struct tgsi_token *tokens,
   uint flags )
{
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;
   unsigned instno = 0;

   /* sanity checks */
   assert(strcmp(TGSI_OPCODES_SHORT[TGSI_OPCODE_CONT], "OPCODE_CONT") == 0);
   assert(strcmp(TGSI_OPCODES_SHORT[TGSI_OPCODE_END], "OPCODE_END") == 0);
   assert(strcmp(TGSI_OPCODES_SHORT[TGSI_OPCODE_END], "END") == 0);

   tgsi_parse_init( &parse, tokens );

   EOL();
   ENM( parse.FullHeader.Processor.Processor, TGSI_PROCESSOR_TYPES_SHORT );
   UID( parse.FullVersion.Version.MajorVersion );
   CHR( '.' );
   UID( parse.FullVersion.Version.MinorVersion );

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         tgsi_dump_declaration(
            &parse.FullToken.FullDeclaration );
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         tgsi_dump_immediate(
            &parse.FullToken.FullImmediate );
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         tgsi_dump_instruction(
            &parse.FullToken.FullInstruction,
            instno );
         instno++;
         break;

      default:
         assert( 0 );
      }
   }

   tgsi_parse_free( &parse );
}
