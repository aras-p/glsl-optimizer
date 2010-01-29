#include <stdlib.h>
#include <assert.h>
#include "eglglobals.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglmutex.h"


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


static _EGL_DECLARE_MUTEX(_eglGlobalMutex);
struct _egl_global _eglGlobal =
{
   &_eglGlobalMutex,       /* Mutex */
   NULL,                   /* DisplayList */
   1,                      /* FreeScreenHandle */
   0,                      /* NumDrivers */
   { NULL },               /* Drivers */
   2,                      /* NumAtExitCalls */
   {
      /* default AtExitCalls, called in reverse order */
      _eglUnloadDrivers, /* always called last */
      _eglFiniDisplay
   },
};


static void
_eglAtExit(void)
{
   EGLint i;
   for (i = _eglGlobal.NumAtExitCalls - 1; i >= 0; i--)
      _eglGlobal.AtExitCalls[i]();
}


void
_eglAddAtExitCall(void (*func)(void))
{
   if (func) {
      static EGLBoolean registered = EGL_FALSE;

      _eglLockMutex(_eglGlobal.Mutex);

      if (!registered) {
         atexit(_eglAtExit);
         registered = EGL_TRUE;
      }

      assert(_eglGlobal.NumAtExitCalls < ARRAY_SIZE(_eglGlobal.AtExitCalls));
      _eglGlobal.AtExitCalls[_eglGlobal.NumAtExitCalls++] = func;

      _eglUnlockMutex(_eglGlobal.Mutex);
   }
}
