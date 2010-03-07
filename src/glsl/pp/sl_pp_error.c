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
#include "sl_pp_context.h"
#include "sl_pp_process.h"
#include "sl_pp_public.h"


void
sl_pp_process_error(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last)
{
   unsigned int out_len = 0;
   unsigned int i;

   for (i = first; i < last; i++) {
      const char *s = NULL;
      char buf[2];

      switch (input[i].token) {
      case SL_PP_WHITESPACE:
         s = " ";
         break;

      case SL_PP_NEWLINE:
         s = "\n";
         break;

      case SL_PP_HASH:
         s = "#";
         break;

      case SL_PP_COMMA:
         s = ",";
         break;

      case SL_PP_SEMICOLON:
         s = ";";
         break;

      case SL_PP_LBRACE:
         s = "{";
         break;

      case SL_PP_RBRACE:
         s = "}";
         break;

      case SL_PP_LPAREN:
         s = "(";
         break;

      case SL_PP_RPAREN:
         s = ")";
         break;

      case SL_PP_LBRACKET:
         s = "[";
         break;

      case SL_PP_RBRACKET:
         s = "]";
         break;

      case SL_PP_DOT:
         s = ".";
         break;

      case SL_PP_INCREMENT:
         s = "++";
         break;

      case SL_PP_ADDASSIGN:
         s = "+=";
         break;

      case SL_PP_PLUS:
         s = "+";
         break;

      case SL_PP_DECREMENT:
         s = "--";
         break;

      case SL_PP_SUBASSIGN:
         s = "-=";
         break;

      case SL_PP_MINUS:
         s = "-";
         break;

      case SL_PP_BITNOT:
         s = "~";
         break;

      case SL_PP_NOTEQUAL:
         s = "!=";
         break;

      case SL_PP_NOT:
         s = "!";
         break;

      case SL_PP_MULASSIGN:
         s = "*=";
         break;

      case SL_PP_STAR:
         s = "*";
         break;

      case SL_PP_DIVASSIGN:
         s = "/=";
         break;

      case SL_PP_SLASH:
         s = "/";
         break;

      case SL_PP_MODASSIGN:
         s = "%=";
         break;

      case SL_PP_MODULO:
         s = "%";
         break;

      case SL_PP_LSHIFTASSIGN:
         s = "<<=";
         break;

      case SL_PP_LSHIFT:
         s = "<<";
         break;

      case SL_PP_LESSEQUAL:
         s = "<=";
         break;

      case SL_PP_LESS:
         s = "<";
         break;

      case SL_PP_RSHIFTASSIGN:
         s = ">>=";
         break;

      case SL_PP_RSHIFT:
         s = ">>";
         break;

      case SL_PP_GREATEREQUAL:
         s = ">=";
         break;

      case SL_PP_GREATER:
         s = ">";
         break;

      case SL_PP_EQUAL:
         s = "==";
         break;

      case SL_PP_ASSIGN:
         s = "=";
         break;

      case SL_PP_AND:
         s = "&&";
         break;

      case SL_PP_BITANDASSIGN:
         s = "&=";
         break;

      case SL_PP_BITAND:
         s = "&";
         break;

      case SL_PP_XOR:
         s = "^^";
         break;

      case SL_PP_BITXORASSIGN:
         s = "^=";
         break;

      case SL_PP_BITXOR:
         s = "^";
         break;

      case SL_PP_OR:
         s = "||";
         break;

      case SL_PP_BITORASSIGN:
         s = "|=";
         break;

      case SL_PP_BITOR:
         s = "|";
         break;

      case SL_PP_QUESTION:
         s = "?";
         break;

      case SL_PP_COLON:
         s = ":";
         break;

      case SL_PP_IDENTIFIER:
         s = sl_pp_context_cstr(context, input[i].data.identifier);
         break;

      case SL_PP_UINT:
         s = sl_pp_context_cstr(context, input[i].data._uint);
         break;

      case SL_PP_FLOAT:
         s = sl_pp_context_cstr(context, input[i].data._float);
         break;

      case SL_PP_OTHER:
         buf[0] = input[i].data.other;
         buf[1] = '\0';
         s = buf;
         break;

      default:
         strcpy(context->error_msg, "internal error");
         return;
      }

      while (*s != '\0' && out_len < sizeof(context->error_msg) - 1) {
         context->error_msg[out_len++] = *s++;
      }
   }

   context->error_msg[out_len] = '\0';
}
