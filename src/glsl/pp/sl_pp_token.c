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
#include "sl_pp_context.h"
#include "sl_pp_token.h"


static int
_is_identifier_char(char c)
{
   return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}


static int
_tokenise_identifier(struct sl_pp_context *context,
                     const char **pinput,
                     struct sl_pp_token_info *info)
{
   const char *input = *pinput;
   char identifier[256];   /* XXX: Remove this artifical limit. */
   unsigned int i = 0;

   info->token = SL_PP_IDENTIFIER;
   info->data.identifier = -1;

   identifier[i++] = *input++;
   while (_is_identifier_char(*input)) {
      if (i >= sizeof(identifier) - 1) {
         strcpy(context->error_msg, "out of memory");
         return -1;
      }
      identifier[i++] = *input++;
   }
   identifier[i++] = '\0';

   info->data.identifier = sl_pp_context_add_unique_str(context, identifier);
   if (info->data.identifier == -1) {
      return -1;
   }

   *pinput = input;
   return 0;
}


/*
 * Return the number of consecutive decimal digits in the input stream.
 */
static unsigned int
_parse_float_digits(const char *input)
{
   unsigned int eaten = 0;

   while (input[eaten] >= '0' && input[eaten] <= '9') {
      eaten++;
   }
   return eaten;
}


/*
 * Try to match one of the following patterns for the fractional part
 * of a floating point number.
 *
 * digits . [digits]
 * . digits
 *
 * Return 0 if the pattern could not be matched, otherwise the number
 * of eaten characters from the input stream.
 */
static unsigned int
_parse_float_frac(const char *input)
{
   unsigned int eaten;

   if (input[0] == '.') {
      eaten = _parse_float_digits(&input[1]);
      if (eaten) {
         return eaten + 1;
      }
      return 0;
   }

   eaten = _parse_float_digits(input);
   if (eaten && input[eaten] == '.') {
      unsigned int trailing;

      trailing = _parse_float_digits(&input[eaten + 1]);
      if (trailing) {
         return eaten + trailing + 1;
      }
      return eaten + 1;
   }

   return 0;
}


/*
 * Try to match the following pattern for the exponential part
 * of a floating point number.
 *
 * (e|E) [(+|-)] digits
 *
 * Return 0 if the pattern could not be matched, otherwise the number
 * of eaten characters from the input stream.
 */
static unsigned int
_parse_float_exp(const char *input)
{
   unsigned int eaten, digits;

   if (input[0] != 'e' && input[0] != 'E') {
      return 0;
   }

   if (input[1] == '-' || input[1] == '+') {
      eaten = 2;
   } else {
      eaten = 1;
   }

   digits = _parse_float_digits(&input[eaten]);
   if (!digits) {
      return 0;
   }

   return eaten + digits;
}


/*
 * Try to match one of the following patterns for a floating point number.
 *
 * fract [exp] [(f|F)]
 * digits exp [(f|F)]
 *
 * Return 0 if the pattern could not be matched, otherwise the number
 * of eaten characters from the input stream.
 */
static unsigned int
_parse_float(const char *input)
{
   unsigned int eaten;

   eaten = _parse_float_frac(input);
   if (eaten) {
      unsigned int exponent;

      exponent = _parse_float_exp(&input[eaten]);
      if (exponent) {
         eaten += exponent;
      }

      if (input[eaten] == 'f' || input[eaten] == 'F') {
         eaten++;
      }

      return eaten;
   }

   eaten = _parse_float_digits(input);
   if (eaten) {
      unsigned int exponent;

      exponent = _parse_float_exp(&input[eaten]);
      if (exponent) {
         eaten += exponent;

         if (input[eaten] == 'f' || input[eaten] == 'F') {
            eaten++;
         }

         return eaten;
      }
   }

   return 0;
}


