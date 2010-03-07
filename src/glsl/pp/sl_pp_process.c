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


int
sl_pp_process_out(struct sl_pp_process_state *state,
                  const struct sl_pp_token_info *token)
{
   if (state->out_len >= state->out_max) {
      unsigned int new_max = state->out_max;

      if (new_max < 0x100) {
         new_max = 0x100;
      } else if (new_max < 0x10000) {
         new_max *= 2;
      } else {
         new_max += 0x10000;
      }

      state->out = realloc(state->out, new_max * sizeof(struct sl_pp_token_info));
      if (!state->out) {
         return -1;
      }
      state->out_max = new_max;
   }

   state->out[state->out_len++] = *token;
   return 0;
}

int
sl_pp_process_get(struct sl_pp_context *context,
                  struct sl_pp_token_info *output)
{
   if (!context->process_state.out) {
      if (context->line > 1) {
         struct sl_pp_token_info ti;

         ti.token = SL_PP_LINE;
         ti.data.line.lineno = context->line - 1;
         ti.data.line.fileno = context->file;
         if (sl_pp_process_out(&context->process_state, &ti)) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }

         ti.token = SL_PP_NEWLINE;
         if (sl_pp_process_out(&context->process_state, &ti)) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }
      }
   }

   for (;;) {
      struct sl_pp_token_info input;
      int found_eof = 0;

      if (context->process_state.out_len) {
         assert(context->process_state.out);
         *output = context->process_state.out[0];

         if (context->process_state.out_len > 1) {
            unsigned int i;

            for (i = 1; i < context->process_state.out_len; i++) {
               context->process_state.out[i - 1] = context->process_state.out[i];
            }
         }
         context->process_state.out_len--;

         return 0;
      }

      if (sl_pp_token_buffer_skip_white(&context->tokens, &input)) {
         return -1;
      }
      if (input.token == SL_PP_HASH) {
         if (sl_pp_token_buffer_skip_white(&context->tokens, &input)) {
            return -1;
         }
         switch (input.token) {
         case SL_PP_IDENTIFIER:
            {
               int name;
               int found_eol = 0;
               struct sl_pp_token_info endof;
               struct sl_pp_token_peek peek;
               int result = 0;

               /* Directive name. */
               name = input.data.identifier;

               if (sl_pp_token_buffer_skip_white(&context->tokens, &input)) {
                  return -1;
               }
               sl_pp_token_buffer_unget(&context->tokens, &input);

               if (sl_pp_token_peek_init(&peek, &context->tokens)) {
                  return -1;
               }

               while (!found_eol) {
                  if (sl_pp_token_peek_get(&peek, &input)) {
                     sl_pp_token_peek_destroy(&peek);
                     return -1;
                  }
                  switch (input.token) {
                  case SL_PP_NEWLINE:
                     /* Preserve newline just for the sake of line numbering. */
                     endof = input;
                     found_eol = 1;
                     break;

                  case SL_PP_EOF:
                     endof = input;
                     found_eof = 1;
                     found_eol = 1;
                     break;

                  default:
                     break;
                  }
               }

               if (name == context->dict._if) {
                  struct sl_pp_token_buffer buffer;

                  result = sl_pp_token_peek_to_buffer(&peek, &buffer);
                  if (result == 0) {
                     result = sl_pp_process_if(context, &buffer);
                     sl_pp_token_buffer_destroy(&buffer);
                  }
               } else if (name == context->dict.ifdef) {
                  result = sl_pp_process_ifdef(context, peek.tokens, 0, peek.size - 1);
               } else if (name == context->dict.ifndef) {
                  result = sl_pp_process_ifndef(context, peek.tokens, 0, peek.size - 1);
               } else if (name == context->dict.elif) {
                  struct sl_pp_token_buffer buffer;

                  result = sl_pp_token_peek_to_buffer(&peek, &buffer);
                  if (result == 0) {
                     result = sl_pp_process_elif(context, &buffer);
                     sl_pp_token_buffer_destroy(&buffer);
                  }
               } else if (name == context->dict._else) {
                  result = sl_pp_process_else(context, peek.tokens, 0, peek.size - 1);
               } else if (name == context->dict.endif) {
                  result = sl_pp_process_endif(context, peek.tokens, 0, peek.size - 1);
               } else if (context->if_value) {
                  if (name == context->dict.define) {
                     result = sl_pp_process_define(context, peek.tokens, 0, peek.size - 1);
                  } else if (name == context->dict.error) {
                     sl_pp_process_error(context, peek.tokens, 0, peek.size - 1);
                     result = -1;
                  } else if (name == context->dict.extension) {
                     result = sl_pp_process_extension(context, peek.tokens, 0, peek.size - 1, &context->process_state);
                  } else if (name == context->dict.line) {
                     struct sl_pp_token_buffer buffer;

                     result = sl_pp_token_peek_to_buffer(&peek, &buffer);
                     if (result == 0) {
                        result = sl_pp_process_line(context, &buffer, &context->process_state);
                        sl_pp_token_buffer_destroy(&buffer);
                     }
                  } else if (name == context->dict.pragma) {
                     result = sl_pp_process_pragma(context, peek.tokens, 0, peek.size - 1, &context->process_state);
                  } else if (name == context->dict.undef) {
                     result = sl_pp_process_undef(context, peek.tokens, 0, peek.size - 1);
                  } else {
                     strcpy(context->error_msg, "unrecognised directive name");
                     result = -1;
                  }
               }

               sl_pp_token_peek_commit(&peek);
               sl_pp_token_peek_destroy(&peek);

               if (result) {
                  return result;
               }

               if (sl_pp_process_out(&context->process_state, &endof)) {
                  strcpy(context->error_msg, "out of memory");
                  return -1;
               }
               context->line++;
            }
            break;

         case SL_PP_NEWLINE:
            /* Empty directive. */
            if (sl_pp_process_out(&context->process_state, &input)) {
               strcpy(context->error_msg, "out of memory");
               return -1;
            }
            context->line++;
            break;

         case SL_PP_EOF:
            /* Empty directive. */
            if (sl_pp_process_out(&context->process_state, &input)) {
               strcpy(context->error_msg, "out of memory");
               return -1;
            }
            found_eof = 1;
            break;

         default:
            strcpy(context->error_msg, "expected a directive name");
            return -1;
         }
      } else {
         int found_eol = 0;

         sl_pp_token_buffer_unget(&context->tokens, &input);

         while (!found_eol) {
            if (sl_pp_token_buffer_get(&context->tokens, &input)) {
               return -1;
            }

            switch (input.token) {
            case SL_PP_WHITESPACE:
               /* Drop whitespace all together at this point. */
               break;

            case SL_PP_NEWLINE:
               /* Preserve newline just for the sake of line numbering. */
               if (sl_pp_process_out(&context->process_state, &input)) {
                  strcpy(context->error_msg, "out of memory");
                  return -1;
               }
               context->line++;
               found_eol = 1;
               break;

            case SL_PP_EOF:
               if (sl_pp_process_out(&context->process_state, &input)) {
                  strcpy(context->error_msg, "out of memory");
                  return -1;
               }
               found_eof = 1;
               found_eol = 1;
               break;

            case SL_PP_IDENTIFIER:
               sl_pp_token_buffer_unget(&context->tokens, &input);
               if (sl_pp_macro_expand(context, &context->tokens, NULL, &context->process_state,
                                      context->if_value ? sl_pp_macro_expand_normal : sl_pp_macro_expand_mute)) {
                  return -1;
               }
               break;

            default:
               if (context->if_value) {
                  if (sl_pp_process_out(&context->process_state, &input)) {
                     strcpy(context->error_msg, "out of memory");
                     return -1;
                  }
               }
            }
         }
      }

      if (found_eof) {
         if (context->if_ptr != SL_PP_MAX_IF_NESTING) {
            strcpy(context->error_msg, "expected `#endif' directive");
            return -1;
         }
      }
   }
}

int
sl_pp_process(struct sl_pp_context *context,
              struct sl_pp_token_info **output)
{
   struct sl_pp_process_state state;

   memset(&state, 0, sizeof(state));
   for (;;) {
      struct sl_pp_token_info input;

      if (sl_pp_process_get(context, &input)) {
         free(state.out);
         return -1;
      }
      if (sl_pp_process_out(&state, &input)) {
         free(state.out);
         return -1;
      }
      if (input.token == SL_PP_EOF) {
         *output = state.out;
         return 0;
      }
   }
}
