/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation streams (the
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

#ifndef TR_STATE_H
#define TR_STATE_H

#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"


void trace_dump_format(struct trace_stream *stream, 
                       enum pipe_format format);

void trace_dump_block(struct trace_stream *stream, 
                      const struct pipe_format_block *block);

void trace_dump_template(struct trace_stream *stream, 
                         const struct pipe_texture *templat);


void trace_dump_rasterizer_state(struct trace_stream *stream, 
                                 const struct pipe_rasterizer_state *state);

void trace_dump_poly_stipple(struct trace_stream *stream,
                             const struct pipe_poly_stipple *state);

void trace_dump_viewport_state(struct trace_stream *stream,
                               const struct pipe_viewport_state *state);

void trace_dump_scissor_state(struct trace_stream *stream,
                              const struct pipe_scissor_state *state);

void trace_dump_clip_state(struct trace_stream *stream,
                           const struct pipe_clip_state *state);

void trace_dump_constant_buffer(struct trace_stream *stream,
                                const struct pipe_constant_buffer *state);

void trace_dump_token(struct trace_stream *stream,
                      const struct tgsi_token *token);

void trace_dump_shader_state(struct trace_stream *stream,
                             const struct pipe_shader_state *state);

void trace_dump_depth_stencil_alpha_state(struct trace_stream *stream,
                                          const struct pipe_depth_stencil_alpha_state *state);

void trace_dump_blend_state(struct trace_stream *stream,
                            const struct pipe_blend_state *state);

void trace_dump_blend_color(struct trace_stream *stream,
                            const struct pipe_blend_color *state);

void trace_dump_framebuffer_state(struct trace_stream *stream,
                                  const struct pipe_framebuffer_state *state);

void trace_dump_sampler_state(struct trace_stream *stream,
                              const struct pipe_sampler_state *state);

void trace_dump_surface(struct trace_stream *stream,
                        const struct pipe_surface *state);

void trace_dump_vertex_buffer(struct trace_stream *stream,
                              const struct pipe_vertex_buffer *state);

void trace_dump_vertex_element(struct trace_stream *stream,
                               const struct pipe_vertex_element *state);


#endif /* TR_STATE_H */
