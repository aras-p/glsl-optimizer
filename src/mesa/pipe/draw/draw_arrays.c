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
 *    Keith Whitwell
 */


#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_prim.h"


/**
 * Draw vertex arrays
 * This is the main entrypoint into the drawing module.
 * \param prim  one of PIPE_PRIM_x
 * \param start  index of first vertex to draw
 * \param count  number of vertices to draw
 */
void
draw_arrays(struct draw_context *draw, unsigned prim,
            unsigned start, unsigned count)
{
   /* tell drawing pipeline we're beginning drawing */
   draw->pipeline.first->begin( draw->pipeline.first );

   draw_invalidate_vcache( draw );

   draw_set_prim( draw, prim );

   /* drawing done here: */
   draw_prim(draw, start, count);

   /* draw any left-over buffered prims */
   draw_flush(draw);

   /* tell drawing pipeline we're done drawing */
   draw->pipeline.first->end( draw->pipeline.first );
}


static INLINE void
emit_vertex_attr(struct vertex_info *vinfo, uint vfAttr, uint format)
{
   const uint n = vinfo->num_attribs;
   vinfo->attr_mask |= (1 << vfAttr);
   vinfo->slot_to_attrib[n] = vfAttr;
   if (n >= 2) {
      /* the first two slots are the vertex header & clippos */
      vinfo->attrib_to_slot[vfAttr] = n - 2;
   }
   /*printf("Vertex slot %d = vfattrib %d\n", n, vfAttr);*/
   /*vinfo->interp_mode[n] = interpMode;*/
   vinfo->format[n] = format;
   vinfo->num_attribs++;

}


static void
compute_vertex_size(struct vertex_info *vinfo)
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
         vinfo->size += 4;
         break;
      default:
         assert(0);
      }
   }
}


void
draw_set_vertex_attributes( struct draw_context *draw,
                            const unsigned *slot_to_vf_attr,
                            unsigned nr_attrs )
{
   struct vertex_info *vinfo = &draw->vertex_info;
   unsigned i;

   assert(slot_to_vf_attr[0] == VF_ATTRIB_POS);

   memset(vinfo, 0, sizeof(*vinfo));

   /*
    * First three attribs are always the same: header, clip pos, winpos
    */
   emit_vertex_attr(vinfo, VF_ATTRIB_VERTEX_HEADER, FORMAT_1F);
   emit_vertex_attr(vinfo, VF_ATTRIB_CLIP_POS, FORMAT_4F);
   emit_vertex_attr(vinfo, VF_ATTRIB_POS, FORMAT_4F_VIEWPORT);

   /*
    * Remaining attribs (color, texcoords, etc)
    */
   for (i = 1; i < nr_attrs; i++) {
      emit_vertex_attr(vinfo, slot_to_vf_attr[i], FORMAT_4F);
   }

   compute_vertex_size(vinfo);
}
