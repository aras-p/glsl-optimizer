#ifndef INLINE_DRM_HELPER_H
#define INLINE_DRM_HELPER_H

#include "state_tracker/drm_driver.h"
#include "target-helpers/inline_debug_helper.h"
#include "loader.h"
#if defined(DRI_TARGET)
#include "dri_screen.h"
#endif

#if GALLIUM_SOFTPIPE
#include "target-helpers/inline_sw_helper.h"
#include "sw/kms-dri/kms_dri_sw_winsys.h"
#endif

#if GALLIUM_I915
#include "i915/drm/i915_drm_public.h"
#include "i915/i915_public.h"
#endif

#if GALLIUM_ILO
#include "intel/drm/intel_drm_public.h"
#include "ilo/ilo_public.h"
#endif

#if GALLIUM_NOUVEAU
#include "nouveau/drm/nouveau_drm_public.h"
#endif

#if GALLIUM_R300
#include "radeon/drm/radeon_winsys.h"
#include "radeon/drm/radeon_drm_public.h"
#include "r300/r300_public.h"
#endif

#if GALLIUM_R600
#include "radeon/drm/radeon_winsys.h"
#include "radeon/drm/radeon_drm_public.h"
#include "r600/r600_public.h"
#endif

#if GALLIUM_RADEONSI
#include "radeon/drm/radeon_winsys.h"
#include "radeon/drm/radeon_drm_public.h"
#include "radeonsi/si_public.h"
#endif

#if GALLIUM_VMWGFX
#include "svga/drm/svga_drm_public.h"
#include "svga/svga_public.h"
#endif

#if GALLIUM_FREEDRENO
#include "freedreno/drm/freedreno_drm_public.h"
#endif

#if GALLIUM_VC4
#include "vc4/drm/vc4_drm_public.h"
#endif

static char* driver_name = NULL;

/* XXX: We need to teardown the winsys if *screen_create() fails. */

#if defined(GALLIUM_SOFTPIPE)
#if defined(DRI_TARGET)
#if defined(HAVE_LIBDRM)

