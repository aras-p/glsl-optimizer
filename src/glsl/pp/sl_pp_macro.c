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
#include <stdio.h>
#include <string.h>
#include "sl_pp_public.h"
#include "sl_pp_macro.h"
#include "sl_pp_process.h"


static void
_macro_init(struct sl_pp_macro *macro)
{
   macro->name = -1;
   macro->num_args = -1;
   macro->arg = NULL;
   macro->body = NULL;
}

struct sl_pp_macro *
sl_pp_macro_new(void)
{
   struct sl_pp_macro *macro;

   macro = calloc(1, sizeof(struct sl_pp_macro));
   if (macro) {
      _macro_init(macro);
   }
   return macro;
}

static void
_macro_destroy(struct sl_pp_macro *macro)
{
   struct sl_pp_macro_formal_arg *arg = macro->arg;

   while (arg) {
      struct sl_pp_macro_formal_arg *next_arg = arg->next;

      free(arg);
      arg = next_arg;
   }

   free(macro->body);
}

void
sl_pp_macro_free(struct sl_pp_macro *macro)
{
   while (macro) {
      struct sl_pp_macro *next_macro = macro->next;

      _macro_destroy(macro);
      free(macro);
      macro = next_macro;
   }
}

void
sl_pp_macro_reset(struct sl_pp_macro *macro)
{
   _macro_destroy(macro);
   _macro_init(macro);
}

static void
skip_whitespace(const struct sl_pp_token_info *input,
                unsigned int *pi)
{
   while (input[*pi].token == SL_PP_WHITESPACE) {
      (*pi)++;
   }
}

static int
_out_number(struct sl_pp_context *context,
            struct sl_pp_process_state *state,
            unsigned int number)
{
   char buf[32];
   struct sl_pp_token_info ti;

   sprintf(buf, "%u", number);

   ti.token = SL_PP_UINT;
   ti.data._uint = sl_pp_context_add_unique_str(context, buf);
   if (sl_pp_process_out(state, &ti)) {
      strcpy(context->error_msg, "out of memory");
      return -1;
   }

   return 0;
}

int
sl_pp_macro_expand(struct sl_pp_context *context,
                   const struct sl_pp_token_info *input,
                   unsigned int *pi,
                   struct sl_pp_macro *local,
                   struct sl_pp_process_state *state,
                   enum sl_pp_macro_expand_behaviour behaviour)
{
   int mute = (behaviour == sl_pp_macro_expand_mute);
   int macro_name;
   struct sl_pp_macro *macro = NULL;
   struct sl_pp_macro *actual_arg = NULL;
   unsigned int j;

   if (input[*pi].token != SL_PP_IDENTIFIER) {
      strcpy(context->error_msg, "expected an identifier");
      return -1;
   }

   macro_name = input[*pi].data.identifier;

   /* First look for predefined macros.
    */

   if (macro_name == context->dict.___LINE__) {
      if (!mute && _out_number(context, state, context->line)) {
         return -1;
      }
      (*pi)++;
      return 0;
   }
   if (macro_name == context->dict.___FILE__) {
      if (!mute && _out_number(context, state, context->file)) {
         return -1;
      }
      (*pi)++;
      return 0;
   }
   if (macro_name == context->dict.___VERSION__) {
      if (!mute && _out_number(context, state, 110)) {
         return -1;
      }
      (*pi)++;
      return 0;
   }

   for (j = 0; j < context->num_predefined; j++) {
      if (macro_name == context->predefined[j].name) {
         if (!mute) {
            struct sl_pp_token_info ti;

            ti.token = SL_PP_UINT;
            ti.data._uint = context->predefined[j].value;
            if (sl_pp_process_out(state, &ti)) {
               strcpy(context->error_msg, "out of memory");
               return -1;
            }
         }
         (*pi)++;
         return 0;
      }
   }

   /* Replace extension names with 1.
    */
   for (j = 0; j < context->num_extensions; j++) {
      if (macro_name == context->extensions[j].name) {
         if (!mute && _out_number(context, state, 1)) {
            return -1;
         }
         (*pi)++;
         return 0;
      }
   }

