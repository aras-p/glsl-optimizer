/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * This file manages the OpenGL API dispatch layer.
 * The dispatch table (struct _glapi_table) is basically just a list
 * of function pointers.
 * There are functions to set/get the current dispatch table for the
 * current thread and to manage registration/dispatch of dynamically
 * added extension functions.
 *
 * It's intended that this file and the other glapi*.[ch] files are
 * flexible enough to be reused in several places:  XFree86, DRI-
 * based libGL.so, and perhaps the SGI SI.
 *
 * NOTE: There are no dependencies on Mesa in this code.
 *
 * Versions (API changes):
 *   2000/02/23  - original version for Mesa 3.3 and XFree86 4.0
 *   2001/01/16  - added dispatch override feature for Mesa 3.5
 *   2002/06/28  - added _glapi_set_warning_func(), Mesa 4.1.
 *   2002/10/01  - _glapi_get_proc_address() will now generate new entrypoints
 *                 itself (using offset ~0).  _glapi_add_entrypoint() can be
 *                 called afterward and it'll fill in the correct dispatch
 *                 offset.  This allows DRI libGL to avoid probing for DRI
 *                 drivers!  No changes to the public glapi interface.
 */



#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include "glapi/mesa.h"
#else
#include "main/glheader.h"
#include "main/compiler.h"
#endif

#include "glapi/glapi.h"
#include "glapi/glapitable.h"

extern _glapi_proc __glapi_noop_table[];


/**
 * \name Current dispatch and current context control variables
 *
 * Depending on whether or not multithreading is support, and the type of
 * support available, several variables are used to store the current context
 * pointer and the current dispatch table pointer.  In the non-threaded case,
 * the variables \c _glapi_Dispatch and \c _glapi_Context are used for this
 * purpose.
 *
 * In the "normal" threaded case, the variables \c _glapi_Dispatch and
 * \c _glapi_Context will be \c NULL if an application is detected as being
 * multithreaded.  Single-threaded applications will use \c _glapi_Dispatch
 * and \c _glapi_Context just like the case without any threading support.
 * When \c _glapi_Dispatch and \c _glapi_Context are \c NULL, the thread state
 * data \c _gl_DispatchTSD and \c ContextTSD are used.  Drivers and the
 * static dispatch functions access these variables via \c _glapi_get_dispatch
 * and \c _glapi_get_context.
 *
 * There is a race condition in setting \c _glapi_Dispatch to \c NULL.  It is
 * possible for the original thread to be setting it at the same instant a new
 * thread, perhaps running on a different processor, is clearing it.  Because
 * of that, \c ThreadSafe, which can only ever be changed to \c GL_TRUE, is
 * used to determine whether or not the application is multithreaded.
 * 
 * In the TLS case, the variables \c _glapi_Dispatch and \c _glapi_Context are
 * hardcoded to \c NULL.  Instead the TLS variables \c _glapi_tls_Dispatch and
 * \c _glapi_tls_Context are used.  Having \c _glapi_Dispatch and
 * \c _glapi_Context be hardcoded to \c NULL maintains binary compatability
 * between TLS enabled loaders and non-TLS DRI drivers.
 */
/*@{*/
#if defined(GLX_USE_TLS)

PUBLIC __thread struct _glapi_table * _glapi_tls_Dispatch
    __attribute__((tls_model("initial-exec")))
    = (struct _glapi_table *) __glapi_noop_table;

PUBLIC __thread void * _glapi_tls_Context
    __attribute__((tls_model("initial-exec")));

PUBLIC const struct _glapi_table *_glapi_Dispatch = NULL;

PUBLIC const void *_glapi_Context = NULL;

#else

#if defined(THREADS)

static GLboolean ThreadSafe = GL_FALSE;  /**< In thread-safe mode? */

_glthread_TSD _gl_DispatchTSD;           /**< Per-thread dispatch pointer */

static _glthread_TSD ContextTSD;         /**< Per-thread context pointer */

#endif /* defined(THREADS) */

PUBLIC struct _glapi_table *_glapi_Dispatch = (struct _glapi_table *) __glapi_noop_table;

PUBLIC void *_glapi_Context = NULL;

#endif /* defined(GLX_USE_TLS) */
/*@}*/



#if defined(THREADS) && !defined(GLX_USE_TLS)

void
_glapi_init_multithread(void)
{
   _glthread_InitTSD(&_gl_DispatchTSD);
   _glthread_InitTSD(&ContextTSD);
}

void
_glapi_destroy_multithread(void)
{
#ifdef WIN32_THREADS
   _glthread_DestroyTSD(&_gl_DispatchTSD);
   _glthread_DestroyTSD(&ContextTSD);
#endif
}

