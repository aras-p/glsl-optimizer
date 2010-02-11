/**************************************************************************
 * 
 * Copyright 1999-2006 Brian Paul
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * 
 **************************************************************************/


/**
 * @file
 * 
 * Thread, mutex, condition variable, barrier, semaphore and
 * thread-specific data functions.
 */


#ifndef OS_THREAD_H_
#define OS_THREAD_H_


#include "pipe/p_compiler.h"
#include "util/u_debug.h" /* for assert */


#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)

#include <pthread.h> /* POSIX threads headers */
#include <stdio.h> /* for perror() */

#define PIPE_THREAD_HAVE_CONDVAR

/* pipe_thread
 */
typedef pthread_t pipe_thread;

#define PIPE_THREAD_ROUTINE( name, param ) \
   void *name( void *param )

static INLINE pipe_thread pipe_thread_create( void *(* routine)( void *), void *param )
{
   pipe_thread thread;
   if (pthread_create( &thread, NULL, routine, param ))
      return 0;
   return thread;
}

static INLINE int pipe_thread_wait( pipe_thread thread )
{
   return pthread_join( thread, NULL );
}

static INLINE int pipe_thread_destroy( pipe_thread thread )
{
   return pthread_detach( thread );
}


/* pipe_mutex
 */
typedef pthread_mutex_t pipe_mutex;

#define pipe_static_mutex(mutex) \
   static pipe_mutex mutex = PTHREAD_MUTEX_INITIALIZER

#define pipe_mutex_init(mutex) \
   (void) pthread_mutex_init(&(mutex), NULL)

#define pipe_mutex_destroy(mutex) \
   pthread_mutex_destroy(&(mutex))

#define pipe_mutex_lock(mutex) \
   (void) pthread_mutex_lock(&(mutex))

#define pipe_mutex_unlock(mutex) \
   (void) pthread_mutex_unlock(&(mutex))


/* pipe_condvar
 */
typedef pthread_cond_t pipe_condvar;

#define pipe_static_condvar(mutex) \
   static pipe_condvar mutex = PTHREAD_COND_INITIALIZER

#define pipe_condvar_init(cond)	\
   pthread_cond_init(&(cond), NULL)

#define pipe_condvar_destroy(cond) \
   pthread_cond_destroy(&(cond))

#define pipe_condvar_wait(cond, mutex) \
  pthread_cond_wait(&(cond), &(mutex))

#define pipe_condvar_signal(cond) \
  pthread_cond_signal(&(cond))

#define pipe_condvar_broadcast(cond) \
  pthread_cond_broadcast(&(cond))



#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)

#include <windows.h>

/* pipe_thread
 */
typedef HANDLE pipe_thread;

#define PIPE_THREAD_ROUTINE( name, param ) \
   void * WINAPI name( void *param )

static INLINE pipe_thread pipe_thread_create( void *(WINAPI * routine)( void *), void *param )
{
   DWORD id;
   return CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) routine, param, 0, &id );
}

static INLINE int pipe_thread_wait( pipe_thread thread )
{
   if (WaitForSingleObject( thread, INFINITE ) == WAIT_OBJECT_0)
      return 0;
   return -1;
}

static INLINE int pipe_thread_destroy( pipe_thread thread )
{
   if (CloseHandle( thread ))
      return 0;
   return -1;
}


/* pipe_mutex
 */
typedef CRITICAL_SECTION pipe_mutex;

#define pipe_static_mutex(mutex) \
   /*static*/ pipe_mutex mutex = {0,0,0,0,0,0}

#define pipe_mutex_init(mutex) \
   InitializeCriticalSection(&mutex)

#define pipe_mutex_destroy(mutex) \
   DeleteCriticalSection(&mutex)

#define pipe_mutex_lock(mutex) \
   EnterCriticalSection(&mutex)

#define pipe_mutex_unlock(mutex) \
   LeaveCriticalSection(&mutex)


/* pipe_condvar (XXX FIX THIS)
 */
typedef unsigned pipe_condvar;

#define pipe_condvar_init(cond) \
   (void) cond

#define pipe_condvar_destroy(cond) \
   (void) cond

#define pipe_condvar_wait(cond, mutex) \
   (void) cond; (void) mutex

#define pipe_condvar_signal(cond) \
   (void) cond

#define pipe_condvar_broadcast(cond) \
   (void) cond


#else

/** Dummy definitions */

typedef unsigned pipe_thread;

#define PIPE_THREAD_ROUTINE( name, param ) \
   void * name( void *param )

static INLINE pipe_thread pipe_thread_create( void *(* routine)( void *), void *param )
{
   return 0;
}

static INLINE int pipe_thread_wait( pipe_thread thread )
{
   return -1;
}

static INLINE int pipe_thread_destroy( pipe_thread thread )
{
   return -1;
}

typedef unsigned pipe_mutex;
typedef unsigned pipe_condvar;

#define pipe_static_mutex(mutex) \
   static pipe_mutex mutex = 0

