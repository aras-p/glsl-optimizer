/* $Id: glthread.c,v 1.1 1999/12/16 17:31:06 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * Thread support for gl dispatch.
 *
 * Initial version by John Stone (j.stone@acm.org) (johns@cs.umr.edu)
 *                and Christoph Poliwoda (poliwoda@volumegraphics.com)
 *
 * Revised by Keith Whitwell
 * Adapted for new gl dispatcher by Brian Paul
 */


#ifdef PC_ALL
#include "all.h"
#else
#include "glheader.h"
#endif



/*
 * This file should still compile even when THREADS is not defined.
 * This is to make things easier to deal with on the makefile scene..
 */
#ifdef THREADS
#include <errno.h>
#include "glthread.h" 


/*
 * Error messages
 */
#define INIT_TSD_ERROR "_glthread_: failed to allocate key for thread specific data"
#define GET_TSD_ERROR "_glthread_: failed to get thread specific data"
#define SET_TSD_ERROR "_glthread_: thread failed to set thread specific data"


/*
 * magic number for win32 and solaris threads equivalents of pthread_once
 * This could probably be done better, but we haven't figured out how yet. 
 */
#define INITFUNC_CALLED_MAGIC 0xff8adc98



/*
 * POSIX Threads -- The best way to go if your platform supports them.
 *                  Solaris >= 2.5 have POSIX threads, IRIX >= 6.4 reportedly
 *                  has them, and many of the free Unixes now have them.
 *                  Be sure to use appropriate -mt or -D_REENTRANT type
 *                  compile flags when building.
 */
#ifdef PTHREADS

unsigned long
_glthread_GetID(void)
{
   return (unsigned long) pthread_self();
}


void
_glthread_InitTSD(_glthread_TSD *tsd)
{
   if (pthread_key_create(&tsd->key, free) != 0) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
}


void *
_glthread_GetTSD(_glthread_TSD *tsd)
{
   return pthread_getspecific(tsd->key);
}


void
_glthread_SetTSD(_glthread_TSD *tsd, void *ptr, void (*initfunc)(void))
{
   pthread_once(&tsd->once, initfunc);
   if (pthread_setspecific(tsd->key, ptr) != 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   };
}

#endif /* PTHREADS */



/*
 * Solaris/Unix International Threads -- Use only if POSIX threads 
 *   aren't available on your Unix platform.  Solaris 2.[34] are examples
 *   of platforms where this is the case.  Be sure to use -mt and/or 
 *   -D_REENTRANT when compiling.
 */
#ifdef SOLARIS_THREADS
#define USE_LOCK_FOR_KEY	/* undef this to try a version without
				   lock for the global key... */

unsigned long
_glthread_GetID(void)
{
   abort();   /* XXX not implemented yet */
   return (unsigned long) 0;
}


void
_glthread_InitTSD(_glthread_TSD *tsd)
{
   if ((errno = mutex_init(&tsd->keylock, 0, NULL)) != 0 ||
      (errno = thr_keycreate(&(tsd->key), free)) != 0) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
}


void *
_glthread_GetTSD(_glthread_TSD *tsd)
{
   void* ret;
#ifdef USE_LOCK_FOR_KEY
   mutex_lock(&tsd->keylock);
   thr_getspecific(tsd->key, &ret);
   mutex_unlock(&tsd->keylock); 
#else
   if ((errno = thr_getspecific(tsd->key, &ret)) != 0) {
      perror(GET_TSD_ERROR);
      exit(-1);
   };
#endif
   return ret;
}


void
_glthread_SetTSD(_glthread_TSD *tsd, void *ptr, void (*initfunc)(void))
{
   /* the following code assumes that the _glthread_TSD has been initialized
      to zero at creation */
   fprintf(stderr, "initfuncCalled = %d\n", tsd->initfuncCalled);
   if (tsd->initfuncCalled != INITFUNC_CALLED_MAGIC) {
      initfunc();
      tsd->initfuncCalled = INITFUNC_CALLED_MAGIC;
   }
   if ((errno = thr_setspecific(tsd->key, ptr)) != 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   };
}

#undef USE_LOCK_FOR_KEY
#endif /* SOLARIS_THREADS */



/*
 * Win32 Threads.  The only available option for Windows 95/NT.
 * Be sure that you compile using the Multithreaded runtime, otherwise
 * bad things will happen.
 */  
#ifdef WIN32

unsigned long
_glthread_GetID(void)
{
   abort();   /* XXX not implemented yet */
   return (unsigned long) 0;
}


void
_glthread_InitTSD(_glthread_TSD *tsd)
{
   tsd->key = TlsAlloc();
   if (tsd->key == 0xffffffff) {
      /* Can Windows handle stderr messages for non-console
         applications? Does Windows have perror? */
      /* perror(SET_INIT_ERROR);*/
      exit(-1);
   }
}


void *
_glthread_GetTSD(_glthread_TSD *tsd)
{
   return TlsGetValue(tsd->key);
}


void
_glthread_SetTSD(_glthread_TSD *tsd, void *ptr, void (*initfunc)(void))
{
   /* the following code assumes that the _glthread_TSD has been initialized
      to zero at creation */
   if (tsd->initfuncCalled != INITFUNC_CALLED_MAGIC) {
      initfunc();
      tsd->initfuncCalled = INITFUNC_CALLED_MAGIC;
   }
   if (TlsSetValue(tsd->key, ptr) == 0) {
      /* Can Windows handle stderr messages for non-console
         applications? Does Windows have perror? */
      /* perror(SET_TSD_ERROR);*/
      exit(-1);
   };
}

#endif /* WIN32 */

#endif /* THREADS */


