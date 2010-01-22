#include "state_tracker/drm_api.h"

static struct drm_api swrast_drm_api =
{
   .name = "swrast",
};

struct drm_api *
drm_api_create()
{
   (void) swrast_drm_api;
   return NULL;
}
