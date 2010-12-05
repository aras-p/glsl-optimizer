/**************************************************************************
 *
 * Copyright 2010 Christian König
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

#ifndef vl_idct_h
#define vl_idct_h

#include <pipe/p_state.h>
#include "vl_vertex_buffers.h"

struct vl_idct
{
   struct pipe_context *pipe;

   unsigned max_blocks;

   struct pipe_viewport_state viewport[2];
   struct pipe_framebuffer_state fb_state[2];

   struct pipe_resource *destination;

   void *rs_state;

   void *vertex_elems_state;

   union
   {
      void *all[4];
      void *stage[2][2];
      struct {
         void *matrix, *source;
         void *transpose, *intermediate;
      } individual;
   } samplers;

   union
   {
      struct pipe_sampler_view *all[4];
      struct pipe_sampler_view *stage[2][2];
      struct {
         struct pipe_sampler_view *matrix, *source;
         struct pipe_sampler_view *transpose, *intermediate;
      } individual;
   } sampler_views;

   void *vs;
   void *matrix_fs, *transpose_fs;

   union
   {
      struct pipe_resource *all[4];
      struct pipe_resource *stage[2][2];
      struct {
         struct pipe_resource *matrix, *source;
         struct pipe_resource *transpose, *intermediate;
      } individual;
   } textures;

   union
   {
      struct pipe_vertex_buffer all[2];
      struct { struct pipe_vertex_buffer quad, pos; } individual;
   } vertex_bufs;

   struct vl_vertex_buffer blocks;

   struct pipe_transfer *tex_transfer;
   short *texels;
};

struct pipe_resource *vl_idct_upload_matrix(struct pipe_context *pipe);

bool vl_idct_init(struct vl_idct *idct, struct pipe_context *pipe, struct pipe_resource *dst, struct pipe_resource *matrix);

void vl_idct_cleanup(struct vl_idct *idct);

void vl_idct_add_block(struct vl_idct *idct, unsigned x, unsigned y, short *block);

void vl_idct_flush(struct vl_idct *idct);

#endif