   /* TODO: For FEATURE_es2_glsl, expand to 1 the following symbols.
    *       GL_ES
    *       GL_FRAGMENT_PRECISION_HIGH
    */

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
      if (behaviour == sl_pp_macro_expand_unknown_to_0) {
         if (_out_number(context, state, 0)) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }
      } else if (!mute) {
         if (sl_pp_process_out(state, &input[*pi])) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }
      }
      (*pi)++;
      return 0;
   }

   (*pi)++;

   if (macro->num_args >= 0) {
      skip_whitespace(input, pi);
      if (input[*pi].token != SL_PP_LPAREN) {
         strcpy(context->error_msg, "expected `('");
         return -1;
      }
      (*pi)++;
      skip_whitespace(input, pi);
   }

   if (macro->num_args > 0) {
      struct sl_pp_macro_formal_arg *formal_arg = macro->arg;
      struct sl_pp_macro **pmacro = &actual_arg;

      for (j = 0; j < (unsigned int)macro->num_args; j++) {
         struct sl_pp_process_state arg_state;
         unsigned int i;
         int done = 0;
         unsigned int paren_nesting = 0;
         struct sl_pp_token_info eof;

         memset(&arg_state, 0, sizeof(arg_state));

         for (i = *pi; !done;) {
            switch (input[i].token) {
            case SL_PP_WHITESPACE:
               i++;
               break;

            case SL_PP_COMMA:
               if (!paren_nesting) {
                  if (j < (unsigned int)macro->num_args - 1) {
                     done = 1;
                     i++;
                  } else {
                     strcpy(context->error_msg, "too many actual macro arguments");
                     return -1;
                  }
               } else {
                  if (sl_pp_process_out(&arg_state, &input[i])) {
                     strcpy(context->error_msg, "out of memory");
                     free(arg_state.out);
                     return -1;
                  }
                  i++;
               }
               break;

            case SL_PP_LPAREN:
               paren_nesting++;
               if (sl_pp_process_out(&arg_state, &input[i])) {
                  strcpy(context->error_msg, "out of memory");
                  free(arg_state.out);
                  return -1;
               }
               i++;
               break;

            case SL_PP_RPAREN:
               if (!paren_nesting) {
                  if (j == (unsigned int)macro->num_args - 1) {
                     done = 1;
                     i++;
                  } else {
                     strcpy(context->error_msg, "too few actual macro arguments");
                     return -1;
                  }
               } else {
                  paren_nesting--;
                  if (sl_pp_process_out(&arg_state, &input[i])) {
                     strcpy(context->error_msg, "out of memory");
                     free(arg_state.out);
                     return -1;
                  }
                  i++;
               }
               break;

            case SL_PP_IDENTIFIER:
               if (sl_pp_macro_expand(context, input, &i, local, &arg_state, sl_pp_macro_expand_normal)) {
                  free(arg_state.out);
                  return -1;
               }
               break;

            case SL_PP_EOF:
               strcpy(context->error_msg, "too few actual macro arguments");
               return -1;

            default:
               if (sl_pp_process_out(&arg_state, &input[i])) {
                  strcpy(context->error_msg, "out of memory");
                  free(arg_state.out);
                  return -1;
               }
               i++;
            }
         }

         (*pi) = i;

         eof.token = SL_PP_EOF;
         if (sl_pp_process_out(&arg_state, &eof)) {
            strcpy(context->error_msg, "out of memory");
            free(arg_state.out);
            return -1;
         }

         *pmacro = sl_pp_macro_new();
         if (!*pmacro) {
            strcpy(context->error_msg, "out of memory");
            free(arg_state.out);
            return -1;
         }

         (**pmacro).name = formal_arg->name;
         (**pmacro).body = arg_state.out;

         formal_arg = formal_arg->next;
         pmacro = &(**pmacro).next;
      }
   }

   /* Right paren for non-empty argument list has already been eaten. */
   if (macro->num_args == 0) {
      skip_whitespace(input, pi);
      if (input[*pi].token != SL_PP_RPAREN) {
         strcpy(context->error_msg, "expected `)'");
         return -1;
      }
      (*pi)++;
   }

   for (j = 0;;) {
      switch (macro->body[j].token) {
      case SL_PP_NEWLINE:
         if (sl_pp_process_out(state, &macro->body[j])) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }
         j++;
         break;

      case SL_PP_IDENTIFIER:
         if (sl_pp_macro_expand(context, macro->body, &j, actual_arg, state, behaviour)) {
            return -1;
         }
         break;

      case SL_PP_EOF:
         sl_pp_macro_free(actual_arg);
         return 0;

      default:
         if (!mute) {
            if (sl_pp_process_out(state, &macro->body[j])) {
               strcpy(context->error_msg, "out of memory");
               return -1;
            }
         }
         j++;
      }
   }
}
