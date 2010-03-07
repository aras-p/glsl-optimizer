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
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi_dump.h"
#include "tgsi_info.h"
#include "tgsi_iterate.h"


/** Number of spaces to indent for IF/LOOP/etc */
static const int indent_spaces = 3;


struct dump_ctx
{
   struct tgsi_iterate_context iter;

   uint instno;
   int indent;
   
   uint indentation;

   void (*printf)(struct dump_ctx *ctx, const char *format, ...);
};

static void 
dump_ctx_printf(struct dump_ctx *ctx, const char *format, ...)
{
   va_list ap;
   (void)ctx;
   va_start(ap, format);
   debug_vprintf(format, ap);
   va_end(ap);
}

static void
dump_enum(
   struct dump_ctx *ctx,
   uint e,
   const char **enums,
   uint enum_count )
{
   if (e >= enum_count)
      ctx->printf( ctx, "%u", e );
   else
      ctx->printf( ctx, "%s", enums[e] );
}

#define EOL()           ctx->printf( ctx, "\n" )
#define TXT(S)          ctx->printf( ctx, "%s", S )
#define CHR(C)          ctx->printf( ctx, "%c", C )
#define UIX(I)          ctx->printf( ctx, "0x%x", I )
#define UID(I)          ctx->printf( ctx, "%u", I )
#define INSTID(I)          ctx->printf( ctx, "% 3u", I )
#define SID(I)          ctx->printf( ctx, "%d", I )
#define FLT(F)          ctx->printf( ctx, "%10.4f", F )
#define ENM(E,ENUMS)    dump_enum( ctx, E, ENUMS, sizeof( ENUMS ) / sizeof( *ENUMS ) )

static const char *processor_type_names[] =
{
   "FRAG",
   "VERT",
   "GEOM"
};

static const char *file_names[TGSI_FILE_COUNT] =
{
   "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMP",
   "ADDR",
   "IMM",
   "LOOP",
   "PRED",
   "SV"
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
   "NORMAL",
   "FACE",
   "EDGEFLAG",
   "PRIM_ID",
   "INSTANCEID"
};

