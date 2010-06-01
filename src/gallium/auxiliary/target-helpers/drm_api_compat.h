/*
 * This file contain a small backwards compatible shim between
 * the old depricated drm_api and the drm_driver interface.
 */

#ifndef DRM_API_COMPAT_H
#define DRM_API_COMPAT_H

#ifdef _DRM_API_H_
#error "Included drm_api.h before drm_api_compat.h"
#endif

#include "state_tracker/drm_driver.h"

/*
 * XXX Hack, can't include both drm_api and drm_driver. Due to name
 * collition of winsys_handle, just use a define to rename it.
 */
#define winsys_handle HACK_winsys_handle
#include "state_tracker/drm_api.h"
#undef winsys_handle

static INLINE struct pipe_screen *
drm_api_compat_create_screen(int fd)
{
   static struct drm_api *api;
   if (!api)
      api = drm_api_create();

   if (!api)
      return NULL;

   return api->create_screen(api, fd);
}

/**
 * Instanciate a drm_driver descriptor.
 */
#define DRM_API_COMPAT_STRUCT(name_str, driver_name_str) \
struct drm_driver_descriptor driver_descriptor = {       \
   .name = name_str,                                     \
   .driver_name = driver_name_str,                       \
   .create_screen = drm_api_compat_create_screen,        \
};

#endif
