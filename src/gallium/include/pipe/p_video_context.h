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

/* XXX: Move to an appropriate place */
#define PIPE_CAP_DECODE_TARGET_PREFERRED_FORMAT 256

struct pipe_screen;
struct pipe_buffer;
struct pipe_surface;
struct pipe_macroblock;
struct pipe_picture_desc;
struct pipe_fence_handle;
struct pipe_video_buffer;

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

   /**
    * Query an integer-valued capability/parameter/limit
    * \param param  one of PIPE_CAP_x
    */
   int (*get_param)(struct pipe_video_context *vpipe, int param);

   /**
    * Check if the given pipe_format is supported as a texture or
    * drawing surface.
    */
   boolean (*is_format_supported)(struct pipe_video_context *vpipe,
                                  enum pipe_format format,
                                  unsigned usage);

   /**
    * destroy context, all buffers must be freed before calling this
    */
   void (*destroy)(struct pipe_video_context *vpipe);

   /**
    * create a surface of a texture
    */
   struct pipe_surface *(*create_surface)(struct pipe_video_context *vpipe,
                                          struct pipe_resource *resource,
                                          const struct pipe_surface *templ);

   /**
    * sampler view handling, used for subpictures for example
    */
   /*@{*/

   /**
    * create a sampler view of a texture, for subpictures for example
    */
   struct pipe_sampler_view *(*create_sampler_view)(struct pipe_video_context *vpipe,
                                                    struct pipe_resource *resource,
                                                    const struct pipe_sampler_view *templ);

   /**
    * upload image data to a sampler
    */
   void (*upload_sampler)(struct pipe_video_context *vpipe,
                          struct pipe_sampler_view *dst,
                          const struct pipe_box *dst_box,
                          const void *src, unsigned src_stride,
                          unsigned src_x, unsigned src_y);

   /**
    * clear a sampler with a specific rgba color
    */
   void (*clear_sampler)(struct pipe_video_context *vpipe,
                         struct pipe_sampler_view *dst,
                         const struct pipe_box *dst_box,
                         const float *rgba);
   /**
    * Creates a buffer as decoding target
    */
   struct pipe_video_buffer *(*create_buffer)(struct pipe_video_context *vpipe);

   /**
    * Picture decoding and displaying
    */

#if 0
   void (*decode_bitstream)(struct pipe_video_context *vpipe,
                            unsigned num_bufs,
                            struct pipe_buffer **bitstream_buf);
#endif

   /**
    * render a video buffer to the frontbuffer
    */
   void (*render_picture)(struct pipe_video_context     *vpipe,
                          struct pipe_video_buffer      *src_surface,
                          struct pipe_video_rect        *src_area,
                          enum pipe_mpeg12_picture_type picture_type,
                          struct pipe_surface           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          struct pipe_fence_handle      **fence);

   /*@}*/

   /**
    * Parameter-like states (or properties)
    */
   /*@{*/

   /**
    * set overlay samplers
    */
   void (*set_picture_layers)(struct pipe_video_context *vpipe,
                              struct pipe_sampler_view *layers[],
                              struct pipe_video_rect *src_rects[],
                              struct pipe_video_rect *dst_rects[],
                              unsigned num_layers);

   void (*set_csc_matrix)(struct pipe_video_context *vpipe, const float *mat);

   /* TODO: Interface for scaling modes, post-processing, etc. */
   /*@}*/
};

struct pipe_video_buffer
{
   struct pipe_video_context* context;

   /**
    * destroy this video buffer
    */
   void (*destroy)(struct pipe_video_buffer *buffer);

   /**
    * map the buffer into memory before calling add_macroblocks
    */
   void (*map)(struct pipe_video_buffer *buffer);

   /**
    * add macroblocks to buffer for decoding
    */
   void (*add_macroblocks)(struct pipe_video_buffer *buffer,
                           unsigned num_macroblocks,
                           struct pipe_macroblock *macroblocks);

   /**
    * unmap buffer before flushing
    */
   void (*unmap)(struct pipe_video_buffer *buffer);

   /**
    * flush buffer to video hardware
    */
   void (*flush)(struct pipe_video_buffer *buffer,
                 struct pipe_video_buffer *ref_frames[2],
                 struct pipe_fence_handle **fence);

};

#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_CONTEXT_H */
