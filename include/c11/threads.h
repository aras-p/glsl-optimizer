/*
 * C11 <threads.h> emulation library
 *
 * (C) Copyright yohhoy 2012.
 * Distributed under the Boost Software License, Version 1.0.
 * (See copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef EMULATED_THREADS_H_INCLUDED_
#define EMULATED_THREADS_H_INCLUDED_

#include <time.h>

#ifndef TIME_UTC
#define TIME_UTC 1
#endif

#include "c99_compat.h" /* for `inline` */

/*---------------------------- types ----------------------------*/
typedef void (*tss_dtor_t)(void*);
typedef int (*thrd_start_t)(void*);

struct xtime {
    time_t sec;
    long nsec;
};
typedef struct xtime xtime;


/*-------------------- enumeration constants --------------------*/
enum {
    mtx_plain     = 0,
    mtx_try       = 1,
    mtx_timed     = 2,
    mtx_recursive = 4
};

enum {
    thrd_success = 0, // succeeded
    thrd_timeout,     // timeout
    thrd_error,       // failed
    thrd_busy,        // resource busy
    thrd_nomem        // out of memory
};

/*-------------------------- functions --------------------------*/

#if defined(_WIN32) && !defined(__CYGWIN__)
#include "threads_win32.h"
#elif defined(HAVE_PTHREAD)
#include "threads_posix.h"
#else
#error Not supported on this platform.
#endif



#endif /* EMULATED_THREADS_H_INCLUDED_ */
