/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef SP_STATE_H
#define SP_STATE_H

#include "glheader.h"
#include "pipe/p_state.h"


void softpipe_set_framebuffer_state( struct pipe_context *,
			     const struct pipe_framebuffer_state * );

void softpipe_set_alpha_test_state( struct pipe_context *,
                                    const struct pipe_alpha_test_state * );

void softpipe_set_blend_state( struct pipe_context *,
                               const struct pipe_blend_state * );

void softpipe_set_clear_color_state( struct pipe_context *,
                                     const struct pipe_clear_color_state * );

void softpipe_set_clip_state( struct pipe_context *,
			     const struct pipe_clip_state * );

void softpipe_set_depth_test_state( struct pipe_context *,
                                    const struct pipe_depth_state * );

void softpipe_set_viewport_state( struct pipe_context *,
                                  const struct pipe_viewport_state * );

void softpipe_set_setup_state( struct pipe_context *,
			      const struct pipe_setup_state * );

void softpipe_set_sampler_state( struct pipe_context *,
                                 GLuint unit,
                                 const struct pipe_sampler_state * );

void softpipe_set_texture_state( struct pipe_context *,
                                 GLuint unit,
                                 struct pipe_texture_object * );

void softpipe_set_scissor_state( struct pipe_context *,
                                 const struct pipe_scissor_state * );

void softpipe_set_fs_state( struct pipe_context *,
			   const struct pipe_fs_state * );

void softpipe_set_polygon_stipple( struct pipe_context *,
				  const struct pipe_poly_stipple * );

void softpipe_update_derived( struct softpipe_context *softpipe );

#endif
