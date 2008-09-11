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
 * Thread, mutex, condition var and thread-specific data functions.
 */


#ifndef _P_THREAD2_H_
#define _P_THREAD2_H_


#include "pipe/p_compiler.h"


#if defined(PIPE_OS_LINUX)

#include <pthread.h> /* POSIX threads headers */
#include <stdio.h> /* for perror() */

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

typedef pthread_mutex_t pipe_mutex;
typedef pthread_cond_t pipe_condvar;

#define pipe_static_mutex(mutex) \
   static pipe_mutex mutex = PTHREAD_MUTEX_INITIALIZER

#define pipe_mutex_init(mutex) \
   pthread_mutex_init(&(mutex), NULL)

#define pipe_mutex_destroy(mutex) \
   pthread_mutex_destroy(&(mutex))

#define pipe_mutex_lock(mutex) \
   (void) pthread_mutex_lock(&(mutex))

#define pipe_mutex_unlock(mutex) \
   (void) pthread_mutex_unlock(&(mutex))

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

typedef CRITICAL_SECTION pipe_mutex;

#define pipe_static_mutex(name) \
   /*static*/ pipe_mutex name = {0,0,0,0,0,0}

#define pipe_mutex_init(name) \
   InitializeCriticalSection(&name)

#define pipe_mutex_destroy(name) \
   DeleteCriticalSection(&name)

#define pipe_mutex_lock(name) \
   EnterCriticalSection(&name)

#define pipe_mutex_unlock(name) \
   LeaveCriticalSection(&name)

/* XXX: dummy definitions, make it compile */

typedef unsigned pipe_condvar;

#define pipe_condvar_init(condvar) \
   (void) condvar

#define pipe_condvar_broadcast(condvar) \
   (void) condvar

#else

/** Dummy definitions */

typedef unsigned pipe_thread;
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
 * Thread-specific data.
 */

typedef struct {
#if defined(PIPE_OS_LINUX)
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
#if defined(PIPE_OS_LINUX)
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
#if defined(PIPE_OS_LINUX)
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
#if defined(PIPE_OS_LINUX)
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



#endif /* _P_THREAD2_H_ */
