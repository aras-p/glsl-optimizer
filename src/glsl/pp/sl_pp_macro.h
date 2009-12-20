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

#ifndef SL_PP_MACRO_H
#define SL_PP_MACRO_H

#include "sl_pp_token.h"


struct sl_pp_context;
struct sl_pp_process_state;
struct sl_pp_token_buffer;

struct sl_pp_macro_formal_arg {
   int name;
   struct sl_pp_macro_formal_arg *next;
};

struct sl_pp_macro {
   int name;
   int num_args;                       /* -1 means no args, 0 means `()' */
   struct sl_pp_macro_formal_arg *arg;
   struct sl_pp_token_info *body;
   struct sl_pp_macro *next;
};

struct sl_pp_macro *
sl_pp_macro_new(void);

void
sl_pp_macro_free(struct sl_pp_macro *macro);

void
sl_pp_macro_reset(struct sl_pp_macro *macro);

enum sl_pp_macro_expand_behaviour {
   sl_pp_macro_expand_normal,
   sl_pp_macro_expand_mute,
   sl_pp_macro_expand_unknown_to_0
};

int
sl_pp_macro_expand(struct sl_pp_context *context,
                   struct sl_pp_token_buffer *tokens,
                   struct sl_pp_macro *local,
                   struct sl_pp_process_state *state,
                   enum sl_pp_macro_expand_behaviour behaviour);

#endif /* SL_PP_MACRO_H */