static const char *immediate_type_names[] =
{
   "FLT32",
   "UINT32",
   "INT32"
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

static const char *property_names[] =
{
   "GS_INPUT_PRIMITIVE",
   "GS_OUTPUT_PRIMITIVE",
   "GS_MAX_OUTPUT_VERTICES",
   "FS_COORD_ORIGIN",
   "FS_COORD_PIXEL_CENTER"
};

static const char *primitive_names[] =
{
   "POINTS",
   "LINES",
   "LINE_LOOP",
   "LINE_STRIP",
   "TRIANGLES",
   "TRIANGLE_STRIP",
   "TRIANGLE_FAN",
   "QUADS",
   "QUAD_STRIP",
   "POLYGON"
};

static const char *fs_coord_origin_names[] =
{
   "UPPER_LEFT",
   "LOWER_LEFT"
};

static const char *fs_coord_pixel_center_names[] =
{
   "HALF_INTEGER",
   "INTEGER"
};


static void
_dump_register_dst(
   struct dump_ctx *ctx,
   uint file,
   int index)
{
   ENM( file, file_names );

   CHR( '[' );
   SID( index );
   CHR( ']' );
}


static void
_dump_register_src(
   struct dump_ctx *ctx,
   const struct tgsi_full_src_register *src )
{
   ENM(src->Register.File, file_names);
   if (src->Register.Dimension) {
      CHR('[');
      SID(src->Dimension.Index);
      CHR(']');
   }
   if (src->Register.Indirect) {
      CHR( '[' );
      ENM( src->Indirect.File, file_names );
      CHR( '[' );
      SID( src->Indirect.Index );
      TXT( "]." );
      ENM( src->Indirect.SwizzleX, swizzle_names );
      if (src->Register.Index != 0) {
         if (src->Register.Index > 0)
            CHR( '+' );
         SID( src->Register.Index );
      }
      CHR( ']' );
   } else {
      CHR( '[' );
      SID( src->Register.Index );
      CHR( ']' );
   }
}

static void
_dump_register_ind(
   struct dump_ctx *ctx,
   uint file,
   int index,
   uint ind_file,
   int ind_index,
   uint ind_swizzle )
{
   ENM( file, file_names );
   CHR( '[' );
   ENM( ind_file, file_names );
   CHR( '[' );
   SID( ind_index );
   TXT( "]." );
   ENM( ind_swizzle, swizzle_names );
   if (index != 0) {
      if (index > 0)
         CHR( '+' );
      SID( index );
   }
   CHR( ']' );
}

static void
_dump_writemask(
   struct dump_ctx *ctx,
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
   struct dump_ctx *ctx = (struct dump_ctx *)iter;

   assert(Elements(semantic_names) == TGSI_SEMANTIC_COUNT);
   assert(Elements(interpolate_names) == TGSI_INTERPOLATE_COUNT);

   TXT( "DCL " );

   ENM(decl->Declaration.File, file_names);

   /* all geometry shader inputs are two dimensional */
   if (decl->Declaration.File == TGSI_FILE_INPUT &&
       iter->processor.Processor == TGSI_PROCESSOR_GEOMETRY) {
      TXT("[]");
   }

   if (decl->Declaration.Dimension) {
      CHR('[');
      SID(decl->Dim.Index2D);
      CHR(']');
   }

   CHR('[');
   SID(decl->Range.First);
   if (decl->Range.First != decl->Range.Last) {
      TXT("..");
      SID(decl->Range.Last);
   }
   CHR(']');

   _dump_writemask(
      ctx,
      decl->Declaration.UsageMask );

   if (decl->Declaration.Semantic) {
      TXT( ", " );
      ENM( decl->Semantic.Name, semantic_names );
      if (decl->Semantic.Index != 0 ||
          decl->Semantic.Name == TGSI_SEMANTIC_GENERIC) {
         CHR( '[' );
         UID( decl->Semantic.Index );
         CHR( ']' );
      }
   }

   if (iter->processor.Processor == TGSI_PROCESSOR_FRAGMENT &&
       decl->Declaration.File == TGSI_FILE_INPUT)
   {
      TXT( ", " );
      ENM( decl->Declaration.Interpolate, interpolate_names );
   }

   if (decl->Declaration.Centroid) {
      TXT( ", CENTROID" );
   }

   if (decl->Declaration.Invariant) {
      TXT( ", INVARIANT" );
   }

   if (decl->Declaration.CylindricalWrap) {
      TXT(", CYLWRAP_");
      if (decl->Declaration.CylindricalWrap & TGSI_CYLINDRICAL_WRAP_X) {
         CHR('X');
      }
      if (decl->Declaration.CylindricalWrap & TGSI_CYLINDRICAL_WRAP_Y) {
         CHR('Y');
      }
      if (decl->Declaration.CylindricalWrap & TGSI_CYLINDRICAL_WRAP_Z) {
         CHR('Z');
      }
      if (decl->Declaration.CylindricalWrap & TGSI_CYLINDRICAL_WRAP_W) {
         CHR('W');
      }
   }

   EOL();

   return TRUE;
}

void
tgsi_dump_declaration(
   const struct tgsi_full_declaration *decl )
{
   struct dump_ctx ctx;

   ctx.printf = dump_ctx_printf;

   iter_declaration( &ctx.iter, (struct tgsi_full_declaration *)decl );
}

static boolean
iter_property(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_property *prop )
{
   int i;
   struct dump_ctx *ctx = (struct dump_ctx *)iter;

   assert(Elements(property_names) == TGSI_PROPERTY_COUNT);

   TXT( "PROPERTY " );
   ENM(prop->Property.PropertyName, property_names);

   if (prop->Property.NrTokens > 1)
      TXT(" ");

   for (i = 0; i < prop->Property.NrTokens - 1; ++i) {
      switch (prop->Property.PropertyName) {
      case TGSI_PROPERTY_GS_INPUT_PRIM:
      case TGSI_PROPERTY_GS_OUTPUT_PRIM:
         ENM(prop->u[i].Data, primitive_names);
         break;
      case TGSI_PROPERTY_FS_COORD_ORIGIN:
         ENM(prop->u[i].Data, fs_coord_origin_names);
         break;
      case TGSI_PROPERTY_FS_COORD_PIXEL_CENTER:
         ENM(prop->u[i].Data, fs_coord_pixel_center_names);
         break;
      default:
         SID( prop->u[i].Data );
         break;
      }
      if (i < prop->Property.NrTokens - 2)
         TXT( ", " );
   }
   EOL();

   return TRUE;
}

void tgsi_dump_property(
   const struct tgsi_full_property *prop )
{
   struct dump_ctx ctx;

   ctx.printf = dump_ctx_printf;

   iter_property( &ctx.iter, (struct tgsi_full_property *)prop );
}

static boolean
iter_immediate(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_immediate *imm )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;

   uint i;

   TXT( "IMM " );
   ENM( imm->Immediate.DataType, immediate_type_names );

   TXT( " { " );

   assert( imm->Immediate.NrTokens <= 4 + 1 );
   for (i = 0; i < imm->Immediate.NrTokens - 1; i++) {
      switch (imm->Immediate.DataType) {
      case TGSI_IMM_FLOAT32:
         FLT( imm->u[i].Float );
         break;
      case TGSI_IMM_UINT32:
         UID(imm->u[i].Uint);
         break;
      case TGSI_IMM_INT32:
         SID(imm->u[i].Int);
         break;
      default:
         assert( 0 );
      }

      if (i < imm->Immediate.NrTokens - 2)
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
   struct dump_ctx ctx;

   ctx.printf = dump_ctx_printf;

   iter_immediate( &ctx.iter, (struct tgsi_full_immediate *)imm );
}

static boolean
iter_instruction(
   struct tgsi_iterate_context *iter,
   struct tgsi_full_instruction *inst )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;
   uint instno = ctx->instno++;
   const struct tgsi_opcode_info *info = tgsi_get_opcode_info( inst->Instruction.Opcode );
   uint i;
   boolean first_reg = TRUE;

   INSTID( instno );
   TXT( ": " );
   
   ctx->indent -= info->pre_dedent;
   for(i = 0; (int)i < ctx->indent; ++i)
      TXT( "  " );
   ctx->indent += info->post_indent;
   
   TXT( info->mnemonic );

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
      const struct tgsi_full_dst_register *dst = &inst->Dst[i];

      if (!first_reg)
         CHR( ',' );
      CHR( ' ' );

      if (dst->Register.Indirect) {
         _dump_register_ind(
            ctx,
            dst->Register.File,
            dst->Register.Index,
            dst->Indirect.File,
            dst->Indirect.Index,
            dst->Indirect.SwizzleX );
      }
      else {
         _dump_register_dst(
            ctx,
            dst->Register.File,
            dst->Register.Index );
      }
      _dump_writemask( ctx, dst->Register.WriteMask );

      first_reg = FALSE;
   }

   for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
      const struct tgsi_full_src_register *src = &inst->Src[i];

      if (!first_reg)
         CHR( ',' );
      CHR( ' ' );

      if (src->Register.Negate)
         CHR( '-' );
      if (src->Register.Absolute)
         CHR( '|' );

      _dump_register_src(ctx, src);

      if (src->Register.SwizzleX != TGSI_SWIZZLE_X ||
          src->Register.SwizzleY != TGSI_SWIZZLE_Y ||
          src->Register.SwizzleZ != TGSI_SWIZZLE_Z ||
          src->Register.SwizzleW != TGSI_SWIZZLE_W) {
         CHR( '.' );
         ENM( src->Register.SwizzleX, swizzle_names );
         ENM( src->Register.SwizzleY, swizzle_names );
         ENM( src->Register.SwizzleZ, swizzle_names );
         ENM( src->Register.SwizzleW, swizzle_names );
      }

      if (src->Register.Absolute)
         CHR( '|' );

      first_reg = FALSE;
   }

   if (inst->Instruction.Texture) {
      TXT( ", " );
      ENM( inst->Texture.Texture, texture_names );
   }

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_ELSE:
   case TGSI_OPCODE_BGNLOOP:
   case TGSI_OPCODE_ENDLOOP:
   case TGSI_OPCODE_CAL:
      TXT( " :" );
      UID( inst->Label.Label );
      break;
   }

   /* update indentation */
   if (inst->Instruction.Opcode == TGSI_OPCODE_IF ||
       inst->Instruction.Opcode == TGSI_OPCODE_ELSE ||
       inst->Instruction.Opcode == TGSI_OPCODE_BGNFOR ||
       inst->Instruction.Opcode == TGSI_OPCODE_BGNLOOP) {
      ctx->indentation += indent_spaces;
   }

   EOL();

   return TRUE;
}

