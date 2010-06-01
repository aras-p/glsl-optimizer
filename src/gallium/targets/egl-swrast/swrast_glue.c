
#include "state_tracker/drm_driver.h"

struct drm_driver_descriptor drm_driver = {
   .name = "swrast";
   .driver_name = NULL;
   .create_screen = NULL;
};

/* A poor man's --whole-archive for EGL drivers */
void *_eglMain(void *);
void *_eglWholeArchive = (void *) _eglMain;
