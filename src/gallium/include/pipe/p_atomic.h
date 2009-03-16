/**
 * Many similar implementations exist. See for example libwsbm
 * or the linux kernel include/atomic.h
 *
 * No copyright claimed on this file.
 *
 */

#ifndef P_ATOMIC_H
#define P_ATOMIC_H

#include "p_compiler.h"
#include "p_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(PIPE_CC_GCC))
struct pipe_atomic {
   int32_t count;
};

#define p_atomic_set(_v, _i) ((_v)->count = (_i))
#define p_atomic_read(_v) ((_v)->count)


static INLINE boolean
p_atomic_dec_zero(struct pipe_atomic *v)
{
#ifdef __i386__
   unsigned char c;

   __asm__ __volatile__("lock; decl %0; sete %1":"+m"(v->count), "=qm"(c)
			::"memory");

   return c != 0;
#else  /* __i386__*/
   return (__sync_sub_and_fetch(&v->count, 1) == 0);
#endif /* __i386__*/
}

static INLINE void
p_atomic_inc(struct pipe_atomic *v)
{
#ifdef __i386__
   __asm__ __volatile__("lock; incl %0":"+m"(v->count));
#else /* __i386__*/
   (void) __sync_add_and_fetch(&v->count, 1);
#endif /* __i386__*/
}

static INLINE void
p_atomic_dec(struct pipe_atomic *v)
{
#ifdef __i386__
   __asm__ __volatile__("lock; decl %0":"+m"(v->count));
#else /* __i386__*/
   (void) __sync_sub_and_fetch(&v->count, 1);
#endif /* __i386__*/
}

static INLINE int32_t
p_atomic_cmpxchg(struct pipe_atomic *v, int32_t old, int32_t new)
{
   return __sync_val_compare_and_swap(&v->count, old, new);
}

#else /* (defined(PIPE_CC_GCC)) */

#include "pipe/p_thread.h"

/**
 * This implementation should really not be used.
 * Add an assembly port instead. It may abort and
 * doesn't destroy used mutexes.
 */

struct pipe_atomic {
   pipe_mutex mutex;
   int32_t count;
};

static INLINE void
p_atomic_set(struct pipe_atomic *v, int32_t i)
{
   int ret;
   ret = pipe_mutex_init(v->mutex);
   if (ret)
      abort();
   pipe_mutex_lock(v->mutex);
   v->count = i;
   pipe_mutex_unlock(v->mutex);
}

static INLINE int32_t
p_atomic_read(struct pipe_atomic *v)
{
   int32_t ret;

   pipe_mutex_lock(v->mutex);
   ret = v->count;
   pipe_mutex_unlock(v->mutex);
   return ret;
}

static INLINE void
p_atomic_inc(struct pipe_atomic *v)
{
   pipe_mutex_lock(v->mutex);
   ++v->count;
   pipe_mutex_unlock(v->mutex);
}

static INLINE void
p_atomic_dec(struct pipe_atomic *v)
{
   pipe_mutex_lock(v->mutex);
   --v->count;
   pipe_mutex_unlock(v->mutex);
}

static INLINE boolean
p_atomic_dec_zero(struct pipe_atomic *v)
{
   boolean ret;

   pipe_mutex_lock(v->mutex);
   ret = (--v->count == 0);
   pipe_mutex_unlock(v->mutex);
   return ret;
}

static INLINE int32_t
p_atomic_cmpxchg(struct pipe_atomic *v, int32_t old, int32_t new)
{
   int32_t ret;

   pipe_mutex_lock(v->mutex);
   ret = v->count;
   if (ret == old)
      v->count = new;
   pipe_mutex_unlock(v->mutex);

   return ret;
}

#endif /* (defined(PIPE_CC_GCC)) */

#ifdef __cplusplus
}
#endif

#endif /* P_ATOMIC_H */
