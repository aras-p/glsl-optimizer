/* $Id: imports.c,v 1.20 2002/10/15 15:36:26 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
 * Imports are services which the device driver or window system or
 * operating system provides to the core renderer.  The core renderer (Mesa)
 * will call these functions in order to do memory allocation, simple I/O,
 * etc.
 *
 * Some drivers will want to override/replace this file with something
 * specialized, but most Mesa drivers will be able to call
 *_mesa_init_default_imports() and go with what's here.
 *
 * Eventually, I'd like to move most of the stuff in glheader.h and mem.[ch]
 * into imports.[ch].  Then we'll really have one, single place where
 * all OS-related dependencies are isolated.
 */


#include "glheader.h"
#include "mtypes.h"
#include "context.h"
#include "imports.h"
#include "mem.h"

#define MAXSTRING 4000  /* for vsnprintf() */

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

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
char * CAPI
_mesa_getenv(__GLcontext *gc, const char *var)
{
   (void) gc;
#ifdef XFree86LOADER
   return xf86getenv(var);
#else
   return getenv(var);
#endif
}


static void
warning(__GLcontext *gc, char *str)
{
   GLboolean debug;
#ifdef DEBUG
   debug = GL_TRUE;
#else
   if (_mesa_getenv(gc , "MESA_DEBUG"))
      debug = GL_TRUE;
   else
      debug = GL_FALSE;
#endif
   if (debug) {
      fprintf(stderr, "Mesa warning: %s\n", str);
   }
}


void
_mesa_fatal(__GLcontext *gc, char *str)
{
   (void) gc;
   fprintf(stderr, "%s\n", str);
   abort();
}


static int CAPI
_mesa_atoi(__GLcontext *gc, const char *str)
{
   (void) gc;
   return atoi(str);
}


int CAPI
_mesa_sprintf(__GLcontext *gc, char *str, const char *fmt, ...)
{
   int r;
   va_list args;
   va_start( args, fmt );  
   r = vsprintf( str, fmt, args );
   va_end( args );
   return r;
}


void * CAPI
_mesa_fopen(__GLcontext *gc, const char *path, const char *mode)
{
   return fopen(path, mode);
}


int CAPI
_mesa_fclose(__GLcontext *gc, void *stream)
{
   return fclose((FILE *) stream);
}


int CAPI
_mesa_fprintf(__GLcontext *gc, void *stream, const char *fmt, ...)
{
   int r;
   va_list args;
   va_start( args, fmt );  
   r = vfprintf( (FILE *) stream, fmt, args );
   va_end( args );
   return r;
}


/* XXX this really is driver-specific and can't be here */
static __GLdrawablePrivate *
_mesa_GetDrawablePrivate(__GLcontext *gc)
{
   return NULL;
}



void
_mesa_warning(__GLcontext *gc, const char *fmtString, ...)
{
   char str[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   (void) vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );
   warning(gc, str);
}


/*
 * This function is called when the Mesa user has stumbled into a code
 * path which may not be implemented fully or correctly.
 */
void
_mesa_problem( const GLcontext *ctx, const char *s )
{
   if (ctx) {
      ctx->imports.fprintf((GLcontext *) ctx, stderr, "Mesa implementation error: %s\n", s);
#ifdef XF86DRI
      ctx->imports.fprintf((GLcontext *) ctx, stderr, "Please report to the DRI bug database at dri.sourceforge.net\n");
#else
      ctx->imports.fprintf((GLcontext *) ctx, stderr, "Please report to the Mesa bug database at www.mesa3d.org\n" );
#endif
   }
   else {
      /* what can we do if we don't have a context???? */
      fprintf( stderr, "Mesa implementation error: %s\n", s );
#ifdef XF86DRI
      fprintf( stderr, "Please report to the DRI bug database at dri.sourceforge.net\n");
#else
      fprintf( stderr, "Please report to the Mesa bug database at www.mesa3d.org\n" );
#endif
   }
}


