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

#ifndef SL_PP_CONTEXT_H
#define SL_PP_CONTEXT_H

#include "sl_pp_dict.h"
#include "sl_pp_macro.h"
#include "sl_pp_process.h"
#include "sl_pp_purify.h"
#include "sl_pp_token_util.h"


#define SL_PP_MAX_IF_NESTING  64

#define SL_PP_MAX_ERROR_MSG   1024

#define SL_PP_MAX_EXTENSIONS  16

#define SL_PP_MAX_PREDEFINED  16

struct sl_pp_extension {
   int name;   /*< GL_VENDOR_extension_name */
};

struct sl_pp_predefined {
   int name;
   int value;
};

union sl_pp_if_state {
   struct {
      unsigned int condition:1;
      unsigned int went_thru_else:1;
      unsigned int had_true_cond:1;
   } u;
   unsigned int value;
};

struct sl_pp_context {
   char *cstr_pool;
   unsigned int cstr_pool_max;
   unsigned int cstr_pool_len;
   struct sl_pp_dict dict;

   struct sl_pp_macro *macro;
   struct sl_pp_macro **macro_tail;

   struct sl_pp_extension extensions[SL_PP_MAX_EXTENSIONS];
   unsigned int num_extensions;

   struct sl_pp_predefined predefined[SL_PP_MAX_PREDEFINED];
   unsigned int num_predefined;

   union sl_pp_if_state if_stack[SL_PP_MAX_IF_NESTING];
   unsigned int if_ptr;
   unsigned int if_value;

   char error_msg[SL_PP_MAX_ERROR_MSG];
   unsigned int error_line;

   unsigned int line;
   unsigned int file;

   struct sl_pp_purify_state pure;

   char *getc_buf;
   unsigned int getc_buf_size;
   unsigned int getc_buf_capacity;

   struct sl_pp_token_buffer tokens;

   struct sl_pp_process_state process_state;
};

#endif /* SL_PP_CONTEXT_H */
