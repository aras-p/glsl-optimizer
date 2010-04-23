#include "glthread.h"

unsigned long
_glthread_GetID(void)
{
   return u_thread_self();
}
