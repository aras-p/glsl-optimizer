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
#include "sl_pp_purify.h"


/*
 * Preprocessor purifier performs the following tasks.
 * - Convert all variants of newlines into a Unix newline.
 * - Merge continued lines into a single long line.
 * - Remove line comments and replace block comments with whitespace.
 */


static unsigned int
_purify_newline(const char *input,
                char *out)
{
   if (input[0] == '\n') {
      *out = '\n';
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
                  char *out)
{
   unsigned int eaten = 0;

   for (;;) {
      if (input[0] == '\\') {
         char next;
         unsigned int next_eaten;

         eaten++;
         input++;

         next_eaten = _purify_newline(input, &next);
         if (next == '\n') {
            /*
             * If this is really a line continuation sequence, eat
             * it and do not exit the loop.
             */
            eaten += next_eaten;
            input += next_eaten;
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
         eaten += _purify_newline(input, out);
         break;
      }
   }
   return eaten;
}


static unsigned int
_purify_comment(const char *input,
                char *out)
{
   unsigned int eaten;
   char curr;

   eaten = _purify_backslash(input, &curr);
   input += eaten;
   if (curr == '/') {
      char next;
      unsigned int next_eaten;

      next_eaten = _purify_backslash(input, &next);
      if (next == '/') {
         eaten += next_eaten;
         input += next_eaten;

         /* Replace a line comment with either a newline or nil. */
         for (;;) {
            next_eaten = _purify_backslash(input, &next);
            eaten += next_eaten;
            input += next_eaten;
            if (next == '\n' || next == '\0') {
               *out = next;
               return eaten;
            }
         }
      } else if (next == '*') {
         eaten += next_eaten;
         input += next_eaten;

         /* Replace a block comment with a whitespace. */
         for (;;) {
            next_eaten = _purify_backslash(input, &next);
            eaten += next_eaten;
            input += next_eaten;
            while (next == '*') {
               next_eaten = _purify_backslash(input, &next);
               eaten += next_eaten;
               input += next_eaten;
               if (next == '/') {
                  *out = ' ';
                  return eaten;
               }
            }
         }
      }
   }
   *out = curr;
   return eaten;
}


int
sl_pp_purify(const char *input,
             const struct sl_pp_purify_options *options,
             char **output)
{
   char *out = NULL;
   unsigned int out_len = 0;
   unsigned int out_max = 0;

   for (;;) {
      char c;

      input += _purify_comment(input, &c);

      if (out_len >= out_max) {
         unsigned int new_max = out_max;

         if (new_max < 0x100) {
            new_max = 0x100;
         } else if (new_max < 0x10000) {
            new_max *= 2;
         } else {
            new_max += 0x10000;
         }

         out = realloc(out, new_max);
         if (!out) {
            return -1;
         }
         out_max = new_max;
      }

      out[out_len++] = c;

      if (c == '\0') {
         break;
      }
   }

   *output = out;
   return 0;
}