static unsigned int
_parse_hex(const char *input)
{
   unsigned int n;

   if (input[0] != '0') {
      return 0;
   }

   if (input[1] != 'x' && input[1] != 'X') {
      return 0;
   }

   n = 2;
   while ((input[n] >= '0' && input[n] <= '9') ||
          (input[n] >= 'a' && input[n] <= 'f') ||
          (input[n] >= 'A' && input[n] <= 'F')) {
      n++;
   }

   if (n > 2) {
      return n;
   }

   return 0;
}


static unsigned int
_parse_oct(const char *input)
{
   unsigned int n;

   if (input[0] != '0') {
      return 0;
   }

   n = 1;
   while ((input[n] >= '0' && input[n] <= '7')) {
      n++;
   }

   return n;
}


static unsigned int
_parse_dec(const char *input)
{
   unsigned int n = 0;

   while ((input[n] >= '0' && input[n] <= '9')) {
      n++;
   }

   return n;
}


static int
_tokenise_number(struct sl_pp_context *context,
                 const char **pinput,
                 struct sl_pp_token_info *info)
{
   const char *input = *pinput;
   unsigned int eaten;
   char number[256];   /* XXX: Remove this artifical limit. */

   eaten = _parse_float(input);
   if (!eaten) {
      eaten = _parse_hex(input);
      if (!eaten) {
         eaten = _parse_oct(input);
         if (!eaten) {
            eaten = _parse_dec(input);
         }
      }
   }

   if (!eaten || _is_identifier_char(input[eaten])) {
      strcpy(context->error_msg, "expected a number");
      return -1;
   }

   if (eaten > sizeof(number) - 1) {
      strcpy(context->error_msg, "out of memory");
      return -1;
   }

   memcpy(number, input, eaten);
   number[eaten] = '\0';

   info->token = SL_PP_NUMBER;
   info->data.number = sl_pp_context_add_unique_str(context, number);
   if (info->data.number == -1) {
      return -1;
   }

   *pinput = input + eaten;
   return 0;
}


int
sl_pp_tokenise(struct sl_pp_context *context,
               const char *input,
               struct sl_pp_token_info **output)
{
   struct sl_pp_token_info *out = NULL;
   unsigned int out_len = 0;
   unsigned int out_max = 0;

