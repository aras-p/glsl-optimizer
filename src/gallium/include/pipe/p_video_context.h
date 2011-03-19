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

   void (*destroy)(struct pipe_video_context *vpipe);

   struct pipe_surface *(*create_surface)(struct pipe_video_context *vpipe,
                                          struct pipe_resource *resource,
                                          const struct pipe_surface *templat);

   /**
    * Picture decoding and displaying
    */
   /*@{*/
   void (*decode_bitstream)(struct pipe_video_context *vpipe,
                            unsigned num_bufs,
                            struct pipe_buffer **bitstream_buf);

   void (*decode_macroblocks)(struct pipe_video_context *vpipe,
                              struct pipe_surface *past,
                              struct pipe_surface *future,
                              unsigned num_macroblocks,
                              struct pipe_macroblock *macroblocks,
                              struct pipe_fence_handle **fence);

   void (*render_picture)(struct pipe_video_context     *vpipe,
                          struct pipe_surface           *src_surface,
                          enum pipe_mpeg12_picture_type picture_type,
                          /*unsigned                    num_past_surfaces,
                          struct pipe_surface           *past_surfaces,
                          unsigned                      num_future_surfaces,
                          struct pipe_surface           *future_surfaces,*/
                          struct pipe_video_rect        *src_area,
                          struct pipe_surface           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          struct pipe_fence_handle      **fence);

   void (*clear_render_target)(struct pipe_video_context *vpipe,
                               struct pipe_surface *dst,
                               unsigned dstx, unsigned dsty,
                               const float *rgba,
                               unsigned width, unsigned height);

   void (*resource_copy_region)(struct pipe_video_context *vpipe,
                                struct pipe_resource *dst,
                                unsigned dstx, unsigned dsty, unsigned dstz,
                                struct pipe_resource *src,
                                unsigned srcx, unsigned srcy, unsigned srcz,
                                unsigned width, unsigned height);

   struct pipe_transfer *(*get_transfer)(struct pipe_video_context *vpipe,
                                         struct pipe_resource *resource,
                                         unsigned level,
                                         unsigned usage,  /* a combination of PIPE_TRANSFER_x */
                                         const struct pipe_box *box);

   void (*transfer_destroy)(struct pipe_video_context *vpipe,
                            struct pipe_transfer *transfer);

   void* (*transfer_map)(struct pipe_video_context *vpipe,
                         struct pipe_transfer *transfer);

   void (*transfer_flush_region)(struct pipe_video_context *vpipe,
                                 struct pipe_transfer *transfer,
                                 const struct pipe_box *box);

   void (*transfer_unmap)(struct pipe_video_context *vpipe,
                          struct pipe_transfer *transfer);

   void (*transfer_inline_write)(struct pipe_video_context *vpipe,
                                 struct pipe_resource *resource,
                                 unsigned level,
                                 unsigned usage, /* a combination of PIPE_TRANSFER_x */
                                 const struct pipe_box *box,
                                 const void *data,
                                 unsigned stride,
                                 unsigned slice_stride);

   /*@}*/

   /**
    * Parameter-like states (or properties)
    */
   /*@{*/
   void (*set_picture_background)(struct pipe_video_context *vpipe,
                                  struct pipe_surface *bg,
                                  struct pipe_video_rect *bg_src_rect);

   void (*set_picture_layers)(struct pipe_video_context *vpipe,
                              struct pipe_surface *layers[],
                              struct pipe_video_rect *src_rects[],
                              struct pipe_video_rect *dst_rects[],
                              unsigned num_layers);

   void (*set_picture_desc)(struct pipe_video_context *vpipe,
                            const struct pipe_picture_desc *desc);

   void (*set_decode_target)(struct pipe_video_context *vpipe,
                             struct pipe_surface *dt);

   void (*set_csc_matrix)(struct pipe_video_context *vpipe, const float *mat);

   /* TODO: Interface for scaling modes, post-processing, etc. */
   /*@}*/
};


#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_CONTEXT_H */
