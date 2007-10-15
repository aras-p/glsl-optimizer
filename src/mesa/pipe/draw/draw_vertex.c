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

/*
 * Functions for specifying the post-transformation vertex layout.
 *
 * Author:
 *    Brian Paul
 *    Keith Whitwell
 */


#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_vertex.h"


static INLINE void
emit_vertex_attr(struct vertex_info *vinfo,
                 attrib_format format, interp_mode interp)
{
   const uint n = vinfo->num_attribs;
   vinfo->interp_mode[n] = interp;
   vinfo->format[n] = format;
   vinfo->num_attribs++;
}


/**
 * Compute the size of a vertex, in dwords/floats, to update the
 * vinfo->size field.
 */
void
draw_compute_vertex_size(struct vertex_info *vinfo)
{
   uint i;

   vinfo->size = 0;
   for (i = 0; i < vinfo->num_attribs; i++) {
      switch (vinfo->format[i]) {
      case FORMAT_OMIT:
         break;
      case FORMAT_4UB:
         /* fall-through */
      case FORMAT_1F:
         vinfo->size += 1;
         break;
      case FORMAT_2F:
         vinfo->size += 2;
         break;
      case FORMAT_3F:
         vinfo->size += 3;
         break;
      case FORMAT_4F:
      case FORMAT_4F_VIEWPORT:
         vinfo->size += 4;
         break;
      default:
         assert(0);
      }
   }
}


/**
 * Tell the drawing module about the layout of post-transformation vertices
 */
void
draw_set_vertex_attributes( struct draw_context *draw,
                            const uint *slot_to_vf_attr,
                            const interp_mode *interps,
                            unsigned nr_attrs )
{
   struct vertex_info *vinfo = &draw->vertex_info;
   unsigned i;

#if 0
   assert(slot_to_vf_attr[0] == TGSI_ATTRIB_POS);
#endif

   memset(vinfo, 0, sizeof(*vinfo));

   /*
    * First three attribs are always the same: header, clip pos, winpos
    */
   emit_vertex_attr(vinfo, FORMAT_1F, INTERP_NONE);
   emit_vertex_attr(vinfo, FORMAT_4F, INTERP_LINEAR);
   emit_vertex_attr(vinfo, FORMAT_4F_VIEWPORT, INTERP_LINEAR);

   /*
    * Remaining attribs (color, texcoords, etc)
    */
   for (i = 1; i < nr_attrs; i++) {
      emit_vertex_attr(vinfo, FORMAT_4F, interps[i]);
   }

   draw_compute_vertex_size(vinfo);
}


/**
 * This function is used to tell the draw module about attributes
 * (like colors) that need to be selected based on front/back face
 * orientation.
 *
 * The logic is:
 *    if (polygon is back-facing) {
 *       vertex->attrib[front0] = vertex->attrib[back0];
 *       vertex->attrib[front1] = vertex->attrib[back1];
 *    }
 *
 * \param front0  first attrib to replace if the polygon is back-facing
 * \param back0  first attrib to copy if the polygon is back-facing
 * \param front1  second attrib to replace if the polygon is back-facing
 * \param back1  second attrib to copy if the polygon is back-facing
 *
 * Pass -1 to disable two-sided attributes.
 */
void
draw_set_twoside_attributes(struct draw_context *draw,
                            uint front0, uint back0,
                            uint front1, uint back1)
{
   /* XXX we could alternately pass an array of front/back attribs if there's
    * ever need for more than two.  One could imagine a shader extension
    * that allows arbitrary attributes to be selected based on polygon
    * orientation...
    */
   draw->attrib_front0 = front0;
   draw->attrib_back0 = back0;
   draw->attrib_front1 = front1;
   draw->attrib_back1 = back1;
}
