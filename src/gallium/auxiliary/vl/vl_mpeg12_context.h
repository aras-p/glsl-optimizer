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

#ifndef VL_MPEG12_CONTEXT_H
#define VL_MPEG12_CONTEXT_H

#include <pipe/p_video_context.h>
#include "vl_idct.h"
#include "vl_mpeg12_mc_renderer.h"
#include "vl_compositor.h"
#include "vl_ycbcr_buffer.h"

struct pipe_screen;
struct pipe_context;

struct vl_mpeg12_context
{
   struct pipe_video_context base;
   struct pipe_context *pipe;
   enum pipe_format decode_format;
   bool pot_buffers;
   unsigned buffer_width, buffer_height;

   const unsigned (*empty_block_mask)[3][2][2];

   struct pipe_vertex_buffer quads;
   unsigned vertex_buffer_size;
   void *vertex_elems_state;

   struct vl_idct idct_y, idct_cr, idct_cb;
   struct vl_mpeg12_mc_renderer mc_renderer;
   struct vl_compositor compositor;

   void *rast;
   void *dsa;
   void *blend;
};

struct vl_mpeg12_buffer
{
   struct pipe_video_buffer base;

   struct vl_ycbcr_buffer idct_source;
   struct vl_ycbcr_buffer idct_2_mc;

   struct pipe_surface *surface;
   struct pipe_sampler_view *sampler_view;

   struct vl_vertex_buffer vertex_stream;

   union
   {
      struct pipe_vertex_buffer all[2];
      struct {
         struct pipe_vertex_buffer quad, stream;
      } individual;
   } vertex_bufs;

   struct vl_idct_buffer idct_y, idct_cb, idct_cr;

   struct vl_mpeg12_mc_buffer mc;
};

/* drivers can call this function in their pipe_video_context constructors and pass it
   an accelerated pipe_context along with suitable buffering modes, etc */
struct pipe_video_context *
vl_create_mpeg12_context(struct pipe_context *pipe,
                         enum pipe_video_profile profile,
                         enum pipe_video_chroma_format chroma_format,
                         unsigned width, unsigned height,
                         bool pot_buffers,
                         enum pipe_format decode_format);

#endif /* VL_MPEG12_CONTEXT_H */
