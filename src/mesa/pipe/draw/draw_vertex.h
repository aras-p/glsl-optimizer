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


/**
 * Vertex attribute emit modes
 */
enum attrib_emit {
   EMIT_OMIT,      /**< don't emit the attribute */
   EMIT_ALL,       /**< emit whole post-xform vertex, w/ header */
   EMIT_1F,
   EMIT_1F_PSIZE,  /**< insert constant point size */
   EMIT_2F,
   EMIT_3F,
   EMIT_4F,
   EMIT_4UB  /**< XXX may need variations for RGBA vs BGRA, etc */
};


/**
 * Attribute interpolation mode
 */
enum interp_mode {
   INTERP_NONE,      /**< never interpolate vertex header info */
   INTERP_POS,       /**< special case for frag position */
   INTERP_CONSTANT,
   INTERP_LINEAR,
   INTERP_PERSPECTIVE
};


/**
 * Information about hardware/rasterization vertex layout.
 */
struct vertex_info
{
   uint num_attribs;
   uint hwfmt[4];      /**< hardware format info for this format */
   enum interp_mode interp_mode[PIPE_MAX_SHADER_OUTPUTS];
   enum attrib_emit emit[PIPE_MAX_SHADER_OUTPUTS];   /**< EMIT_x */
   uint src_index[PIPE_MAX_SHADER_OUTPUTS]; /**< map to post-xform attribs */
   uint size;          /**< total vertex size in dwords */
};



/**
 * Add another attribute to the given vertex_info object.
 * \param src_index  indicates which post-transformed vertex attrib slot
 *                   corresponds to this attribute.
 * \return slot in which the attribute was added
 */
static INLINE uint
draw_emit_vertex_attr(struct vertex_info *vinfo,
                      enum attrib_emit emit, enum interp_mode interp,
                      uint src_index)
{
   const uint n = vinfo->num_attribs;
   assert(n < PIPE_MAX_SHADER_OUTPUTS);
   vinfo->emit[n] = emit;
   vinfo->interp_mode[n] = interp;
   vinfo->src_index[n] = src_index;
   vinfo->num_attribs++;
   return n;
}


extern void draw_compute_vertex_size(struct vertex_info *vinfo);


#endif /* DRAW_VERTEX_H */
