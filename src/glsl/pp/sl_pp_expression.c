/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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

#include <stdlib.h>
#include <string.h>
#include "sl_pp_expression.h"
#include "sl_pp_public.h"


struct parse_context {
   struct sl_pp_context *context;
   const struct sl_pp_token_info *input;
};

static int
_parse_or(struct parse_context *ctx,
          int *result);

static int
_parse_primary(struct parse_context *ctx,
               int *result)
{
   if (ctx->input->token == SL_PP_UINT) {
      *result = atoi(sl_pp_context_cstr(ctx->context, ctx->input->data._uint));
      ctx->input++;
   } else {
      if (ctx->input->token != SL_PP_LPAREN) {
         strcpy(ctx->context->error_msg, "expected `('");
         return -1;
      }
      ctx->input++;
      if (_parse_or(ctx, result)) {
         return -1;
      }
      if (ctx->input->token != SL_PP_RPAREN) {
         strcpy(ctx->context->error_msg, "expected `)'");
         return -1;
      }
      ctx->input++;
   }
   return 0;
}

static int
_parse_unary(struct parse_context *ctx,
             int *result)
{
   if (!_parse_primary(ctx, result)) {
      return 0;
   }

   switch (ctx->input->token) {
   case SL_PP_PLUS:
      ctx->input++;
      if (_parse_unary(ctx, result)) {
         return -1;
      }
      *result = +*result;
      break;

   case SL_PP_MINUS:
      ctx->input++;
      if (_parse_unary(ctx, result)) {
         return -1;
      }
      *result = -*result;
      break;

   case SL_PP_NOT:
      ctx->input++;
      if (_parse_unary(ctx, result)) {
         return -1;
      }
      *result = !*result;
      break;

   case SL_PP_BITNOT:
      ctx->input++;
      if (_parse_unary(ctx, result)) {
         return -1;
      }
      *result = ~*result;
      break;

   default:
      return -1;
   }

   return 0;
}

static int
_parse_multiplicative(struct parse_context *ctx,
                      int *result)
{
   if (_parse_unary(ctx, result)) {
      return -1;
   }
   for (;;) {
      int right;

      switch (ctx->input->token) {
      case SL_PP_STAR:
         ctx->input++;
         if (_parse_unary(ctx, &right)) {
            return -1;
         }
         *result = *result * right;
         break;

      case SL_PP_SLASH:
         ctx->input++;
         if (_parse_unary(ctx, &right)) {
            return -1;
         }
         *result = *result / right;
         break;

      case SL_PP_MODULO:
         ctx->input++;
         if (_parse_unary(ctx, &right)) {
            return -1;
         }
         *result = *result % right;
         break;

      default:
         return 0;
      }
   }
}

static int
_parse_additive(struct parse_context *ctx,
                int *result)
{
   if (_parse_multiplicative(ctx, result)) {
      return -1;
   }
   for (;;) {
      int right;

      switch (ctx->input->token) {
      case SL_PP_PLUS:
         ctx->input++;
         if (_parse_multiplicative(ctx, &right)) {
            return -1;
         }
         *result = *result + right;
         break;

      case SL_PP_MINUS:
         ctx->input++;
         if (_parse_multiplicative(ctx, &right)) {
            return -1;
         }
         *result = *result - right;
         break;

      default:
         return 0;
      }
   }
}

static int
_parse_shift(struct parse_context *ctx,
             int *result)
{
   if (_parse_additive(ctx, result)) {
      return -1;
   }
   for (;;) {
      int right;

      switch (ctx->input->token) {
      case SL_PP_LSHIFT:
         ctx->input++;
         if (_parse_additive(ctx, &right)) {
            return -1;
         }
         *result = *result << right;
         break;

      case SL_PP_RSHIFT:
         ctx->input++;
         if (_parse_additive(ctx, &right)) {
            return -1;
         }
         *result = *result >> right;
         break;

      default:
         return 0;
      }
   }
}

static int
_parse_relational(struct parse_context *ctx,
                  int *result)
{
   if (_parse_shift(ctx, result)) {
      return -1;
   }
   for (;;) {
      int right;

      switch (ctx->input->token) {
      case SL_PP_LESSEQUAL:
         ctx->input++;
         if (_parse_shift(ctx, &right)) {
            return -1;
         }
         *result = *result <= right;
         break;

      case SL_PP_GREATEREQUAL:
         ctx->input++;
         if (_parse_shift(ctx, &right)) {
            return -1;
         }
         *result = *result >= right;
         break;

      case SL_PP_LESS:
         ctx->input++;
         if (_parse_shift(ctx, &right)) {
            return -1;
         }
         *result = *result < right;
         break;

      case SL_PP_GREATER:
         ctx->input++;
         if (_parse_shift(ctx, &right)) {
            return -1;
         }
         *result = *result > right;
         break;

      default:
         return 0;
      }
   }
}

static int
_parse_equality(struct parse_context *ctx,
                int *result)
{
   if (_parse_relational(ctx, result)) {
      return -1;
   }
   for (;;) {
      int right;

      switch (ctx->input->token) {
      case SL_PP_EQUAL:
         ctx->input++;
         if (_parse_relational(ctx, &right)) {
            return -1;
         }
         *result = *result == right;
         break;

      case SL_PP_NOTEQUAL:
         ctx->input++;
         if (_parse_relational(ctx, &right)) {
            return -1;
         }
         *result = *result != right;
         break;

      default:
         return 0;
      }
   }
}

static int
_parse_bitand(struct parse_context *ctx,
              int *result)
{
   if (_parse_equality(ctx, result)) {
      return -1;
   }
   while (ctx->input->token == SL_PP_BITAND) {
      int right;

      ctx->input++;
      if (_parse_equality(ctx, &right)) {
         return -1;
      }
      *result = *result & right;
   }
   return 0;
}

static int
_parse_xor(struct parse_context *ctx,
           int *result)
{
   if (_parse_bitand(ctx, result)) {
      return -1;
   }
   while (ctx->input->token == SL_PP_XOR) {
      int right;

      ctx->input++;
      if (_parse_bitand(ctx, &right)) {
         return -1;
      }
      *result = *result ^ right;
   }
   return 0;
}

static int
_parse_bitor(struct parse_context *ctx,
             int *result)
{
   if (_parse_xor(ctx, result)) {
      return -1;
   }
   while (ctx->input->token == SL_PP_BITOR) {
      int right;

      ctx->input++;
      if (_parse_xor(ctx, &right)) {
         return -1;
      }
      *result = *result | right;
   }
   return 0;
}

static int
_parse_and(struct parse_context *ctx,
           int *result)
{
   if (_parse_bitor(ctx, result)) {
      return -1;
   }
   while (ctx->input->token == SL_PP_AND) {
      int right;

      ctx->input++;
      if (_parse_bitor(ctx, &right)) {
         return -1;
      }
      *result = *result && right;
   }
   return 0;
}

static int
_parse_or(struct parse_context *ctx,
          int *result)
{
   if (_parse_and(ctx, result)) {
      return -1;
   }
   while (ctx->input->token == SL_PP_OR) {
      int right;

      ctx->input++;
      if (_parse_and(ctx, &right)) {
         return -1;
      }
      *result = *result || right;
   }
   return 0;
}

int
sl_pp_execute_expression(struct sl_pp_context *context,
                         const struct sl_pp_token_info *input,
                         int *result)
{
   struct parse_context ctx;

   ctx.context = context;
   ctx.input = input;

   return _parse_or(&ctx, result);
}
