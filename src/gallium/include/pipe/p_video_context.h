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

   void *priv; /**< context private data (for DRI for example) */

   /**
    * destroy context, all objects created from this context
    * (buffers, decoders, compositors etc...) must be freed before calling this
    */
   void (*destroy)(struct pipe_video_context *context);

   /**
    * Query an integer-valued capability/parameter/limit
    * \param param  one of PIPE_CAP_x
    */
   int (*get_param)(struct pipe_video_context *context, int param);

   /**
    * Check if the given pipe_format is supported as a texture or
    * drawing surface.
    */
   boolean (*is_format_supported)(struct pipe_video_context *context,
                                  enum pipe_format format,
                                  unsigned usage);

   /**
    * create a surface of a texture
    */
   struct pipe_surface *(*create_surface)(struct pipe_video_context *context,
                                          struct pipe_resource *resource,
                                          const struct pipe_surface *templ);

   /**
    * sampler view handling, used for subpictures for example
    */
   /*@{*/

   /**
    * create a sampler view of a texture, for subpictures for example
    */
   struct pipe_sampler_view *(*create_sampler_view)(struct pipe_video_context *context,
                                                    struct pipe_resource *resource,
                                                    const struct pipe_sampler_view *templ);

   /**
    * upload image data to a sampler
    */
   void (*upload_sampler)(struct pipe_video_context *context,
                          struct pipe_sampler_view *dst,
                          const struct pipe_box *dst_box,
                          const void *src, unsigned src_stride,
                          unsigned src_x, unsigned src_y);

   /**
    * clear a sampler with a specific rgba color
    */
   void (*clear_sampler)(struct pipe_video_context *context,
                         struct pipe_sampler_view *dst,
                         const struct pipe_box *dst_box,
                         const float *rgba);

   /*}@*/

   /**
    * create a decoder for a specific video profile
    */
   struct pipe_video_decoder *(*create_decoder)(struct pipe_video_context *context,
                                                enum pipe_video_profile profile,
                                                enum pipe_video_entrypoint entrypoint,
                                                enum pipe_video_chroma_format chroma_format,
                                                unsigned width, unsigned height);

   /**
    * Creates a buffer as decoding target
    */
   struct pipe_video_buffer *(*create_buffer)(struct pipe_video_context *context,
                                              enum pipe_format buffer_format,
                                              enum pipe_video_chroma_format chroma_format,
                                              unsigned width, unsigned height);

   /**
    * Creates a video compositor
    */
   struct pipe_video_compositor *(*create_compositor)(struct pipe_video_context *context);
};

/**
 * decoder for a specific video codec
 */
struct pipe_video_decoder
{
   struct pipe_video_context *context;

   enum pipe_video_profile profile;
   enum pipe_video_entrypoint entrypoint;
   enum pipe_video_chroma_format chroma_format;
   unsigned width;
   unsigned height;

   /**
    * destroy this video decoder
    */
   void (*destroy)(struct pipe_video_decoder *decoder);

   /**
    * Creates a buffer as decoding input
    */
   struct pipe_video_decode_buffer *(*create_buffer)(struct pipe_video_decoder *decoder);

   /**
    * flush decoder buffer to video hardware
    */
   void (*flush_buffer)(struct pipe_video_decode_buffer *decbuf,
                        unsigned num_ycbcr_blocks[3],
                        struct pipe_video_buffer *ref_frames[2],
                        struct pipe_video_buffer *dst);
};

/**
 * input buffer for a decoder
 */
struct pipe_video_decode_buffer
{
   struct pipe_video_decoder *decoder;

   /**
    * destroy this decode buffer
    */
   void (*destroy)(struct pipe_video_decode_buffer *decbuf);

   /**
    * map the input buffer into memory before starting decoding
    */
   void (*begin_frame)(struct pipe_video_decode_buffer *decbuf);

   /**
    * get the pointer where to put the ycbcr blocks of a component
    */
   struct pipe_ycbcr_block *(*get_ycbcr_stream)(struct pipe_video_decode_buffer *, int component);

