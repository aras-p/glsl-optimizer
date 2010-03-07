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
_parse_formal_args(struct sl_pp_context *context,
                   const struct sl_pp_token_info *input,
                   unsigned int *first,
                   unsigned int last,
                   struct sl_pp_macro *macro)
{
   struct sl_pp_macro_formal_arg **arg;

   macro->num_args = 0;

   skip_whitespace(input, first, last);
   if (*first < last) {
      if (input[*first].token == SL_PP_RPAREN) {
         (*first)++;
         return 0;
      }
   } else {
      strcpy(context->error_msg, "expected either macro formal argument or `)'");
      return -1;
   }

   arg = &macro->arg;

   for (;;) {
      if (*first < last && input[*first].token != SL_PP_IDENTIFIER) {
         strcpy(context->error_msg, "expected macro formal argument");
         return -1;
      }

      *arg = malloc(sizeof(struct sl_pp_macro_formal_arg));
      if (!*arg) {
         strcpy(context->error_msg, "out of memory");
         return -1;
      }

      (**arg).name = input[*first].data.identifier;
      (*first)++;

      (**arg).next = NULL;
      arg = &(**arg).next;

      macro->num_args++;

      skip_whitespace(input, first, last);
      if (*first < last) {
         if (input[*first].token == SL_PP_COMMA) {
            (*first)++;
            skip_whitespace(input, first, last);
         } else if (input[*first].token == SL_PP_RPAREN) {
            (*first)++;
            return 0;
         } else {
            strcpy(context->error_msg, "expected either `,' or `)'");
            return -1;
         }
      } else {
         strcpy(context->error_msg, "expected either `,' or `)'");
         return -1;
      }
   }

   /* Should not gete here. */
}


int
sl_pp_process_define(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last)
{
   int macro_name = -1;
   struct sl_pp_macro *macro;
   unsigned int i;
   unsigned int body_len;
   unsigned int j;

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      macro_name = input[first].data.identifier;
      first++;
   }
   if (macro_name == -1) {
      strcpy(context->error_msg, "expected macro name");
      return -1;
   }

   /* Check for reserved macro names */
   {
      const char *name = sl_pp_context_cstr(context, macro_name);

      if (strstr(name, "__")) {
         strcpy(context->error_msg, "macro names containing `__' are reserved");
         return 1;
      }
      if (name[0] == 'G' && name[1] == 'L' && name[2] == '_') {
         strcpy(context->error_msg, "macro names prefixed with `GL_' are reserved");
         return 1;
      }
   }

   for (macro = context->macro; macro; macro = macro->next) {
      if (macro->name == macro_name) {
         break;
      }
   }

   if (!macro) {
      macro = sl_pp_macro_new();
      if (!macro) {
         strcpy(context->error_msg, "out of memory");
         return -1;
      }

      *context->macro_tail = macro;
      context->macro_tail = &macro->next;
   } else {
      sl_pp_macro_reset(macro);
   }

   macro->name = macro_name;

   /*
    * If there is no whitespace between macro name and left paren, a macro
    * formal argument list follows. This is the only place where the presence
    * of a whitespace matters and it's the only reason why we are dealing
    * with whitespace at this level.
    */
   if (first < last && input[first].token == SL_PP_LPAREN) {
      first++;
      if (_parse_formal_args(context, input, &first, last, macro)) {
         return -1;
      }
   }

   /* Calculate body size, trim out whitespace, make room for EOF. */
   body_len = 1;
   for (i = first; i < last; i++) {
      if (input[i].token != SL_PP_WHITESPACE) {
         body_len++;
      }
   }

   macro->body = malloc(sizeof(struct sl_pp_token_info) * body_len);
   if (!macro->body) {
      strcpy(context->error_msg, "out of memory");
      return -1;
   }

   for (j = 0, i = first; i < last; i++) {
      if (input[i].token != SL_PP_WHITESPACE) {
         macro->body[j++] = input[i];
      }
   }
   macro->body[j++].token = SL_PP_EOF;

   return 0;
}


int
sl_pp_process_undef(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last)
{
   int macro_name = -1;
   struct sl_pp_macro **pmacro;
   struct sl_pp_macro *macro;

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      macro_name = input[first].data.identifier;
   }
   if (macro_name == -1) {
      return 0;
   }

   for (pmacro = &context->macro; *pmacro; pmacro = &(**pmacro).next) {
      if ((**pmacro).name == macro_name) {
         break;
      }
   }
   if (!*pmacro) {
      return 0;
   }

   macro = *pmacro;
   *pmacro = macro->next;
   macro->next = NULL;
   sl_pp_macro_free(macro);

   return 0;
}
