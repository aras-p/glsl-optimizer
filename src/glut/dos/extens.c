/*
 * DOS/DJGPP Mesa Utility Toolkit
 * Version:  1.0
 *
 * Copyright (C) 2005  Daniel Borca   All Rights Reserved.
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
 * DANIEL BORCA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <string.h>

#include "internal.h"


int APIENTRY
glutExtensionSupported (const char *extension)
{
   static const GLubyte *extensions = NULL;
   const GLubyte *last, *where;

   /* Extension names should not have spaces. */
   if (strchr(extension, ' ') || *extension == '\0') {
      return GL_FALSE;
   }

   /* Not my problem if you don't have a valid OpenGL context */
   if (!extensions) {
      extensions = glGetString(GL_EXTENSIONS);
   }
   if (!extensions) {
      return GL_FALSE;
   }

   /* Take care of sub-strings etc. */
   for (last = extensions;;) {
      if ((where = (GLubyte *)strstr((const char *)last, extension)) == NULL) {
         return GL_FALSE;
      }
      last = where + strlen(extension);
      if (where == extensions || *(where - 1) == ' ') {
         if (*last == ' ' || *last == '\0') {
            return GL_TRUE;
         }
      }
   }
}


GLUTproc APIENTRY
glutGetProcAddress (const char *procName)
{
   /* TODO - handle glut namespace */
   return (GLUTproc)DMesaGetProcAddress(procName);
}
