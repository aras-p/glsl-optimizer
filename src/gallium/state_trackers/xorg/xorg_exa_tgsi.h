#ifndef XORG_EXA_TGSI_H
#define XORG_EXA_TGSI_H

#include "xorg_exa.h"

struct xorg_shader {
   void *fs;
   void *vs;
};

struct xorg_shader xorg_shader_construct(struct exa_context *exa,
                                         int op,
                                         PicturePtr src_picture,
                                         PicturePtr mask_picture,
                                         PicturePtr dst_picture);

#endif
