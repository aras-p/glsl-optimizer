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

struct pipe_context;
struct pipe_video_surface;
struct pipe_macroblock;

/* A slice is video-width (rounded up to a multiple of macroblock width) x macroblock height */
enum VL_MPEG12_MC_RENDERER_BUFFER_MODE
{
   VL_MPEG12_MC_RENDERER_BUFFER_SLICE,  /* Saves memory at the cost of smaller batches */
   VL_MPEG12_MC_RENDERER_BUFFER_PICTURE /* Larger batches, more memory */
};

enum VL_MPEG12_MC_RENDERER_EMPTY_BLOCK
{
   VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ALL, /* Waste of memory bandwidth */
   VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_ONE, /* Can only do point-filtering when interpolating subsampled chroma channels */
   VL_MPEG12_MC_RENDERER_EMPTY_BLOCK_XFER_NONE /* Needs conditional texel fetch! */
};

struct vl_mpeg12_mc_renderer
{
   struct pipe_context *pipe;
   unsigned picture_width;
   unsigned picture_height;
   enum pipe_video_chroma_format chroma_format;
   enum VL_MPEG12_MC_RENDERER_BUFFER_MODE bufmode;
   enum VL_MPEG12_MC_RENDERER_EMPTY_BLOCK eb_handling;
   bool pot_buffers;
   unsigned macroblocks_per_batch;

   struct pipe_viewport_state viewport;
   struct pipe_scissor_state scissor;
   struct pipe_buffer *vs_const_buf;
   struct pipe_buffer *fs_const_buf;
   struct pipe_framebuffer_state fb_state;
   struct pipe_vertex_element vertex_elems[8];
	
   union
   {
      void *all[5];
      struct { void *y, *cb, *cr, *ref[2]; } individual;
   } samplers;
	
   void *i_vs, *p_vs[2], *b_vs[2];
   void *i_fs, *p_fs[2], *b_fs[2];
	
   union
   {
      struct pipe_texture *all[5];
      struct { struct pipe_texture *y, *cb, *cr, *ref[2]; } individual;
   } textures;

   union
   {
      struct pipe_vertex_buffer all[3];
      struct { struct pipe_vertex_buffer ycbcr, ref[2]; } individual;
   } vertex_bufs;
	
   struct pipe_texture *surface, *past, *future;
   struct pipe_fence_handle **fence;
   unsigned num_macroblocks;
   struct pipe_mpeg12_macroblock *macroblock_buf;
   struct pipe_transfer *tex_transfer[3];
   short *texels[3];
   struct { float x, y; } surface_tex_inv_size;
   struct { float x, y; } zero_block[3];
};

bool vl_mpeg12_mc_renderer_init(struct vl_mpeg12_mc_renderer *renderer,
                                struct pipe_context *pipe,
                                unsigned picture_width,
                                unsigned picture_height,
                                enum pipe_video_chroma_format chroma_format,
                                enum VL_MPEG12_MC_RENDERER_BUFFER_MODE bufmode,
                                enum VL_MPEG12_MC_RENDERER_EMPTY_BLOCK eb_handling,
                                bool pot_buffers);

void vl_mpeg12_mc_renderer_cleanup(struct vl_mpeg12_mc_renderer *renderer);

void vl_mpeg12_mc_renderer_render_macroblocks(struct vl_mpeg12_mc_renderer *renderer,
                                              struct pipe_texture *surface,
                                              struct pipe_texture *past,
                                              struct pipe_texture *future,
                                              unsigned num_macroblocks,
                                              struct pipe_mpeg12_macroblock *mpeg12_macroblocks,
                                              struct pipe_fence_handle **fence);

#endif /* vl_mpeg12_mc_renderer_h */
