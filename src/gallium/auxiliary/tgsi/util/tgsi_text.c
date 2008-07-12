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

   if (ctx->cur[0] == 'F' && ctx->cur[1] == 'R' && ctx->cur[2] == 'A' && ctx->cur[3] == 'G') {
      ctx->cur += 4;
      processor = TGSI_PROCESSOR_FRAGMENT;
   }
   else if (ctx->cur[0] == 'V' && ctx->cur[1] == 'E' && ctx->cur[2] == 'R' && ctx->cur[3] == 'T') {
      ctx->cur += 4;
      processor = TGSI_PROCESSOR_VERTEX;
   }
   else if (ctx->cur[0] == 'G' && ctx->cur[1] == 'E' && ctx->cur[2] == 'O' && ctx->cur[3] == 'M') {
      ctx->cur += 4;
      processor = TGSI_PROCESSOR_GEOMETRY;
   }
   else {
      report_error( ctx, "Unknown processor type" );
      return FALSE;
   }

   if (ctx->cur[0] == '1' && ctx->cur[1] == '.' && ctx->cur[2] == '1') {
      ctx->cur += 3;
   }
   else {
      report_error( ctx, "Unknown version" );
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
      const char *name = file_names[i];

      while (*name != '\0' && *name == toupper( *cur )) {
         name++;
         cur++;
      }
      if (*name == '\0' && !is_digit_alpha_underscore( cur )) {
         ctx->cur = cur;
         *file = i;
         return TRUE;
      }
   }
   report_error( ctx, "Unknown register file" );
   return FALSE;
}

static boolean
parse_register(
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
   eat_opt_white( &ctx->cur );
   if (*ctx->cur != ']') {
      report_error( ctx, "Expected `]'" );
      return FALSE;
   }
   ctx->cur++;
   /* TODO: Modulate suffix */
   return TRUE;
}

static boolean
parse_dst_operand(
   struct translate_ctx *ctx,
   struct tgsi_full_dst_register *dst )
{
   const char *cur;
   uint file;
   uint index;

   if (!parse_register( ctx, &file, &index ))
      return FALSE;
   dst->DstRegister.File = file;
   dst->DstRegister.Index = index;

