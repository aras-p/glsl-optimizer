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
#include "sl_pp_process.h"
#include "sl_pp_public.h"


static void
skip_whitespace(const struct sl_pp_token_info *input,
                unsigned int *pi)
{
   while (input[*pi].token == SL_PP_WHITESPACE) {
      (*pi)++;
   }
}

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
sl_pp_process(struct sl_pp_context *context,
              const struct sl_pp_token_info *input,
              struct sl_pp_token_info **output)
{
   unsigned int i = 0;
   int found_eof = 0;
   struct sl_pp_process_state state;

   memset(&state, 0, sizeof(state));

   if (context->line > 1) {
      struct sl_pp_token_info ti;

      ti.token = SL_PP_LINE;
      ti.data.line.lineno = context->line - 1;
      ti.data.line.fileno = context->file;
      if (sl_pp_process_out(&state, &ti)) {
         strcpy(context->error_msg, "out of memory");
         return -1;
      }

      ti.token = SL_PP_NEWLINE;
      if (sl_pp_process_out(&state, &ti)) {
         strcpy(context->error_msg, "out of memory");
         return -1;
      }
   }

   while (!found_eof) {
      skip_whitespace(input, &i);
      if (input[i].token == SL_PP_HASH) {
         i++;
         skip_whitespace(input, &i);
         switch (input[i].token) {
         case SL_PP_IDENTIFIER:
            {
               int name;
               int found_eol = 0;
               unsigned int first;
               unsigned int last;
               struct sl_pp_token_info endof;

               /* Directive name. */
               name = input[i].data.identifier;
               i++;
               skip_whitespace(input, &i);

               first = i;

               while (!found_eol) {
                  switch (input[i].token) {
                  case SL_PP_NEWLINE:
                     /* Preserve newline just for the sake of line numbering. */
                     endof = input[i];
                     i++;
                     found_eol = 1;
                     break;

                  case SL_PP_EOF:
                     endof = input[i];
                     i++;
                     found_eof = 1;
                     found_eol = 1;
                     break;

                  default:
                     i++;
                  }
               }

               last = i - 1;

               if (name == context->dict._if) {
                  if (sl_pp_process_if(context, input, first, last)) {
                     return -1;
                  }
               } else if (name == context->dict.ifdef) {
                  if (sl_pp_process_ifdef(context, input, first, last)) {
                     return -1;
                  }
               } else if (name == context->dict.ifndef) {
                  if (sl_pp_process_ifndef(context, input, first, last)) {
                     return -1;
                  }
               } else if (name == context->dict.elif) {
                  if (sl_pp_process_elif(context, input, first, last)) {
                     return -1;
                  }
               } else if (name == context->dict._else) {
                  if (sl_pp_process_else(context, input, first, last)) {
                     return -1;
                  }
               } else if (name == context->dict.endif) {
                  if (sl_pp_process_endif(context, input, first, last)) {
                     return -1;
                  }
               } else if (context->if_value) {
                  if (name == context->dict.define) {
                     if (sl_pp_process_define(context, input, first, last)) {
                        return -1;
                     }
                  } else if (name == context->dict.error) {
                     sl_pp_process_error(context, input, first, last);
                     return -1;
                  } else if (name == context->dict.extension) {
                     if (sl_pp_process_extension(context, input, first, last, &state)) {
                        return -1;
                     }
                  } else if (name == context->dict.line) {
                     if (sl_pp_process_line(context, input, first, last, &state)) {
                        return -1;
                     }
                  } else if (name == context->dict.pragma) {
                     if (sl_pp_process_pragma(context, input, first, last, &state)) {
                        return -1;
                     }
                  } else if (name == context->dict.undef) {
                     if (sl_pp_process_undef(context, input, first, last)) {
                        return -1;
                     }
                  } else {
                     strcpy(context->error_msg, "unrecognised directive name");
                     return -1;
                  }
               }

               if (sl_pp_process_out(&state, &endof)) {
                  strcpy(context->error_msg, "out of memory");
                  return -1;
               }
               context->line++;
            }
            break;

         case SL_PP_NEWLINE:
            /* Empty directive. */
            if (sl_pp_process_out(&state, &input[i])) {
               strcpy(context->error_msg, "out of memory");
               return -1;
            }
            context->line++;
            i++;
            break;

         case SL_PP_EOF:
            /* Empty directive. */
            if (sl_pp_process_out(&state, &input[i])) {
               strcpy(context->error_msg, "out of memory");
               return -1;
            }
            i++;
            found_eof = 1;
            break;

         default:
            strcpy(context->error_msg, "expected a directive name");
            return -1;
         }
      } else {
         int found_eol = 0;

         while (!found_eol) {
            switch (input[i].token) {
            case SL_PP_WHITESPACE:
               /* Drop whitespace all together at this point. */
               i++;
               break;

            case SL_PP_NEWLINE:
               /* Preserve newline just for the sake of line numbering. */
               if (sl_pp_process_out(&state, &input[i])) {
                  strcpy(context->error_msg, "out of memory");
                  return -1;
               }
               context->line++;
               i++;
               found_eol = 1;
               break;

            case SL_PP_EOF:
               if (sl_pp_process_out(&state, &input[i])) {
                  strcpy(context->error_msg, "out of memory");
                  return -1;
               }
               i++;
               found_eof = 1;
               found_eol = 1;
               break;

            case SL_PP_IDENTIFIER:
               if (sl_pp_macro_expand(context, input, &i, NULL, &state,
                                      context->if_value ? sl_pp_macro_expand_normal : sl_pp_macro_expand_mute)) {
                  return -1;
               }
               break;

            default:
               if (context->if_value) {
                  if (sl_pp_process_out(&state, &input[i])) {
                     strcpy(context->error_msg, "out of memory");
                     return -1;
                  }
               }
               i++;
            }
         }
      }
   }

   if (context->if_ptr != SL_PP_MAX_IF_NESTING) {
      strcpy(context->error_msg, "expected `#endif' directive");
      return -1;
   }

   *output = state.out;
   return 0;
}
