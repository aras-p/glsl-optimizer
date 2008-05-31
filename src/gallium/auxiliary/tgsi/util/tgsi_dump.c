/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include "pipe/p_shader_tokens.h"
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

static const char *TGSI_PROCESSOR_TYPES[] =
{
   "PROCESSOR_FRAGMENT",
   "PROCESSOR_VERTEX",
   "PROCESSOR_GEOMETRY"
};

static const char *TGSI_PROCESSOR_TYPES_SHORT[] =
{
   "FRAG",
   "VERT",
   "GEOM"
};

static const char *TGSI_TOKEN_TYPES[] =
{
   "TOKEN_TYPE_DECLARATION",
   "TOKEN_TYPE_IMMEDIATE",
   "TOKEN_TYPE_INSTRUCTION"
};

static const char *TGSI_FILES[] =
{
   "FILE_NULL",
   "FILE_CONSTANT",
   "FILE_INPUT",
   "FILE_OUTPUT",
   "FILE_TEMPORARY",
   "FILE_SAMPLER",
   "FILE_ADDRESS",
   "FILE_IMMEDIATE"
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

static const char *TGSI_DECLARES[] =
{
   "DECLARE_RANGE",
   "DECLARE_MASK"
};

static const char *TGSI_INTERPOLATES[] =
{
   "INTERPOLATE_CONSTANT",
   "INTERPOLATE_LINEAR",
   "INTERPOLATE_PERSPECTIVE",
   "INTERPOLATE_ATTRIB"
};

static const char *TGSI_INTERPOLATES_SHORT[] =
{
   "CONSTANT",
   "LINEAR",
   "PERSPECTIVE",
   "ATTRIB"
};

static const char *TGSI_SEMANTICS[] =
{
   "SEMANTIC_POSITION",
   "SEMANTIC_COLOR",
   "SEMANTIC_BCOLOR",
   "SEMANTIC_FOG",
   "SEMANTIC_PSIZE",
   "SEMANTIC_GENERIC",
   "SEMANTIC_NORMAL"
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

static const char *TGSI_IMMS[] =
{
   "IMM_FLOAT32"
};

static const char *TGSI_IMMS_SHORT[] =
{
   "FLT32"
};

static const char *TGSI_OPCODES[TGSI_OPCODE_LAST] =
{
   "OPCODE_ARL",
   "OPCODE_MOV",
   "OPCODE_LIT",
   "OPCODE_RCP",
   "OPCODE_RSQ",
   "OPCODE_EXP",
   "OPCODE_LOG",
   "OPCODE_MUL",
   "OPCODE_ADD",
   "OPCODE_DP3",
   "OPCODE_DP4",
   "OPCODE_DST",
   "OPCODE_MIN",
   "OPCODE_MAX",
   "OPCODE_SLT",
   "OPCODE_SGE",
   "OPCODE_MAD",
   "OPCODE_SUB",
   "OPCODE_LERP",
   "OPCODE_CND",
   "OPCODE_CND0",
   "OPCODE_DOT2ADD",
   "OPCODE_INDEX",
   "OPCODE_NEGATE",
   "OPCODE_FRAC",
   "OPCODE_CLAMP",
   "OPCODE_FLOOR",
   "OPCODE_ROUND",
   "OPCODE_EXPBASE2",
   "OPCODE_LOGBASE2",
   "OPCODE_POWER",
   "OPCODE_CROSSPRODUCT",
   "OPCODE_MULTIPLYMATRIX",
   "OPCODE_ABS",
   "OPCODE_RCC",
   "OPCODE_DPH",
   "OPCODE_COS",
   "OPCODE_DDX",
   "OPCODE_DDY",
   "OPCODE_KILP",
   "OPCODE_PK2H",
   "OPCODE_PK2US",
   "OPCODE_PK4B",
   "OPCODE_PK4UB",
   "OPCODE_RFL",
   "OPCODE_SEQ",
   "OPCODE_SFL",
   "OPCODE_SGT",
   "OPCODE_SIN",
   "OPCODE_SLE",
   "OPCODE_SNE",
   "OPCODE_STR",
   "OPCODE_TEX",
   "OPCODE_TXD",
   "OPCODE_TXP",
   "OPCODE_UP2H",
   "OPCODE_UP2US",
   "OPCODE_UP4B",
   "OPCODE_UP4UB",
   "OPCODE_X2D",
   "OPCODE_ARA",
   "OPCODE_ARR",
   "OPCODE_BRA",
   "OPCODE_CAL",
   "OPCODE_RET",
   "OPCODE_SSG",
   "OPCODE_CMP",
   "OPCODE_SCS",
   "OPCODE_TXB",
   "OPCODE_NRM",
   "OPCODE_DIV",
   "OPCODE_DP2",
   "OPCODE_TXL",
   "OPCODE_BRK",
   "OPCODE_IF",
   "OPCODE_LOOP",
   "OPCODE_REP",
   "OPCODE_ELSE",
   "OPCODE_ENDIF",
   "OPCODE_ENDLOOP",
   "OPCODE_ENDREP",
   "OPCODE_PUSHA",
   "OPCODE_POPA",
   "OPCODE_CEIL",
   "OPCODE_I2F",
   "OPCODE_NOT",
   "OPCODE_TRUNC",
   "OPCODE_SHL",
   "OPCODE_SHR",
   "OPCODE_AND",
   "OPCODE_OR",
   "OPCODE_MOD",
   "OPCODE_XOR",
   "OPCODE_SAD",
   "OPCODE_TXF",
   "OPCODE_TXQ",
   "OPCODE_CONT",
   "OPCODE_EMIT",
   "OPCODE_ENDPRIM",
   "OPCODE_BGNLOOP2",
   "OPCODE_BGNSUB",
   "OPCODE_ENDLOOP2",
   "OPCODE_ENDSUB",
   "OPCODE_NOISE1",
   "OPCODE_NOISE2",
   "OPCODE_NOISE3",
   "OPCODE_NOISE4",
   "OPCODE_NOP",
   "OPCODE_M4X3",
   "OPCODE_M3X4",
   "OPCODE_M3X3",
   "OPCODE_M3X2",
   "OPCODE_NRM4",
   "OPCODE_CALLNZ",
   "OPCODE_IFC",
   "OPCODE_BREAKC",
   "OPCODE_KIL",
   "OPCODE_END"
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
   "END"
};

static const char *TGSI_SATS[] =
{
   "SAT_NONE",
   "SAT_ZERO_ONE",
   "SAT_MINUS_PLUS_ONE"
};

static const char *TGSI_INSTRUCTION_EXTS[] =
{
   "INSTRUCTION_EXT_TYPE_NV",
   "INSTRUCTION_EXT_TYPE_LABEL",
   "INSTRUCTION_EXT_TYPE_TEXTURE"
};

static const char *TGSI_PRECISIONS[] =
{
   "PRECISION_DEFAULT",
   "TGSI_PRECISION_FLOAT32",
   "TGSI_PRECISION_FLOAT16",
   "TGSI_PRECISION_FIXED12"
};

static const char *TGSI_CCS[] =
{
   "CC_GT",
   "CC_EQ",
   "CC_LT",
   "CC_UN",
   "CC_GE",
   "CC_LE",
   "CC_NE",
   "CC_TR",
   "CC_FL"
};

static const char *TGSI_SWIZZLES[] =
{
   "SWIZZLE_X",
   "SWIZZLE_Y",
   "SWIZZLE_Z",
   "SWIZZLE_W"
};

static const char *TGSI_SWIZZLES_SHORT[] =
{
   "x",
   "y",
   "z",
   "w"
};

static const char *TGSI_TEXTURES[] =
{
   "TEXTURE_UNKNOWN",
   "TEXTURE_1D",
   "TEXTURE_2D",
   "TEXTURE_3D",
   "TEXTURE_CUBE",
   "TEXTURE_RECT",
   "TEXTURE_SHADOW1D",
   "TEXTURE_SHADOW2D",
   "TEXTURE_SHADOWRECT"
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

static const char *TGSI_SRC_REGISTER_EXTS[] =
{
   "SRC_REGISTER_EXT_TYPE_SWZ",
   "SRC_REGISTER_EXT_TYPE_MOD"
};

static const char *TGSI_EXTSWIZZLES[] =
{
   "EXTSWIZZLE_X",
   "EXTSWIZZLE_Y",
   "EXTSWIZZLE_Z",
   "EXTSWIZZLE_W",
   "EXTSWIZZLE_ZERO",
   "EXTSWIZZLE_ONE"
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

static const char *TGSI_WRITEMASKS[] =
{
   "0",
   "WRITEMASK_X",
   "WRITEMASK_Y",
   "WRITEMASK_XY",
   "WRITEMASK_Z",
   "WRITEMASK_XZ",
   "WRITEMASK_YZ",
   "WRITEMASK_XYZ",
   "WRITEMASK_W",
   "WRITEMASK_XW",
   "WRITEMASK_YW",
   "WRITEMASK_XYW",
   "WRITEMASK_ZW",
   "WRITEMASK_XZW",
   "WRITEMASK_YZW",
   "WRITEMASK_XYZW"
};

static const char *TGSI_DST_REGISTER_EXTS[] =
{
   "DST_REGISTER_EXT_TYPE_CONDCODE",
   "DST_REGISTER_EXT_TYPE_MODULATE"
};

static const char *TGSI_MODULATES[] =
{
   "MODULATE_1X",
   "MODULATE_2X",
   "MODULATE_4X",
   "MODULATE_8X",
   "MODULATE_HALF",
   "MODULATE_QUARTER",
   "MODULATE_EIGHTH"
};

void
tgsi_dump_declaration(
   const struct tgsi_full_declaration  *decl )
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
      if (decl->Semantic.SemanticIndex != 0) {
         CHR( '[' );
         UID( decl->Semantic.SemanticIndex );
         CHR( ']' );
      }
   }

   TXT( ", " );
   ENM( decl->Declaration.Interpolate, TGSI_INTERPOLATES_SHORT );
}

static void
dump_declaration_verbose(
   struct tgsi_full_declaration  *decl,
   unsigned                      ignored,
   unsigned                      deflt,
   struct tgsi_full_declaration  *fd )
{
   TXT( "\nFile       : " );
   ENM( decl->Declaration.File, TGSI_FILES );
   if( deflt || fd->Declaration.UsageMask != decl->Declaration.UsageMask ) {
      TXT( "\nUsageMask  : " );
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_X ) {
         CHR( 'X' );
      }
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_Y ) {
         CHR( 'Y' );
      }
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_Z ) {
         CHR( 'Z' );
      }
      if( decl->Declaration.UsageMask & TGSI_WRITEMASK_W ) {
         CHR( 'W' );
      }
   }
   if( deflt || fd->Declaration.Interpolate != decl->Declaration.Interpolate ) {
      TXT( "\nInterpolate: " );
      ENM( decl->Declaration.Interpolate, TGSI_INTERPOLATES );
   }
   if( deflt || fd->Declaration.Semantic != decl->Declaration.Semantic ) {
      TXT( "\nSemantic   : " );
      UID( decl->Declaration.Semantic );
   }
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( decl->Declaration.Padding );
   }

   EOL();
   TXT( "\nFirst: " );
   UID( decl->DeclarationRange.First );
   TXT( "\nLast : " );
   UID( decl->DeclarationRange.Last );

   if( decl->Declaration.Semantic ) {
      EOL();
      TXT( "\nSemanticName : " );
      ENM( decl->Semantic.SemanticName, TGSI_SEMANTICS );
      TXT( "\nSemanticIndex: " );
      UID( decl->Semantic.SemanticIndex );
      if( ignored ) {
         TXT( "\nPadding      : " );
         UIX( decl->Semantic.Padding );
      }
   }
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

