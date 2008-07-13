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
#include "tgsi_text.h"
#include "tgsi_build.h"
#include "tgsi_parse.h"
#include "tgsi_util.h"

static boolean is_alpha_underscore( const char *cur )
{
   return
      (*cur >= 'a' && *cur <= 'z') ||
      (*cur >= 'A' && *cur <= 'Z') ||
      *cur == '_';
}

static boolean is_digit( const char *cur )
{
   return *cur >= '0' && *cur <= '9';
}

static boolean is_digit_alpha_underscore( const char *cur )
{
   return is_digit( cur ) || is_alpha_underscore( cur );
}

static boolean str_match_no_case( const char **pcur, const char *str )
{
   const char *cur = *pcur;

   while (*str != '\0' && *str == toupper( *cur )) {
      str++;
      cur++;
   }
   if (*str == '\0') {
      *pcur = cur;
      return TRUE;
   }
   return FALSE;
}

/* Eat zero or more whitespaces.
 */
static void eat_opt_white( const char **pcur )
{
   while (**pcur == ' ' || **pcur == '\t' || **pcur == '\n')
      (*pcur)++;
}

/* Eat one or more whitespaces.
 * Return TRUE if at least one whitespace eaten.
 */
static boolean eat_white( const char **pcur )
{
   const char *cur = *pcur;

   eat_opt_white( pcur );
   return *pcur > cur;
}

/* Parse unsigned integer.
 * No checks for overflow.
 */
static boolean parse_uint( const char **pcur, uint *val )
{
   const char *cur = *pcur;

   if (is_digit( cur )) {
      *val = *cur++ - '0';
      while (is_digit( cur ))
         *val = *val * 10 + *cur++ - '0';
      *pcur = cur;
      return TRUE;
   }
   return FALSE;
}

struct translate_ctx
{
   const char *text;
   const char *cur;
   struct tgsi_token *tokens;
   struct tgsi_token *tokens_cur;
   struct tgsi_token *tokens_end;
   struct tgsi_header *header;
};

static void report_error( struct translate_ctx *ctx, const char *msg )
{
   debug_printf( "\nError: %s", msg );
}

/* Parse shader header.
 * Return TRUE for one of the following headers.
 *    FRAG1.1
 *    GEOM1.1
 *    VERT1.1
 */
static boolean parse_header( struct translate_ctx *ctx )
{
   uint processor;

   if (str_match_no_case( &ctx->cur, "FRAG1.1" ))
      processor = TGSI_PROCESSOR_FRAGMENT;
   else if (str_match_no_case( &ctx->cur, "VERT1.1" ))
      processor = TGSI_PROCESSOR_VERTEX;
   else if (str_match_no_case( &ctx->cur, "GEOM1.1" ))
      processor = TGSI_PROCESSOR_GEOMETRY;
   else {
      report_error( ctx, "Unknown header" );
      return FALSE;
   }

   if (ctx->tokens_cur >= ctx->tokens_end)
      return FALSE;
   *(struct tgsi_version *) ctx->tokens_cur++ = tgsi_build_version();

   if (ctx->tokens_cur >= ctx->tokens_end)
      return FALSE;
   ctx->header = (struct tgsi_header *) ctx->tokens_cur++;
   *ctx->header = tgsi_build_header();

   if (ctx->tokens_cur >= ctx->tokens_end)
      return FALSE;
   *(struct tgsi_processor *) ctx->tokens_cur++ = tgsi_build_processor( processor, ctx->header );

   return TRUE;
}

static boolean parse_label( struct translate_ctx *ctx, uint *val )
{
   const char *cur = ctx->cur;

   if (parse_uint( &cur, val )) {
      eat_opt_white( &cur );
      if (*cur == ':') {
         cur++;
         ctx->cur = cur;
         return TRUE;
      }
   }
   return FALSE;
}

static const char *file_names[TGSI_FILE_COUNT] =
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
parse_file(
   struct translate_ctx *ctx,
   uint *file )
{
   uint i;

   for (i = 0; i < TGSI_FILE_COUNT; i++) {
      const char *cur = ctx->cur;

      if (str_match_no_case( &cur, file_names[i] )) {
         if (!is_digit_alpha_underscore( cur )) {
            ctx->cur = cur;
            *file = i;
            return TRUE;
         }
      }
   }
   report_error( ctx, "Unknown register file" );
   return FALSE;
}

