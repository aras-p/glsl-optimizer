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

#ifndef SL_PP_PURIFY_H
#define SL_PP_PURIFY_H

struct sl_pp_purify_options {
   unsigned int preserve_columns:1;
   unsigned int tab_width:4;
};

int
sl_pp_purify(const char *input,
             const struct sl_pp_purify_options *options,
             char **output,
             char *errormsg,
             unsigned int cberrormsg,
             unsigned int *errorline);

struct sl_pp_purify_state {
   struct sl_pp_purify_options options;
   const char *input;
   unsigned int current_line;
   unsigned int inside_c_comment:1;
};

void
sl_pp_purify_state_init(struct sl_pp_purify_state *state,
                        const char *input,
                        const struct sl_pp_purify_options *options);

unsigned int
sl_pp_purify_getc(struct sl_pp_purify_state *state,
                  char *output,
                  unsigned int *current_line,
                  char *errormsg,
                  unsigned int cberrormsg);

#endif /* SL_PP_PURIFY_H */