const __DRIextension **__driDriverGetExtensions_kms_swrast(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_kms_swrast(void)
{
   globalDriverAPI = &dri_kms_driver_api;
   return galliumdrm_driver_extensions;
}

struct pipe_screen *
kms_swrast_create_screen(int fd)
{
   struct sw_winsys *sws;
   struct pipe_screen *screen;

   sws = kms_dri_create_winsys(fd);
   if (!sws)
      return NULL;

   screen = sw_screen_create(sws);
   return screen ? debug_screen_wrap(screen) : NULL;
}
#endif
#endif
#endif

#if defined(GALLIUM_I915)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_i915(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_i915(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_i915_create_screen(int fd)
{
   struct i915_winsys *iws;
   struct pipe_screen *screen;

   iws = i915_drm_winsys_create(fd);
   if (!iws)
      return NULL;

   screen = i915_screen_create(iws);
   return screen ? debug_screen_wrap(screen) : NULL;
}
#endif

#if defined(GALLIUM_ILO)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_i965(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_i965(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_ilo_create_screen(int fd)
{
   struct intel_winsys *iws;
   struct pipe_screen *screen;

   iws = intel_winsys_create_for_fd(fd);
   if (!iws)
      return NULL;

   screen = ilo_screen_create(iws);
   return screen ? debug_screen_wrap(screen) : NULL;
}
#endif

#if defined(GALLIUM_NOUVEAU)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_nouveau(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_nouveau(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_nouveau_create_screen(int fd)
{
   struct pipe_screen *screen;

   screen = nouveau_drm_screen_create(fd);
   return screen ? debug_screen_wrap(screen) : NULL;
}
#endif

#if defined(GALLIUM_R300)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_r300(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_r300(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_r300_create_screen(int fd)
{
   struct radeon_winsys *rw;

   rw = radeon_drm_winsys_create(fd, r300_screen_create);
   return rw ? debug_screen_wrap(rw->screen) : NULL;
}
#endif

#if defined(GALLIUM_R600)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_r600(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_r600(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_r600_create_screen(int fd)
{
   struct radeon_winsys *rw;

   rw = radeon_drm_winsys_create(fd, r600_screen_create);
   return rw ? debug_screen_wrap(rw->screen) : NULL;
}
#endif

#if defined(GALLIUM_RADEONSI)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_radeonsi(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_radeonsi(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_radeonsi_create_screen(int fd)
{
   struct radeon_winsys *rw;

   rw = radeon_drm_winsys_create(fd, radeonsi_screen_create);
   return rw ? debug_screen_wrap(rw->screen) : NULL;
}
#endif

#if defined(GALLIUM_VMWGFX)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_vmwgfx(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_vmwgfx(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_vmwgfx_create_screen(int fd)
{
   struct svga_winsys_screen *sws;
   struct pipe_screen *screen;

   sws = svga_drm_winsys_screen_create(fd);
   if (!sws)
      return NULL;

   screen = svga_screen_create(sws);
   return screen ? debug_screen_wrap(screen) : NULL;
}
#endif

#if defined(GALLIUM_FREEDRENO)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_msm(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_msm(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}

const __DRIextension **__driDriverGetExtensions_kgsl(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_kgsl(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

static struct pipe_screen *
pipe_freedreno_create_screen(int fd)
{
   struct pipe_screen *screen;

   screen = fd_drm_screen_create(fd);
   return screen ? debug_screen_wrap(screen) : NULL;
}
#endif

#if defined(GALLIUM_VC4)
#if defined(DRI_TARGET)

const __DRIextension **__driDriverGetExtensions_vc4(void);

PUBLIC const __DRIextension **__driDriverGetExtensions_vc4(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}

#if defined(USE_VC4_SIMULATOR)
const __DRIextension **__driDriverGetExtensions_i965(void);

/**
 * When building using the simulator (on x86), we advertise ourselves as the
 * i965 driver so that you can just make a directory with a link from
 * i965_dri.so to the built vc4_dri.so, and point LIBGL_DRIVERS_PATH to that
 * on your i965-using host to run the driver under simulation.
 *
 * This is, of course, incompatible with building with the ilo driver, but you
 * shouldn't be building that anyway.
 */
PUBLIC const __DRIextension **__driDriverGetExtensions_i965(void)
{
   globalDriverAPI = &galliumdrm_driver_api;
   return galliumdrm_driver_extensions;
}
#endif

#endif

static struct pipe_screen *
pipe_vc4_create_screen(int fd)
{
   struct pipe_screen *screen;

   screen = vc4_drm_screen_create(fd);
   return screen ? debug_screen_wrap(screen) : NULL;
}
#endif

inline struct pipe_screen *
dd_create_screen(int fd)
{
   driver_name = loader_get_driver_for_fd(fd, _LOADER_GALLIUM);
   if (!driver_name)
      return NULL;

#if defined(GALLIUM_I915)
   if (strcmp(driver_name, "i915") == 0)
      return pipe_i915_create_screen(fd);
   else
#endif
#if defined(GALLIUM_ILO)
   if (strcmp(driver_name, "i965") == 0)
      return pipe_ilo_create_screen(fd);
   else
#endif
#if defined(GALLIUM_NOUVEAU)
   if (strcmp(driver_name, "nouveau") == 0)
      return pipe_nouveau_create_screen(fd);
   else
#endif
#if defined(GALLIUM_R300)
   if (strcmp(driver_name, "r300") == 0)
      return pipe_r300_create_screen(fd);
   else
#endif
#if defined(GALLIUM_R600)
   if (strcmp(driver_name, "r600") == 0)
      return pipe_r600_create_screen(fd);
   else
#endif
#if defined(GALLIUM_RADEONSI)
   if (strcmp(driver_name, "radeonsi") == 0)
      return pipe_radeonsi_create_screen(fd);
   else
#endif
#if defined(GALLIUM_VMWGFX)
   if (strcmp(driver_name, "vmwgfx") == 0)
      return pipe_vmwgfx_create_screen(fd);
   else
#endif
#if defined(GALLIUM_FREEDRENO)
   if ((strcmp(driver_name, "kgsl") == 0) || (strcmp(driver_name, "msm") == 0))
      return pipe_freedreno_create_screen(fd);
   else
#endif
#if defined(GALLIUM_VC4)
   if (strcmp(driver_name, "vc4") == 0)
      return pipe_vc4_create_screen(fd);
   else
#if defined(USE_VC4_SIMULATOR)
   if (strcmp(driver_name, "i965") == 0)
      return pipe_vc4_create_screen(fd);
   else
#endif
#endif
      return NULL;
}

inline const char *
dd_driver_name(void)
{
   return driver_name;
}

static const struct drm_conf_ret throttle_ret = {
   DRM_CONF_INT,
   {2},
};

static const struct drm_conf_ret share_fd_ret = {
   DRM_CONF_BOOL,
   {true},
};

static inline const struct drm_conf_ret *
configuration_query(enum drm_conf conf)
{
   switch (conf) {
   case DRM_CONF_THROTTLE:
      return &throttle_ret;
   case DRM_CONF_SHARE_FD:
      return &share_fd_ret;
   default:
      break;
   }
   return NULL;
}

inline const struct drm_conf_ret *
dd_configuration(enum drm_conf conf)
{
   if (!driver_name)
      return NULL;

#if defined(GALLIUM_I915)
   if (strcmp(driver_name, "i915") == 0)
      return NULL;
   else
#endif
#if defined(GALLIUM_ILO)
   if (strcmp(driver_name, "i965") == 0)
      return configuration_query(conf);
   else
#endif
#if defined(GALLIUM_NOUVEAU)
   if (strcmp(driver_name, "nouveau") == 0)
      return configuration_query(conf);
   else
#endif
#if defined(GALLIUM_R300)
   if (strcmp(driver_name, "r300") == 0)
      return configuration_query(conf);
   else
#endif
#if defined(GALLIUM_R600)
   if (strcmp(driver_name, "r600") == 0)
      return configuration_query(conf);
   else
#endif
#if defined(GALLIUM_RADEONSI)
   if (strcmp(driver_name, "radeonsi") == 0)
      return configuration_query(conf);
   else
#endif
#if defined(GALLIUM_VMWGFX)
   if (strcmp(driver_name, "vmwgfx") == 0)
      return configuration_query(conf);
   else
#endif
#if defined(GALLIUM_FREEDRENO)
   if ((strcmp(driver_name, "kgsl") == 0) || (strcmp(driver_name, "msm") == 0))
      return configuration_query(conf);
   else
#endif
      return NULL;
}
#endif /* INLINE_DRM_HELPER_H */
