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
#include "sl_pp_context.h"
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
                   struct sl_pp_token_buffer *tokens,
                   struct sl_pp_macro *local,
                   struct sl_pp_process_state *state,
                   enum sl_pp_macro_expand_behaviour behaviour)
{
   int mute = (behaviour == sl_pp_macro_expand_mute);
   struct sl_pp_token_info input;
   int macro_name;
   struct sl_pp_macro *macro = NULL;
   struct sl_pp_macro *actual_arg = NULL;
   unsigned int j;

   if (sl_pp_token_buffer_get(tokens, &input)) {
      return -1;
   }

   if (input.token != SL_PP_IDENTIFIER) {
      strcpy(context->error_msg, "expected an identifier");
      return -1;
   }

   macro_name = input.data.identifier;

   /* First look for predefined macros.
    */

   if (macro_name == context->dict.___LINE__) {
      if (!mute && _out_number(context, state, context->line)) {
         return -1;
      }
      return 0;
   }
   if (macro_name == context->dict.___FILE__) {
      if (!mute && _out_number(context, state, context->file)) {
         return -1;
      }
      return 0;
   }
   if (macro_name == context->dict.___VERSION__) {
      if (!mute && _out_number(context, state, 110)) {
         return -1;
      }
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
         return 0;
      }
   }

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
         if (sl_pp_process_out(state, &input)) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }
      }
      return 0;
   }

   if (macro->num_args >= 0) {
      if (sl_pp_token_buffer_skip_white(tokens, &input)) {
         return -1;
      }
      if (input.token != SL_PP_LPAREN) {
         strcpy(context->error_msg, "expected `('");
         return -1;
      }
      if (sl_pp_token_buffer_skip_white(tokens, &input)) {
         return -1;
      }
      sl_pp_token_buffer_unget(tokens, &input);
   }

   if (macro->num_args > 0) {
      struct sl_pp_macro_formal_arg *formal_arg = macro->arg;
      struct sl_pp_macro **pmacro = &actual_arg;

      for (j = 0; j < (unsigned int)macro->num_args; j++) {
         struct sl_pp_process_state arg_state;
         int done = 0;
         unsigned int paren_nesting = 0;
         struct sl_pp_token_info eof;

         memset(&arg_state, 0, sizeof(arg_state));

         while (!done) {
            if (sl_pp_token_buffer_get(tokens, &input)) {
               goto fail_arg;
            }
            switch (input.token) {
            case SL_PP_WHITESPACE:
               break;

            case SL_PP_COMMA:
               if (!paren_nesting) {
                  if (j < (unsigned int)macro->num_args - 1) {
                     done = 1;
                  } else {
                     strcpy(context->error_msg, "too many actual macro arguments");
                     goto fail_arg;
                  }
               } else {
                  if (sl_pp_process_out(&arg_state, &input)) {
                     strcpy(context->error_msg, "out of memory");
                     goto fail_arg;
                  }
               }
               break;

            case SL_PP_LPAREN:
               paren_nesting++;
               if (sl_pp_process_out(&arg_state, &input)) {
                  goto oom_arg;
               }
               break;

            case SL_PP_RPAREN:
               if (!paren_nesting) {
                  if (j == (unsigned int)macro->num_args - 1) {
                     done = 1;
                  } else {
                     strcpy(context->error_msg, "too few actual macro arguments");
                     goto fail_arg;
                  }
               } else {
                  paren_nesting--;
                  if (sl_pp_process_out(&arg_state, &input)) {
                     goto oom_arg;
                  }
               }
               break;

            case SL_PP_IDENTIFIER:
               sl_pp_token_buffer_unget(tokens, &input);
               if (sl_pp_macro_expand(context, tokens, local, &arg_state, sl_pp_macro_expand_normal)) {
                  goto fail_arg;
               }
               break;

            case SL_PP_EOF:
               strcpy(context->error_msg, "too few actual macro arguments");
               goto fail_arg;

            default:
               if (sl_pp_process_out(&arg_state, &input)) {
                  goto oom_arg;
               }
            }
         }

         eof.token = SL_PP_EOF;
         if (sl_pp_process_out(&arg_state, &eof)) {
            goto oom_arg;
         }

         *pmacro = sl_pp_macro_new();
         if (!*pmacro) {
            goto oom_arg;
         }

         (**pmacro).name = formal_arg->name;
         (**pmacro).body = arg_state.out;

         formal_arg = formal_arg->next;
         pmacro = &(**pmacro).next;

         continue;

oom_arg:
         strcpy(context->error_msg, "out of memory");
fail_arg:
         free(arg_state.out);
         goto fail;
      }
   }

   /* Right paren for non-empty argument list has already been eaten. */
   if (macro->num_args == 0) {
      if (sl_pp_token_buffer_skip_white(tokens, &input)) {
         goto fail;
      }
      if (input.token != SL_PP_RPAREN) {
         strcpy(context->error_msg, "expected `)'");
         goto fail;
      }
   }

   /* XXX: This is all wrong, we should be ungetting all tokens
    *      back to the main token buffer.
    */
   {
      struct sl_pp_token_buffer buffer;

      /* Seek to the end.
       */
      for (j = 0; macro->body[j].token != SL_PP_EOF; j++) {
      }
      j++;

      /* Create a context-less token buffer since we are not going to underrun
       * its internal buffer.
       */
      if (sl_pp_token_buffer_init(&buffer, NULL)) {
         strcpy(context->error_msg, "out of memory");
         goto fail;
      }

      /* Unget the tokens in reverse order so later they will be fetched correctly.
       */
      for (; j > 0; j--) {
         sl_pp_token_buffer_unget(&buffer, &macro->body[j - 1]);
      }

      /* Expand.
       */
      for (;;) {
         struct sl_pp_token_info input;

         sl_pp_token_buffer_get(&buffer, &input);
         switch (input.token) {
         case SL_PP_NEWLINE:
            if (sl_pp_process_out(state, &input)) {
               strcpy(context->error_msg, "out of memory");
               sl_pp_token_buffer_destroy(&buffer);
               goto fail;
            }
            break;

         case SL_PP_IDENTIFIER:
            sl_pp_token_buffer_unget(&buffer, &input);
            if (sl_pp_macro_expand(context, &buffer, actual_arg, state, behaviour)) {
               sl_pp_token_buffer_destroy(&buffer);
               goto fail;
            }
            break;

         case SL_PP_EOF:
            sl_pp_token_buffer_destroy(&buffer);
            sl_pp_macro_free(actual_arg);
            return 0;

         default:
            if (!mute) {
               if (sl_pp_process_out(state, &input)) {
                  strcpy(context->error_msg, "out of memory");
                  sl_pp_token_buffer_destroy(&buffer);
                  goto fail;
               }
            }
         }
      }
   }

fail:
   sl_pp_macro_free(actual_arg);
   return -1;
}
