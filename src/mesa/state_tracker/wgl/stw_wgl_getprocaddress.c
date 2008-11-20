/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#define _GDI32_

#include <windows.h>
#include "glapi/glapi.h"
#include "stw_wgl_arbextensionsstring.h"
#include "stw_wgl_arbpixelformat.h"

struct extension_entry
{
   const char *name;
   PROC proc;
};

#define EXTENTRY(P) { #P, (PROC) P }

static struct extension_entry extension_entries[] = {

   /* WGL_ARB_extensions_string */
   EXTENTRY( wglGetExtensionsStringARB ),

   /* WGL_ARB_pixel_format */
   EXTENTRY( wglChoosePixelFormatARB ),
   EXTENTRY( wglGetPixelFormatAttribfvARB ),
   EXTENTRY( wglGetPixelFormatAttribivARB ),

   { NULL, NULL }
};

WINGDIAPI PROC APIENTRY
wglGetProcAddress(
   LPCSTR lpszProc )
{
   struct extension_entry *entry;

   PROC p = (PROC) _glapi_get_proc_address( (const char *) lpszProc );
   if (p)
      return p;

   for (entry = extension_entries; entry->name; entry++)
      if (strcmp( lpszProc, entry->name ) == 0)
         return entry->proc;

   return NULL;
}
