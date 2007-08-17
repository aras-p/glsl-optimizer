
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
struct draw_context;
struct draw_stage;


struct draw_context *draw_create( void );

void draw_destroy( struct draw_context *draw );

void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport );

void draw_set_clip_state( struct draw_context *pipe,
                          const struct pipe_clip_state *clip );

void draw_set_setup_state( struct draw_context *draw,
                           const struct pipe_setup_state *setup );

void draw_set_setup_stage( struct draw_context *draw,
                           struct draw_stage *stage );

void draw_set_vertex_attributes( struct draw_context *draw,
				 const unsigned *attrs,
				 unsigned nr_attrs );

/* XXX temporary */
void draw_set_vertex_attributes2( struct draw_context *draw,
				 const unsigned *attrs,
				 unsigned nr_attrs );

void draw_set_vertex_array_info(struct draw_context *draw,
                                const struct pipe_vertex_buffer *buffers,
                                const struct pipe_vertex_element *elements);

/* XXX temporary */
void draw_vb(struct draw_context *draw,
	     struct vertex_buffer *VB );

void draw_vertices(struct draw_context *draw,
                   unsigned mode,
                   unsigned numVertex, const float *verts,
                   unsigned numAttribs, const unsigned attribs[]);


#endif /* DRAW_CONTEXT_H */
