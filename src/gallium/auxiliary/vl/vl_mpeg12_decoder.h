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

#ifndef vl_mpeg12_decoder_h
#define vl_mpeg12_decoder_h

#include <pipe/p_video_context.h>

#include "vl_idct.h"
#include "vl_mpeg12_mc_renderer.h"

#include "vl_vertex_buffers.h"
#include "vl_video_buffer.h"

struct pipe_screen;
struct pipe_context;

struct vl_mpeg12_decoder
{
   struct pipe_video_decoder base;
   struct pipe_context *pipe;

   const unsigned (*empty_block_mask)[3][2][2];
   unsigned nr_of_idct_render_targets;

   enum pipe_format idct_source_format;
   enum pipe_format idct_intermediate_format;
   enum pipe_format mc_source_format;

   struct pipe_vertex_buffer quads;
   void *ves_eb[VL_MAX_PLANES];
   void *ves_mv[2];

   struct vl_idct idct_y, idct_c;
   struct vl_mpeg12_mc_renderer mc_y, mc_c;

   void *dsa;
   void *blend;
};

struct vl_mpeg12_buffer
{
   struct pipe_video_decode_buffer base;

   struct vl_vertex_buffer vertex_stream;

   struct pipe_video_buffer *idct_source;
   struct pipe_video_buffer *idct_intermediate;
   struct pipe_video_buffer *mc_source;

   union
   {
      struct pipe_vertex_buffer all[2];
      struct {
         struct pipe_vertex_buffer quad, stream;
      } individual;
   } vertex_bufs;

   struct vl_idct_buffer idct[VL_MAX_PLANES];
   struct vl_mpeg12_mc_buffer mc[VL_MAX_PLANES];

   struct pipe_transfer *tex_transfer[VL_MAX_PLANES];
   short *texels[VL_MAX_PLANES];
};

/* drivers can call this function in their pipe_video_context constructors and pass it
   an accelerated pipe_context along with suitable buffering modes, etc */
struct pipe_video_decoder *
vl_create_mpeg12_decoder(struct pipe_video_context *context,
                         struct pipe_context *pipe,
                         enum pipe_video_profile profile,
                         enum pipe_video_entrypoint entrypoint,
                         enum pipe_video_chroma_format chroma_format,
                         unsigned width, unsigned height);

#endif /* vl_mpeg12_decoder_h */
