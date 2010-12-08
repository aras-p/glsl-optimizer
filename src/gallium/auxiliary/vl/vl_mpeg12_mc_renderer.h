/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef vl_mpeg12_mc_renderer_h
#define vl_mpeg12_mc_renderer_h

#include <pipe/p_compiler.h>
#include <pipe/p_state.h>
#include <pipe/p_video_state.h>
#include "vl_types.h"
#include "vl_idct.h"
#include "vl_vertex_buffers.h"

struct pipe_context;
struct pipe_macroblock;
struct keymap;

/* A slice is video-width (rounded up to a multiple of macroblock width) x macroblock height */
enum VL_MPEG12_MC_RENDERER_BUFFER_MODE
{
   VL_MPEG12_MC_RENDERER_BUFFER_SLICE,  /* Saves memory at the cost of smaller batches */
   VL_MPEG12_MC_RENDERER_BUFFER_PICTURE /* Larger batches, more memory */
};

enum VL_MACROBLOCK_TYPE
{
   VL_MACROBLOCK_TYPE_INTRA,
   VL_MACROBLOCK_TYPE_FWD_FRAME_PRED,
   VL_MACROBLOCK_TYPE_FWD_FIELD_PRED,
   VL_MACROBLOCK_TYPE_BKWD_FRAME_PRED,
   VL_MACROBLOCK_TYPE_BKWD_FIELD_PRED,
   VL_MACROBLOCK_TYPE_BI_FRAME_PRED,
   VL_MACROBLOCK_TYPE_BI_FIELD_PRED,

   VL_NUM_MACROBLOCK_TYPES
};

struct vl_mc_mbtype_handler
{
   void *vs, *fs;

   struct vl_vertex_buffer pos;
   struct vl_vertex_buffer mv[4];
};

struct vl_mpeg12_mc_renderer
{
   struct pipe_context *pipe;
   unsigned buffer_width;
   unsigned buffer_height;
   enum pipe_video_chroma_format chroma_format;
   enum VL_MPEG12_MC_RENDERER_BUFFER_MODE bufmode;
   unsigned macroblocks_per_batch;

   struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state fb_state;

   struct vl_idct idct_luma, idct_chroma;
   struct vl_idct_buffer idct_y, idct_cb, idct_cr;

   void *vertex_elems_state;

   void *rs_state;

   union
   {
      void *all[5];
      struct { void *y, *cb, *cr, *ref[2]; } individual;
   } samplers;

   union
   {
      struct pipe_sampler_view *all[5];
      struct { struct pipe_sampler_view *y, *cb, *cr, *ref[2]; } individual;
   } sampler_views;

   struct vl_mc_mbtype_handler mbtype_handlers[VL_NUM_MACROBLOCK_TYPES];

   union
   {
      struct pipe_resource *all[5];
      struct { struct pipe_resource *y, *cb, *cr, *ref[2]; } individual;
   } textures;

   union
   {
      struct pipe_vertex_buffer all[6];
      struct {
         struct pipe_vertex_buffer quad, pos, mv[4];
      } individual;
   } vertex_bufs;

   struct pipe_surface *surface, *past, *future;
   struct pipe_fence_handle **fence;
   unsigned num_macroblocks;

   struct keymap *texview_map;
};

bool vl_mpeg12_mc_renderer_init(struct vl_mpeg12_mc_renderer *renderer,
                                struct pipe_context *pipe,
                                unsigned picture_width,
                                unsigned picture_height,
                                enum pipe_video_chroma_format chroma_format,
                                enum VL_MPEG12_MC_RENDERER_BUFFER_MODE bufmode);

void vl_mpeg12_mc_renderer_cleanup(struct vl_mpeg12_mc_renderer *renderer);

void vl_mpeg12_mc_renderer_render_macroblocks(struct vl_mpeg12_mc_renderer *renderer,
                                              struct pipe_surface *surface,
                                              struct pipe_surface *past,
                                              struct pipe_surface *future,
                                              unsigned num_macroblocks,
                                              struct pipe_mpeg12_macroblock *mpeg12_macroblocks,
                                              struct pipe_fence_handle **fence);

void vl_mpeg12_mc_renderer_flush(struct vl_mpeg12_mc_renderer *renderer);

#endif /* vl_mpeg12_mc_renderer_h */