static void
dump_immediate_verbose(
   struct tgsi_full_immediate *imm,
   unsigned                   ignored )
{
   unsigned i;

   TXT( "\nDataType   : " );
   ENM( imm->Immediate.DataType, TGSI_IMMS );
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( imm->Immediate.Padding );
   }

   for( i = 0; i < imm->Immediate.Size - 1; i++ ) {
      EOL();
      switch( imm->Immediate.DataType ) {
      case TGSI_IMM_FLOAT32:
         TXT( "\nFloat: " );
         FLT( imm->u.ImmediateFloat32[i].Float );
         break;

      default:
         assert( 0 );
      }
   }
}

void
tgsi_dump_instruction(
   const struct tgsi_full_instruction  *inst,
   unsigned                      instno )
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
      TXT( "_SAT[-1,1]" );
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

      switch (dst->DstRegisterExtModulate.Modulate) {
      case TGSI_MODULATE_1X:
         break;
      case TGSI_MODULATE_2X:
         TXT( "_2X" );
         break;
      case TGSI_MODULATE_4X:
         TXT( "_4X" );
         break;
      case TGSI_MODULATE_8X:
         TXT( "_8X" );
         break;
      case TGSI_MODULATE_HALF:
         TXT( "_D2" );
         break;
      case TGSI_MODULATE_QUARTER:
         TXT( "_D4" );
         break;
      case TGSI_MODULATE_EIGHTH:
         TXT( "_D8" );
         break;
      default:
         assert( 0 );
      }

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

