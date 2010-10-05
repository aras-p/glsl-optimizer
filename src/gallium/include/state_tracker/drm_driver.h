
#ifndef _DRM_DRIVER_H_
#define _DRM_DRIVER_H_

#include "pipe/p_compiler.h"

struct pipe_screen;
struct pipe_winsys;
struct pipe_context;
struct pipe_resource;

#define DRM_API_HANDLE_TYPE_SHARED 0
#define DRM_API_HANDLE_TYPE_KMS    1

/**
 * For use with pipe_screen::{texture_from_handle|texture_get_handle}.
 */
struct winsys_handle
{
   /**
    * Unused for texture_from_handle, always
    * DRM_API_HANDLE_TYPE_SHARED.  Input to texture_get_handle,
    * use TEXTURE_USAGE to select handle for kms or ipc.
    */
   unsigned type;
   /**
    * Input to texture_from_handle.
    * Output for texture_get_handle.
    */
   unsigned handle;
   /**
    * Input to texture_from_handle.
    * Output for texture_get_handle.
    */
   unsigned stride;
};

struct drm_driver_descriptor
{
   /**
    * Identifying sufix/prefix of the binary, used by egl.
    */
   const char *name;

   /**
    * Kernel driver name, as accepted by drmOpenByName.
    */
   const char *driver_name;

   /**
    * Create a pipe srcreen.
    *
    * This function does any wrapping of the screen.
    * For example wrapping trace or rbug debugging drivers around it.
    */
   struct pipe_screen* (*create_screen)(int drm_fd);
};

extern struct drm_driver_descriptor driver_descriptor;

/**
 * Instantiate a drm_driver_descriptor struct.
 */
#define DRM_DRIVER_DESCRIPTOR(name_str, driver_name_str, func) \
struct drm_driver_descriptor driver_descriptor = {             \
   .name = name_str,                                           \
   .driver_name = driver_name_str,                             \
   .create_screen = func,                                      \
};

#endif
