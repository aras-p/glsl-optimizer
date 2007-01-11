/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "imports.h"
#include "slang_error.h"


static char ErrorText[10000];
static char FormattedErrorText[10000];
static int ErrorPos;


void
_slang_reset_error(void)
{
   ErrorText[0] = 0;
   ErrorPos = -1;
}


/**
 * Record an error message, if one hasn't been recorded already.
 */
void
_slang_record_error(const char *msg1, const char *msg2,
                    GLint pos, const char *file, int line)
{
   /* don't overwrite a previously recorded error */
   if (!ErrorText[0]) {
      _mesa_sprintf(ErrorText, "%s %s", msg1, msg2);
      ErrorPos = -1;
#ifdef DEBUG
      fprintf(stderr, "Mesa shader compile error: %s %s at %d (%s line %d)\n",
              msg1, msg2, pos, file, line);
#endif
   }
   abort();
}


/** 
 * Return formatted error text.
 */
const char *
_slang_error_text(void)
{
   /*
    * NVIDIA formats errors like this:
    *   (LINE_NUMBER) : error ERROR_CODE: ERROR_TEXT
    * Example:
    *   (7) : error C1048: invalid character 'P' in swizzle "P"
    */
   _mesa_sprintf(FormattedErrorText,
                 "(%d) : error: %s", ErrorPos, ErrorText);
   return FormattedErrorText;
}

