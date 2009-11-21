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
#include "sl_pp_purify.h"


#define SL_PP_MAX_IF_NESTING  64

#define SL_PP_MAX_ERROR_MSG   1024

struct sl_pp_context {
   char *cstr_pool;
   unsigned int cstr_pool_max;
   unsigned int cstr_pool_len;
   struct sl_pp_dict dict;

   struct sl_pp_macro *macro;
   struct sl_pp_macro **macro_tail;

   unsigned int if_stack[SL_PP_MAX_IF_NESTING];
   unsigned int if_ptr;
   unsigned int if_value;

   char error_msg[SL_PP_MAX_ERROR_MSG];

   unsigned int line;
   unsigned int file;

   struct sl_pp_purify_state pure;

   char *getc_buf;
   unsigned int getc_buf_size;
   unsigned int getc_buf_capacity;
};

#endif /* SL_PP_CONTEXT_H */
