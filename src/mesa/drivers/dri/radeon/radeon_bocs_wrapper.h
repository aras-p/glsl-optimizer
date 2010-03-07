#ifndef RADEON_CS_WRAPPER_H
#define RADEON_CS_WRAPPER_H

#ifdef HAVE_LIBDRM_RADEON

#include "radeon_bo.h"
#include "radeon_bo_gem.h"
#include "radeon_cs.h"
#include "radeon_cs_gem.h"

#else
#include <stdint.h>

#define RADEON_GEM_DOMAIN_CPU 0x1   // Cached CPU domain
#define RADEON_GEM_DOMAIN_GTT 0x2   // GTT or cache flushed
#define RADEON_GEM_DOMAIN_VRAM 0x4  // VRAM domain

#define RADEON_TILING_MACRO 0x1
#define RADEON_TILING_MICRO 0x2
#define RADEON_TILING_SWAP 0x4

#ifndef RADEON_TILING_SURFACE
#define RADEON_TILING_SURFACE 0x8 /* this object requires a surface
				   * when mapped - i.e. front buffer */
#endif

/* to be used to build locally in mesa with no libdrm bits */
#include "../radeon/radeon_bo_drm.h"
#include "../radeon/radeon_cs_drm.h"

#ifndef DRM_RADEON_GEM_INFO
#define DRM_RADEON_GEM_INFO 0x1c

struct drm_radeon_gem_info {
        uint64_t gart_size;
        uint64_t vram_size;
        uint64_t vram_visible;
};

struct drm_radeon_info {
	uint32_t request;
	uint32_t pad;
	uint32_t value;
};
#endif

#ifndef RADEON_PARAM_DEVICE_ID
#define RADEON_PARAM_DEVICE_ID 16
#endif

#ifndef RADEON_PARAM_NUM_Z_PIPES
#define RADEON_PARAM_NUM_Z_PIPES 17
#endif

#ifndef RADEON_INFO_DEVICE_ID
#define RADEON_INFO_DEVICE_ID 0
#endif
#ifndef RADEON_INFO_NUM_GB_PIPES
#define RADEON_INFO_NUM_GB_PIPES 0
#endif

#ifndef RADEON_INFO_NUM_Z_PIPES
#define RADEON_INFO_NUM_Z_PIPES 0
#endif

#ifndef DRM_RADEON_INFO
#define DRM_RADEON_INFO 0x1
#endif


static inline uint32_t radeon_gem_name_bo(struct radeon_bo *dummy)
{
  return 0;
}

static inline void *radeon_bo_manager_gem_ctor(int fd)
{
  return NULL;
}

static inline void radeon_bo_manager_gem_dtor(void *dummy)
{
}

static inline void *radeon_cs_manager_gem_ctor(int fd)
{
  return NULL;
}

static inline void radeon_cs_manager_gem_dtor(void *dummy)
{
}

static inline void radeon_tracker_print(void *ptr, int io)
{
}
#endif

#include "radeon_bo_legacy.h"
#include "radeon_cs_legacy.h"

#endif
