/* $Id: imports.c,v 1.30 2003/01/19 15:27:38 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
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
 * specialized, but that'll be rare.
 *
 * Eventually, I want to move roll the glheader.h file into this.
 *
 * The OpenGL SI's __GLimports structure allows per-context specification of
 * replacements for the standard C lib functions.  In practice that's probably
 * never needed; compile-time replacements are far more likely.
 *
 * The _mesa_foo() functions defined here don't in general take a context
 * parameter.  I guess we can change that someday, if need be.
 * So for now, the __GLimports stuff really isn't used.
 */


#include "glheader.h"
#include "mtypes.h"
#include "context.h"
#include "imports.h"


#define MAXSTRING 4000  /* for vsnprintf() */

#ifdef WIN32
#define vsnprintf _vsnprintf
#elif defined(__IBMC__) || defined(__IBMCPP__) || defined(VMS)
extern int vsnprintf(char *str, size_t count, const char *fmt, va_list arg);
#endif


/**********************************************************************/
/* Wrappers for standard C library functions                          */
/**********************************************************************/

/*
 * Functions still needed:
 * scanf
 * qsort
 * bsearch
 * rand and RAND_MAX
 */

void *
_mesa_malloc(size_t bytes)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86malloc(bytes);
#else
   return malloc(bytes);
#endif
}


void *
_mesa_calloc(size_t bytes)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86calloc(1, bytes);
#else
   return calloc(1, bytes);
#endif
}


void
_mesa_free(void *ptr)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86free(ptr);
#else
   free(ptr);
#endif
}


void *
_mesa_align_malloc(size_t bytes, unsigned long alignment)
{
   unsigned long ptr, buf;

   ASSERT( alignment > 0 );

   /* Allocate extra memory to accomodate rounding up the address for
    * alignment and to record the real malloc address.
    */
   ptr = (unsigned long) _mesa_malloc(bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (ptr + alignment + sizeof(void *)) & ~(unsigned long)(alignment - 1);
   *(unsigned long *)(buf - sizeof(void *)) = ptr;

#ifdef DEBUG
   /* mark the non-aligned area */
   while ( ptr < buf - sizeof(void *) ) {
      *(unsigned long *)ptr = 0xcdcdcdcd;
      ptr += sizeof(unsigned long);
   }
#endif

   return (void *) buf;
}


void *
_mesa_align_calloc(size_t bytes, unsigned long alignment)
{
   unsigned long ptr, buf;

   ASSERT( alignment > 0 );

   ptr = (unsigned long) _mesa_calloc(bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (ptr + alignment + sizeof(void *)) & ~(unsigned long)(alignment - 1);
   *(unsigned long *)(buf - sizeof(void *)) = ptr;

#ifdef DEBUG
   /* mark the non-aligned area */
   while ( ptr < buf - sizeof(void *) ) {
      *(unsigned long *)ptr = 0xcdcdcdcd;
      ptr += sizeof(unsigned long);
   }
#endif

   return (void *)buf;
}


void
_mesa_align_free(void *ptr)
{
#if 0
   _mesa_free( (void *)(*(unsigned long *)((unsigned long)ptr - sizeof(void *))) );
#else
   /* The actuall address to free is stuffed in the word immediately
    * before the address the client sees.
    */
   void **cubbyHole = (void **) ((char *) ptr - sizeof(void *));
   void *realAddr = *cubbyHole;
   _mesa_free(realAddr);
#endif
}


void *
_mesa_memcpy(void *dest, const void *src, size_t n)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86memcpy(dest, src, n);
#elif defined(SUNOS4)
   return memcpy((char *) dest, (char *) src, (int) n);
#else
   return memcpy(dest, src, n);
#endif
}


void
_mesa_memset( void *dst, int val, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86memset( dst, val, n );
#elif defined(SUNOS4)
   memset( (char *) dst, (int) val, (int) n );
#else
   memset(dst, val, n);
#endif
}


void
_mesa_memset16( unsigned short *dst, unsigned short val, size_t n )
{
   while (n-- > 0)
      *dst++ = val;
}


void
_mesa_bzero( void *dst, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86memset( dst, 0, n );
#elif defined(__FreeBSD__)
   bzero( dst, n );
#else
   memset( dst, 0, n );
#endif
}


double
_mesa_sin(double a)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86sin(a);
#else
   return sin(a);
#endif
}


double
_mesa_cos(double a)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86cos(a);
#else
   return cos(a);
#endif
}


double
_mesa_sqrt(double x)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86sqrt(x);
#else
   return sqrt(x);
#endif
}


double
_mesa_pow(double x, double y)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86pow(x, y);
#else
   return pow(x, y);
#endif
}


char *
_mesa_getenv( const char *var )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86getenv(var);
#else
   return getenv(var);
#endif
}


char *
_mesa_strstr( const char *haystack, const char *needle )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strstr(haystack, needle);
#else
   return strstr(haystack, needle);
#endif
}


char *
_mesa_strncat( char *dest, const char *src, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strncat(dest, src, n);
#else
   return strncat(dest, src, n);
#endif
}


char *
_mesa_strcpy( char *dest, const char *src )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strcpy(dest, src);
#else
   return strcpy(dest, src);
#endif
}


char *
_mesa_strncpy( char *dest, const char *src, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strncpy(dest, src, n);
#else
   return strncpy(dest, src, n);
#endif
}


size_t
_mesa_strlen( const char *s )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strlen(s);
#else
   return strlen(s);
#endif
}


int
_mesa_strcmp( const char *s1, const char *s2 )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strcmp(s1, s2);
#else
   return strcmp(s1, s2);
#endif
}


