#ifndef __NOUVEAU_VIDEO_H__
#define __NOUVEAU_SCREEN_H__

#include "nv17_mpeg.xml.h"
#include "nv31_mpeg.xml.h"

struct nouveau_video_buffer {
   struct pipe_video_buffer base;
   unsigned num_planes;
   struct pipe_resource     *resources[3];
   struct pipe_sampler_view *sampler_view_planes[3];
   struct pipe_sampler_view *sampler_view_components[3];
   struct pipe_surface      *surfaces[3];
};

struct nouveau_decoder {
   struct pipe_video_decoder base;
   struct nouveau_screen *screen;
   struct nouveau_grobj *mpeg;
   struct nouveau_bo *cmd_bo, *data_bo, *fence_bo;

   unsigned *fence_map;
   unsigned fence_seq;

   unsigned ofs;
   unsigned *cmds;

   unsigned *data;
   unsigned data_pos;
   unsigned picture_structure;

   unsigned past, future, current;
   unsigned num_surfaces;
   struct nouveau_video_buffer *surfaces[8];
};

static INLINE void
nouveau_vpe_write(struct nouveau_decoder *dec, unsigned data) {
   dec->cmds[dec->ofs++] = data;
}

#endif
