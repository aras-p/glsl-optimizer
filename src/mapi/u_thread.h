/*
 * Mesa 3-D graphics library
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Thread support for gl dispatch.
 *
 * Initial version by John Stone (j.stone@acm.org) (johns@cs.umr.edu)
 *                and Christoph Poliwoda (poliwoda@volumegraphics.com)
 * Revised by Keith Whitwell
 * Adapted for new gl dispatcher by Brian Paul
 * Modified for use in mapi by Chia-I Wu
 */

/*
 * If this file is accidentally included by a non-threaded build,
 * it should not cause the build to fail, or otherwise cause problems.
 * In general, it should only be included when needed however.
 */

#ifndef _U_THREAD_H_
#define _U_THREAD_H_

#include <stdio.h>
#include <stdlib.h>
#include "u_compiler.h"

#include "c11/threads.h"

#if defined(HAVE_PTHREAD) || defined(_WIN32)
#ifndef THREADS
#define THREADS
#endif
#endif

/*
 * Error messages
 */
#define INIT_TSD_ERROR "Mesa: failed to allocate key for thread specific data"
#define GET_TSD_ERROR "Mesa: failed to get thread specific data"
#define SET_TSD_ERROR "Mesa: thread failed to set thread specific data"


/*
 * Magic number to determine if a TSD object has been initialized.
 * Kind of a hack but there doesn't appear to be a better cross-platform
 * solution.
 */
#define INIT_MAGIC 0xff8adc98

#ifdef __cplusplus
extern "C" {
#endif


struct u_tsd {
   tss_t key;
   unsigned initMagic;
};


static INLINE unsigned long
u_thread_self(void)
{
   /*
    * XXX: Callers of u_thread_self assume it is a lightweight function,
    * returning a numeric value.  But unfortunately C11's thrd_current() gives
    * no such guarantees.  In fact, it's pretty hard to have a compliant
    * implementation of thrd_current() on Windows with such characteristics.
    * So for now, we side-step this mess and use Windows thread primitives
    * directly here.
    *
    * FIXME: On the other hand, u_thread_self() is a bad
    * abstraction.  Even with pthreads, there is no guarantee that
    * pthread_self() will return numeric IDs -- we should be using
    * pthread_equal() instead of assuming we can compare thread ids...
    */
#ifdef _WIN32
   return GetCurrentThreadId();
#else
   return (unsigned long) (uintptr_t) thrd_current();
#endif
}


static INLINE void
u_tsd_init(struct u_tsd *tsd)
{
   if (tss_create(&tsd->key, NULL/*free*/) != 0) {
      perror(INIT_TSD_ERROR);
      exit(-1);
   }
   tsd->initMagic = INIT_MAGIC;
}


static INLINE void *
u_tsd_get(struct u_tsd *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   return tss_get(tsd->key);
}


static INLINE void
u_tsd_set(struct u_tsd *tsd, void *ptr)
{
   if (tsd->initMagic != INIT_MAGIC) {
      u_tsd_init(tsd);
   }
   if (tss_set(tsd->key, ptr) != 0) {
      perror(SET_TSD_ERROR);
      exit(-1);
   }
}


static INLINE void
u_tsd_destroy(struct u_tsd *tsd)
{
   if (tsd->initMagic != INIT_MAGIC) {
      return;
   }
   tss_delete(tsd->key);
   tsd->initMagic = 0x0;
}


#ifdef __cplusplus
}
#endif

#endif /* _U_THREAD_H_ */
