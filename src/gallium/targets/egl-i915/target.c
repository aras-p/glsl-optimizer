
#include "target-helpers/drm_api_compat.h"

DRM_API_COMPAT_STRUCT("i915", "i915")

/* A poor man's --whole-archive for EGL drivers */
void *_eglMain(void *);
void *_eglWholeArchive = (void *) _eglMain;
