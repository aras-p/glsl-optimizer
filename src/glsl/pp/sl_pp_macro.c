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
#include "sl_pp_macro.h"
#include "sl_pp_process.h"


struct sl_pp_macro *
sl_pp_macro_new(void)
{
   struct sl_pp_macro *macro;

   macro = calloc(1, sizeof(struct sl_pp_macro));
   if (macro) {
      macro->name = -1;
      macro->num_args = -1;
   }
   return macro;
}

void
sl_pp_macro_free(struct sl_pp_macro *macro)
{
   while (macro) {
      struct sl_pp_macro *next_macro = macro->next;
      struct sl_pp_macro_formal_arg *arg = macro->arg;

      while (arg) {
         struct sl_pp_macro_formal_arg *next_arg = arg->next;

         free(arg);
         arg = next_arg;
      }

      free(macro->body);

      free(macro);
      macro = next_macro;
   }
}

static void
skip_whitespace(const struct sl_pp_token_info *input,
                unsigned int *pi)
{
   while (input[*pi].token == SL_PP_WHITESPACE) {
      (*pi)++;
   }
}

int
sl_pp_macro_expand(struct sl_pp_context *context,
                   const struct sl_pp_token_info *input,
                   unsigned int *pi,
                   struct sl_pp_macro *local,
                   struct sl_pp_process_state *state)
{
   int macro_name;
   struct sl_pp_macro *macro = NULL;
   struct sl_pp_macro *actual_arg = NULL;
   unsigned int j;

   if (input[*pi].token != SL_PP_IDENTIFIER) {
      return -1;
   }

   macro_name = input[*pi].data.identifier;

   if (local) {
      for (macro = local; macro; macro = macro->next) {
         if (macro->name == macro_name) {
            break;
         }
      }
   }

   if (!macro) {
      for (macro = context->macro; macro; macro = macro->next) {
         if (macro->name == macro_name) {
            break;
         }
      }
   }

   if (!macro) {
      if (sl_pp_process_out(state, &input[*pi])) {
         return -1;
      }
      (*pi)++;
      return 0;
   }

   (*pi)++;

   if (macro->num_args >= 0) {
      skip_whitespace(input, pi);
      if (input[*pi].token != SL_PP_LPAREN) {
         return -1;
      }
      (*pi)++;
      skip_whitespace(input, pi);
   }

   if (macro->num_args > 0) {
      struct sl_pp_macro_formal_arg *formal_arg = macro->arg;
      struct sl_pp_macro **pmacro = &actual_arg;

      for (j = 0; j < (unsigned int)macro->num_args; j++) {
         unsigned int body_len;
         unsigned int i;
         int done = 0;
         unsigned int paren_nesting = 0;
         unsigned int k;

         *pmacro = sl_pp_macro_new();
         if (!*pmacro) {
            return -1;
         }

         (**pmacro).name = formal_arg->name;

         body_len = 1;
         for (i = *pi; !done; i++) {
            switch (input[i].token) {
            case SL_PP_WHITESPACE:
               break;

            case SL_PP_COMMA:
               if (!paren_nesting) {
                  if (j < (unsigned int)macro->num_args - 1) {
                     done = 1;
                  } else {
                     return -1;
                  }
               } else {
                  body_len++;
               }
               break;

            case SL_PP_LPAREN:
               paren_nesting++;
               body_len++;
               break;

            case SL_PP_RPAREN:
               if (!paren_nesting) {
                  if (j == (unsigned int)macro->num_args - 1) {
                     done = 1;
                  } else {
                     return -1;
                  }
               } else {
                  paren_nesting--;
                  body_len++;
               }
               break;

            case SL_PP_EOF:
               return -1;

            default:
               body_len++;
            }
         }

         (**pmacro).body = malloc(sizeof(struct sl_pp_token_info) * body_len);
         if (!(**pmacro).body) {
            return -1;
         }

         for (done = 0, k = 0, i = *pi; !done; i++) {
            switch (input[i].token) {
            case SL_PP_WHITESPACE:
               break;

            case SL_PP_COMMA:
               if (!paren_nesting && j < (unsigned int)macro->num_args - 1) {
                  done = 1;
               } else {
                  (**pmacro).body[k++] = input[i];
               }
               break;

            case SL_PP_LPAREN:
               paren_nesting++;
               (**pmacro).body[k++] = input[i];
               break;

            case SL_PP_RPAREN:
               if (!paren_nesting && j == (unsigned int)macro->num_args - 1) {
                  done = 1;
               } else {
                  paren_nesting--;
                  (**pmacro).body[k++] = input[i];
               }
               break;

            default:
               (**pmacro).body[k++] = input[i];
            }
         }

         (**pmacro).body[k++].token = SL_PP_EOF;
         (*pi) = i;

         formal_arg = formal_arg->next;
         pmacro = &(**pmacro).next;
      }
   }

   /* Right paren for non-empty argument list has already been eaten. */
   if (macro->num_args == 0) {
      skip_whitespace(input, pi);
      if (input[*pi].token != SL_PP_RPAREN) {
         return -1;
      }
      (*pi)++;
   }

   for (j = 0;;) {
      switch (macro->body[j].token) {
      case SL_PP_IDENTIFIER:
         if (sl_pp_macro_expand(context, macro->body, &j, actual_arg, state)) {
            return -1;
         }
         break;

      case SL_PP_EOF:
         sl_pp_macro_free(actual_arg);
         return 0;

      default:
         if (sl_pp_process_out(state, &macro->body[j])) {
            return -1;
         }
         j++;
      }
   }
}