   /**
    * get the pointer where to put the ycbcr dct block data of a component
    */
   short *(*get_ycbcr_buffer)(struct pipe_video_decode_buffer *, int component);

   /**
    * get the stride of the mv buffer
    */
   unsigned (*get_mv_stream_stride)(struct pipe_video_decode_buffer *decbuf);

   /**
    * get the pointer where to put the motion vectors of a ref frame
    */
   struct pipe_motionvector *(*get_mv_stream)(struct pipe_video_decode_buffer *decbuf, int ref_frame);

   /**
    * decode a bitstream
    */
   void (*decode_bitstream)(struct pipe_video_decode_buffer *decbuf,
                            unsigned num_bytes, const void *data,
                            struct pipe_mpeg12_picture_desc *picture,
                            unsigned num_ycbcr_blocks[3]);

   /**
    * unmap decoder buffer before flushing
    */
   void (*end_frame)(struct pipe_video_decode_buffer *decbuf);
};

/**
 * output for decoding / input for displaying
 */
struct pipe_video_buffer
{
   struct pipe_video_context *context;

   enum pipe_format buffer_format;
   enum pipe_video_chroma_format chroma_format;
   unsigned width;
   unsigned height;

   /**
    * destroy this video buffer
    */
   void (*destroy)(struct pipe_video_buffer *buffer);

   /**
    * get a individual sampler view for each plane
    */
   struct pipe_sampler_view **(*get_sampler_view_planes)(struct pipe_video_buffer *buffer);

   /**
    * get a individual sampler view for each component
    */
   struct pipe_sampler_view **(*get_sampler_view_components)(struct pipe_video_buffer *buffer);

   /**
    * get a individual surfaces for each plane
    */
   struct pipe_surface **(*get_surfaces)(struct pipe_video_buffer *buffer);
};

/**
 * composing and displaying of image data
 */
struct pipe_video_compositor
{
   struct pipe_video_context *context;

   /**
    * destroy this compositor
    */
   void (*destroy)(struct pipe_video_compositor *compositor);

   /**
    * set yuv -> rgba conversion matrix
    */
   void (*set_csc_matrix)(struct pipe_video_compositor *compositor, const float mat[16]);

   /**
    * reset dirty area, so it's cleared with the clear colour
    */
   void (*reset_dirty_area)(struct pipe_video_compositor *compositor);

   /**
    * set the clear color
    */
   void (*set_clear_color)(struct pipe_video_compositor *compositor, float color[4]);

   /**
    * set overlay samplers
    */
   /*@{*/

   /**
    * reset all currently set layers
    */
   void (*clear_layers)(struct pipe_video_compositor *compositor);

   /**
    * set a video buffer as a layer to render
    */
   void (*set_buffer_layer)(struct pipe_video_compositor *compositor,
                            unsigned layer,
                            struct pipe_video_buffer *buffer,
                            struct pipe_video_rect *src_rect,
                            struct pipe_video_rect *dst_rect);

   /**
    * set a paletted sampler as a layer to render
    */
   void (*set_palette_layer)(struct pipe_video_compositor *compositor,
                             unsigned layer,
                             struct pipe_sampler_view *indexes,
                             struct pipe_sampler_view *palette,
                             struct pipe_video_rect *src_rect,
                             struct pipe_video_rect *dst_rect);

   /**
    * set a rgba sampler as a layer to render
    */
   void (*set_rgba_layer)(struct pipe_video_compositor *compositor,
                          unsigned layer,
                          struct pipe_sampler_view *rgba,
                          struct pipe_video_rect *src_rect,
                          struct pipe_video_rect *dst_rect);

   /*@}*/

   /**
    * render the layers to the frontbuffer
    */
   void (*render_picture)(struct pipe_video_compositor  *compositor,
                          enum pipe_mpeg12_picture_type picture_type,
                          struct pipe_surface           *dst_surface,
                          struct pipe_video_rect        *dst_area,
                          struct pipe_fence_handle      **fence);

};

#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_CONTEXT_H */