void
tgsi_dump_instruction(
   const struct tgsi_full_instruction *inst,
   uint instno )
{
   struct dump_ctx ctx;

   ctx.instno = instno;
   ctx.indent = 0;
   ctx.printf = dump_ctx_printf;
   ctx.indentation = 0;

   iter_instruction( &ctx.iter, (struct tgsi_full_instruction *)inst );
}

static boolean
prolog(
   struct tgsi_iterate_context *iter )
{
   struct dump_ctx *ctx = (struct dump_ctx *) iter;
   ENM( iter->processor.Processor, processor_type_names );
   EOL();
   return TRUE;
}

void
tgsi_dump(
   const struct tgsi_token *tokens,
   uint flags )
{
   struct dump_ctx ctx;

   ctx.iter.prolog = prolog;
   ctx.iter.iterate_instruction = iter_instruction;
   ctx.iter.iterate_declaration = iter_declaration;
   ctx.iter.iterate_immediate = iter_immediate;
   ctx.iter.iterate_property = iter_property;
   ctx.iter.epilog = NULL;

   ctx.instno = 0;
   ctx.indent = 0;
   ctx.printf = dump_ctx_printf;
   ctx.indentation = 0;

   tgsi_iterate_shader( tokens, &ctx.iter );
}

struct str_dump_ctx
{
   struct dump_ctx base;
   char *str;
   char *ptr;
   int left;
};

