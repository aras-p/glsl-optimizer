
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

/**
 * \brief  Public interface into the drawing module.
 */

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */


#ifndef DRAW_CONTEXT_H
#define DRAW_CONTEXT_H


#include "pipe/p_state.h"


struct vertex_buffer;
struct vertex_info;
struct draw_context;
struct draw_stage;


/**
 * Clipmask flags
 */
/*@{*/
#define CLIP_RIGHT_BIT   0x01
#define CLIP_LEFT_BIT    0x02
#define CLIP_TOP_BIT     0x04
#define CLIP_BOTTOM_BIT  0x08
#define CLIP_NEAR_BIT    0x10
#define CLIP_FAR_BIT     0x20
#define CLIP_CULL_BIT    (1 << (6 + PIPE_MAX_CLIP_PLANES)) /*unused? */
/*@}*/

/**
 * Bitshift for each clip flag
 */
/*@{*/
#define CLIP_RIGHT_SHIFT 	0
#define CLIP_LEFT_SHIFT 	1
#define CLIP_TOP_SHIFT  	2
#define CLIP_BOTTOM_SHIFT       3
#define CLIP_NEAR_SHIFT  	4
#define CLIP_FAR_SHIFT  	5
/*@}*/


struct draw_context *draw_create( void );

void draw_destroy( struct draw_context *draw );

void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport );

void draw_set_clip_state( struct draw_context *pipe,
                          const struct pipe_clip_state *clip );

void draw_set_feedback_state( struct draw_context *draw,
                              const struct pipe_feedback_state * );

void draw_set_setup_state( struct draw_context *draw,
                           const struct pipe_rasterizer_state *raster );

void draw_set_setup_stage( struct draw_context *draw,
                           struct draw_stage *stage );

unsigned draw_prim_info( unsigned prim, unsigned *first, unsigned *incr );

unsigned draw_trim( unsigned count, unsigned first, unsigned incr );


void
draw_set_vertex_shader(struct draw_context *draw,
                       const struct pipe_shader_state *shader);


void
draw_set_vertex_buffer(struct draw_context *draw,
                       unsigned attr,
                       const struct pipe_vertex_buffer *buffer);

void
draw_set_vertex_element(struct draw_context *draw,
                        unsigned attr,
                        const struct pipe_vertex_element *element);

void draw_set_mapped_element_buffer( struct draw_context *draw,
                                     unsigned eltSize, void *elements );

void draw_set_mapped_vertex_buffer(struct draw_context *draw,
                                   unsigned attr, const void *buffer);

void draw_set_mapped_constant_buffer(struct draw_context *draw,
                                     const void *buffer);

void
draw_set_mapped_feedback_buffer(struct draw_context *draw, uint index,
                                void *buffer, uint size);

void
draw_arrays(struct draw_context *draw, unsigned prim,
            unsigned start, unsigned count);


#endif /* DRAW_CONTEXT_H */
