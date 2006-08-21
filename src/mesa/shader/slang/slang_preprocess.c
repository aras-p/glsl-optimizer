/*
 * Mesa 3-D graphics library
 * Version:  6.5
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

/**
 * \file slang_preprocess.c
 * slang preprocessor
 * \author Michal Krol
 */

#include "imports.h"
#include "grammar_mesa.h"
#include "slang_preprocess.h"

static const char *slang_pp_version_syn =
#include "library/slang_pp_version_syn.h"
;

static GLvoid
grammar_error_to_log (slang_info_log *log)
{
   char buf[1024];
   int pos;

   grammar_get_last_error ((byte *) (buf), sizeof (buf), &pos);
   slang_info_log_error (log, buf);
}

GLboolean
_slang_preprocess_version (const char *text, GLuint *version, GLuint *eaten, slang_info_log *log)
{
   grammar id;
   byte *prod, *I;
   unsigned int size;

   id = grammar_load_from_text ((const byte *) (slang_pp_version_syn));
   if (id == 0) {
      grammar_error_to_log (log);
      return GL_FALSE;
   }

   if (!grammar_fast_check (id, (const byte *) (text), &prod, &size, 8)) {
      grammar_error_to_log (log);
      grammar_destroy (id);
      return GL_FALSE;
   }

   /* there can be multiple #version directives - grab the last one */
   I = &prod[size - 6];
   *version = (GLuint) (I[0]) + (GLuint) (I[1]) * 100;
   *eaten = (GLuint) (I[2]) + ((GLuint) (I[3]) << 8) + ((GLuint) (I[4]) << 16) + ((GLuint) (I[5]) << 24);

   grammar_destroy (id);
   grammar_alloc_free (prod);
   return GL_TRUE;
}