   for (;;) {
      struct sl_pp_token_info info;

      switch (*input) {
      case ' ':
      case '\t':
         input++;
         info.token = SL_PP_WHITESPACE;
         break;

      case '\n':
         input++;
         info.token = SL_PP_NEWLINE;
         break;

      case '#':
         input++;
         info.token = SL_PP_HASH;
         break;

      case ',':
         input++;
         info.token = SL_PP_COMMA;
         break;

      case ';':
         input++;
         info.token = SL_PP_SEMICOLON;
         break;

      case '{':
         input++;
         info.token = SL_PP_LBRACE;
         break;

      case '}':
         input++;
         info.token = SL_PP_RBRACE;
         break;

      case '(':
         input++;
         info.token = SL_PP_LPAREN;
         break;

      case ')':
         input++;
         info.token = SL_PP_RPAREN;
         break;

      case '[':
         input++;
         info.token = SL_PP_LBRACKET;
         break;

      case ']':
         input++;
         info.token = SL_PP_RBRACKET;
         break;

      case '.':
         if (input[1] >= '0' && input[1] <= '9') {
            if (_tokenise_number(context, &input, &info)) {
               free(out);
               return -1;
            }
         } else {
            input++;
            info.token = SL_PP_DOT;
         }
         break;

      case '+':
         input++;
         if (*input == '+') {
            input++;
            info.token = SL_PP_INCREMENT;
         } else if (*input == '=') {
            input++;
            info.token = SL_PP_ADDASSIGN;
         } else {
            info.token = SL_PP_PLUS;
         }
         break;

      case '-':
         input++;
         if (*input == '-') {
            input++;
            info.token = SL_PP_DECREMENT;
         } else if (*input == '=') {
            input++;
            info.token = SL_PP_SUBASSIGN;
         } else {
            info.token = SL_PP_MINUS;
         }
         break;

      case '~':
         input++;
         info.token = SL_PP_BITNOT;
         break;

      case '!':
         input++;
         if (*input == '=') {
            input++;
            info.token = SL_PP_NOTEQUAL;
         } else {
            info.token = SL_PP_NOT;
         }
         break;

      case '*':
         input++;
         if (*input == '=') {
            input++;
            info.token = SL_PP_MULASSIGN;
         } else {
            info.token = SL_PP_STAR;
         }
         break;

      case '/':
         input++;
         if (*input == '=') {
            input++;
            info.token = SL_PP_DIVASSIGN;
         } else {
            info.token = SL_PP_SLASH;
         }
         break;

      case '%':
         input++;
         if (*input == '=') {
            input++;
            info.token = SL_PP_MODASSIGN;
         } else {
            info.token = SL_PP_MODULO;
         }
         break;

      case '<':
         input++;
         if (*input == '<') {
            input++;
            if (*input == '=') {
               input++;
               info.token = SL_PP_LSHIFTASSIGN;
            } else {
               info.token = SL_PP_LSHIFT;
            }
         } else if (*input == '=') {
            input++;
            info.token = SL_PP_LESSEQUAL;
         } else {
            info.token = SL_PP_LESS;
         }
         break;

      case '>':
         input++;
         if (*input == '>') {
            input++;
            if (*input == '=') {
               input++;
               info.token = SL_PP_RSHIFTASSIGN;
            } else {
               info.token = SL_PP_RSHIFT;
            }
         } else if (*input == '=') {
            input++;
            info.token = SL_PP_GREATEREQUAL;
         } else {
            info.token = SL_PP_GREATER;
         }
         break;

      case '=':
         input++;
         if (*input == '=') {
            input++;
            info.token = SL_PP_EQUAL;
         } else {
            info.token = SL_PP_ASSIGN;
         }
         break;

      case '&':
         input++;
         if (*input == '&') {
            input++;
            info.token = SL_PP_AND;
         } else if (*input == '=') {
            input++;
            info.token = SL_PP_BITANDASSIGN;
         } else {
            info.token = SL_PP_BITAND;
         }
         break;

      case '^':
         input++;
         if (*input == '^') {
            input++;
            info.token = SL_PP_XOR;
         } else if (*input == '=') {
            input++;
            info.token = SL_PP_BITXORASSIGN;
         } else {
            info.token = SL_PP_BITXOR;
         }
         break;

      case '|':
         input++;
         if (*input == '|') {
            input++;
            info.token = SL_PP_OR;
         } else if (*input == '=') {
            input++;
            info.token = SL_PP_BITORASSIGN;
         } else {
            info.token = SL_PP_BITOR;
         }
         break;

      case '?':
         input++;
         info.token = SL_PP_QUESTION;
         break;

      case ':':
         input++;
         info.token = SL_PP_COLON;
         break;

      case '\0':
         info.token = SL_PP_EOF;
         break;

      default:
         if ((*input >= 'a' && *input <= 'z') ||
             (*input >= 'A' && *input <= 'Z') ||
             (*input == '_')) {
            if (_tokenise_identifier(context, &input, &info)) {
               free(out);
               return -1;
            }
         } else if (*input >= '0' && *input <= '9') {
            if (_tokenise_number(context, &input, &info)) {
               free(out);
               return -1;
            }
         } else {
            info.data.other = *input++;
            info.token = SL_PP_OTHER;
         }
      }

      if (out_len >= out_max) {
         unsigned int new_max = out_max;

         if (new_max < 0x100) {
            new_max = 0x100;
         } else if (new_max < 0x10000) {
            new_max *= 2;
         } else {
            new_max += 0x10000;
         }

         out = realloc(out, new_max * sizeof(struct sl_pp_token_info));
         if (!out) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }
         out_max = new_max;
      }

      out[out_len++] = info;

      if (info.token == SL_PP_EOF) {
         break;
      }
   }

   *output = out;
   return 0;
}
