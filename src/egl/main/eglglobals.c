#include <stdlib.h>
#include "eglglobals.h"
#include "egldisplay.h"
#include "egllog.h"

struct _egl_global _eglGlobal =
{
   1,                      /* FreeScreenHandle */
   0x0,                    /* ClientAPIsMask */
   { 0x0 },                /* ClientAPIs */
   0,                      /* NumDrivers */
   { NULL },               /* Drivers */
};