/**
 * Mutex for multithread check.
 */
#ifdef WIN32_THREADS
/* _glthread_DECLARE_STATIC_MUTEX is broken on windows.  There will be race! */
#define CHECK_MULTITHREAD_LOCK()
#define CHECK_MULTITHREAD_UNLOCK()
#else
_glthread_DECLARE_STATIC_MUTEX(ThreadCheckMutex);
#define CHECK_MULTITHREAD_LOCK() _glthread_LOCK_MUTEX(ThreadCheckMutex)
#define CHECK_MULTITHREAD_UNLOCK() _glthread_UNLOCK_MUTEX(ThreadCheckMutex)
#endif

/**
 * We should call this periodically from a function such as glXMakeCurrent
 * in order to test if multiple threads are being used.
 */
PUBLIC void
_glapi_check_multithread(void)
{
   static unsigned long knownID;
   static GLboolean firstCall = GL_TRUE;

   if (ThreadSafe)
      return;

   CHECK_MULTITHREAD_LOCK();
   if (firstCall) {
      _glapi_init_multithread();

      knownID = _glthread_GetID();
      firstCall = GL_FALSE;
   }
   else if (knownID != _glthread_GetID()) {
      ThreadSafe = GL_TRUE;
      _glapi_set_dispatch(NULL);
      _glapi_set_context(NULL);
   }
   CHECK_MULTITHREAD_UNLOCK();
}

#else

void
_glapi_init_multithread(void) { }

void
_glapi_destroy_multithread(void) { }

PUBLIC void
_glapi_check_multithread(void) { }

#endif



/**
 * Set the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast to
 * void from the real context pointer type.
 */
PUBLIC void
_glapi_set_context(void *context)
{
#if defined(GLX_USE_TLS)
   _glapi_tls_Context = context;
#elif defined(THREADS)
   _glthread_SetTSD(&ContextTSD, context);
   _glapi_Context = (ThreadSafe) ? NULL : context;
#else
   _glapi_Context = context;
#endif
}



/**
 * Get the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast from
 * void to the real context pointer type.
 */
PUBLIC void *
_glapi_get_context(void)
{
#if defined(GLX_USE_TLS)
   return _glapi_tls_Context;
#elif defined(THREADS)
   return (ThreadSafe) ? _glthread_GetTSD(&ContextTSD) : _glapi_Context;
#else
   return _glapi_Context;
#endif
}



/**
 * Set the global or per-thread dispatch table pointer.
 * If the dispatch parameter is NULL we'll plug in the no-op dispatch
 * table (__glapi_noop_table).
 */
PUBLIC void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
   init_glapi_relocs_once();

   if (dispatch == NULL) {
      /* use the no-op functions */
      dispatch = (struct _glapi_table *) __glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table_not_null(dispatch);
      _glapi_check_table(dispatch);
   }
#endif

#if defined(GLX_USE_TLS)
   _glapi_tls_Dispatch = dispatch;
#elif defined(THREADS)
   _glthread_SetTSD(&_gl_DispatchTSD, (void *) dispatch);
   _glapi_Dispatch = (ThreadSafe) ? NULL : dispatch;
#else
   _glapi_Dispatch = dispatch;
#endif
}



/**
 * Return pointer to current dispatch table for calling thread.
 */
PUBLIC struct _glapi_table *
_glapi_get_dispatch(void)
{
#if defined(GLX_USE_TLS)
   return _glapi_tls_Dispatch;
#elif defined(THREADS)
   return (ThreadSafe)
     ? (struct _glapi_table *) _glthread_GetTSD(&_gl_DispatchTSD)
     : _glapi_Dispatch;
#else
   return _glapi_Dispatch;
#endif
}




/*
 * The dispatch table size (number of entries) is the size of the
 * _glapi_table struct plus the number of dynamic entries we can add.
 * The extra slots can be filled in by DRI drivers that register new extension
 * functions.
 */
#define DISPATCH_TABLE_SIZE (sizeof(struct _glapi_table) / sizeof(void *) + MAX_EXTENSION_FUNCS)


/**
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
PUBLIC GLuint
_glapi_get_dispatch_table_size(void)
{
   return DISPATCH_TABLE_SIZE;
}


/**
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intended for debugging purposes.
 */
void
_glapi_check_table_not_null(const struct _glapi_table *table)
{
#if 0 /* enable this for extra DEBUG */
   const GLuint entries = _glapi_get_dispatch_table_size();
   const void **tab = (const void **) table;
   GLuint i;
   for (i = 1; i < entries; i++) {
      assert(tab[i]);
   }
#else
   (void) table;
#endif
}
