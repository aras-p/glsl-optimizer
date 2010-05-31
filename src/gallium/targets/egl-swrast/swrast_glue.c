#include "state_tracker/drm_api.h"

struct drm_api *
drm_api_create()
{
   return NULL;
}

/* A poor man's --whole-archive for EGL drivers */
void *_eglMain(void *);
void *_eglWholeArchive = (void *) _eglMain;
