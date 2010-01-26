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
#include <stdarg.h>
#include <stdio.h>
#include "sl_pp_purify.h"


/*
 * Preprocessor purifier performs the following tasks.
 * - Convert all variants of newlines into a Unix newline.
 * - Merge continued lines into a single long line.
 * - Remove line comments and replace block comments with whitespace.
 */


static unsigned int
_purify_newline(const char *input,
                char *out,
                unsigned int *current_line)
{
   if (input[0] == '\n') {
      *out = '\n';
      (*current_line)++;
      if (input[1] == '\r') {
         /*
          * The GLSL spec is not explicit about whether this
          * combination is a valid newline or not.
          * Let's assume it is acceptable.
          */
         return 2;
      }
      return 1;
   }
   if (input[0] == '\r') {
      *out = '\n';
      (*current_line)++;
      if (input[1] == '\n') {
         return 2;
      }
      return 1;
   }
   *out = input[0];
   return 1;
}


static unsigned int
_purify_backslash(const char *input,
                  char *out,
                  unsigned int *current_line)
{
   unsigned int eaten = 0;

   for (;;) {
      if (input[0] == '\\') {
         char next;
         unsigned int next_eaten;
         unsigned int next_line = *current_line;

         eaten++;
         input++;

         next_eaten = _purify_newline(input, &next, &next_line);
         if (next == '\n') {
            /*
             * If this is really a line continuation sequence, eat
             * it and do not exit the loop.
             */
            eaten += next_eaten;
            input += next_eaten;
            *current_line = next_line;
         } else {
            /*
             * It is an error to put anything between a backslash
             * and a newline and still expect it to behave like a line
             * continuation sequence.
             * Even if it is an innocent whitespace.
             */
            *out = '\\';
            break;
         }
      } else {
         eaten += _purify_newline(input, out, current_line);
         break;
      }
   }
   return eaten;
}


static void
_report_error(char *buf,
              unsigned int cbbuf,
              const char *msg,
              ...)
{
   va_list args;

   va_start(args, msg);
   vsnprintf(buf, cbbuf, msg, args);
   va_end(args);
}


void
sl_pp_purify_state_init(struct sl_pp_purify_state *state,
                        const char *input,
                        const struct sl_pp_purify_options *options)
{
   state->options = *options;
   state->input = input;
   state->current_line = 1;
   state->inside_c_comment = 0;
}


static unsigned int
_purify_comment(struct sl_pp_purify_state *state,
                char *output,
                unsigned int *current_line,
                char *errormsg,
                unsigned int cberrormsg)
{
   for (;;) {
      unsigned int eaten;
      char next;

      eaten = _purify_backslash(state->input, &next, current_line);
      state->input += eaten;
      while (next == '*') {
         eaten = _purify_backslash(state->input, &next, current_line);
         state->input += eaten;
         if (next == '/') {
            *output = ' ';
            state->inside_c_comment = 0;
            return 1;
         }
      }
      if (next == '\n') {
         *output = '\n';
         state->inside_c_comment = 1;
         return 1;
      }
      if (next == '\0') {
         _report_error(errormsg, cberrormsg, "expected `*/' but end of translation unit found");
         return 0;
      }
   }
}


unsigned int
sl_pp_purify_getc(struct sl_pp_purify_state *state,
                  char *output,
                  unsigned int *current_line,
                  char *errormsg,
                  unsigned int cberrormsg)
{
   unsigned int eaten;

   if (state->inside_c_comment) {
      return _purify_comment(state, output, current_line, errormsg, cberrormsg);
   }

   eaten = _purify_backslash(state->input, output, current_line);
   state->input += eaten;
   if (*output == '/') {
      char next;
      unsigned int next_line = *current_line;

      eaten = _purify_backslash(state->input, &next, &next_line);
      if (next == '/') {
         state->input += eaten;
         *current_line = next_line;

         /* Replace a line comment with either a newline or nil. */
         for (;;) {
            eaten = _purify_backslash(state->input, &next, current_line);
            state->input += eaten;
            if (next == '\n' || next == '\0') {
               *output = next;
               return eaten;
            }
         }
      } else if (next == '*') {
         state->input += eaten;
         *current_line = next_line;

         return _purify_comment(state, output, current_line, errormsg, cberrormsg);
      }
   }
   return eaten;
}


struct out_buf {
   char *out;
   unsigned int len;
   unsigned int capacity;
   unsigned int current_line;
   char *errormsg;
   unsigned int cberrormsg;
};


static int
_out_buf_putc(struct out_buf *obuf,
              char c)
{
   if (obuf->len >= obuf->capacity) {
      unsigned int new_max = obuf->capacity;

      if (new_max < 0x100) {
         new_max = 0x100;
      } else if (new_max < 0x10000) {
         new_max *= 2;
      } else {
         new_max += 0x10000;
      }

      obuf->out = realloc(obuf->out, new_max);
      if (!obuf->out) {
         _report_error(obuf->errormsg, obuf->cberrormsg, "out of memory");
         return -1;
      }
      obuf->capacity = new_max;
   }

   obuf->out[obuf->len++] = c;

   return 0;
}


int
sl_pp_purify(const char *input,
             const struct sl_pp_purify_options *options,
             char **output,
             char *errormsg,
             unsigned int cberrormsg,
             unsigned int *errorline)
{
   struct out_buf obuf;
   struct sl_pp_purify_state state;

   obuf.out = NULL;
   obuf.len = 0;
   obuf.capacity = 0;
   obuf.current_line = 1;
   obuf.errormsg = errormsg;
   obuf.cberrormsg = cberrormsg;

   sl_pp_purify_state_init(&state, input, options);

   for (;;) {
      unsigned int eaten;
      char c;

      eaten = sl_pp_purify_getc(&state, &c, &obuf.current_line, errormsg, cberrormsg);
      if (!eaten) {
         *errorline = obuf.current_line;
         return -1;
      }
      if (_out_buf_putc(&obuf, c)) {
         *errorline = obuf.current_line;
         return -1;
      }

      if (c == '\0') {
         break;
      }
   }

   *output = obuf.out;
   return 0;
}
