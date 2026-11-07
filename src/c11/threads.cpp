/* glsl-optimizer change: implementation of the minimal c11/threads.h. */
#include "c11/threads.h"
#include <mutex>

// never destroyed: Mesa code may still lock it from exit handlers
static std::recursive_mutex&
global_mutex()
{
   static std::recursive_mutex* m = new std::recursive_mutex;
   return *m;
}

extern "C" void
glslopt_global_lock(void)
{
   global_mutex().lock();
}

extern "C" void
glslopt_global_unlock(void)
{
   global_mutex().unlock();
}

extern "C" void
call_once(once_flag *flag, void (*func)(void))
{
   std::lock_guard<std::recursive_mutex> lock(global_mutex());
   if (!flag->done) {
      func();
      flag->done = 1;
   }
}
