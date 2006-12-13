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

#ifndef SLANG_ERROR_H
#define SLANG_ERROR_H


extern void
_slang_reset_error(void);


extern void
_slang_record_error(const char *msg1, const char *msg2,
                    GLint pos, const char *file, int line);


extern const char *
_slang_error_text(void);


/**
 * Record a compilation error, single string message.
 */
#define RETURN_ERROR(MSG, POS)                             \
do {                                                       \
   _slang_record_error(MSG, "", POS, __FILE__, __LINE__);  \
   return GL_FALSE;                                        \
} while (0)


/**
 * Record a compilation error, two-string message.
 */
#define RETURN_ERROR2(MSG1, MSG2, POS)                        \
do {                                                          \
   _slang_record_error(MSG1, MSG2, POS, __FILE__, __LINE__);  \
   return GL_FALSE;                                           \
} while (0)


/**
 * Record a nil error.  Either a real error message or out of memory should
 * have already been recorded.
 */
#define RETURN_NIL()                                            \
do {                                                            \
   _slang_record_error("unknown", "", -1, __FILE__, __LINE__);  \
   return GL_FALSE;                                             \
} while (0)


/**
 * Used to report an out of memory condition.
 */
#define RETURN_OUT_OF_MEMORY()                                        \
do {                                                                  \
   _slang_record_error("Out of memory", "", -1, __FILE__, __LINE__);  \
   return GL_FALSE;                                                   \
} while (0)




#endif /* SLANG_ERROR_H */
