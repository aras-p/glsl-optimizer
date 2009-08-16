/**
 * DRI initialization.  The DRI loaders are defined in src/glx/x11/.
 */

#include <sys/time.h>

#include "glxclient.h"
#include "driinit.h"

/* for __DRI_SYSTEM_TIME extension */
_X_HIDDEN int
__glXGetUST(int64_t * ust)
{
   struct timeval tv;

   if (ust == NULL) {
      return -EFAULT;
   }

   if (gettimeofday(&tv, NULL) == 0) {
      ust[0] = (tv.tv_sec * 1000000) + tv.tv_usec;
      return 0;
   }
   else {
      return -errno;
   }
}

_X_HIDDEN GLboolean
__driGetMscRateOML(__DRIdrawable * draw,
                   int32_t * numerator, int32_t * denominator, void *private)
{
   return GL_FALSE;
}

/* ignore glx extensions */
_X_HIDDEN void
__glXEnableDirectExtension(__GLXscreenConfigs * psc, const char *name)
{
}

_X_HIDDEN __GLXDRIdisplay *
__driCreateDisplay(__GLXdisplayPrivate *dpyPriv, int *version)
{
   __GLXDRIdisplay *driDisplay;
   int ver = 0;

   /* try DRI2 first */
   driDisplay = dri2CreateDisplay(dpyPriv->dpy);
   if (driDisplay) {
      /* fill in the required field */
      dpyPriv->dri2Display = driDisplay;
      ver = 2;
   }
   else {
      /* try DRI */
      driDisplay = driCreateDisplay(dpyPriv->dpy);
      if (driDisplay) {
         dpyPriv->driDisplay = driDisplay;
         ver = 1;
      }
   }

   if (version)
      *version = ver;
   return driDisplay;
}
