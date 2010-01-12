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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "sl_pp_public.h"
#include "sl_pp_context.h"
#include "sl_pp_token.h"


#define PURE_ERROR 256

static int
_pure_getc(struct sl_pp_context *context)
{
   char c;

   if (context->getc_buf_size) {
      return context->getc_buf[--context->getc_buf_size];
   }

   if (sl_pp_purify_getc(&context->pure, &c, &context->error_line, context->error_msg, sizeof(context->error_msg)) == 0) {
      return PURE_ERROR;
   }
   return c;
}


static void
_pure_ungetc(struct sl_pp_context *context,
             int c)
{
   assert(c != PURE_ERROR);

   if (context->getc_buf_size == context->getc_buf_capacity) {
      context->getc_buf_capacity += 64;
      context->getc_buf = realloc(context->getc_buf, context->getc_buf_capacity * sizeof(char));
      assert(context->getc_buf);                            
   }

   context->getc_buf[context->getc_buf_size++] = (char)c;
}


struct lookahead_state {
   char buf[256];
   unsigned int pos;
   struct sl_pp_context *context;
};


static void
_lookahead_init(struct lookahead_state *lookahead,
                struct sl_pp_context *context)
{
   lookahead->pos = 0;
   lookahead->context = context;
}


static unsigned int
_lookahead_tell(const struct lookahead_state *lookahead)
{
   return lookahead->pos;
}


static const void *
_lookahead_buf(const struct lookahead_state *lookahead)
{
   return lookahead->buf;
}


static void
_lookahead_revert(struct lookahead_state *lookahead,
                  unsigned int pos)
{
   assert(pos <= lookahead->pos);

   while (lookahead->pos > pos) {
      _pure_ungetc(lookahead->context, lookahead->buf[--lookahead->pos]);
   }
}


static int
_lookahead_getc(struct lookahead_state *lookahead)
{
   int c;

   assert(lookahead->pos < sizeof(lookahead->buf) / sizeof(lookahead->buf[0]));

   c = _pure_getc(lookahead->context);
   if (c != PURE_ERROR) {
      lookahead->buf[lookahead->pos++] = (char)c;
   }
   return c;
}


static int
_is_identifier_char(char c)
{
   return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}


static int
_tokenise_identifier(struct sl_pp_context *context,
                     struct sl_pp_token_info *out)
{
   int c;
   char identifier[256];   /* XXX: Remove this artifical limit. */
   unsigned int i = 0;

   out->token = SL_PP_IDENTIFIER;
   out->data.identifier = -1;

   c = _pure_getc(context);
   if (c == PURE_ERROR) {
      return -1;
   }
   identifier[i++] = (char)c;
   for (;;) {
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }

      if (_is_identifier_char((char)c)) {
         if (i >= sizeof(identifier) / sizeof(char) - 1) {
            strcpy(context->error_msg, "out of memory");
            _pure_ungetc(context, c);
            while (i) {
               _pure_ungetc(context, identifier[--i]);
            }
            return -1;
         }
         identifier[i++] = (char)c;
      } else {
         _pure_ungetc(context, c);
         break;
      }
   }
   identifier[i] = '\0';

   out->data.identifier = sl_pp_context_add_unique_str(context, identifier);
   if (out->data.identifier == -1) {
      while (i) {
         _pure_ungetc(context, identifier[--i]);
      }
      return -1;
   }

   return 0;
}


/*
 * Return the number of consecutive decimal digits in the input stream.
 */
