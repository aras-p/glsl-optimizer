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


static int
_macro_is_defined(struct sl_pp_context *context,
                  int macro_name)
{
   unsigned int i;
   struct sl_pp_macro *macro;

   for (i = 0; i < context->num_extensions; i++) {
      if (macro_name == context->extensions[i].name) {
         return 1;
      }
   }

   for (macro = context->macro; macro; macro = macro->next) {
      if (macro_name == macro->name) {
         return 1;
      }
   }

   return 0;
}

static int
_parse_defined(struct sl_pp_context *context,
               struct sl_pp_token_buffer *buffer,
               struct sl_pp_process_state *state)
{
   struct sl_pp_token_info input;
   int parens = 0;
   int defined;
   struct sl_pp_token_info result;

   if (sl_pp_token_buffer_skip_white(buffer, &input)) {
      return -1;
   }

   if (input.token == SL_PP_LPAREN) {
      if (sl_pp_token_buffer_skip_white(buffer, &input)) {
         return -1;
      }
      parens = 1;
   }

   if (input.token != SL_PP_IDENTIFIER) {
      strcpy(context->error_msg, "expected an identifier");
      return -1;
   }

   defined = _macro_is_defined(context, input.data.identifier);

   if (parens) {
      if (sl_pp_token_buffer_skip_white(buffer, &input)) {
         return -1;
      }
      if (input.token != SL_PP_RPAREN) {
         strcpy(context->error_msg, "expected `)'");
         return -1;
      }
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
      if (!context->if_stack[i].u.condition) {
         return 0;
      }
   }
   return 1;
}

static int
_parse_if(struct sl_pp_context *context,
          struct sl_pp_token_buffer *buffer)
{
   struct sl_pp_process_state state;
   int found_end = 0;
   struct sl_pp_token_info eof;
   int result;

   if (!context->if_ptr) {
      strcpy(context->error_msg, "`#if' nesting too deep");
      return -1;
   }

   memset(&state, 0, sizeof(state));
   while (!found_end) {
      struct sl_pp_token_info input;

      sl_pp_token_buffer_get(buffer, &input);
      switch (input.token) {
      case SL_PP_WHITESPACE:
         break;

      case SL_PP_IDENTIFIER:
         if (input.data.identifier == context->dict.defined) {
            if (_parse_defined(context, buffer, &state)) {
               free(state.out);
               return -1;
            }
         } else {
            sl_pp_token_buffer_unget(buffer, &input);
            if (sl_pp_macro_expand(context, buffer, NULL, &state, sl_pp_macro_expand_unknown_to_0)) {
               free(state.out);
               return -1;
            }
         }
         break;

      case SL_PP_NEWLINE:
      case SL_PP_EOF:
         found_end = 1;
         break;

      default:
         if (sl_pp_process_out(&state, &input)) {
            strcpy(context->error_msg, "out of memory");
            free(state.out);
            return -1;
         }
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
   context->if_stack[context->if_ptr].value = 0;
   context->if_stack[context->if_ptr].u.condition = result ? 1 : 0;
   context->if_value = _evaluate_if_stack(context);

   return 0;
}

static int
_parse_else(struct sl_pp_context *context)
{
   union sl_pp_if_state *state = &context->if_stack[context->if_ptr];

   if (context->if_ptr == SL_PP_MAX_IF_NESTING) {
      strcpy(context->error_msg, "no matching `#if'");
      return -1;
   }

   if (state->u.went_thru_else) {
      strcpy(context->error_msg, "no matching `#if'");
      return -1;
   }

   /* Once we had a true condition, the subsequent #elifs should always be false. */
   state->u.had_true_cond |= state->u.condition;

   /* Update current condition value and mark that we are in the #else block. */
   state->u.condition = !(state->u.had_true_cond | state->u.condition);
   state->u.went_thru_else = 1;
   context->if_value = _evaluate_if_stack(context);

   return 0;
}

int
sl_pp_process_if(struct sl_pp_context *context,
                 struct sl_pp_token_buffer *buffer)
{
   return _parse_if(context, buffer);
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
         context->if_ptr--;
         context->if_stack[context->if_ptr].value = 0;
         context->if_stack[context->if_ptr].u.condition = _macro_is_defined(context, input[i].data.identifier);
         context->if_value = _evaluate_if_stack(context);
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
         context->if_ptr--;
         context->if_stack[context->if_ptr].value = 0;
         context->if_stack[context->if_ptr].u.condition = !_macro_is_defined(context, input[i].data.identifier);
         context->if_value = _evaluate_if_stack(context);
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
                   struct sl_pp_token_buffer *buffer)
{
   if (_parse_else(context)) {
      return -1;
   }

   if (context->if_stack[context->if_ptr].u.condition) {
      context->if_ptr++;
      if (_parse_if(context, buffer)) {
         return -1;
      }
   }

   /* We are still in the #if block. */
   context->if_stack[context->if_ptr].u.went_thru_else = 0;

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