#define pipe_mutex_init(mutex) \
   (void) mutex

#define pipe_mutex_destroy(mutex) \
   (void) mutex

#define pipe_mutex_lock(mutex) \
   (void) mutex

#define pipe_mutex_unlock(mutex) \
   (void) mutex

#define pipe_static_condvar(condvar) \
   static unsigned condvar = 0

#define pipe_condvar_init(condvar) \
   (void) condvar

#define pipe_condvar_destroy(condvar) \
   (void) condvar

#define pipe_condvar_wait(condvar, mutex) \
   (void) condvar

#define pipe_condvar_signal(condvar) \
   (void) condvar

#define pipe_condvar_broadcast(condvar) \
   (void) condvar


#endif  /* PIPE_OS_? */


/*
 * pipe_barrier
 */

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_HAIKU)

typedef pthread_barrier_t pipe_barrier;

static INLINE void pipe_barrier_init(pipe_barrier *barrier, unsigned count)
{
   pthread_barrier_init(barrier, NULL, count);
}

static INLINE void pipe_barrier_destroy(pipe_barrier *barrier)
{
   pthread_barrier_destroy(barrier);
}

static INLINE void pipe_barrier_wait(pipe_barrier *barrier)
{
   pthread_barrier_wait(barrier);
}


#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)

/* XXX FIX THIS */
typedef unsigned pipe_barrier;

static INLINE void pipe_barrier_init(pipe_barrier *barrier, unsigned count)
{
   /* XXX we could implement barriers with a mutex and condition var */
}

static INLINE void pipe_barrier_destroy(pipe_barrier *barrier)
{
}

static INLINE void pipe_barrier_wait(pipe_barrier *barrier)
{
   assert(0);
}


#else

typedef unsigned pipe_barrier;

static INLINE void pipe_barrier_init(pipe_barrier *barrier, unsigned count)
{
   /* XXX we could implement barriers with a mutex and condition var */
   assert(0);
}

static INLINE void pipe_barrier_destroy(pipe_barrier *barrier)
{
   assert(0);
}

static INLINE void pipe_barrier_wait(pipe_barrier *barrier)
{
   assert(0);
}


#endif


/*
 * Semaphores
 */

typedef struct
{
   pipe_mutex mutex;
   pipe_condvar cond;
   int counter;
} pipe_semaphore;


static INLINE void
pipe_semaphore_init(pipe_semaphore *sema, int init_val)
{
   pipe_mutex_init(sema->mutex);
   pipe_condvar_init(sema->cond);
   sema->counter = init_val;
}

static INLINE void
pipe_semaphore_destroy(pipe_semaphore *sema)
{
   pipe_mutex_destroy(sema->mutex);
   pipe_condvar_destroy(sema->cond);
}

/** Signal/increment semaphore counter */
static INLINE void
pipe_semaphore_signal(pipe_semaphore *sema)
{
   pipe_mutex_lock(sema->mutex);
   sema->counter++;
   pipe_condvar_signal(sema->cond);
   pipe_mutex_unlock(sema->mutex);
}

/** Wait for semaphore counter to be greater than zero */
static INLINE void
pipe_semaphore_wait(pipe_semaphore *sema)
{
   pipe_mutex_lock(sema->mutex);
   while (sema->counter <= 0) {
      pipe_condvar_wait(sema->cond, sema->mutex);
   }
   sema->counter--;
   pipe_mutex_unlock(sema->mutex);
}



/*
 * Thread-specific data.
 */

typedef struct {
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)
   pthread_key_t key;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   DWORD key;
#endif
   int initMagic;
} pipe_tsd;


#define PIPE_TSD_INIT_MAGIC 0xff8adc98


static INLINE void
pipe_tsd_init(pipe_tsd *tsd)
{
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)
   if (pthread_key_create(&tsd->key, NULL/*free*/) != 0) {
      perror("pthread_key_create(): failed to allocate key for thread specific data");
      exit(-1);
   }
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   assert(0);
#endif
   tsd->initMagic = PIPE_TSD_INIT_MAGIC;
}

static INLINE void *
pipe_tsd_get(pipe_tsd *tsd)
{
   if (tsd->initMagic != (int) PIPE_TSD_INIT_MAGIC) {
      pipe_tsd_init(tsd);
   }
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)
   return pthread_getspecific(tsd->key);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   assert(0);
   return NULL;
#else
   assert(0);
   return NULL;
#endif
}

static INLINE void
pipe_tsd_set(pipe_tsd *tsd, void *value)
{
   if (tsd->initMagic != (int) PIPE_TSD_INIT_MAGIC) {
      pipe_tsd_init(tsd);
   }
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)
   if (pthread_setspecific(tsd->key, value) != 0) {
      perror("pthread_set_specific() failed");
      exit(-1);
   }
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   assert(0);
#else
   assert(0);
#endif
}



#endif /* OS_THREAD_H_ */