static unsigned int
_parse_float_digits(struct lookahead_state *lookahead)
{
   unsigned int eaten;

   for (eaten = 0;; eaten++) {
      unsigned int pos = _lookahead_tell(lookahead);
      char c = _lookahead_getc(lookahead);

      if (c < '0' || c > '9') {
         _lookahead_revert(lookahead, pos);
         break;
      }
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
_parse_float_frac(struct lookahead_state *lookahead)
{
   unsigned int pos;
   int c;
   unsigned int eaten;

   pos = _lookahead_tell(lookahead);
   c = _lookahead_getc(lookahead);
   if (c == '.') {
      eaten = _parse_float_digits(lookahead);
      if (eaten) {
         return eaten + 1;
      }
      _lookahead_revert(lookahead, pos);
      return 0;
   }

   _lookahead_revert(lookahead, pos);
   eaten = _parse_float_digits(lookahead);
   if (eaten) {
      c = _lookahead_getc(lookahead);
      if (c == '.') {
         return eaten + 1 + _parse_float_digits(lookahead);
      }
   }

   _lookahead_revert(lookahead, pos);
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
_parse_float_exp(struct lookahead_state *lookahead)
{
   unsigned int pos, pos2;
   int c;
   unsigned int eaten, digits;

   pos = _lookahead_tell(lookahead);
   c = _lookahead_getc(lookahead);
   if (c != 'e' && c != 'E') {
      _lookahead_revert(lookahead, pos);
      return 0;
   }

   pos2 = _lookahead_tell(lookahead);
   c = _lookahead_getc(lookahead);
   if (c == '-' || c == '+') {
      eaten = 2;
   } else {
      _lookahead_revert(lookahead, pos2);
      eaten = 1;
   }

   digits = _parse_float_digits(lookahead);
   if (!digits) {
      _lookahead_revert(lookahead, pos);
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
_parse_float(struct lookahead_state *lookahead)
{
   unsigned int eaten;

   eaten = _parse_float_frac(lookahead);
   if (eaten) {
      unsigned int pos;
      int c;

      eaten += _parse_float_exp(lookahead);

      pos = _lookahead_tell(lookahead);
      c = _lookahead_getc(lookahead);
      if (c == 'f' || c == 'F') {
         eaten++;
      } else {
         _lookahead_revert(lookahead, pos);
      }

      return eaten;
   }

   eaten = _parse_float_digits(lookahead);
   if (eaten) {
      unsigned int exponent;

      exponent = _parse_float_exp(lookahead);
      if (exponent) {
         unsigned int pos;
         int c;

         eaten += exponent;

         pos = _lookahead_tell(lookahead);
         c = _lookahead_getc(lookahead);
         if (c == 'f' || c == 'F') {
            eaten++;
         } else {
            _lookahead_revert(lookahead, pos);
         }

         return eaten;
      }
   }

   _lookahead_revert(lookahead, 0);
   return 0;
}


static unsigned int
_parse_hex(struct lookahead_state *lookahead)
{
   int c;
   unsigned int n;

   c = _lookahead_getc(lookahead);
   if (c != '0') {
      _lookahead_revert(lookahead, 0);
      return 0;
   }

   c = _lookahead_getc(lookahead);
   if (c != 'x' && c != 'X') {
      _lookahead_revert(lookahead, 0);
      return 0;
   }

   for (n = 2;;) {
      unsigned int pos = _lookahead_tell(lookahead);

      c = _lookahead_getc(lookahead);
      if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
         n++;
      } else {
         _lookahead_revert(lookahead, pos);
         break;
      }
   }

   if (n > 2) {
      return n;
   }

   _lookahead_revert(lookahead, 0);
   return 0;
}


static unsigned int
_parse_oct(struct lookahead_state *lookahead)
{
   int c;
   unsigned int n;

   c = _lookahead_getc(lookahead);
   if (c != '0') {
      _lookahead_revert(lookahead, 0);
      return 0;
   }

   for (n = 1;;) {
      unsigned int pos = _lookahead_tell(lookahead);

      c = _lookahead_getc(lookahead);
      if ((c >= '0' && c <= '7')) {
         n++;
      } else {
         _lookahead_revert(lookahead, pos);
         break;
      }
   }

   return n;
}


static unsigned int
_parse_dec(struct lookahead_state *lookahead)
{
   unsigned int n = 0;

   for (;;) {
      unsigned int pos = _lookahead_tell(lookahead);
      int c = _lookahead_getc(lookahead);

      if ((c >= '0' && c <= '9')) {
         n++;
      } else {
         _lookahead_revert(lookahead, pos);
         break;
      }
   }

   return n;
}


static int
_tokenise_number(struct sl_pp_context *context,
                 struct sl_pp_token_info *out)
{
   struct lookahead_state lookahead;
   unsigned int eaten;
   unsigned int is_float = 0;
   unsigned int pos;
   int c;
   char number[256];   /* XXX: Remove this artifical limit. */

   _lookahead_init(&lookahead, context);

   eaten = _parse_float(&lookahead);
   if (!eaten) {
      eaten = _parse_hex(&lookahead);
      if (!eaten) {
         eaten = _parse_oct(&lookahead);
         if (!eaten) {
            eaten = _parse_dec(&lookahead);
         }
      }
   } else {
      is_float = 1;
   }

   if (!eaten) {
      strcpy(context->error_msg, "expected a number");
      return -1;
   }

   pos = _lookahead_tell(&lookahead);
   c = _lookahead_getc(&lookahead);
   _lookahead_revert(&lookahead, pos);

   if (_is_identifier_char(c)) {
      strcpy(context->error_msg, "expected a number");
      _lookahead_revert(&lookahead, 0);
      return -1;
   }

   if (eaten > sizeof(number) - 1) {
      strcpy(context->error_msg, "out of memory");
      _lookahead_revert(&lookahead, 0);
      return -1;
   }

   assert(_lookahead_tell(&lookahead) == eaten);

   memcpy(number, _lookahead_buf(&lookahead), eaten);
   number[eaten] = '\0';

   if (is_float) {
      out->token = SL_PP_FLOAT;
      out->data._float = sl_pp_context_add_unique_str(context, number);
      if (out->data._float == -1) {
         _lookahead_revert(&lookahead, 0);
         return -1;
      }
   } else {
      out->token = SL_PP_UINT;
      out->data._uint = sl_pp_context_add_unique_str(context, number);
      if (out->data._uint == -1) {
         _lookahead_revert(&lookahead, 0);
         return -1;
      }
   }

   return 0;
}


int
sl_pp_token_get(struct sl_pp_context *context,
                struct sl_pp_token_info *out)
{
   int c = _pure_getc(context);

   switch (c) {
   case ' ':
   case '\t':
      out->token = SL_PP_WHITESPACE;
      break;

   case '\n':
      out->token = SL_PP_NEWLINE;
      break;

   case '#':
      out->token = SL_PP_HASH;
      break;

   case ',':
      out->token = SL_PP_COMMA;
      break;

   case ';':
      out->token = SL_PP_SEMICOLON;
      break;

   case '{':
      out->token = SL_PP_LBRACE;
      break;

   case '}':
      out->token = SL_PP_RBRACE;
      break;

   case '(':
      out->token = SL_PP_LPAREN;
      break;

   case ')':
      out->token = SL_PP_RPAREN;
      break;

   case '[':
      out->token = SL_PP_LBRACKET;
      break;

   case ']':
      out->token = SL_PP_RBRACKET;
      break;

   case '.':
      {
         int c2 = _pure_getc(context);

         if (c2 == PURE_ERROR) {
            return -1;
         }
         if (c2 >= '0' && c2 <= '9') {
            _pure_ungetc(context, c2);
            _pure_ungetc(context, c);
            if (_tokenise_number(context, out)) {
               return -1;
            }
         } else {
            _pure_ungetc(context, c2);
            out->token = SL_PP_DOT;
         }
      }
      break;

   case '+':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '+') {
         out->token = SL_PP_INCREMENT;
      } else if (c == '=') {
         out->token = SL_PP_ADDASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_PLUS;
      }
      break;

   case '-':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '-') {
         out->token = SL_PP_DECREMENT;
      } else if (c == '=') {
         out->token = SL_PP_SUBASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_MINUS;
      }
      break;

   case '~':
      out->token = SL_PP_BITNOT;
      break;

   case '!':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '=') {
         out->token = SL_PP_NOTEQUAL;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_NOT;
      }
      break;

   case '*':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '=') {
         out->token = SL_PP_MULASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_STAR;
      }
      break;

   case '/':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '=') {
         out->token = SL_PP_DIVASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_SLASH;
      }
      break;

   case '%':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '=') {
         out->token = SL_PP_MODASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_MODULO;
      }
      break;

   case '<':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '<') {
         c = _pure_getc(context);
         if (c == PURE_ERROR) {
            return -1;
         }
         if (c == '=') {
            out->token = SL_PP_LSHIFTASSIGN;
         } else {
            _pure_ungetc(context, c);
            out->token = SL_PP_LSHIFT;
         }
      } else if (c == '=') {
         out->token = SL_PP_LESSEQUAL;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_LESS;
      }
      break;

   case '>':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '>') {
         c = _pure_getc(context);
         if (c == PURE_ERROR) {
            return -1;
         }
         if (c == '=') {
            out->token = SL_PP_RSHIFTASSIGN;
         } else {
            _pure_ungetc(context, c);
            out->token = SL_PP_RSHIFT;
         }
      } else if (c == '=') {
         out->token = SL_PP_GREATEREQUAL;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_GREATER;
      }
      break;

   case '=':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '=') {
         out->token = SL_PP_EQUAL;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_ASSIGN;
      }
      break;

   case '&':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '&') {
         out->token = SL_PP_AND;
      } else if (c == '=') {
         out->token = SL_PP_BITANDASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_BITAND;
      }
      break;

   case '^':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '^') {
         out->token = SL_PP_XOR;
      } else if (c == '=') {
         out->token = SL_PP_BITXORASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_BITXOR;
      }
      break;

   case '|':
      c = _pure_getc(context);
      if (c == PURE_ERROR) {
         return -1;
      }
      if (c == '|') {
         out->token = SL_PP_OR;
      } else if (c == '=') {
         out->token = SL_PP_BITORASSIGN;
      } else {
         _pure_ungetc(context, c);
         out->token = SL_PP_BITOR;
      }
      break;

   case '?':
      out->token = SL_PP_QUESTION;
      break;

   case ':':
      out->token = SL_PP_COLON;
      break;

   case '\0':
      out->token = SL_PP_EOF;
      break;

   case PURE_ERROR:
      return -1;

   default:
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
         _pure_ungetc(context, c);
         if (_tokenise_identifier(context, out)) {
            return -1;
         }
      } else if (c >= '0' && c <= '9') {
         _pure_ungetc(context, c);
         if (_tokenise_number(context, out)) {
            return -1;
         }
      } else {
         out->data.other = c;
         out->token = SL_PP_OTHER;
      }
   }

   return 0;
}


int
sl_pp_tokenise(struct sl_pp_context *context,
               struct sl_pp_token_info **output)
{
   struct sl_pp_token_info *out = NULL;
   unsigned int out_len = 0;
   unsigned int out_max = 0;

   for (;;) {
      struct sl_pp_token_info info;

      if (sl_pp_token_buffer_get(&context->tokens, &info)) {
         free(out);
         return -1;
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
