
#include "state_tracker/drm_driver.h"

DRM_DRIVER_DESCRIPTOR("swrast", NULL, NULL)

/* A poor man's --whole-archive for EGL drivers */
void *_eglMain(void *);
void *_eglWholeArchive = (void *) _eglMain;
