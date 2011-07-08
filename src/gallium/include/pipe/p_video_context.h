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

   /**
    * destroy context, all objects created from this context
    * (buffers, decoders, compositors etc...) must be freed before calling this
    */
   void (*destroy)(struct pipe_video_context *context);

   /**
    * create a decoder for a specific video profile
    */
   struct pipe_video_decoder *(*create_decoder)(struct pipe_video_context *context,
                                                enum pipe_video_profile profile,
                                                enum pipe_video_entrypoint entrypoint,
                                                enum pipe_video_chroma_format chroma_format,
                                                unsigned width, unsigned height);

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
    * set the quantification matrixes
    */
   void (*set_quant_matrix)(struct pipe_video_decode_buffer *decbuf,
                            const uint8_t intra_matrix[64],
                            const uint8_t non_intra_matrix[64]);

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
   struct pipe_context *context;

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

#ifdef __cplusplus
}
#endif

#endif /* PIPE_VIDEO_CONTEXT_H */
