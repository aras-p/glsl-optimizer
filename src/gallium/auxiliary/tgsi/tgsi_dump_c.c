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

#include "util/u_debug.h"
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

static const char *TGSI_FILES[TGSI_FILE_COUNT] =
{
   "FILE_NULL",
   "FILE_CONSTANT",
   "FILE_INPUT",
   "FILE_OUTPUT",
   "FILE_TEMPORARY",
   "FILE_SAMPLER",
   "FILE_ADDRESS",
   "FILE_IMMEDIATE",
   "FILE_LOOP",
   "FILE_PREDICATE"
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
   if (deflt || fd->Declaration.Centroid != decl->Declaration.Centroid) {
      TXT("\nCentroid   : ");
      UID(decl->Declaration.Centroid);
   }
   if (deflt || fd->Declaration.Invariant != decl->Declaration.Invariant) {
      TXT("\nInvariant  : ");
      UID(decl->Declaration.Invariant);
   }
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( decl->Declaration.Padding );
   }

   EOL();
   TXT( "\nFirst: " );
   UID( decl->Range.First );
   TXT( "\nLast : " );
   UID( decl->Range.Last );

   if( decl->Declaration.Semantic ) {
      EOL();
      TXT( "\nName : " );
      ENM( decl->Semantic.Name, TGSI_SEMANTICS );
      TXT( "\nIndex: " );
      UID( decl->Semantic.Index );
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

   assert( imm->Immediate.NrTokens <= 4 + 1 );
   for( i = 0; i < imm->Immediate.NrTokens - 1; i++ ) {
      EOL();
      switch( imm->Immediate.DataType ) {
      case TGSI_IMM_FLOAT32:
         TXT( "\nFloat: " );
         FLT( imm->u[i].Float );
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
   if (deflt || fi->Instruction.Predicate != inst->Instruction.Predicate) {
      TXT("\nPredicate  : ");
      UID(inst->Instruction.Predicate);
   }
   if (deflt || fi->Instruction.Label != inst->Instruction.Label) {
      TXT("\nLabel      : ");
      UID(inst->Instruction.Label);
   }
   if (deflt || fi->Instruction.Texture != inst->Instruction.Texture) {
      TXT("\nTexture    : ");
      UID(inst->Instruction.Texture);
   }
   if( ignored ) {
      TXT( "\nPadding    : " );
      UIX( inst->Instruction.Padding );
   }

   if (deflt || inst->Instruction.Label) {
      EOL();
      if (deflt || fi->Label.Label != inst->Label.Label) {
         TXT( "\nLabel   : " );
         UID(inst->Label.Label);
      }
      if( ignored ) {
         TXT( "\nPadding : " );
         UIX(inst->Label.Padding);
      }
   }

   if (deflt || inst->Instruction.Texture) {
      EOL();
      if (deflt || fi->Texture.Texture != inst->Texture.Texture) {
         TXT( "\nTexture : " );
         ENM(inst->Texture.Texture, TGSI_TEXTURES);
      }
      if( ignored ) {
         TXT( "\nPadding : " );
         UIX(inst->Texture.Padding);
      }
   }

   for( i = 0; i < inst->Instruction.NumDstRegs; i++ ) {
      struct tgsi_full_dst_register *dst = &inst->Dst[i];
      struct tgsi_full_dst_register *fd = &fi->Dst[i];

      EOL();
      TXT( "\nFile     : " );
      ENM( dst->Register.File, TGSI_FILES );
      if( deflt || fd->Register.WriteMask != dst->Register.WriteMask ) {
         TXT( "\nWriteMask: " );
         ENM( dst->Register.WriteMask, TGSI_WRITEMASKS );
      }
      if( ignored ) {
         if( deflt || fd->Register.Indirect != dst->Register.Indirect ) {
            TXT( "\nIndirect : " );
            UID( dst->Register.Indirect );
         }
         if( deflt || fd->Register.Dimension != dst->Register.Dimension ) {
            TXT( "\nDimension: " );
            UID( dst->Register.Dimension );
         }
      }
      if( deflt || fd->Register.Index != dst->Register.Index ) {
         TXT( "\nIndex    : " );
         SID( dst->Register.Index );
      }
      if( ignored ) {
         TXT( "\nPadding  : " );
         UIX( dst->Register.Padding );
      }
   }

   for( i = 0; i < inst->Instruction.NumSrcRegs; i++ ) {
      struct tgsi_full_src_register *src = &inst->Src[i];
      struct tgsi_full_src_register *fs = &fi->Src[i];

      EOL();
      TXT( "\nFile     : ");
      ENM( src->Register.File, TGSI_FILES );
      if( deflt || fs->Register.SwizzleX != src->Register.SwizzleX ) {
         TXT( "\nSwizzleX : " );
         ENM( src->Register.SwizzleX, TGSI_SWIZZLES );
      }
      if( deflt || fs->Register.SwizzleY != src->Register.SwizzleY ) {
         TXT( "\nSwizzleY : " );
         ENM( src->Register.SwizzleY, TGSI_SWIZZLES );
      }
      if( deflt || fs->Register.SwizzleZ != src->Register.SwizzleZ ) {
         TXT( "\nSwizzleZ : " );
         ENM( src->Register.SwizzleZ, TGSI_SWIZZLES );
      }
      if( deflt || fs->Register.SwizzleW != src->Register.SwizzleW ) {
         TXT( "\nSwizzleW : " );
         ENM( src->Register.SwizzleW, TGSI_SWIZZLES );
      }
      if (deflt || fs->Register.Absolute != src->Register.Absolute) {
         TXT("\nAbsolute : ");
         UID(src->Register.Absolute);
      }
      if( deflt || fs->Register.Negate != src->Register.Negate ) {
         TXT( "\nNegate   : " );
         UID( src->Register.Negate );
      }
      if( ignored ) {
         if( deflt || fs->Register.Indirect != src->Register.Indirect ) {
            TXT( "\nIndirect : " );
            UID( src->Register.Indirect );
         }
         if( deflt || fs->Register.Dimension != src->Register.Dimension ) {
            TXT( "\nDimension: " );
            UID( src->Register.Dimension );
         }
      }
      if( deflt || fs->Register.Index != src->Register.Index ) {
         TXT( "\nIndex    : " );
         SID( src->Register.Index );
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

   tgsi_parse_init( &parse, tokens );

   TXT( "tgsi-dump begin -----------------" );

   TXT( "\nMajor: " );
   UID( parse.FullVersion.Version.Major );
   TXT( "\nMinor: " );
   UID( parse.FullVersion.Version.Minor );
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
         UID( parse.FullToken.Token.NrTokens );
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
