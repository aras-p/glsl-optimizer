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
#include "tgsi_info.h"
#include "tgsi_parse.h"
#include "tgsi_sanity.h"
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

static boolean uprcase( char c )
{
   if (c >= 'a' && c <= 'z')
      return c += 'A' - 'a';
   return c;
}

static boolean str_match_no_case( const char **pcur, const char *str )
{
   const char *cur = *pcur;

   while (*str != '\0' && *str == uprcase( *cur )) {
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

/* Parse floating point.
 */
static boolean parse_float( const char **pcur, float *val )
{
   const char *cur = *pcur;
   boolean integral_part = FALSE;
   boolean fractional_part = FALSE;

   *val = (float) atof( cur );

   if (*cur == '-' || *cur == '+')
      cur++;
   if (is_digit( cur )) {
      cur++;
      integral_part = TRUE;
      while (is_digit( cur ))
         cur++;
   }
   if (*cur == '.') {
      cur++;
      if (is_digit( cur )) {
         cur++;
         fractional_part = TRUE;
         while (is_digit( cur ))
            cur++;
      }
   }
   if (!integral_part && !fractional_part)
      return FALSE;
   if (uprcase( *cur ) == 'E') {
      cur++;
      if (*cur == '-' || *cur == '+')
         cur++;
      if (is_digit( cur )) {
         cur++;
         while (is_digit( cur ))
            cur++;
      }
      else
         return FALSE;
   }
   *pcur = cur;
   return TRUE;
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
parse_file( const char **pcur, uint *file )
{
   uint i;

   for (i = 0; i < TGSI_FILE_COUNT; i++) {
      const char *cur = *pcur;

      if (str_match_no_case( &cur, file_names[i] )) {
         if (!is_digit_alpha_underscore( cur )) {
            *pcur = cur;
            *file = i;
            return TRUE;
         }
      }
   }
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
      if (uprcase( *cur ) == 'X') {
         cur++;
         *writemask |= TGSI_WRITEMASK_X;
      }
      if (uprcase( *cur ) == 'Y') {
         cur++;
         *writemask |= TGSI_WRITEMASK_Y;
      }
      if (uprcase( *cur ) == 'Z') {
         cur++;
         *writemask |= TGSI_WRITEMASK_Z;
      }
      if (uprcase( *cur ) == 'W') {
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

/* <register_file_bracket> ::= <file> `['
 */
static boolean
parse_register_file_bracket(
   struct translate_ctx *ctx,
   uint *file )
{
   if (!parse_file( &ctx->cur, file )) {
      report_error( ctx, "Unknown register file" );
      return FALSE;
   }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '[') {
      report_error( ctx, "Expected `['" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

/* <register_file_bracket_index> ::= <register_file_bracket> <uint>
 */
static boolean
parse_register_file_bracket_index(
   struct translate_ctx *ctx,
   uint *file,
   int *index )
{
   uint uindex;

   if (!parse_register_file_bracket( ctx, file ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (!parse_uint( &ctx->cur, &uindex )) {
      report_error( ctx, "Expected literal unsigned integer" );
      return FALSE;
   }
   *index = (int) uindex;
   return TRUE;
}

/* Parse destination register operand.
 *    <register_dst> ::= <register_file_bracket_index> `]'
 */
static boolean
parse_register_dst(
   struct translate_ctx *ctx,
   uint *file,
   int *index )
{
   if (!parse_register_file_bracket_index( ctx, file, index ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

/* Parse source register operand.
 *    <register_src> ::= <register_file_bracket_index> `]' |
 *                       <register_file_bracket> <register_dst> `]' |
 *                       <register_file_bracket> <register_dst> `+' <uint> `]' |
 *                       <register_file_bracket> <register_dst> `-' <uint> `]'
 */
static boolean
parse_register_src(
   struct translate_ctx *ctx,
   uint *file,
   int *index,
   uint *ind_file,
   int *ind_index )
{
   const char *cur;
   uint uindex;

   if (!parse_register_file_bracket( ctx, file ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   cur = ctx->cur;
   if (parse_file( &cur, ind_file )) {
      if (!parse_register_dst( ctx, ind_file, ind_index ))
         return FALSE;
      eat_opt_white( &ctx->cur );
      if (*ctx->cur == '+' || *ctx->cur == '-') {
         boolean negate;

         negate = *ctx->cur == '-';
         ctx->cur++;
         eat_opt_white( &ctx->cur );
         if (!parse_uint( &ctx->cur, &uindex )) {
            report_error( ctx, "Expected literal unsigned integer" );
            return FALSE;
         }
         if (negate)
            *index = -(int) uindex;
         else
            *index = (int) uindex;
      }
      else {
         *index = 0;
      }
   }
   else {
      if (!parse_uint( &ctx->cur, &uindex )) {
         report_error( ctx, "Expected literal unsigned integer" );
         return FALSE;
      }
      *index = (int) uindex;
      *ind_file = TGSI_FILE_NULL;
      *ind_index = 0;
   }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      return FALSE;
   }
   ctx->cur++;
   return TRUE;
}

/* Parse register declaration.
 *    <register_dcl> ::= <register_file_bracket_index> `]' |
 *                       <register_file_bracket_index> `..' <index> `]'
 */
static boolean
parse_register_dcl(
   struct translate_ctx *ctx,
   uint *file,
   int *first,
   int *last )
{
   if (!parse_register_file_bracket_index( ctx, file, first ))
      return FALSE;
   eat_opt_white( &ctx->cur );
   if (ctx->cur[0] == '.' && ctx->cur[1] == '.') {
      uint uindex;

      ctx->cur += 2;
      eat_opt_white( &ctx->cur );
      if (!parse_uint( &ctx->cur, &uindex )) {
         report_error( ctx, "Expected literal integer" );
         return FALSE;
      }
      *last = (int) uindex;
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

static const char *modulate_names[TGSI_MODULATE_COUNT] =
{
   "_1X",
   "_2X",
   "_4X",
   "_8X",
   "_D2",
   "_D4",
   "_D8"
};

static boolean
parse_dst_operand(
   struct translate_ctx *ctx,
   struct tgsi_full_dst_register *dst )
{
   uint file;
   int index;
   uint writemask;
   const char *cur;

   if (!parse_register_dst( ctx, &file, &index ))
      return FALSE;

   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == '_') {
      uint i;

      for (i = 0; i < TGSI_MODULATE_COUNT; i++) {
         if (str_match_no_case( &cur, modulate_names[i] )) {
            if (!is_digit_alpha_underscore( cur )) {
               dst->DstRegisterExtModulate.Modulate = i;
               ctx->cur = cur;
               break;
            }
         }
      }
   }

   if (!parse_opt_writemask( ctx, &writemask ))
      return FALSE;

   dst->DstRegister.File = file;
   dst->DstRegister.Index = index;
   dst->DstRegister.WriteMask = writemask;
   return TRUE;
}

static boolean
parse_optional_swizzle(
   struct translate_ctx *ctx,
   uint swizzle[4],
   boolean *parsed_swizzle,
   boolean *parsed_extswizzle )
{
   const char *cur = ctx->cur;

   *parsed_swizzle = FALSE;
   *parsed_extswizzle = FALSE;

   eat_opt_white( &cur );
   if (*cur == '.') {
      uint i;

      cur++;
      eat_opt_white( &cur );
      for (i = 0; i < 4; i++) {
         if (uprcase( *cur ) == 'X')
            swizzle[i] = TGSI_SWIZZLE_X;
         else if (uprcase( *cur ) == 'Y')
            swizzle[i] = TGSI_SWIZZLE_Y;
         else if (uprcase( *cur ) == 'Z')
            swizzle[i] = TGSI_SWIZZLE_Z;
         else if (uprcase( *cur ) == 'W')
            swizzle[i] = TGSI_SWIZZLE_W;
         else {
            if (*cur == '0')
               swizzle[i] = TGSI_EXTSWIZZLE_ZERO;
            else if (*cur == '1')
               swizzle[i] = TGSI_EXTSWIZZLE_ONE;
            else {
               report_error( ctx, "Expected register swizzle component `x', `y', `z', `w', `0' or `1'" );
               return FALSE;
            }
            *parsed_extswizzle = TRUE;
         }
         cur++;
      }
      *parsed_swizzle = TRUE;
      ctx->cur = cur;
   }
   return TRUE;
}

static boolean
parse_src_operand(
   struct translate_ctx *ctx,
   struct tgsi_full_src_register *src )
{
   const char *cur;
   float value;
   uint file;
   int index;
   uint ind_file;
   int ind_index;
   uint swizzle[4];
   boolean parsed_swizzle;
   boolean parsed_extswizzle;

   if (*ctx->cur == '-') {
      cur = ctx->cur;
      cur++;
      eat_opt_white( &cur );
      if (*cur == '(') {
         cur++;
         src->SrcRegisterExtMod.Negate = 1;
         eat_opt_white( &cur );
         ctx->cur = cur;
      }
   }

   if (*ctx->cur == '|') {
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      src->SrcRegisterExtMod.Absolute = 1;
   }

   if (*ctx->cur == '-') {
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      src->SrcRegister.Negate = 1;
   }

   cur = ctx->cur;
   if (parse_float( &cur, &value )) {
      if (value == 2.0f) {
         eat_opt_white( &cur );
         if (*cur != '*') {
            report_error( ctx, "Expected `*'" );
            return FALSE;
         }
         cur++;
         if (*cur != '(') {
            report_error( ctx, "Expected `('" );
            return FALSE;
         }
         cur++;
         src->SrcRegisterExtMod.Scale2X = 1;
         eat_opt_white( &cur );
         ctx->cur = cur;
      }
   }

   if (*ctx->cur == '(') {
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      src->SrcRegisterExtMod.Bias = 1;
   }

   cur = ctx->cur;
   if (parse_float( &cur, &value )) {
      if (value == 1.0f) {
         eat_opt_white( &cur );
         if (*cur != '-') {
            report_error( ctx, "Expected `-'" );
            return FALSE;
         }
         cur++;
         if (*cur != '(') {
            report_error( ctx, "Expected `('" );
            return FALSE;
         }
         cur++;
         src->SrcRegisterExtMod.Complement = 1;
         eat_opt_white( &cur );
         ctx->cur = cur;
      }
   }

   if (!parse_register_src( ctx, &file, &index, &ind_file, &ind_index ))
      return FALSE;
   src->SrcRegister.File = file;
   src->SrcRegister.Index = index;
   if (ind_file != TGSI_FILE_NULL) {
      src->SrcRegister.Indirect = 1;
      src->SrcRegisterInd.File = ind_file;
      src->SrcRegisterInd.Index = ind_index;
   }

   /* Parse optional swizzle.
    */
   if (parse_optional_swizzle( ctx, swizzle, &parsed_swizzle, &parsed_extswizzle )) {
      if (parsed_extswizzle) {
         assert( parsed_swizzle );

         src->SrcRegisterExtSwz.ExtSwizzleX = swizzle[0];
         src->SrcRegisterExtSwz.ExtSwizzleY = swizzle[1];
         src->SrcRegisterExtSwz.ExtSwizzleZ = swizzle[2];
         src->SrcRegisterExtSwz.ExtSwizzleW = swizzle[3];
      }
      else if (parsed_swizzle) {
         src->SrcRegister.SwizzleX = swizzle[0];
         src->SrcRegister.SwizzleY = swizzle[1];
         src->SrcRegister.SwizzleZ = swizzle[2];
         src->SrcRegister.SwizzleW = swizzle[3];
      }
   }

   if (src->SrcRegisterExtMod.Complement) {
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != ')') {
         report_error( ctx, "Expected `)'" );
         return FALSE;
      }
      ctx->cur++;
   }

   if (src->SrcRegisterExtMod.Bias) {
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != ')') {
         report_error( ctx, "Expected `)'" );
         return FALSE;
      }
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != '-') {
         report_error( ctx, "Expected `-'" );
         return FALSE;
      }
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      if (!parse_float( &ctx->cur, &value )) {
         report_error( ctx, "Expected literal floating point" );
         return FALSE;
      }
      if (value != 0.5f) {
         report_error( ctx, "Expected 0.5" );
         return FALSE;
      }
   }

   if (src->SrcRegisterExtMod.Scale2X) {
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != ')') {
         report_error( ctx, "Expected `)'" );
         return FALSE;
      }
      ctx->cur++;
   }

   if (src->SrcRegisterExtMod.Absolute) {
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != '|') {
         report_error( ctx, "Expected `|'" );
         return FALSE;
      }
      ctx->cur++;
   }

   if (src->SrcRegisterExtMod.Negate) {
      eat_opt_white( &ctx->cur );
      if (*ctx->cur != ')') {
         report_error( ctx, "Expected `)'" );
         return FALSE;
      }
      ctx->cur++;
   }

   return TRUE;
}

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

static boolean
parse_instruction(
   struct translate_ctx *ctx,
   boolean has_label )
{
   uint i;
   uint saturate = TGSI_SAT_NONE;
   const struct tgsi_opcode_info *info;
   struct tgsi_full_instruction inst;
   uint advance;

   /* Parse instruction name.
    */
   eat_opt_white( &ctx->cur );
   for (i = 0; i < TGSI_OPCODE_LAST; i++) {
      const char *cur = ctx->cur;

      info = tgsi_get_opcode_info( i );
      if (str_match_no_case( &cur, info->mnemonic )) {
         if (str_match_no_case( &cur, "_SATNV" ))
            saturate = TGSI_SAT_MINUS_PLUS_ONE;
         else if (str_match_no_case( &cur, "_SAT" ))
            saturate = TGSI_SAT_ZERO_ONE;

         if (info->num_dst + info->num_src + info->is_tex == 0) {
            if (!is_digit_alpha_underscore( cur )) {
               ctx->cur = cur;
               break;
            }
         }
         else if (*cur == '\0' || eat_white( &cur )) {
            ctx->cur = cur;
            break;
         }
      }
   }
   if (i == TGSI_OPCODE_LAST) {
      if (has_label)
         report_error( ctx, "Unknown opcode" );
      else
         report_error( ctx, "Expected `DCL', `IMM' or a label" );
      return FALSE;
   }

   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = i;
   inst.Instruction.Saturate = saturate;
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

   if (info->is_branch) {
      uint target;

      eat_opt_white( &ctx->cur );
      if (*ctx->cur != ':') {
         report_error( ctx, "Expected `:'" );
         return FALSE;
      }
      ctx->cur++;
      eat_opt_white( &ctx->cur );
      if (!parse_uint( &ctx->cur, &target )) {
         report_error( ctx, "Expected a label" );
         return FALSE;
      }
      inst.InstructionExtLabel.Label = target;
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
   int first;
   int last;
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

static boolean parse_immediate( struct translate_ctx *ctx )
{
   struct tgsi_full_immediate imm;
   uint i;
   float values[4];
   uint advance;

   if (!eat_white( &ctx->cur )) {
      report_error( ctx, "Syntax error" );
      return FALSE;
   }
   if (!str_match_no_case( &ctx->cur, "FLT32" ) || is_digit_alpha_underscore( ctx->cur )) {
      report_error( ctx, "Expected `FLT32'" );
      return FALSE;
   }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '{') {
      report_error( ctx, "Expected `{'" );
      return FALSE;
   }
   ctx->cur++;
   for (i = 0; i < 4; i++) {
      eat_opt_white( &ctx->cur );
      if (i > 0) {
         if (*ctx->cur != ',') {
            report_error( ctx, "Expected `,'" );
            return FALSE;
         }
         ctx->cur++;
         eat_opt_white( &ctx->cur );
      }
      if (!parse_float( &ctx->cur, &values[i] )) {
         report_error( ctx, "Expected literal floating point" );
         return FALSE;
      }
   }
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != '}') {
      report_error( ctx, "Expected `}'" );
      return FALSE;
   }
   ctx->cur++;

   imm = tgsi_default_full_immediate();
   imm.Immediate.Size += 4;
   imm.Immediate.DataType = TGSI_IMM_FLOAT32;
   imm.u.Pointer = values;

   advance = tgsi_build_full_immediate(
      &imm,
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

      if (*ctx->cur == '\0')
         break;

      if (parse_label( ctx, &label_val )) {
         if (!parse_instruction( ctx, TRUE ))
            return FALSE;
      }
      else if (str_match_no_case( &ctx->cur, "DCL" )) {
         if (!parse_declaration( ctx ))
            return FALSE;
      }
      else if (str_match_no_case( &ctx->cur, "IMM" )) {
         if (!parse_immediate( ctx ))
            return FALSE;
      }
      else if (!parse_instruction( ctx, FALSE )) {
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

   if (!translate( &ctx ))
      return FALSE;

   return tgsi_sanity_check( tokens );
}
