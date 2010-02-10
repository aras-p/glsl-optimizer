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

#ifndef SL_PP_PUBLIC_H
#define SL_PP_PUBLIC_H


struct sl_pp_context;


#include "sl_pp_purify.h"
#include "sl_pp_token.h"


struct sl_pp_context *
sl_pp_context_create(const char *input,
                     const struct sl_pp_purify_options *options);

void
sl_pp_context_destroy(struct sl_pp_context *context);

const char *
sl_pp_context_error_message(const struct sl_pp_context *context);

void
sl_pp_context_error_position(const struct sl_pp_context *context,
                             unsigned int *file,
                             unsigned int *line);

int
sl_pp_context_add_extension(struct sl_pp_context *context,
                            const char *name);

int
sl_pp_context_add_predefined(struct sl_pp_context *context,
                             const char *name,
                             const char *value);

int
sl_pp_context_add_unique_str(struct sl_pp_context *context,
                             const char *str);

const char *
sl_pp_context_cstr(const struct sl_pp_context *context,
                   int offset);

int
sl_pp_version(struct sl_pp_context *context,
              unsigned int *version);

int
sl_pp_process_get(struct sl_pp_context *context,
                  struct sl_pp_token_info *output);

int
sl_pp_process(struct sl_pp_context *context,
              struct sl_pp_token_info **output);

#endif /* SL_PP_PUBLIC_H */