   /* Parse optional write mask.
    */
   cur = ctx->cur;
   eat_opt_white( &cur );
   if (*cur == '.') {
      uint writemask = TGSI_WRITEMASK_NONE;

      cur++;
      eat_opt_white( &cur );
      if (toupper( *cur ) == 'X') {
         cur++;
         writemask |= TGSI_WRITEMASK_X;
      }
      if (toupper( *cur ) == 'Y') {
         cur++;
         writemask |= TGSI_WRITEMASK_Y;
      }
      if (toupper( *cur ) == 'Z') {
         cur++;
         writemask |= TGSI_WRITEMASK_Z;
      }
      if (toupper( *cur ) == 'W') {
         cur++;
         writemask |= TGSI_WRITEMASK_W;
      }

      if (writemask == TGSI_WRITEMASK_NONE) {
         report_error( ctx, "Writemask expected" );
         return FALSE;
      }

      dst->DstRegister.WriteMask = writemask;
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
   const char *mnemonic;
};

static const struct opcode_info opcode_info[TGSI_OPCODE_LAST] =
{
{ 1, 1, "ARL" },
   { 1, 1, "MOV" },
{ 1, 1, "LIT" },
   { 1, 1, "RCP" },
   { 1, 1, "RSQ" },
   { 1, 1, "EXP" },
   { 1, 1, "LOG" },
   { 1, 2, "MUL" },
   { 1, 2, "ADD" },
   { 1, 2, "DP3" },
   { 1, 2, "DP4" },
{ 1, 2, "DST" },
   { 1, 2, "MIN" },
   { 1, 2, "MAX" },
{ 1, 2, "SLT" },
{ 1, 2, "SGE" },
   { 1, 3, "MAD" },
   { 1, 2, "SUB" },
{ 1, 3, "LERP" },
{ 1, 2, "CND" },
{ 1, 2, "CND0" },
{ 1, 2, "DOT2ADD" },
{ 1, 2, "INDEX" },
{ 1, 2, "NEGATE" },
{ 1, 2, "FRAC" },
{ 1, 2, "CLAMP" },
{ 1, 2, "FLOOR" },
{ 1, 2, "ROUND" },
{ 1, 2, "EXPBASE2" },
{ 1, 2, "LOGBASE2" },
{ 1, 2, "POWER" },
{ 1, 2, "CROSSPRODUCT" },
{ 1, 2, "MULTIPLYMATRIX" },
   { 1, 2, "ABS" },
{ 1, 2, "RCC" },
{ 1, 2, "DPH" },
{ 1, 2, "COS" },
{ 1, 2, "DDX" },
{ 1, 2, "DDY" },
{ 1, 2, "KILP" },
{ 1, 2, "PK2H" },
{ 1, 2, "PK2US" },
{ 1, 2, "PK4B" },
{ 1, 2, "PK4UB" },
{ 1, 2, "RFL" },
{ 1, 2, "SEQ" },
{ 1, 2, "SFL" },
{ 1, 2, "SGT" },
{ 1, 2, "SIN" },
{ 1, 2, "SLE" },
{ 1, 2, "SNE" },
{ 1, 2, "STR" },
{ 1, 2, "TEX" },
{ 1, 2, "TXD" },
{ 1, 2, "TXP" },
{ 1, 2, "UP2H" },
{ 1, 2, "UP2US" },
{ 1, 2, "UP4B" },
{ 1, 2, "UP4UB" },
{ 1, 2, "X2D" },
{ 1, 2, "ARA" },
{ 1, 2, "ARR" },
{ 1, 2, "BRA" },
{ 1, 2, "CAL" },
{ 1, 2, "RET" },
{ 1, 2, "SSG" },
{ 1, 2, "CMP" },
{ 1, 2, "SCS" },
{ 1, 2, "TXB" },
{ 1, 2, "NRM" },
{ 1, 2, "DIV" },
{ 1, 2, "DP2" },
{ 1, 2, "TXL" },
{ 1, 2, "BRK" },
{ 1, 2, "IF" },
{ 1, 2, "LOOP" },
{ 1, 2, "REP" },
{ 1, 2, "ELSE" },
{ 1, 2, "ENDIF" },
{ 1, 2, "ENDLOOP" },
{ 1, 2, "ENDREP" },
{ 1, 2, "PUSHA" },
{ 1, 2, "POPA" },
{ 1, 2, "CEIL" },
{ 1, 2, "I2F" },
{ 1, 2, "NOT" },
{ 1, 2, "TRUNC" },
{ 1, 2, "SHL" },
{ 1, 2, "SHR" },
{ 1, 2, "AND" },
{ 1, 2, "OR" },
{ 1, 2, "MOD" },
{ 1, 2, "XOR" },
{ 1, 2, "SAD" },
{ 1, 2, "TXF" },
{ 1, 2, "TXQ" },
{ 1, 2, "CONT" },
{ 1, 2, "EMIT" },
{ 1, 2, "ENDPRIM" },
{ 1, 2, "BGNLOOP2" },
{ 1, 2, "BGNSUB" },
{ 1, 2, "ENDLOOP2" },
{ 1, 2, "ENDSUB" },
{ 1, 2, "NOISE1" },
{ 1, 2, "NOISE2" },
{ 1, 2, "NOISE3" },
{ 1, 2, "NOISE4" },
{ 1, 2, "NOP" },
{ 1, 2, "M4X3" },
{ 1, 2, "M3X4" },
{ 1, 2, "M3X3" },
{ 1, 2, "M3X2" },
{ 1, 2, "NRM4" },
{ 1, 2, "CALLNZ" },
{ 1, 2, "IFC" },
{ 1, 2, "BREAKC" },
{ 1, 2, "KIL" },
   { 0, 0, "END" }
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
      const char *op = opcode_info[i].mnemonic;

      while (*op != '\0' && *op == toupper( *cur )) {
         op++;
         cur++;
      }
      if (*op == '\0') {
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
   for (i = 0; i < info->num_dst + info->num_src; i++) {
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
      else {
         if (!parse_src_operand( ctx, &inst.FullSrcRegisters[i - info->num_dst] ))
            return FALSE;
      }
   }
   eat_opt_white( &ctx->cur );

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

static boolean translate( struct translate_ctx *ctx )
{
   eat_opt_white( &ctx->cur );
   if (!parse_header( ctx ))
      return FALSE;

   eat_white( &ctx->cur );
   while (*ctx->cur != '\0') {
      uint label_val = 0;

      if (parse_label( ctx, &label_val )) {
         if (!parse_instruction( ctx ))
            return FALSE;
      }
      else {
         report_error( ctx, "Instruction expected" );
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
