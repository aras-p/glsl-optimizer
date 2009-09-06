#ifndef XORG_EXA_TGSI_H
#define XORG_EXA_TGSI_H

#include "xorg_exa.h"

enum xorg_vs_traits {
   VS_COMPOSITE        = 1 << 0,
   VS_MASK             = 1 << 1,
   VS_SOLID_FILL       = 1 << 2,
   VS_LINGRAD_FILL     = 1 << 3,
   VS_RADGRAD_FILL     = 1 << 4,
   VS_FILL             = (VS_SOLID_FILL |
                          VS_LINGRAD_FILL |
                          VS_RADGRAD_FILL)
   /*VS_TRANSFORM      = 1 << 5*/
};

enum xorg_fs_traits {
   FS_COMPOSITE        = 1 << 0,
   FS_MASK             = 1 << 1,
   FS_SOLID_FILL       = 1 << 2,
   FS_LINGRAD_FILL     = 1 << 3,
   FS_RADGRAD_FILL     = 1 << 4,
   FS_FILL             = (FS_SOLID_FILL |
                          FS_LINGRAD_FILL |
                          FS_RADGRAD_FILL)
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
