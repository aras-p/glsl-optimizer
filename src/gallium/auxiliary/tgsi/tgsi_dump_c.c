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
#include "tgsi_dump_c.h"
#include "tgsi_build.h"
#include "tgsi_info.h"
#include "tgsi_parse.h"

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

static const char *TGSI_INTERPOLATES[] =
{
   "INTERPOLATE_CONSTANT",
   "INTERPOLATE_LINEAR",
   "INTERPOLATE_PERSPECTIVE"
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

static const char *TGSI_IMMS[] =
{
   "IMM_FLOAT32"
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
   "PRECISION_FLOAT32",
   "PRECISION_FLOAT16",
   "PRECISION_FIXED12"
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

static void
dump_instruction_verbose(
   struct tgsi_full_instruction  *inst,
   unsigned                      ignored,
   unsigned                      deflt,
   struct tgsi_full_instruction  *fi )
{
   unsigned i;

   TXT( "\nOpcode     : OPCODE_" );
   TXT( tgsi_get_opcode_info( inst->Instruction.Opcode )->mnemonic );
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
tgsi_dump_c(
   const struct tgsi_token *tokens,
   uint flags )
{
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;
   uint ignored = flags & TGSI_DUMP_C_IGNORED;
   uint deflt = flags & TGSI_DUMP_C_DEFAULT;
   uint instno = 0;

   tgsi_parse_init( &parse, tokens );

   TXT( "tgsi-dump begin -----------------" );

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

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

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

   TXT( "\ntgsi-dump end -------------------\n" );

   tgsi_parse_free( &parse );
}