static void
str_dump_ctx_printf(struct dump_ctx *ctx, const char *format, ...)
{
   struct str_dump_ctx *sctx = (struct str_dump_ctx *)ctx;
   
   if(sctx->left > 1) {
      int written;
      va_list ap;
      va_start(ap, format);
      written = util_vsnprintf(sctx->ptr, sctx->left, format, ap);
      va_end(ap);

      /* Some complicated logic needed to handle the return value of
       * vsnprintf:
       */
      if (written > 0) {
         written = MIN2(sctx->left, written);
         sctx->ptr += written;
         sctx->left -= written;
      }
   }
}

void
tgsi_dump_str(
   const struct tgsi_token *tokens,
   uint flags,
   char *str,
   size_t size)
{
   struct str_dump_ctx ctx;

   ctx.base.iter.prolog = prolog;
   ctx.base.iter.iterate_instruction = iter_instruction;
   ctx.base.iter.iterate_declaration = iter_declaration;
   ctx.base.iter.iterate_immediate = iter_immediate;
   ctx.base.iter.iterate_property = iter_property;
   ctx.base.iter.epilog = NULL;

   ctx.base.instno = 0;
   ctx.base.indent = 0;
   ctx.base.printf = &str_dump_ctx_printf;
   ctx.base.indentation = 0;

   ctx.str = str;
   ctx.str[0] = 0;
   ctx.ptr = str;
   ctx.left = (int)size;

   tgsi_iterate_shader( tokens, &ctx.base.iter );
}
