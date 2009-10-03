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

#ifndef PIPE_VIDEO_CONTEXT_H
#define PIPE_VIDEO_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pipe/p_video_state.h>

struct pipe_screen;
struct pipe_buffer;
struct pipe_surface;
struct pipe_video_surface;
struct pipe_macroblock;
struct pipe_picture_desc;
struct pipe_fence_handle;

/**
 * Gallium video rendering context
 */
struct pipe_video_context
{
   struct pipe_screen *screen;
   enum pipe_video_profile profile;
   enum pipe_video_chroma_format chroma_format;
   unsigned width;
   unsigned height;

   void *priv; /**< context private data (for DRI for example) */

   void (*destroy)(struct pipe_video_context *vpipe);

   /**
    * Picture decoding and displaying
    */
   /*@{*/
   void (*decode_bitstream)(struct pipe_video_context *vpipe,
                            unsigned num_bufs,
                            struct pipe_buffer **bitstream_buf);

   void (*decode_macroblocks)(struct pipe_video_context *vpipe,
                              struct pipe_video_surface *past,
                              struct pipe_video_surface *future,
                              unsigned num_macroblocks,
                              struct pipe_macroblock *macroblocks,
                              struct pipe_fence_handle **fence);

   void (*clear_surface)(struct pipe_video_context *vpipe,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height,
                         unsigned value,
                         struct pipe_surface *surface);

   void (*render_picture)(struct pipe_video_context     *vpipe,
                          /*struct pipe_surface         *backround,
                          struct pipe_video_rect        *backround_area,*/
                          struct pipe_video_surface     *src_surface,
                          enum pipe_mpeg12_picture_type picture_type,
                          /*unsigned                    num_past_surfaces,
                          struct pipe_video_surface     *past_surfaces,
                          unsigned                      num_future_surfaces,
                          struct pipe_video_surface     *future_surfaces,*/
                          struct pipe_video_rect        *src_area,
                          struct pipe_surface           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          /*unsigned                      num_layers,
                          struct pipe_texture           *layers,
                          struct pipe_video_rect        *layer_src_areas,
                          struct pipe_video_rect        *layer_dst_areas,*/
                          struct pipe_fence_handle      **fence);
   /*@}*/

   /**
    * Parameter-like states (or properties)
    */
   /*@{*/
   void (*set_picture_desc)(struct pipe_video_context *vpipe,
                            const struct pipe_picture_desc *desc);

   void (*set_decode_target)(struct pipe_video_context *vpipe,
                             struct pipe_video_surface *dt);

   void (*set_csc_matrix)(struct pipe_video_context *vpipe, const float *mat);

   /* TODO: Interface for scaling modes, post-processing, etc. */
   /*@}*/
};


#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_CONTEXT_H */
