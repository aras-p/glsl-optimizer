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
#include "sl_pp_process.h"


static void
skip_whitespace(const struct sl_pp_token_info *input,
                unsigned int *first,
                unsigned int last)
{
   while (*first < last && input[*first].token == SL_PP_WHITESPACE) {
      (*first)++;
   }
}


static int
_parse_formal_args(const struct sl_pp_token_info *input,
                   unsigned int *first,
                   unsigned int last,
                   struct sl_pp_macro *macro)
{
   struct sl_pp_macro_formal_arg **arg;

   skip_whitespace(input, first, last);
   if (*first < last) {
      if (input[*first].token == SL_PP_RPAREN) {
         (*first)++;
         return 0;
      }
   } else {
      /* Expected either an identifier or `)'. */
      return -1;
   }

   arg = &macro->arg;

   for (;;) {
      if (*first < last && input[*first].token != SL_PP_IDENTIFIER) {
         /* Expected an identifier. */
         return -1;
      }

      *arg = malloc(sizeof(struct sl_pp_macro_formal_arg));
      if (!*arg) {
         return -1;
      }

      (**arg).name = input[*first].data.identifier;
      (*first)++;

      (**arg).next = NULL;
      arg = &(**arg).next;

      skip_whitespace(input, first, last);
      if (*first < last) {
         if (input[*first].token == SL_PP_COMMA) {
            (*first)++;
         } else if (input[*first].token == SL_PP_RPAREN) {
            (*first)++;
            return 0;
         } else {
            /* Expected either `,' or `)'. */
            return -1;
         }
      } else {
         /* Expected either `,' or `)'. */
         return -1;
      }
   }
}


int
sl_pp_process_define(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last,
                     struct sl_pp_macro *macro)
{
   macro->name = -1;
   macro->arg = NULL;
   macro->body = NULL;
   macro->next = NULL;

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      macro->name = input[first].data.identifier;
      first++;
   }

   if (macro->name == -1) {
      return -1;
   }

   /*
    * If there is no whitespace between macro name and left paren, a macro
    * formal argument list follows. This is the only place where the presence
    * of a whitespace matters and it's the only reason why we are dealing
    * with whitespace at this level.
    */
   if (first < last && input[first].token == SL_PP_LPAREN) {
      first++;
      if (_parse_formal_args(input, &first, last, macro)) {
         return -1;
      }
   }

   /* Trim whitespace from the left side. */
   skip_whitespace(input, &first, last);

   /* Trom whitespace from the right side. */
   while (first < last && input[last - 1].token == SL_PP_WHITESPACE) {
      last--;
   }

   /* All that is left between first and last is the macro definition. */
   macro->body_len = last - first;
   if (macro->body_len) {
      macro->body = malloc(sizeof(struct sl_pp_token_info) * macro->body_len);
      if (!macro->body) {
         return -1;
      }

      memcpy(macro->body,
             &input[first],
             sizeof(struct sl_pp_token_info) * macro->body_len);
   }

   return 0;
}
