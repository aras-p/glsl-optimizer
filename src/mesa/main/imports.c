/* $Id: imports.c,v 1.9 2001/03/27 19:18:02 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


/*
 * Imports are functions which the device driver or window system or
 * operating system provides to the core renderer.  The core renderer (Mesa)
 * will call these functions in order to do memory allocation, simple I/O,
 * etc.
 *
 * Some drivers will need to implement these functions themselves but
 * many (most?) Mesa drivers will be fine using these.
 *
 * A server-side GL renderer will likely not use these functions since
 * the renderer should use the XFree86-wrapped system calls.
 */


#include "glheader.h"
#include "imports.h"
#include "mem.h"
#include "mtypes.h"


static void *
_mesa_Malloc(__GLcontext *gc, size_t size)
{
   return MALLOC(size);
}

static void *
_mesa_Calloc(__GLcontext *gc, size_t numElem, size_t elemSize)
{
   return CALLOC(numElem * elemSize);
}

static void *
_mesa_Realloc(__GLcontext *gc, void *oldAddr, size_t newSize)
{
   return realloc(oldAddr, newSize);
}

static void
_mesa_Free(__GLcontext *gc, void *addr)
{
   FREE(addr);
}

/* Must be before '#undef getenv' for inclusion in XFree86.
 */
static char *
_mesa_getenv(__GLcontext *gc, const char *var)
{
   (void) gc;
   return getenv(var);
}

static void
_mesa_warning(__GLcontext *gc, char *str)
{
   GLboolean debug;
#ifdef DEBUG
   debug = GL_TRUE;
#else
/* Whacko XFree86 macro:
 */
#ifdef getenv
#undef getenv
#endif
   if (gc->imports.getenv(gc, "MESA_DEBUG")) {
      debug = GL_TRUE;
   }
   else {
      debug = GL_FALSE;
   }
#endif
   if (debug) {
      fprintf(stderr, "Mesa warning: %s\n", str);
   }
}

static void
_mesa_fatal(__GLcontext *gc, char *str)
{
   fprintf(stderr, "%s\n", str);
   abort();
}

static int
_mesa_atoi(__GLcontext *gc, const char *str)
{
   (void) gc;
   return atoi(str);
}

static int
_mesa_sprintf(__GLcontext *gc, char *str, const char *fmt, ...)
{
   /* XXX fix this */
   return sprintf(str, fmt);
}

static void *
_mesa_fopen(__GLcontext *gc, const char *path, const char *mode)
{
   return fopen(path, mode);
}

static int
_mesa_fclose(__GLcontext *gc, void *stream)
{
   return fclose((FILE *) stream);
}

static int
_mesa_fprintf(__GLcontext *gc, void *stream, const char *fmt, ...)
{
   /* XXX fix this */
   return fprintf((FILE *) stream, fmt);
}

/* XXX this really is driver-specific and can't be here */
static __GLdrawablePrivate *
_mesa_GetDrawablePrivate(__GLcontext *gc)
{
   return NULL;
}


void
_mesa_InitDefaultImports(__GLimports *imports, void *driverCtx, void *other)
{
   imports->malloc = _mesa_Malloc;
   imports->calloc = _mesa_Calloc;
   imports->realloc = _mesa_Realloc;
   imports->free = _mesa_Free;
   imports->warning = _mesa_warning;
   imports->fatal = _mesa_fatal;
   imports->getenv = _mesa_getenv;
   imports->atoi = _mesa_atoi;
   imports->sprintf = _mesa_sprintf;
   imports->fopen = _mesa_fopen;
   imports->fclose = _mesa_fclose;
   imports->fprintf = _mesa_fprintf;
   imports->getDrawablePrivate = _mesa_GetDrawablePrivate;
/*     imports->wscx = driverCtx; */
   imports->other = driverCtx;
}
