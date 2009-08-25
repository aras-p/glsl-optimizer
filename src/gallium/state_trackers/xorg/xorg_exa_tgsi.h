#ifndef XORG_EXA_TGSI_H
#define XORG_EXA_TGSI_H

#include "xorg_exa.h"

enum xorg_vs_traits {
   VS_COMPOSITE        = 1 << 0,
   VS_FILL             = 1 << 1
   /*VS_TRANSFORM      = 1 << 2*/
};

enum xorg_fs_traits {
   FS_COMPOSITE        = 1 << 0,
   FS_MASK             = 1 << 1,
   FS_FILL             = 1 << 2,
   FS_LINEAR_GRADIENT  = 1 << 3,
   FS_RADIAL_GRADIENT  = 1 << 4
};

struct xorg_shader {
   void *fs;
   void *vs;
};

struct xorg_shaders;

struct xorg_shaders *xorg_shaders_create(struct exa_context *exa);
void xorg_shaders_destroy(struct xorg_shaders *shaders);

struct xorg_shader xorg_shaders_get(struct xorg_shaders *shaders,
                                    unsigned vs_traits,
                                    unsigned fs_traits);

#endif
