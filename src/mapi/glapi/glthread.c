/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * XXX There's probably some work to do in order to make this file
 * truly reusable outside of Mesa.  First, the glheader.h include must go.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include "glapi/mesa.h"
#else
#include "main/compiler.h"
#endif

#include "glapi/glthread.h"


/*
 * This file should still compile even when THREADS is not defined.
 * This is to make things easier to deal with on the makefile scene..
 */
#ifdef THREADS
#include <errno.h>

/*
 * Error messages
 */
#define INIT_TSD_ERROR "_glthread_: failed to allocate key for thread specific data"
#define GET_TSD_ERROR "_glthread_: failed to get thread specific data"
#define SET_TSD_ERROR "_glthread_: thread failed to set thread specific data"


/*
 * Magic number to determine if a TSD object has been initialized.
 * Kind of a hack but there doesn't appear to be a better cross-platform
 * solution.
 */
#define INIT_MAGIC 0xff8adc98



/*
 * POSIX Threads -- The best way to go if your platform supports them.
 *                  Solaris >= 2.5 have POSIX threads, IRIX >= 6.4 reportedly
 *                  has them, and many of the free Unixes now have them.
 *                  Be sure to use appropriate -mt or -D_REENTRANT type
 *                  compile flags when building.
 */
#ifdef PTHREADS

PUBLIC unsigned long
_glthread_GetID(void)
{
   return (unsigned long) pthread_self();
}


void
_glthread_InitTSD(_glthread_TSD *tsd)
{
   if (pthread_key_create(&tsd->key, NULL/*free*/) != 0) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
   tsd->initMagic = INIT_MAGIC;
}


void *
_glthread_GetTSD(_glthread_TSD *tsd)
{
   if (tsd->initMagic != (int) INIT_MAGIC) {
      _glthread_InitTSD(tsd);
   }
   return pthread_getspecific(tsd->key);
}


void
_glthread_SetTSD(_glthread_TSD *tsd, void *ptr)
{
   if (tsd->initMagic != (int) INIT_MAGIC) {
      _glthread_InitTSD(tsd);
   }
   if (pthread_setspecific(tsd->key, ptr) != 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   }
}

#endif /* PTHREADS */



/*
 * Win32 Threads.  The only available option for Windows 95/NT.
 * Be sure that you compile using the Multithreaded runtime, otherwise
 * bad things will happen.
 */
#ifdef WIN32_THREADS

static void InsteadOf_exit(int nCode)
{
   DWORD dwErr = GetLastError();
}

PUBLIC unsigned long
_glthread_GetID(void)
{
   return GetCurrentThreadId();
}


void
_glthread_InitTSD(_glthread_TSD *tsd)
{
   tsd->key = TlsAlloc();
   if (tsd->key == TLS_OUT_OF_INDEXES) {
      perror(INIT_TSD_ERROR);
      InsteadOf_exit(-1);
   }
   tsd->initMagic = INIT_MAGIC;
}


void
_glthread_DestroyTSD(_glthread_TSD *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      return;
   }
   TlsFree(tsd->key);
   tsd->initMagic = 0x0;
}


void *
_glthread_GetTSD(_glthread_TSD *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      _glthread_InitTSD(tsd);
   }
   return TlsGetValue(tsd->key);
}


void
_glthread_SetTSD(_glthread_TSD *tsd, void *ptr)
{
   /* the following code assumes that the _glthread_TSD has been initialized
      to zero at creation */
   if (tsd->initMagic != INIT_MAGIC) {
      _glthread_InitTSD(tsd);
   }
   if (TlsSetValue(tsd->key, ptr) == 0) {
      perror(SET_TSD_ERROR);
      InsteadOf_exit(-1);
   }
}

#endif /* WIN32_THREADS */

/*
 * BeOS threads
 */
#ifdef BEOS_THREADS

PUBLIC unsigned long
_glthread_GetID(void)
{
   return (unsigned long) find_thread(NULL);
}

void
_glthread_InitTSD(_glthread_TSD *tsd)
{
   tsd->key = tls_allocate();
   tsd->initMagic = INIT_MAGIC;
}

void *
_glthread_GetTSD(_glthread_TSD *tsd)
{
   if (tsd->initMagic != (int) INIT_MAGIC) {
      _glthread_InitTSD(tsd);
   }
   return tls_get(tsd->key);
}

void
_glthread_SetTSD(_glthread_TSD *tsd, void *ptr)
{
   if (tsd->initMagic != (int) INIT_MAGIC) {
      _glthread_InitTSD(tsd);
   }
   tls_set(tsd->key, ptr);
}

#endif /* BEOS_THREADS */



#else  /* THREADS */


/*
 * no-op functions
 */

PUBLIC unsigned long
_glthread_GetID(void)
{
   return 0;
}


void
_glthread_InitTSD(_glthread_TSD *tsd)
{
   (void) tsd;
}


void *
_glthread_GetTSD(_glthread_TSD *tsd)
{
   (void) tsd;
   return NULL;
}


void
_glthread_SetTSD(_glthread_TSD *tsd, void *ptr)
{
   (void) tsd;
   (void) ptr;
}


#endif /* THREADS */