/*
 * If in debug mode, print error message to stdout.
 * Also, record the error code by calling _mesa_record_error().
 * Input:  ctx - the GL context
 *         error - the error value
 *         fmtString - printf-style format string, followed by optional args
 */
void
_mesa_error( GLcontext *ctx, GLenum error, const char *fmtString, ... )
{
   const char *debugEnv;
   GLboolean debug;

   debugEnv = _mesa_getenv(ctx, "MESA_DEBUG");

#ifdef DEBUG
   if (debugEnv && strstr(debugEnv, "silent"))
      debug = GL_FALSE;
   else
      debug = GL_TRUE;
#else
   if (debugEnv)
      debug = GL_TRUE;
   else
      debug = GL_FALSE;
#endif

   if (debug) {
      va_list args;
      char where[MAXSTRING];
      const char *errstr;

      va_start( args, fmtString );  
      vsnprintf( where, MAXSTRING, fmtString, args );
      va_end( args );

      switch (error) {
	 case GL_NO_ERROR:
	    errstr = "GL_NO_ERROR";
	    break;
	 case GL_INVALID_VALUE:
	    errstr = "GL_INVALID_VALUE";
	    break;
	 case GL_INVALID_ENUM:
	    errstr = "GL_INVALID_ENUM";
	    break;
	 case GL_INVALID_OPERATION:
	    errstr = "GL_INVALID_OPERATION";
	    break;
	 case GL_STACK_OVERFLOW:
	    errstr = "GL_STACK_OVERFLOW";
	    break;
	 case GL_STACK_UNDERFLOW:
	    errstr = "GL_STACK_UNDERFLOW";
	    break;
	 case GL_OUT_OF_MEMORY:
	    errstr = "GL_OUT_OF_MEMORY";
	    break;
         case GL_TABLE_TOO_LARGE:
            errstr = "GL_TABLE_TOO_LARGE";
            break;
	 default:
	    errstr = "unknown";
	    break;
      }
      _mesa_debug(ctx, "Mesa user error: %s in %s\n", errstr, where);
   }

   _mesa_record_error(ctx, error);
}  


/*
 * Call this to report debug information.  Uses stderr.
 */
void
_mesa_debug( const GLcontext *ctx, const char *fmtString, ... )
{
   char s[MAXSTRING];
   va_list args;
   va_start(args, fmtString);
   vsnprintf(s, MAXSTRING, fmtString, args);
   if (ctx)
      (void) ctx->imports.fprintf( (__GLcontext *) ctx, stderr, s );
   else
      fprintf( stderr, s );
   va_end(args);
}


/*
 * A wrapper for printf.  Uses stdout.
 */
void
_mesa_printf( const GLcontext *ctx, const char *fmtString, ... )
{
   char s[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   vsnprintf(s, MAXSTRING, fmtString, args);
   if (ctx)
      (void) ctx->imports.fprintf( (__GLcontext *) ctx, stdout, s );
   else
      printf( s );
   va_end( args );
}


/*
 * Initialize a __GLimports object to point to the functions in
 * this file.  This is to be called from device drivers.
 * Input:  imports - the object to init
 *         driverCtx - pointer to device driver-specific data
 */
void
_mesa_init_default_imports(__GLimports *imports, void *driverCtx)
{
   imports->malloc = _mesa_Malloc;
   imports->calloc = _mesa_Calloc;
   imports->realloc = _mesa_Realloc;
   imports->free = _mesa_Free;
   imports->warning = warning;
   imports->fatal = _mesa_fatal;
   imports->getenv = _mesa_getenv;
   imports->atoi = _mesa_atoi;
   imports->sprintf = _mesa_sprintf;
   imports->fopen = _mesa_fopen;
   imports->fclose = _mesa_fclose;
   imports->fprintf = _mesa_fprintf;
   imports->getDrawablePrivate = _mesa_GetDrawablePrivate;
   imports->other = driverCtx;
}
