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
#include "sl_pp_process.h"


static void
skip_whitespace(const struct sl_pp_token_info *input,
                unsigned int *pi)
{
   while (input[*pi].token == SL_PP_WHITESPACE) {
      (*pi)++;
   }
}

static int
_parse_defined(struct sl_pp_context *context,
               const struct sl_pp_token_info *input,
               unsigned int *pi,
               struct sl_pp_process_state *state)
{
   int parens = 0;
   int macro_name;
   struct sl_pp_macro *macro;
   int defined = 0;
   struct sl_pp_token_info result;

   skip_whitespace(input, pi);
   if (input[*pi].token == SL_PP_LPAREN) {
      (*pi)++;
      skip_whitespace(input, pi);
      parens = 1;
   }

   if (input[*pi].token != SL_PP_IDENTIFIER) {
      strcpy(context->error_msg, "expected an identifier");
      return -1;
   }

   macro_name = input[*pi].data.identifier;
   for (macro = context->macro; macro; macro = macro->next) {
      if (macro->name == macro_name) {
         defined = 1;
         break;
      }
   }
   (*pi)++;

   if (parens) {
      skip_whitespace(input, pi);
      if (input[*pi].token != SL_PP_RPAREN) {
         strcpy(context->error_msg, "expected `)'");
         return -1;
      }
      (*pi)++;
   }

   result.token = SL_PP_UINT;
   result.data._uint = (defined ? context->dict._1 : context->dict._0);

   if (sl_pp_process_out(state, &result)) {
      strcpy(context->error_msg, "out of memory");
      return -1;
   }

   return 0;
}

static unsigned int
_evaluate_if_stack(struct sl_pp_context *context)
{
   unsigned int i;

   for (i = context->if_ptr; i < SL_PP_MAX_IF_NESTING; i++) {
      if (!(context->if_stack[i] & 1)) {
         return 0;
      }
   }
   return 1;
}

static int
_parse_if(struct sl_pp_context *context,
          const struct sl_pp_token_info *input,
          unsigned int first,
          unsigned int last)
{
   unsigned int i;
   struct sl_pp_process_state state;
   struct sl_pp_token_info eof;
   int result;

   if (!context->if_ptr) {
      strcpy(context->error_msg, "`#if' nesting too deep");
      return -1;
   }

   memset(&state, 0, sizeof(state));
   for (i = first; i < last;) {
      switch (input[i].token) {
      case SL_PP_WHITESPACE:
         i++;
         break;

      case SL_PP_IDENTIFIER:
         if (input[i].data.identifier == context->dict.defined) {
            i++;
            if (_parse_defined(context, input, &i, &state)) {
               free(state.out);
               return -1;
            }
         } else {
            if (sl_pp_macro_expand(context, input, &i, NULL, &state, sl_pp_macro_expand_unknown_to_0)) {
               free(state.out);
               return -1;
            }
         }
         break;

      default:
         if (sl_pp_process_out(&state, &input[i])) {
            strcpy(context->error_msg, "out of memory");
            free(state.out);
            return -1;
         }
         i++;
      }
   }

   eof.token = SL_PP_EOF;
   if (sl_pp_process_out(&state, &eof)) {
      strcpy(context->error_msg, "out of memory");
      free(state.out);
      return -1;
   }

   if (sl_pp_execute_expression(context, state.out, &result)) {
      free(state.out);
      return -1;
   }

   free(state.out);

   context->if_ptr--;
   context->if_stack[context->if_ptr] = result ? 1 : 0;
   context->if_value = _evaluate_if_stack(context);

   return 0;
}

static int
_parse_else(struct sl_pp_context *context)
{
   if (context->if_ptr == SL_PP_MAX_IF_NESTING) {
      strcpy(context->error_msg, "no matching `#if'");
      return -1;
   }

   /* Bit b1 indicates we already went through #else. */
   if (context->if_stack[context->if_ptr] & 2) {
      strcpy(context->error_msg, "no matching `#if'");
      return -1;
   }

   /* Invert current condition value and mark that we are in the #else block. */
   context->if_stack[context->if_ptr] = (1 - (context->if_stack[context->if_ptr] & 1)) | 2;
   context->if_value = _evaluate_if_stack(context);

   return 0;
}

int
sl_pp_process_if(struct sl_pp_context *context,
                 const struct sl_pp_token_info *input,
                 unsigned int first,
                 unsigned int last)
{
   return _parse_if(context, input, first, last);
}

int
sl_pp_process_ifdef(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last)
{
   unsigned int i;

   if (!context->if_ptr) {
      strcpy(context->error_msg, "`#if' nesting too deep");
      return -1;
   }

   for (i = first; i < last; i++) {
      switch (input[i].token) {
      case SL_PP_IDENTIFIER:
         {
            struct sl_pp_macro *macro;
            int macro_name = input[i].data.identifier;
            int defined = 0;

            for (macro = context->macro; macro; macro = macro->next) {
               if (macro->name == macro_name) {
                  defined = 1;
                  break;
               }
            }

            context->if_ptr--;
            context->if_stack[context->if_ptr] = defined ? 1 : 0;
            context->if_value = _evaluate_if_stack(context);
         }
         return 0;

      case SL_PP_WHITESPACE:
         break;

      default:
         strcpy(context->error_msg, "expected an identifier");
         return -1;
      }
   }

   strcpy(context->error_msg, "expected an identifier");
   return -1;
}

int
sl_pp_process_ifndef(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last)
{
   unsigned int i;

   if (!context->if_ptr) {
      strcpy(context->error_msg, "`#if' nesting too deep");
      return -1;
   }

   for (i = first; i < last; i++) {
      switch (input[i].token) {
      case SL_PP_IDENTIFIER:
         {
            struct sl_pp_macro *macro;
            int macro_name = input[i].data.identifier;
            int defined = 0;

            for (macro = context->macro; macro; macro = macro->next) {
               if (macro->name == macro_name) {
                  defined = 1;
                  break;
               }
            }

            context->if_ptr--;
            context->if_stack[context->if_ptr] = defined ? 0 : 1;
            context->if_value = _evaluate_if_stack(context);
         }
         return 0;

      case SL_PP_WHITESPACE:
         break;

      default:
         strcpy(context->error_msg, "expected an identifier");
         return -1;
      }
   }

   strcpy(context->error_msg, "expected an identifier");
   return -1;
}

int
sl_pp_process_elif(struct sl_pp_context *context,
                   const struct sl_pp_token_info *input,
                   unsigned int first,
                   unsigned int last)
{
   if (_parse_else(context)) {
      return -1;
   }

   if (context->if_stack[context->if_ptr] & 1) {
      context->if_ptr++;
      if (_parse_if(context, input, first, last)) {
         return -1;
      }
   }

   /* We are still in the #if block. */
   context->if_stack[context->if_ptr] = context->if_stack[context->if_ptr] & ~2;

   return 0;
}

int
sl_pp_process_else(struct sl_pp_context *context,
                   const struct sl_pp_token_info *input,
                   unsigned int first,
                   unsigned int last)
{
   return _parse_else(context);
}

int
sl_pp_process_endif(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last)
{
   if (context->if_ptr == SL_PP_MAX_IF_NESTING) {
      strcpy(context->error_msg, "no matching `#if'");
      return -1;
   }

   context->if_ptr++;
   context->if_value = _evaluate_if_stack(context);

   return 0;
}
