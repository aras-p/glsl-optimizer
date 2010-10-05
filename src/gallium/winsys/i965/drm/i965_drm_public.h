
#ifndef I965_DRM_PUBLIC_H
#define I965_DRM_PUBLIC_H

struct brw_winsys_screen;

struct brw_winsys_screen * i965_drm_winsys_screen_create(int drmFD);

#endif
