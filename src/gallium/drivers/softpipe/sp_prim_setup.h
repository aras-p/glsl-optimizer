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


#ifndef SP_PRIM_SETUP_H
#define SP_PRIM_SETUP_H


/**
 * vbuf is a special stage to gather the stream of triangles, lines, points
 * together and reconstruct vertex buffers for hardware upload.
 *
 * First attempt, work in progress.
 * 
 * TODO:
 *    - separate out vertex buffer building and primitive emit, ie >1 draw per vb.
 *    - tell vbuf stage how to build hw vertices directly
 *    - pass vbuf stage a buffer pointer for direct emit to agp/vram.
 *
 *
 *
 * Vertices are just an array of floats, with all the attributes
 * packed.  We currently assume a layout like:
 *
 * attr[0][0..3] - window position
 * attr[1..n][0..3] - remaining attributes.
 *
 * Attributes are assumed to be 4 floats wide but are packed so that
 * all the enabled attributes run contiguously.
 */


struct draw_stage;
struct softpipe_context;


typedef void (*vbuf_draw_func)( struct pipe_context *pipe,
                                unsigned prim,
                                const ushort *elements,
                                unsigned nr_elements,
                                const void *vertex_buffer,
                                unsigned nr_vertices );


extern struct draw_stage *
sp_draw_render_stage( struct softpipe_context *softpipe );

extern struct setup_context *
sp_draw_setup_context( struct draw_stage * );

extern void
sp_draw_flush( struct draw_stage * );


extern struct draw_stage *
sp_draw_vbuf_stage( struct draw_context *draw_context,
                    struct pipe_context *pipe,
                    vbuf_draw_func draw );


#endif /* SP_PRIM_SETUP_H */
