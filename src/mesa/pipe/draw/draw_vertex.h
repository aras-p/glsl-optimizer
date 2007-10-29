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

/* Author:
 *    Brian Paul
 */


#ifndef DRAW_VERTEX_H
#define DRAW_VERTEX_H


struct draw_context;


/**
 * Vertex attribute format
 */
enum attrib_format {
   FORMAT_OMIT,
   FORMAT_1F,
   FORMAT_2F,
   FORMAT_3F,
   FORMAT_4F,
   FORMAT_4F_VIEWPORT,
   FORMAT_4UB
};


/**
 * Attribute interpolation mode
 */
enum interp_mode {
   INTERP_NONE,      /**< never interpolate vertex header info */
   INTERP_CONSTANT,
   INTERP_LINEAR,
   INTERP_PERSPECTIVE
};


/**
 * Information about post-transformed vertex layout.
 */
struct vertex_info
{
   uint num_attribs;
   uint hwfmt[4];      /**< hardware format info for this format */
   enum interp_mode interp_mode[PIPE_MAX_SHADER_OUTPUTS];
   enum attrib_format format[PIPE_MAX_SHADER_OUTPUTS];   /**< FORMAT_x */
   uint size;          /**< total vertex size in dwords */
};



/**
 * Add another attribute to the given vertex_info object.
 * \return slot in which the attribute was added
 */
static INLINE uint
draw_emit_vertex_attr(struct vertex_info *vinfo,
                      enum attrib_format format, enum interp_mode interp)
{
   const uint n = vinfo->num_attribs;
   assert(n < PIPE_MAX_SHADER_OUTPUTS);
   vinfo->format[n] = format;
   vinfo->interp_mode[n] = interp;
   vinfo->num_attribs++;
   return n;
}


extern void draw_set_vertex_attributes( struct draw_context *draw,
                                        const uint *attrs,
                                        const enum interp_mode *interps,
                                        unsigned nr_attrs );

extern void draw_set_twoside_attributes(struct draw_context *draw,
                                        uint front0, uint back0,
                                        uint front1, uint back1);

extern void draw_compute_vertex_size(struct vertex_info *vinfo);


#endif /* DRAW_VERTEX_H */