static boolean
parse_opt_writemask(
   struct translate_ctx *ctx,
   uint *writemask )
{
   const char *cur;

   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == '.') {
      cur++;
      *writemask = TGSI_WRITEMASK_NONE;
      eat_opt_white( &cur );
      if (toupper( *cur ) == 'X') {
         cur++;
         *writemask |= TGSI_WRITEMASK_X;
      }
      if (toupper( *cur ) == 'Y') {
         cur++;
         *writemask |= TGSI_WRITEMASK_Y;
      }
      if (toupper( *cur ) == 'Z') {
         cur++;
         *writemask |= TGSI_WRITEMASK_Z;
      }
      if (toupper( *cur ) == 'W') {
         cur++;
         *writemask |= TGSI_WRITEMASK_W;
      }

      if (*writemask == TGSI_WRITEMASK_NONE) {
         report_error( ctx, "Writemask expected" );
         return FALSE;
      }

      ctx->cur = cur;
   }
   else {
      *writemask = TGSI_WRITEMASK_XYZW;
   }
   return TRUE;
}

/* Parse register part common for decls and operands.
 *    <register_prefix> ::= <file> `[' <index>
 */
static boolean
parse_register_prefix(
   struct translate_ctx *ctx,
   uint *file,
   uint *index )
{
   if (!parse_file( ctx, file ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '[') {
      report_error( ctx, "Expected `['" );
      return FALSE;
   }
   ctx->cur++;
   eat_opt_white( &ctx->cur );
   if (!parse_uint( &ctx->cur, index )) {
      report_error( ctx, "Expected literal integer" );
      return FALSE;
   }
   return TRUE;
}

/* Parse register operand.
 *    <register> ::= <register_prefix> `]'
 */
static boolean
parse_register(
   struct translate_ctx *ctx,
   uint *file,
   uint *index )
{
   if (!parse_register_prefix( ctx, file, index ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      return FALSE;
   }
   ctx->cur++;
   /* TODO: Modulate suffix */
   return TRUE;
}

/* Parse register declaration.
 *    <register> ::= <register_prefix> `]' | <register_prefix> `..' <index> `]'
 */
static boolean
parse_register_dcl(
   struct translate_ctx *ctx,
   uint *file,
   uint *first,
   uint *last )
{
   if (!parse_register_prefix( ctx, file, first ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (ctx->cur[0] == '.' && ctx->cur[1] == '.') {
      ctx->cur += 2;
      eat_opt_white( &ctx->cur );
      if (!parse_uint( &ctx->cur, last )) {
         report_error( ctx, "Expected literal integer" );
         return FALSE;
      }
      eat_opt_white( &ctx->cur );
   }
   else {
      *last = *first;
   }
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]' or `..'" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

static boolean
parse_dst_operand(
   struct translate_ctx *ctx,
   struct tgsi_full_dst_register *dst )
{
   uint file;
   uint index;
   uint writemask;

   if (!parse_register( ctx, &file, &index ))
      return FALSE;
   if (!parse_opt_writemask( ctx, &writemask ))
      return FALSE;
   dst->DstRegister.File = file;
   dst->DstRegister.Index = index;
   dst->DstRegister.WriteMask = writemask;
   return TRUE;
}

static boolean
parse_src_operand(
   struct translate_ctx *ctx,
   struct tgsi_full_src_register *src )
{
   const char *cur;
   uint file;
   uint index;

   /* TODO: Extended register modifiers */
   if (*ctx->cur == '-') {
      ctx->cur++;
      src->SrcRegister.Negate = 1;
      eat_opt_white( &ctx->cur );
   }

   if (!parse_register( ctx, &file, &index ))
      return FALSE;
   src->SrcRegister.File = file;
   src->SrcRegister.Index = index;

   /* Parse optional swizzle
    */
   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == '.') {
      uint i;

      cur++;
      eat_opt_white( &cur );
      for (i = 0; i < 4; i++) {
         uint swizzle;

         if (toupper( *cur ) == 'X')
            swizzle = TGSI_SWIZZLE_X;
         else if (toupper( *cur ) == 'Y')
            swizzle = TGSI_SWIZZLE_Y;
         else if (toupper( *cur ) == 'Z')
            swizzle = TGSI_SWIZZLE_Z;
         else if (toupper( *cur ) == 'W')
            swizzle = TGSI_SWIZZLE_W;
         else {
            report_error( ctx, "Expected register swizzle component either `x', `y', `z' or `w'" );
            return FALSE;
         }
         cur++;
         tgsi_util_set_src_register_swizzle( &src->SrcRegister, swizzle, i );
      }

      ctx->cur = cur;
   }
   return TRUE;
}

struct opcode_info
{
   uint num_dst;
   uint num_src;
   uint is_tex;
   const char *mnemonic;
};

static const struct opcode_info opcode_info[TGSI_OPCODE_LAST] =
{
   { 1, 1, 0, "ARL" },
   { 1, 1, 0, "MOV" },
   { 1, 1, 0, "LIT" },
   { 1, 1, 0, "RCP" },
   { 1, 1, 0, "RSQ" },
   { 1, 1, 0, "EXP" },
   { 1, 1, 0, "LOG" },
   { 1, 2, 0, "MUL" },
   { 1, 2, 0, "ADD" },
   { 1, 2, 0, "DP3" },
   { 1, 2, 0, "DP4" },
   { 1, 2, 0, "DST" },
   { 1, 2, 0, "MIN" },
   { 1, 2, 0, "MAX" },
   { 1, 2, 0, "SLT" },
   { 1, 2, 0, "SGE" },
   { 1, 3, 0, "MAD" },
   { 1, 2, 0, "SUB" },
   { 1, 3, 0, "LERP" },
   { 1, 3, 0, "CND" },
   { 1, 3, 0, "CND0" },
   { 1, 3, 0, "DOT2ADD" },
   { 1, 2, 0, "INDEX" },
   { 1, 1, 0, "NEGATE" },
   { 1, 1, 0, "FRAC" },
   { 1, 3, 0, "CLAMP" },
   { 1, 1, 0, "FLOOR" },
   { 1, 1, 0, "ROUND" },
   { 1, 1, 0, "EXPBASE2" },
   { 1, 1, 0, "LOGBASE2" },
   { 1, 2, 0, "POWER" },
   { 1, 2, 0, "CROSSPRODUCT" },
   { 1, 2, 0, "MULTIPLYMATRIX" },
   { 1, 1, 0, "ABS" },
   { 1, 1, 0, "RCC" },
   { 1, 2, 0, "DPH" },
   { 1, 1, 0, "COS" },
   { 1, 1, 0, "DDX" },
   { 1, 1, 0, "DDY" },
   { 0, 1, 0, "KILP" },
   { 1, 1, 0, "PK2H" },
   { 1, 1, 0, "PK2US" },
   { 1, 1, 0, "PK4B" },
   { 1, 1, 0, "PK4UB" },
   { 1, 2, 0, "RFL" },
   { 1, 2, 0, "SEQ" },
   { 1, 2, 0, "SFL" },
   { 1, 2, 0, "SGT" },
   { 1, 1, 0, "SIN" },
   { 1, 2, 0, "SLE" },
   { 1, 2, 0, "SNE" },
   { 1, 2, 0, "STR" },
   { 1, 2, 1, "TEX" },
   { 1, 4, 1, "TXD" },
   { 1, 2, 1, "TXP" },
   { 1, 1, 0, "UP2H" },
   { 1, 1, 0, "UP2US" },
   { 1, 1, 0, "UP4B" },
   { 1, 1, 0, "UP4UB" },
   { 1, 3, 0, "X2D" },
   { 1, 1, 0, "ARA" },
   { 1, 1, 0, "ARR" },
   { 0, 1, 0, "BRA" },
   { 0, 0, 0, "CAL" },
   { 0, 0, 0, "RET" },
   { 1, 1, 0, "SSG" },
   { 1, 3, 0, "CMP" },
   { 1, 1, 0, "SCS" },
   { 1, 2, 1, "TXB" },
   { 1, 1, 0, "NRM" },
   { 1, 2, 0, "DIV" },
   { 1, 2, 0, "DP2" },
   { 1, 2, 1, "TXL" },
   { 0, 0, 0, "BRK" },
   { 0, 1, 0, "IF" },
   { 0, 0, 0, "LOOP" },
   { 0, 1, 0, "REP" },
   { 0, 0, 0, "ELSE" },
   { 0, 0, 0, "ENDIF" },
   { 0, 0, 0, "ENDLOOP" },
   { 0, 0, 0, "ENDREP" },
   { 0, 1, 0, "PUSHA" },
   { 1, 0, 0, "POPA" },
   { 1, 1, 0, "CEIL" },
   { 1, 1, 0, "I2F" },
   { 1, 1, 0, "NOT" },
   { 1, 1, 0, "TRUNC" },
   { 1, 2, 0, "SHL" },
   { 1, 2, 0, "SHR" },
   { 1, 2, 0, "AND" },
   { 1, 2, 0, "OR" },
   { 1, 2, 0, "MOD" },
   { 1, 2, 0, "XOR" },
   { 1, 3, 0, "SAD" },
   { 1, 2, 1, "TXF" },
   { 1, 2, 1, "TXQ" },
   { 0, 0, 0, "CONT" },
   { 0, 0, 0, "EMIT" },
   { 0, 0, 0, "ENDPRIM" },
   { 0, 0, 0, "BGNLOOP2" },
   { 0, 0, 0, "BGNSUB" },
   { 0, 0, 0, "ENDLOOP2" },
   { 0, 0, 0, "ENDSUB" },
   { 1, 1, 0, "NOISE1" },
   { 1, 1, 0, "NOISE2" },
   { 1, 1, 0, "NOISE3" },
   { 1, 1, 0, "NOISE4" },
   { 0, 0, 0, "NOP" },
   { 1, 2, 0, "M4X3" },
   { 1, 2, 0, "M3X4" },
   { 1, 2, 0, "M3X3" },
   { 1, 2, 0, "M3X2" },
   { 1, 1, 0, "NRM4" },
   { 0, 1, 0, "CALLNZ" },
   { 0, 1, 0, "IFC" },
   { 0, 1, 0, "BREAKC" },
   { 0, 0, 0, "KIL" },
   { 0, 0, 0, "END" },
   { 1, 1, 0, "SWZ" }
};

static const char *texture_names[TGSI_TEXTURE_COUNT] =
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

static boolean parse_instruction( struct translate_ctx *ctx )
{
   uint i;
   const struct opcode_info *info;
   struct tgsi_full_instruction inst;
   uint advance;

   /* Parse instruction name.
    */
   eat_opt_white( &ctx->cur );
   for (i = 0; i < TGSI_OPCODE_LAST; i++) {
      const char *cur = ctx->cur;

      if (str_match_no_case( &cur, opcode_info[i].mnemonic )) {
         /* TODO: _SAT suffix */
         if (*cur == '\0' || eat_white( &cur )) {
            ctx->cur = cur;
            break;
         }
      }
   }
   if (i == TGSI_OPCODE_LAST) {
      report_error( ctx, "Unknown opcode" );
      return FALSE;
   }
   info = &opcode_info[i];

   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = i;
   inst.Instruction.NumDstRegs = info->num_dst;
   inst.Instruction.NumSrcRegs = info->num_src;

   /* Parse instruction operands.
    */
   for (i = 0; i < info->num_dst + info->num_src + info->is_tex; i++) {
      if (i > 0) {
         eat_opt_white( &ctx->cur );
         if (*ctx->cur != ',') {
            report_error( ctx, "Expected `,'" );
            return FALSE;
         }
         ctx->cur++;
         eat_opt_white( &ctx->cur );
      }

      if (i < info->num_dst) {
         if (!parse_dst_operand( ctx, &inst.FullDstRegisters[i] ))
            return FALSE;
      }
      else if (i < info->num_dst + info->num_src) {
         if (!parse_src_operand( ctx, &inst.FullSrcRegisters[i - info->num_dst] ))
            return FALSE;
      }
      else {
         uint j;

         for (j = 0; j < TGSI_TEXTURE_COUNT; j++) {
            if (str_match_no_case( &ctx->cur, texture_names[j] )) {
               if (!is_digit_alpha_underscore( ctx->cur )) {
                  inst.InstructionExtTexture.Texture = j;
                  break;
               }
            }
         }
         if (j == TGSI_TEXTURE_COUNT) {
            report_error( ctx, "Expected texture target" );
            return FALSE;
         }
      }
   }

   advance = tgsi_build_full_instruction(
      &inst,
      ctx->tokens_cur,
      ctx->header,
      (uint) (ctx->tokens_end - ctx->tokens_cur) );
   if (advance == 0)
      return FALSE;
   ctx->tokens_cur += advance;

   return TRUE;
}

static const char *semantic_names[TGSI_SEMANTIC_COUNT] =
{
   "POSITION",
   "COLOR",
   "BCOLOR",
   "FOG",
   "PSIZE",
   "GENERIC",
   "NORMAL"
};

static const char *interpolate_names[TGSI_INTERPOLATE_COUNT] =
{
   "CONSTANT",
   "LINEAR",
   "PERSPECTIVE"
};

static boolean parse_declaration( struct translate_ctx *ctx )
{
   struct tgsi_full_declaration decl;
   uint file;
   uint first;
   uint last;
   uint writemask;
   const char *cur;
   uint advance;

   if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      return FALSE;
   }
   if (!parse_register_dcl( ctx, &file, &first, &last ))
      return FALSE;
   if (!parse_opt_writemask( ctx, &writemask ))
      return FALSE;

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = file;
   decl.Declaration.UsageMask = writemask;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;

   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',') {
      uint i;

      cur++;
      eat_opt_white( &cur );
      for (i = 0; i < TGSI_SEMANTIC_COUNT; i++) {
         if (str_match_no_case( &cur, semantic_names[i] )) {
            const char *cur2 = cur;
            uint index;

            if (is_digit_alpha_underscore( cur ))
               continue;
            eat_opt_white( &cur2 );
            if (*cur2 == '[') {
               cur2++;
               eat_opt_white( &cur2 );
               if (!parse_uint( &cur2, &index )) {
                  report_error( ctx, "Expected literal integer" );
                  return FALSE;
               }
               eat_opt_white( &cur2 );
               if (*cur2 != ']') {
                  report_error( ctx, "Expected `]'" );
                  return FALSE;
               }
               cur2++;

               decl.Semantic.SemanticIndex = index;

               cur = cur2;
            }

            decl.Declaration.Semantic = 1;
            decl.Semantic.SemanticName = i;

            ctx->cur = cur;
            break;
         }
      }
   }

   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == ',') {
      uint i;

      cur++;
      eat_opt_white( &cur );
      for (i = 0; i < TGSI_INTERPOLATE_COUNT; i++) {
         if (str_match_no_case( &cur, interpolate_names[i] )) {
            if (is_digit_alpha_underscore( cur ))
               continue;
            decl.Declaration.Interpolate = i;

            ctx->cur = cur;
            break;
         }
      }
      if (i == TGSI_INTERPOLATE_COUNT) {
         report_error( ctx, "Expected semantic or interpolate attribute" );
         return FALSE;
      }
   }

   advance = tgsi_build_full_declaration(
      &decl,
      ctx->tokens_cur,
      ctx->header,
      (uint) (ctx->tokens_end - ctx->tokens_cur) );
   if (advance == 0)
      return FALSE;
   ctx->tokens_cur += advance;

   return TRUE;
}

static boolean translate( struct translate_ctx *ctx )
{
   eat_opt_white( &ctx->cur );
   if (!parse_header( ctx ))
      return FALSE;

   while (*ctx->cur != '\0') {
      uint label_val = 0;

      if (!eat_white( &ctx->cur )) {
         report_error( ctx, "Syntax error" );
         return FALSE;
      }

      if (parse_label( ctx, &label_val )) {
         if (!parse_instruction( ctx ))
            return FALSE;
      }
      else if (str_match_no_case( &ctx->cur, "DCL" )) {
         if (!parse_declaration( ctx ))
            return FALSE;
      }
      else {
         report_error( ctx, "Expected `DCL' or a label" );
         return FALSE;
      }
   }

   return TRUE;
}

boolean
tgsi_text_translate(
   const char *text,
   struct tgsi_token *tokens,
   uint num_tokens )
{
   struct translate_ctx ctx;

   ctx.text = text;
   ctx.cur = text;
   ctx.tokens = tokens;
   ctx.tokens_cur = tokens;
   ctx.tokens_end = tokens + num_tokens;

   return translate( &ctx );
}
