/*
 * Mesa 3-D graphics library
 * Version:  3.4
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * DOS/DJGPP glut driver v1.5 for Mesa
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <string.h>

#include <GL/glut.h>

#include "GL/dmesa.h"


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