static void
dump_instruction_verbose(
   struct tgsi_full_instruction  *inst,
   unsigned                      ignored,
   unsigned                      deflt,
   struct tgsi_full_instruction  *fi )
{
   unsigned i;

   TXT( "\nOpcode     : " );
   ENM( inst->Instruction.Opcode, TGSI_OPCODES );
   if( deflt || fi->Instruction.Saturate != inst->Instruction.Saturate ) {
      TXT( "\nSaturate   : " );
      ENM( inst->Instruction.Saturate, TGSI_SATS );
   }
   if( deflt || fi->Instruction.NumDstRegs != inst->Instruction.NumDstRegs ) {
      TXT( "\nNumDstRegs : " );
      UID( inst->Instruction.NumDstRegs );
   }
   if( deflt || fi->Instruction.NumSrcRegs != inst->Instruction.NumSrcRegs ) {
      TXT( "\nNumSrcRegs : " );
      UID( inst->Instruction.NumSrcRegs );
   }
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( inst->Instruction.Padding );
   }

   if( deflt || tgsi_compare_instruction_ext_nv( inst->InstructionExtNv, fi->InstructionExtNv ) ) {
      EOL();
      TXT( "\nType          : " );
      ENM( inst->InstructionExtNv.Type, TGSI_INSTRUCTION_EXTS );
      if( deflt || fi->InstructionExtNv.Precision != inst->InstructionExtNv.Precision ) {
         TXT( "\nPrecision     : " );
         ENM( inst->InstructionExtNv.Precision, TGSI_PRECISIONS );
      }
      if( deflt || fi->InstructionExtNv.CondDstIndex != inst->InstructionExtNv.CondDstIndex ) {
         TXT( "\nCondDstIndex  : " );
         UID( inst->InstructionExtNv.CondDstIndex );
      }
      if( deflt || fi->InstructionExtNv.CondFlowIndex != inst->InstructionExtNv.CondFlowIndex ) {
         TXT( "\nCondFlowIndex : " );
         UID( inst->InstructionExtNv.CondFlowIndex );
      }
      if( deflt || fi->InstructionExtNv.CondMask != inst->InstructionExtNv.CondMask ) {
         TXT( "\nCondMask      : " );
         ENM( inst->InstructionExtNv.CondMask, TGSI_CCS );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleX != inst->InstructionExtNv.CondSwizzleX ) {
         TXT( "\nCondSwizzleX  : " );
         ENM( inst->InstructionExtNv.CondSwizzleX, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleY != inst->InstructionExtNv.CondSwizzleY ) {
         TXT( "\nCondSwizzleY  : " );
         ENM( inst->InstructionExtNv.CondSwizzleY, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleZ != inst->InstructionExtNv.CondSwizzleZ ) {
         TXT( "\nCondSwizzleZ  : " );
         ENM( inst->InstructionExtNv.CondSwizzleZ, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondSwizzleW != inst->InstructionExtNv.CondSwizzleW ) {
         TXT( "\nCondSwizzleW  : " );
         ENM( inst->InstructionExtNv.CondSwizzleW, TGSI_SWIZZLES );
      }
      if( deflt || fi->InstructionExtNv.CondDstUpdate != inst->InstructionExtNv.CondDstUpdate ) {
         TXT( "\nCondDstUpdate : " );
         UID( inst->InstructionExtNv.CondDstUpdate );
      }
      if( deflt || fi->InstructionExtNv.CondFlowEnable != inst->InstructionExtNv.CondFlowEnable ) {
         TXT( "\nCondFlowEnable: " );
         UID( inst->InstructionExtNv.CondFlowEnable );
      }
      if( ignored ) {
         TXT( "\nPadding       : " );
         UIX( inst->InstructionExtNv.Padding );
         if( deflt || fi->InstructionExtNv.Extended != inst->InstructionExtNv.Extended ) {
            TXT( "\nExtended      : " );
            UID( inst->InstructionExtNv.Extended );
         }
      }
   }

   if( deflt || tgsi_compare_instruction_ext_label( inst->InstructionExtLabel, fi->InstructionExtLabel ) ) {
      EOL();
      TXT( "\nType    : " );
      ENM( inst->InstructionExtLabel.Type, TGSI_INSTRUCTION_EXTS );
      if( deflt || fi->InstructionExtLabel.Label != inst->InstructionExtLabel.Label ) {
         TXT( "\nLabel   : " );
         UID( inst->InstructionExtLabel.Label );
      }
      if( ignored ) {
         TXT( "\nPadding : " );
         UIX( inst->InstructionExtLabel.Padding );
         if( deflt || fi->InstructionExtLabel.Extended != inst->InstructionExtLabel.Extended ) {
            TXT( "\nExtended: " );
            UID( inst->InstructionExtLabel.Extended );
         }
      }
   }

   if( deflt || tgsi_compare_instruction_ext_texture( inst->InstructionExtTexture, fi->InstructionExtTexture ) ) {
      EOL();
      TXT( "\nType    : " );
      ENM( inst->InstructionExtTexture.Type, TGSI_INSTRUCTION_EXTS );
      if( deflt || fi->InstructionExtTexture.Texture != inst->InstructionExtTexture.Texture ) {
         TXT( "\nTexture : " );
         ENM( inst->InstructionExtTexture.Texture, TGSI_TEXTURES );
      }
      if( ignored ) {
         TXT( "\nPadding : " );
         UIX( inst->InstructionExtTexture.Padding );
         if( deflt || fi->InstructionExtTexture.Extended != inst->InstructionExtTexture.Extended ) {
            TXT( "\nExtended: " );
            UID( inst->InstructionExtTexture.Extended );
         }
      }
   }

   for( i = 0; i < inst->Instruction.NumDstRegs; i++ ) {
      struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];
      struct tgsi_full_dst_register *fd = &fi->FullDstRegisters[i];

      EOL();
      TXT( "\nFile     : " );
      ENM( dst->DstRegister.File, TGSI_FILES );
      if( deflt || fd->DstRegister.WriteMask != dst->DstRegister.WriteMask ) {
         TXT( "\nWriteMask: " );
         ENM( dst->DstRegister.WriteMask, TGSI_WRITEMASKS );
      }
      if( ignored ) {
         if( deflt || fd->DstRegister.Indirect != dst->DstRegister.Indirect ) {
            TXT( "\nIndirect : " );
            UID( dst->DstRegister.Indirect );
         }
         if( deflt || fd->DstRegister.Dimension != dst->DstRegister.Dimension ) {
            TXT( "\nDimension: " );
            UID( dst->DstRegister.Dimension );
         }
      }
      if( deflt || fd->DstRegister.Index != dst->DstRegister.Index ) {
         TXT( "\nIndex    : " );
         SID( dst->DstRegister.Index );
      }
      if( ignored ) {
         TXT( "\nPadding  : " );
         UIX( dst->DstRegister.Padding );
         if( deflt || fd->DstRegister.Extended != dst->DstRegister.Extended ) {
            TXT( "\nExtended : " );
            UID( dst->DstRegister.Extended );
         }
      }

      if( deflt || tgsi_compare_dst_register_ext_concode( dst->DstRegisterExtConcode, fd->DstRegisterExtConcode ) ) {
         EOL();
         TXT( "\nType        : " );
         ENM( dst->DstRegisterExtConcode.Type, TGSI_DST_REGISTER_EXTS );
         if( deflt || fd->DstRegisterExtConcode.CondMask != dst->DstRegisterExtConcode.CondMask ) {
            TXT( "\nCondMask    : " );
            ENM( dst->DstRegisterExtConcode.CondMask, TGSI_CCS );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleX != dst->DstRegisterExtConcode.CondSwizzleX ) {
            TXT( "\nCondSwizzleX: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleX, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleY != dst->DstRegisterExtConcode.CondSwizzleY ) {
            TXT( "\nCondSwizzleY: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleY, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleZ != dst->DstRegisterExtConcode.CondSwizzleZ ) {
            TXT( "\nCondSwizzleZ: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleZ, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSwizzleW != dst->DstRegisterExtConcode.CondSwizzleW ) {
            TXT( "\nCondSwizzleW: " );
            ENM( dst->DstRegisterExtConcode.CondSwizzleW, TGSI_SWIZZLES );
         }
         if( deflt || fd->DstRegisterExtConcode.CondSrcIndex != dst->DstRegisterExtConcode.CondSrcIndex ) {
            TXT( "\nCondSrcIndex: " );
            UID( dst->DstRegisterExtConcode.CondSrcIndex );
         }
         if( ignored ) {
            TXT( "\nPadding     : " );
            UIX( dst->DstRegisterExtConcode.Padding );
            if( deflt || fd->DstRegisterExtConcode.Extended != dst->DstRegisterExtConcode.Extended ) {
               TXT( "\nExtended    : " );
               UID( dst->DstRegisterExtConcode.Extended );
            }
         }
      }

      if( deflt || tgsi_compare_dst_register_ext_modulate( dst->DstRegisterExtModulate, fd->DstRegisterExtModulate ) ) {
         EOL();
         TXT( "\nType    : " );
         ENM( dst->DstRegisterExtModulate.Type, TGSI_DST_REGISTER_EXTS );
         if( deflt || fd->DstRegisterExtModulate.Modulate != dst->DstRegisterExtModulate.Modulate ) {
            TXT( "\nModulate: " );
            ENM( dst->DstRegisterExtModulate.Modulate, TGSI_MODULATES );
         }
         if( ignored ) {
            TXT( "\nPadding : " );
            UIX( dst->DstRegisterExtModulate.Padding );
            if( deflt || fd->DstRegisterExtModulate.Extended != dst->DstRegisterExtModulate.Extended ) {
               TXT( "\nExtended: " );
               UID( dst->DstRegisterExtModulate.Extended );
            }
         }
      }
   }

   for( i = 0; i < inst->Instruction.NumSrcRegs; i++ ) {
      struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];
      struct tgsi_full_src_register *fs = &fi->FullSrcRegisters[i];

      EOL();
      TXT( "\nFile     : ");
      ENM( src->SrcRegister.File, TGSI_FILES );
      if( deflt || fs->SrcRegister.SwizzleX != src->SrcRegister.SwizzleX ) {
         TXT( "\nSwizzleX : " );
         ENM( src->SrcRegister.SwizzleX, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.SwizzleY != src->SrcRegister.SwizzleY ) {
         TXT( "\nSwizzleY : " );
         ENM( src->SrcRegister.SwizzleY, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.SwizzleZ != src->SrcRegister.SwizzleZ ) {
         TXT( "\nSwizzleZ : " );
         ENM( src->SrcRegister.SwizzleZ, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.SwizzleW != src->SrcRegister.SwizzleW ) {
         TXT( "\nSwizzleW : " );
         ENM( src->SrcRegister.SwizzleW, TGSI_SWIZZLES );
      }
      if( deflt || fs->SrcRegister.Negate != src->SrcRegister.Negate ) {
         TXT( "\nNegate   : " );
         UID( src->SrcRegister.Negate );
      }
      if( ignored ) {
         if( deflt || fs->SrcRegister.Indirect != src->SrcRegister.Indirect ) {
            TXT( "\nIndirect : " );
            UID( src->SrcRegister.Indirect );
         }
         if( deflt || fs->SrcRegister.Dimension != src->SrcRegister.Dimension ) {
            TXT( "\nDimension: " );
            UID( src->SrcRegister.Dimension );
         }
      }
      if( deflt || fs->SrcRegister.Index != src->SrcRegister.Index ) {
         TXT( "\nIndex    : " );
         SID( src->SrcRegister.Index );
      }
      if( ignored ) {
         if( deflt || fs->SrcRegister.Extended != src->SrcRegister.Extended ) {
            TXT( "\nExtended : " );
            UID( src->SrcRegister.Extended );
         }
      }

      if( deflt || tgsi_compare_src_register_ext_swz( src->SrcRegisterExtSwz, fs->SrcRegisterExtSwz ) ) {
         EOL();
         TXT( "\nType       : " );
         ENM( src->SrcRegisterExtSwz.Type, TGSI_SRC_REGISTER_EXTS );
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleX != src->SrcRegisterExtSwz.ExtSwizzleX ) {
            TXT( "\nExtSwizzleX: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleX, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleY != src->SrcRegisterExtSwz.ExtSwizzleY ) {
            TXT( "\nExtSwizzleY: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleY, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleZ != src->SrcRegisterExtSwz.ExtSwizzleZ ) {
            TXT( "\nExtSwizzleZ: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleZ, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.ExtSwizzleW != src->SrcRegisterExtSwz.ExtSwizzleW ) {
            TXT( "\nExtSwizzleW: " );
            ENM( src->SrcRegisterExtSwz.ExtSwizzleW, TGSI_EXTSWIZZLES );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateX != src->SrcRegisterExtSwz.NegateX ) {
            TXT( "\nNegateX   : " );
            UID( src->SrcRegisterExtSwz.NegateX );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateY != src->SrcRegisterExtSwz.NegateY ) {
            TXT( "\nNegateY   : " );
            UID( src->SrcRegisterExtSwz.NegateY );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateZ != src->SrcRegisterExtSwz.NegateZ ) {
            TXT( "\nNegateZ   : " );
            UID( src->SrcRegisterExtSwz.NegateZ );
         }
         if( deflt || fs->SrcRegisterExtSwz.NegateW != src->SrcRegisterExtSwz.NegateW ) {
            TXT( "\nNegateW   : " );
            UID( src->SrcRegisterExtSwz.NegateW );
         }
         if( ignored ) {
            TXT( "\nPadding   : " );
            UIX( src->SrcRegisterExtSwz.Padding );
            if( deflt || fs->SrcRegisterExtSwz.Extended != src->SrcRegisterExtSwz.Extended ) {
               TXT( "\nExtended   : " );
               UID( src->SrcRegisterExtSwz.Extended );
            }
         }
      }

      if( deflt || tgsi_compare_src_register_ext_mod( src->SrcRegisterExtMod, fs->SrcRegisterExtMod ) ) {
         EOL();
         TXT( "\nType     : " );
         ENM( src->SrcRegisterExtMod.Type, TGSI_SRC_REGISTER_EXTS );
         if( deflt || fs->SrcRegisterExtMod.Complement != src->SrcRegisterExtMod.Complement ) {
            TXT( "\nComplement: " );
            UID( src->SrcRegisterExtMod.Complement );
         }
         if( deflt || fs->SrcRegisterExtMod.Bias != src->SrcRegisterExtMod.Bias ) {
            TXT( "\nBias     : " );
            UID( src->SrcRegisterExtMod.Bias );
         }
         if( deflt || fs->SrcRegisterExtMod.Scale2X != src->SrcRegisterExtMod.Scale2X ) {
            TXT( "\nScale2X   : " );
            UID( src->SrcRegisterExtMod.Scale2X );
         }
         if( deflt || fs->SrcRegisterExtMod.Absolute != src->SrcRegisterExtMod.Absolute ) {
            TXT( "\nAbsolute  : " );
            UID( src->SrcRegisterExtMod.Absolute );
         }
         if( deflt || fs->SrcRegisterExtMod.Negate != src->SrcRegisterExtMod.Negate ) {
            TXT( "\nNegate   : " );
            UID( src->SrcRegisterExtMod.Negate );
         }
         if( ignored ) {
            TXT( "\nPadding   : " );
            UIX( src->SrcRegisterExtMod.Padding );
            if( deflt || fs->SrcRegisterExtMod.Extended != src->SrcRegisterExtMod.Extended ) {
               TXT( "\nExtended  : " );
               UID( src->SrcRegisterExtMod.Extended );
            }
         }
      }
   }
}

void
tgsi_dump(
   const struct tgsi_token *tokens,
   unsigned                flags )
{
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;
   unsigned verbose = flags & TGSI_DUMP_VERBOSE;
   unsigned ignored = !(flags & TGSI_DUMP_NO_IGNORED);
   unsigned deflt = !(flags & TGSI_DUMP_NO_DEFAULT);
   unsigned instno = 0;

   /* sanity checks */
   assert(strcmp(TGSI_OPCODES[TGSI_OPCODE_CONT], "OPCODE_CONT") == 0);
   assert(strcmp(TGSI_OPCODES[TGSI_OPCODE_END], "OPCODE_END") == 0);
   assert(strcmp(TGSI_OPCODES_SHORT[TGSI_OPCODE_END], "END") == 0);

   tgsi_parse_init( &parse, tokens );

   TXT( "tgsi-dump begin -----------------" );

   EOL();
   ENM( parse.FullHeader.Processor.Processor, TGSI_PROCESSOR_TYPES_SHORT );
   UID( parse.FullVersion.Version.MajorVersion );
   CHR( '.' );
   UID( parse.FullVersion.Version.MinorVersion );

   if( verbose ) {
      TXT( "\nMajorVersion: " );
      UID( parse.FullVersion.Version.MajorVersion );
      TXT( "\nMinorVersion: " );
      UID( parse.FullVersion.Version.MinorVersion );
      EOL();

      TXT( "\nHeaderSize: " );
      UID( parse.FullHeader.Header.HeaderSize );
      TXT( "\nBodySize  : " );
      UID( parse.FullHeader.Header.BodySize );
      TXT( "\nProcessor : " );
      ENM( parse.FullHeader.Processor.Processor, TGSI_PROCESSOR_TYPES );
      EOL();
   }

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

      if( verbose ) {
         TXT( "\nType       : " );
         ENM( parse.FullToken.Token.Type, TGSI_TOKEN_TYPES );
         if( ignored ) {
            TXT( "\nSize       : " );
            UID( parse.FullToken.Token.Size );
            if( deflt || parse.FullToken.Token.Extended ) {
               TXT( "\nExtended   : " );
               UID( parse.FullToken.Token.Extended );
            }
         }

         switch( parse.FullToken.Token.Type ) {
         case TGSI_TOKEN_TYPE_DECLARATION:
            dump_declaration_verbose(
               &parse.FullToken.FullDeclaration,
               ignored,
               deflt,
               &fd );
            break;

         case TGSI_TOKEN_TYPE_IMMEDIATE:
            dump_immediate_verbose(
               &parse.FullToken.FullImmediate,
               ignored );
            break;

         case TGSI_TOKEN_TYPE_INSTRUCTION:
            dump_instruction_verbose(
               &parse.FullToken.FullInstruction,
               ignored,
               deflt,
               &fi );
            break;

         default:
            assert( 0 );
         }

         EOL();
      }
   }

   TXT( "\ntgsi-dump end -------------------\n" );

   tgsi_parse_free( &parse );
}