int
_mesa_strncmp( const char *s1, const char *s2, size_t n )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strncmp(s1, s2, n);
#else
   return strncmp(s1, s2, n);
#endif
}


char *
_mesa_strdup( const char *s )
{
   int l = _mesa_strlen(s);
   char *s2 = (char *) _mesa_malloc(l + 1);
   if (s2)
      _mesa_strcpy(s2, s);
   return s2;
}


int
_mesa_atoi(const char *s)
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86atoi(s);
#else
   return atoi(s);
#endif
}


float
_mesa_strtof( const char *s, char **end )
{
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86strtof(s, end);
#else
   return (float) strtod(s, end);
#endif
}


int
_mesa_sprintf( char *str, const char *fmt, ... )
{
   int r;
   va_list args;
   va_start( args, fmt );  
   va_end( args );
#if defined(XFree86LOADER) && defined(IN_MODULE)
   r = xf86vsprintf( str, fmt, args );
#else
   r = vsprintf( str, fmt, args );
#endif
   return r;
}


void
_mesa_printf( const char *fmtString, ... )
{
   char s[MAXSTRING];
   va_list args;
   va_start( args, fmtString );  
   vsnprintf(s, MAXSTRING, fmtString, args);
   va_end( args );
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86printf("%s", s);
#else
   printf("%s", s);
#endif
}


void
_mesa_warning( GLcontext *ctx, const char *fmtString, ... )
{
   GLboolean debug;
   char str[MAXSTRING];
   va_list args;
   (void) ctx;
   va_start( args, fmtString );  
   (void) vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );
#ifdef DEBUG
   debug = GL_TRUE; /* always print warning */
#else
   debug = _mesa_getenv("MESA_DEBUG") ? GL_TRUE : GL_FALSE;
#endif
   if (debug) {
#if defined(XFree86LOADER) && defined(IN_MODULE)
      xf86fprintf(stderr, "Mesa warning: %s\n", str);
#else
      fprintf(stderr, "Mesa warning: %s\n", str);
#endif
   }
}


/*
 * This function is called when the Mesa user has stumbled into a code
 * path which may not be implemented fully or correctly.
 */
void
_mesa_problem( const GLcontext *ctx, const char *fmtString, ... )
{
   va_list args;
   char str[MAXSTRING];
   (void) ctx;

   va_start( args, fmtString );  
   vsnprintf( str, MAXSTRING, fmtString, args );
   va_end( args );

#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86fprintf(stderr, "Mesa implementation error: %s\n", str);
   xf86fprintf(stderr, "Please report to the DRI project at dri.sourceforge.net\n");
#else
   fprintf(stderr, "Mesa implementation error: %s\n", str);
   fprintf(stderr, "Please report to the Mesa bug database at www.mesa3d.org\n" );
#endif
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

   debugEnv = _mesa_getenv("MESA_DEBUG");

#ifdef DEBUG
   if (debugEnv && _mesa_strstr(debugEnv, "silent"))
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
   va_end(args);
#if defined(XFree86LOADER) && defined(IN_MODULE)
   xf86fprintf(stderr, "Mesa: %s", s);
#else
   fprintf(stderr, "Mesa: %s", s);
#endif
}



/**********************************************************************/
/* Default Imports Wrapper                                            */
/**********************************************************************/

static void *
default_malloc(__GLcontext *gc, size_t size)
{
   (void) gc;
   return _mesa_malloc(size);
}

static void *
default_calloc(__GLcontext *gc, size_t numElem, size_t elemSize)
{
   (void) gc;
   return _mesa_calloc(numElem * elemSize);
}

static void *
default_realloc(__GLcontext *gc, void *oldAddr, size_t newSize)
{
   (void) gc;
#if defined(XFree86LOADER) && defined(IN_MODULE)
   return xf86realloc(oldAddr, newSize);
#else
   return realloc(oldAddr, newSize);
#endif
}

static void
default_free(__GLcontext *gc, void *addr)
{
   (void) gc;
   _mesa_free(addr);
}

static char * CAPI
default_getenv( __GLcontext *gc, const char *var )
{
   (void) gc;
   return _mesa_getenv(var);
}

static void
default_warning(__GLcontext *gc, char *str)
{
   _mesa_warning(gc, str);
}

static void
default_fatal(__GLcontext *gc, char *str)
{
   _mesa_problem(gc, str);
   abort();
}

static int CAPI
default_atoi(__GLcontext *gc, const char *str)
{
   (void) gc;
   return atoi(str);
}

static int CAPI
default_sprintf(__GLcontext *gc, char *str, const char *fmt, ...)
{
   int r;
   va_list args;
   va_start( args, fmt );  
   r = vsprintf( str, fmt, args );
   va_end( args );
   return r;
}

static void * CAPI
default_fopen(__GLcontext *gc, const char *path, const char *mode)
{
   return fopen(path, mode);
}

static int CAPI
default_fclose(__GLcontext *gc, void *stream)
{
   return fclose((FILE *) stream);
}

static int CAPI
default_fprintf(__GLcontext *gc, void *stream, const char *fmt, ...)
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
default_GetDrawablePrivate(__GLcontext *gc)
{
   return NULL;
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
   imports->malloc = default_malloc;
   imports->calloc = default_calloc;
   imports->realloc = default_realloc;
   imports->free = default_free;
   imports->warning = default_warning;
   imports->fatal = default_fatal;
   imports->getenv = default_getenv; /* not used for now */
   imports->atoi = default_atoi;
   imports->sprintf = default_sprintf;
   imports->fopen = default_fopen;
   imports->fclose = default_fclose;
   imports->fprintf = default_fprintf;
   imports->getDrawablePrivate = default_GetDrawablePrivate;
   imports->other = driverCtx;
}
